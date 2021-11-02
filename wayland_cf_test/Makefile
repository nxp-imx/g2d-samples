#/*
# *  Copyright (C) 2016 Freescale Semiconductor, Inc.
# *  All Rights Reserved.
# *
# *  The following programs are the sole property of Freescale Semiconductor Inc.,
# *  and contain its proprietary and confidential information.
# *
# */
#
# Linux build file for g2d library
#
ROOTFS ?= $(SDKTARGETSYSROOT)
ROOTFS_USR:=$(ROOTFS)/usr
WAYLAND=`pkg-config wayland-client --cflags --libs`

TARGET := g2d_wayland_cf
OBJS := main.o test_context.o test.o

CFLAGS += -g -O0 -I../../source -I../../include -I$(ROOTFS)/usr/include -I../../source/os/linux -I../../source/dpuv1/include -I../../source/dpuv1/hardware --sysroot=$(ROOTFS)

LFLAGS += -g -O0 -Wl,-rpath-link,$(ROOTFS)/usr/lib -Wl,-rpath-link,$(ROOTFS)/usr/lib/arm-linux-gnueabi -L ../../lib -lg2d --sysroot=$(ROOTFS)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LFLAGS) $(WAYLAND) -o $@

.PHONY: clean
clean:
	-@rm $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
