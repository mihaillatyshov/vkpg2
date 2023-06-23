#include "ServiceClear.hpp"

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

class ServiceClear final
    : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-service-clear";

  ServiceClear(const userver::components::ComponentConfig& config,
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
    auto result = m_ClusterPG->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "TRUNCATE tp.users CASCADE");
    return userver::formats::json::MakeObject("message", "Cleared");
  }

 private:
  userver::storages::postgres::ClusterPtr m_ClusterPG;
};

}  // namespace

void AppendServiceClear(userver::components::ComponentList& component_list) {
  component_list.Append<ServiceClear>();
}

}  // namespace vkpg