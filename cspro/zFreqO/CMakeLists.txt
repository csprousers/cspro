project(zFreqO)

add_library(${PROJECT_NAME}
    ExcelFrequencyPrinter.cpp
    Frequency.cpp
    FrequencyCounter.cpp
    FrequencyPrinterEntry.cpp
    FrequencyPrinterHelpers.cpp
    FrequencyPrinterOptions.cpp
    HtmlFrequencyPrinter.cpp
    JsonFileFrequencyPrinter.cpp
    JsonFrequencyPrinter.cpp
    JsonStringFrequencyPrinter.cpp
    TextFrequencyPrinter.cpp
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    stdafx.h
)
