#
# plugins.mak
#
# Make rules for building libraries rather than applications.
#
# Portable Windows Library
#
# Copyright (c) 1993-1998 Equivalence Pty. Ltd.
#
# The contents of this file are subject to the Mozilla Public License
# Version 1.0 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
# the License for the specific language governing rights and limitations
# under the License.
#
# The Original Code is Portable Windows Library.
#
# The Initial Developer of the Original Code is Equivalence Pty. Ltd.
#
# Contributor(s): ______________________________________.
#       
# $Revision: 23463 $
# $Author: csoutheren $
# $Date: 2009-09-16 02:57:13 +0000 (Wed, 16 Sep 2009) $
#

ifndef PTLIBDIR
PTLIBDIR=$(HOME)/ptlib
endif

include $(PTLIBDIR)/make/unix.mak

PLUGIN_FILENAME = $(PLUGIN_NAME)_pwplugin.$(LIB_SUFFIX)

OBJDIR = $(PTLIBDIR)/lib_$(PLATFORM_TYPE)/$(PLUGIN_FAMILY)

TARGET = $(OBJDIR)/$(PLUGIN_FILENAME)

ifeq ($(OSTYPE),solaris)
  LDSOPTS += -R$(LIBDIR) -G
else
  ifneq ($(OSTYPE),Darwin)
    LDSOPTS += -shared
  endif
endif

ifeq ($(VERBOSE),)
Q = @echo $@ ;
endif


ifeq ($(MACHTYPE),x86_64)
  STDCCFLAGS += -fPIC
endif

ifeq ($(MACHTYPE),hppa)
  STDCCFLAGS += -fPIC
endif

$(OBJDIR)/$(PLUGIN_FILENAME): $(PLUGIN_SOURCES)
	@mkdir -p $(OBJDIR)
	$(Q_CC)$(CXX) $(STDCCFLAGS) $(CFLAGS) \
	$(LDSOPTS) $< \
	$(PLUGIN_LIBS) \
	$(LDFLAGS) \
	-o $@

OBJS	 := $(patsubst %.c, $(OBJDIR)/%.o, $(patsubst %.cxx, $(OBJDIR)/%.o, $(notdir $(PLUGIN_SOURCES))))

CLEAN_FILES += $(OBJDIR)/$(PLUGIN_FILENAME)

include $(PTLIBDIR)/make/common.mak
