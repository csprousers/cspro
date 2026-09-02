project(zXml)

add_library(${PROJECT_NAME}
    SimpleXml.cpp
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    StdAfx.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    # pugixml
    ${THIRD_PARTY_LIB_DIR}/libpugixml${THIRD_PARTY_LIB_EXT}
)
