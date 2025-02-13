LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zExcelO
ZEXCELO_SRC_PATH        := ../../../../../zExcelO

LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/ExcelWriter.cpp
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/src/app.c
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/src/chart.c
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/src/chartsheet.c
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/src/comment.c
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/src/content_types.c
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/src/core.c
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/src/custom.c
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/src/drawing.c
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/src/format.c
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/src/hash_table.c
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/src/metadata.c
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/src/packager.c
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/src/relationships.c
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/src/shared_strings.c
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/src/styles.c
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/src/table.c
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/src/theme.c
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/src/utility.c
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/src/vml.c
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/src/workbook.c
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/src/worksheet.c
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/src/xmlwriter.c
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/third_party/md5/md5.c
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/third_party/minizip/ioapi.c
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/third_party/minizip/zip.c
LOCAL_SRC_FILES         += $(ZEXCELO_SRC_PATH)/../external/libxlsxwriter/third_party/tmpfileplus/tmpfileplus.c

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_CFLAGS            += -DNOUNCRYPT=1
LOCAL_CFLAGS            += -DIOAPI_NO_64=1
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external/libxlsxwriter/include
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external/zlib
LOCAL_STATIC_LIBRARIES  := zlib zPlatformO zToolsO

include $(BUILD_STATIC_LIBRARY)
