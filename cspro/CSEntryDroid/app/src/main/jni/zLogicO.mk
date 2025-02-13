include $(CLEAR_VARS)

LOCAL_MODULE            := zLogicO
ZLOGICO_SRC_PATH        := ../../../../../zLogicO

LOCAL_SRC_FILES         += $(ZLOGICO_SRC_PATH)/ActionInvoker.cpp
LOCAL_SRC_FILES         += $(ZLOGICO_SRC_PATH)/BaseCompiler.cpp
LOCAL_SRC_FILES         += $(ZLOGICO_SRC_PATH)/BasicTokenCompiler.cpp
LOCAL_SRC_FILES         += $(ZLOGICO_SRC_PATH)/FunctionTable.cpp
LOCAL_SRC_FILES         += $(ZLOGICO_SRC_PATH)/GeneralizedFunction.cpp
LOCAL_SRC_FILES         += $(ZLOGICO_SRC_PATH)/KeywordTable.cpp
LOCAL_SRC_FILES         += $(ZLOGICO_SRC_PATH)/Preprocessor.cpp
LOCAL_SRC_FILES         += $(ZLOGICO_SRC_PATH)/ReservedWords.cpp
LOCAL_SRC_FILES         += $(ZLOGICO_SRC_PATH)/SourceBuffer.cpp
LOCAL_SRC_FILES         += $(ZLOGICO_SRC_PATH)/Symbol.cpp
LOCAL_SRC_FILES         += $(ZLOGICO_SRC_PATH)/SymbolTable.cpp
LOCAL_SRC_FILES         += $(ZLOGICO_SRC_PATH)/SymbolType.cpp

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zAppO zUtilO zJson zToolsO zPlatformO

include $(BUILD_STATIC_LIBRARY)
