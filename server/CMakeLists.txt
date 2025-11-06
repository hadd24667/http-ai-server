cmake_minimum_required(VERSION 3.16)
project(HttpAIServer)

file(GLOB_RECURSE SRC_FILES src/*.cpp)
add_executable(http_server ${SRC_FILES})
target_include_directories(http_server PUBLIC include)
target_link_libraries(http_server PRIVATE nlohmann_json::nlohmann_json)
target_compile_options(http_server PRIVATE -Wall -Wextra -pedantic -O2)
find_package(nlohmann_json CONFIG REQUIRED)
