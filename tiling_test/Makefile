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
SUBDIRS := basic_test
SUBINSTALL = $(addsuffix .install,$(SUBDIRS))
SUBCLEAN = $(addsuffix .clean,$(SUBDIRS))

.PHONY: all $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

.PHONY: install $(SUBINSTALL)

install: $(SUBINSTALL)

$(SUBINSTALL): %.install:
	$(MAKE) install -C $*

.PHONY: clean $(SUBCLEAN)
clean: $(SUBCLEAN)

$(SUBCLEAN): %.clean:
	$(MAKE) clean -C $*
