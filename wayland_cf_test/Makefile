#*
#* Copyright 2021 NXP
#* Copyright 2016 Freescale Semiconductor, Inc.
#* All rights reserved.
#*
#* SPDX-License-Identifier: BSD-3-Clause
#*
#
# Linux build file for g2d wayland cf test code
#
#

WAYLAND := `pkg-config wayland-client --cflags --libs`
TARGET := g2d_wayland_cf_test

PREFIX ?= /usr
CC ?= $(CROSS_COMPILE)gcc

LDFLAGS += -lg2d

OBJECTS += \
	g2d_wayland_cf.o \
	cf_test.o \
	test_context.o

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