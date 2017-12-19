#
# opal_inc.mak
#
# Make symbols include file for Open Phone Abstraction library
#
# Copyright (c) 2001 Equivalence Pty. Ltd.
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
# The Original Code is Open Phone Abstraction library.
#
# The Initial Developer of the Original Code is Equivalence Pty. Ltd.
#
# Contributor(s): ______________________________________.
#
# $Revision: 21133 $
# $Author: ms30002000 $
# $Date: 2008-09-22 16:33:07 +0000 (Mon, 22 Sep 2008) $
#

ifndef DEBUG_BUILD
DEBUG_BUILD             = @DEBUG_BUILD@
endif
ifndef OPAL_SHARED_LIB
OPAL_SHARED_LIB         = @OPAL_SHARED_LIB@
endif


# Tool names detected by configure
CXX                     = @CXX@
CC                      = @CC@
INSTALL                 = @INSTALL@
AR                      = @AR@
RANLIB                  = @RANLIB@
LD                      = @LD@
ARCHIVE                 = @ARCHIVE@

# The install directories
prefix                  = @prefix@
exec_prefix             = @exec_prefix@
libdir                  = @libdir@
includedir              = @includedir@
datarootdir             = @datarootdir@

# The opal source and destination dirs
OPALDIR                 = @OPALDIR@
OPAL_SRCDIR             = $(OPALDIR)/src
OPAL_INCDIR             = $(OPALDIR)/include
OPAL_LIBDIR             = @OPAL_LIBDIR@

# The library file names
RELEASE_LIB_NAME                = @RELEASE_LIB_NAME@
RELEASE_LIB_FILENAME_STATIC     = @RELEASE_LIB_FILENAME_STATIC@
RELEASE_LIB_FILENAME_SHARED     = @RELEASE_LIB_FILENAME_SHARED@
RELEASE_LIB_FILENAME_SHARED_MAJ = @RELEASE_LIB_FILENAME_SHARED_MAJ@
RELEASE_LIB_FILENAME_SHARED_MIN = @RELEASE_LIB_FILENAME_SHARED_MIN@
RELEASE_LIB_FILENAME_SHARED_PAT = @RELEASE_LIB_FILENAME_SHARED_PAT@
RELEASE_CFLAGS                  = @RELEASE_CFLAGS@
RELEASE_LIBS                    = @RELEASE_LIBS@
RELEASE_OPAL_OBJDIR	        = @RELEASE_OPAL_OBJDIR@
RELEASE_OPAL_DEPDIR	        = @RELEASE_OPAL_OBJDIR@

# The library file names
DEBUG_LIB_NAME                  = @DEBUG_LIB_NAME@
DEBUG_LIB_FILENAME_STATIC       = @DEBUG_LIB_FILENAME_STATIC@
DEBUG_LIB_FILENAME_SHARED       = @DEBUG_LIB_FILENAME_SHARED@
DEBUG_LIB_FILENAME_SHARED_MAJ   = @DEBUG_LIB_FILENAME_SHARED_MAJ@
DEBUG_LIB_FILENAME_SHARED_MIN   = @DEBUG_LIB_FILENAME_SHARED_MIN@
DEBUG_LIB_FILENAME_SHARED_PAT   = @DEBUG_LIB_FILENAME_SHARED_PAT@
DEBUG_CFLAGS                    = @DEBUG_CFLAGS@
DEBUG_LIBS                      = @DEBUG_LIBS@
DEBUG_OPAL_OBJDIR	        = @DEBUG_OPAL_OBJDIR@
DEBUG_OPAL_DEPDIR	        = @DEBUG_OPAL_OBJDIR@

# Compile and linker flags
CFLAGS                  = @CFLAGS@
CXXFLAGS                = @CXXFLAGS@
CFLAGS                  = @CFLAGS@
LIBS                    = @LIBS@
LDSOOPTS                = @LDSOOPTS@
HAVE_RANLIB             = @HAVE_RANLIB@

OPAL_H323         = @OPAL_H323@
OPAL_SIP          = @OPAL_SIP@
OPAL_IAX2         = @OPAL_IAX2@
OPAL_VIDEO        = @OPAL_VIDEO@
OPAL_ZRTP         = @OPAL_ZRTP@
OPAL_LID	  = @OPAL_LID@
OPAL_IVR	  = @OPAL_IVR@
OPAL_H224FECC     = @OPAL_H224FECC@
OPAL_H460         = @OPAL_H460@
OPAL_SRTP	  = @OPAL_SRTP@
OPAL_RFC4175	  = @OPAL_RFC4175@
OPAL_AEC          = @OPAL_AEC@
OPAL_G711PLC      = @OPAL_G711PLC@
OPAL_T38_CAP	  = @OPAL_T38_CAPABILITY@
OPAL_FAX          = @OPAL_FAX@
OPAL_JAVA         = @OPAL_JAVA@
SPEEXDSP_SYSTEM   = @SPEEXDSP_SYSTEM@

OPAL_PLUGINS      = @OPAL_PLUGINS@
OPAL_SAMPLES      = @OPAL_SAMPLES@

OPAL_PTLIB_SSL    = @OPAL_PTLIB_SSL@
OPAL_PTLIB_SSL_AES= @OPAL_PTLIB_SSL_AES@
OPAL_PTLIB_ASN    = @OPAL_PTLIB_ASN@
OPAL_PTLIB_EXPAT  = @OPAL_PTLIB_EXPAT@
OPAL_PTLIB_AUDIO  = @OPAL_PTLIB_AUDIO@
OPAL_PTLIB_VIDEO  = @OPAL_PTLIB_VIDEO@
OPAL_PTLIB_WAVFILE= @OPAL_PTLIB_WAVFILE@
OPAL_PTLIB_DTMF   = @OPAL_PTLIB_DTMF@
OPAL_PTLIB_IPV6   = @OPAL_PTLIB_IPV6@
OPAL_PTLIB_DNS    = @OPAL_PTLIB_DNS@
OPAL_PTLIB_LDAP   = @OPAL_PTLIB_LDAP@
OPAL_PTLIB_VXML   = @OPAL_PTLIB_VXML@
OPAL_PTLIB_CONFIG_FILE=@OPAL_PTLIB_CONFIG_FILE@


ifeq ($(DEBUG_BUILD),yes)
  CFLAGS   := $(DEBUG_CFLAGS) $(CFLAGS)
  CXXFLAGS := $(DEBUG_CFLAGS) $(CXXFLAGS)
  LIBS     += $(DEBUG_LIBS)
  LIB_NAME  = $(DEBUG_LIB_NAME)
else
  CFLAGS   := $(RELEASE_CFLAGS) $(CFLAGS)
  CXXFLAGS := $(RELEASE_CFLAGS) $(CXXFLAGS)
  LIBS     += $(RELEASE_LIBS)
  LIB_NAME  = $(RELEASE_LIB_NAME)
endif

CXXFLAGS   += -I$(OPAL_INCDIR)
CFLAGS     += -I$(OPAL_INCDIR)


# End of file

