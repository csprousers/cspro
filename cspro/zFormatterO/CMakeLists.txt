project(zFormatterO)

include(../WASM/Definitions.cmake)

add_library(${PROJECT_NAME}
    QuestionnaireContentCreator.cpp
    QuestionnaireViewer.cpp
)
