cmake_minimum_required(VERSION 3.25)

project(jpeg-decoder LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_path(FFTW_INCLUDE_DIR NAMES fftw3.h REQUIRED)
find_library(FFTW_LIBRARY NAMES fftw3 REQUIRED)

file(GLOB_RECURSE SOLUTION_SRC CONFIGURE_DEPENDS "src/decoder/*.cpp")
add_library(decoder ${SOLUTION_SRC})
target_include_directories(decoder PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(decoder PRIVATE "${FFTW_INCLUDE_DIR}")
target_link_libraries(decoder PRIVATE "${FFTW_LIBRARY}")

add_executable(jpeg-decoder src/main.cpp)
target_link_libraries(jpeg-decoder PRIVATE decoder Qt6::Widgets)

add_subdirectory(test)
