#*
#* Copyright 2022 NXP
#* Copyright 2016 Freescale Semiconductor, Inc.
#* All rights reserved.
#*
#* SPDX-License-Identifier: BSD-3-Clause
#*
#
# Linux build file for g2d-dpu warp/dewarp test
#
#

TARGET := g2d_dpu_warp_dewarp_test

PREFIX ?= /usr
CC ?= $(CROSS_COMPILE)gcc

CFLAGS += $(shell pkg-config --cflags cairo)
LDFLAGS += -lg2d -lm $(shell pkg-config --libs cairo)

OBJECTS += \
	g2d_dpu_warp_dewarp_test.o

$(TARGET) : $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(CFLAGS) $(LDFLAGS)

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
