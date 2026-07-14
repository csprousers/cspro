cmake_minimum_required(VERSION 3.10)

project(CSPro)

# global compiler options
add_compile_options(
    # enable exception handling
    -fwasm-exceptions

    # allow multithreading using Wasm Workers
    -sWASM_WORKERS

    # require non-void functions to have a return statement
    -Werror=return-type

    # suppress warnings: expression result unused
    -Wno-unused-value

    # suppress warnings: enumeration values not handled in switch
    -Wno-switch
)

# global compiler definitions
add_compile_definitions(
    WASM
    UNICODE=1
    _UNICODE=1
)

# set the directories for the CSPro solution root and the external libraries
set(CSPRO_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(EXTERNAL_DIRECTORY ${CSPRO_DIRECTORY}/external)

# global include directories
include_directories(
    ${CSPRO_DIRECTORY}
)

# add_subdirectory(../external/zlib zlib)
# add_subdirectory(../engine engine)
# add_subdirectory(../zAction zAction)
add_subdirectory(../zAppO zAppO)
# add_subdirectory(../zBridgeO zBridgeO)
# add_subdirectory(../zCaseO zCaseO)
# add_subdirectory(../zConcatO zConcatO)
# add_subdirectory(../zDataO zDataO)
# add_subdirectory(../zDictO zDictO)
# add_subdirectory(../zDiffO zDiffO)
# add_subdirectory(../zEngineF zEngineF)
# add_subdirectory(../zEngineO zEngineO)
# add_subdirectory(../zExcelO zExcelO)
# add_subdirectory(../zExportO zExportO)
# add_subdirectory(../zFormatterO zFormatterO)
# add_subdirectory(../zFormO zFormO)
# add_subdirectory(../zFreqO zFreqO)
add_subdirectory(../zHtml zHtml)
# add_subdirectory(../zIndexO zIndexO)
# add_subdirectory(../zJavaScript zJavaScript)
add_subdirectory(../zJson zJson)
# add_subdirectory(../zListingO zListingO)
# add_subdirectory(../zLogicO zLogicO)
# add_subdirectory(../zMapping zMapping)
# add_subdirectory(../zMarkdown zMarkdown)
# add_subdirectory(../zMessageO zMessageO)
# add_subdirectory(../zMultimediaO zMultimediaO)
# add_subdirectory(../zNetwork zNetwork)
# add_subdirectory(../zPackO zPackO)
# add_subdirectory(../zParadataO zParadataO)
add_subdirectory(../zPlatformO zPlatformO)
# add_subdirectory(../zReformatO zReformatO)
# add_subdirectory(../zReportO zReportO)
# add_subdirectory(../zSortO zSortO)
add_subdirectory(../zSql zSql)
# add_subdirectory(../zSyncO zSyncO)
add_subdirectory(../zToolsO zToolsO)
# add_subdirectory(../zUtilF zUtilF)
add_subdirectory(../zUtilO zUtilO)
# add_subdirectory(../zViewO zViewO)
# add_subdirectory(../zYaml zYaml)
# add_subdirectory(../zZip zZip)

add_executable(${PROJECT_NAME}
    "WasmBindings.cpp"
    "WasmController.cpp"
    "WasmLocalFileServer.cpp"
)

# add JavaScript bindings
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS -lembind)

# enable exceptions
target_link_options(${PROJECT_NAME} PUBLIC -fwasm-exceptions)

# allow memory to grow as needed
target_link_options(${PROJECT_NAME} PUBLIC -sALLOW_MEMORY_GROWTH)

# allow multithreading using Wasm Workers
target_link_options(${PROJECT_NAME} PUBLIC -sWASM_WORKERS)

# use the WasmFS file system, which allows access to the file system from multiple threads
target_link_options(${PROJECT_NAME} PUBLIC -sWASMFS)

# bundle the assets
target_link_options(${PROJECT_NAME} PUBLIC --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/Assets@Assets/)

# copy the WASM html directory to the output root
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_CURRENT_SOURCE_DIR}/html ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(${PROJECT_NAME}
    # zlib
    # engine
    # zAction
    zAppO
    # zBridgeO
    # zCaseO
    # zConcatO
    # zDataO
    # zDictO
    # zDiffO
    # zEngineF
    # zEngineO
    # zExcelO
    # zExportO
    # zFormatterO
    # zFormO
    # zFreqO
    zHtml
    # zIndexO
    # zJavaScript
    zJson
    # zListingO
    # zLogicO
    # zMapping
    # zMarkdown
    # zMessageO
    # zMultimediaO
    # zNetwork
    # zPackO
    # zParadataO
    zPlatformO
    # zReformatO
    # zReportO
    # zSortO
    zSql
    # zSyncO
    zToolsO
    # zUtilF
    # zViewO
    zUtilO
    # zYaml
    # zZip
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
