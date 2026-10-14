cmake_minimum_required(VERSION 3.28)
project(warp_tunnel)

set(CMAKE_C_COMPILER   clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(TARGET warp_tunnel)

# set(CMAKE_CXX_STANDARD 23)
# set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(${TARGET})

# Include directory
target_include_directories(${TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Compile options
target_compile_options(${TARGET} PUBLIC
    -g
    -Os
    $<$<CONFIG:DEBUG>:-g -fsanitize=address>
    $<$<CONFIG:RELEASE>:-O3>
)

target_link_options(${TARGET} PRIVATE -lSDL2)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)

set_target_properties(${TARGET}
                      PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

install(TARGETS ${TARGET}
        RUNTIME DESTINATION bin)