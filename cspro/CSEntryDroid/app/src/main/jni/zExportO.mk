LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zExportO
ZEXPORTO_SRC_PATH         := ../../../../../zExportO

LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/CSProExportWriter.cpp
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/DelimitedTextExportWriter.cpp
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/EncodedTextWriter.cpp
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/ExcelExportWriter.cpp
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/ExportDefinitions.cpp
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/ExportWriterBase.cpp
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/ReadStatExportWriterBase.cpp
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/RExportWriter.cpp
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/SasExportWriter.cpp
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/SasTransportWriter.cpp
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/SingleRecordExportWriterBase.cpp
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/SpssExportWriter.cpp
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/StataExportWriter.cpp

LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/../external/librdata+ReadStat/CKHashTable.c
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/../external/librdata+ReadStat/readstat_bits.c

LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/../external/librdata/rdata_error.c
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/../external/librdata/rdata_io_unistd.c
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/../external/librdata/rdata_parser.c
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/../external/librdata/rdata_write.c

LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/../external/ReadStat/readstat_error.c
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/../external/ReadStat/readstat_io_unistd.c
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/../external/ReadStat/readstat_malloc.c
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/../external/ReadStat/readstat_metadata.c
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/../external/ReadStat/readstat_parser.c
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/../external/ReadStat/readstat_value.c
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/../external/ReadStat/readstat_variable.c
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/../external/ReadStat/readstat_writer.c
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/../external/ReadStat/sas/ieee.c
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/../external/ReadStat/spss/readstat_sav_compress.c
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/../external/ReadStat/spss/readstat_sav_write.c
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/../external/ReadStat/spss/readstat_spss.c
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/../external/ReadStat/spss/readstat_spss_parse.c
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/../external/ReadStat/stata/readstat_dta.c
LOCAL_SRC_FILES         += $(ZEXPORTO_SRC_PATH)/../external/ReadStat/stata/readstat_dta_write.c

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1

include $(BUILD_STATIC_LIBRARY)
