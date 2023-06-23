#include "PostsGet.hpp"

#include "../Forums/Forum.hpp"
#include "../Threads/Thread.hpp"
#include "../Users/User.hpp"
#include "../Utils/Utils.hpp"
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

class PostsGet final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-posts-get";

  PostsGet(const userver::components::ComponentConfig& config,
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

    int since = Utils::GetSinceInt(request, -1);
    int limit = Utils::GetLimit(request);
    bool desc = Utils::GetDesc(request);
    std::string sort = Utils::GetSort(request);

    auto threadResult =
        Thread::SelectIdAndForumIdSlugBySlugOrId(m_ClusterPG, slugOrId);
    if (threadResult.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
      return Thread::ReturnNotFound(request, slugOrId);
    }
    const auto [threadId, forumId, forumSlug] =
        threadResult.AsSingleRow<std::tuple<int, int, std::string>>(
            userver::storages::postgres::kRowTag);

    userver::storages::postgres::ParameterStore params;
    params.PushBack(threadId);
    if (since != -1) params.PushBack(since);

    if (sort == "parent_tree") {
      std::string topSinceStr = "";
      if (since != -1) {
        topSinceStr = fmt::format(
            "AND path[1] {} (SELECT path[1] FROM tp.posts WHERE id = $2)",
            desc ? "<" : ">");
      }
      std::string topOrderStr =
          fmt::format("ORDER BY id {}", desc ? "DESC" : "ASC");
      params.PushBack(limit);
      std::string topLimitStr = fmt::format("LIMIT ${}", params.Size());
      std::string top = fmt::format(
          "SELECT id FROM tp.posts WHERE thread_id = $1 AND parent_id = 0 "
          "{} {} {}",
          topSinceStr, topOrderStr, topLimitStr);
      std::string orderStr =
          fmt::format("ORDER BY path[1] {}, path, id", desc ? "DESC" : "ASC");

      auto result = m_ClusterPG->Execute(
          userver::storages::postgres::ClusterHostType::kMaster,
          fmt::format(
              "SELECT p.id, p.parent_id, u.nickname, p.message, "
              "p.is_edited, p.thread_id, "
              "TO_CHAR(p.created_at::TIMESTAMPTZ AT TIME ZONE 'UTC', "
              "'YYYY-MM-DD HH24:MI:SS.MS') "
              "FROM tp.posts AS p JOIN tp.users AS u ON u.id = p.user_id "
              "WHERE path[1] = ANY({}) "
              "{} ",
              top, orderStr),
          params);

      if (result.IsEmpty()) {
        return userver::formats::json::MakeArray();
      }

      auto posts = result.AsSetOf<Post::TypeBatchGetPG>(
          userver::storages::postgres::kRowTag);

      userver::formats::json::ValueBuilder builder;
      for (const auto& post : posts) {
        builder.PushBack(Post::MakeJson(post, forumSlug));
      }
      return builder.ExtractValue();
    }

    std::string orderStr = "";
    std::string sortStr = "";
    std::string joinStr = "";
    if (sort == "tree") {
      sortStr =
          fmt::format("AND p.path {} (SELECT path FROM tp.posts WHERE id = $2)",
                      desc ? "<" : ">");
      orderStr = "ORDER BY p.path";

    } else {
      sortStr = fmt::format("AND p.id {} $2", desc ? "<" : ">");
      orderStr = "ORDER BY p.id";
    }
    if (since == -1) sortStr = "";

    const std::string descStr = desc ? "DESC" : "";
    params.PushBack(limit);
    std::string limitStr = fmt::format("${}", params.Size());

    auto result = m_ClusterPG->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        fmt::format("SELECT p.id, p.parent_id, u.nickname, p.message, "
                    "p.is_edited, p.thread_id, "
                    "TO_CHAR(p.created_at::TIMESTAMPTZ AT TIME ZONE 'UTC', "
                    "'YYYY-MM-DD HH24:MI:SS.MS') "
                    "FROM tp.posts AS p JOIN tp.users AS u ON u.id = p.user_id "
                    "WHERE p.thread_id = $1 {} "
                    "{} {} "
                    "LIMIT {} ",
                    sortStr, orderStr, descStr, limitStr),
        params);
    if (result.IsEmpty()) {
      return userver::formats::json::MakeArray();
    }

    auto posts = result.AsSetOf<Post::TypeBatchGetPG>(
        userver::storages::postgres::kRowTag);

    userver::formats::json::ValueBuilder builder;
    for (const auto& post : posts) {
      builder.PushBack(Post::MakeJson(post, forumSlug));
    }
    return builder.ExtractValue();
  }

 private:
  userver::storages::postgres::ClusterPtr m_ClusterPG;
};

}  // namespace

void AppendPostsGet(userver::components::ComponentList& component_list) {
  component_list.Append<PostsGet>();
}

}  // namespace vkpg
