cmake_minimum_required(VERSION 4.3.3)
add_subdirectory(${CMAKE_SOURCE_DIR}/src)
add_executable(main ${CMAKE_SOURCE_DIR}/src/cmd/main.cpp)
