cmake_minimum_required(VERSION 3.12)
project(vkpg CXX)

include(third_party/userver/cmake/SetupEnvironment.cmake)
include(GNUInstallDirs)

add_subdirectory(third_party/userver)


# Common sources
add_library(${PROJECT_NAME}_objs OBJECT
    src/hello.hpp
    src/hello.cpp
    
    src/Utils/Utils.hpp
    src/Utils/Utils.cpp

    src/Users/User.hpp
    src/Users/User.cpp
    src/Users/UserCreate.hpp
    src/Users/UserCreate.cpp
    src/Users/UserProfile.hpp
    src/Users/UserProfile.cpp
    src/Users/UsersByForum.hpp
    src/Users/UsersByForum.cpp
    
    src/Forums/Forum.hpp
    src/Forums/Forum.cpp
    src/Forums/ForumCreate.hpp
    src/Forums/ForumCreate.cpp
    src/Forums/ForumDetails.hpp
    src/Forums/ForumDetails.cpp
    
    src/Threads/Thread.hpp
    src/Threads/Thread.cpp
    src/Threads/ThreadCreate.hpp
    src/Threads/ThreadCreate.cpp
    src/Threads/ThreadDetails.hpp
    src/Threads/ThreadDetails.cpp
    src/Threads/ThreadsByForum.hpp
    src/Threads/ThreadsByForum.cpp
    src/Threads/Vote.hpp
    src/Threads/Vote.cpp
    
    src/Posts/Post.hpp
    src/Posts/Post.cpp
    src/Posts/PostsCreate.hpp
    src/Posts/PostsCreate.cpp
    src/Posts/PostsGet.hpp
    src/Posts/PostsGet.cpp
    src/Posts/PostDetails.hpp
    src/Posts/PostDetails.cpp

    src/Service/ServiceClear.hpp
    src/Service/ServiceClear.cpp
    src/Service/ServiceStatus.hpp
    src/Service/ServiceStatus.cpp
)
target_link_libraries(${PROJECT_NAME}_objs PUBLIC userver-core userver-postgresql)


# The Service
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_objs)


# Unit Tests
add_executable(${PROJECT_NAME}_unittest
    src/hello_test.cpp
)
target_link_libraries(${PROJECT_NAME}_unittest PRIVATE ${PROJECT_NAME}_objs userver-utest)
add_google_tests(${PROJECT_NAME}_unittest)


# Benchmarks
add_executable(${PROJECT_NAME}_benchmark
	src/hello_benchmark.cpp
)
target_link_libraries(${PROJECT_NAME}_benchmark PRIVATE ${PROJECT_NAME}_objs userver-ubench)
add_google_benchmark_tests(${PROJECT_NAME}_benchmark)

# Functional Tests
add_subdirectory(tests)

if(DEFINED ENV{PREFIX})
	message(STATUS "Set install prefix: $ENV{PREFIX}")
	file(TO_CMAKE_PATH "$ENV{PREFIX}" PREFIX_PATH)
	set(CMAKE_INSTALL_PREFIX ${PREFIX_PATH})
endif()

set(CONFIG_VAR_PATH ../configs/config_vars.yaml)
set(CONFIG_FALLBACK_PATH ../configs/dynamic_config_fallback.json)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configs/static_config.yaml.in ${CMAKE_CURRENT_SOURCE_DIR}/configs/static_config.yaml)

FILE(GLOB CONFIGS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.yaml ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.json)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${PROJECT_NAME})
install(FILES ${CONFIGS_FILES} DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/${PROJECT_NAME} COMPONENT ${PROJECT_NAME})
