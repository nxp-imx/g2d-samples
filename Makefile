#/*
# *  Copyright (C) 2013 Freescale Semiconductor, Inc.
# *  All Rights Reserved.
# *
# *  The following programs are the sole property of Freescale Semiconductor Inc.,
# *  and contain its proprietary and confidential information.
# *
# */
#
# Linux build file for g2d test code
#
#
.PHONY: clean


ifeq ($(ARCH), arm-yocto)
ifeq ($(BUILD_HARD_VFP),1)
CFLAGS += -mfpu=vfp -mfloat-abi=hard
LFLAGS += -mfpu=vfp -mfloat-abi=hard
PFLAGS += -mfpu=vfp -mfloat-abi=hard
endif
endif

OBJS := g2d_test.o

CFLAGS += -I $(ROOTFS_USR)/include -I ../include

ifeq ($(SDK_BUILD), 1)
LFLAGS += -Wl,-rpath-link,$(SDK_DIR)/drivers -Wl,-rpath-link,$(ROOTFS_USR)/lib -L $(SDK_DIR)/drivers -L $(ROOTFS_USR)/lib -lg2d
else
LFLAGS += -Wl,-rpath-link,$(ROOTFS_USR)/lib -Wl,-rpath-link,$(ROOTFS_USR)/lib/arm-linux-gnueabi -L ../lib -L $(ROOTFS_USR)/lib -lg2d
endif

CC ?= $(CROSS_COMPILE)gcc --sysroot=$(ROOTFS)

# build as executable binary
TARGET := g2d_test

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LFLAGS)

clean:
	-@rm $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
