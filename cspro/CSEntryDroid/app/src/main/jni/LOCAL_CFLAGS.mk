
# building on Android
LOCAL_CFLAGS += -DANDROID=1

# suppress warning: expression result unused
LOCAL_CFLAGS += -Wno-unused-value

# suppress warning: enumeration values not handled in switch
LOCAL_CFLAGS += -Wno-switch
