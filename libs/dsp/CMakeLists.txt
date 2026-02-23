cmake_minimum_required(VERSION 3.22)
project(dsp LANGUAGES CXX)

add_library(dsp INTERFACE)
target_compile_features(dsp INTERFACE cxx_std_20)
target_include_directories(dsp INTERFACE include)

if(HOST_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
