#pragma once

#include <string>
#include <tuple>

#include <userver/formats/json.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/storages/postgres/cluster.hpp>

namespace vkpg {

namespace Forum {

using TypeNoUserPG = std::tuple<std::string,  // title
                                std::string,  // slug
                                int,          // posts
                                int           // threads
                                >;

using TypePG = std::tuple<std::string,  // title
                          std::string,  // user
                          std::string,  // slug
                          int,          // posts
                          int           // threads
                          >;

userver::formats::json::Value MakeJson(std::string_view title,  //
                                       std::string_view user,   //
                                       std::string_view slug,   //
                                       int posts,               //
                                       int threads);

userver::formats::json::Value MakeJson(const TypeNoUserPG& forum,
                                       std::string_view nickname);

userver::formats::json::Value MakeJson(const TypePG& forum);

userver::storages::postgres::ResultSet SelectBySlug(
    const userver::storages::postgres::ClusterPtr& cluster,
    std::string_view slug);

userver::storages::postgres::ResultSet SelectIdBySlug(
    const userver::storages::postgres::ClusterPtr& cluster,
    std::string_view slug);

userver::storages::postgres::ResultSet SelectIdSlugBySlug(
    const userver::storages::postgres::ClusterPtr& cluster,
    std::string_view slug);

userver::formats::json::Value ReturnNotFound(
    const userver::server::http::HttpRequest& request, std::string_view slug);

void AddUser(const userver::storages::postgres::ClusterPtr& cluster, int userId,
             std::string_view slug);

void AddUser(const userver::storages::postgres::ClusterPtr& cluster, int userId,
             int forumId);

void IncreaseThread(const userver::storages::postgres::ClusterPtr& cluster,
                    int forumId);

void IncreasePosts(const userver::storages::postgres::ClusterPtr& cluster,
                   int forumId, int postsCount);

}  // namespace Forum

}  // namespace vkpg