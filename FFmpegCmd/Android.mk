LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)
# -mfloat-abi=soft is a workaround for FP register corruption on Exynos 4210
# http://www.spinics.net/lists/arm-kernel/msg368417.html
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_CFLAGS += -mfloat-abi=soft
endif
LOCAL_CFLAGS += -std=c99
LOCAL_LDLIBS += -llog -landroid

LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(realpath $(LOCAL_PATH)/..)
LOCAL_C_INCLUDES += $(MY_APP_FFMPEG_INCLUDE_PATH)
LOCAL_C_INCLUDES += $(realpath $(LOCAL_PATH)/../../extra/ffmpeg/)

#LOCAL_SRC_FILES += init_jni.c
LOCAL_SRC_FILES += ffmpeg_cmd_jni.c

LOCAL_SRC_FILES += ffmpeg.c
LOCAL_SRC_FILES += cmdutils.c
LOCAL_SRC_FILES += ffmpeg_filter.c
LOCAL_SRC_FILES += ffmpeg_opt.c

LOCAL_SHARED_LIBRARIES := ijkffmpeg

LOCAL_MODULE := jl_ff_cmd
include $(BUILD_SHARED_LIBRARY)
