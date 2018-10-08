LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := xiaomengDaemon
LOCAL_LDLIBS := -llog
LOCAL_CFLAGS := -std=c++11 -O3 -lpthread
LOCAL_C_INCLUDES += ..
LOCAL_SRC_FILES := ../daemon.cpp

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_MODULE := xiaomengDaemon_internal
LOCAL_LDLIBS := -llog
LOCAL_CFLAGS := -std=c++11 -O3 -lpthread
LOCAL_C_INCLUDES += ..
LOCAL_SRC_FILES := ../daemon_internal.cpp

include $(BUILD_EXECUTABLE)
