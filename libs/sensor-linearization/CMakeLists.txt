cmake_minimum_required(VERSION 3.22)
project(sensor_linearization LANGUAGES CXX)

add_library(sensor_linearization INTERFACE)
target_compile_features(sensor_linearization INTERFACE cxx_std_20)
target_include_directories(sensor_linearization INTERFACE include)

if(HOST_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
