cmake_minimum_required(VERSION 3.22)
project(shell LANGUAGES CXX)

if(NOT TARGET io)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../io io)
endif()

add_library(shell STATIC)
target_compile_features(shell PUBLIC cxx_std_20)
target_include_directories(shell PUBLIC include)
target_sources(shell PRIVATE
  src/command_parser.cpp
  src/command_dispatcher.cpp
  src/line_editor.cpp
  src/shell_engine.cpp
)
target_link_libraries(shell PUBLIC io)

if(HOST_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
