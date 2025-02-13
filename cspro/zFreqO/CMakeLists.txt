project(zFreqO)

include(../WASM/Definitions.cmake)

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
