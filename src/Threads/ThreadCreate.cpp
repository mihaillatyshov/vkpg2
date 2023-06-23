#include "ThreadCreate.hpp"

#include "../Forums/Forum.hpp"
#include "../Users/User.hpp"
#include "Thread.hpp"

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

class ThreadCreate final
    : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-thread-create";

  ThreadCreate(const userver::components::ComponentConfig& config,
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
    const auto& forumSlug = request.GetPathArg("slug");
    const auto title = json["title"].As<std::string>("");
    const auto nickname = json["author"].As<std::string>("");
    const auto message = json["message"].As<std::string>("");
    const auto created = json["created"].As<std::optional<std::string>>({});
    const auto threadSlug = json["slug"].As<std::optional<std::string>>({});

    if (forumSlug.empty() || title.empty() || nickname.empty() ||
        message.empty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      return userver::formats::json::MakeObject("message", "bad data");
    }

    auto userResult = User::SelectIdByNickname(m_ClusterPG, nickname);
    if (userResult.IsEmpty()) {
      return User::ReturnNotFound(request, nickname);
    }
    const int userId = userResult.AsSingleRow<int>();

    auto forumResult = Forum::SelectIdSlugBySlug(m_ClusterPG, forumSlug);
    if (forumResult.IsEmpty()) {
      return Forum::ReturnNotFound(request, forumSlug);
    }
    const auto& [forumId, goodForumSlug] =
        forumResult.AsSingleRow<std::tuple<int, std::string>>(
            userver::storages::postgres::kRowTag);

    try {
      auto result = m_ClusterPG->Execute(
          userver::storages::postgres::ClusterHostType::kMaster,
          "INSERT INTO tp.threads "
          "(title, slug, message, created_at, forum_id, user_id) "
          "VALUES($1, $2, $3, "
          "CASE WHEN $4 IS NULL THEN NOW() ELSE $4::TIMESTAMPTZ END, "
          "$5, $6) "
          "RETURNING id, title, message, slug, votes, "
          "TO_CHAR(created_at::TIMESTAMPTZ AT TIME ZONE 'UTC',"
          "'YYYY-MM-DD HH24:MI:SS.MS') ",
          title, threadSlug, message, created, forumId, userId);
      Forum::AddUser(m_ClusterPG, userId, forumId);
      Forum::IncreaseThread(m_ClusterPG, forumId);
      const auto& thread = result.AsSingleRow<Thread::TypeNoUserNoForumPG>(
          userver::storages::postgres::kRowTag);

      request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
      return Thread::MakeJson(thread, nickname, goodForumSlug);
    } catch (...) {
      auto result = Thread::SelectBySlug(m_ClusterPG, threadSlug.value());
      const auto& thread = result.AsSingleRow<Thread::TypePG>(
          userver::storages::postgres::kRowTag);

      request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
      return Thread::MakeJson(thread);
    }
  }

 private:
  userver::storages::postgres::ClusterPtr m_ClusterPG;
};

}  // namespace

void AppendThreadCreate(userver::components::ComponentList& component_list) {
  component_list.Append<ThreadCreate>();
}

}  // namespace vkpg