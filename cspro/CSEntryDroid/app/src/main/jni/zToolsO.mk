LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE            := zToolsO
SOLUTION_SRC_PATH       := $(JNI_PATH)/../../../../..
ZTOOLSO_SRC_PATH        := $(SOLUTION_SRC_PATH)/zToolsO
BZIP2_SRC_PATH          := $(SOLUTION_SRC_PATH)/external/bzip2
MD4C_SRC_PATH           := $(SOLUTION_SRC_PATH)/external/md4c
RIJNDAEL_SRC_PATH       := $(SOLUTION_SRC_PATH)/zToolsO/rijndael
SCRYPT_SRC_PATH         := $(SOLUTION_SRC_PATH)/external/scrypt

LOCAL_C_INCLUDES        += $(SOLUTION_SRC_PATH)
LOCAL_C_INCLUDES        += $(SOLUTION_SRC_PATH)/external

LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/base64.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/BinaryGen.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/DateTime.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/DebugLogging.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/DelimitedTextCreator.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/DirectoryLister.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/Encoders.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/Encryption.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/ErrorMessageDisplayer.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/ExceptionHolder.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/File.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/FileIO.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/FuzzyWuzzy.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/Hash.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/ImsaStrMem.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/IniFile.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/InterfaceString.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/md5.c
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/MemoryStream.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/NewlineSubstitutor.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/NumberToString.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/ObjectTransporter.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/Path.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/PortableFunctions.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/Screen.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/serializer.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/SharableString.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/Special.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/StringOperations.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/TextConverterAnsi.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/TextEncoding.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/Tools.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/UniqueId.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/Utf8.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/Utf8Convert.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/Utf8FileStream.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/uuid.cpp
LOCAL_SRC_FILES         += $(ZTOOLSO_SRC_PATH)/VarFuncs.cpp

LOCAL_SRC_FILES         += $(BZIP2_SRC_PATH)/bzlib.c

LOCAL_SRC_FILES         += $(MD4C_SRC_PATH)/entity.c

LOCAL_SRC_FILES         += $(RIJNDAEL_SRC_PATH)/rijndael-alg-fst.c

LOCAL_SRC_FILES         += $(SCRYPT_SRC_PATH)/insecure_memzero.c
LOCAL_SRC_FILES         += $(SCRYPT_SRC_PATH)/sha256.c

include $(LOCAL_PATH)/LOCAL_CFLAGS.mk
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1

include $(BUILD_STATIC_LIBRARY)
