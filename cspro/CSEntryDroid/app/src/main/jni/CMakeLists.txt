cmake_minimum_required(VERSION 3.22)

project(CSEntry LANGUAGES C CXX)

# use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# use ccache when available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

# global compiler options
add_compile_options(
    # use Run-Time Type Information
    -frtti

    # enable exception handling
    -fexceptions

    # require non-void functions to have a return statement
    -Werror=return-type

    # suppress warnings: expression result unused
    -Wno-unused-value

    # suppress warnings: enumeration values not handled in switch
    -Wno-switch
)

# global compiler definitions
add_compile_definitions(
    ANDROID=1
    CSPRO_NO_DLLS=1
    UNICODE=1
    _UNICODE=1
)

# set the directories for the CSPro solution root and the external libraries
set(CSPRO_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../../../..)
set(EXTERNAL_DIRECTORY ${CSPRO_DIRECTORY}/external)

# global include directories
include_directories(
    ${CSPRO_DIRECTORY}
)

# CSEntry (Android) files
add_library(${PROJECT_NAME} SHARED
    ActionInvoker.cpp
    ActionInvokerPortableRunner.cpp
    AndroidApplicationInterface.cpp
    AndroidBluetoothAdapter.cpp
    AndroidBluetoothObexTransport.cpp
    AndroidEngineInterface.cpp
    AndroidFtpConnection.cpp
    AndroidHtmlMapUI.cpp
    AndroidHttpConnection.cpp
    AndroidLocalFileServer.cpp
    AndroidMapUI.cpp
    GeometryJni.cpp
    gov_census_cspro_bridge_CNPifFile.cpp
    gov_census_cspro_engine_EngineInterface_jni.cpp
    gov_census_cspro_form_CDEField_jni.cpp
    gov_census_cspro_form_EntryPage_jni.cpp
    gov_census_cspro_smartsync_addapp_DeploymentPackageDownloader.cpp
    gov_census_cspro_smartsync_http_IStreamWrapper_jni.cpp
    gov_census_cspro_smartsync_http_OStreamWrapper_jni.cpp
    gov_census_cspro_smartsync_p2p_AndroidBluetoothAdapter_jni.cpp
    gov_census_cspro_smartsync_SyncListenerWrapper_jni.cpp
    JNIHelpers.cpp
)

# CSEntry (Android) include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${EXTERNAL_DIRECTORY}
    ${EXTERNAL_DIRECTORY}/rxcpp
    ${EXTERNAL_DIRECTORY}/geometry.hpp/include
    ${EXTERNAL_DIRECTORY}/variant/include
)

# enable 16 KiB memory pages
target_link_options(${PROJECT_NAME} PRIVATE
    "-Wl,-z,max-page-size=16384"
)

# supporting libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    # Android libraries
    android
    log

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
