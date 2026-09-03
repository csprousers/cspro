project(zExcelO)

set(LIBXLSXWRITER_DIR ${EXTERNAL_DIR}/libxlsxwriter)

add_library(${PROJECT_NAME}
    ExcelWriter.cpp

    ${LIBXLSXWRITER_DIR}/src/app.c
    ${LIBXLSXWRITER_DIR}/src/chart.c
    ${LIBXLSXWRITER_DIR}/src/chartsheet.c
    ${LIBXLSXWRITER_DIR}/src/comment.c
    ${LIBXLSXWRITER_DIR}/src/content_types.c
    ${LIBXLSXWRITER_DIR}/src/core.c
    ${LIBXLSXWRITER_DIR}/src/custom.c
    ${LIBXLSXWRITER_DIR}/src/drawing.c
    ${LIBXLSXWRITER_DIR}/src/format.c
    ${LIBXLSXWRITER_DIR}/src/hash_table.c
    ${LIBXLSXWRITER_DIR}/src/metadata.c
    ${LIBXLSXWRITER_DIR}/src/packager.c
    ${LIBXLSXWRITER_DIR}/src/relationships.c
    ${LIBXLSXWRITER_DIR}/src/rich_value.c
    ${LIBXLSXWRITER_DIR}/src/rich_value_rel.c
    ${LIBXLSXWRITER_DIR}/src/rich_value_structure.c
    ${LIBXLSXWRITER_DIR}/src/rich_value_types.c
    ${LIBXLSXWRITER_DIR}/src/shared_strings.c
    ${LIBXLSXWRITER_DIR}/src/styles.c
    ${LIBXLSXWRITER_DIR}/src/table.c
    ${LIBXLSXWRITER_DIR}/src/theme.c
    ${LIBXLSXWRITER_DIR}/src/utility.c
    ${LIBXLSXWRITER_DIR}/src/vml.c
    ${LIBXLSXWRITER_DIR}/src/workbook.c
    ${LIBXLSXWRITER_DIR}/src/worksheet.c
    ${LIBXLSXWRITER_DIR}/src/xmlwriter.c
    ${LIBXLSXWRITER_DIR}/third_party/md5/md5.c
    ${LIBXLSXWRITER_DIR}/third_party/minizip/ioapi.c
    $<$<BOOL:${WIN32}>:${LIBXLSXWRITER_DIR}/third_party/minizip/iowin32.c>
    ${LIBXLSXWRITER_DIR}/third_party/minizip/zip.c
    ${LIBXLSXWRITER_DIR}/third_party/tmpfileplus/tmpfileplus.c
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<NOT:$<BOOL:${WIN32}>>:IOAPI_NO_64>
    NOUNCRYPT
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${LIBXLSXWRITER_DIR}/include
    ${EXTERNAL_DIR}/zlib
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h>"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    # zlib
    ${THIRD_PARTY_LIB_DIR}/libz${THIRD_PARTY_LIB_EXT}
)
