cmake_minimum_required(VERSION 3.0) # total guess
project(example-plugin)

# Don't use absolute paths when linking shared libraries
set(CMAKE_SKIP_BUILD_RPATH TRUE)

#
# Dependencies
#
add_subdirectory(source-sdk-tf2)

#
# Plugin shared library
#
add_library(plugin SHARED
    "${CMAKE_CURRENT_LIST_DIR}/plugin.cpp"
)
target_link_libraries(plugin PRIVATE mathlib tier0 tier1 vstdlib)
