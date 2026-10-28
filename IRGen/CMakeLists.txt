find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION} at ${LLVM_DIR}")

set(IRGEN_TARGET app_ir_gen)

add_executable(${IRGEN_TARGET}
               ${CMAKE_CURRENT_SOURCE_DIR}/app_ir_gen.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/../src/sim.c)

target_include_directories(${IRGEN_TARGET} PRIVATE
                           ${CMAKE_CURRENT_SOURCE_DIR}/../include
                           ${LLVM_INCLUDE_DIRS})

separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND "${LLVM_DEFINITIONS}")
target_compile_definitions(${IRGEN_TARGET} PRIVATE ${LLVM_DEFINITIONS_LIST})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
target_compile_options(${IRGEN_TARGET} PRIVATE
    -g
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
)

target_link_libraries(${IRGEN_TARGET} PRIVATE LLVM)
target_link_options(${IRGEN_TARGET} PRIVATE -lSDL2)

set_target_properties(${IRGEN_TARGET}
                      PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
