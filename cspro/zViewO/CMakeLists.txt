project(zViewO)

add_library(${PROJECT_NAME}
    MarkdownViewInput.cpp
    ViewInput.cpp
    ViewInputCreator.cpp
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    StdAfx.h
)
