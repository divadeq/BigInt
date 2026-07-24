cmake_minimum_required(VERSION 3.14)
project(BigIntCrypto CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

add_executable(bigint_app src/main.cpp)
target_include_directories(bigint_app PRIVATE include)

enable_testing()
add_executable(bigint_tests tests/test_bigint.cpp)
target_include_directories(bigint_tests PRIVATE include)
target_link_libraries(bigint_tests PRIVATE GTest::gtest_main)