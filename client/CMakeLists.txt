cmake_minimum_required(VERSION 3.12)
project(hawkbit-client)

set(CMAKE_CXX_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CURL REQUIRED libcurl)

add_executable(client
    src/main.cpp
    src/http_client.cpp
    src/hawkbit_client.cpp
)

target_include_directories(client PRIVATE 
    include
    ${CURL_INCLUDE_DIRS}
)

target_link_libraries(client 
    ${CURL_LIBRARIES}
)

target_compile_options(client PRIVATE 
    ${CURL_CFLAGS_OTHER}
)