#*
#* Copyright 2021 NXP
#* Copyright 2016 Freescale Semiconductor, Inc.
#* All rights reserved.
#*
#* SPDX-License-Identifier: BSD-3-Clause
#*
#
# Linux build file for g2d wayland dmabuf test code
#
#

WAYLAND := `pkg-config wayland-client --cflags --libs`
TARGET := g2d_wayland_dmabuf_test

PREFIX ?= /usr
CC ?= $(CROSS_COMPILE)gcc

LDFLAGS += -lg2d -lm
CFLAGS += -I ../os/linux/wayland

OBJECTS += \
	g2d_wayland_dmabuf.o \
	dmabuf_test.o \
	test_context.o \
	linux_dmabuf_wp.o \
	../os/linux/wayland/xdg-shell-protocol.o

$(OBJECTS) : linux_dmabuf_wp.h

$(TARGET) : $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(WAYLAND)

.PHONY: install
install: $(TARGET)
	mkdir -p $(DESTDIR)/opt/g2d_samples/
	cp $< $(DESTDIR)/opt/g2d_samples/$(TARGET)

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)/opt/g2d_samples/$(TARGET)

.PHONY: clean
clean:
	rm -f linux_dmabuf_wp.h linux_dmabuf_wp.c $(OBJECTS) $(OBJECTS:.o=.d) $(TARGET)

linux_dmabuf_wp.h: $(SDKTARGETSYSROOT)/usr/share/wayland-protocols/unstable/linux-dmabuf/linux-dmabuf-unstable-v1.xml
	wayland-scanner client-header $< $@

linux_dmabuf_wp.c: $(SDKTARGETSYSROOT)/usr/share/wayland-protocols/unstable/linux-dmabuf/linux-dmabuf-unstable-v1.xml
	wayland-scanner code $< $@