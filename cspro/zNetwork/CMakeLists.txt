project(zNetwork)

include(../WASM/Definitions.cmake)

add_library(${PROJECT_NAME} 
	FileInfo.cpp
    HeaderList.cpp
)
