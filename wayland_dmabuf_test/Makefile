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

TARGET := g2d_wayland_dmabuf_test

OBJS := main.o test_context.o test.o linux_dmabuf_wp.o

CFLAGS += -g -O3
CFLAGS += -I../../source
CFLAGS += -I../../include
CFLAGS += -I$(ROOTFS)/usr/include
CFLAGS += -I../../source/os/linux
CFLAGS += -I../../source/dpuv1/include
CFLAGS += -I../../source/dpuv1/hardware
CFLAGS += --sysroot=$(ROOTFS)
CFLAGS += -Wdouble-promotion
CFLAGS += -ftree-vectorize
CFLAGS += -ftree-vectorizer-verbose=2
CFLAGS += -fassociative-math -ffast-math
CFLAGS += -fsingle-precision-constant
CFLAGS += -funsafe-math-optimizations

LFLAGS += -O3
LFLAGS += -Wl,-rpath-link,$(ROOTFS)/usr/lib
LFLAGS += -Wl,-rpath-link,$(ROOTFS)/usr/lib/arm-linux-gnueabi
LFLAGS += -L ../../lib
LFLAGS += -lg2d -lm
LFLAGS += -fassociative-math -ffast-math
LFLAGS += -ftree-vectorize
LFLAGS += -ftree-vectorizer-verbose=2
LFLAGS += --sysroot=$(ROOTFS)


$(TARGET): linux_dmabuf_wp.h $(OBJS)
	$(CC) -o $@ $(OBJS) $(LFLAGS) $(WAYLAND)

.PHONY: clean
clean:
	-@rm $(TARGET) $(OBJS)

linux_dmabuf_wp.h: $(SDKTARGETSYSROOT)/usr/share/wayland-protocols/unstable/linux-dmabuf/linux-dmabuf-unstable-v1.xml
	wayland-scanner client-header $< $@

linux_dmabuf_wp.c: $(SDKTARGETSYSROOT)/usr/share/wayland-protocols/unstable/linux-dmabuf/linux-dmabuf-unstable-v1.xml
	wayland-scanner code $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
