/*
 * MobileOPAL.cpp
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
 * $Revision: 19582 $
 * $Author: rjongbloed $
 * $Date: 2008-02-22 07:04:40 +0000 (Fri, 22 Feb 2008) $
 */

#include "stdafx.h"
#include "MobileOPAL.h"
#include "MobileOpalDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMobileOpalApp

BEGIN_MESSAGE_MAP(CMobileOpalApp, CWinApp)
END_MESSAGE_MAP()


// CMobileOpalApp construction
CMobileOpalApp::CMobileOpalApp()
  : CWinApp()
{
}


// The one and only CMobileOpalApp object
CMobileOpalApp theApp;

// CMobileOpalApp initialization

BOOL CMobileOpalApp::InitInstance()
{
#if defined(WIN32_PLATFORM_PSPC) || defined(WIN32_PLATFORM_WFSP)
  // SHInitExtraControls should be called once during your application's initialization to initialize any
  // of the Windows Mobile specific controls such as CAPEDIT and SIPPREF.
  SHInitExtraControls();
#endif // WIN32_PLATFORM_PSPC || WIN32_PLATFORM_WFSP

  // Standard initialization
  // If you are not using these features and wish to reduce the size
  // of your final executable, you should remove from the following
  // the specific initialization routines you do not need
  // Change the registry key under which our settings are stored
  // TODO: You should modify this string to be something appropriate
  // such as the name of your company or organization
  SetRegistryKey(_T("Vox Lucida"));

  CMobileOpalDlg dlg;
  m_pMainWnd = &dlg;
  dlg.DoModal();

  // Since the dialog has been closed, return FALSE so that we exit the
  //  application, rather than start the application's message pump.
  return FALSE;
}
