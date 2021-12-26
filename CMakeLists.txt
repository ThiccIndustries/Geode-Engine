cmake_minimum_required(VERSION 3.20)
project(Geode-Lib)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")


find_package(OpenGL REQUIRED)
add_library(Geode-Lib g_entity.cpp g_input.cpp g_rendering.cpp g_texture.cpp g_time.cpp g_world.cpp geode.h)
target_link_libraries(Geode-Lib -lglew32 -lglfw3 -lopengl32)
