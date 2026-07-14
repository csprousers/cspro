project(zIndexO)

add_library(${PROJECT_NAME}
    Indexer.cpp
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    stdafx.h
)
