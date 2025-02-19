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
    Exception.cpp
    File.cpp
    Hash.cpp
    Localhost.cpp
    Logic.cpp
    Path.cpp
    Message.cpp
    Network.cpp
    Settings.cpp
    Sqlite.cpp
    Sync.cpp
    System.cpp
    UserInterface.cpp
)
