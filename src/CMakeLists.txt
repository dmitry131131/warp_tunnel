set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sim.c
            ${CMAKE_CURRENT_SOURCE_DIR}/app.c
            ${CMAKE_CURRENT_SOURCE_DIR}/runtime.c)

target_sources(${TARGET} PRIVATE ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/main.c)

set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/app.c
    TARGET_DIRECTORY ${TARGET}
    PROPERTIES COMPILE_OPTIONS "-Os;-fpass-plugin=/home/cybermeatball/Files/LLVMInstrumentationPass/libLLVMInstrumentationPass.so"
)