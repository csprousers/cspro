project(zZip)

set(MINIZ_DIR ${EXTERNAL_DIR}/miniz)

add_library(${PROJECT_NAME}
    ZipException.cpp
    ZipFile.cpp
    ZLib.cpp

    ${MINIZ_DIR}/miniz.c
    ${MINIZ_DIR}/miniz_tdef.c
    ${MINIZ_DIR}/miniz_tinfl.c
    ${MINIZ_DIR}/miniz_zip.c
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h>"
)
