project(zDictO)

include(../WASM/Definitions.cmake)

add_library(${PROJECT_NAME}
    CaptureInfo.cpp
    DDClass.cpp
    Definitions.cpp
    DictBase.cpp
    DictionaryComparer.cpp
    DictionaryValidator.cpp
    DictItem.cpp
    DictLevel.cpp
    DictNamedBase.cpp
    DictRecord.cpp
    DictRelation.cpp
    DictValue.cpp
    DictValuePair.cpp
    DictValueSet.cpp
    ItemIndexHelper.cpp
    Pre80SpecFile.cpp
    ValueProcessor.cpp
    ValueSetFixer.cpp
    ValueSetResponse.cpp
)
