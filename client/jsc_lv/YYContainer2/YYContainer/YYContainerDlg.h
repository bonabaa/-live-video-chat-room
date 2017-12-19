// YYContainerDlg.h : ͷ�ļ�
//

#pragma once
#include "SimpleTray.h"
#include <vector>
using namespace std;

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
typedef CDialog CMagDialog ;
// CYYContainerDlg �Ի���
class CLocalVideoDlg;

class CYYContainerDlg : public CDHtmlDialog
{
//dock dlg begin
  CLocalVideoDlg *m_pDlgLocalVideo;


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
	void UnDockMagneticDialog(CDialog* pDialog);
	void DockMagneticDialog(CDialog* pDialog,DWORD nEdge);
	void UpdateMagPosition(CDialog* pDialog);
	void MoveMagDialog(LPCRECT lpRect, BOOL bRepaint, BOOL bDisablePosFix);
	void UpdateDockData(dk_window dkdata);

public:
	void EnableMagnetic(DWORD dwMagType,CMagDialog* pMagParentDlg);
	void AddMagneticDialog(CLocalVideoDlg* pDialog,BOOL bDocked = FALSE,DWORD dwMagWhere =0);
  //dock dlg end
// ����
public:
	CYYContainerDlg(CWnd* pParent = NULL);	// ��׼���캯��
  void ShowLocalVideoDlg(bool b);
  em_Style m_emStyle;
  unsigned int m_nUserID;
  unsigned int m_nYYmeetingUserID;
  CString m_strUserAlias;
// �Ի�������
	enum { IDD = IDD_YYCONTAINER_DIALOG, IDH = IDR_HTML_YYCONTAINER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��

	HRESULT OnButtonOK(IHTMLElement *pElement);
	HRESULT OnButtonCancel(IHTMLElement *pElement);
  HWND GetYYMeetingWindow();
	virtual void OnDocumentComplete(LPDISPATCH pDisp, LPCTSTR szUrl);
	virtual void OnNavigateComplete(LPDISPATCH pDisp, LPCTSTR szUrl);

// ʵ��
protected:
  int m_nHotKeyID[3];
  ATOM m_atom1;
	HICON m_hIcon;
  CSimpleTray* m_pTrayIcon;
  WINDOWPLACEMENT m_OldWndPlacement; //������������ԭ����λ�� 
	int m_fullwinwidth, m_fullwinheight;
	int m_winwidth, m_winheight;	
	// ���ɵ���Ϣӳ�亯��
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
  int OnSendJSMsg(LPCTSTR p );
  BSTR OnSendJSMsgEx(LPCTSTR p );
  virtual BOOL CanAccessExternal();

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
};
