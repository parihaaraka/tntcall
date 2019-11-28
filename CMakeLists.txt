cmake_minimum_required(VERSION 3.10)

project(tntcall CXX)

# https://blog.kitware.com/cmake-and-the-default-build-type/
#    * see comments for explanation
# Set a default build type if none was specified
set(default_build_type "Release")
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  set(default_build_type "Debug")
endif()
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set(CMAKE_CXX_STANDARD 17)

set(CPP2TNT_LIBEV true)
add_subdirectory(third_party/cpp2tnt)

add_executable(tntcall main.cpp)
target_include_directories(${PROJECT_NAME} BEFORE PRIVATE third_party)
target_link_libraries(${PROJECT_NAME} cpp2tnt ev pthread)
