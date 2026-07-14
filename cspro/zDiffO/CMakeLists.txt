project(zDiffO)

add_library(${PROJECT_NAME}
    Differ.cpp
    DiffSpec.cpp
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    stdafx.h
)
