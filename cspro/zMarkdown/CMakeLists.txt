project(zMarkdown)

set(MD4C_DIRECTORY ${EXTERNAL_DIRECTORY}/md4c)

add_library(${PROJECT_NAME}
    HtmlParserCallback.cpp
    Markdown.cpp

    ${MD4C_DIRECTORY}/md4c.c
    ${MD4C_DIRECTORY}/md4c-html.c
)
