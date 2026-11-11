cmake_minimum_required(VERSION 3.16)
project(warp_sim LANGUAGES C CXX)

find_package(SDL2 REQUIRED)

add_executable(warp_sim main.cpp parser.cpp simulator.cpp ../../src/sim.c)
target_compile_features(warp_sim PRIVATE cxx_std_17)
target_include_directories(warp_sim PRIVATE ../../include)
target_link_libraries(warp_sim PRIVATE SDL2::SDL2)
set_target_properties(warp_sim PROPERTIES CXX_EXTENSIONS OFF)
