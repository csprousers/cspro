project(zConcatO)

include(../WASM/Definitions.cmake)

add_library(${PROJECT_NAME}
    CaseConcatenator.cpp
    Concatenator.cpp
    CSConcatReporter.cpp
    DuplicateCaseChecker.cpp
    TextConcatenator.cpp
)
