project(zMessageO)

add_library(${PROJECT_NAME}
    MessageEvaluator.cpp
    MessageFile.cpp
    MessageManager.cpp
    Messages.cpp
    SystemMessageFormatter.cpp
    SystemMessages.cpp
)
