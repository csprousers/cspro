project(zAction)

include(../WASM/Definitions.cmake)

add_library(${PROJECT_NAME}
    ActionFunctionMapping.cpp
    ActionInvoker.cpp
    Application.cpp
    Caller.cpp
    Clipboard.cpp
    Data.cpp
    Dictionary.cpp
    Encoding.cpp
    File.cpp
    Hash.cpp
    Localhost.cpp
    Logic.cpp
    Path.cpp
    Message.cpp
    Settings.cpp
    Sqlite.cpp
    System.cpp
    UserInterface.cpp
)
