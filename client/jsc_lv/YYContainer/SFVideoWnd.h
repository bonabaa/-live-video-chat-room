// SFPhoneTestDlg.h : header file
//

#pragma once
#include "afxcmn.h"
#include "vfw.h"
#pragma   comment(lib,   "vfw32.lib")   
#define WM_VIDEOWND_DBCLICK 15001
#ifndef D_LOCAL_PANCEL_CONTAINER
#include <ptlib.h>
#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>
#endif
#define D_WIN7_VER 6
// CSFVideoWnd dialog
class CSFVideoWnd : public CWnd
{
// Construction
public:
	CSFVideoWnd();	// standard constructor
	virtual ~CSFVideoWnd();

// Implementation
protected:
	// Generated message map functions
	DECLARE_MESSAGE_MAP()
public:
	bool m_bEnableDragMouse;
  DWORD m_nWinver;
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	virtual BOOL DestroyWindow();
	virtual void Draw(const char* handle,const void* data, int nColorSpace,unsigned w, unsigned h);
	virtual void DrawBGR24(const char* handle,const void* data, int nColorSpace,unsigned w, unsigned h);
	void DrawWithBGR24Data(const char* handle,const void* data, int nColorSpace,unsigned w, unsigned h);
  void* ConvertYUV420PToRGB24(const char* handle,const void* data, int nColorSpace,unsigned w, unsigned h);
 	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
protected:
	CRect				m_savedRect;
//#if WINVER >= D_WIN7_VER
	BITMAPINFOHEADER	m_destinfo;
	HDRAWDIB			m_hDibDraw;
#ifndef D_LOCAL_PANCEL_CONTAINER

	PColourConverter* m_converter;
	PBYTEArray m_tempPixelBuffer;
	PBYTEArray m_arrRGBline;
#endif
//#endif
public:
  afx_msg void OnNcLButtonUp(UINT nHitTest, CPoint point);
  afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
  virtual BOOL PreTranslateMessage(MSG* pMsg);
};
