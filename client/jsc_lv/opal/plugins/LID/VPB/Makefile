#
# Makefile for Voicetronics VPB LID plugin
#
# Copyright (C) 2006 Post Increment, All Rights Reserved
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
# The Original Code is OPAL.
#
# The Initial Developer of the Original Code is Post Increment
#
# Contributor(s): ______________________________________.
#
# $Log: Makefile.in,v $
# Revision 1.5  2007/09/05 11:52:55  csoutheren
# Fix compilation on MacOSX
#
# Revision 1.4  2007/04/19 06:09:03  csoutheren
# Fixed plugin build system to use better detection for OSX
# Resolved 1636262 - Fix for audio plugins for OSX
#
# Revision 1.3  2006/10/10 01:42:54  csoutheren
# Applied patches from Ekiga bug 360869 - fix OpenSolaris build problems
# Thanks to Boying Lu
#
# Revision 1.2  2006/10/04 04:21:16  rjongbloed
# Fixed VPB plug in build under linux
#
# Revision 1.1  2006/10/02 13:30:53  rjongbloed
# Added LID plug ins
#
#

STDCCFLAGS =@STDCCFLAGS@
CXXFLAGS = @CXXFLAGS@

LID_PLUGIN_DIR=@LID_PLUGIN_DIR@
prefix=@prefix@
exec_prefix=@exec_prefix@
libdir=@libdir@

CC=@CC@
CXX=@CXX@
PLUGINEXT=so
SONAME	= vpb
SRCDIR	= ./src
OBJDIR	= ./obj
LDSO=@LDSO@

SRCS	+= vpb.cpp

vpath	%.o $(OBJDIR)
vpath	%.cpp $(SRCDIR)

$(OBJDIR)/%.o : %.cpp
	@mkdir -p $(OBJDIR) >/dev/null 2>&1
	$(CXX) -I../../../include $(CXXFLAGS) $(STDCCFLAGS) $(OPTCCFLAGS) -c $< -o $@

PLUGIN	= ./vpb_lid_pwplugin.$(PLUGINEXT)

OBJECTS = $(addprefix $(OBJDIR)/,$(patsubst %.cpp,%.o,$(notdir $(SRCS))))

$(PLUGIN): $(OBJECTS)
	$(CC) $(LDSO) -o $@ $^ $(EXTRALIBS)

install:
	mkdir -p $(DESTDIR)$(libdir)/$(LID_PLUGIN_DIR)
	install $(PLUGIN) $(DESTDIR)$(libdir)/$(LID_PLUGIN_DIR)

uninstall:
	rm -f $(DESTDIR)$(libdir)/$(LID_PLUGIN_DIR)/$(PLUGIN)

clean:
	rm -f $(OBJECTS) $(PLUGIN)

###########################################

