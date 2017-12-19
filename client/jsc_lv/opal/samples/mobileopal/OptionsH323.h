/*
 * OptionsH323.h
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

#pragma once


// COptionsH323 dialog

class COptionsH323 : public CDialog
{
  DECLARE_DYNAMIC(COptionsH323)

public:
  COptionsH323(CWnd* pParent = NULL);   // standard constructor
  virtual ~COptionsH323();

  // Dialog Data
  enum { IDD = IDD_OPTIONS_H323 };

protected:
  CCommandBar m_dlgCommandBar;

  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  virtual BOOL OnInitDialog();

  DECLARE_MESSAGE_MAP()

  afx_msg void OnBnClickedGatekeeperType();

public:
  UINT m_uiGatekeeperType;
  CEdit m_ctrlGkId;
  CString m_strGkId;
  CEdit m_ctrlGkHost;
  CString m_strGkHost;
  CString m_strAlias;
  CString m_strAuthUser;
  CString m_strPassword;
};
