#*
#* Copyright 2021-2022 NXP
#* All rights reserved.
#*
#* SPDX - License - Identifier : BSD - 3 - Clause
#*
#
# Linux build file for g2d test code
#
#

# Caller must set BUILD_IMPLEMENTATION to one of the following:
#   dpu       - DPU-based implementation
#   gpu-drm   - GPU-based DRM implementation
#   gpu-fbdev - GPU-based fbdev implementation
#   pxp       - PXP-based implementation
BUILD_IMPLEMENTATION_USAGE_SUGGESTION=Please set it to one of the following: dpu|gpu-drm|gpu-fbdev|pxp
ifndef BUILD_IMPLEMENTATION
    $(error BUILD_IMPLEMENTATION is not defined. $(BUILD_IMPLEMENTATION_USAGE_SUGGESTION))
endif

SUBDIRS_dpu = basic_test extended_test warp_dewarp_test wayland_cf_test wayland_dmabuf_test wayland_shm_test yuv_test
SUBDIRS_gpu-drm = basic_test multiblit_test wayland_cf_test wayland_dmabuf_test wayland_shm_test yuv_test
SUBDIRS_gpu-fbdev = basic_test overlay_test
SUBDIRS_pxp = basic_test wayland_cf_test wayland_dmabuf_test wayland_shm_test yuv_test
SUBDIRS = $(SUBDIRS_$(BUILD_IMPLEMENTATION))
ifeq ($(SUBDIRS),)
    $(error BUILD_IMPLEMENTATION '$(BUILD_IMPLEMENTATION)' is not known. $(BUILD_IMPLEMENTATION_USAGE_SUGGESTION))
endif

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
