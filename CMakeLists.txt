cmake_minimum_required(VERSION 3.20)
project(Geode-Lib)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")


find_package(OpenGL REQUIRED)
add_library(Geode-Lib g_entity.cpp g_input.cpp g_rendering.cpp g_texture.cpp g_time.cpp g_world.cpp g_ui.cpp geode_ntn.cpp geode_ntn.h geode_uiextensions.cpp geode_uiextensions.h geode.h)
target_link_libraries(Geode-Lib -lGLEW -lglfw -lGL)
