LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zDataO
ZDATAO_SRC_PATH         := ../../../../../zDataO

LOCAL_SRC_FILES         += $(ZDATAO_SRC_PATH)/CacheableCaseWrapperRepository.cpp
LOCAL_SRC_FILES         += $(ZDATAO_SRC_PATH)/DataRepository.cpp
LOCAL_SRC_FILES         += $(ZDATAO_SRC_PATH)/DataRepositoryHelpers.cpp
LOCAL_SRC_FILES         += $(ZDATAO_SRC_PATH)/EncryptedSQLiteRepository.cpp
LOCAL_SRC_FILES         += $(ZDATAO_SRC_PATH)/EncryptedSQLiteRepositoryPasswordManager.cpp
LOCAL_SRC_FILES         += $(ZDATAO_SRC_PATH)/ExportWriterRepository.cpp
LOCAL_SRC_FILES         += $(ZDATAO_SRC_PATH)/IndexableTextRepository.cpp
LOCAL_SRC_FILES         += $(ZDATAO_SRC_PATH)/JsonRepository.cpp
LOCAL_SRC_FILES         += $(ZDATAO_SRC_PATH)/MemoryRepository.cpp
LOCAL_SRC_FILES         += $(ZDATAO_SRC_PATH)/NullRepository.cpp
LOCAL_SRC_FILES         += $(ZDATAO_SRC_PATH)/ParadataWrapperRepository.cpp
LOCAL_SRC_FILES         += $(ZDATAO_SRC_PATH)/SQLiteBinaryItemSerializer.cpp
LOCAL_SRC_FILES         += $(ZDATAO_SRC_PATH)/SQLiteBlobQuestionnaireSerializer.cpp
LOCAL_SRC_FILES         += $(ZDATAO_SRC_PATH)/SQLiteDictionarySchemaGenerator.cpp
LOCAL_SRC_FILES         += $(ZDATAO_SRC_PATH)/SQLiteDictionarySchemaReconciler.cpp
LOCAL_SRC_FILES         += $(ZDATAO_SRC_PATH)/SQLiteQuestionnaireSerializer.cpp
LOCAL_SRC_FILES         += $(ZDATAO_SRC_PATH)/SQLiteRepository.cpp
LOCAL_SRC_FILES         += $(ZDATAO_SRC_PATH)/SQLiteRepositoryCaseLoader.cpp
LOCAL_SRC_FILES         += $(ZDATAO_SRC_PATH)/SQLiteRepositoryIterators.cpp
LOCAL_SRC_FILES         += $(ZDATAO_SRC_PATH)/TextRepository.cpp
LOCAL_SRC_FILES         += $(ZDATAO_SRC_PATH)/TextRepositoryNotesFile.cpp
LOCAL_SRC_FILES         += $(ZDATAO_SRC_PATH)/TextRepositoryStatusFile.cpp

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_C_INCLUDES		+= $(JNI_PATH)/../../../../../external
LOCAL_C_INCLUDES		+= $(JNI_PATH)/../../../../../external/jsoncons
LOCAL_STATIC_LIBRARIES  := zPlatformO zToolsO zJson zUtilO zMessageO zDictO zCaseO zExportO zExcelO SQLite

include $(BUILD_STATIC_LIBRARY)
