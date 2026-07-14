project(zBridgeO)

add_library(${PROJECT_NAME}
    NPff.cpp
    RunApl.cpp
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    StdAfx.h
)
