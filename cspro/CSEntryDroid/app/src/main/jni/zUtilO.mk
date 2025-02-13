LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zUtilO
LOCAL_C_INCLUDES        := src
ZUTILO_SRC_PATH         := ../../../../../zUtilO

LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/BasicLogger.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/BinaryDataAccessor.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/BinaryDataMetadata.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/CommonStore.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/ConnectionString.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/CredentialStore.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/CSProExecutables.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/DataTypes.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/ExecutionStack.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/FileExtensions.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/imsaStr.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/Interapp.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/MediaStore.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/MimeType.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/MimeTypeMap.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/NameShortener.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/PathHelpers.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/PortableColor.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/PortableFileSystem.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/PortableFont.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/ProcessSummary.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/PropertyString.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/Randomizer.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/SimpleDbMap.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/Specfile.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/SpecialDirectoryLister.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/SQLiteSchema.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/SqlLogicFunctions.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/StdioFileUnicode.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/TemporaryFile.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/TextSource.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/TextSourceEditable.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/TextSourceExternal.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/TransactionManager.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/Versioning.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/Viewers.cpp
LOCAL_SRC_FILES         += $(ZUTILO_SRC_PATH)/VirtualDirectoryLister.cpp

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zPlatformO SQLite zJson

include $(BUILD_STATIC_LIBRARY)
