#*
#* Copyright 2021 NXP
#* Copyright 2013 Freescale Semiconductor, Inc.
#* All rights reserved.
#*
#* SPDX - License - Identifier : BSD - 3 - Clause
#*
#
# Linux build file for g2d test code
#
#
TARGET := g2d_test
PREFIX ?= /usr

CC ?= $(CROSS_COMPILE)gcc
LDFLAGS +=  -lg2d

OBJECTS += \
	g2d_test.o	

$(TARGET) : $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS)

.PHONY: install
install: $(TARGET)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp $< $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	mkdir -p $(DESTDIR)/opt/g2d_samples
	ln -sf /usr/bin/$(TARGET) $(DESTDIR)/opt/g2d_samples/$(TARGET)

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	rm -f $(DESTDIR)/opt/g2d_samples/$(TARGET)

.PHONY: clean
clean:
	rm -f $(OBJECTS) $(OBJECTS:.o=.d) $(TARGET)
