#include "ForumDetails.hpp"

#include "Forum.hpp"

#include <fmt/format.h>
#include <string>
#include <vector>

#include <userver/clients/dns/component.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fixed_array.hpp>

namespace vkpg {

namespace {

class ForumDetails final
    : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-forum-details";

  ForumDetails(const userver::components::ComponentConfig& config,
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

    if (slug.empty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      return userver::formats::json::MakeObject("message", "bad data");
    }

    auto result = Forum::SelectBySlug(m_ClusterPG, slug);
    if (result.IsEmpty()) {
      return Forum::ReturnNotFound(request, slug);
    }
    const auto& forum =
        result.AsSingleRow<Forum::TypePG>(userver::storages::postgres::kRowTag);

    return Forum::MakeJson(forum);
  }

 private:
  userver::storages::postgres::ClusterPtr m_ClusterPG;
};

}  // namespace

void AppendForumDetails(userver::components::ComponentList& component_list) {
  component_list.Append<ForumDetails>();
}

}  // namespace vkpg