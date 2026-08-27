project(zEngineF)

add_library(${PROJECT_NAME}
    EngineUI.cpp
    EngineUIPortable.cpp
    ErrmsgDlg.cpp
    PortableUserbar.cpp
    ReviewNotesDlg.cpp
    TraceHandler.cpp
    $<$<BOOL:${WIN32}>:WindowsApplicationInterface.cpp>
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    StdAfx.h
)
