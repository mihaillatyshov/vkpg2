#pragma once

#include <string>

#include <userver/server/http/http_request.hpp>
#include <userver/storages/postgres/parameter_store.hpp>

namespace vkpg {

namespace Utils {

int GetLimit(const userver::server::http::HttpRequest& request, int def = 100);

bool GetDesc(const userver::server::http::HttpRequest& request,
             bool def = false);

std::string GetSinceStr(const userver::server::http::HttpRequest& request,
                        std::string def = "");

int GetSinceInt(const userver::server::http::HttpRequest& request, int def = 0);

std::string GetSort(const userver::server::http::HttpRequest& request,
                    std::string def = "flat");

template <typename T>
std::string AddToUpdatePG(userver::storages::postgres::ParameterStore& params,
                          std::string_view name, const T& param,
                          bool isLast = false) {
  params.PushBack(param);
  return fmt::format("{} = ${}{}", name, params.Size(), isLast ? "" : ",");
}

}  // namespace Utils

}  // namespace vkpg
