#*
#* Copyright 2021 NXP
#* Copyright 2013 Freescale Semiconductor, Inc.
#* All rights reserved.
#*
#* SPDX-License-Identifier: BSD-3-Clause
#*
#
# Linux build file for g2d overlay tile test code
#
#
TARGET := g2d_overlay_tile_test

CC ?= $(CROSS_COMPILE)gcc
CFLAGS += -I ../../os/linux
LDFLAGS +=  -lg2d

DIRS = . \
	../../os/linux
SOURCES = $(foreach DIR,$(DIRS),$(wildcard $(DIR)/*.c))
OBJECTS = $(notdir $(SOURCES:.c=.o))
VPATH = $(DIRS)

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $< $(LDFLAGS)

$(TARGET) : $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

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

