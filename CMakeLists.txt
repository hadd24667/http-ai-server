cmake_minimum_required(VERSION 3.16)
project(HttpAIServer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(server)
enable_testing()
add_subdirectory(tests)
