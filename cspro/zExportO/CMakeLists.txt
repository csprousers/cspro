project(zExportO)

set(LIBRDATA_READSTAT_DIR ${THIRD_PARTY_SOURCES_DIR}/librdata+readstat)
set(LIBRDATA_DIR ${THIRD_PARTY_SOURCES_DIR}/librdata)
set(READSTAT_DIR ${THIRD_PARTY_SOURCES_DIR}/readstat)

add_library(${PROJECT_NAME}
    CSProExportWriter.cpp
    DelimitedTextExportWriter.cpp
    ExcelExportWriter.cpp
    ExportDefinitions.cpp
    ExportWriterBase.cpp
    ReadStatExportWriterBase.cpp
    RExportWriter.cpp
    SasExportWriter.cpp
    SasTransportWriter.cpp
    SingleRecordExportWriterBase.cpp
    SpssExportWriter.cpp
    StataExportWriter.cpp

    ${LIBRDATA_READSTAT_DIR}/CKHashTable.c
    ${LIBRDATA_READSTAT_DIR}/readstat_bits.c

    ${LIBRDATA_DIR}/rdata_error.c
    ${LIBRDATA_DIR}/rdata_io_unistd.c
    ${LIBRDATA_DIR}/rdata_parser.c
    ${LIBRDATA_DIR}/rdata_write.c

    ${READSTAT_DIR}/readstat_error.c
    ${READSTAT_DIR}/readstat_io_unistd.c
    ${READSTAT_DIR}/readstat_malloc.c
    ${READSTAT_DIR}/readstat_metadata.c
    ${READSTAT_DIR}/readstat_parser.c
    ${READSTAT_DIR}/readstat_value.c
    ${READSTAT_DIR}/readstat_variable.c
    ${READSTAT_DIR}/readstat_writer.c
    ${READSTAT_DIR}/sas/ieee.c
    ${READSTAT_DIR}/spss/readstat_sav_compress.c
    ${READSTAT_DIR}/spss/readstat_sav_write.c
    ${READSTAT_DIR}/spss/readstat_spss.c
    ${READSTAT_DIR}/spss/readstat_spss_parse.c
    ${READSTAT_DIR}/stata/readstat_dta.c
    ${READSTAT_DIR}/stata/readstat_dta_write.c
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h>"
)
