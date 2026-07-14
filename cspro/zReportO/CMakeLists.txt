project(zReportO)

add_library(${PROJECT_NAME}
    Pre77Report.cpp
    Pre77ReportManager.cpp
    Pre77ReportNodes.cpp
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    stdafx.h
)
