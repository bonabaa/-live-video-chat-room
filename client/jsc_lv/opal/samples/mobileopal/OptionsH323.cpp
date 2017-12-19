/*
 * OptionsH323.cpp
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
#include "OptionsH323.h"


// COptionsH323 dialog

IMPLEMENT_DYNAMIC(COptionsH323, CDialog)

COptionsH323::COptionsH323(CWnd* pParent /*=NULL*/)
  : CDialog(COptionsH323::IDD, pParent)
  , m_uiGatekeeperType(0)
  , m_strGkId(_T(""))
  , m_strGkHost(_T(""))
  , m_strAlias(_T(""))
  , m_strAuthUser(_T(""))
  , m_strPassword(_T(""))
{

}

COptionsH323::~COptionsH323()
{
}

void COptionsH323::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);

  if (pDX->m_bSaveAndValidate)
    m_uiGatekeeperType = GetCheckedRadioButton(IDC_NO_GATEKEEPER, IDC_GATEKEEPER_HOST) - IDC_NO_GATEKEEPER;
  else
    CheckRadioButton(IDC_NO_GATEKEEPER, IDC_GATEKEEPER_HOST, IDC_NO_GATEKEEPER+m_uiGatekeeperType);

  DDX_Control(pDX, IDC_GK_ID, m_ctrlGkId);
  DDX_Text(pDX, IDC_GK_ID, m_strGkId);
  DDX_Control(pDX, IDC_GK_HOST, m_ctrlGkHost);
  DDX_Text(pDX, IDC_GK_HOST, m_strGkHost);
  DDX_Text(pDX, IDC_USERNAME, m_strAlias);
  DDX_Text(pDX, IDC_AUTH_USER, m_strAuthUser);
  DDX_Text(pDX, IDC_PASSWORD, m_strPassword);
}


BEGIN_MESSAGE_MAP(COptionsH323, CDialog)
  ON_BN_CLICKED(IDC_NO_GATEKEEPER, &COptionsH323::OnBnClickedGatekeeperType)
  ON_BN_CLICKED(IDC_DISCOVER_GATEKEEPER, &COptionsH323::OnBnClickedGatekeeperType)
  ON_BN_CLICKED(IDC_GATEKEEPER_ZONE, &COptionsH323::OnBnClickedGatekeeperType)
  ON_BN_CLICKED(IDC_GATEKEEPER_HOST, &COptionsH323::OnBnClickedGatekeeperType)
END_MESSAGE_MAP()


BOOL COptionsH323::OnInitDialog()
{
  CDialog::OnInitDialog();

  if (!m_dlgCommandBar.Create(this) || !m_dlgCommandBar.InsertMenuBar(IDR_DIALOGS)) {
    TRACE0("Failed to create CommandBar\n");
  }

  return TRUE;
}


// COptionsH323 message handlers

void COptionsH323::OnBnClickedGatekeeperType()
{
  UINT type = GetCheckedRadioButton(IDC_NO_GATEKEEPER, IDC_GATEKEEPER_HOST);
  m_ctrlGkId.EnableWindow(type == IDC_GATEKEEPER_ZONE);
  m_ctrlGkHost.EnableWindow(type == IDC_GATEKEEPER_HOST);
}
