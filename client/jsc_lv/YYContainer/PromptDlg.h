#pragma once
#include "afxwin.h"
#include "afxext.h"
#include "btnst.h"

class CYYContainerDlg;

// CPromptDlg dialog

class CPromptDlg : public CDialog
{
	DECLARE_DYNAMIC(CPromptDlg)

public:
	CPromptDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPromptDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG_PROMPT_ONLINE };
  //
  bool m_bEraseBkgGroup;
  CString m_strAlias;
  int m_uid, m_status, m_image;
  CYYContainerDlg *m_ower;
  void SetPrompt(CString strAlias);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
  virtual BOOL OnInitDialog();
  afx_msg BOOL OnEraseBkgnd(CDC* pDC);
protected:
	CBitmap m_bkBitmap;
	CBitmap * m_oldBitmap; //指向内存DC原来的 Bitmap 
	CDC m_DC;              //用于存放背景图片的内存DC 
	CButtonST m_btnClose;
  int m_nTimeOutDestroy,m_x ,m_y;
   BITMAP bitInfo; 
public:
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnBnClickedOk();
  afx_msg void OnTimer(UINT nIDEvent);
  afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
  afx_msg void OnBnClickedCheckClose();
  afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
};
