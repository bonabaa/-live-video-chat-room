/*
 * MobileOpalDlg.h
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
 * $Revision: 20832 $
 * $Author: rjongbloed $
 * $Date: 2008-09-03 03:00:52 +0000 (Wed, 03 Sep 2008) $
 */

#pragma once
#include "afxwin.h"

// CMobileOpalDlg dialog
class CMobileOpalDlg : public CDialog
{
  // Construction
public:
  CMobileOpalDlg(CWnd* pParent = NULL);	// standard constructor
  ~CMobileOpalDlg();

  // Dialog Data
  enum { IDD = IDD_MOBILEOPAL_DIALOG };

protected:
  virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

  // Implementation
protected:
  HICON m_hIcon;
  CCommandBar m_dlgCommandBar;
  OpalHandle m_opal;
  unsigned m_opalVersion;
  CStatic m_ctrlStatus;
  CStatic m_ctrlLocalStatus;
  CComboBox m_ctrlCallAddress;
  CString m_callAddress;
  CString m_localAddress;
  CStringA m_incomingCallToken;
  CStringA m_currentCallToken;
  bool m_speakerphone;
  CStringA m_currentAOR;
  CStringA m_currentHost;

  // Generated message map functions
  virtual BOOL OnInitDialog();
  afx_msg void OnSize(UINT nType, int cx, int cy);
  DECLARE_MESSAGE_MAP()

  void InitialiseOPAL();
  void ErrorBox(UINT strId, const OpalMessage * response = NULL);
  void SetStatusText(UINT ids, const char * str = NULL);
  void SetCallButton(bool enabled, UINT strId = 0);
  void AddRecentCall(const CString & uri);
  void HandleMessage(OpalMessage & message);

public:
  afx_msg void OnTimer(UINT_PTR nIDEvent);
  afx_msg void OnOK();
  afx_msg void OnCancel();
  afx_msg void OnCallAnswer();
  afx_msg void OnChangedAddress();
  afx_msg void OnSelectedAddress();
  afx_msg void OnSpeakerphone();
  afx_msg void OnMenuOptionsGeneral();
  afx_msg void OnMenuOptionsH323();
  afx_msg void OnMenuOptionsSIP();
  afx_msg void OnExit();
};
