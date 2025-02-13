project(zPackO)

include(../WASM/Definitions.cmake)

add_library(${PROJECT_NAME}
    PackEntry.cpp
    Packer.cpp
    PackSpec.cpp
)
