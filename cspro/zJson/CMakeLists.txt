project(zJson)

include_directories(${PROJECT_NAME} PUBLIC
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
