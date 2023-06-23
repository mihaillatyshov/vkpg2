#include "Post.hpp"

#include <fmt/format.h>

namespace vkpg {

namespace Post {

userver::formats::json::Value MakeJson(int id,                    //
                                       int parent,                //
                                       std::string_view author,   //
                                       std::string_view message,  //
                                       bool isEdited,             //
                                       std::string_view forum,    //
                                       int thread,                //
                                       std::string_view created) {
  userver::formats::json::ValueBuilder builder;
  builder["id"] = id;
  builder["parent"] = parent;
  builder["author"] = author;
  builder["message"] = message;

  builder["isEdited"] = isEdited;
  builder["forum"] = forum;
  builder["thread"] = thread;

  std::string created_at = created.data();
  created_at[10] = 'T';
  created_at.push_back('Z');
  builder["created"] = created_at;

  return builder.ExtractValue();
}

userver::formats::json::Value MakeJson(const TypeInsertPG& post,   //
                                       std::string_view nickname,  //
                                       std::string_view forum) {
  return MakeJson(std::get<0>(post),  //
                  std::get<1>(post),  //
                  nickname,           //
                  std::get<2>(post),  //
                  std::get<3>(post),  //
                  forum,              //
                  std::get<4>(post),  //
                  std::get<5>(post));
}

userver::formats::json::Value MakeJson(const TypeBatchGetPG& post,  //
                                       std::string_view forum) {
  return MakeJson(std::get<0>(post),  //
                  std::get<1>(post),  //
                  std::get<2>(post),  //
                  std::get<3>(post),  //
                  std::get<4>(post),  //
                  forum,              //
                  std::get<5>(post),  //
                  std::get<6>(post));
}

userver::formats::json::Value MakeJson(const TypePG& post) {
  return MakeJson(std::get<0>(post),  //
                  std::get<1>(post),  //
                  std::get<2>(post),  //
                  std::get<3>(post),  //
                  std::get<4>(post),  //
                  std::get<5>(post),  //
                  std::get<6>(post),  //
                  std::get<7>(post));
}

userver::storages::postgres::ResultSet SelectById(
    const userver::storages::postgres::ClusterPtr& cluster, int id) {
  return cluster->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT p.id, p.parent_id, u.nickname, p.message, p.is_edited, "
      "f.slug, p.thread_id, "
      "TO_CHAR(p.created_at::TIMESTAMPTZ AT TIME ZONE 'UTC', "
      "'YYYY-MM-DD HH24:MI:SS.MS') "
      "FROM tp.posts AS p "
      "JOIN tp.users AS u ON u.id = p.user_id "
      "JOIN tp.threads AS t ON t.id = p.thread_id "
      "JOIN tp.forums AS f ON f.id = t.forum_id "
      "WHERE p.id = $1 ",
      id);
}

userver::formats::json::Value ReturnNotFound(
    const userver::server::http::HttpRequest& request, int id) {
  request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
  return userver::formats::json::MakeObject(
      "message", fmt::format("Can't find post with id {}", id));
}

}  // namespace Post

}  // namespace vkpg