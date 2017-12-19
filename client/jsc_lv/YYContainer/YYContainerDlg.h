// YYContainerDlg.h : 头文件
//

#pragma once
#include "SimpleTray.h"
#include <vector>
using namespace std;
enum BBQ_CLIENT_STATUS {
	CLIENT_NORMAL,
	CLIENT_BUSY,
	CLIENT_ONTHEPHONE,
	CLIENT_AWAY,
	CLIENT_BERIGHTBACK,
	CLIENT_OUTLUNCH,
	CLIENT_INVISIBLE,
	CLIENT_INCONF,

	CLIENT_MCU = 0xFE,
	CLIENT_OFFLINE = 0xFF,
};
enum em_OnNotify_FromPopDlg//use by window's sendmessage 
{
  em_SetUserInfo = 0, em_IMMessage_received=1, em_IMMessage_sending=2, em_ClosePop/*standarl win send msg to close popdlg*/, em_ClosingPop/* call in popdlg's onclose */,
  em_LoadSettingessage_request /*send it to activex*/, em_LoadSettingMessage_reponse ,
   em_SaveLocalOptionsInfo = 1000,em_LoadLocalOptionsInfo = 1001 
};
enum em_Style
{
  em_standard, em_popIM, em_optDlg
};
#include "../miniweb/httpapi.h"

typedef CDialog CMagDialog ;
// CYYContainerDlg 对话框
class CLocalVideoDlg;
class CPromptDlg;
class CYYWin32ThreadDlg;
#include <map>
typedef std::map<unsigned, CPromptDlg*> MAP_PROMPTS ;
class CYYContainerDlg : public CDHtmlDialog
{

//dock dlg begin
  CLocalVideoDlg *m_pDlgLocalVideo;
  MAP_PROMPTS m_mapPrompts;
  CYYContainerDlg*	m_pMagParentDlg;
	DWORD		m_dwMagType;
	DWORD		nEdge;
	BOOL		m_bDocked;
	BOOL		m_bDisablePosFix;
	int			m_iDelta;

	std::vector <dk_window> m_dkDialogs;
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


public:
  void SetWindowDefaultSize();
  void DestroyPrompt(unsigned uid);
	void UnDockMagneticDialog(CDialog* pDialog);
	void DockMagneticDialog(CDialog* pDialog,DWORD nEdge);
	void UpdateMagPosition(CDialog* pDialog);
	void MoveMagDialog(LPCRECT lpRect, BOOL bRepaint, BOOL bDisablePosFix);
	void UpdateDockData(dk_window dkdata);
  void OnKeyDownVK_YYKey( WPARAM wParam,LPARAM lParam);
public:
	void EnableMagnetic(DWORD dwMagType,CMagDialog* pMagParentDlg);
	void AddMagneticDialog(CLocalVideoDlg* pDialog,BOOL bDocked = FALSE,DWORD dwMagWhere =0);
  //dock dlg end
// 构造
public:
      HttpParam m_hp;
	CYYContainerDlg(CWnd* pParent = NULL);	// 标准构造函数
  ~CYYContainerDlg();
  void ShowLocalVideoDlg(bool b);
  em_Style m_emStyle;
  unsigned int m_nUserID;
  unsigned int m_nYYmeetingUserID;
  CString m_strUserAlias, m_strTalker;
// 对话框数据
	enum { IDD = IDD_YYCONTAINER_DIALOG, IDH = IDR_HTML_YYCONTAINER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

	HRESULT OnButtonOK(IHTMLElement *pElement);
	HRESULT OnButtonCancel(IHTMLElement *pElement);
  HWND GetYYMeetingWindow();
	virtual void OnDocumentComplete(LPDISPATCH pDisp, LPCTSTR szUrl);
	virtual void OnNavigateComplete(LPDISPATCH pDisp, LPCTSTR szUrl);

// 实现
protected:
  int m_nHotKeyID[3];

  ATOM m_atom1;
	HICON m_hIcon;
  CSimpleTray* m_pTrayIcon;
  WINDOWPLACEMENT m_OldWndPlacement; //／／用来保存原窗口位置 
	int m_fullwinwidth, m_fullwinheight;
	int m_winwidth, m_winheight;	
	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnTrayNotify(WPARAM wParam, LPARAM lParam);
  void OnMenuItem(UINT nID) ;

	DECLARE_MESSAGE_MAP()
	DECLARE_DHTML_EVENT_MAP()
  DECLARE_DISPATCH_MAP()

public:
  afx_msg LONG OnHotKey(WPARAM wParam,LPARAM lParam);
void OnOK();

void OnCancel();

  afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
  BSTR OnSendJSMsg(LPCTSTR p );
  BSTR OnSendJSMsgEx(LPCTSTR p );
  virtual BOOL CanAccessExternal();
  //static unsigned int ThreadFunc( LPVOID pParam );
  static unsigned int ThreadminiWeb(PVOID p);
  void SetFullScreen(bool bfull);
  CComVariant CallJScript(const CString strFunc,
                                    const CStringArray&
                                          paramArray);
  //CComVariant CallJScriptSimple(const CString strFunc,
  //                                  const CString 
  //                                        str);
  bool GetJScript(CComPtr<IDispatch>& spDisp);
  afx_msg void OnSize(UINT nType, int cx, int cy);
  afx_msg void OnClose();
  afx_msg void OnDestroy();
  afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
  afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
  virtual BOOL PreTranslateMessage(MSG* pMsg);
  virtual BOOL DestroyWindow();
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
   afx_msg void OnTimer(UINT nIDEvent);
protected:
  virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
public:
  virtual void OnFinalRelease();
  virtual void OnBeforeNavigate(LPDISPATCH pDisp, LPCTSTR szUrl);
  afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
  afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
  afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};
