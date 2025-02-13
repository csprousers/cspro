project(zEngineF)

include(../WASM/Definitions.cmake)

add_library(${PROJECT_NAME}
    EngineUI.cpp
    EngineUIPortable.cpp
    ErrmsgDlg.cpp
    PortableUserbar.cpp
    ReviewNotesDlg.cpp
    TraceHandler.cpp
)
