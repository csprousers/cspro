project(zZip)

set(MINIZ_DIRECTORY ${EXTERNAL_DIRECTORY}/miniz)

add_library(${PROJECT_NAME}
    ZipException.cpp
    ZipFile.cpp
    ZLib.cpp

    ${MINIZ_DIRECTORY}/miniz.c
    ${MINIZ_DIRECTORY}/miniz_tdef.c
    ${MINIZ_DIRECTORY}/miniz_tinfl.c
    ${MINIZ_DIRECTORY}/miniz_zip.c
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h>"
)
