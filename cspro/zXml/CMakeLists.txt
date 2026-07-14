project(zXml)

set(PUGIXML_DIRECTORY ${EXTERNAL_DIRECTORY}/pugixml)

add_library(${PROJECT_NAME}
    SimpleXml.cpp

    ${PUGIXML_DIRECTORY}/pugixml.cpp
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    StdAfx.h
)

set_source_files_properties(
    ${PUGIXML_DIRECTORY}/pugixml.cpp
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON
)
