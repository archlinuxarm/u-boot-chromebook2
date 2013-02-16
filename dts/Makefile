#
# Copyright (c) 2011 The Chromium OS Authors.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundatio; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

# This Makefile builds the internal U-Boot fdt if CONFIG_OF_CONTROL is
# enabled. See doc/README.fdt-control for more details.

include $(TOPDIR)/config.mk

LIB	= $(obj)libdts.o

ifeq ($(DEVICE_TREE),)
$(if $(CONFIG_DEFAULT_DEVICE_TREE),,\
$(error Please define CONFIG_DEFAULT_DEVICE_TREE in your board header file))
DEVICE_TREE = $(subst ",,$(CONFIG_DEFAULT_DEVICE_TREE))
endif

$(if $(CONFIG_ARCH_DEVICE_TREE),,\
$(error Your architecture does not have device tree support enabled. \
Please define CONFIG_ARCH_DEVICE_TREE))

# Provide a list of include directories for dtc
DTS_INCS-y := -i $(SRCTREE)/arch/$(ARCH)/dts

DTS_INCS-y += -i $(SRCTREE)/board/$(VENDOR)/dts

DTS_INCS-$(CONFIG_CHROMEOS) += -i $(SRCTREE)/cros/dts

# Check if our dtc includes
DTS_FLAGS := $(shell if ! dtc -i 2>&1 | grep -q "invalid option"; then \
		echo $(DTS_INCS-y); fi)

# We preprocess the device tree file provide a useful define
# Undefine 'linux' since it might be used in device tree files
DTS_CPPFLAGS := -x assembler-with-cpp -Ulinux \
		-DARCH_CPU_DTS=\"$(SRCTREE)/arch/$(ARCH)/dts/$(CONFIG_ARCH_DEVICE_TREE).dtsi\" \
		-DBOARD_DTS=\"$(SRCTREE)/board/$(VENDOR)/$(BOARD)/dts/$(DEVICE_TREE).dts\" \
		-D__ASSEMBLY__ -I$(OBJTREE)/include -I$(SRCTREE)/include \
		-I$(OBJTREE)/include2 \
		-I$(SRCTREE)/board/$(VENDOR)/dts -I$(SRCTREE)/arch/$(ARCH)/dts \
		-include $(OBJTREE)/include/config.h

DTS_TMP := $(OBJTREE)/include/generated/$(DEVICE_TREE).dts.in
DTS_SRC := board/$(VENDOR)/dts/$(DEVICE_TREE).dts

all:	$(obj).depend $(LIB)

# Use a constant name for this so we can access it from C code.
# objcopy doesn't seem to allow us to set the symbol name independently of
# the filename.
DT_BIN	:= $(obj)dt.dtb

DTC_CMD := $(DTC) -R 4 -p 0x1000 -O dtb -o ${DT_BIN} $(DTS_FLAGS) $(DTS_TMP)

$(DT_BIN): $(TOPDIR)/$(DTS_SRC)
	rc=$$( \
		cat $< | $(CPP) -P $(DTS_CPPFLAGS) - > $(DTS_TMP); \
		{ { $(DTC_CMD)  2>&1 ; \
		    echo $$? >&3 ; } | \
		  grep -v '^DTC: dts->dtb  on file' ; \
	        } 3>&1 1>&2 ) ; \
	if [ $$rc != 0 ]; then \
		echo "Source file is $(DTS_SRC)"; \
		echo "Compiler: $(DTC_CMD)"; \
	fi; \
	exit $$rc

process_lds = \
	$(1) | sed -r -n 's/^OUTPUT_$(2)[ ("]*([^")]*).*/\1/p'

# Run the compiler and get the link script from the linker
GET_LDS = $(CC) $(CFLAGS) $(LDFLAGS) -Wl,--verbose 2>&1

$(obj)dt.o: $(DT_BIN)
	# We want the output format and arch.
	# We also hope to win a prize for ugliest Makefile / shell interaction
	# We look in the LDSCRIPT first.
	# Then try the linker which should give us the answer.
	# Then check it worked.
	[ -n "$(LDSCRIPT)" ] && \
		oformat=`$(call process_lds,cat $(LDSCRIPT),FORMAT)` && \
		oarch=`$(call process_lds,cat $(LDSCRIPT),ARCH)` ;\
	\
	[ -z $${oformat} ] && \
		oformat=`$(call process_lds,$(GET_LDS),FORMAT)` ;\
	[ -z $${oarch} ] && \
		oarch=`$(call process_lds,$(GET_LDS),ARCH)` ;\
	\
	[ -z $${oformat} ] && \
		echo "Cannot read OUTPUT_FORMAT from lds file $(LDSCRIPT)" && \
		exit 1 || true ;\
	[ -z $${oarch} ] && \
		echo "Cannot read OUTPUT_ARCH from lds file $(LDSCRIPT)" && \
		exit 1 || true ;\
	\
	cd $(dir ${DT_BIN}) && \
	$(OBJCOPY) -I binary -O $${oformat} -B $${oarch} \
		$(notdir ${DT_BIN}) $@
	rm $(DT_BIN)

OBJS-$(CONFIG_OF_EMBED)	:= dt.o

COBJS	:= $(OBJS-y)

OBJS	:= $(addprefix $(obj),$(COBJS))

binary:	$(DT_BIN)

$(LIB):	$(OBJS) $(DTB)
	$(call cmd_link_o_target, $(OBJS))

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
