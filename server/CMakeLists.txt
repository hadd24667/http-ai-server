cmake_minimum_required(VERSION 3.16)
project(HttpAIServer)

# 1. JSON library
find_package(nlohmann_json CONFIG REQUIRED)

# 2. OpenSSL library
find_package(OpenSSL REQUIRED)

# 3. Scan all cpp files
file(GLOB_RECURSE SRC_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# 4. Build executable
add_executable(http_server ${SRC_FILES})

# 5. Include directories
target_include_directories(http_server PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 6. Link libraries
target_link_libraries(http_server PRIVATE 
    nlohmann_json::nlohmann_json
    pthread
    OpenSSL::SSL
    OpenSSL::Crypto
)

# 7. Compile flags
target_compile_options(http_server PRIVATE -Wall -Wextra -pedantic -O2)

find_package(CURL REQUIRED)
target_link_libraries(http_server PRIVATE CURL::libcurl)
