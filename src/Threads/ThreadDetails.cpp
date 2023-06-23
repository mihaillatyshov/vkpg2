#include "ThreadDetails.hpp"

#include "../Utils/Utils.hpp"
#include "Thread.hpp"

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

class ThreadDetails final
    : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-thread-details";

  ThreadDetails(const userver::components::ComponentConfig& config,
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

    if (slugOrId.empty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      return userver::formats::json::MakeObject("message", "no slug_or_id");
    }

    switch (request.GetMethod()) {
      case userver::server::http::HttpMethod::kGet:
        return GetValue(request, slugOrId);
      case userver::server::http::HttpMethod::kPost:
        return PostValue(request, slugOrId, json);
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
      const userver::server::http::HttpRequest& request,
      std::string_view slugOrId) const {
    auto result = Thread::SelectBySlugOrId(m_ClusterPG, slugOrId);
    if (result.IsEmpty()) {
      return Thread::ReturnNotFound(request, slugOrId);
    }

    const auto& thread = result.AsSingleRow<Thread::TypePG>(
        userver::storages::postgres::kRowTag);

    return Thread::MakeJson(thread);
  }

  // TODO: Check json fields to update?
  userver::formats::json::Value PostValue(
      const userver::server::http::HttpRequest& request,
      std::string_view slugOrId,
      const userver::formats::json::Value& json) const {
    const auto title = json["title"].As<std::string>("");
    const auto message = json["message"].As<std::string>("");

    if (title.empty() && message.empty()) {
      auto result = Thread::SelectBySlugOrId(m_ClusterPG, slugOrId);
      if (result.IsEmpty()) {
        return Thread::ReturnNotFound(request, slugOrId);
      }
      const auto& thread = result.AsSingleRow<Thread::TypePG>(
          userver::storages::postgres::kRowTag);

      return Thread::MakeJson(thread);
    }

    userver::storages::postgres::ParameterStore params;
    std::string insertParams = "";
    if (!title.empty())
      insertParams += Utils::AddToUpdatePG(params, "title", title);
    if (!message.empty())
      insertParams += Utils::AddToUpdatePG(params, "message", message);
    insertParams.pop_back();

    bool slugIsInt =
        slugOrId.find_first_not_of("0123456789") == std::string::npos;

    int id = 0;
    if (slugIsInt) {
      params.PushBack(std::stoi(slugOrId.data()));
      auto result = m_ClusterPG->Execute(
          userver::storages::postgres::ClusterHostType::kMaster,
          fmt::format("UPDATE tp.threads SET {} "
                      "WHERE id = ${} "
                      "RETURNING id ",
                      insertParams, params.Size()),
          params);
      if (result.IsEmpty()) {
        return Thread::ReturnNotFound(request, slugOrId);
      }
      id = result.AsSingleRow<int>();
    } else {
      params.PushBack(slugOrId);
      auto result = m_ClusterPG->Execute(
          userver::storages::postgres::ClusterHostType::kMaster,
          fmt::format("UPDATE tp.threads SET {} "
                      "WHERE lower(slug) = lower(${}) "
                      "RETURNING id ",
                      insertParams, params.Size()),
          params);
      if (result.IsEmpty()) {
        return Thread::ReturnNotFound(request, slugOrId);
      }
      id = result.AsSingleRow<int>();
    }
    auto result = Thread::SelectById(m_ClusterPG, id);
    const auto& thread = result.AsSingleRow<Thread::TypePG>(
        userver::storages::postgres::kRowTag);

    return Thread::MakeJson(thread);
  }

 private:
  userver::storages::postgres::ClusterPtr m_ClusterPG;
};

}  // namespace

void AppendThreadDetails(userver::components::ComponentList& component_list) {
  component_list.Append<ThreadDetails>();
}

}  // namespace vkpg