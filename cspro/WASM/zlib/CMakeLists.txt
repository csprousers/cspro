project(zlib)

include(../Definitions.cmake)

set(ZLIB_DIRECTORY ${EXTERNAL_DIRECTORY}/zlib)

add_library(${PROJECT_NAME}
    ${ZLIB_DIRECTORY}/adler32.c
    ${ZLIB_DIRECTORY}/compress.c
    ${ZLIB_DIRECTORY}/crc32.c
    ${ZLIB_DIRECTORY}/deflate.c
    ${ZLIB_DIRECTORY}/gzclose.c
    ${ZLIB_DIRECTORY}/gzlib.c
    ${ZLIB_DIRECTORY}/gzread.c
    ${ZLIB_DIRECTORY}/gzwrite.c
    ${ZLIB_DIRECTORY}/infback.c
    ${ZLIB_DIRECTORY}/inffast.c
    ${ZLIB_DIRECTORY}/inflate.c
    ${ZLIB_DIRECTORY}/inftrees.c
    ${ZLIB_DIRECTORY}/trees.c
    ${ZLIB_DIRECTORY}/uncompr.c
    ${ZLIB_DIRECTORY}/zutil.c
)
