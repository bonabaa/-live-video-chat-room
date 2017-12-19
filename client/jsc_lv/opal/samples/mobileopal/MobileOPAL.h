/*
 * MobileOPAL.h
 *
 * Sample Windows Mobile application.
 *
 * Open Phone Abstraction Library
 *
 * Copyright (c) 2008 Vox Lucida
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Vox Lucida (Robert Jongbloed)
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 20523 $
 * $Author: rjongbloed $
 * $Date: 2008-07-02 05:45:45 +0000 (Wed, 02 Jul 2008) $
 */

#pragma once

#ifndef __AFXWIN_H__
#error "include 'stdafx.h' before including this file for PCH"
#endif

#ifdef POCKETPC2003_UI_MODEL
#include "resourceppc.h"
#endif 
#ifdef SMARTPHONE2003_UI_MODEL
#include "resourcesp.h"
#endif

// CMobileOpalApp:
// See MobileOPAL.cpp for the implementation of this class
//

class CMobileOpalApp : public CWinApp
{
public:
  CMobileOpalApp();

  // Overrides
public:
  virtual BOOL InitInstance();

  // Implementation

  DECLARE_MESSAGE_MAP()
};

extern CMobileOpalApp theApp;

void GetNetworkInterfaces(CStringArray & interfaces, bool includeNames);
