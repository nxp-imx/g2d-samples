#*
#* Copyright 2021 NXP
#* Copyright 2013 Freescale Semiconductor, Inc.
#* All rights reserved.
#*
#* SPDX - License - Identifier : BSD - 3 - Clause
#*
#
# Linux build file for g2d basic tile test
#
#
TARGET := g2d_basic_tile_test
PREFIX ?= /usr

CC ?= $(CROSS_COMPILE)gcc
LDFLAGS +=  -lg2d

OBJECTS += \
	g2d_basic_tile.o

$(TARGET) : $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS)

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
