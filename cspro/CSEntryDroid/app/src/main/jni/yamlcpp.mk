LOCAL_PATH := $(call my-dir)
JNI_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    		:= yamlcpp
CPPYAML_SRC_PATH       := ../../../../../external/yaml-cpp/src

LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/binary.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/convert.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/depthguard.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/directives.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/emit.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/emitfromevents.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/emitter.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/emitterstate.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/emitterutils.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/exceptions.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/exp.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/memory.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/node.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/node_data.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/nodebuilder.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/nodeevents.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/null.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/ostream_wrapper.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/parse.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/parser.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/regex_yaml.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/scanner.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/scanscalar.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/scantag.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/scantoken.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/simplekey.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/singledocparser.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/stream.cpp
LOCAL_SRC_FILES         += $(CPPYAML_SRC_PATH)/tag.cpp

LOCAL_CFLAGS    		+= -DANDROID=1
LOCAL_CFLAGS    		+= -DUNICODE=1
LOCAL_CFLAGS    		+= -D_UNICODE=1
LOCAL_C_INCLUDES        += $(JNI_PATH)/../../../../../external/yaml-cpp/include

include $(BUILD_STATIC_LIBRARY)
