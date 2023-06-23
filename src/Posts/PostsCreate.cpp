#include "PostsCreate.hpp"

#include "../Forums/Forum.hpp"
#include "../Threads/Thread.hpp"
#include "../Users/User.hpp"
#include "Post.hpp"

#include <fmt/format.h>
#include <chrono>
#include <string>
#include <vector>

#include <userver/clients/dns/component.hpp>
#include <userver/formats/json.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fixed_array.hpp>

namespace vkpg {

namespace {

class PostsCreate final
    : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-posts-create";

  PostsCreate(const userver::components::ComponentConfig& config,
              const userver::components::ComponentContext& component_context)
      : HttpHandlerJsonBase(config, component_context),
        m_ClusterPG(
            component_context
                .FindComponent<userver::components::Postgres>("postgres-db-1")
                .GetCluster()) {}

  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& json,
      userver::server::request::RequestContext&) const override {
    const auto& slugOrId = request.GetPathArg("slug_or_id");

    if (!json.IsArray() || slugOrId.empty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      return userver::formats::json::MakeObject("message", "bad data");
    }
    const auto threadResult =
        Thread::SelectIdAndForumIdSlugBySlugOrId(m_ClusterPG, slugOrId);
    if (threadResult.IsEmpty()) {
      return Thread::ReturnNotFound(request, slugOrId);
    }
    const auto [threadId, forumId, forumSlug] =
        threadResult.AsSingleRow<std::tuple<int, int, std::string>>(
            userver::storages::postgres::kRowTag);

    userver::storages::postgres::ParameterStore params;
    params.PushBack(threadId);
    params.PushBack(std::chrono::system_clock::now());

    std::string valuesStr = "";

    std::vector<User::TypeWithIdPG> users;
    for (const auto& post : json) {
      const auto& nickname = post["author"].As<std::string>("");
      auto userResult = User::SelectFullByNickname(m_ClusterPG, nickname);
      if (userResult.IsEmpty()) {
        return User::ReturnNotFound(request, nickname);
      }
      const auto& user = userResult.AsSingleRow<User::TypeWithIdPG>(
          userver::storages::postgres::kRowTag);
      users.push_back(user);

      valuesStr +=
          fmt::format("($1, $2, ${}, ${}, ${}),",  //
                      params.Size() + 1, params.Size() + 2, params.Size() + 3);
      params.PushBack(std::get<0>(user));
      params.PushBack(post["parent"].As<int>(0));
      params.PushBack(post["message"].As<std::string>(""));
    }
    if (users.size() == 0) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
      return userver::formats::json::MakeArray();
    }
    valuesStr.pop_back();

    try {
      auto result = m_ClusterPG->Execute(
          userver::storages::postgres::ClusterHostType::kMaster,
          fmt::format("INSERT INTO tp.posts "
                      "(thread_id, created_at, user_id, parent_id, message) "
                      "VALUES {} "
                      "RETURNING id, parent_id, message, is_edited, thread_id, "
                      "TO_CHAR(created_at::TIMESTAMPTZ AT TIME ZONE 'UTC',"
                      "'YYYY-MM-DD HH24:MI:SS.MS') ",
                      valuesStr),
          params);
      for (const auto& user : users) {
        Forum::AddUser(m_ClusterPG, std::get<0>(user), forumId);
      }

      const auto& posts = result.AsSetOf<Post::TypeInsertPG>(
          userver::storages::postgres::kRowTag);

      userver::formats::json::ValueBuilder builder;

      for (int i = 0; i < posts.Size(); ++i) {
        const auto& post = posts[i];
        builder.PushBack(
            Post::MakeJson(post, std::get<1>(users[i]), forumSlug));
      }

      Forum::IncreasePosts(m_ClusterPG, forumId, posts.Size());

      request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
      return builder.ExtractValue();
    } catch (...) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
      return userver::formats::json::MakeObject("message",
                                                "Error with posts parents!");
    }
  }

 private:
  userver::storages::postgres::ClusterPtr m_ClusterPG;
};

}  // namespace

void AppendPostsCreate(userver::components::ComponentList& component_list) {
  component_list.Append<PostsCreate>();
}

}  // namespace vkpg