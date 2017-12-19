#pragma once

// YYSpriteX-2Ctrl.h : Declaration of the CYYSpriteX2Ctrl ActiveX Control class.
#define W_1 1021
#define YY_OnNotifyContactActionBy_WM    W_1+1
#define YY_OnBuddyStatusChanged_WM       W_1+2
#define YY_OnHeartbeatFeedback_WM        W_1+3
#define YY_OnHeartbeatTimeout_WM         W_1+4
#define YY_OnSessionExpired_WM           W_1+5
#define YY_OnLoginReplaced_WM            W_1+6
#define YY_OnNotifyOfflineIM_WM          W_1+7
#define YY_OnEstableshedCall_WM          W_1+8
#define YY_OnClearedCall_WM              W_1+9
#define YY_OnIncomingCall_WM             W_1+10
#define YY_OnOutgoing_WM                 W_1+11
#define YY_OnCreatedYYScprite_WM         W_1+12
#define YY_OnAddFriend_WM                W_1+13
#define YY_HUNGUP_WM                W_1+14
#define YY_ANSWERCALL_WM                W_1+15
#define YY_INCOMINGCALL_WM                W_1+16
#define YY_ONMSGDOWNLOADFILE_WM                W_1+17
#define YY_ONMSGSENDFILEMESSAGE_WM                W_1+18
#define YY_OnP2PFile_WM                W_1+19
#define YY_OnReceivingFileP2P_WM                W_1+20
#define YY_OnSendingFileP2P_WM                W_1+21
#define YY_CREAT_REMOTE_DLG_WM                W_1+22
#define YY_DESTROY_REMOTE_DLG_WM                W_1+23
#define YY_OnSETPIXELS_NOCONV_WM                W_1+24
#define YY_OnyFTOFState_WM                W_1+25
#define YY_OnSetTlight_WM                W_1+26
#define YY_CREAT_INCOMING_DLG_WM                W_1+27
#define YY_OnCallState_WM                W_1+28
#define YY_OnYYClientState_WM                W_1+29
#define YY_OnNotify_WM                W_1+30
#define YY_DESTROY_INCOMING_DLG_WM                W_1+31
#define YY_CREATE_POPIM_DLG_WM                W_1+32
#define YY_CREATE_CCVIDEOLAYOUT_WM                W_1+33
#define YY_VIDEOCAMEIN_CCVIDEOLAYOUT_WM                W_1+34
#define YY_DESTROY_POPIM_DLG_WM                W_1+35
#define YY_MAIN_SHOWORHIDE_WIN                W_1+36
#define YY_SYNC_DATA_MAINTHREAD                W_1+37

// CYYSpriteX2Ctrl : See YYSpriteX-2Ctrl.cpp for implementation.
class LPNCoreMgr;
class CIncomingDlg;
class yDlgXAML;
#include <list>
#include <map>
class COptionsExDlg; 
class PStringArray;
class CVideoPanelDlg;
class CCatchScreenDlg;
typedef std::map<unsigned int, COptionsExDlg*> MapIMDialogs;
class BBQTrees;
 #define BEGIN_OLEFACTORYCC(class_name) \
protected: \
	class class_name##Factory : public COleObjectFactoryEx \
	{ \
	public: \
    LPNCoreMgr* m_pMgr;\
		class_name##Factory(REFCLSID clsid, CRuntimeClass* pRuntimeClass, \
			BOOL bMultiInstance, LPCTSTR lpszProgID) ;\
		virtual BOOL UpdateRegistry(BOOL);

#define END_OLEFACTORYCC(class_name) \
	}; \
	friend class class_name##Factory; \
	static class_name##Factory factory; \
public: \
	static const GUID guid; \
	virtual HRESULT GetClassID(LPCLSID pclsid);
///

 #define CC_DECLARE_OLECREATE_EX(class_name) \
	BEGIN_OLEFACTORYCC(class_name) \
	END_OLEFACTORYCC(class_name)
 class CYYSpriteX2Ctrl : public COleControl
{
	DECLARE_DYNCREATE(CYYSpriteX2Ctrl)
 
// Constructor
public:
	CYYSpriteX2Ctrl();
#ifdef D_CMYWIN
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#else
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
// Overrides
public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();

// Implementation
protected:
	~CYYSpriteX2Ctrl();

	CC_DECLARE_OLECREATE_EX/*DECLARE_OLECREATE_EX*/(CYYSpriteX2Ctrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CYYSpriteX2Ctrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CYYSpriteX2Ctrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CYYSpriteX2Ctrl)		// Type name and misc status

// Message maps
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();

// Event maps
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
    dispidOnNotifyEx = 28L,    dispidIMDlgControl = 38L,    dispidyDestroy = 37L,    dispidDlgOption = 36L,    dispidDlgLocalVideo = 35L,    dispidEnterRoom = 34L,		dispidOnNotify = 27L,		dispidSheduleMeet = 33L,		dispidOnYYClientState = 26L,		dispidOnCallState = 25L,		dispidOnSETPIXELS_NOCONV2 = 24L,		dispidOnSETPIXELS_NOCONV_WMEx2 = 23L,		dispidOnSETPIXELS_NOCONV_WMEx = 22L,		dispidWindowProc = 32L,		dispidOnSetTlight = 21L,		dispidOnyFTOFState = 20L,		dispidFTOF = 31L,		dispidOnSETPIXELS_NOCONV = 19L,		dispidChangeStatus = 30L,		dispidOnSendingFileP2P = 18L,		dispidOnReceivingFileP2P = 17L,		dispidP2PAFile = 29L,		dispidAcceptP2PFile = 28L,		dispidOnP2PFile = 16L,		dispidPPT = 27L,		dispidOnMsgSendFileMessage = 15L,		dispidOnMsgDownloadFile = 14L,		dispidUpdateUserData = 26L,		dispidRemoveFriend = 25L,		dispidRemoveBlock = 24L,		dispidViewUserData = 23L,		dispidSendOfflineIM = 22L,		dispidSubscribe = 21L,		dispidOnNotifyOfflineIM = 13L,		dispidOnLoginReplaced = 12L,		dispidOnSessionExpired = 11L,		dispidOnHeartbeatTimeout = 10L,		dispidOnHeartbeatFeedback = 9L,		dispidOnBuddyStatusChanged = 8L,		dispidAddFriendEx = 20L,		dispidHangupEx = 19L,		dispidHungup = 18L,		dispidGetTree = 17L,		dispidGetBlockList = 16L,		dispidListOnlineUser = 15L,		dispidConfirmContactActionBy = 14L,		dispidOnNotifyContactActionBy = 7L,		dispidOnCreatedYYScprite = 6L,		dispidGetLastErrorCode = 13L,		dispidGetFirendListEx = 12L,		dispidOnAddFriend = 5L,		dispidAddFriend = 11L,		dispidGetFriendsList = 10L,		dispidShowUserPanel = 9L,		dispidOnOutgoing = 4L,		dispidOnIncomingCall = 3L,		dispidLogout = 8L,		dispidOnClearedCall = 2L,		dispidOnEstableshedCall = 1L,		dispidIsLogin = 7L,		dispidSetCameraName = 6L,		dispidGetCameraNames = 5L,		dispidinitialise = 4L,		dispidHangup = 3L,		dispidMakeCall = 2L,		dispidLogin = 1L
	};
	LPNCoreMgr* m_pMgr;
protected:
	HWND m_hWndEx,m_hwndLocalVideoPanel, m_hwndMainContainerPanel;
  //std::map<unsigned,unsigned> m_uidbyViewIntree;/*cug tree user*/
  CString m_strTreeUsers;
public:
	HWND GetHWNDMainContainer(){return m_hwndMainContainerPanel;};
	HWND GetHWNDLocalVideoPanel(){return m_hwndLocalVideoPanel;};
	HWND GetHWNDEX(){return m_hWndEx;};
	void ReInitYYmeetingUser( );
	bool LoginInThread(LPCTSTR strURL, LPCTSTR tag);
	BSTR Login(LPCTSTR strURL, LPCTSTR tag);
	LONG MakeCall(LPCTSTR strURl, LPCTSTR tag);
	LONG Hangup(LPCTSTR strPeer, LPCTSTR tag);
	void initialise(LPCTSTR tag);
	BSTR GetCameraNames(void);
	LONG SetCameraName(LPCTSTR strName, LPCTSTR tag);
	LONG IsLogin(void);
	void Logout(void);
	void CreateMyWin( );

public:
	LONG ShowUserPanel(LPCTSTR strUserID, LONG bShow, SHORT left, SHORT top, SHORT width, SHORT hight);
	BSTR GetFriendsList(LPCTSTR xfriend);
	CString GetFriendsListReturnCString(LPCTSTR xfriend);
	void AddFriend(LPCTSTR strUserID);
	BSTR GetFirendListEx(void);
	LONG GetLastErrorCode(void);
	LONG ConfirmContactActionBy(LONG nUserid, LONG nAction, LONG bAccepted);
	BSTR ListOnlineUser(void);
	BSTR GetTree(bool bForceUpdate );
	CString GetTreeReturnCString(bool bForceUpdate );
  bool GetTreeSubTreeIDs( unsigned nGroupID ,std::list<unsigned int> &list );

protected:
	BSTR GetBlockList(void);
	LONG Hungup(LPCTSTR strUserID);
	LONG HangupEx(LPCTSTR strUserURL);
	LONG AddFriendEx(LONG uid, LPCTSTR lpszText);
protected:
	void OnBuddyStatusChanged(LPCTSTR strInfoValue);
	void OnHeartbeatFeedback(LPCTSTR status);
	void OnHeartbeatTimeout(void);
	void OnSessionExpired(void);
	void OnLoginReplaced(LPCTSTR addr);
	LONG OnNotifyOfflineIM(LONG index, LONG total, LPCTSTR msg);
	void OnEstableshedCall(LPCTSTR strIDs, LPCTSTR tag);
	void OnClearedCall(LPCTSTR strIDs, LPCTSTR reason, LPCTSTR tag);
	void OnIncomingCall(LPCTSTR strIDs, LPCTSTR Caller);
	void OnOutgoing(LPCTSTR strIDs, LPCTSTR Callee);
	void OnCreatedYYScprite(void);
	void OnAddFriend(LPCTSTR strUserID, LONG code);
	void OnSetTlight(LONG uid, LPCTSTR state );
	void OnNotifyContactActionBy(LONG byid, LONG nAction);
public:
	void OnNotifyContactActionBy_WM(LONG byid, LONG nAction);
	void OnBuddyStatusChanged_WM(LPCTSTR strInfoValue);
	void OnHeartbeatFeedback_WM(LPCTSTR status);
	void OnHeartbeatTimeout_WM(void);
	void OnSessionExpired_WM(void);
	void OnLoginReplaced_WM(LPCTSTR addr);
  bool OnNotifyOfflineIM_Prepared( unsigned int uid);//display the window
	void OnNotifyOfflineIM_WM(LONG index, LONG total, LPCTSTR msg);
	void OnEstableshedCall_WM(LPCTSTR strIDs, LPCTSTR tag);
	void OnClearedCall_WM(LPCTSTR strIDs, LPCTSTR reason, LPCTSTR tag);
	void OnIncomingCall_WM(LPCTSTR strIDs, LPCTSTR Caller, LPCTSTR hanle);
	void OnOutgoing_WM(LPCTSTR strIDs, LPCTSTR Callee);
	void OnCreatedYYScprite_WM(void);
	void OnAddFriend_WM(LPCTSTR strUserID, LONG code);
	void OnMsgDownloadFile_WM(LPCTSTR url, LONG page);
	void OnMsgSendFileProcess_WM(LPCTSTR param );
	void OnP2PFile_WM(LPCTSTR param );
	void OnReceivingFileP2P_WM(LPCTSTR param );
	void OnSendingFileP2P_WM(LPCTSTR param );
	void	OnSETPIXELS_NOCONV_WM(LPCTSTR xy_wh_value, LONG uid );
	void OnSetTlight_WM(LONG uid, LPCTSTR state );
	void OnyFTOFState_WM(LONG code, LPCTSTR state );
	void OnCallState_WM(LONG state, LPCTSTR Params);
	void OnNotify_WM(LONG state, LPCTSTR Params);
	void OnYYClientState_WM(LONG state, LPCTSTR Params);
protected:
	BSTR Subscribe(LPCTSTR strValue);
	BSTR ViewUserData(LPCTSTR uid_b);
	LONG RemoveBlock(LONG uid);
	LONG RemoveFriend(LONG uid);
  CCatchScreenDlg* m_wndCaptureDlg;

	CString strwndClassName;
  MapIMDialogs m_mapIMDlgs;// access it in main thread( create destroy), so it is safe
  MapIMDialogs m_mapMCCNotifyDlgs;// access it in main thread( create destroy), so it is safe
  CWnd* m_pMyWnd;
  BBQTrees* m_pCUGTree;
  bool m_bInit;
public:
	std::list<CIncomingDlg* > m_listIncomingDlgs;

protected:
	void OnMsgDownloadFile(LPCTSTR strurl, LONG whichpage);
	void OnMsgSendFileMessage(LPCTSTR filesendtimepercent);
	LONG PPT(LPCTSTR strUser);
	void OnP2PFile(LPCTSTR value);
	LONG AcceptP2PFile(LPCTSTR value);
	void OnReceivingFileP2P(LPCTSTR v);
	void OnSendingFileP2P(LPCTSTR value);
	LONG ChangeStatus(LONG state, LPCTSTR strState);
public:
	LONG UpdateUserData(LPCTSTR strUserinfo);
	virtual BOOL DestroyWindow();
	BSTR  P2PAFile(LPCTSTR v);
protected:
	void OnSETPIXELS_NOCONV(LPCTSTR buffer, LONG x, LONG y, LONG w, LONG h);
	LONG FTOF(LPCTSTR uid, LPCTSTR pws);
	void OnyFTOFState(LONG code, LPCTSTR state);
	LONG WindowProc(LONG uid, LONG iMsg, LPCTSTR Params);
	void OnSETPIXELS_NOCONV_WMEx(LPCTSTR tag, VARIANT* value);
	void OnSETPIXELS_NOCONV_WMEx2(LPCTSTR tag, VARIANT value);
	void OnSETPIXELS_NOCONV2(LPCTSTR OnSETPIXELS_NOCONV2, LONG myVar);
	void OnCallState(LONG state, LPCTSTR Params);
	void OnYYClientState(LONG code, LPCTSTR Tag);
  LONG EnterRoom(LONG roomid, LPCTSTR tag);
  BSTR DlgLocalVideo(void);
  void DlgOption(LONG nWhichTab);
  void yDestroy(void);
  BOOL Pop_CreateProcess(unsigned int uid,const char* uidAlias);
  bool AutoPrintDocument(unsigned int uid,const CString& strPath, int roomid=-1);
public:
	int OnNotify(LONG type, LPCTSTR value);
  HWND GetVideoPanelHWND();
 
	LONG SheduleMeet(LPCTSTR strV);
  CString ViewUserDataDirect(LPCTSTR uid_b);

  BSTR IMDlgControl(LPCTSTR ctrlcode);
 // void RemoveIMDlg(unsigned int uid);

  void GetOptions(PStringArray& strValues, CString & retValue);
  COptionsExDlg* CreatePopIMDlg(unsigned int  imDlgOwer,const char* imDlgOweraAlias, CWnd* pParent=0, bool bShow=true, bool bforcechild= false);
  bool  DestroyPopDlg(unsigned int  imDlgOwer, int nStyle  );
  void ShowOrHidPopIMDlg(unsigned int  imDlgOwer, bool bShow);

  void DestroyPopIMDlg(unsigned int  imDlgOwer);

	LONG SendOfflineIM(LPCTSTR strMsg);
  afx_msg void OnClose();
  COptionsExDlg* m_pOptdlg, *m_plistonlinedlg,*m_pviewuserdlg, *m_pcreatemeetdlg,*m_pmessageboxdlg ,*m_pmwelcomedlg,*m_pguideAudioDlg,*m_pcreatefreegroupdlg  ;//em_optDlg, em_listonlineDlg,  em_viewuserDlg,  em_createmeetDlg, 
  CVideoPanelDlg *m_pVideoPanelDlg;
  afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
  void OnNotifyVideoPanel(unsigned int uid, int pos,  const char* userDetail);
protected:
  LONG OnNotifyEx(LONG CMD, LPCTSTR tag);
public:
  virtual BOOL PreTranslateMessage(MSG* pMsg);
};

