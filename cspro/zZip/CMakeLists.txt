project(zZip)

add_library(${PROJECT_NAME}
    ZipException.cpp
    ZipFile.cpp
    ZLib.cpp
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    stdafx.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    # miniz
    ${THIRD_PARTY_LIB_DIR}/libminiz${THIRD_PARTY_LIB_EXT}
)
