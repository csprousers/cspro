project(zAppO)

include(../WASM/Definitions.cmake)

include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

add_library(${PROJECT_NAME}
    AppFileType.cpp
    Application.cpp
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

    Properties/ApplicationProperties.cpp
    Properties/JsonProperties.cpp
    Properties/MappingProperties.cpp
    Properties/MappingTileProviderProperties.cpp
    Properties/ParadataProperties.cpp
)
