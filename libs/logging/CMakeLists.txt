cmake_minimum_required(VERSION 3.22)
project(logging LANGUAGES CXX)

add_library(logging INTERFACE)
target_compile_features(logging INTERFACE cxx_std_20)
target_include_directories(logging INTERFACE include)

if(HOST_TESTS AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/tests/CMakeLists.txt")
  enable_testing()
  add_subdirectory(tests)
endif()
