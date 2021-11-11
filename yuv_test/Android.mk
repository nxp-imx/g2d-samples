#*
#* Copyright 2021 NXP
#* All rights reserved.
#*
#* SPDX - License - Identifier : BSD - 3 - Clause
#*

LOCAL_PATH := $(call my-dir)

# Share library
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	g2d_yuv.c

LOCAL_SHARED_LIBRARIES := libutils libc liblog

ifeq ($(shell expr $(PLATFORM_SDK_VERSION) ">=" 29),1)
LOCAL_SHARED_LIBRARIES += libg2d-viv
else
LOCAL_SHARED_LIBRARIES += libg2d
endif

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../include/ \
		    $(FSL_PROPRIETARY_PATH)/fsl-proprietary/include

LOCAL_VENDOR_MODULE := true
LOCAL_MODULE := g2d_yuv_test

LOCAL_LD_FLAGS += -nostartfiles
LOCAL_PRELINK_MODULE := false

include $(BUILD_EXECUTABLE)
