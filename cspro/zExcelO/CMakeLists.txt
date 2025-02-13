project(zExcelO)

include(../WASM/Definitions.cmake)

set(LIBXLSXWRITER_DIRECTORY ${EXTERNAL_DIRECTORY}/libxlsxwriter)

include_directories(${PROJECT_NAME} PUBLIC
    ${LIBXLSXWRITER_DIRECTORY}/include
    ${EXTERNAL_DIRECTORY}/zlib
)

add_compile_definitions(IOAPI_NO_64)
add_compile_definitions(NOUNCRYPT)

add_library(${PROJECT_NAME}
    ExcelWriter.cpp

    ${LIBXLSXWRITER_DIRECTORY}/src/app.c
    ${LIBXLSXWRITER_DIRECTORY}/src/chart.c
    ${LIBXLSXWRITER_DIRECTORY}/src/chartsheet.c
    ${LIBXLSXWRITER_DIRECTORY}/src/comment.c
    ${LIBXLSXWRITER_DIRECTORY}/src/content_types.c
    ${LIBXLSXWRITER_DIRECTORY}/src/core.c
    ${LIBXLSXWRITER_DIRECTORY}/src/custom.c
    ${LIBXLSXWRITER_DIRECTORY}/src/drawing.c
    ${LIBXLSXWRITER_DIRECTORY}/src/format.c
    ${LIBXLSXWRITER_DIRECTORY}/src/hash_table.c
    ${LIBXLSXWRITER_DIRECTORY}/src/metadata.c
    ${LIBXLSXWRITER_DIRECTORY}/src/packager.c
    ${LIBXLSXWRITER_DIRECTORY}/src/relationships.c
    ${LIBXLSXWRITER_DIRECTORY}/src/shared_strings.c
    ${LIBXLSXWRITER_DIRECTORY}/src/styles.c
    ${LIBXLSXWRITER_DIRECTORY}/src/table.c
    ${LIBXLSXWRITER_DIRECTORY}/src/theme.c
    ${LIBXLSXWRITER_DIRECTORY}/src/utility.c
    ${LIBXLSXWRITER_DIRECTORY}/src/vml.c
    ${LIBXLSXWRITER_DIRECTORY}/src/workbook.c
    ${LIBXLSXWRITER_DIRECTORY}/src/worksheet.c
    ${LIBXLSXWRITER_DIRECTORY}/src/xmlwriter.c
    ${LIBXLSXWRITER_DIRECTORY}/third_party/md5/md5.c
    ${LIBXLSXWRITER_DIRECTORY}/third_party/minizip/ioapi.c
    ${LIBXLSXWRITER_DIRECTORY}/third_party/minizip/zip.c
    ${LIBXLSXWRITER_DIRECTORY}/third_party/tmpfileplus/tmpfileplus.c
)
