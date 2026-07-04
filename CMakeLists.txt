cmake_minimum_required(VERSION 3.20)

project(MediaAnalyzerCore VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(media_analyzer_core
  cpp/src/analysis.cpp
  cpp/src/format_detector.cpp
  cpp/src/iso_bmff.cpp
)

target_include_directories(media_analyzer_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp/include
)

target_compile_options(media_analyzer_core
  PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_executable(media-analyzer-core
  cpp/src/main.cpp
)

target_link_libraries(media-analyzer-core
  PRIVATE
    media_analyzer_core
)
