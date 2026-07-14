project(zXml)

set(PUGIXML_DIRECTORY ${EXTERNAL_DIRECTORY}/pugixml)

add_library(${PROJECT_NAME}
    SimpleXml.cpp

    ${PUGIXML_DIRECTORY}/pugixml.cpp
)
