#include "ForumCreate.hpp"

#include "../Users/User.hpp"
#include "Forum.hpp"

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

class ForumCreate final
    : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-forum-create";

  ForumCreate(const userver::components::ComponentConfig& config,
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
    const auto title = json["title"].As<std::string>("");
    const auto nickname = json["user"].As<std::string>("");
    const auto slug = json["slug"].As<std::string>("");

    if (title.empty() || nickname.empty() || slug.empty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      return userver::formats::json::MakeObject("message", "bad data");
    }

    auto userResult = User::SelectFullByNickname(m_ClusterPG, nickname);
    if (userResult.IsEmpty()) {
      return User::ReturnNotFound(request, nickname);
    }
    const User::TypeWithIdPG user = userResult.AsSingleRow<User::TypeWithIdPG>(
        userver::storages::postgres::kRowTag);
    const int userId = std::get<0>(user);
    const auto& nicknamePG = std::get<1>(user);

    try {
      auto result = m_ClusterPG->Execute(
          userver::storages::postgres::ClusterHostType::kMaster,
          "INSERT INTO tp.forums(title, slug, user_id) "
          "VALUES($1, $2, $3) "
          "RETURNING title, slug, posts, threads ",
          title, slug, userId);
      // Forum::AddUser(m_ClusterPG, userId, slug);
      const auto& forum = result.AsSingleRow<Forum::TypeNoUserPG>(
          userver::storages::postgres::kRowTag);

      request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
      return Forum::MakeJson(forum, nicknamePG);
    } catch (...) {
      auto result = Forum::SelectBySlug(m_ClusterPG, slug);
      const auto& forum = result.AsSingleRow<Forum::TypePG>(
          userver::storages::postgres::kRowTag);

      request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
      return Forum::MakeJson(forum);
    }
  }

 private:
  userver::storages::postgres::ClusterPtr m_ClusterPG;
};

}  // namespace

void AppendForumCreate(userver::components::ComponentList& component_list) {
  component_list.Append<ForumCreate>();
}

}  // namespace vkpg