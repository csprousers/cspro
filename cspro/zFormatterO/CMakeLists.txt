project(zFormatterO)

add_library(${PROJECT_NAME}
    QuestionnaireContentCreator.cpp
    QuestionnaireViewer.cpp
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    stdafx.h
)
