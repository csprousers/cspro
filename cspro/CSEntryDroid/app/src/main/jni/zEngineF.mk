include $(CLEAR_VARS)

LOCAL_MODULE            := zEngineF
ZENGINEF_SRC_PATH       := ../../../../../zEngineF

LOCAL_SRC_FILES         += $(ZENGINEF_SRC_PATH)/EngineUI.cpp
LOCAL_SRC_FILES         += $(ZENGINEF_SRC_PATH)/EngineUIPortable.cpp
LOCAL_SRC_FILES         += $(ZENGINEF_SRC_PATH)/ErrmsgDlg.cpp
LOCAL_SRC_FILES         += $(ZENGINEF_SRC_PATH)/PortableUserbar.cpp
LOCAL_SRC_FILES         += $(ZENGINEF_SRC_PATH)/ReviewNotesDlg.cpp
LOCAL_SRC_FILES         += $(ZENGINEF_SRC_PATH)/TraceHandler.cpp

LOCAL_CFLAGS            += -DANDROID=1
LOCAL_CFLAGS            += -DUNICODE=1
LOCAL_CFLAGS            += -D_UNICODE=1
LOCAL_STATIC_LIBRARIES  := zEngineO zAppO zDataO zDictO zFormO zMapping zUtilF zHtml zUtilO zJson zToolsO zPlatformO 

include $(BUILD_STATIC_LIBRARY)
