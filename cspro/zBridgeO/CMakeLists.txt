project(zBridgeO)

include(../WASM/Definitions.cmake)

add_library(${PROJECT_NAME}
    NPff.cpp
    RunApl.cpp
)
