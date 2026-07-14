project(zAppO)

include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

add_library(${PROJECT_NAME}
    AppFileType.cpp
    Application.cpp
    AppMessageFile.cpp
    AppResource.cpp
    AppSyncParameters.cpp
    CodeFile.cpp
    DictionaryDescription.cpp
    EnumJsonSerializers.cpp
    LabelSet.cpp
    Language.cpp
    LogicSettings.cpp
    MappingDefines.cpp
    OccurrenceLabels.cpp
    PFF.cpp
    Pre80SpecFile.cpp
    ReportFile.cpp

    Properties/ApplicationProperties.cpp
    Properties/JavaScriptProperties.cpp
    Properties/JsonProperties.cpp
    Properties/MappingProperties.cpp
    Properties/MappingTileProviderProperties.cpp
    Properties/ParadataProperties.cpp
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    stdafx.h
)
