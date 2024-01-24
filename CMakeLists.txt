cmake_minimum_required(VERSION 3.24)
project(finddups CXX)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include(FetchContent)
FetchContent_Declare(cxxopts
  GIT_REPOSITORY     https://github.com/jarro2783/cxxopts
  GIT_TAG            v3.1.1)
FetchContent_MakeAvailable(cxxopts)
FetchContent_Declare(xxhash_cpp
  GIT_REPOSITORY     https://github.com/RedSpah/xxhash_cpp
  GIT_TAG            0.8.1)
FetchContent_MakeAvailable(xxhash_cpp)

add_executable(finddups finddups.cpp dehumanize.cpp)
target_compile_features(finddups PRIVATE cxx_std_23)
target_link_libraries(finddups PRIVATE cxxopts::cxxopts)
target_include_directories(finddups PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
