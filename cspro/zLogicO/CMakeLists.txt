project(zLogicO)

include(../WASM/Definitions.cmake)

add_library(${PROJECT_NAME}
    ActionInvoker.cpp
    BaseCompiler.cpp
    BasicTokenCompiler.cpp
    FunctionTable.cpp
    GeneralizedFunction.cpp
    KeywordTable.cpp
    Preprocessor.cpp
    ReservedWords.cpp
    SourceBuffer.cpp
    Symbol.cpp
    SymbolTable.cpp
    SymbolType.cpp
)
