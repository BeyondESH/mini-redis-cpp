cmake_minimum_required(VERSION 3.10)

project(mini_redis)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
set(CMAKE_CXX_FLAGS "-Wall -Wextra -g")
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fsanitize=address -fno-omit-frame-pointer")

add_executable(mini_redis
    src/main.cpp
    src/resp.cpp
    test/test_resp.cpp
    src/server.cpp
    src/connection.cpp
)

target_include_directories(mini_redis PRIVATE ${PROJECT_SOURCE_DIR}/include)

# 链接库
find_package(Threads REQUIRED)
target_link_libraries(mini_redis PRIVATE Threads::Threads)