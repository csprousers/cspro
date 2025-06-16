project(zReportO)

include(../WASM/Definitions.cmake)

add_library(${PROJECT_NAME}
    Pre77Report.cpp
    Pre77ReportManager.cpp
    Pre77ReportNodes.cpp
)
