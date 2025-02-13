project(zCaseO)

include(../WASM/Definitions.cmake)

add_library(${PROJECT_NAME}
    BinaryCaseItem.cpp
    Case.cpp
    CaseAccess.cpp
    CaseIO.cpp
    CaseItem.cpp
    CaseItemIndex.cpp
    CaseItemPrinter.cpp
    CaseItemReference.cpp
    CaseJsonSerializer.cpp
    CaseLevel.cpp
    CaseRecord.cpp
    FixedWidthNumericCaseItem.cpp
    FixedWidthNumericWithStringBufferCaseItem.cpp
    FixedWidthStringCaseItem.cpp
    ItemSubitemSyncTask.cpp
    NumericCaseItem.cpp
    Pre74_Case.cpp
    StringBasedCaseConstructionReporter.cpp
    StringCaseItem.cpp
    TextToCaseConverter.cpp
    VectorClock.cpp
)
