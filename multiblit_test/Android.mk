LOCAL_PATH := $(call my-dir)
include $(LOCAL_PATH)/../../../../driver/Android.mk.def

# Share library
include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	g2d_multiblit_test.c

LOCAL_CFLAGS += -DBUILD_FOR_ANDROID -DIMX6Q

LOCAL_SHARED_LIBRARIES := libutils libc liblog libGAL libEGL

ifeq ($(shell expr $(PLATFORM_SDK_VERSION) ">=" 29),1)
LOCAL_SHARED_LIBRARIES += libg2d-viv
else
LOCAL_SHARED_LIBRARIES += libg2d
endif

LOCAL_C_INCLUDES := $(LOCAL_PATH)

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../include/ $(FSL_PROPRIETARY_PATH)/fsl-proprietary/include/
LOCAL_C_INCLUDES += $(AQROOT)/driver/android/gralloc \
                    $(AQROOT)/sdk/inc \
                    $(AQROOT)/hal/inc   


LOCAL_MODULE := g2d_multiblit_test
LOCAL_LD_FLAGS += -nostartfiles
LOCAL_PRELINK_MODULE := false
LOCAL_VENDOR_MODULE  := true
include $(BUILD_EXECUTABLE)
