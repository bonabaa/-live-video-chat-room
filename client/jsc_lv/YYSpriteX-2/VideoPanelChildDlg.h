#pragma once
#include "SFVideoWnd.h"

// CVideoPanelChildDlg dialog

class CVideoPanelChildDlg : public CDialog
{
	DECLARE_DYNAMIC(CVideoPanelChildDlg)

public:
	CVideoPanelChildDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CVideoPanelChildDlg();

// Dialog Data
  enum { IDD = IDD_DIALOG_VIDEO_CHILD };
  int m_calltype;
	//CString m_strRemoteUserID;
	 int  m_nUserID;
	CString m_strRemoteUserDisplayName;
	CString m_strHandle;
  static int m_nTitleHeight ;

protected:

  bool m_bFullScreen;
  HCURSOR m_hCursor, m_hDefaultCursor;              //¹â±ê
  CSFVideoWnd m_wndVideo;
  int m_nFrameW,m_nFrameH;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  void OnMenuItem(UINT nID) ;
	DECLARE_MESSAGE_MAP()
public:
 
  CRect m_rcPreRect;
  virtual BOOL OnInitDialog();
	void SetFrameSize(int w,int h);
	//void SetWindowPosInScreen();
	bool SetFrameData(unsigned x, unsigned y,
                                        unsigned width, unsigned height,
                                        const BYTE * data
                                        );

protected:
public:
  afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
  afx_msg void OnSize(UINT nType, int cx, int cy);
  afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
  
  afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
  afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
  afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
  virtual BOOL PreTranslateMessage(MSG* pMsg);
  afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
  afx_msg void OnBnClickedOk();
  void SetVideoWindowTitle(bool bForce, const char* txt);
  afx_msg BOOL OnEraseBkgnd(CDC* pDC);
  afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
};
