project(zMessageO)

include(../WASM/Definitions.cmake)

add_library(${PROJECT_NAME} 
    MessageEvaluator.cpp
    MessageFile.cpp
    MessageManager.cpp
    Messages.cpp
    SystemMessageIssuer.cpp
    SystemMessages.cpp
)
