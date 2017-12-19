/*
 * OptionsGeneral.h
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

#include "stdafx.h"
#include "MobileOPAL.h"
#include "OptionsGeneral.h"


// COptionsGeneral dialog

IMPLEMENT_DYNAMIC(COptionsGeneral, CDialog)

COptionsGeneral::COptionsGeneral(CWnd* pParent /*=NULL*/)
  : CDialog(COptionsGeneral::IDD, pParent)
  , m_strUsername(_T(""))
  , m_strDisplayName(_T(""))
  , m_strStunServer(_T(""))
  , m_interfaceAddress(_T(""))
{

}

COptionsGeneral::~COptionsGeneral()
{
}

void COptionsGeneral::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Text(pDX, IDC_USERNAME, m_strUsername);
  DDX_Text(pDX, IDC_DISPLAYNAME, m_strDisplayName);
  DDX_Text(pDX, IDC_STUN_SERVER, m_strStunServer);
  DDX_CBString(pDX, IDC_INTERFACE_ADDRESS, m_interfaceAddress);
  DDX_Control(pDX, IDC_INTERFACE_ADDRESS, m_interfaceAddressCombo);
}


BEGIN_MESSAGE_MAP(COptionsGeneral, CDialog)
END_MESSAGE_MAP()


BOOL COptionsGeneral::OnInitDialog()
{
  CDialog::OnInitDialog();

  if (!m_dlgCommandBar.Create(this) || !m_dlgCommandBar.InsertMenuBar(IDR_DIALOGS)) {
    TRACE0("Failed to create CommandBar\n");
  }

  m_interfaceAddressCombo.AddString(L"*");

  // Get interfaces
  CStringArray interfaces;
  GetNetworkInterfaces(interfaces, true);
  for (int i = 0; i < interfaces.GetSize(); ++i) {
    CString iface = interfaces[i];
    int percent = iface.Find('%');
    if (percent > 0) {
      AddToCombo(iface.Left(percent));
      AddToCombo(iface.Mid(percent));
    }
    AddToCombo(iface);
  }

  UpdateData(false);

  return TRUE;
}


void COptionsGeneral::AddToCombo(const CString & str)
{
  if (m_interfaceAddressCombo.FindStringExact(-1, str) < 0)
    m_interfaceAddressCombo.AddString(str);
}


// COptionsGeneral message handlers
