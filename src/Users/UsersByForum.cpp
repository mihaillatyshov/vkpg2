#include "UsersByForum.hpp"

#include "../Forums/Forum.hpp"
#include "../Utils/Utils.hpp"
#include "User.hpp"

#include <fmt/format.h>
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

class UsersByForum final
    : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-forum-users";

  UsersByForum(const userver::components::ComponentConfig& config,
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

    std::string since = Utils::GetSinceStr(request);
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
    std::string sortStr = "";
    if (!since.empty()) {
      params.PushBack(since);
      sortStr = fmt::format("AND lower(u.nickname) {} lower($3 COLLATE \"C\")",
                            desc ? "<" : ">");
    }
    auto result = m_ClusterPG->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        fmt::format("SELECT u.nickname, u.fullname, u.about, u.email FROM "
                    "tp.users AS u "
                    "JOIN tp.forums_users AS fu ON fu.user_id = u.id "
                    "JOIN tp.forums AS f ON fu.forum_id = f.id "
                    "WHERE fu.forum_id = $1 {} "
                    "ORDER BY lower(u.nickname) {} "
                    "LIMIT $2 ",
                    sortStr, desc ? "DESC" : ""),
        params);

    if (result.IsEmpty()) {
      return userver::formats::json::MakeArray();
    }
    auto users =
        result.AsSetOf<User::TypePG>(userver::storages::postgres::kRowTag);

    userver::formats::json::ValueBuilder builder;
    for (const auto& user : users) {
      builder.PushBack(User::MakeJson(user));
    }
    return builder.ExtractValue();
  }

 private:
  userver::storages::postgres::ClusterPtr m_ClusterPG;
};

}  // namespace

void AppendUsersByForum(userver::components::ComponentList& component_list) {
  component_list.Append<UsersByForum>();
}

}  // namespace vkpg