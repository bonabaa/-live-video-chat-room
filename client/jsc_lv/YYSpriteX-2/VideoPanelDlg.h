#pragma once
#include <list>
using namespace std;
class CVideoPanelChildDlg;
class COptionsExDlg;
class CVideoPanelDlg;
struct stChildDlg{
  CVideoPanelChildDlg* m_pChild1;
  COptionsExDlg* m_pChild2;
  CVideoPanelDlg* m_pParent;
  int pos;
  ~stChildDlg();
  stChildDlg(CVideoPanelDlg* p )
  {
    pos=0;
    m_pChild1 =0;m_pChild2=0;m_pParent =p;
  };
  unsigned int GetUserID();
};
//typedef std::list<  CVideoPanelChildDlg*> MAP_PanelChildren;
typedef std::list<  stChildDlg*> MAP_PanelChildren;
typedef std::list<  CRect> LST_PanelChildRects;
// CVideoPanelDlg dialog
class LPNMgr;
class COptionsExDlg;
typedef struct stYYRational{
    int num; ///< numerator
    int den; ///< denominator
} AVRational;

class CVideoPanelDlg : public CDialog
{
	DECLARE_DYNAMIC(CVideoPanelDlg)

public:
  void SwitchToTheVideoAdcanceWindow();
  CVideoPanelChildDlg* m_pChildMouseDown;
  COptionsExDlg *m_pMCCDocFlg;
  unsigned int m_nRoomID;
  LPNMgr*  m_pMgr ;
  bool m_bMCCLayout;
	CVideoPanelDlg(CWnd* pParent , LPNMgr* pMgr);   // standard constructor
  void DoSwapWindow(CDialog* pSrc, CDialog* pDst,CPoint& mouseDstPos );
	virtual ~CVideoPanelDlg();
  CVideoPanelChildDlg* GetChildByUid(unsigned int uid);
  int  GetChildUIDByCWnd(CDialog * pDlg);
  void SetExtendtoRight(bool b);
  void SetChildWinSize(bool bFull, CVideoPanelChildDlg * pChild);
   void SetWindowDefaultSize();
// Dialog Data
	enum { IDD = IDD_DIALOG_VIDEO_PANEL };

protected:
  MAP_PanelChildren m_mapChildren;//access in main thread,so no lock here ,we do not call work thread
  LST_PanelChildRects m_lstChildrect;//所有的video child 空区域
  //CRect m_rcPrepareForChild;
  int m_nLayoutmenuID;//layout style
  stYYRational m_stAspectRatio;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
  bool IsMCCNormalLayout(){ return m_nLayoutmenuID== ID_Normal;};
  int GetLayoutStyle(){ return m_nLayoutmenuID;};
  void HungupCall();
  virtual BOOL OnInitDialog();
  virtual BOOL PreTranslateMessage(MSG* pMsg);
  BOOL SetChildLayout(int menuID);
   
protected:
  virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
public:
  afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
  afx_msg void OnDestroy();
  afx_msg void OnBnClickedCancel();
  void OnMenuItem(UINT nID) ;
  afx_msg void OnBnClickedCancel2();
  afx_msg BOOL OnEraseBkgnd(CDC* pDC);
  afx_msg void OnPaint();
  afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
  afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
  //void SwapWindow(unsigned int uidSrc,unsigned int uidDst );

  afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
  void GetApectRatioedHxW( int& childW,  int& childH, int& wOffset , int& hOffset);

  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
  virtual BOOL DestroyWindow();
  afx_msg void OnClose();
  afx_msg void OnSize(UINT nType, int cx, int cy);
  void Notify(int cmd, int userid);
};
