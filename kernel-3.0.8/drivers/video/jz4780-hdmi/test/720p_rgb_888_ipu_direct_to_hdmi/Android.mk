# Test ipu bi-cube, bilinear and output to fg1.

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES := 720p_rgb_888_ipu_direct_to_hdmi.c
LOCAL_SHARED_LIBRARIES := \
	libdmmu \
	libjzipu

LOCAL_MODULE := 720p_rgb_888_ipu_direct_to_hdmi

include $(BUILD_EXECUTABLE)