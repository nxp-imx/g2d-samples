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
CFLAGS += -I ../os/linux/wayland

OBJECTS += \
	g2d_wayland_shm.o \
	shm_test.o \
	test_context.o \
	../os/linux/wayland/xdg-shell-protocol.o

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
	rm -f $(OBJECTS) $(OBJECTS:.o=.d) $(TARGET)