project(zConcatO)

add_library(${PROJECT_NAME}
    CaseConcatenator.cpp
    Concatenator.cpp
    CSConcatReporter.cpp
    TextConcatenator.cpp
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    stdafx.h
)
