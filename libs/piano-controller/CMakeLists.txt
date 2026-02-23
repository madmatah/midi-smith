cmake_minimum_required(VERSION 3.22)
project(piano_controller LANGUAGES CXX)

if(NOT TARGET midi)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../midi midi)
endif()

add_library(piano_controller STATIC)
target_compile_features(piano_controller PUBLIC cxx_std_20)
target_include_directories(piano_controller PUBLIC include)
target_sources(piano_controller PRIVATE src/midi_piano.cpp)
target_link_libraries(piano_controller PUBLIC midi)

if(HOST_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
