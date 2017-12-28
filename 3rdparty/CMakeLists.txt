set(SPDLOG_BUILD_TESTING OFF CACHE BOOL "don't build spdlog tests")
add_subdirectory(spdlog)
add_subdirectory(zxing-cpp/core)

set(BUILD_CPR_TESTS OFF CACHE BOOL "don't compile tests")
set(USE_SYSTEM_CURL ON CACHE BOOL "use system curl libraries")
#set(CURL_CA_PATH none CACHE PATH "Path to SSL cert dir")
#set(CMAKE_USE_OPENSSL OFF CACHE BOOL "don't use OpenSSL code")
add_subdirectory(cpr)

