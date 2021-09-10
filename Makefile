#*
#* Copyright 2021 NXP
#* All rights reserved.
#*
#* SPDX - License - Identifier : BSD - 3 - Clause
#*
#
# Linux build file for g2d test code
#
#
SUBDIRS := $(wildcard */.)

.PHONY: install $(SUBDIRS)

install: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) install -C $@

.PHONY: clean
clean: 
	find ./ -name "*.o" -delete
