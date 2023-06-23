#include "Thread.hpp"

#include <fmt/format.h>

namespace vkpg {

namespace Thread {

userver::formats::json::Value MakeJson(int id,                           //
                                       std::string_view title,           //
                                       std::string_view message,         //
                                       std::optional<std::string> slug,  //
                                       int votes,                        //
                                       std::string_view author,          //
                                       std::string_view forum,           //
                                       std::string_view created) {
  userver::formats::json::ValueBuilder builder;
  builder["id"] = id;
  builder["title"] = title;
  builder["message"] = message;
  if (slug.has_value()) {
    builder["slug"] = slug.value();
  }
  builder["votes"] = votes;
  builder["author"] = author;
  builder["forum"] = forum;
  std::string created_at = created.data();
  created_at[10] = 'T';
  created_at.push_back('Z');
  builder["created"] = created_at;

  return builder.ExtractValue();
}

userver::formats::json::Value MakeJson(const TypePG& thread) {
  return MakeJson(std::get<0>(thread),  //
                  std::get<1>(thread),  //
                  std::get<2>(thread),  //
                  std::get<3>(thread),  //
                  std::get<4>(thread),  //
                  std::get<5>(thread),  //
                  std::get<6>(thread),  //
                  std::get<7>(thread));
}

userver::formats::json::Value MakeJson(const TypeNoUserNoForumPG& thread,
                                       std::string_view author,
                                       std::string_view forum) {
  return MakeJson(std::get<0>(thread),  //
                  std::get<1>(thread),  //
                  std::get<2>(thread),  //
                  std::get<3>(thread),  //
                  std::get<4>(thread),  //
                  author,               //
                  forum,                //
                  std::get<5>(thread));
}

userver::storages::postgres::ResultSet SelectBySlug(
    const userver::storages::postgres::ClusterPtr& cluster,
    std::string_view slug) {
  return cluster->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT t.id, t.title, t.message, t.slug, t.votes, "
      "u.nickname, f.slug, "
      "TO_CHAR(t.created_at::TIMESTAMPTZ AT TIME ZONE 'UTC', "
      "'YYYY-MM-DD HH24:MI:SS.MS') "
      "FROM tp.threads AS t "
      "JOIN tp.forums AS f ON t.forum_id = f.id "
      "JOIN tp.users AS u ON t.user_id = u.id "
      "WHERE lower(t.slug) = lower($1) ",
      slug);
}

userver::storages::postgres::ResultSet SelectById(
    const userver::storages::postgres::ClusterPtr& cluster, int id) {
  return cluster->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT t.id, t.title, t.message, t.slug, t.votes, "
      "u.nickname, f.slug, "
      "TO_CHAR(t.created_at::TIMESTAMPTZ AT TIME ZONE 'UTC', "
      "'YYYY-MM-DD HH24:MI:SS.MS') "
      "FROM tp.threads AS t "
      "JOIN tp.forums AS f ON t.forum_id = f.id "
      "JOIN tp.users AS u ON t.user_id = u.id "
      "WHERE t.id = $1 ",
      id);
}

userver::storages::postgres::ResultSet SelectBySlugOrId(
    const userver::storages::postgres::ClusterPtr& cluster,
    std::string_view slugOrId) {
  bool slugIsInt =
      slugOrId.find_first_not_of("0123456789") == std::string::npos;
  if (slugIsInt) {
    return SelectById(cluster, std::stoi(slugOrId.data()));
  }
  return SelectBySlug(cluster, slugOrId);
}

userver::storages::postgres::ResultSet SelectIdAndForumIdSlugBySlugOrId(
    const userver::storages::postgres::ClusterPtr& cluster,
    std::string_view slugOrId) {
  bool slugIsInt =
      slugOrId.find_first_not_of("0123456789") == std::string::npos;
  if (slugIsInt) {
    return cluster->Execute(  //
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT t.id, t.forum_id, f.slug FROM tp.threads AS t "
        "JOIN tp.forums AS f ON t.forum_id = f.id "
        "WHERE t.id = $1 ",
        std::stoi(slugOrId.data()));
  }
  return cluster->Execute(  //
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT t.id, t.forum_id, f.slug FROM tp.threads AS t "
      "JOIN tp.forums AS f ON t.forum_id = f.id "
      "WHERE lower(t.slug) = lower($1) ",
      slugOrId);
}

userver::storages::postgres::ResultSet SelectIdBySlugOrId(
    const userver::storages::postgres::ClusterPtr& cluster,
    std::string_view slugOrId) {
  bool slugIsInt =
      slugOrId.find_first_not_of("0123456789") == std::string::npos;
  if (slugIsInt) {
    return cluster->Execute(  //
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT id FROM tp.threads "
        "WHERE id = $1 ",
        std::stoi(slugOrId.data()));
  }
  return cluster->Execute(  //
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id FROM tp.threads "
      "WHERE lower(slug) = lower($1) ",
      slugOrId);
}

userver::formats::json::Value ReturnNotFound(
    const userver::server::http::HttpRequest& request, std::string_view slug) {
  request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
  return userver::formats::json::MakeObject(
      "message", fmt::format("Can't find thread with id {}", slug));
}

}  // namespace Thread

}  // namespace vkpg