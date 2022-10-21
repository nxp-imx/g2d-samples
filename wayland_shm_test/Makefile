#*
#* Copyright 2021 NXP
#* Copyright 2016 Freescale Semiconductor, Inc.
#* All rights reserved.
#*
#* SPDX-License-Identifier: BSD-3-Clause
#*
#
# Linux build file for g2d wayland shm test code
#
#

WAYLAND := `pkg-config wayland-client --cflags --libs`
TARGET := g2d_wayland_shm_test

PREFIX ?= /usr
CC ?= $(CROSS_COMPILE)gcc

LDFLAGS += -lg2d

OBJECTS += \
	g2d_wayland_shm.o \
	shm_test.o \
	test_context.o \
	xdg-shell-protocol.o

$(OBJECTS) : xdg-shell-client-protocol.h

$(TARGET) : $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(WAYLAND)

xdg-shell-client-protocol.h:
	wayland-scanner client-header \
		$(SDKTARGETSYSROOT)/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml $@

xdg-shell-protocol.c: xdg-shell-client-protocol.h
	wayland-scanner private-code \
		$(SDKTARGETSYSROOT)/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml $@

.PHONY: install
install: $(TARGET)
	mkdir -p $(DESTDIR)/opt/g2d_samples/
	cp $< $(DESTDIR)/opt/g2d_samples/$(TARGET)

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)/opt/g2d_samples/$(TARGET)

.PHONY: clean
clean:
	rm -f xdg-shell-client-protocol.h xdg-shell-protocol.c $(OBJECTS) $(OBJECTS:.o=.d) $(TARGET)
