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

LIB	= $(obj)libvboot.o

COBJS-$(CONFIG_CHROMEOS) += boot_device.o
COBJS-$(CONFIG_SANDBOX) += boot_device_host.o
COBJS-$(CONFIG_CHROMEOS_IDE) += boot_device_ide.o
COBJS-$(CONFIG_MMC) += boot_device_mmc.o
COBJS-$(CONFIG_CHROMEOS_USB) += boot_device_usb.o
COBJS-$(CONFIG_SCSI_AHCI) += boot_device_scsi.o
COBJS-$(CONFIG_CHROMEOS) += display.o
COBJS-$(CONFIG_CHROMEOS) += ec.o
COBJS-$(CONFIG_CHROMEOS) += keyboard.o
COBJS-$(CONFIG_CHROMEOS) += load_firmware.o
COBJS-$(CONFIG_CHROMEOS) += misc.o
COBJS-$(CONFIG_CHROMEOS) += nvstorage.o
COBJS-$(CONFIG_CHROMEOS) += nvstorage_disk.o
COBJS-$(CONFIG_CHROMEOS) += nvstorage_cros_ec.o
COBJS-$(CONFIG_CHROMEOS) += tlcl_stub.o
COBJS-$(CONFIG_CHROMEOS) += utility.o

COBJS	:= $(COBJS-y)
OBJS	:= $(addprefix $(obj),$(COBJS))

$(LIB):	$(obj).depend $(OBJS)
	$(call cmd_link_o_target, $(OBJS) $(SOBJS))

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
