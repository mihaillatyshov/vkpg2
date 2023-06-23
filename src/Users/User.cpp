#include "User.hpp"

#include <fmt/format.h>

namespace vkpg {

namespace User {

userver::formats::json::Value MakeJson(std::string_view nickname,         //
                                       std::string_view fullname,         //
                                       std::optional<std::string> about,  //
                                       std::string_view email) {
  userver::formats::json::ValueBuilder builder;
  builder["nickname"] = nickname;
  builder["fullname"] = fullname;
  builder["email"] = email;
  if (about.has_value()) {
    builder["about"] = about.value();
  }
  return builder.ExtractValue();
}

userver::formats::json::Value MakeJson(const TypePG& user) {
  return MakeJson(std::get<0>(user),  //
                  std::get<1>(user),  //
                  std::get<2>(user),  //
                  std::get<3>(user));
}

userver::storages::postgres::ResultSet SelectByNickname(
    const userver::storages::postgres::ClusterPtr& cluster,
    std::string_view nickname) {
  return cluster->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT nickname, fullname, about, email FROM tp.users "
      "WHERE lower(nickname) = lower($1) ",
      nickname);
}

userver::storages::postgres::ResultSet SelectFullByNickname(
    const userver::storages::postgres::ClusterPtr& cluster,
    std::string_view nickname) {
  return cluster->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id, nickname, fullname, about, email FROM tp.users "
      "WHERE lower(nickname) = lower($1) ",
      nickname);
}

userver::storages::postgres::ResultSet SelectIdByNickname(
    const userver::storages::postgres::ClusterPtr& cluster,
    std::string_view nickname) {
  return cluster->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT id FROM tp.users WHERE lower(nickname) = lower($1) ", nickname);
}

userver::formats::json::Value ReturnNotFound(
    const userver::server::http::HttpRequest& request,
    std::string_view nickname) {
  request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
  return userver::formats::json::MakeObject(
      "message", fmt::format("Can't find user with id {}", nickname));
}

}  // namespace User

}  // namespace vkpg