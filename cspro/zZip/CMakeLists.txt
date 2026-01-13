project(zZip)

include(../WASM/Definitions.cmake)

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
