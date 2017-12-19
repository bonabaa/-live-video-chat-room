#
# Makefile
#
# Makefile for PlayRTP
#
# Copyright (c) 2007 Equivalence Pty. Ltd.
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
# $Log: Makefile,v $
# Revision 1.1  2007/09/28 02:33:26  rjongbloed
# Added application to play back RTP out of a Wireshark capture file.
#


PROG = playrtp
SOURCES := main.cxx

ifndef OPALDIR
ifneq (,$(wildcard $(HOME)/opal))
OPALDIR=$(HOME)/opal
else
ifneq (,$(wildcard /usr/local/opal))
OPALDIR=/usr/local/opal
else
default_target :
	@echo Cannot find OPAL in standard locations, you must set the OPALDIR
	@echo environment variable to build this application.
endif
endif
endif

ifdef OPALDIR
include $(OPALDIR)/opal_inc.mak
endif

