#
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

ifdef BUILD_FACTORY_IMAGE
PLATFORM_CPPFLAGS += -DCONFIG_FACTORY_IMAGE
endif
