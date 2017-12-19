/*
 * dllmain.cxx
 *
 * DLL main entry point for OpenH323.dll
 *
 * Open H323 Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23895 $
 * $Author: rjongbloed $
 * $Date: 2009-12-23 06:09:09 +0000 (Wed, 23 Dec 2009) $
 */

#include <ptlib.h>

// Include header files for everything that uses factories or plugins
#include <h323/h235auth.h>
#include <rtp/rtp.h>
#include <rtp/srtp.h>
#include <codec/opalwavfile.h>
#include <codec/ratectl.h>
#include <codec/opalpluginmgr.h>
#include <lids/lidpluginmgr.h>
#include <opal/recording.h>


#if OPAL_RUBY
extern "C" void Init_opal();
static void (*dummy)() = Init_opal;
#endif


///////////////////////////////////////////////////////////////////////////////


#ifndef _WIN32_WCE
HINSTANCE PDllInstance;
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID)
#else
HANDLE PDllInstance;
BOOL WINAPI DllMain(HANDLE hinstDLL, DWORD fdwReason, LPVOID)
#endif
{
  if (fdwReason == DLL_PROCESS_ATTACH)
    PDllInstance = hinstDLL;
  return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
