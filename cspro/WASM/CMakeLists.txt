cmake_minimum_required(VERSION 3.8)

add_subdirectory(../rtf2html_dll rtf2html_dll)
add_subdirectory(../SQLite SQLite)
add_subdirectory(../yaml-cpp yaml-cpp)
add_subdirectory(zlib)
add_subdirectory(../engine engine)
add_subdirectory(../zAction zAction)
add_subdirectory(../zAppO zAppO)
add_subdirectory(../zBridgeO zBridgeO)
add_subdirectory(../zCaseO zCaseO)
add_subdirectory(../zConcatO zConcatO)
add_subdirectory(../zDataO zDataO)
add_subdirectory(../zDictO zDictO)
add_subdirectory(../zDiffO zDiffO)
add_subdirectory(../zEngineF zEngineF)
add_subdirectory(../zEngineO zEngineO)
add_subdirectory(../zExcelO zExcelO)
add_subdirectory(../zExportO zExportO)
add_subdirectory(../zFormatterO zFormatterO)
add_subdirectory(../zFormO zFormO)
add_subdirectory(../zFreqO zFreqO)
add_subdirectory(../zHtml zHtml)
add_subdirectory(../zIndexO zIndexO)
add_subdirectory(../zJavaScript zJavaScript)
add_subdirectory(../zJson zJson)
add_subdirectory(../zListingO zListingO)
add_subdirectory(../zLogicO zLogicO)
add_subdirectory(../zMapping zMapping)
add_subdirectory(../zMessageO zMessageO)
add_subdirectory(../zMultimediaO zMultimediaO)
add_subdirectory(../zNetwork zNetwork)
add_subdirectory(../zPackO zPackO)
add_subdirectory(../zParadataO zParadataO)
add_subdirectory(../zPlatformO zPlatformO)
add_subdirectory(../zReformatO zReformatO)
add_subdirectory(../zReportO zReportO)
add_subdirectory(../zSortO zSortO)
add_subdirectory(../zSyncO zSyncO)
add_subdirectory(../zToolsO zToolsO)
add_subdirectory(../zUtilF zUtilF)
add_subdirectory(../zUtilO zUtilO)
add_subdirectory(../zZipO zZipO)


project(CSPro)

include(Definitions.cmake)

add_executable(${PROJECT_NAME}
    "PortableLocalhostWasm.cpp"
    "WASM.cpp"
)


# enable exceptions
target_link_options(${PROJECT_NAME} PUBLIC -fwasm-exceptions)

# allow memory to grow as needed
target_link_options(${PROJECT_NAME} PUBLIC -sALLOW_MEMORY_GROWTH)

# bundle the assets
target_link_options(${PROJECT_NAME} PUBLIC --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/Assets@Assets/)


target_link_libraries(${PROJECT_NAME}
    rtf2html_dll
    SQLite
    yaml-cpp
    zlib
    engine
    zAction
    zAppO
    zBridgeO
    zCaseO
    zConcatO
    zDataO
    zDictO
    zDiffO
    zEngineF
    zEngineO
    zExcelO
    zExportO
    zFormatterO
    zFormO
    zFreqO
    zHtml
    zIndexO
    zJavaScript
    zJson
    zListingO
    zLogicO
    zMapping
    zMessageO
    zMultimediaO
    zNetwork
    zPackO
    zParadataO
    zPlatformO
    zReformatO
    zReportO
    zSortO
    zSyncO
    zToolsO
    zUtilF
    zUtilO
    zZipO
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
