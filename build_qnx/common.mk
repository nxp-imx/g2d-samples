#
# Target platform name. Change to your board according $QNX_TARGET/<arch name>/usr/lib/graphics/<platform>
#
ifeq ($(QNX_TARGET_PLATFORM),)
  QNX_TARGET_PLATFORM=$(PLATFORM)
endif

#
# target architecture name. Change to architecture according to $QNX_TARGET/<arch name>.
#
ifeq ($(QNX_TARGET_ARCH),)
  QNX_TARGET_ARCH=$(TARGET_ARCH)
endif

#
# QNX_GRAPHICS_ROOT variable points to platform-dependent GPU libraries. If not defined,
# default path is constructed from QNX_TARGET_ARCH and QNX_TARGET_PLATFORM variables.
# No change needed in most cases. 
#
ifeq ($(QNX_GRAPHICS_ROOT),)
  QNX_GRAPHICS_ROOT=$(QNX_TARGET)/$(QNX_TARGET_ARCH)/usr/lib/graphics/$(QNX_TARGET_PLATFORM)
  $(warning Environment variable QNX_GRAPHICS_ROOT is not set. Used default path $(QNX_GRAPHICS_ROOT))
endif

install_dir=$(qnx_build_dir)/platform_binaries/$(QNX_TARGET_PLATFORM)/$(QNX_TARGET_ARCH)
EXTRA_LIBVPATH += $(QNX_GRAPHICS_ROOT)

USEFILE=

# This prevents the platform/board name from getting appended to every build target name.
# This happens automatically as the build directory structure now includes the board above the
# common.mk for each component.
EXTRA_SILENT_VARIANTS+=$(PLATFORM)

# Perform clean of local install directory
POST_CLEAN=$(RM_HOST) $(install_dir)/*$(NAME)*