project(zJson)

add_library(${PROJECT_NAME}
    IncrementalJsonObjectArrayParser.cpp
    Json.cpp
    JsonFormattingOptions.cpp
    JsonNode.cpp
    JsonObjectCreator.cpp
    JsonSpecFile.cpp
    JsonStream.cpp
    JsonWriter.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${EXTERNAL_DIRECTORY}
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    stdafx.h
)
