project(zAction)

add_library(${PROJECT_NAME}
    ActionFunctionMapping.cpp
    ActionInvoker.cpp
    Application.cpp
    Caller.cpp
    CallerWrappingCaseConstructionReporter.cpp
    Clipboard.cpp
    Data.cpp
    Dictionary.cpp
    Encoding.cpp
    Exception.cpp
    File.cpp
    Hash.cpp
    JsonExecutor.cpp
    Localhost.cpp
    Logic.cpp
    Message.cpp
    Network.cpp
    Path.cpp
    Settings.cpp
    Sqlite.cpp
    Sync.cpp
    System.cpp
    UserInterface.cpp
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    stdafx.h
)
