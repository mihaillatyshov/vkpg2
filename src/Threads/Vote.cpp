#include "Vote.hpp"

#include "../Users/User.hpp"
#include "Thread.hpp"

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

class ThreadVote final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-thread-vote";

  ThreadVote(const userver::components::ComponentConfig& config,
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
    const auto nickname = json["nickname"].As<std::string>("");
    const auto voice = json["voice"].As<int>(0);

    if (nickname.empty() || voice == 0) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      return userver::formats::json::MakeObject("message", "bad data");
    }

    auto threadResult = Thread::SelectIdBySlugOrId(m_ClusterPG, slugOrId);
    if (threadResult.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
      return Thread::ReturnNotFound(request, slugOrId);
    }
    int threadId = threadResult.AsSingleRow<int>();

    auto userResult = User::SelectIdByNickname(m_ClusterPG, nickname);
    if (userResult.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
      return User::ReturnNotFound(request, nickname);
    }
    int userId = userResult.AsSingleRow<int>();

    try {
      m_ClusterPG->Execute(
          userver::storages::postgres::ClusterHostType::kMaster,
          "INSERT INTO tp.votes (thread_id, user_id, voice) "
          "VALUES ($1, $2, $3) "
          "ON CONFLICT (thread_id, user_id) "
          "DO UPDATE SET voice = EXCLUDED.voice ",
          threadId, userId, voice);

      auto result = Thread::SelectById(m_ClusterPG, threadId);
      Thread::TypePG thread = result.AsSingleRow<Thread::TypePG>(
          userver::storages::postgres::kRowTag);

      return Thread::MakeJson(thread);
    } catch (...) {
      request.SetResponseStatus(
          userver::server::http::HttpStatus::kInternalServerError);
      return userver::formats::json::MakeObject("message", "Error!!!");
    }
  }

 private:
  userver::storages::postgres::ClusterPtr m_ClusterPG;
};
}  // namespace

void AppendThreadVote(userver::components::ComponentList& component_list) {
  component_list.Append<ThreadVote>();
}

}  // namespace vkpg