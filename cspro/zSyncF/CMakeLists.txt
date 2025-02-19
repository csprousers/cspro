project(zSyncF)

include(../WASM/Definitions.cmake)

add_library(${PROJECT_NAME}
    DialogBasedSyncListener.cpp
    SyncLoginAccessor.cpp
)
