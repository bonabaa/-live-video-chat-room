#
# Makefile
#
# Copyright (C) 2008 Post Increment, All Rights Reserved
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
# The Original Code is OPAL library.
#
# The Initial Developer of the Original Code is Post Increment
#
# Contributor(s): Matthias Schneider (ma30002000@yahoo.de)
#

PROG		= simpleopal
SRCS            = main.cxx


# If we have environment variables for local copies of
# OPAL and PTLib then use those packages by overriding
# the search path for pkg-config
ifneq ($(PTLIBDIR),)
ifneq ($(OPALDIR),)
PKG_CONFIG_PATH=$(OPALDIR):$(PTLIBDIR)
endif
endif

# Determine which package to use debug or release
ifeq ($(DEBUG_BUILD), yes)
DEBUG_SUFFIX=_d
CFLAGS += -g
else
DEBUG_SUFFIX=
CFLAGS += -O3
endif

ifeq ($(STATIC_BUILD), yes)
SUFFIX=$(DEBUG_SUFFIX)_s --static
else
SUFFIX=$(DEBUG_SUFFIX)
endif

ifneq ($(SUFFIX),)
PKG_FLAGS = --define-variable=suffix=$(SUFFIX)
endif
# Get required flags from library

CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config $(PKG_FLAGS) --cflags opal)
LDFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config $(PKG_FLAGS) --libs opal)


# This part is we like to be tidy and put all the build products
# in a sub-directory of a name specific to platform type.
OBJDIR = $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config $(PKG_FLAGS) --variable=objdir opal)

vpath	%.o   $(OBJDIR)

$(OBJDIR)/%.o : %.cxx
	@mkdir -p $(OBJDIR) >/dev/null 2>&1
	$(CXX) $(CFLAGS) -c $< -o $@

$(OPAL_DEPDIR)/%.dep : %.cxx 
	@if [ ! -d $(OPAL_DEPDIR) ] ; then mkdir -p $(OPAL_DEPDIR) ; fi
	@printf %s $(OPAL_OBJDIR)/ > $@
	$(CXX) $(CXXFLAGS:-g=) -M $< >> $@


# This builds the application

OBJECTS = $(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(patsubst %.cxx,%.o,$(notdir $(SRCS)))))

$(OBJDIR)/$(PROG): $(OBJECTS)
	 $(CXX) $^ $(LDFLAGS) -o $@

distclean:
	$(MAKE) DEBUG_BUILD=no  STATIC_BUILD=no  clean
	$(MAKE) DEBUG_BUILD=yes STATIC_BUILD=no  clean
	$(MAKE) DEBUG_BUILD=no  STATIC_BUILD=yes clean
	$(MAKE) DEBUG_BUILD=yes STATIC_BUILD=yes clean

clean:
	rm -rf $(OBJDIR)


# Some extra targets for convenience

opt:
	$(MAKE) DEBUG_BUILD=no STATIC_BUILD=no

debug:
	$(MAKE) DEBUG_BUILD=yes STATIC_BUILD=no
	
both:
	$(MAKE) DEBUG_BUILD=no STATIC_BUILD=no
	$(MAKE) DEBUG_BUILD=yes STATIC_BUILD=no

optnoshared:
	$(MAKE) DEBUG_BUILD=no STATIC_BUILD=yes

debugnoshared:
	$(MAKE) DEBUG_BUILD=yes STATIC_BUILD=yes
	
bothnoshared:
	$(MAKE) DEBUG_BUILD=no STATIC_BUILD=yes
	$(MAKE) DEBUG_BUILD=yes STATIC_BUILD=yes

###########################################
