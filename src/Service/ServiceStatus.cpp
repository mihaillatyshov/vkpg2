#include "ServiceStatus.hpp"

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

class ServiceStatus final
    : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-service-status";

  ServiceStatus(const userver::components::ComponentConfig& config,
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
    ;
    auto resultUsers = m_ClusterPG->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT COUNT(*) FROM tp.users");
    int usersCount = resultUsers.AsSingleRow<int>();

    auto resultOther = m_ClusterPG->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT COUNT(*), COALESCE(SUM(posts), 0), "
        "COALESCE(SUM(threads), 0) FROM tp.forums ");
    const auto& [forumsCount, postsCount, threadsCount] =
        resultOther.AsSingleRow<std::tuple<int, int, int>>(
            userver::storages::postgres::kRowTag);

    userver::formats::json::ValueBuilder builder;
    builder["user"] = usersCount;
    builder["forum"] = forumsCount;
    builder["thread"] = threadsCount;
    builder["post"] = postsCount;
    return builder.ExtractValue();
  }

 private:
  userver::storages::postgres::ClusterPtr m_ClusterPG;
};

}  // namespace

void AppendServiceStatus(userver::components::ComponentList& component_list) {
  component_list.Append<ServiceStatus>();
}

}  // namespace vkpg