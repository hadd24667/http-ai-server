cmake_minimum_required(VERSION 3.16)
project(HttpAIServer)

# 1. Tìm JSON lib (đặt trước khi sử dụng)
find_package(nlohmann_json CONFIG REQUIRED)

# 2. Scan toàn bộ file cpp
file(GLOB_RECURSE SRC_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# 3. Build server executable
add_executable(http_server ${SRC_FILES})

# 4. Include directory
target_include_directories(http_server PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 5. Link libraries (json, pthread)
target_link_libraries(http_server PRIVATE 
    nlohmann_json::nlohmann_json
    pthread
)

# 6. Compiler flags
target_compile_options(http_server PRIVATE -Wall -Wextra -pedantic -O2)
