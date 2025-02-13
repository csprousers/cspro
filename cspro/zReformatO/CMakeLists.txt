project(zReformatO)

include(../WASM/Definitions.cmake)

add_library(${PROJECT_NAME}
    Reformatter.cpp
    ToolReformatter.cpp
)
