cmake_minimum_required(VERSION 3.22)
project(config LANGUAGES CXX)

add_library(config INTERFACE)
target_compile_features(config INTERFACE cxx_std_20)
target_include_directories(config INTERFACE include)

if(HOST_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
