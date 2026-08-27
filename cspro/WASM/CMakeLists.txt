cmake_minimum_required(VERSION 3.22)

project(CSEntry LANGUAGES C CXX)

# Make sure that all environment variables are set.
if(NOT DEFINED ENV{EMSCRIPTEN_CMAKE_TOOLCHAIN_FILE} OR ENV{EMSCRIPTEN_CMAKE_TOOLCHAIN_FILE} STREQUAL "")
    message(FATAL_ERROR "Define the environment variable EMSCRIPTEN_CMAKE_TOOLCHAIN_FILE to build WASM libraries.")
endif()

# use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# CSPro build type
set(CSPRO_BUILD_WASM ON CACHE BOOL "")

# global compiler options
add_compile_options(
    # enable exception handling
    -fwasm-exceptions

    # allow multithreading using Wasm Workers
    -sWASM_WORKERS
    -pthread

    # require non-void functions to have a return statement
    -Werror=return-type

    # suppress warnings: expression result unused
    -Wno-unused-value

    # suppress warnings: enumeration values not handled in switch
    -Wno-switch
)

# global compiler definitions
add_compile_definitions(
    WASM=1
    CSPRO_NO_DLLS=1
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

# CSEntry (WASM) files
add_executable(${PROJECT_NAME}
    WasmBindings.cpp
    WasmController.cpp
    WasmLocalFileServer.cpp
)

set_target_properties(${PROJECT_NAME}
    # add JavaScript bindings
    PROPERTIES LINK_FLAGS -lembind
)

target_link_options(${PROJECT_NAME} PUBLIC
    # enable exceptions
    -fwasm-exceptions

    # allow memory to grow as needed
    -sALLOW_MEMORY_GROWTH

    # allow multithreading using Wasm Workers
    -sWASM_WORKERS
    -pthread

    # use the WasmFS file system, which allows access to the file system from multiple threads
    -sWASMFS

    # bundle the assets
    --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/Assets@Assets/
)

# copy the WASM html directory to the output root
add_custom_command(TARGET ${PROJECT_NAME}
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_CURRENT_SOURCE_DIR}/html ${CMAKE_CURRENT_BINARY_DIR}
)

# supporting libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    # external libraries
    zlib

    # CSPro libraries
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
    zMarkdown
    zMessageO
    zMultimediaO
    zNetwork
    zPackO
    zParadataO
    zPlatformO
    zReformatO
    zReportO
    zSortO
    zSql
    zSyncO
    zToolsO
    zUtilF
    zUtilO
    zViewO
    zXml
    zYaml
    zZip
)

# external libraries
add_subdirectory(${EXTERNAL_DIRECTORY}/zlib zlib)

# CSPro libraries
add_subdirectory(${CSPRO_DIRECTORY}/engine engine)
add_subdirectory(${CSPRO_DIRECTORY}/zAction zAction)
add_subdirectory(${CSPRO_DIRECTORY}/zAppO zAppO)
add_subdirectory(${CSPRO_DIRECTORY}/zBridgeO zBridgeO)
add_subdirectory(${CSPRO_DIRECTORY}/zCaseO zCaseO)
add_subdirectory(${CSPRO_DIRECTORY}/zConcatO zConcatO)
add_subdirectory(${CSPRO_DIRECTORY}/zDataO zDataO)
add_subdirectory(${CSPRO_DIRECTORY}/zDictO zDictO)
add_subdirectory(${CSPRO_DIRECTORY}/zDiffO zDiffO)
add_subdirectory(${CSPRO_DIRECTORY}/zEngineF zEngineF)
add_subdirectory(${CSPRO_DIRECTORY}/zEngineO zEngineO)
add_subdirectory(${CSPRO_DIRECTORY}/zExcelO zExcelO)
add_subdirectory(${CSPRO_DIRECTORY}/zExportO zExportO)
add_subdirectory(${CSPRO_DIRECTORY}/zFormatterO zFormatterO)
add_subdirectory(${CSPRO_DIRECTORY}/zFormO zFormO)
add_subdirectory(${CSPRO_DIRECTORY}/zFreqO zFreqO)
add_subdirectory(${CSPRO_DIRECTORY}/zHtml zHtml)
add_subdirectory(${CSPRO_DIRECTORY}/zIndexO zIndexO)
add_subdirectory(${CSPRO_DIRECTORY}/zJavaScript zJavaScript)
add_subdirectory(${CSPRO_DIRECTORY}/zJson zJson)
add_subdirectory(${CSPRO_DIRECTORY}/zListingO zListingO)
add_subdirectory(${CSPRO_DIRECTORY}/zLogicO zLogicO)
add_subdirectory(${CSPRO_DIRECTORY}/zMapping zMapping)
add_subdirectory(${CSPRO_DIRECTORY}/zMarkdown zMarkdown)
add_subdirectory(${CSPRO_DIRECTORY}/zMessageO zMessageO)
add_subdirectory(${CSPRO_DIRECTORY}/zMultimediaO zMultimediaO)
add_subdirectory(${CSPRO_DIRECTORY}/zNetwork zNetwork)
add_subdirectory(${CSPRO_DIRECTORY}/zPackO zPackO)
add_subdirectory(${CSPRO_DIRECTORY}/zParadataO zParadataO)
add_subdirectory(${CSPRO_DIRECTORY}/zPlatformO zPlatformO)
add_subdirectory(${CSPRO_DIRECTORY}/zReformatO zReformatO)
add_subdirectory(${CSPRO_DIRECTORY}/zReportO zReportO)
add_subdirectory(${CSPRO_DIRECTORY}/zSortO zSortO)
add_subdirectory(${CSPRO_DIRECTORY}/zSql zSql)
add_subdirectory(${CSPRO_DIRECTORY}/zSyncO zSyncO)
add_subdirectory(${CSPRO_DIRECTORY}/zToolsO zToolsO)
add_subdirectory(${CSPRO_DIRECTORY}/zUtilF zUtilF)
add_subdirectory(${CSPRO_DIRECTORY}/zUtilO zUtilO)
add_subdirectory(${CSPRO_DIRECTORY}/zViewO zViewO)
add_subdirectory(${CSPRO_DIRECTORY}/zXml zXml)
add_subdirectory(${CSPRO_DIRECTORY}/zYaml zYaml)
add_subdirectory(${CSPRO_DIRECTORY}/zZip zZip)
