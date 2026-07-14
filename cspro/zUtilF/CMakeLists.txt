project(zUtilF)

add_library(${PROJECT_NAME}
    ChoiceDlg.cpp
    HtmlDialogFunctionRunner.cpp
    ImageCaptureDlg.cpp
    ImageViewDlg.cpp
    KeyboardLoader.cpp
    NoteEditDlg.cpp
    ProcessSummaryDlg.cpp
    ProgressDlgFactory.cpp
    SelectDlg.cpp
    SelectFileDlg.cpp
    TextInputDlg.cpp
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    StdAfx.h
)
