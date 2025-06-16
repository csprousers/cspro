project(zBridgeO)

include(../WASM/Definitions.cmake)

add_library(${PROJECT_NAME}
    NPFF.cpp
    RunApl.cpp
)
