#/*---------------------------------------------------------------------------*/
#/* Copyright 2021 NXP                                                        */
#/*                                                                           */
#/* NXP Confidential. This software is owned or controlled by NXP and may only*/
#/* be used strictly in accordance with the applicable license terms.         */
#/* By expressly accepting such terms or by downloading, installing,          */
#/* activating and/or otherwise using the software, you are agreeing that you */
#/* have read, and that you agree to comply with and are bound by, such       */
#/* license terms. If you do not agree to be bound by the applicable license  */
#/* terms, then you may not retain, install, activate or otherwise use the    */
#/* software.                                                                 */
#/*---------------------------------------------------------------------------*/

LOCAL_PATH := $(call my-dir)

# Share library
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	g2d_ocl_csc.c

LOCAL_SHARED_LIBRARIES := libutils libc liblog

ifeq ($(shell expr $(PLATFORM_SDK_VERSION) ">=" 29),1)
LOCAL_SHARED_LIBRARIES += libg2d-viv
else
LOCAL_SHARED_LIBRARIES += libg2d
endif

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../include/ \
		    $(FSL_PROPRIETARY_PATH)/fsl-proprietary/include

LOCAL_VENDOR_MODULE := true
LOCAL_MODULE := g2d_ocl_csc

LOCAL_LD_FLAGS += -nostartfiles
LOCAL_PRELINK_MODULE := false

include $(BUILD_EXECUTABLE)
