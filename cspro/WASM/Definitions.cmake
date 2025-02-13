
get_filename_component(CSPRO_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}.. DIRECTORY)
set(EXTERNAL_DIRECTORY ${CSPRO_DIRECTORY}/external)

include_directories(${PROJECT_NAME} PUBLIC 
    ${CSPRO_DIRECTORY}
)

add_compile_definitions(WASM)

add_compile_definitions(UNICODE=1)
add_compile_definitions(_UNICODE=1)

add_compile_definitions(GENERATE_BINARY=1)
add_compile_definitions(USE_BINARY=1)

# enable exceptions
add_compile_options(-fwasm-exceptions)
