#include "Forum.hpp"

#include <fmt/format.h>

namespace vkpg {

namespace Forum {

userver::formats::json::Value MakeJson(std::string_view title,  //
                                       std::string_view user,   //
                                       std::string_view slug,   //
                                       int posts,               //
                                       int threads) {
  userver::formats::json::ValueBuilder builder;
  builder["title"] = title;
  builder["user"] = user;
  builder["slug"] = slug;
  builder["posts"] = posts;
  builder["threads"] = threads;
  return builder.ExtractValue();
}

userver::formats::json::Value MakeJson(const TypeNoUserPG& forum,
                                       std::string_view nickname) {
  return MakeJson(std::get<0>(forum),  //
                  nickname,            //
                  std::get<1>(forum),  //
                  std::get<2>(forum),  //
                  std::get<3>(forum)   //
  );
}

userver::formats::json::Value MakeJson(const TypePG& forum) {
  return MakeJson(std::get<0>(forum),  //
                  std::get<1>(forum),  //
                  std::get<2>(forum),  //
                  std::get<3>(forum),  //
                  std::get<4>(forum));
}

userver::storages::postgres::ResultSet SelectBySlug(
    const userver::storages::postgres::ClusterPtr& cluster,
    std::string_view slug) {
  return cluster->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT f.title, u.nickname, f.slug, f.posts, f.threads "
      "FROM tp.forums AS f "
      "JOIN tp.users AS u ON f.user_id = u.id "
      "WHERE lower(f.slug) = lower($1) ",
      slug);
}

userver::storages::postgres::ResultSet SelectIdBySlug(
    const userver::storages::postgres::ClusterPtr& cluster,
    std::string_view slug) {
  return cluster->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                          "SELECT id FROM tp.forums "
                          "WHERE lower(slug) = lower($1) ",
                          slug);
}

userver::storages::postgres::ResultSet SelectIdSlugBySlug(
    const userver::storages::postgres::ClusterPtr& cluster,
    std::string_view slug) {
  return cluster->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                          "SELECT id, slug FROM tp.forums "
                          "WHERE lower(slug) = lower($1) ",
                          slug);
}

userver::formats::json::Value ReturnNotFound(
    const userver::server::http::HttpRequest& request, std::string_view slug) {
  request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
  return userver::formats::json::MakeObject(
      "message", fmt::format("Can't find forum with id {}", slug));
}

void AddUser(const userver::storages::postgres::ClusterPtr& cluster, int userId,
             std::string_view slug) {
  cluster->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "INSERT INTO tp.forums_users(forum_id, user_id) "
      "VALUES((SELECT id FROM tp.forums WHERE lower(slug) = lower($1)), $2) "
      "ON CONFLICT DO NOTHING ",
      slug, userId);
}

void AddUser(const userver::storages::postgres::ClusterPtr& cluster, int userId,
             int forumId) {
  cluster->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                   "INSERT INTO tp.forums_users(forum_id, user_id) "
                   "VALUES($1, $2) "
                   "ON CONFLICT DO NOTHING ",
                   forumId, userId);
}

void IncreaseThread(const userver::storages::postgres::ClusterPtr& cluster,
                    int forumId) {
  cluster->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                   "UPDATE tp.forums SET threads = threads + 1 WHERE id = $1 ",
                   forumId);
}

void IncreasePosts(const userver::storages::postgres::ClusterPtr& cluster,
                   int forumId, int postsCount) {
  cluster->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                   "UPDATE tp.forums SET posts = posts + $2 WHERE id = $1 ",
                   forumId, postsCount);
}

}  // namespace Forum

}  // namespace vkpg