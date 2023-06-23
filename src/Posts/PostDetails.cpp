#include "PostDetails.hpp"

#include "../Forums/Forum.hpp"
#include "../Threads/Thread.hpp"
#include "../Users/User.hpp"
#include "../Utils/Utils.hpp"
#include "Post.hpp"

#include <fmt/format.h>
#include <string>
#include <vector>

#include <userver/clients/dns/component.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/parameter_store.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fixed_array.hpp>

namespace vkpg {

namespace {

class PostDetails final
    : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-post-details";

  PostDetails(const userver::components::ComponentConfig& config,
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
    const auto& id = request.GetPathArg("id");

    if (id.empty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      return userver::formats::json::MakeObject("message", "no id");
    }

    switch (request.GetMethod()) {
      case userver::server::http::HttpMethod::kGet:
        return GetValue(request, std::stoi(id));
      case userver::server::http::HttpMethod::kPost:
        return PostValue(request, std::stoi(id), json);
      default:
        request.SetResponseStatus(
            userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::MakeObject(
            "message",
            fmt::format("Unsupported method {}", request.GetMethodStr()));
    }
  }

 private:
  userver::formats::json::Value GetValue(
      const userver::server::http::HttpRequest& request, int id) const {
    const auto related = request.GetArg("related");

    auto result = Post::SelectById(m_ClusterPG, id);
    if (result.IsEmpty()) {
      return Post::ReturnNotFound(request, id);
    }

    const auto& post =
        result.AsSingleRow<Post::TypePG>(userver::storages::postgres::kRowTag);

    const auto userResult =
        User::SelectByNickname(m_ClusterPG, std::get<2>(post));
    const auto& user = userResult.AsSingleRow<User::TypePG>(
        userver::storages::postgres::kRowTag);

    const auto threadResult =
        Thread::SelectById(m_ClusterPG, std::get<6>(post));
    const auto& thread = threadResult.AsSingleRow<Thread::TypePG>(
        userver::storages::postgres::kRowTag);

    const auto forumResult =
        Forum::SelectBySlug(m_ClusterPG, std::get<5>(post));
    const auto& forum = forumResult.AsSingleRow<Forum::TypePG>(
        userver::storages::postgres::kRowTag);

    userver::formats::json::ValueBuilder builder;
    builder["post"] = Post::MakeJson(post);
    if (related.find("user") != std::string::npos) {
      builder["author"] = User::MakeJson(user);
    }
    if (related.find("thread") != std::string::npos) {
      builder["thread"] = Thread::MakeJson(thread);
    }
    if (related.find("forum") != std::string::npos) {
      builder["forum"] = Forum::MakeJson(forum);
    }

    return builder.ExtractValue();
  }

  // TODO: Check json fields to update?
  userver::formats::json::Value PostValue(
      const userver::server::http::HttpRequest& request, int id,
      const userver::formats::json::Value& json) const {
    const auto message = json["message"].As<std::string>("");

    auto result = Post::SelectById(m_ClusterPG, id);
    if (result.IsEmpty()) {
      return Post::ReturnNotFound(request, id);
    }
    const auto& post =
        result.AsSingleRow<Post::TypePG>(userver::storages::postgres::kRowTag);
    if (message.empty() || (message == std::get<3>(post))) {
      return Post::MakeJson(post);
    }

    m_ClusterPG->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                         "UPDATE tp.posts SET is_edited = TRUE, message = $2 "
                         "WHERE id = $1 ",
                         id, message);

    auto resultNew = Post::SelectById(m_ClusterPG, id);
    const auto& postNew = resultNew.AsSingleRow<Post::TypePG>(
        userver::storages::postgres::kRowTag);
    return Post::MakeJson(postNew);
  }

 private:
  userver::storages::postgres::ClusterPtr m_ClusterPG;
};

}  // namespace

void AppendPostDetails(userver::components::ComponentList& component_list) {
  component_list.Append<PostDetails>();
}

}  // namespace vkpg