#include "UserProfile.hpp"

#include "../Utils/Utils.hpp"
#include "User.hpp"

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

class UserProfile final
    : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-user-profile";

  UserProfile(const userver::components::ComponentConfig& config,
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
    const auto& nickname = request.GetPathArg("nickname");

    if (nickname.empty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      return userver::formats::json::MakeObject("message", "no nickname");
    }

    switch (request.GetMethod()) {
      case userver::server::http::HttpMethod::kGet:
        return GetValue(request, nickname);
      case userver::server::http::HttpMethod::kPost:
        return PostValue(request, nickname, json);
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
      std::string_view nickname) const {
    auto result = User::SelectByNickname(m_ClusterPG, nickname);
    if (result.IsEmpty()) {
      return User::ReturnNotFound(request, nickname);
    }

    const auto& user =
        result.AsSingleRow<User::TypePG>(userver::storages::postgres::kRowTag);

    return User::MakeJson(user);
  }

  // TODO: Check json fields to update?
  userver::formats::json::Value PostValue(
      const userver::server::http::HttpRequest& request,
      std::string_view nickname,
      const userver::formats::json::Value& json) const {
    const auto fullname = json["fullname"].As<std::string>("");
    const auto about = json["about"].As<std::string>("");
    const auto email = json["email"].As<std::string>("");

    auto result = User::SelectByNickname(m_ClusterPG, nickname);
    if (result.IsEmpty()) {
      return User::ReturnNotFound(request, nickname);
    }

    if (fullname.empty() && email.empty() && about.empty()) {
      const auto& user = result.AsSingleRow<User::TypePG>(
          userver::storages::postgres::kRowTag);
      return User::MakeJson(user);
    }

    try {
      userver::storages::postgres::ParameterStore params;
      params.PushBack(nickname);
      std::string insertParams = "";
      if (!fullname.empty())
        insertParams += Utils::AddToUpdatePG(params, "fullname", fullname);
      if (!email.empty())
        insertParams += Utils::AddToUpdatePG(params, "email", email);
      if (!about.empty())
        insertParams += Utils::AddToUpdatePG(params, "about", about);
      insertParams.pop_back();

      auto result = m_ClusterPG->Execute(
          userver::storages::postgres::ClusterHostType::kMaster,
          fmt::format("UPDATE tp.users SET {} "
                      "WHERE lower(nickname) = lower($1) "
                      "RETURNING nickname, fullname, about, email",
                      insertParams),
          params);
      if (result.IsEmpty()) {
        User::ReturnNotFound(request, nickname);
      }

      const auto& user = result.AsSingleRow<User::TypePG>(
          userver::storages::postgres::kRowTag);
      return User::MakeJson(user);
    } catch (...) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
      return userver::formats::json::MakeObject("message",
                                                "Some data has intersections");
    }
  }

 private:
  userver::storages::postgres::ClusterPtr m_ClusterPG;
};

}  // namespace

void AppendUserProfile(userver::components::ComponentList& component_list) {
  component_list.Append<UserProfile>();
}

}  // namespace vkpg