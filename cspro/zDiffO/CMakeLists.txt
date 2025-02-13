project(zDiffO)

include(../WASM/Definitions.cmake)

add_library(${PROJECT_NAME}
    Differ.cpp
    DiffSpec.cpp
)
