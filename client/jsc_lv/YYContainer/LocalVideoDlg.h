#pragma once
#include <vector>
#include "yycontainer.h"
#include "SFVideoWnd.h"
#include <list>
using namespace std;
class CYYContainerDlg;
// CLocalVideoDlg dialog
typedef CDialog CMagDialog;
struct stMsgVideoChild {//received by  video  child dlg
  int w,h,Cmd;
  void* pData;
  char m_strAlias[32];
  unsigned int uid;
};
struct stMsgVideoFrame
{
    unsigned   frameWidth;
    unsigned   frameHeight;
    unsigned   frameRate;

};
class CLocalVideoDlg : public CDialog
{
//dock dlg beging
//struct dk_window 
//	{
//		CMagDialog*	pWnd;
//		BOOL		bDocked;
//		DWORD		nEdge;
//		DWORD		dwMagType;
//		int			delta;
//	};
public:
  bool m_bOpened;
CYYContainerDlg*	m_pMagParentDlg;
	DWORD		m_dwMagType;
	DWORD		nEdge;
	BOOL		m_bDocked;
	BOOL		m_bDisablePosFix;
	int			m_iDelta;

	std::vector <dk_window> m_dkDialogs;
 

//void  SetWindowDefaultSize();

	enum	{	DKDLG_NONE	 = 0,
				DKDLG_ANY	 = 1,
				DKDLG_LEFT	 = 2,
				DKDLG_RIGHT	 = 4,
				DKDLG_TOP	 = 8,
				DKDLG_BOTTOM = 16
			};

protected:
	afx_msg void OnMoving(UINT fwSide, LPRECT pRect);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
 	CBitmap m_bkBitmap;
	CBitmap * m_oldBitmap; //指向内存DC原来的 Bitmap 
	//char* m_pBitmapData;//
	CDC m_DC;              //用于存放背景图片的内存DC 
  	BITMAPINFOHEADER	m_destinfo;

	CSFVideoWnd m_wndVideo;
  CYYContainerDlg *m_pContainer;
  list<stMsgVideoFrame> m_listVideoSize;
  //int m_nSelectSize, m_nSelectFPS, m_nSelectBitrate;
	CMenu * m_popupMenu ;

public:
	//void UnDockMagneticDialog(CMagDialog* pDialog);
	//void DockMagneticDialog(CMagDialog* pDialog,DWORD nEdge);
	//void UpdateMagPosition(CMagDialog* pDialog);
	void MoveMagDialog(LPCRECT lpRect, BOOL bRepaint, BOOL bDisablePosFix);
	void UpdateDockData(dk_window dkdata);

public:
  int C_W,C_H;

	void EnableMagnetic(DWORD dwMagType,CYYContainerDlg* pMagParentDlg);
	//void AddMagneticDialog(CMagDialog* pDialog,BOOL bDocked = FALSE,DWORD dwMagWhere =0);
  //end
	DECLARE_DYNAMIC(CLocalVideoDlg)

public:
	CLocalVideoDlg(CWnd* pParent ,CYYContainerDlg* pContainer);   // standard constructor
	virtual ~CLocalVideoDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG_LOCAL_VIDEO };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
  afx_msg BOOL OnEraseBkgnd(CDC* pDC);
  virtual BOOL OnInitDialog();
  virtual BOOL DestroyWindow();
  afx_msg void OnBnClickedCancel();
  virtual BOOL PreTranslateMessage(MSG* pMsg);
  afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
protected:
  virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
public:
  afx_msg void OnBnClickedOk();
  afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
  afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
  void OnMenuItem(UINT nID) ;
};
