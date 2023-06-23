
#include "ThreadsByForum.hpp"

#include "../Forums/Forum.hpp"
#include "../Utils/Utils.hpp"
#include "Thread.hpp"

#include <fmt/format.h>
#include <string>
#include <vector>

#include <userver/clients/dns/component.hpp>
#include <userver/formats/json.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/parameter_store.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fixed_array.hpp>

namespace vkpg {

namespace {

class ThreadsByForum final
    : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-forum-threads";

  ThreadsByForum(const userver::components::ComponentConfig& config,
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
    const auto& slug = request.GetPathArg("slug");

    std::string since = request.GetArg("since");
    int limit = Utils::GetLimit(request);
    bool desc = Utils::GetDesc(request);

    if (slug.empty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      return userver::formats::json::MakeObject("message", "bad data");
    }

    auto forumResult = Forum::SelectIdBySlug(m_ClusterPG, slug);
    if (forumResult.IsEmpty()) {
      return Forum::ReturnNotFound(request, slug);
    }
    const int forumId = forumResult.AsSingleRow<int>();

    userver::storages::postgres::ParameterStore params;
    params.PushBack(forumId);
    params.PushBack(limit);
    if (!since.empty()) params.PushBack(since);
    std::string sortStr =
        since.empty() ? ""
                      : fmt::format("AND t.created_at {} ${}::TIMESTAMPTZ",
                                    desc ? "<=" : ">=", params.Size());

    const std::string descStr = desc ? "DESC" : "";
    auto result = m_ClusterPG->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        fmt::format("SELECT t.id, t.title, t.message, t.slug, t.votes, "
                    "u.nickname, f.slug, "
                    "TO_CHAR(t.created_at::TIMESTAMPTZ AT TIME ZONE 'UTC', "
                    "'YYYY-MM-DD HH24:MI:SS.MS') "
                    "FROM tp.threads AS t "
                    "JOIN tp.forums AS f ON t.forum_id = f.id "
                    "JOIN tp.users AS u ON t.user_id = u.id "
                    "WHERE t.forum_id = $1 {} "
                    "ORDER BY t.created_at {} "
                    "LIMIT $2 ",
                    sortStr, descStr),
        params);

    if (result.IsEmpty()) {
      return userver::formats::json::MakeArray();
    }
    auto threads =
        result.AsSetOf<Thread::TypePG>(userver::storages::postgres::kRowTag);

    userver::formats::json::ValueBuilder builder;
    for (const auto& thread : threads) {
      builder.PushBack(Thread::MakeJson(thread));
    }
    return builder.ExtractValue();
  }

 private:
  userver::storages::postgres::ClusterPtr m_ClusterPG;
};

}  // namespace

void AppendThreadsByForum(userver::components::ComponentList& component_list) {
  component_list.Append<ThreadsByForum>();
}

}  // namespace vkpg