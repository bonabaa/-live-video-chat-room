#pragma once
enum em_OnNotify_FromPopDlg//use by window's sendmessage 
{
  em_SetUserInfo = 0, em_IMMessage_received=1, em_IMMessage_sending=2, em_ClosePop/*standarl win send msg to close popdlg*/, em_ClosingPop/* call in popdlg's onclose */,
  em_LoadSettingessage_request /*send it to activex*/, em_LoadSettingMessage_reponse ,
  em_xamlInitOK=999, em_SaveLocalOptionsInfo = 1000,em_LoadLocalOptionsInfo = 1001,em_CloseMe = 1002,  em_SendPicture=1003, em_ySaveSheduleMeet=1004,em_ySaveUseinfo=1005, em_yLoadUseinfo=1006,
  em_yListOnline=1007,
  em_yAddFriend=1008,em_SendvideoCall=1009,em_SendAudiocall=1010, em_SendPPT= 1011,em_SendFile=1012,em_SendWhiteBoard=1013,em_SendDesktop=1014 ,em_SipCommand=1015,em_DlgWelcomeMsg=1016,em_CallEstablish=1017,
  em_CallClearFromPop=1018,em_SwitchToVideoPanel=1019,em_popOptionsDlg=1020 ,em_popRequestSpeak=1021,em_popResponseSpeak=1022,em_popRecording=1023,em_popRequestMCUMeeting=1024,em_popResponceMCUMeeting=1025,
  em_popTuningDlg=1026/*声音测试向导*/, em_yGetFriends=1027,em_yGetCUGContacts=1028,em_yEnterMeeting=1029,em_ySendFileOpenfolder=1030,em_yGetIMHistory=1031,em_yAddorEditSIPAccount=1032, em_yPopSaveIMSetting=1033,em_yLayoutExtendToRight=1034/*去掉右面的videocol 最大化*/, em_clipboard=1035,em_meeting_ctrl2=1036,em_mouse_down=1037,em_yViewUserINFO=1038,em_yManageRoomINFO=1039
};
//enum em_OnNotify_FromPopDlg//use by window's sendmessage 
//{
//  em_SaveLocalOptionsInfo = 1000,em_LoadLocalOptionsInfo = 1001,em_CloseMe = 1002
//};
enum em_JSMsgType{
  em_sizeContainerWindow,
  em_sendIM,
  em_ExitProgram=10,
  em_sizeSLLeftPanelWindow= 100,
   em_pop_GetUserID= 103,
   em_pop_GetYYMeetingUserID= 104,
  em_setWindowTitle= 102,
  em_PrepareLogin=105

};
enum em_Style
{
   em_standard, em_popIM, em_optDlg, em_listonlineDlg,  em_viewuserDlg,  em_createmeetDlg, em_MsgBox=10, em_welcomeDlg=11,em_guideAudioDlg=12,em_createfreegroupDlg=13,em_MCCNotifyDlg=14,em_sipaccountDlg=15, em_msgfromcontainer=20, em_DTMF=30,em_controlmedia =40
};
enum emSubStyle
{
   em_dispListOnline=0, em_dispSipAccount =1
};
class CSFVideoWnd;
#ifndef LPN_SDK
// COptionsExDlg dialog
#include <afxdhtml.h>
class LPNMgr;
class COptionsExDlg : public CDHtmlDialog
{
	DECLARE_DYNCREATE(COptionsExDlg)
protected:
  int m_workwidth;
	int m_workheight,m_winW,m_winH;

public:
  CSFVideoWnd* m_pLocalVideo;
  CPoint m_ptPosBeforeShake;
  BOOL m_bFlashWindow;
  int m_nMsgBoxStyle;
  em_Style m_emStyle;
  int m_emSubStyle;
  CString m_strMeetingid;
  CString m_strMsgPrompt;
  CString m_strUserinfodetail;//userdetail info, or sip detail info 
  unsigned int m_nUserID;//但前正在会话ID
  bool m_bXamlInit;
  unsigned int m_nYYmeetingUserID;//登陆的用户ID
  CString m_strUserAlias;//not UTF8
  void ShakeWindow()  ;//整动窗口
  COptionsExDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~COptionsExDlg();
// Overrides
	HRESULT OnButtonOK(IHTMLElement *pElement);
	HRESULT OnButtonCancel(IHTMLElement *pElement);

// Dialog Data
	enum { IDD = IDD_DIALOG_OPTIONS, IDH = IDR_HTML_OPTIONSEXDLG };
	virtual void OnDocumentComplete(LPDISPATCH pDisp, LPCTSTR szUrl);
	virtual void OnNavigateComplete(LPDISPATCH pDisp, LPCTSTR szUrl);
  int DLGNotifyOfflineIM(LONG index, LONG total, LPCTSTR msg);
  void OnNotifySLDlg(int cmd, const char* msg);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	DECLARE_DHTML_EVENT_MAP()
  DECLARE_DISPATCH_MAP()
  void OnOK();

  void OnCancel();
  int OnSendJSMsg(LPCTSTR p );
  BSTR OnSendJSMsgEx(LPCTSTR p );
  virtual BOOL CanAccessExternal();

  //void SetFullScreen(bool bfull);
  //CComVariant CallJScriptSimple(const CString strFunc,
  //                                  const CString 
  //                                        str);
  bool GetJScript(CComPtr<IDispatch>& spDisp);
  CString LoadSetting();
  void SaveSetting(CString strOptions);
  //void HandleDisplayVideo();
public:
  	HICON m_hIcon;
  LPNMgr* m_pMgr;
  virtual BOOL DestroyWindow();
  CComVariant CallJScript(const CString strFunc,
                                    const CStringArray&
                                          paramArray);
  void ShowWindowEx();
 
  virtual BOOL PreTranslateMessage(MSG* pMsg);
  afx_msg void OnTimer(UINT nIDEvent);
  afx_msg void OnClose();
  bool m_bLoadedXAML;
  afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
  //afx_msg void OnPaint();
  //afx_msg BOOL OnEraseBkgnd(CDC* pDC);
  afx_msg void OnSize(UINT nType, int cx, int cy);
  afx_msg void OnBnClickedOk();
  virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};
#include <ptlib.h>
 
#include <ptlib/thread.h>
#include "yywin32thread.h"

//#define PT_THREAD
class YYThreadDlg:
#ifdef PT_THREAD
  public PThread
{
	 PCLASSINFO(YYThreadDlg, PThread);
#else
  public CYYWin32Thread
{
	 //PCLASSINFO(YYThreadDlg, PObject);
#endif
 public:
	YYThreadDlg(LPNMgr* pLPNMgr, unsigned int owerID, const char* imDlgOweraAlias );
public:
	~YYThreadDlg();
	HWND GetHWND();
  AFX_MODULE_THREAD_STATE* ref_mts;
 	virtual void Main( void );
  public:
	static YYThreadDlg* Create(LPNMgr* pLPNMgr);
	static void Destroy(unsigned int uid);
  COptionsExDlg* m_pDlg;
	LPNMgr* m_pLPNMgr;
  em_Style m_emStyle;
  CString m_strMeetingid;
  CString m_strUserinfodetail;
  unsigned int m_nUserID;
  unsigned int m_nYYmeetingUserID;
  CString m_strUserAlias;//not UTF8
   PSyncPoint m_started;
 //static DWORD ThreadFunc(PVOID ref_mts);
 
};
#endif//sdk
//typedef COptionsExDlg yDlgXAML;
