#include "Utils.hpp"

namespace vkpg {

namespace Utils {

int GetLimit(const userver::server::http::HttpRequest& request, int def) {
  if (request.HasArg("limit")) return std::stoi(request.GetArg("limit"));
  return def;
}

bool GetDesc(const userver::server::http::HttpRequest& request, bool def) {
  if (request.HasArg("desc") && request.GetArg("desc") == "true") return true;
  return def;
}

std::string GetSinceStr(const userver::server::http::HttpRequest& request,
                        std::string def) {
  if (request.HasArg("since")) return request.GetArg("since");
  return def;
}

int GetSinceInt(const userver::server::http::HttpRequest& request, int def) {
  if (request.HasArg("since")) return std::stoi(request.GetArg("since"));
  return def;
}

std::string GetSort(const userver::server::http::HttpRequest& request,
                    std::string def) {
  if (request.HasArg("sort")) return request.GetArg("sort");
  return def;
}

}  // namespace Utils

}  // namespace vkpg
