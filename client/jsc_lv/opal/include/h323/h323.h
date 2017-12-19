/*
 * h323.h
 *
 * H.323 protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2000 Equivalence Pty. Ltd.
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
 * Portions of this code were written with the assisance of funding from
 * Vovida Networks, Inc. http://www.vovida.com.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 21293 $
 * $Author: rjongbloed $
 * $Date: 2008-10-12 23:24:41 +0000 (Sun, 12 Oct 2008) $
 */

#ifndef OPAL_H323_H323_H
#define OPAL_H323_H323_H

#include <opal/buildopts.h>

#include <h323/h323ep.h>
#include <h323/h323con.h>
#include <h323/gkclient.h>
#include <opal/buildopts.h>

PString  OpalGetVersion();
unsigned OpalGetMajorVersion();
unsigned OpalGetMinorVersion();
unsigned OpalGetBuildNumber();


#endif // OPAL_H323_H323_H

/////////////////////////////////////////////////////////////////////////////
