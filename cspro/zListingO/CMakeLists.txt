project(zListingO)

include(../WASM/Definitions.cmake)

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
