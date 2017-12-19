/*
 * OptionsSIP.cpp
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
#include "OptionsSIP.h"


// COptionsSIP dialog

IMPLEMENT_DYNAMIC(COptionsSIP, CDialog)

COptionsSIP::COptionsSIP(CWnd* pParent /*=NULL*/)
  : CDialog(COptionsSIP::IDD, pParent)
  , m_strAddressOfRecord(_T(""))
  , m_strHostName(_T(""))
  , m_strAuthUser(_T(""))
  , m_strPassword(_T(""))
  , m_strRealm(_T(""))
{

}

COptionsSIP::~COptionsSIP()
{
}

void COptionsSIP::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Text(pDX, IDC_REGISTRAR_ID, m_strAddressOfRecord);
  DDX_Text(pDX, IDC_HOST_NAME, m_strHostName);
  DDX_Text(pDX, IDC_AUTH_USER, m_strAuthUser);
  DDX_Text(pDX, IDC_PASSWORD, m_strPassword);
  DDX_Text(pDX, IDC_REALM, m_strRealm);
}


BEGIN_MESSAGE_MAP(COptionsSIP, CDialog)
END_MESSAGE_MAP()


BOOL COptionsSIP::OnInitDialog()
{
  CDialog::OnInitDialog();

  if (!m_dlgCommandBar.Create(this) || !m_dlgCommandBar.InsertMenuBar(IDR_DIALOGS)) {
    TRACE0("Failed to create CommandBar\n");
  }

  return TRUE;
}


// COptionsSIP message handlers
