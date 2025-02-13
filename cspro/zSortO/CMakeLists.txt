project(zSortO)

include(../WASM/Definitions.cmake)

add_library(${PROJECT_NAME}
    SortableKeyDatabase.cpp
    Sorter.cpp
    SortSpec.cpp
)
