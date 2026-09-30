cmake_minimum_required(VERSION 4.3.3)
project(MiniDB LANGUAGES CXX)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)

find_package(OpenSSL REQUIRED)

add_executable(main ${CMAKE_CURRENT_SOURCE_DIR}/src/cmd/main.cpp)
target_include_directories(main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(main SYSTEM PRIVATE ${OPENSSL_INCLUDE_DIR})
target_compile_features(main PUBLIC cxx_std_20)
