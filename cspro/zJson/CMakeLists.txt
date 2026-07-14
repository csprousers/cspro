project(zJson)

include_directories(${PROJECT_NAME} PRIVATE
    ${EXTERNAL_DIRECTORY}
)

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
