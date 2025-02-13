project(zJson)

include(../WASM/Definitions.cmake)

include_directories(${PROJECT_NAME} PUBLIC
    ${EXTERNAL_DIRECTORY}
    ${EXTERNAL_DIRECTORY}/jsoncons
)

add_library(${PROJECT_NAME} 
	Json.cpp
	JsonFormattingOptions.cpp
	JsonNode.cpp
	JsonObjectCreator.cpp
	JsonSpecFile.cpp
	JsonStream.cpp
)
