cmake_minimum_required(VERSION 3.8)

# add_subdirectory(zlib)
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
# add_subdirectory(../zSyncF zSyncF)
# add_subdirectory(../zSyncO zSyncO)
add_subdirectory(../zToolsO zToolsO)
# add_subdirectory(../zUtilF zUtilF)
add_subdirectory(../zUtilO zUtilO)
# add_subdirectory(../zYaml zYaml)
# add_subdirectory(../zZipO zZipO)


project(CSPro)

include(Definitions.cmake)

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
    # zSyncF
    # zSyncO
    zToolsO
    # zUtilF
    zUtilO
    # zYaml
    # zZipO
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
