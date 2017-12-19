/*
 * precompile.h
 *
 * PWLib application source file for OPAL Gateway
 *
 * Precompiled header generation file.
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 21004 $
 * $Author: rjongbloed $
 * $Date: 2008-09-16 07:08:56 +0000 (Tue, 16 Sep 2008) $
 */

#include <ptlib.h>

#include <opal/manager.h>

#if OPAL_PTLIB_SSL
#include <ptclib/shttpsvc.h>
typedef PSecureHTTPServiceProcess OpalGwProcessAncestor;
#else
#include <ptclib/httpsvc.h>
typedef PHTTPServiceProcess OpalGwProcessAncestor;
#endif

#if OPAL_H323
#include <h323/h323.h>
#include <h323/gkclient.h>
#include <h323/gkserver.h>
#endif

#if OPAL_SIP
#include <sip/sip.h>
#endif

#include <lids/lidep.h>

#if OPAL_IVR
#include <opal/ivr.h>
#endif



// End of File ///////////////////////////////////////////////////////////////
