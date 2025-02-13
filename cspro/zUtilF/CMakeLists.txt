project(zUtilF)

include(../WASM/Definitions.cmake)

add_library(${PROJECT_NAME}
    ChoiceDlg.cpp
    HtmlDialogFunctionRunner.cpp
    ImageCaptureDlg.cpp
    ImageViewDlg.cpp
    NoteEditDlg.cpp
    ProcessSummaryDlg.cpp
    ProgressDlgFactory.cpp
    SelectDlg.cpp
    SelectFileDlg.cpp
    TextInputDlg.cpp
)
