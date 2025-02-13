LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zToolsO
LOCAL_C_INCLUDES        := src
ZTOOLSO_SRC_PATH        := ../../../../../zToolsO

LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/rijndael/rijndael-alg-fst.c
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/scrypt/insecure_memzero.c
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/scrypt/sha256.c
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/base64.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/BinaryGen.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/bzlib.c
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/CSProException.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/DateTime.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/DelimitedTextCreator.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/DirectoryLister.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/Encoders.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/Encryption.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/ErrorMessageDisplayer.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/FileIO.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/FuzzyWuzzy.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/Hash.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/ImsaStrMem.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/md5.c
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/Memctrl.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/NumberToString.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/ObjectTransporter.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/PortableFunctions.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/Screen.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/serializer.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/Special.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/StringOperations.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/TextConverterAnsi.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/Tools.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/Utf8Convert.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/Utf8FileStream.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/uuid.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/VarFuncs.cpp

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_CFLAGS            += -DGENERATE_BINARY=1
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external

include $(BUILD_STATIC_LIBRARY)
