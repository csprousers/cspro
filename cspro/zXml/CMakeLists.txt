project(zXml)

set(PUGIXML_DIR ${EXTERNAL_DIR}/pugixml)

add_library(${PROJECT_NAME}
    SimpleXml.cpp

    ${PUGIXML_DIR}/pugixml.cpp
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    StdAfx.h
)

set_source_files_properties(
    ${PUGIXML_DIR}/pugixml.cpp
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON
)
