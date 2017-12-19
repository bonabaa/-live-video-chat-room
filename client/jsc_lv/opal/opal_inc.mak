#
# opal_inc.mak
#
# Default build environment for OPAL applications
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
# $Revision: 20959 $
# $Author: ms30002000 $
# $Date: 2008-09-15 03:57:09 +1000 (Mon, 15 Sep 2008) $
#


# This file is for backward compatibility with old OPAL
# applications that are not pkg-config aware.



ifdef DEBUG
DEBUG_BUILD:=yes
endif
 
ifeq ($(P_SHARELIB),0)
OPAL_SHARED_LIB:=no
endif


ifdef OPALDIR
include $(OPALDIR)/opal_defs.mak
else
default_target:
	@echo "Must have OPALDIR defined"
	@false
endif


ifdef PTLIBDIR
opt ::
	@true
include $(PTLIBDIR)/make/ptlib.mak
else
default_target:
	@echo "Must have PTLIBDIR defined"
	@false
endif


LDFLAGS	         += -L$(OPAL_LIBDIR)
LDLIBS	         := -l$(subst lib,,$(LIB_NAME))$(LIB_TYPE) $(LDLIBS)

$(TARGET) :	$(OPAL_LIBDIR)/$(OPAL_FILE)


# End of file
