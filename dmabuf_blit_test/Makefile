#*
#* Copyright 2025 NXP
#* All rights reserved.
#*
#* SPDX - License - Identifier : BSD - 3 - Clause
#*
#
# Linux build file for g2d dmabuf blit test
#
#
TARGET := g2d_dmabuf_blit_test
PREFIX ?= /usr

CC ?= $(CROSS_COMPILE)gcc
LDFLAGS +=  -lg2d

OBJECTS += \
	g2d_dmabuf_blit.o

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
