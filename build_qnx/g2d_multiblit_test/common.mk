# Find the driver's root directory, which is 3 levels below the current make file
test_source_root:=$(abspath ../../$(dir $(lastword $(MAKEFILE_LIST))))
qnx_build_dir:=$(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)


ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

define PINFO
PINFO DESCRIPTION="g2d multiblit test"
endef

NAME=g2d_multiblit_test

include $(qnx_build_dir)/common.mk

EXTRA_INCVPATH += $(VIVANTE_SDK_INC)
EXTRA_INCVPATH += $(VIVANTE_SDK_DIR)/hal/inc
EXTRA_INCVPATH += $(test_source_root)/../include

EXTRA_SRCVPATH += $(test_source_root)/multiblit

SOURCE_OBJECTS += $(test_source_root)/multiblit/g2d_multiblit_test.o

OBJECTS_FROM_SRCVPATH := $(basename $(wildcard $(foreach dir, $(EXTRA_SRCVPATH), $(addprefix $(dir)/*., s S c cc cpp))))
MISSING_OBJECTS := $(filter-out $(OBJECTS_FROM_SRCVPATH), $(basename $(SOURCE_OBJECTS)))
ifneq ($(MISSING_OBJECTS), )
$(error ***** Missing source objects:  $(MISSING_OBJECTS))
endif

EXCLUDE_OBJS += $(addsuffix .o, $(notdir $(filter-out $(basename $(SOURCE_OBJECTS)), $(OBJECTS_FROM_SRCVPATH))))
#$(warning ***** Excluded objects: $(EXCLUDE_OBJS))

include $(MKFILES_ROOT)/qmacros.mk

LIBS += g2d

POST_BUILD=$(CP_HOST) $(NAME) $(install_dir)/$(NAME)

include $(qnx_build_dir)/.qnx_internal.mk
include $(MKFILES_ROOT)/qtargets.mk
