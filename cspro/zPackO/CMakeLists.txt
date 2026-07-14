project(zPackO)

add_library(${PROJECT_NAME}
    PackEntry.cpp
    Packer.cpp
    PackSpec.cpp
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    stdafx.h
)
