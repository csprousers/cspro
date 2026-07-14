project(zSortO)

add_library(${PROJECT_NAME}
    SortableKeyDatabase.cpp
    Sorter.cpp
    SortSpec.cpp
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    stdafx.h
)
