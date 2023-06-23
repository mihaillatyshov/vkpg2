#pragma once

#include <string>
#include <tuple>

#include <userver/formats/json.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/storages/postgres/cluster.hpp>

namespace vkpg {

namespace Thread {

using TypeNoUserNoForumPG = std::tuple<int,                         // id
                                       std::string,                 // title
                                       std::string,                 // message
                                       std::optional<std::string>,  // slug
                                       int,                         // votes
                                       std::string                  // created
                                       >;

using TypePG = std::tuple<int,                         // id
                          std::string,                 // title
                          std::string,                 // message
                          std::optional<std::string>,  // slug
                          int,                         // votes
                          std::string,                 // author (user nickname)
                          std::string,                 // forum
                          std::string                  // created
                          >;

userver::formats::json::Value MakeJson(int id,                           //
                                       std::string_view title,           //
                                       std::string_view message,         //
                                       std::optional<std::string> slug,  //
                                       int votes,                        //
                                       std::string_view author,          //
                                       std::string_view forum,           //
                                       std::string_view created);

userver::formats::json::Value MakeJson(const TypePG& thread);

userver::formats::json::Value MakeJson(const TypeNoUserNoForumPG& thread,
                                       std::string_view author,
                                       std::string_view forum);

userver::storages::postgres::ResultSet SelectBySlug(
    const userver::storages::postgres::ClusterPtr& cluster,
    std::string_view slug);

userver::storages::postgres::ResultSet SelectById(
    const userver::storages::postgres::ClusterPtr& cluster, int id);

userver::storages::postgres::ResultSet SelectBySlugOrId(
    const userver::storages::postgres::ClusterPtr& cluster,
    std::string_view slugOrId);

userver::storages::postgres::ResultSet SelectIdAndForumIdSlugBySlugOrId(
    const userver::storages::postgres::ClusterPtr& cluster,
    std::string_view slugOrId);

userver::storages::postgres::ResultSet SelectIdBySlugOrId(
    const userver::storages::postgres::ClusterPtr& cluster,
    std::string_view slugOrId);

userver::formats::json::Value ReturnNotFound(
    const userver::server::http::HttpRequest& request, std::string_view slug);

}  // namespace Thread

}  // namespace vkpg