#
# Copyright (C) 2012 Samsung Electronics
# R. Chandrasekar <rcsekar@samsung.com>
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
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

include $(TOPDIR)/config.mk

LIB	:= $(obj)libsound.o

COBJS-$(CONFIG_SOUND)	+= sound.o
COBJS-$(CONFIG_I2S)	+= samsung-i2s.o
COBJS-$(CONFIG_SOUND_INTEL_HDA)	+= hda_codec.o
COBJS-$(CONFIG_SOUND_PCAT_BEEP)	+= pcat_beep.o
COBJS-$(CONFIG_SOUND_WM8994)	+= wm8994.o
COBJS-$(CONFIG_SOUND_MAXIM_CODEC)	+= maxim_codec.o
COBJS-$(CONFIG_SOUND_MAX98095)	+= max98095.o
COBJS-$(CONFIG_SOUND_MAX98090)	+= max98090.o

COBJS	:= $(COBJS-y)
SRCS	:= $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))

all:	$(LIB)

$(LIB):	$(obj).depend $(OBJS)
	$(call cmd_link_o_target, $(OBJS))

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#
