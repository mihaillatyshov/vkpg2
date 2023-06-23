#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include "Forums/ForumCreate.hpp"
#include "Forums/ForumDetails.hpp"
#include "Posts/PostDetails.hpp"
#include "Posts/PostsCreate.hpp"
#include "Posts/PostsGet.hpp"
#include "Service/ServiceClear.hpp"
#include "Service/ServiceStatus.hpp"
#include "Threads/ThreadCreate.hpp"
#include "Threads/ThreadDetails.hpp"
#include "Threads/ThreadsByForum.hpp"
#include "Threads/Vote.hpp"
#include "Users/UserCreate.hpp"
#include "Users/UserProfile.hpp"
#include "Users/UsersByForum.hpp"
#include "hello.hpp"

int main(int argc, char* argv[]) {
  auto component_list = userver::components::MinimalServerComponentList()
                            .Append<userver::server::handlers::Ping>()
                            .Append<userver::components::TestsuiteSupport>()
                            .Append<userver::components::HttpClient>()
                            .Append<userver::server::handlers::TestsControl>();

  vkpg::AppendHello(component_list);
  vkpg::AppendUserCreate(component_list);
  vkpg::AppendUserProfile(component_list);
  vkpg::AppendUsersByForum(component_list);

  vkpg::AppendForumCreate(component_list);
  vkpg::AppendForumDetails(component_list);

  vkpg::AppendThreadCreate(component_list);
  vkpg::AppendThreadDetails(component_list);
  vkpg::AppendThreadsByForum(component_list);
  vkpg::AppendThreadVote(component_list);

  vkpg::AppendPostsCreate(component_list);
  vkpg::AppendPostsGet(component_list);
  vkpg::AppendPostDetails(component_list);

  vkpg::AppendServiceClear(component_list);
  vkpg::AppendServiceStatus(component_list);

  return userver::utils::DaemonMain(argc, argv, component_list);
}
