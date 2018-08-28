LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := xiaomengDaemon
LOCAL_LDLIBS := -llog
LOCAL_CFLAGS := -std=c11 -O3 -lpthread
LOCAL_C_INCLUDES += ..
LOCAL_SRC_FILES := ../daemon.c \
                   ../error.c \
                   ../utils.c

include $(BUILD_EXECUTABLE)
