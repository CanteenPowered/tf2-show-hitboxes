cmake_minimum_required(VERSION 3.0) # total guess
project(tf2-show-hitboxes)

# Don't use absolute paths when linking shared libraries
set(CMAKE_SKIP_BUILD_RPATH TRUE)

#
# Dependencies
#
add_subdirectory(source-sdk-tf2)

#
# Plugin shared library
#
add_library(show-hitboxes-plugin SHARED
    "${CMAKE_CURRENT_LIST_DIR}/plugin.cpp"
)
set_property(TARGET show-hitboxes-plugin PROPERTY CXX_STANDARD 11)
target_link_libraries(show-hitboxes-plugin PRIVATE mathlib tier0 tier1 vstdlib)
