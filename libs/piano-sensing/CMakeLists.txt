cmake_minimum_required(VERSION 3.22)
project(piano_sensing LANGUAGES CXX)

if(NOT TARGET midi)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../midi midi)
endif()

add_library(piano_sensing INTERFACE)
target_compile_features(piano_sensing INTERFACE cxx_std_20)
target_include_directories(piano_sensing INTERFACE include)
target_link_libraries(piano_sensing INTERFACE midi)

if(HOST_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
