#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Alternatively, this software may be distributed under the terms of the
# GNU General Public License ("GPL") version 2 as published by the Free
# Software Foundation.
#

include $(TOPDIR)/config.mk

LIB	= $(obj)libcros_cmd.o

COBJS-$(CONFIG_CHROMEOS_TEST)	+= cmd_cros_tpm_test.o
COBJS-$(CONFIG_CHROMEOS_TEST)	+= cmd_cros_test.o
COBJS-$(CONFIG_CHROMEOS_TEST)	+= cmd_vbexport_test.o
COBJS-$(CONFIG_CHROMEOS_TEST)	+= cmd_vboot_test.o
COBJS-$(CONFIG_CHROMEOS)	+= cmd_vboot_twostop.o

COBJS	:= $(COBJS-y)
OBJS	:= $(addprefix $(obj),$(COBJS))

$(LIB):	$(obj).depend $(OBJS)
	$(call cmd_link_o_target, $(OBJS) $(SOBJS))

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
