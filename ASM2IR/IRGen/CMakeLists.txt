cmake_minimum_required(VERSION 3.16)
project(sim_asm_irgen LANGUAGES C CXX)

find_package(LLVM REQUIRED CONFIG)
find_package(SDL2 REQUIRED)

add_library(sim_jit STATIC jit.cpp ../SIM/parser.cpp ../SIM/simulator.cpp ../../src/sim.c)
target_compile_features(sim_jit PUBLIC cxx_std_17)
target_include_directories(sim_jit SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
target_include_directories(sim_jit PRIVATE ../../include)
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND "${LLVM_DEFINITIONS}")
target_compile_definitions(sim_jit PRIVATE ${LLVM_DEFINITIONS_LIST})
target_link_libraries(sim_jit PUBLIC LLVM SDL2::SDL2)

add_executable(sim_jit_calls main.cpp)
target_link_libraries(sim_jit_calls PRIVATE sim_jit)
target_compile_definitions(sim_jit_calls PRIVATE SIM_HANDLER_CALLS=1)

add_executable(sim_jit_ir main.cpp)
target_link_libraries(sim_jit_ir PRIVATE sim_jit)

