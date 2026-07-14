project(zListingO)

add_library(${PROJECT_NAME}
    CsvLister.cpp
    DataFileLister.cpp
    ErrorLister.cpp
    ExcelLister.cpp
    HtmlLister.cpp
    JsonLister.cpp
    Lister.cpp
    ListingHelpers.cpp
    TextLister.cpp
    TextWriteFile.cpp
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    stdafx.h
)
