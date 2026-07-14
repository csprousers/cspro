project(zLogicO)

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
    SpecialFunction.cpp
    Symbol.cpp
    SymbolTable.cpp
    SymbolType.cpp
    TextTemplateTokenizer.cpp
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    stdafx.h
)
