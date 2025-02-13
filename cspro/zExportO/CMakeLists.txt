project(zExportO)

include(../WASM/Definitions.cmake)

set(LIBRDATA_READSTAT_DIRECTORY ${EXTERNAL_DIRECTORY}/librdata+ReadStat)
set(LIBRDATA_DIRECTORY ${EXTERNAL_DIRECTORY}/librdata)
set(READSTAT_DIRECTORY ${EXTERNAL_DIRECTORY}/ReadStat)

add_library(${PROJECT_NAME}
    CSProExportWriter.cpp
    DelimitedTextExportWriter.cpp
    EncodedTextWriter.cpp
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

    ${LIBRDATA_READSTAT_DIRECTORY}/CKHashTable.c
    ${LIBRDATA_READSTAT_DIRECTORY}/readstat_bits.c

    ${LIBRDATA_DIRECTORY}/rdata_error.c
    ${LIBRDATA_DIRECTORY}/rdata_io_unistd.c
    ${LIBRDATA_DIRECTORY}/rdata_parser.c
    ${LIBRDATA_DIRECTORY}/rdata_write.c

    ${READSTAT_DIRECTORY}/readstat_error.c
    ${READSTAT_DIRECTORY}/readstat_io_unistd.c
    ${READSTAT_DIRECTORY}/readstat_malloc.c
    ${READSTAT_DIRECTORY}/readstat_metadata.c
    ${READSTAT_DIRECTORY}/readstat_parser.c
    ${READSTAT_DIRECTORY}/readstat_value.c
    ${READSTAT_DIRECTORY}/readstat_variable.c
    ${READSTAT_DIRECTORY}/readstat_writer.c
    ${READSTAT_DIRECTORY}/sas/ieee.c
    ${READSTAT_DIRECTORY}/spss/readstat_sav_compress.c
    ${READSTAT_DIRECTORY}/spss/readstat_sav_write.c
    ${READSTAT_DIRECTORY}/spss/readstat_spss.c
    ${READSTAT_DIRECTORY}/spss/readstat_spss_parse.c
    ${READSTAT_DIRECTORY}/stata/readstat_dta.c
    ${READSTAT_DIRECTORY}/stata/readstat_dta_write.c
)
