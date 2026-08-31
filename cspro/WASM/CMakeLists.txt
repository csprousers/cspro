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
set(CSPRO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(EXTERNAL_DIR ${CSPRO_DIR}/external)
set(THIRD_PARTY_INCLUDE_DIR ${CSPRO_DIR}/../third_party/include)
set(THIRD_PARTY_LIB_DIR ${CSPRO_DIR}/../third_party/lib/wasm)
set(THIRD_PARTY_LIB_EXT "$<IF:$<CONFIG:Debug>,d.a,.a>")

# global include directories
include_directories(
    ${CSPRO_DIR}
    ${THIRD_PARTY_INCLUDE_DIR}
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
add_subdirectory(${EXTERNAL_DIR}/zlib zlib)

# CSPro libraries
add_subdirectory(${CSPRO_DIR}/engine engine)
add_subdirectory(${CSPRO_DIR}/zAction zAction)
add_subdirectory(${CSPRO_DIR}/zAppO zAppO)
add_subdirectory(${CSPRO_DIR}/zBridgeO zBridgeO)
add_subdirectory(${CSPRO_DIR}/zCaseO zCaseO)
add_subdirectory(${CSPRO_DIR}/zConcatO zConcatO)
add_subdirectory(${CSPRO_DIR}/zDataO zDataO)
add_subdirectory(${CSPRO_DIR}/zDictO zDictO)
add_subdirectory(${CSPRO_DIR}/zDiffO zDiffO)
add_subdirectory(${CSPRO_DIR}/zEngineF zEngineF)
add_subdirectory(${CSPRO_DIR}/zEngineO zEngineO)
add_subdirectory(${CSPRO_DIR}/zExcelO zExcelO)
add_subdirectory(${CSPRO_DIR}/zExportO zExportO)
add_subdirectory(${CSPRO_DIR}/zFormatterO zFormatterO)
add_subdirectory(${CSPRO_DIR}/zFormO zFormO)
add_subdirectory(${CSPRO_DIR}/zFreqO zFreqO)
add_subdirectory(${CSPRO_DIR}/zHtml zHtml)
add_subdirectory(${CSPRO_DIR}/zIndexO zIndexO)
add_subdirectory(${CSPRO_DIR}/zJavaScript zJavaScript)
add_subdirectory(${CSPRO_DIR}/zJson zJson)
add_subdirectory(${CSPRO_DIR}/zListingO zListingO)
add_subdirectory(${CSPRO_DIR}/zLogicO zLogicO)
add_subdirectory(${CSPRO_DIR}/zMapping zMapping)
add_subdirectory(${CSPRO_DIR}/zMarkdown zMarkdown)
add_subdirectory(${CSPRO_DIR}/zMessageO zMessageO)
add_subdirectory(${CSPRO_DIR}/zMultimediaO zMultimediaO)
add_subdirectory(${CSPRO_DIR}/zNetwork zNetwork)
add_subdirectory(${CSPRO_DIR}/zPackO zPackO)
add_subdirectory(${CSPRO_DIR}/zParadataO zParadataO)
add_subdirectory(${CSPRO_DIR}/zPlatformO zPlatformO)
add_subdirectory(${CSPRO_DIR}/zReformatO zReformatO)
add_subdirectory(${CSPRO_DIR}/zReportO zReportO)
add_subdirectory(${CSPRO_DIR}/zSortO zSortO)
add_subdirectory(${CSPRO_DIR}/zSql zSql)
add_subdirectory(${CSPRO_DIR}/zSyncO zSyncO)
add_subdirectory(${CSPRO_DIR}/zToolsO zToolsO)
add_subdirectory(${CSPRO_DIR}/zUtilF zUtilF)
add_subdirectory(${CSPRO_DIR}/zUtilO zUtilO)
add_subdirectory(${CSPRO_DIR}/zViewO zViewO)
add_subdirectory(${CSPRO_DIR}/zXml zXml)
add_subdirectory(${CSPRO_DIR}/zYaml zYaml)
add_subdirectory(${CSPRO_DIR}/zZip zZip)
