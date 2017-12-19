// YYSpriteX-2Ctrl.cpp : Implementation of the CYYSpriteX2Ctrl ActiveX Control class.

#include "stdafx.h"
#include "YYSpriteX-2.h"
#include "YYSpriteX-2Ctrl.h"
#include "YYSpriteX-2PropPage.h"
#include ".\yyspritex-2ctrl.h"
#include "lpncoremgr.h"
#include "main.h"
#include "myclient.h"
#include "comcat.h"
#include "strsafe.h"
#include "objsafe.h"
#include "remotedlg.h"
#include "CatchScreenDlg.h"
#include "Winspool.h"
#include "bzip2/bzlib.h"
#include ".\sfcallsmanager.h"
#include ".\VideoPanelDlg.h"
#include ".\myhwnd.h"
#include ".\sfvideownd.h"

//#include "atlsafe.h"
#include "yrdconnection.h"
#include "OptionsExDlg.h"
#include ".\yusertousertransmitdatamgr.h"

#include <ptlib/sockets.h>
#include <ptclib/http.h>
extern WORD g_httpPort;
extern PString ToBase64(const char* src );
extern PString FromBase64(const char* src );

extern int  g_VideoLayoutForce;
extern bool  g_CanTransmitVideo ;
//#define D_CMYWIN
LPNCoreMgr * g_pMgr=0;
CYYSpriteX2Ctrl::CYYSpriteX2CtrlFactory::CYYSpriteX2CtrlFactory (REFCLSID clsid, CRuntimeClass* pRuntimeClass, 
			BOOL bMultiInstance, LPCTSTR lpszProgID)
:	COleObjectFactoryEx(clsid, pRuntimeClass, bMultiInstance, 
				lpszProgID) {
          //g_pMgr = new LPNCoreMgr();
        int i=0;
        } 
//请注意 千万不能点用 ptrace函数在主线程中，
extern bool g_InitMultLanguageString( );
extern bool g_AddHTTPFileResource(const char* filetitle, const char* filepath);
extern int g_VideoLayout;
extern const char*  D_DIRECTSHOW_NAME ;
extern PString g_GetAliasName(uint32 uid, bool bUTF8);

#ifdef LPN_ACTIVEX_PRJ
#include "..\YYSpriteX-2\resource.h"
//#include "..\lpMultiPeopleVideo\resource.h"
#include "incomingdlg.h"
extern PString UTF8ToPlainText(const char* lpszContent);
#else
#include "..\lpMultiPeopleVideo\resource.h"
#include "incomingdlg.h"
#endif
#include ".\yyvideooutputdevice.h"
 
#include <map>
typedef int BOOL;
bool gfx_DeleteDirectory(const PString& strPath);
//#include "capplication.h"
//#include "cdocuments.h"
//#include "cdocument0.h"
std::map<HWND, CYYSpriteX2Ctrl*> g_YYSpriteCtrls;
//CYYSpriteX2Ctrl* g_pSpriteX2Ctrl;
 HWND   g_GetActiveXHWND(CYYSpriteX2Ctrl* pSpriteX2Ctrl) 
 { 
	 for(std::map<HWND, CYYSpriteX2Ctrl*>::iterator itor=g_YYSpriteCtrls.begin();itor!=g_YYSpriteCtrls.end();itor++ 		 )
	 {
		 if (itor->second == pSpriteX2Ctrl)
			 return itor->first;
	 }
	 return NULL;
 };
 
extern HRESULT   CreateComponentCategory(CATID   catid,   WCHAR*   catDescription)     ;
extern  HRESULT   RegisterCLSIDInCategory(REFCLSID   clsid,   CATID   catid) ;    
extern HRESULT   UnRegisterCLSIDInCategory(REFCLSID   clsid,   CATID   catid)   ;
LRESULT CALLBACK CYYSpriteX2Ctrl::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  if (uMsg == WM_CREATE)
		::SetWindowLong(hWnd, 0, (LONG)((LPCREATESTRUCT)lParam)->lpCreateParams);

  
	std::map<HWND, CYYSpriteX2Ctrl*>::iterator itor=  g_YYSpriteCtrls.find(hWnd);
	if (itor!= g_YYSpriteCtrls.end())
	{
		CYYSpriteX2Ctrl * pSpriteX2Ctrl = itor->second;
		switch (uMsg)
		{
      case WM_COPYDATA:
        pSpriteX2Ctrl->OnCopyData( pSpriteX2Ctrl, (COPYDATASTRUCT*)lParam );
				//pSpriteX2Ctrl->OnSETPIXELS_NOCONV((LPCSTR)wParam, lParam, 0, 0,0);
				break;
			//case YY_OnSetTlight_WM:
			//	pSpriteX2Ctrl->OnSetTlight(wParam,(LPCSTR) lParam );
			//	break;
			case YY_OnNotifyContactActionBy_WM:
					pSpriteX2Ctrl->OnNotifyContactActionBy(wParam, lParam);
				break;
			case YY_OnBuddyStatusChanged_WM       :
						pSpriteX2Ctrl->OnBuddyStatusChanged((LPCSTR)wParam);
					break;
			case YY_OnHeartbeatFeedback_WM        :
        {
            PString* p= (PString*)wParam;
						pSpriteX2Ctrl->OnHeartbeatFeedback((LPCSTR)(const char*)(*p));
            delete p;p=NULL;
        }
					break;
			case YY_OnHeartbeatTimeout_WM         :
						pSpriteX2Ctrl->OnHeartbeatTimeout();
					break;
			case YY_OnSessionExpired_WM           :
						pSpriteX2Ctrl->OnSessionExpired();
					break;
			case YY_OnLoginReplaced_WM            :
						pSpriteX2Ctrl->OnLoginReplaced((LPCSTR)wParam);
					break;
			case YY_OnNotifyOfflineIM_WM          :
				{LONG index= wParam& 0xff, total = wParam>> 16;
					 return pSpriteX2Ctrl->OnNotifyOfflineIM(index, total, (LPCSTR) lParam);
            
				}
					break;
			case YY_OnEstableshedCall_WM          :
						pSpriteX2Ctrl->OnEstableshedCall((LPCSTR)wParam,(LPCSTR) lParam);
					break;
      case YY_OnClearedCall_WM              :
        {
          	AFX_MANAGE_STATE(AfxGetStaticModuleState());
            PString strHandle = (const char *)wParam;
            PStringArray tmp = strHandle.Tokenise(";");
            if (tmp.GetSize() >=3) strHandle= tmp[2];
            for(std::list<CIncomingDlg* >::iterator i = pSpriteX2Ctrl->m_listIncomingDlgs.begin();i!= pSpriteX2Ctrl->m_listIncomingDlgs.end();i++ ){
              if ((*i)->m_strHandle == strHandle) {
                //(*i)->OnBnClickedButtonReject();
                (*i)->DestroyWindow();
                delete *i;
                pSpriteX2Ctrl->m_listIncomingDlgs.erase(i);
                break;
              }
            }
						pSpriteX2Ctrl->OnClearedCall((LPCSTR)wParam, (LPCSTR)lParam, "");
        }	break;
			case YY_OnIncomingCall_WM             :
						pSpriteX2Ctrl->OnIncomingCall((LPCSTR)wParam,(LPCSTR) lParam);
					break;
			case YY_OnOutgoing_WM                 :
						pSpriteX2Ctrl->OnOutgoing((LPCSTR)wParam,(LPCSTR) lParam);
					break;
			case YY_OnCreatedYYScprite_WM         :
						pSpriteX2Ctrl->OnCreatedYYScprite( );
					break;
			case YY_OnAddFriend_WM                :
						pSpriteX2Ctrl->OnAddFriend((LPCSTR)wParam, lParam);
					break;
			case YY_HUNGUP_WM                :
        {
					//pSpriteX2Ctrl->m_pMgr->GetOPALMgr()->HangupCurrentCall(wParam);
          PString strHandle="";
          if (lParam>0 ){
            PString *p= (PString*) (lParam);
            strHandle = * p;
            delete p;
          }
          pSpriteX2Ctrl->m_pMgr->GetOPALMgr()->HangupCallSafe(wParam,strHandle);
        }
					break;
			case YY_ANSWERCALL_WM                :
        {
#if 1
         stThreadCommand * pCmd = new stThreadCommand();
          pCmd->cmd = em_threadAnswerCall;
          PString* pTag= new PString();
          pTag->sprintf("%s:%u", (const char*)wParam,lParam );
          pCmd->tag= pTag;
          pSpriteX2Ctrl->m_pMgr->GetMEMgr()->PostPTThreadCommand(pCmd);
#else
				  pSpriteX2Ctrl->m_pMgr->GetOPALMgr()->AnswerCall((LPCSTR)wParam, lParam );
#endif
        }
					break;
			case YY_ONMSGDOWNLOADFILE_WM                :
				pSpriteX2Ctrl->OnMsgDownloadFile((LPCSTR)wParam, lParam );
					break;
			case YY_ONMSGSENDFILEMESSAGE_WM                :
				pSpriteX2Ctrl->OnMsgSendFileMessage((LPCSTR)wParam );
					break;
			case YY_OnP2PFile_WM                :
				pSpriteX2Ctrl->OnP2PFile((LPCSTR)wParam );
					break;
			case YY_OnReceivingFileP2P_WM                :
				pSpriteX2Ctrl->OnReceivingFileP2P((LPCSTR)wParam );
					break;
			case YY_OnSendingFileP2P_WM                :
				pSpriteX2Ctrl->OnSendingFileP2P((LPCSTR)wParam );
					break;
			case YY_OnyFTOFState_WM                :
				pSpriteX2Ctrl->OnyFTOFState(wParam ,(LPCSTR)lParam );
					break;
			case YY_OnCallState_WM                :
				pSpriteX2Ctrl->OnCallState(wParam ,(LPCSTR)lParam );
					break;
			case YY_CREAT_REMOTE_DLG_WM                :
				{
					CRemoteDlg* pDlg = (CRemoteDlg*) wParam;
					pDlg->Create((UINT)lParam,FromHandle( pSpriteX2Ctrl->GetHWNDEX()) );pDlg->ShowWindow(SW_SHOW);
				}
					break;			//	 pSpriteX2Ctrl->IncomingCall();
			case YY_CREAT_INCOMING_DLG_WM                :
				{
						AFX_MANAGE_STATE(AfxGetStaticModuleState());
 					PStringArray arr = ((PString*)wParam)->Tokenise("|");
          bool bIsRinging= false;
           for(std::list<CIncomingDlg* > ::iterator i = pSpriteX2Ctrl->m_listIncomingDlgs.begin();i!=pSpriteX2Ctrl->m_listIncomingDlgs.end();i++){
             if ((*i)-> m_nTypeRinging == CIncomingDlg::em_ipp &&(*i)->m_strRemoteUserID== (const char*)arr[0]&& (*i)->GetSafeHwnd()!=NULL&& (*i)->IsWindowVisible() ){
               bIsRinging = true;
             }
            }
           if (bIsRinging == false){
             CWnd *pWnd=  FromHandle(pSpriteX2Ctrl->m_hwndMainContainerPanel );
             CIncomingDlg* p= new CIncomingDlg(pSpriteX2Ctrl,pWnd);
					    p->m_nTypeRinging = CIncomingDlg::em_ipp;
					    p->m_strHandle = "";
					    if (arr.GetSize()>0)
						    p->m_strRemoteUserID = (const char*)arr[0];
					    if (arr.GetSize()>1)
						    p->m_strRemoteUserDisplayName =(const char*)UTF8ToPlainText( (const char*)arr[1]);
              if (arr.GetSize()>2){
                memcpy(&p->m_RDPacket  , (const char*)arr[2].AsInteger(), sizeof(p->m_RDPacket)); 
              }

					    p->Create(IDD_DIALOG_INCOMING);
					    p->ShowWindow(SW_SHOW);
              p->SetOwner(pWnd);
					    pSpriteX2Ctrl->m_listIncomingDlgs.push_back(p);
           }
 				}
					break;	 
			//case YY_DESTROY_INCOMING_DLG_WM                :
			//	{
			//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
 		//		}
			//		break;	 
			case YY_DESTROY_REMOTE_DLG_WM                :
				{
					CRemoteDlg* pDlg = (CRemoteDlg*) wParam;
					pDlg->CloseWindow();
					pDlg->DestroyWindow(); 
				}
					break;	 
			case YY_OnNotify_WM                :
				{
          PString * str = (PString *  ) lParam;


					int retcode =  pSpriteX2Ctrl->OnNotify(wParam ,(LPCSTR)(const char*)(*str)/*lParam*/ );

          delete str;
          return retcode;
				}
					break;			 
			case YY_OnYYClientState_WM                :
				{
					pSpriteX2Ctrl->OnYYClientState(wParam ,(LPCSTR)lParam );
				}
					break;	 
			case YY_CREATE_POPIM_DLG_WM                :
				{
          pSpriteX2Ctrl->CreatePopIMDlg(wParam ,(LPCSTR)lParam );
				}
					break;				//		break;
			case YY_DESTROY_POPIM_DLG_WM                :
				{
           AFX_MANAGE_STATE(AfxGetStaticModuleState());
          pSpriteX2Ctrl->DestroyPopIMDlg(wParam   );
          if ( lParam == 1 ){//also close video panel
            if (pSpriteX2Ctrl->m_pVideoPanelDlg ){
              ::SendMessage(pSpriteX2Ctrl->m_pVideoPanelDlg->GetSafeHwnd(),  WM_CLOSE, 0, 0);
              ::SendMessage(pSpriteX2Ctrl->m_pVideoPanelDlg->GetSafeHwnd(), WM_DESTROY, 0, 0);
              pSpriteX2Ctrl->m_pVideoPanelDlg->DestroyWindow();
              delete pSpriteX2Ctrl->m_pVideoPanelDlg;
              pSpriteX2Ctrl->m_pVideoPanelDlg =NULL;
            }
          }
				}
					break;			
			case YY_MAIN_SHOWORHIDE_WIN                :
				{
          AFX_MANAGE_STATE(AfxGetStaticModuleState());
          ::ShowWindow((HWND) wParam, (int)lParam);
          
				}
					break;		
			case YY_SYNC_DATA_MAINTHREAD                :
				{
          AFX_MANAGE_STATE(AfxGetStaticModuleState());
          PString* strData =  (PString*)wParam;
          PStringArray arr= strData->Tokenise(";");
          if (arr.GetSize() >=2){
            if ( pSpriteX2Ctrl->m_pVideoPanelDlg ){
              pSpriteX2Ctrl->m_pVideoPanelDlg->OnMenuItem(arr[1].AsUnsigned());
            }
          }
          delete strData;
          
				}
					break; 
      case YY_CREATE_CCVIDEOLAYOUT_WM                :
				{
          AFX_MANAGE_STATE(AfxGetStaticModuleState());
          if (wParam == 0 )
          {//P2pmeeting
            delete pSpriteX2Ctrl->m_pVideoPanelDlg;
            //if (pSpriteX2Ctrl->m_pVideoPanelDlg==NULL)
            {
              pSpriteX2Ctrl->m_pVideoPanelDlg = new CVideoPanelDlg( GetDesktopWindow() , pSpriteX2Ctrl->m_pMgr);
              pSpriteX2Ctrl->m_pVideoPanelDlg->m_nRoomID = wParam;
              pSpriteX2Ctrl->m_pVideoPanelDlg->m_bMCCLayout = !(wParam == 0);
              pSpriteX2Ctrl->m_pVideoPanelDlg->Create(IDD_DIALOG_VIDEO_PANEL, GetDesktopWindow());
            }
          }else{
            // mcc 
            if (pSpriteX2Ctrl->m_pVideoPanelDlg  ){
#ifndef _DEBUG
              if (pSpriteX2Ctrl->m_pVideoPanelDlg->GetSafeHwnd() == NULL || pSpriteX2Ctrl->m_pVideoPanelDlg->m_nRoomID != pSpriteX2Ctrl->m_pMgr->GetMEMgr()->m_nCurectEnteredRoom )
#endif
              {
                delete pSpriteX2Ctrl->m_pVideoPanelDlg;pSpriteX2Ctrl->m_pVideoPanelDlg=0;
              }
            }
            if (pSpriteX2Ctrl->m_pVideoPanelDlg == NULL){
               pSpriteX2Ctrl->m_pVideoPanelDlg = new CVideoPanelDlg( 0, pSpriteX2Ctrl->m_pMgr);
              pSpriteX2Ctrl->m_pVideoPanelDlg->m_nRoomID = pSpriteX2Ctrl->m_pMgr->GetMEMgr()->m_nCurectEnteredRoom;
              pSpriteX2Ctrl->m_pVideoPanelDlg->m_bMCCLayout = !(wParam == 0);
              pSpriteX2Ctrl->m_pVideoPanelDlg->Create(IDD_DIALOG_VIDEO_PANEL); 
               
            }else{
              
            }
          }
          pSpriteX2Ctrl->m_pVideoPanelDlg->OnMenuItem(ID_Normal);
          pSpriteX2Ctrl->m_pVideoPanelDlg->ShowWindow(SW_SHOW);
          pSpriteX2Ctrl->m_pVideoPanelDlg->SetForegroundWindow();
          
				}
				break;				//		break;		

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
    if (   wParam==VK_ESCAPE){
      pSpriteX2Ctrl->IMDlgControl("20;9");
     /* if ( pSpriteX2Ctrl->m_pVideoPanelDlg&& pSpriteX2Ctrl->m_pVideoPanelDlg->IsWindowVisible() ){
        pSpriteX2Ctrl->m_pVideoPanelDlg->OnMenuItem( ID_Menu_Layout_normal);
        return TRUE;
      }*/
    }
    break;

      };
	}
	//if (uMsg== 1011) pSpriteX2Ctrl->OnNotifyContactActionBy(0,0);

  //YYVideoOutputDevice * vodw = (YYVideoOutputDevice *)GetWindowLong(hWnd, 0);
  //if (vodw != NULL)
  //  return vodw->WndProc(uMsg, wParam, lParam);

	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}
static int nRegisterClass =0;
extern PString UTF8ToPlainText(const char* lpszContent);
extern PString PlainTextToUTF8(const char* lpszContent);
#define D_ACTIVE_WIN_TITLE "yymeeting_main_"
void CYYSpriteX2Ctrl::CreateMyWin( )
{
	strwndClassName =D_ACTIVE_WIN_TITLE;//.Format("YYVideoWin223", nRegisterClass++);
#ifdef D_CMYWIN
  m_pMyWnd= new CMyHwnd();
  RECT rect={0,0,0,0};
	// TODO: Add your specialized code here and/or call the base class
	CString strMyClass;

	// load stock cursor, brush, and icon for
	// my own window class

	try
	{
	   strMyClass = AfxRegisterWndClass(
		  CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS
		  );
	}
	catch (CResourceException* pEx)
	{
		  AfxMessageBox(
			 _T("Couldn't register class! (Already registered?)"));
		  pEx->Delete();
		  return  ;
	}
  //CSFVideoWnd ll;
  //ll.Create(WS_CHILD|WS_VISIBLE,rect,this,AFX_IDW_PANE_FIRST);m_hWndEx=ll.GetSafeHwnd();
  BOOL bRet =  m_pMyWnd->CreateEx(WS_CHILD, strMyClass, "ccvision1", WS_POPUP, rect, GetDesktopWindow(), AFX_IDW_PANE_FIRST);
  int d= ::GetLastError();
  m_hWndEx = m_pMyWnd->GetSafeHwnd();
  //m_pMyWnd->Create(WS_CHILD|WS_VISIBLE,rect,0,AFX_IDW_PANE_FIRST);
#else
  m_pMyWnd=NULL;
  DWORD dwStyle;
  HWND hParent;
	
  WNDCLASS wndClass;
  memset(&wndClass, 0, sizeof(wndClass));
  wndClass.style = CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS;
	wndClass.lpszClassName = strwndClassName.GetBuffer();
	wndClass.lpfnWndProc = CYYSpriteX2Ctrl::WndProc;
  wndClass.cbWndExtra = sizeof(this);
  (RegisterClass(&wndClass));

    m_hWndEx = CreateWindow(wndClass.lpszClassName,
                          strwndClassName, 
                          ( WS_POPUP/*|WS_CLIPCHILDREN */),
                          0, 0, 10, 10,
                          HWND_DESKTOP/*NULL*/, NULL, GetModuleHandle(NULL), this);
#endif
 if (m_hWndEx==NULL)
	 MessageBox("Failed to create window");
 else
 {
	 g_YYSpriteCtrls[m_hWndEx] = this;
 }
}

//#ifdef _DEBUG
//#define new DEBUG_NEW
//#endif



IMPLEMENT_DYNCREATE(CYYSpriteX2Ctrl, COleControl)



// Message map

BEGIN_MESSAGE_MAP(CYYSpriteX2Ctrl, COleControl)
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
  ON_WM_CLOSE()
  ON_WM_COPYDATA()
END_MESSAGE_MAP()



// Dispatch map

BEGIN_DISPATCH_MAP(CYYSpriteX2Ctrl, COleControl)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "Login", dispidLogin, Login, VT_BSTR/*VT_I4*/, VTS_BSTR VTS_BSTR)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "MakeCall", dispidMakeCall, MakeCall, VT_I4, VTS_BSTR VTS_BSTR)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "Hangup", dispidHangup, Hangup, VT_I4, VTS_BSTR VTS_BSTR)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "initialise", dispidinitialise, initialise, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "GetCameraNames", dispidGetCameraNames, GetCameraNames, VT_BSTR, VTS_NONE)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "SetCameraName", dispidSetCameraName, SetCameraName, VT_I4, VTS_BSTR VTS_BSTR)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "IsLogin", dispidIsLogin, IsLogin, VT_I4, VTS_NONE)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "Logout", dispidLogout, Logout, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "ShowUserPanel", dispidShowUserPanel, ShowUserPanel, VT_I4, VTS_BSTR VTS_I4 VTS_I2 VTS_I2 VTS_I2 VTS_I2)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "GetFriendsList", dispidGetFriendsList, GetFriendsList, VT_BSTR, VTS_BSTR)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "AddFriend", dispidAddFriend, AddFriend, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "GetFirendListEx", dispidGetFirendListEx, GetFirendListEx, VT_BSTR, VTS_NONE)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "GetLastErrorCode", dispidGetLastErrorCode, GetLastErrorCode, VT_I4, VTS_NONE)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "ConfirmContactActionBy", dispidConfirmContactActionBy, ConfirmContactActionBy, VT_I4, VTS_I4 VTS_I4 VTS_I4)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "ListOnlineUser", dispidListOnlineUser, ListOnlineUser, VT_BSTR, VTS_NONE)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "GetBlockList", dispidGetBlockList, GetBlockList, VT_BSTR, VTS_NONE)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "GetTree", dispidGetTree, GetTree, VT_BSTR, VTS_NONE)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "Hungup", dispidHungup, Hungup, VT_I4, VTS_BSTR)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "HangupEx", dispidHangupEx, HangupEx, VT_I4, VTS_BSTR)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "AddFriendEx", dispidAddFriendEx, AddFriendEx, VT_I4, VTS_I4 VTS_BSTR)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "Subscribe", dispidSubscribe, Subscribe, VT_BSTR, VTS_BSTR)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "SendOfflineIM", dispidSendOfflineIM, SendOfflineIM, VT_I4, VTS_BSTR)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "ViewUserData", dispidViewUserData, ViewUserData, VT_BSTR, VTS_BSTR)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "RemoveBlock", dispidRemoveBlock, RemoveBlock, VT_I4, VTS_I4)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "RemoveFriend", dispidRemoveFriend, RemoveFriend, VT_I4, VTS_I4)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "UpdateUserData", dispidUpdateUserData, UpdateUserData, VT_I4, VTS_BSTR)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "PPT", dispidPPT, PPT, VT_I4, VTS_BSTR)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "AcceptP2PFile", dispidAcceptP2PFile, AcceptP2PFile, VT_I4, VTS_BSTR)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "P2PAFile", dispidP2PAFile, P2PAFile,VT_BSTR /*VT_I4*/, VTS_BSTR)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "ChangeStatus", dispidChangeStatus, ChangeStatus, VT_I4, VTS_I4 VTS_BSTR)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "FTOF", dispidFTOF, FTOF, VT_I4, VTS_BSTR VTS_BSTR)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "WindowProc", dispidWindowProc, WindowProc, VT_I4, VTS_I4 VTS_I4 VTS_BSTR)
	DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "SheduleMeet", dispidSheduleMeet, SheduleMeet, VT_I4, VTS_BSTR)
  DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "EnterRoom", dispidEnterRoom, EnterRoom, VT_I4, VTS_I4 VTS_BSTR)
  DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "DlgLocalVideo", dispidDlgLocalVideo, DlgLocalVideo, VT_BSTR, VTS_NONE)
  DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "DlgOption", dispidDlgOption, DlgOption, VT_EMPTY, VTS_I4)
  DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "yDestroy", dispidyDestroy, yDestroy, VT_EMPTY, VTS_NONE)
  DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "IMDlgControl", dispidIMDlgControl, IMDlgControl, VT_BSTR/*VT_EMPTY*/, VTS_BSTR)
  DISP_FUNCTION_ID(CYYSpriteX2Ctrl, "OnNotifyEx", dispidOnNotifyEx, OnNotifyEx, VT_I4, VTS_I4 VTS_BSTR)
END_DISPATCH_MAP()



// Event map

BEGIN_EVENT_MAP(CYYSpriteX2Ctrl, COleControl)
	EVENT_CUSTOM_ID("OnCallState", dispidOnCallState, OnCallState,  VTS_I4 VTS_BSTR)
	EVENT_CUSTOM_ID("OnYYClientState", dispidOnYYClientState, OnYYClientState,  VTS_I4 VTS_BSTR)
	EVENT_CUSTOM_ID("OnNotify", dispidOnNotify, OnNotify,  VTS_I4 VTS_BSTR)
	//EVENT_CUSTOM_ID("OnCallState", dispidOnCallState, OnCallState, VTS_I4 VTS_BSTR)
	EVENT_CUSTOM_ID("OnSETPIXELS_NOCONV_WMEx2", dispidOnSETPIXELS_NOCONV_WMEx2, OnSETPIXELS_NOCONV_WMEx2,  VTS_BSTR VTS_VARIANT)
	EVENT_CUSTOM_ID("OnSETPIXELS_NOCONV_WMEx", dispidOnSETPIXELS_NOCONV_WMEx, OnSETPIXELS_NOCONV_WMEx,  VTS_BSTR VTS_PVARIANT)
	EVENT_CUSTOM_ID("OnSetTlight", dispidOnSetTlight, OnSetTlight, VTS_I4 VTS_BSTR)
	EVENT_CUSTOM_ID("OnyFTOFState", dispidOnyFTOFState, OnyFTOFState, VTS_I4 VTS_BSTR)
	EVENT_CUSTOM_ID("OnSETPIXELS_NOCONV", dispidOnSETPIXELS_NOCONV, OnSETPIXELS_NOCONV, VTS_BSTR VTS_VARIANT)
	EVENT_CUSTOM_ID("OnSETPIXELS_NOCONV2", dispidOnSETPIXELS_NOCONV2, OnSETPIXELS_NOCONV2,  VTS_BSTR VTS_VARIANT)
	//EVENT_CUSTOM_ID("OnSETPIXELS_NOCONV", dispidOnSETPIXELS_NOCONV, OnSETPIXELS_NOCONV, VTS_BSTR VTS_I4 VTS_I4 VTS_I4 VTS_I4)
	EVENT_CUSTOM_ID("OnSendingFileP2P", dispidOnSendingFileP2P, OnSendingFileP2P, VTS_BSTR)
	EVENT_CUSTOM_ID("OnReceivingFileP2P", dispidOnReceivingFileP2P, OnReceivingFileP2P, VTS_BSTR)
	EVENT_CUSTOM_ID("OnP2PFile", dispidOnP2PFile, OnP2PFile,  VTS_BSTR)
	EVENT_CUSTOM_ID("OnMsgDownloadFile", dispidOnMsgDownloadFile, OnMsgDownloadFile,  VTS_BSTR VTS_I4)
	EVENT_CUSTOM_ID("OnMsgSendFileMessage", dispidOnMsgSendFileMessage, OnMsgSendFileMessage,  VTS_BSTR)
	EVENT_CUSTOM_ID("OnBuddyStatusChanged", dispidOnBuddyStatusChanged, OnBuddyStatusChanged, VTS_BSTR)
	EVENT_CUSTOM_ID("OnCreatedYYScprite", dispidOnCreatedYYScprite, OnCreatedYYScprite,  VTS_NONE)
	EVENT_CUSTOM_ID("OnNotifyContactActionBy", dispidOnNotifyContactActionBy, OnNotifyContactActionBy,  VTS_I4 VTS_I4)
	EVENT_CUSTOM_ID("OnAddFriend", dispidOnAddFriend, OnAddFriend, VTS_BSTR VTS_I4)
	EVENT_CUSTOM_ID("OnEstableshedCall", dispidOnEstableshedCall, OnEstableshedCall, VTS_BSTR VTS_BSTR)
	EVENT_CUSTOM_ID("OnClearedCall", dispidOnClearedCall, OnClearedCall, VTS_BSTR VTS_BSTR VTS_BSTR)
	EVENT_CUSTOM_ID("OnIncomingCall", dispidOnIncomingCall, OnIncomingCall, VTS_BSTR VTS_BSTR)
	EVENT_CUSTOM_ID("OnOutgoing", dispidOnOutgoing, OnOutgoing, VTS_BSTR VTS_BSTR)
	EVENT_CUSTOM_ID("OnHeartbeatFeedback", dispidOnHeartbeatFeedback, OnHeartbeatFeedbac, VTS_BSTR)
	EVENT_CUSTOM_ID("OnHeartbeatTimeout", dispidOnHeartbeatTimeout, OnHeartbeatTimeout, VTS_NONE)
	EVENT_CUSTOM_ID("OnSessionExpired", dispidOnSessionExpired, OnSessionExpired, VTS_NONE)
	EVENT_CUSTOM_ID("OnLoginReplaced", dispidOnLoginReplaced, OnLoginReplaced, VTS_BSTR)
	EVENT_CUSTOM_ID("OnNotifyOfflineIM", dispidOnNotifyOfflineIM, OnNotifyOfflineIM, VTS_I4 VTS_I4 VTS_BSTR)
END_EVENT_MAP()



// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CYYSpriteX2Ctrl, 1)
	PROPPAGEID(CYYSpriteX2PropPage::guid)
END_PROPPAGEIDS(CYYSpriteX2Ctrl)



// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CYYSpriteX2Ctrl, "YYSPRITEX2.YYSpriteX2Ctrl.1",
	0x74e59815, 0x8639, 0x4e51, 0x99, 0xa1, 0x47, 0xf1, 0xa5, 0x71, 0x60, 0xd9)



// Type library ID and version

IMPLEMENT_OLETYPELIB(CYYSpriteX2Ctrl, _tlid, _wVerMajor, _wVerMinor)



// Interface IDs

const IID BASED_CODE IID_DYYSpriteX2 =
		{ 0x83A7866F, 0xF1E8, 0x4EFD, { 0x81, 0xB9, 0x34, 0xC3, 0xD, 0x35, 0x61, 0x99 } };
const IID BASED_CODE IID_DYYSpriteX2Events =
		{ 0xAB76D3A, 0x9CAA, 0x44CA, { 0x9C, 0xD, 0xB7, 0x28, 0x43, 0xC1, 0x4C, 0x58 } };



// Control type information

static const DWORD BASED_CODE _dwYYSpriteX2OleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CYYSpriteX2Ctrl, IDS_YYSPRITEX2, _dwYYSpriteX2OleMisc)



// CYYSpriteX2Ctrl::CYYSpriteX2CtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CYYSpriteX2Ctrl

BOOL CYYSpriteX2Ctrl::CYYSpriteX2CtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: Verify that your control follows apartment-model threading rules.
	// Refer to MFC TechNote 64 for more information.
	// If your control does not conform to the apartment-model rules, then
	// you must modify the code below, changing the 6th parameter from
	// afxRegApartmentThreading to 0.
#if 0
	if (bRegister)
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_YYSPRITEX2,
			IDB_YYSPRITEX2,
			afxRegApartmentThreading,
			_dwYYSpriteX2OleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
#else
// TODO: 验证您的控件是否符合单元模型线程处理规则。
 // 有关更多信息，请参考 MFC 技术说明 64。
 // 如果您的控件不符合单元模型规则，则
 // 必须修改如下代码，将第六个参数从
 // afxRegApartmentThreading 改为 0。

 /*if (bRegister)
  return AfxOleRegisterControlClass(
   AfxGetInstanceHandle(),
   m_clsid,
   m_lpszProgID,
   IDS_SIPXTOCX,
   IDB_SIPXTOCX,
   afxRegApartmentThreading,
   _dwsipXtOCXOleMisc,
   _tlid,
   _wVerMajor,
   _wVerMinor);
 else
  return AfxOleUnregisterClass(m_clsid, m_lpszProgID);*/
 if   (bRegister)     
 {     
 HRESULT   hr   =   S_OK   ;     
     
 //   register   as   safe   for   scripting     
 hr   =   CreateComponentCategory(CATID_SafeForScripting,     
 L"Controls   that   are   safely   scriptable");
 if(FAILED(hr))
  return   FALSE;
 hr = RegisterCLSIDInCategory(m_clsid,   CATID_SafeForScripting);    
 if(FAILED(hr))
  return   FALSE;
 //   register   as   safe   for   initializing     
 hr =CreateComponentCategory(CATID_SafeForInitializing,     
 L"Controls   safely   initializable   from   persistent   data");        
 if(FAILED(hr))
  return   FALSE;
     
 hr   =   RegisterCLSIDInCategory(m_clsid,   CATID_SafeForInitializing);     
     
 if   (FAILED(hr))     
      return   FALSE;     
 return   AfxOleRegisterControlClass(     
 AfxGetInstanceHandle(),     
 m_clsid,     
 m_lpszProgID,     
 IDS_YYSPRITEX2,     
 IDS_YYSPRITEX2,     
 afxRegInsertable   |   afxRegApartmentThreading,     
 _dwYYSpriteX2OleMisc,     
 _tlid,     
 _wVerMajor,     
 _wVerMinor);     
 }     
 else     
 {     
 HRESULT hr =   S_OK   ;     
     
 hr=UnRegisterCLSIDInCategory(m_clsid,CATID_SafeForScripting);     
     
 if(FAILED(hr))     
  return   FALSE;     
     
 hr=UnRegisterCLSIDInCategory(m_clsid,   CATID_SafeForInitializing);     
     
 if(FAILED(hr))     
 return   FALSE;     
     
 return   AfxOleUnregisterClass(m_clsid,   m_lpszProgID);     
  } 






#endif
}

extern void initAudio();

// CYYSpriteX2Ctrl::CYYSpriteX2Ctrl - Constructor
CYYSpriteX2Ctrl::CYYSpriteX2Ctrl()
{

  m_bInit= false;
	//if (g_pSpriteX2Ctrl ) delete g_pSpriteX2Ctrl;
	//g_pSpriteX2Ctrl = this;
	InitializeIIDs(&IID_DYYSpriteX2, &IID_DYYSpriteX2Events);
 // m_pCUGTree = NULL;
 // m_hwndLocalVideoPanel= NULL;
 // m_wndCaptureDlg=0;
 // 
 // m_pcreatefreegroupdlg= m_pguideAudioDlg=m_pmessageboxdlg = m_plistonlinedlg = m_pviewuserdlg = m_pcreatemeetdlg=m_pOptdlg =m_pmwelcomedlg= NULL;
 // //int p= sizeof( BBQ_MeetingInfo );
 // m_pVideoPanelDlg =NULL;
 // //g_InitMultLanguageString();
	//m_hWndEx =NULL;	
 // //CreateMyWin();
	//m_pMgr =0;
  //MessageBox("CYYSpriteX2Ctrl");
 //initAudio();
	//LPNMgr::RegisterPlugIN(m_pMgr);
	//HINSTANCE dll_hInstance = GetModuleHandle("YYSpriteX-2.ocx");
	//if (dll_hInstance==NULL) PAssertAlways("dll_hInstance is null");
	//else PAssertAlways("dll_hInstance is not null");
	 
}



// CYYSpriteX2Ctrl::~CYYSpriteX2Ctrl - Destructor

CYYSpriteX2Ctrl::~CYYSpriteX2Ctrl()
{
  if ( m_bInit )
    yDestroy();
}



// CYYSpriteX2Ctrl::OnDraw - Drawing function

void CYYSpriteX2Ctrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	if (!pdc)
		return;

	// TODO: Replace the following code with your own drawing code.
	pdc->FillRect(rcBounds, CBrush::FromHandle((HBRUSH)GetStockObject(BLACK_BRUSH/*WHITE_BRUSH*/)));
	pdc->Ellipse(rcBounds);
}



// CYYSpriteX2Ctrl::DoPropExchange - Persistence support
CString m_StockProps, m_cc_meet_id,m_cc_id;
 CString  GetCCVisionPath( )
{
  CString strresult="";
  TCHAR* sRoot=_T("SOFTWARE\\YYMeeting");
  HKEY hKey;
  if(RegOpenKeyEx(HKEY_CURRENT_USER,sRoot,0,KEY_ALL_ACCESS,&hKey) == ERROR_SUCCESS){
    // TCHAR sKey[MAX_PATH]=_T("");
    DWORD dwSize; 
    char strPrinting[1024]={0};
    //sprintf(strPrinting, "%u;%u;%s", nPrintingPage,isEndPage?1:0, g_strSavePath.c_str() );
    dwSize=sizeof(strPrinting);
    	
    DWORD valtype;
    //ret=RegQueryValueEx(hKey,_T("429dbd81"),0, &valtype,(BYTE*)strPrinting,&dwSize);
    //ret=RegQueryValueEx(hKey,"path",0, &valtype,(BYTE*)strPrinting,&dwSize);
    int ret=RegQueryValueEx(hKey,(const char*)"CC_PATH",0, &valtype,(BYTE*)strPrinting,&dwSize);
  
      
  if(ret==ERROR_SUCCESS)  {  
    strresult=strPrinting;
  }
  //
}
RegCloseKey (hKey);
return strresult;
	 
}
void CYYSpriteX2Ctrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);
  PX_String(pPX,_T("_StockProps"),m_StockProps);
  PX_String(pPX,_T("cc_id"),m_cc_id);
  PX_String(pPX,_T("cc_meet_id"),m_cc_meet_id);
  //AfxMessageBox((const char*) m_cc_meet_id);
  //m_cc_id ="2007";
  if (m_cc_id.GetLength()>0 ){
   // m_cc_id ="2007";
    PTCPSocket socket;
    if ( socket.Listen(5, g_httpPort)){
     

      PString strPath = GetCCVisionPath();
      strPath.sprintf("\\CCVisionD.exe -talker 0%s", (const char*)m_cc_id);

      WinExec((const char*)  strPath, SW_SHOW);
    }else{
      PHTTPClient client("cc");
      PString url,databody;
      PMIMEInfo outMime, replymime;
      url.sprintf("http://127.0.0.1:%d/broadcast/?talker=0%s", g_httpPort, (const char*) m_cc_id);
      client.ExecuteCommand(PHTTP::GET, url, outMime, databody, replymime);
    }
    m_cc_id = "";

  }
  //auto mcc meeting
  if (m_cc_meet_id.GetLength()>0 ){
   // m_cc_id ="2007";
    PTCPSocket socket;
    if ( socket.Listen(5, g_httpPort)){
     

      PString strPath = GetCCVisionPath();
      strPath.sprintf("\\CCVision.exe -talker 1%s", (const char*)m_cc_meet_id);

      WinExec((const char*)  strPath, SW_SHOW);
    }else{
      PHTTPClient client("cc");
      PString url,databody;
      PMIMEInfo outMime, replymime;
      url.sprintf("http://127.0.0.1:%d/broadcast/?talker=1%s", g_httpPort, (const char*) m_cc_meet_id);
      client.ExecuteCommand(PHTTP::GET, url, outMime, databody, replymime);
    }
    m_cc_meet_id = "";

  }

	// TODO: Call PX_ functions for each persistent custom property.
}



// CYYSpriteX2Ctrl::OnResetState - Reset control to default state

void CYYSpriteX2Ctrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}



// CYYSpriteX2Ctrl::AboutBox - Display an "About" box to the user

void CYYSpriteX2Ctrl::AboutBox()
{
	CDialog dlgAbout(IDD_ABOUTBOX_YYSPRITEX2);
	dlgAbout.DoModal();
}



// CYYSpriteX2Ctrl message handlers
void CYYSpriteX2Ctrl::ReInitYYmeetingUser( )
{
  m_pMgr->GetOPALMgr()->HangupCallSafe();
  if (m_pOptdlg)
   delete m_pOptdlg; 
  if (m_plistonlinedlg)
    delete m_plistonlinedlg; 
  if (m_pviewuserdlg)
    delete m_pviewuserdlg;
  if (m_pcreatemeetdlg)
    delete m_pcreatemeetdlg ;
  if (m_pcreatefreegroupdlg)
    delete m_pcreatefreegroupdlg ;
  if (m_pmessageboxdlg)
    delete m_pmessageboxdlg ;
  if (m_pmwelcomedlg)
    delete m_pmwelcomedlg ;
  if (m_pguideAudioDlg)
    delete m_pguideAudioDlg ;
  if (m_pVideoPanelDlg)
    delete m_pVideoPanelDlg ;
 
m_pcreatefreegroupdlg = m_pguideAudioDlg = m_pmessageboxdlg = m_pcreatemeetdlg= m_pviewuserdlg= m_plistonlinedlg = m_pOptdlg=m_pmwelcomedlg=NULL;
//clear im
  for(MapIMDialogs::iterator i=  m_mapIMDlgs.begin();i!= m_mapIMDlgs.end();i++)
    delete i->second;

  for(MapIMDialogs::iterator i=  m_mapMCCNotifyDlgs.begin();i!= m_mapMCCNotifyDlgs.end();i++)
    delete i->second;

  m_mapMCCNotifyDlgs.clear();
  m_mapIMDlgs.clear();

  m_pMgr->GetMEMgr()-> ReInit();
}
bool CYYSpriteX2Ctrl::LoginInThread(LPCTSTR strURL, LPCTSTR tag)//execute in ptlib thread
{
	//AFX_MANAGE_STATE(AfxGetStaticModuleState());
  CString strResult;
#ifdef _DEBUG
  if (strURL== NULL || strlen(strURL)<=0)
    strURL="2001:123";
#endif 
  PStringArray arr= PString(tag).Tokenise(";");
  if (arr.GetSize()>=3){
    m_pMgr->GetMEMgr()->m_stLogin.status= arr[0].AsInteger();
    m_pMgr->GetMEMgr()->m_stLogin.bremenberpws= arr[1]=="1";
    m_pMgr->GetMEMgr()->m_stLogin.bautologin= arr[2]=="1";
  }
  //const char* ip = (const char*) m_pMgr->GetOPALMgr()->m_strVfonDomain ;
  if ( m_pMgr->GetMEMgr()->SetServerAddress(m_pMgr->GetOPALMgr()->m_strVfonDomain, D_SERVER_PORT)){
    //check version
    PString downloadURL;
    SIMD_VERSIONINFO pVer={0};
    m_pMgr->GetMEMgr()->QueryVersionInfo(&pVer,downloadURL);
    uint32 clientVersionRequired = ((pVer.client.majorRequired & 0xff) << 24) | ((pVer.client.minorRequired & 0xff) << 16) | (pVer.client.buildRequired & 0xffff);
    if (clientVersionRequired> 0 && m_pMgr->GetMEMgr()->GetYYmeetingVersion()< clientVersionRequired ){
      m_pMgr->GetMEMgr()->Logout();
      //notify login result 
      strResult.Format("%d;%d", (1), 10/* must download new version*/);
      OnNotify_WM(em_UserLogin, strResult);

      return false;

    }    
  }
  
  //int port = D_SERVER_PORT;
	// TODO: Add your dispatch handler code here
  int nLogin = m_pMgr->Register(strURL);
  if ( nLogin == 0){


  
    //loaduserfriends
    if ( g_VideoLayoutForce == 0) //特殊处理
    {
      //notify login result 
      strResult.Format("%d;%d", (nLogin), m_pMgr->GetMEMgr()->m_stLogin.status);
      OnNotify_WM(em_UserLogin, strResult);
      CString str= GetFriendsListReturnCString("1");
      
      OnNotify_WM(em_UserInfoFriendOrCUGOrFreeGroup, "1"+str);
      //loadcug
      str= GetTreeReturnCString(true);  
      if (str.GetLength()>0){
        OnNotify_WM(em_UserInfoFriendOrCUGOrFreeGroup, "2"+str);
      //loadfreegroup
        OnNotify_WM(em_UserInfoFriendOrCUGOrFreeGroup, "3"+str);
      }
      //loadsipaccounts
      uint32 nLen=0;
      char* pData=NULL;
      m_pMgr->GetMEMgr()->DownloadDataBlock( "@MySipAccounts@", "1",nLen, pData );
      if (pData!=NULL){
        PString strSIPS = pData;
        m_pMgr->GetOPALMgr()->SipAccountsPStringToList(strSIPS);
        OnNotify_WM(em_UserInfoFriendOrCUGOrFreeGroup, "4"+strSIPS);
      }

      delete pData;

      //send history to silveright 
      PString* pMsg =new PString();
      pMsg->sprintf("0;%u",  m_pMgr->GetMEMgr()->GetID());
      
      //*pMsg = strMsg;
      stThreadCommand * pCmd = new stThreadCommand();
      pCmd->cmd = em_threadNotifyHistory;
      pCmd->tag =  (void*)pMsg;
      
      m_pMgr->GetMEMgr()->PostPTThreadCommand(pCmd);
      m_pMgr->GetMEMgr()->NotifyReady();
      PThread::Sleep(200);
      m_pMgr->GetMEMgr()->DeleteOfflineIM();
    }
  }else{
    //notify login result 
    strResult.Format("%d;%d", (nLogin), m_pMgr->GetMEMgr()->m_stLogin.status);
    OnNotify_WM(em_UserLogin, strResult);

  }

  m_pMgr->GetMEMgr()->CheckAutoDoTask();
  return true ;

}


BSTR CYYSpriteX2Ctrl::Login(LPCTSTR strURL, LPCTSTR tag)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  m_hwndMainContainerPanel = ::AfxGetMainWnd()->GetSafeHwnd();
  CString strResult;
    ReInitYYmeetingUser();

#if 1
  PString strTag=  PString(tag);
  if (strTag == "logout"){
    m_pMgr->GetMEMgr()->Logout();
  }else if(strTag =="cc"){
    strResult=  m_pMgr->Register(strURL)?"1":"0";
    return strResult.AllocSysString();
  }
  else{
    PString* pMsg=new PString();
    *pMsg = PString(strURL) +PString(",")+ (tag);
    stThreadCommand * pCmd = new stThreadCommand();
    pCmd->cmd = em_threadLogin;
    pCmd->tag =  (void*)pMsg;
    
  //MessageBox(strURL);
    m_pMgr->GetMEMgr()->PostPTThreadCommand(pCmd);
   
   /* char tmp[128]={0};
    g_GetSettingStringSafe("welcome_not_show",tmp, "0");
    if (atoi(tmp)== 0)
      IMDlgControl("11:0");*/
  }
#else
  ReInitYYmeetingUser();
//#ifdef _DEBUG
//  if (strURL== NULL || strlen(strURL)<=0)
//    strURL="2001:123";
//#endif
  PStringArray arr= PString(tag).Tokenise(";");
  if (arr.GetSize()>=3){
    m_pMgr->GetMEMgr()->m_stLogin.status= arr[0].AsInteger();
    m_pMgr->GetMEMgr()->m_stLogin.bremenberpws= arr[1]=="1";
    m_pMgr->GetMEMgr()->m_stLogin.bautologin= arr[2]=="1";
  }
	// TODO: Add your dispatch handler code here
  int nLogin = m_pMgr->Register(strURL);
  strResult.Format("%d;%d", (nLogin), m_pMgr->GetMEMgr()->m_stLogin.status);
  //send history to silveright 
  PString* pMsg =new PString();
  pMsg->sprintf("0;%u",  m_pMgr->GetMEMgr()->GetID());

  //*pMsg = strMsg;
  stThreadCommand * pCmd = new stThreadCommand();
  pCmd->cmd = em_threadNotifyHistory;
  pCmd->tag =  (void*)pMsg;
  
  m_pMgr->GetMEMgr()->PostPTThreadCommand(pCmd);
  if (nLogin == 0){
    char tmp[128]={0};
    g_GetSettingStringSafe("welcome_not_show",tmp, "0");
    if (atoi(tmp)== 0){
#if 0
      IMDlgControl("11:0");
#else
      //COptionsExDlg * p =m_pmwelcomedlg= new COptionsExDlg();
     
      //p->m_pMgr= m_pMgr;
      //p->m_emStyle= em_welcomeDlg;
      //p->m_nUserID = uid;
      //p->m_nYYmeetingUserID =  m_pMgr->GetMEMgr()->GetID();
      ////p->m_strUserAlias = (const char*) strValues[2];
      //p->Create(IDD_DIALOG_OPTIONS);

#endif
    }else{
    }
  }
#endif
  return strResult.AllocSysString();

}

LONG CYYSpriteX2Ctrl::MakeCall(LPCTSTR strURl, LPCTSTR tag)
{
  //test
//  m_pMgr->GetMEMgr()->NotifyMCCEventCMD(0,0, "pTransmit->strOragnalFileName",   
//  m_pMgr->GetMEMgr()->NotifyMCCEventCMD(0,1, 0,  0);
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
#ifdef _DEBUG
  if (strURl== NULL || strlen(strURl)<=0){
    strURl="z:100";tag="AAAAAA";
  }
#endif
	CString strProto, strValue=strURl ;
  int nDelayForCall=0;
	if (strValue.GetLength()>0&& strURl[1]==':')
	{
		char key = strURl[0];
		switch (key)
		{
		case 'f'://yy room
			{ //m_pMgr->GetOPALMgr()->HangupCallSafe();
        //m_pMgr->GetOPALMgr()->HangupCallSafe(0, "" );
        nDelayForCall=3000;
			}
			//break;
		case 'z'://yy call
			{
        //g_VideoLayoutForce=0;
				if (key == 'z' && m_pMgr->GetOPALMgr()->IsInCallSession(&strURl[2])) 
          return -2;
        PString str = PString(&strURl[2]);
        const char* pConf = tag;
        if (str.Find("100")!= P_MAX_INDEX && ( (tag!=NULL && strlen(tag)<=0 )|| tag==NULL)){
          pConf=D_LOOPBACK_MCU_CONFID;
        }
        if (pConf && strlen(tag)>0)
          m_pMgr->GetMEMgr()-> m_nCurectEnteredRoom = m_pMgr->GetMEMgr()->GetRoomIDByMeetingID(pConf);
        if (str.AsUnsigned() == m_pMgr->GetMEMgr()->GetID())
        {
          stCallUserData data={0};data.calltype= em_CallSip;data.callrepairsockettype = em_standard_udp;
          // self call
          PString strURL;strURL.sprintf("sip:%s@%s:%d",(const char*)str,    (const char*)m_pMgr->GetOPALMgr()->m_addrLocalinterface.AsString(),  m_pMgr->GetOPALMgr()->m_nlistenPort );
           m_pMgr->MakeSipCall(strURL, false ,&data);
           return 0;
        }
        //const char* pConf= (str.Find("2006")!= P_MAX_INDEX ||str.Find("100")!= P_MAX_INDEX ) ? "aaaaaa":"";
        /*if (tag!=NULL && strlen(tag)>0 ){
           pConf=tag;
        }*/
        //
          PStringArray arrtags;
        if (strlen(pConf)>0 ){
          arrtags= PString(pConf).Tokenise(";");
          m_pMgr->GetMEMgr()-> m_nCurectEnteredRoom=arrtags[1].AsInteger();
          PString str =FromBase64(arrtags[2] );
          str = ::UTF8ToPlainText(str);
          m_pMgr->GetMEMgr()-> m_nCurectEnteredRoom_alias=str;// FromBase64(arrtags[2] );
          pConf= (const char*)arrtags[0];
        }
        //

 				if (m_pMgr->GetMEMgr()->OnRecvCallers(&strURl[2], pConf/*tag*/, nDelayForCall) )
					return 0;
				else
					return m_pMgr->GetMEMgr()->GetLastErrorCode();
			}
			break;
		case 's'://sip
      {
        stCallUserData data;data.calltype= em_CallSip;data.callrepairsockettype = em_standard_udp;
        m_pMgr->MakeSipCall(&strURl[2], false ,&data);
      }
      return 0;
			break;
    case 'c':
      {
        if (m_pMgr->GetOPALMgr()->IsInCallSession(&strURl[2])) return -2;
        g_VideoLayoutForce =1;
 				if (m_pMgr->GetMEMgr()->OnRecvCallers(&strURl[2],  tag ,nDelayForCall) )
					return 0;
				else
					return m_pMgr->GetMEMgr()->GetLastErrorCode();
      }
      return 0;
			break;
		case 'a'://muit sip
      {
        //PStringArray arr = PString(&strURl[2]).Tokenise(";");
        //if 
        stCallUserData data;data.calltype= em_CallSip;data.callrepairsockettype = em_standard_udp;
        m_pMgr->MakeSipCall(&strURl[2], false, &data );
        //m_pMgr->GetOPALMgr()-> MakeSipCallForApp(&strURl[2]  );
      }
      return 0;
			break;
		case 'm'://muit sip
      {
        //PStringArray arr = PString(&strURl[2]).Tokenise(";");
        //if 
        stCallUserData data;data.calltype= em_CallSip;data.callrepairsockettype = em_standard_udp;
        m_pMgr->MakeSipCall(&strURl[2], false, &data );
        //m_pMgr->GetOPALMgr()-> MakeSipCallForApp(&strURl[2]  );
      }
      return 0;
			break;    case 'h'://h323
			break;
		default:
			break;

		};
	}
	// TODO: Add your dispatch handler code here

	return -1;
}

LONG CYYSpriteX2Ctrl::Hangup(LPCTSTR strPeer, LPCTSTR tag)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: Add your dispatch handler code here

	return 0;
}

void CYYSpriteX2Ctrl::initialise(LPCTSTR tag)
{
  m_bInit = true;
 // unsigned char out[128]={0};
 // 	int ret = PKI_encode((const unsigned char*)")I M:", 5, out);
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  m_pCUGTree = new BBQTrees();
  m_hwndLocalVideoPanel= NULL;
  m_wndCaptureDlg=0;
  PString strMainPath; strMainPath.sprintf("%s\\YY", GetAppPath()) ;
    if (PDirectory::Exists(strMainPath)==false )
      PDirectory::Create(strMainPath);

  m_pcreatefreegroupdlg= m_pguideAudioDlg=m_pmessageboxdlg = m_plistonlinedlg = m_pviewuserdlg = m_pcreatemeetdlg=m_pOptdlg =m_pmwelcomedlg= NULL;
  //int p= sizeof( BBQ_MeetingInfo );
  m_pVideoPanelDlg =NULL;
  g_InitMultLanguageString();
	m_hWndEx =NULL;	
  CreateMyWin();
	m_pMgr = new LPNCoreMgr();
	m_pMgr->InitSpriteX2Ctrl(this);
  //MessageBox("initialise begin");
  PString strtag = tag;
  if (strtag.GetLength() ==1 ){
    if (strtag=="1")
      g_VideoLayoutForce=1;

    //
	  m_pMgr->initialise("");
  }else
	  m_pMgr->initialise(tag);
 
	// TODO: Add your dispatch handler code here
}

BSTR CYYSpriteX2Ctrl::GetCameraNames(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CString strResult;
	PString strDriverName = D_DIRECTSHOW_NAME;
  //static char dev[20][256];
	//static PStringList devices;
  PStringList devices;
  devices =  PVideoInputDevice::GetDriversDeviceNames(strDriverName);
  
  int i=0;
	for ( PINDEX j = 0 ; j < devices.GetSize()&& j< 20; j++  ) {
		//szNames[j] =&devices[j][0];
    if (PString(devices[j]).Find("(VFW)")!=P_MAX_INDEX )
       continue;
    else
    {
			strResult+=(devices[j]+";");
       i++;
    }
	}
  strResult += C_SCREEN_CAPTURE_NAME;
	// TODO: Add your dispatch handler code here

	return strResult.AllocSysString();
}

LONG CYYSpriteX2Ctrl::SetCameraName(LPCTSTR strName, LPCTSTR tag)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// TODO: Add your dispatch handler code here
	if (m_pMgr->SetCamaraName(strName))
		return 0;
	else
		return 1;
}

LONG CYYSpriteX2Ctrl::IsLogin(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: Add your dispatch handler code here
	if (m_pMgr->IsLogin())
    return 1;
  else
	  return 0;
}

void CYYSpriteX2Ctrl::OnEstableshedCall(LPCTSTR strIDs, LPCTSTR tag)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	BYTE parms[] =
		VTS_BSTR VTS_BSTR;
	FireEvent(dispidOnEstableshedCall, parms,
		 strIDs, tag);

  PStringArray arr= PString(strIDs).Tokenise(";");
  if (arr.GetSize()>1 ){
    unsigned int peerID =  arr[0].AsUnsigned();
    peerID = (peerID  == m_pMgr->GetMEMgr()->GetID() ? arr[1].AsUnsigned(): peerID);
    
    MapIMDialogs::iterator ifound=  m_mapIMDlgs.find(peerID);
    if (ifound!= m_mapIMDlgs.end() && ifound->second->IsWindowVisible() ){
      ifound->second->OnNotifySLDlg(em_CallEstablish, strIDs);//em_CallEstablish send to pop im dlg
      
    }
  }

	// TODO: Add your dispatch handler code here
}

void CYYSpriteX2Ctrl::OnClearedCall(LPCTSTR strIDs, LPCTSTR reason, LPCTSTR tag)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: Add your dispatch handler code here
	BYTE parms[] =
		VTS_BSTR VTS_BSTR VTS_BSTR;
	FireEvent(dispidOnClearedCall , parms, strIDs,
		 reason, tag);

  PStringArray arr= PString(strIDs).Tokenise(";");
  if (arr.GetSize()>1 ){
    unsigned int peerID =  arr[0].AsUnsigned();
    peerID = (peerID  == m_pMgr->GetMEMgr()->GetID() ? arr[1].AsUnsigned(): peerID);
    
    MapIMDialogs::iterator ifound=  m_mapIMDlgs.find(peerID);
    if (ifound!= m_mapIMDlgs.end()  && ifound->second->GetSafeHwnd()&& ifound->second->IsWindowVisible() ){
      ifound->second->OnNotifySLDlg(em_CallClearFromPop, strIDs);//em_CallEstablish send to pop im dlg
      
    }
  }
}

void CYYSpriteX2Ctrl::Logout(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	m_pMgr->GetMEMgr()->Logout();
	// TODO: Add your dispatch handler code here
}

void CYYSpriteX2Ctrl::OnIncomingCall(LPCTSTR strHandle, LPCTSTR Caller)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
#if 0
	BYTE parms[] =
		VTS_BSTR VTS_BSTR;
	FireEvent(dispidOnIncomingCall ,parms, strIDs,
		 Caller );
#else
  CWnd *pWnd= FromHandle(m_hwndMainContainerPanel);
	CIncomingDlg* p= new CIncomingDlg(this,pWnd);
	p->m_strHandle = strHandle;
	PStringArray arr = PString(Caller).Tokenise("|");
	if (arr.GetSize()>0)
		p->m_strRemoteUserID = (const char*)arr[0];
	if (arr.GetSize()>1)
		p->m_strRemoteUserDisplayName =(const char*)UTF8ToPlainText( (const char*)arr[1]);

	p->Create(IDD_DIALOG_INCOMING);
	p->ShowWindow(SW_SHOW);
  p->SetOwner(pWnd);
	m_listIncomingDlgs.push_back(p);
#endif

}

void CYYSpriteX2Ctrl::OnOutgoing(LPCTSTR strIDs, LPCTSTR Callee)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
		BYTE parms[] =
		VTS_BSTR VTS_BSTR;
	FireEvent(dispidOnOutgoing ,parms,  strIDs,
		 Callee );
	// TODO: Add your dispatch handler code here
}

LONG CYYSpriteX2Ctrl::ShowUserPanel(LPCTSTR strUserID, LONG bShow, SHORT left, SHORT top, SHORT width, SHORT hight)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: Add your dispatch handler code here
	return m_pMgr->ShowUserPanel(strUserID, bShow , left, top, width, hight);
}
CString CYYSpriteX2Ctrl::GetFriendsListReturnCString(LPCTSTR xfriend)
{
	CString strResult="<?xml version='1.0'?><list>";
  PString strParam = xfriend;

	UserStatusList list;
	if (!m_pMgr->GetMEMgr()->GetFriendList(list, strParam =="1" ))
	{
		return CString("");
	}
	// TODO: Add your dispatch handler code here
	//for(int i=0;i< 2;i++)
	//{
	//	ONLINE_USER * p= new ONLINE_USER();
	//	p->act_status = 255;
	//	p->id = 2001+i;
	//	list.push_back(p);
	//}
	CString strValue ;
  stViewUserParam* pParam = new stViewUserParam(list.size()+1, false);
  PIntArray* data = pParam->data ;//new PIntArray(list.size());
  int j=0;
  (*data)[j++] = m_pMgr->GetMEMgr()->GetID();
	for(UserStatusList::iterator i=list.begin(); i!= list.end();i++)
	{
#ifdef _DEBUG
#else
    if ((*i)->id == D_CLIENT_MCU_ID) continue;
#endif
    (*data)[j++]=  (*i)->id;
    if (strParam == "3" ){
      // strValue.Format("<f uid=\"%u\" status=\"%u\" alias=\"%s\"></f>", (*i)->id , (*i)->act_status,(const char*)g_GetAliasName((*i)->id, false) );
      strValue.Format("<f uid=\"%u\" status=\"%u\" alias=\"%s\"></f>", (*i)->id , (*i)->act_status, ((const char*)g_GetAliasName((*i)->id, false) ));
    }else{
      strValue.Format("<f uid=\"%u\" status=\"%u\"></f>", (*i)->id , (*i)->act_status );
    }
		strResult+=strValue;
	}
	strResult+= "</list>";

   m_pMgr->GetMEMgr()->ThreadViewUserData(*pParam);
   return strResult;
}

BSTR CYYSpriteX2Ctrl::GetFriendsList(LPCTSTR xfriend)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  PString strParam = xfriend;
  CString strResult;
  if (strParam=="2" )//getcug
  {
    return GetTree(true);
  }
  strResult=GetFriendsListReturnCString(xfriend);

	return strResult.AllocSysString();
	
}

void CYYSpriteX2Ctrl::AddFriend(LPCTSTR strUserID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: Add your dispatch handler code here
	CString strUserIDe= strUserID, strRealUserID = strUserID,strText = "";
	int nPos=-1;
	if ((nPos=strUserIDe.Find("@")) >=0)
	{
		strRealUserID = strUserIDe.Left(nPos);
		strText = strUserIDe.Right(strUserIDe.GetLength() - nPos-1);
	}
	m_pMgr->GetMEMgr()->AddFriend( atoi(strRealUserID.GetBuffer()), strText.GetBuffer());
}

void CYYSpriteX2Ctrl::OnAddFriend(LPCTSTR strUserID, LONG code)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: Add your dispatch handler code here
}

BSTR CYYSpriteX2Ctrl::GetFirendListEx(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

  CString strResult;
  return strResult.AllocSysString();
}

LONG CYYSpriteX2Ctrl::GetLastErrorCode(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: Add your dispatch handler code here

	return 0;
}

void CYYSpriteX2Ctrl::OnCreatedYYScprite(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: Add your dispatch handler code here
}

void CYYSpriteX2Ctrl::OnNotifyContactActionBy(LONG byid, LONG nAction)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	BYTE parms[] =
		VTS_I4 VTS_I4;
	FireEvent(dispidOnNotifyContactActionBy, parms,
		 byid, nAction);
}
void CYYSpriteX2Ctrl::OnNotifyContactActionBy_WM(LONG byid, LONG nAction)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (m_hWndEx==NULL ) return;////MessageBox("Invalid hwnd ");
	::SendMessage(m_hWndEx, YY_OnNotifyContactActionBy_WM,byid,nAction);
}
LONG CYYSpriteX2Ctrl::ConfirmContactActionBy(LONG nUserid, LONG nAction, LONG bAccepted)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	m_pMgr->GetMEMgr()->ConfirmContactActionBy(nUserid, nAction, bAccepted !=0);
	// TODO: Add your dispatch handler code here

	return 0;
}

BSTR CYYSpriteX2Ctrl::ListOnlineUser(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

  CString strResult="";
	//pack all info from  BBQUserBriefRecord
	uint32 nStart=0,nTotal=100, nCount=100;
  unsigned char out[6000]={0};
	BBQUserBriefRecord* pRecs=NULL;
	if (!m_pMgr->GetMEMgr()->ListOnlineUser(nStart, nTotal, nCount,pRecs) || pRecs==NULL) 
		return CString("").AllocSysString();
 

 	int ret = PKI_encode((const unsigned char*)pRecs, sizeof(BBQUserBriefRecord)* nCount, out);
	if (ret> sizeof(out))
		return strResult.AllocSysString();
  else{
	  // TODO: Add your dispatch handler code here
    strResult.Format("%d;%d;%d;%d;%s", nStart, nTotal, nCount,sizeof(BBQUserBriefRecord), (const char*)out); 
  }
  delete[] pRecs;pRecs=0;
	return strResult.AllocSysString();
}
BSTR CYYSpriteX2Ctrl::GetBlockList(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CString strResult="<?xml version='1.0'?><list>";

	UserIdList list;
	if (!m_pMgr->GetMEMgr()->GetBlockList(list, true))
	{
		return CString("").AllocSysString();
	}
	// TODO: Add your dispatch handler code here
	//for(int i=0;i< 10;i++)
	//{
	//	ONLINE_USER * p= new ONLINE_USER();
	//	p->act_status = 0;
	//	p->id = 2010+i;
	//	list.push_back(p);
	//}
	CString strValue ;
	for(int i=0; i < list.Count();i++)
	{
		strValue.Format("<f  >%u</f>",   list.Data()[i] );
		strResult+=strValue;
	}
	strResult+= "</list>";
	// TODO: Add your dispatch handler code here

	return strResult.AllocSysString();
}
bool CYYSpriteX2Ctrl::GetTreeSubTreeIDs( unsigned nGroupID ,uint_list &list )
{
   uint_list * pList =m_pCUGTree-> FindNode( nGroupID );
   if (pList)
    list = *pList;
  // m_pCUGTree->PrintXML(str,m_uidbyViewIntree, true);
  
  return true;
}

CString CYYSpriteX2Ctrl::GetTreeReturnCString(bool bForceUpdate )
{
  std::map<uint32,uint32>  m_uidbyViewIntree;
	CString strResult;
  if (bForceUpdate ){
    m_pCUGTree->RemoveAll();
	  BBQTrees& tree = *m_pCUGTree;
	  if (!m_pMgr->GetMEMgr()->GetIdTrees(tree))
	  {
		  return CString("") ;
	  }
    //if (tree.m_listRootValues.size() ==1 /* &&std::find( tree.m_listRootValues.begin(), tree.m_listRootValues.end(), m_pMgr->GetMEMgr()->GetID()) !=  tree.m_listRootValues.end()*/)
    //   return CString("") ;
    //(*data)[0]=  uidadd;
	  PString str;
    //if ( bContainAliasName ){
    //}else{
      tree.PrintXML(str,m_uidbyViewIntree, true);
    //}
    strResult =  m_strTreeUsers= (const char*)str;
    stViewUserParam* pParam = new stViewUserParam(m_uidbyViewIntree.size(), false, 2000);
    PIntArray* data = pParam->data ;//new PIntArray(list.size());
    int j=0;
    for(std::map<uint32,uint32>::iterator i= m_uidbyViewIntree.begin();i!=m_uidbyViewIntree.end();i++ ){
      (*data)[j++]=  i->first;
    }
	  m_pMgr->GetMEMgr()->ThreadViewUserData(*pParam);
  }else{
    strResult= m_strTreeUsers;
  }

  return strResult;
}

BSTR CYYSpriteX2Ctrl::GetTree(bool bForceUpdate )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  CString strResult=GetTreeReturnCString(bForceUpdate);
//strResult =  "<?xml version=\"1.0\" encoding=\"UTF-8\"?><entry uid=\"100003\">  <entry uid=\"2001\"></entry>  <entry uid=\"2002\"></entry><entry uid=\"2003\"></entry>	<entry uid=\"100004\">  		<entry uid=\"2001\"></entry>  		<entry uid=\"2002\"></entry><entry uid=\"2003\"></entry>  		<entry uid=\"2004\"></entry><entry uid=\"2005\"></entry>	</entry>  <entry uid=\"2004\"></entry><entry uid=\"2005\"></entry>  <entry uid=\"2006\"></entry><entry uid=\"2007\"></entry>  <entry uid=\"2008\"></entry><entry uid=\"2009\"></entry>  <entry uid=\"2010\"></entry><entry uid=\"100004\">  <entry uid=\"2001\"></entry>  <entry uid=\"2002\"></entry><entry uid=\"2003\"></entry>  <entry uid=\"2004\"></entry><entry uid=\"2005\"></entry></entry></entry>";
	// TODO: Add your dispatch handler code here
	//for(int i=0;i< 10;i++)
	//{
	//	ONLINE_USER * p= new ONLINE_USER();
	//	p->act_status = 0;
	//	p->id = 2010+i;
	//	list.push_back(p);
	//}
	/*CString strValue ;
	for(UserStatusList::iterator i=list.begin(); i!= list.end();i++)
	{
		strValue.Format("<f status=\"%u\">%u</f>", (*i)->act_status, (*i)->id );
		strResult+=strValue;
	}
	strResult+= "</list>";*/
	// TODO: Add your dispatch handler code here

	// TODO: Add your dispatch handler code here

	return strResult.AllocSysString();
}

LONG CYYSpriteX2Ctrl::Hungup(LPCTSTR strUserID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: Add your dispatch handler code here
  m_pMgr->GetOPALMgr()->HangupCallSafe(PString(strUserID) );
	return  0;
}

LONG CYYSpriteX2Ctrl::HangupEx(LPCTSTR strUserURL)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: Add your dispatch handler code here
  m_pMgr->GetOPALMgr()->HangupCallSafe(PString(strUserURL)  );

	return 0;
}

LONG CYYSpriteX2Ctrl::AddFriendEx(LONG uid, LPCTSTR lpszText)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: Add your dispatch handler code here
	m_pMgr->GetMEMgr()->AddFriend( uid, lpszText);

	return 0;
}

void CYYSpriteX2Ctrl::OnBuddyStatusChanged(LPCTSTR strInfoValue)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	//VFONID					peerId;							// 8
	//uint32					onlineStatus;					// 4
	//char					alias[ USERDATA_NAME_SIZE ];	// 32
	//char					bindId[ USERDATA_ADDRESS_SIZE ];// 64
		BYTE parms[] =
		VTS_BSTR;
	FireEvent(dispidOnBuddyStatusChanged ,parms, strInfoValue
		  );
	// TODO: Add your dispatch handler code here
}


void CYYSpriteX2Ctrl::OnHeartbeatFeedback(LPCTSTR status)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
		BYTE parms[] =
		VTS_BSTR;
	FireEvent(dispidOnHeartbeatFeedback ,parms, status
		  );

	// TODO: Add your dispatch handler code here
}

void CYYSpriteX2Ctrl::OnHeartbeatTimeout(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	BYTE parms[] ={0}
		;
	FireEvent(dispidOnHeartbeatTimeout ,parms
		  );

	// TODO: Add your dispatch handler code here
}

void CYYSpriteX2Ctrl::OnSessionExpired(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	BYTE parms[] ={0}
		;
	FireEvent(dispidOnSessionExpired ,parms
		  );

	// TODO: Add your dispatch handler code here
}

void CYYSpriteX2Ctrl::OnLoginReplaced(LPCTSTR addr)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
		BYTE parms[] =
		VTS_BSTR;
	FireEvent(dispidOnLoginReplaced ,parms, addr
		  );

	// TODO: Add your dispatch handler code here
}
//HWND   Pop_FindWindowYY(const char * strFind)   
//{
//  char   strTitle[256]; 
//  HWND   hwnd=NULL; 
//  HWND   AfterHwnd   =   NULL; 
//  while(true) 
//  {
//    hwnd=::FindWindowEx(NULL,AfterHwnd,NULL/*(LPCSTR) "#32770 "*/,NULL); 
//    if(!hwnd) 
//      return NULL;
//    else 
//    { 
//      if(::GetWindowText(hwnd,strTitle,256)) 
//                      if(strstr(strTitle,strFind)!=0) 
//      { 
//                          //找到窗口后的操作 
//        return hwnd; 
//      } 
//    } 
//    AfterHwnd   =   hwnd; 
//  } 
//return NULL;
//} 

//HWND   Pop_FindWindowYYByUserID(uint32 uid)
//{
//  PString strWinTitle ;strWinTitle.sprintf("(%u)", uid );
//  return Pop_FindWindowYY( (const char*)strWinTitle );      //进程B的接收数据窗口对象
//}

LONG CYYSpriteX2Ctrl::OnNotifyOfflineIM(LONG index, LONG total, LPCTSTR msg)
{
  //execute main thread, do not worry
  const char * pfrom_vfonid="from_vfonid=\"";
  const char * pto_vfonid="to_vfonid=\"";
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  PString strMsg = msg;
  /*if (strMsg.Find("conf_id=@mcc") == P_MAX_INDEX)*/{
    PINDEX nPos1= strMsg.Find( pfrom_vfonid) +  strlen(pfrom_vfonid);
    PINDEX nPos2= strMsg.Find( "\" ", nPos1);
    PString strfromvfonid = strMsg.Mid(nPos1 ,nPos2-nPos1);
    uint32  uid =strfromvfonid.AsUnsigned();
    //to
    nPos1= strMsg.Find( pto_vfonid) +  strlen(pto_vfonid);
    nPos2= strMsg.Find( "\" ", nPos1);
    PString strtovfonid = strMsg.Mid(nPos1 ,nPos2-nPos1);
    uint32  uidTo =strtovfonid.AsUnsigned();
    if (uidTo < D_FREEGROUP_ID_UPPERLIMIT/*D_ROOM_ID_UPPERLIMIT*/) uid=uidTo;
    MapIMDialogs::iterator ifound = m_mapIMDlgs.find(uid);
    if (ifound ==  m_mapIMDlgs.end() ){
      //do create 
      BBQUserInfo userinfo={0};
      m_pMgr->GetMEMgr()->ViewUserData(uid, userinfo);
      PString param; param.sprintf("1;%u;%s", uid, (const char*)UTF8ToPlainText( userinfo.basic.alias));
      IMDlgControl(param);

    }else
    {
      if ( uid > D_ROOM_ID_UPPERLIMIT ){ //free group and normal user do 
        ifound->second->ShowWindow(SW_SHOW);
        ifound->second->FlashWindow(TRUE);
      }
 
    }
    ifound = m_mapIMDlgs.find(uid);
    if (ifound !=  m_mapIMDlgs.end() && ifound->second->m_emStyle== em_popIM ){
     // while ( !ifound->second->IsWindowVisible() )
     //   Sleep(10);

     return ifound->second->DLGNotifyOfflineIM(index, total, msg);
       
    }else {
     // 请注意 千万不能点用 ptrace函数在主线程中
      //PTRACE(3, "NotifyOfflineIM\t failed notify IM to " << uid); 
      return 0;
    }
      ///test msg between processs
      //PString strWinTitle ;strWinTitle.sprintf("(%s)", (const char*)strvfonid );
     // HWND hWndReceived = Pop_FindWindowYY( (const char*)strWinTitle );      //进程B的接收数据窗口对象
     // if (hWndReceived == NULL){
     //   uint32 uid = strvfonid.AsUnsigned();
     //   BBQUserInfo userinfo={0};
     //   m_pMgr->GetMEMgr()->ViewUserData(uid, userinfo);
     //   if(Pop_CreateProcess(uid, UTF8ToPlainText( userinfo.basic.alias)))
     //   {
     //     Sleep(1500);
     //      hWndReceived = Pop_FindWindowYYByUserID(uid);
     //   }
     // }
     //// if (hWndReceived) MessageBox("found a win");
     // //COPYDATASTRUCT结构是WM_COPYDATA传递的数据结构对象
     // COPYDATASTRUCT cpd;
     // cpd.dwData =  1 /*em_IMMessage_received*/;
     // PString strMsg1 ;strMsg1.sprintf("%d;%d;%s", index, total, msg );
     // //MessageBox(strMsg1);
     // cpd.cbData  =   strMsg1.GetLength();            //传递的数据长度
     // cpd.lpData    =  (void*) (const char*)strMsg1;  //传递的数据地址
     // ::SendMessage( hWndReceived, WM_COPYDATA, 0, (LPARAM) & cpd );
  }/*else{

		BYTE parms[] =
		VTS_I4 VTS_I4 VTS_BSTR;
	FireEvent(dispidOnNotifyOfflineIM ,parms, index, total, msg
		  );
  }*/
	// TODO: Add your dispatch handler code here
}
//void CYYSpriteX2Ctrl::OnNotifyContactActionBy_WM(LONG byid, LONG nAction){
//	if (m_hWndEx==NULL ) return;////MessageBox("Invalid hwnd ");
//	::SendMessage(m_hWndEx, YY_OnNotifyContactActionBy_WM,byid,nAction);
//}
void CYYSpriteX2Ctrl::OnBuddyStatusChanged_WM(LPCTSTR strInfoValue)
{
	if (m_hWndEx==NULL ) return;////MessageBox("Invalid hwnd ");
	::SendMessage(m_hWndEx, YY_OnBuddyStatusChanged_WM,(WPARAM) strInfoValue ,0);
}
void CYYSpriteX2Ctrl::OnHeartbeatFeedback_WM(LPCTSTR status)
{
	if (m_hWndEx==NULL ) return;////MessageBox("Invalid hwnd ");
  PString *p=new PString();
  *p = status;
  ::PostMessage(m_hWndEx, YY_OnHeartbeatFeedback_WM,(WPARAM) p,0);
}
void CYYSpriteX2Ctrl::OnHeartbeatTimeout_WM(void)
{
	if (m_hWndEx==NULL ) return;////MessageBox("Invalid hwnd ");
	::SendMessage(m_hWndEx, YY_OnHeartbeatTimeout_WM,0,0);
}
void CYYSpriteX2Ctrl::OnSessionExpired_WM(void)
{
	if (m_hWndEx==NULL ) return;////MessageBox("Invalid hwnd ");
	::SendMessage(m_hWndEx, YY_OnSessionExpired_WM,0,0);
}
void CYYSpriteX2Ctrl::OnLoginReplaced_WM(LPCTSTR addr)
{
	if (m_hWndEx==NULL ) return;////MessageBox("Invalid hwnd ");
	::SendMessage(m_hWndEx, YY_OnLoginReplaced_WM,(WPARAM)addr,0);
}
void CYYSpriteX2Ctrl::OnNotifyOfflineIM_WM(LONG index, LONG total, LPCTSTR msg)
{
	if (m_hWndEx==NULL ) return  ;////MessageBox("Invalid hwnd ");
	LONG v =0;
	v = index &0xff | (total & 0xff << 16);
  LRESULT ret  =0;
  int nTrying=300;
  do{
      ret =  ::SendMessage(m_hWndEx, YY_OnNotifyOfflineIM_WM,v,(LPARAM)msg);
      if (ret == 200) {
        PTRACE(5, "Notify offline msg OK " << msg);
        return ;
      }
      else
         Sleep(100);
  }while  (ret!= 200&& nTrying-->0 );
  return  ;

}
bool CYYSpriteX2Ctrl::OnNotifyOfflineIM_Prepared( unsigned int uid)
{
     MapIMDialogs::iterator ifound = m_mapIMDlgs.find(uid);
      return  (ifound !=  m_mapIMDlgs.end() &&ifound->second->GetSafeHwnd()&& ifound->second->m_bLoadedXAML && ::IsWindowVisible( ifound->second->GetSafeHwnd()) );

}
void CYYSpriteX2Ctrl::OnEstableshedCall_WM(LPCTSTR strIDs, LPCTSTR tag)
{
	if (m_hWndEx==NULL ) return;////MessageBox("Invalid hwnd ");
	::SendMessage(m_hWndEx, YY_OnEstableshedCall_WM,(WPARAM)strIDs,(LPARAM)tag);
}
void CYYSpriteX2Ctrl::OnClearedCall_WM(LPCTSTR strIDs, LPCTSTR reason, LPCTSTR tag)
{
	if (m_hWndEx==NULL ) return;
	::SendMessage(m_hWndEx, YY_OnClearedCall_WM,(WPARAM)strIDs,(LPARAM)reason);
}
void CYYSpriteX2Ctrl::OnIncomingCall_WM(LPCTSTR strIDs, LPCTSTR Caller,   LPCTSTR handle)
{
	if (m_hWndEx==NULL ) return;////MessageBox("Invalid hwnd ");
	::SendMessage(m_hWndEx, YY_OnIncomingCall_WM,(WPARAM)handle,(LPARAM)Caller);
}
void CYYSpriteX2Ctrl::OnOutgoing_WM(LPCTSTR strIDs, LPCTSTR Callee)
{
	if (m_hWndEx==NULL ) return;////MessageBox("Invalid hwnd ");
	::SendMessage(m_hWndEx, YY_OnOutgoing_WM,(WPARAM)strIDs,(LPARAM)Callee);
}
void CYYSpriteX2Ctrl::OnCreatedYYScprite_WM(void)
{
	if (m_hWndEx==NULL ) return;////MessageBox("Invalid hwnd ");
	::SendMessage(m_hWndEx, YY_OnCreatedYYScprite_WM,0,0);
}
void CYYSpriteX2Ctrl::OnAddFriend_WM(LPCTSTR strUserID, LONG code)
{
	if (m_hWndEx==NULL ) return;////MessageBox("Invalid hwnd ");
	::SendMessage(m_hWndEx, YY_OnAddFriend_WM,(WPARAM)strUserID,(LPARAM)code);
}
void CYYSpriteX2Ctrl::OnMsgDownloadFile_WM(LPCTSTR strurl, LONG page)
{
	if (m_hWndEx==NULL ) return;////MessageBox("Invalid hwnd ");
	::SendMessage(m_hWndEx, YY_ONMSGDOWNLOADFILE_WM,(WPARAM)strurl,(LPARAM)page);
}
void CYYSpriteX2Ctrl::OnMsgSendFileProcess_WM(LPCTSTR strparam )
{
	if (m_hWndEx==NULL ) return;////MessageBox("Invalid hwnd ");
	::SendMessage(m_hWndEx, YY_ONMSGSENDFILEMESSAGE_WM,(WPARAM)strparam,0 );
}

BSTR CYYSpriteX2Ctrl::Subscribe(LPCTSTR strValue)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CString strResult;
//
//	const char * pAlias[7] = {"赵飞燕","苏小小","西门","美作","道明寺", "花泽类", "杉菜"};
//	int nIndex = time(NULL) % 7;
//	PString aliasUTF8 =  PlainTextToUTF8(pAlias[nIndex]);
//	const char* alias= (const char*)aliasUTF8;
//	const char * pwd = "123";
//	PString strHwId = "GetHardwareToken()";
//
//	SIMD_CS_SUBSCRIBE in;
//	SIMD_SC_SUBSCRIBE out;
//	memset( & in, 0, sizeof(in) );
//	memset( & out, 0, sizeof(out) );
//	
//	m_pMgr->GetMEMgr()->SetServerAddress(m_pMgr->GetOPALMgr()->m_strVfonDomain, 5101, false );
//
////	memcpy( in.hardwareId, (const char *) strHwId, USERDATA_NAME_SIZE ); in.hardwareId[ USERDATA_NAME_SIZE-1 ] = '\0';
//	strncpy( in.userInfo.basic.alias, (const char*)alias, sizeof(in.userInfo.basic.alias)  -1);
//	MD5Encryption( in.userInfo.security.newpass, pwd );
//	MD5Encryption( in.userInfo.security.oldpass, pwd );
//
//	if(m_pMgr->GetMEMgr()->Subscribe( & in, & out ) ) {
//		PString strId( PString::Printf, "%u", out.vfonId.userid );
//		strResult= (const char*)(strId+":123");
//		//Status( PString(PString::Printf, "Subscription done, Id: %u\r\n", out.vfonId.userid) ); 
//	} else {
//		///Status( "Subscription failed." );
//		strResult.Format("error=%d", m_pMgr->GetMEMgr()->GetLastErrorCode());
//	}
//		// TODO: Add your dispatch handler code here

	return strResult.AllocSysString();
}

LONG CYYSpriteX2Ctrl::SendOfflineIM(LPCTSTR strMsg)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: Add your dispatch handler code here
	//	char				conf_id[37];		// Conference ID
	//char				sentence_id[37];	// Sentence ID
	//uint64				from_vfonid;		// From
	//uint64				to_vfonid;			// To
	//uint64				send_time;			// timestamp
	//int					length;				// Sentence length
	//char				data[1];			// Sentence content
#if 0
	if (strMsg)
	{
    //MessageBox(" CYYSpriteX2Ctrl::SendOfflineIM(LPCTSTR str");
		PString str = strMsg;
		PStringArray arr= str.Tokenise(";");
		if (arr.GetSize()>=7)
		{
      unsigned int nOragnalIMLen =arr[5].AsUnsigned();
      bool bNeedCompressed= false;
      const char* pIMBody =NULL;
      int nImLen =0;
      char *compr=NULL;
      if (nOragnalIMLen>1024){
        //compress it 
		    compr =new char[nOragnalIMLen+1024];
		    unsigned int comprLen= nOragnalIMLen+1024;
        if (0== BZ2_bzBuffToBuffCompress(compr, &comprLen, ( char*)((const char*)arr[6]) , nOragnalIMLen, 9,0,0)){
          bNeedCompressed = true;
          nImLen = comprLen;
          pIMBody = compr;
        }
			  //if (BZ2_bzBuffToBuffDecompressEx(btUncompr , &uncomprLen ,((char*) reply->msg.simData)+ nHeaderLen, comprLen , 0, 0)== 0)
      }else{
        pIMBody =(const char*)arr[6];nImLen = nOragnalIMLen;
      }
			BBQOfflineIMMessage * pIm = BBQOfflineIMMessageNEW( nImLen/*arr[5].AsUnsigned() */);
			BBQOfflineIMMessage& im = *pIm;
			strncpy(im.conf_id, (const char*)arr[0], sizeof(im.conf_id));
			strncpy(im.sentence_id, (const char*)arr[1], sizeof(im.sentence_id));
			uint32 im_from_id = im.from_vfonid = arr[2].AsUnsigned();
			im.to_vfonid = arr[3].AsUnsigned();
			im.send_time =arr[4].AsUnsigned() ;
      im.compressed =bNeedCompressed? 1:0;
			im.length = nImLen+1;//arr[5].AsUnsigned();
			//im.data[] = new char[im.length +1];
			//memset(&im.data[0], 0, im.length +1 );
			memcpy(&im.data[0],pIMBody /*(const char*)arr[6]*/, im.length);
			im.data[im.length-1]=0;
			m_pMgr->GetMEMgr()->SendOfflineIM(&im);
 
			BBQOfflineIMMessageDELETE(pIm);
      delete[] compr;
      compr=NULL;
		}
	}
#else
  PString* pMsg =new PString();
  *pMsg = strMsg;
  stThreadCommand * pCmd = new stThreadCommand();
  pCmd->cmd = em_threadSendIM;
  pCmd->tag = pMsg;
  
  m_pMgr->GetMEMgr()->PostPTThreadCommand(pCmd);
#endif
	return 0;
}

BSTR CYYSpriteX2Ctrl::ViewUserData(LPCTSTR uid_b)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CString strResult;
#if 0
	BBQUserInfo user={0};
	unsigned char out[1400]={0};
	int uid = atol(uid_b);
	if (m_pMgr->GetMEMgr()->ViewUserData(uid, user, true))
	{
    if (m_pMgr->GetOPALMgr()->GetClientID() == uid)
			m_pMgr->GetOPALMgr()->SetDisplayName(user.basic.alias );

		//PString strAliasPaintext = UTF8ToPlainText(user.basic.alias);
		//strncpy(user.basic.alias, (const char*) strAliasPaintext, sizeof(user.basic.alias));
		int ret = PKI_encode((const unsigned char*)&user, sizeof(BBQUserInfo), out);
		if (ret> sizeof(out))
			return strResult.AllocSysString();
	}
	// TODO: Add your dispatch handler code here
	strResult =out;
#else 
  PStringArray arr= PString(uid_b).Tokenise(":");
  stViewUserParam *pParam = new stViewUserParam(arr.GetSize(), true);
  PIntArray* data= pParam->data;//new PIntArray(arr.GetSize());
  for(int i=0;i< data->GetSize();i++)
    (*data)[i] = arr[i].AsUnsigned();

  m_pMgr->GetMEMgr()->ThreadViewUserData(*pParam);
  //delete[] data; delete in other thread who used
#endif
	return strResult.AllocSysString();
}

CString CYYSpriteX2Ctrl::ViewUserDataDirect(LPCTSTR uid_b)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CString strResult;
#if 1
	BBQUserInfo user={0};
	unsigned char out[1400]={0};
	int uid = atol(uid_b);
	if (m_pMgr->GetMEMgr()->ViewUserData(uid, user, true))
	{
    if (m_pMgr->GetOPALMgr()->GetClientID() == uid)
			m_pMgr->GetOPALMgr()->SetDisplayName(user.basic.alias );

		//PString strAliasPaintext = UTF8ToPlainText(user.basic.alias);
		//strncpy(user.basic.alias, (const char*) strAliasPaintext, sizeof(user.basic.alias));
		int ret = PKI_encode((const unsigned char*)&user, sizeof(BBQUserInfo), out);
		if (ret> sizeof(out))
			return strResult;
	}
	// TODO: Add your dispatch handler code here
	strResult =out;
  PString strNotify;strNotify.sprintf("%d;%d;%s",uid,em_SubcmdViewUserInfo, (const char*) out);
  OnNotify_WM(em_ViewUserInfo, strNotify);

#else 
  PStringArray arr= PString(uid_b).Tokenise(":");
  PIntArray* data= new PIntArray(arr.GetSize());
  for(int i=0;i< data->GetSize();i++)
    (*data)[i] = arr[i].AsUnsigned();

  m_pMgr->GetMEMgr()->ThreadViewUserData(*data);
  //delete[] data; delete in other thread who used
#endif
	return strResult ;
}

LONG CYYSpriteX2Ctrl::RemoveBlock(LONG uid)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: Add your dispatch handler code here
	m_pMgr->GetMEMgr()->RemoveBlock(uid);
	return 0;
}

LONG CYYSpriteX2Ctrl::RemoveFriend(LONG uid)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: Add your dispatch handler code here
	m_pMgr->GetMEMgr()->RemoveFriend(uid);

	return 0;
}

LONG CYYSpriteX2Ctrl::UpdateUserData(LPCTSTR strUserinfo)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  LONG result =0;
	// TODO: Add your dispatch handler code here
	BBQUserInfo userinfo={0};

	unsigned char out[1400]={0};
	//m_pMgr->GetMEMgr()->ViewUserData(atol(uid_b), user, true);
	int ret = PKI_decode((const unsigned char*)strUserinfo, strlen(strUserinfo), out);

	if (ret> sizeof(userinfo))
	{
		MessageBox("UpdateUserData error for decode.");
		return -1;
	}
	
	memcpy(&userinfo, out, ret);
	//PString strAliasUTF8 = PlainTextToUTF8(userinfo.basic.alias);
	//strncpy(userinfo.basic.alias, (const char*)strAliasUTF8, sizeof(userinfo.basic.alias)-1);
	m_pMgr->GetOPALMgr()->SetDisplayName(userinfo.basic.alias);
  //check if change PWS
  //if ( strlen(userinfo.security.oldpass)>0 && strlen(userinfo.security.newpass)>0 )
  //{
  //  char tmp[128]={0};
  //  MD5Encryption(  tmp,   userinfo.security.oldpass);
  //  memcpy(userinfo.security.oldpass, tmp, sizeof(userinfo.security.oldpass));
  //  memset(tmp,0, sizeof(tmp));
  //  MD5Encryption(  tmp,   userinfo.security.newpass);
  //  memcpy(userinfo.security.newpass, tmp, sizeof(userinfo.security.newpass));
  //  userinfo.security.modify= 1;
  //  //MessageBox( userinfo.security.newpass);
  //}
	PString strAlias;
	if (!m_pMgr->GetMEMgr()->UpdateUserData(userinfo, strAlias))
  {
    result = -2;
  }
  m_pMgr->GetSettingXMLMgr()->SaveLoginInfomation(m_pMgr->GetMEMgr()->GetID(), "",0,true, true, userinfo.basic.image, true);
  ViewUserData((const char*) PString(m_pMgr->GetMEMgr()->GetID()));
	return 0;
}



void CYYSpriteX2Ctrl::OnMsgDownloadFile(LPCTSTR strurl, LONG whichpage)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
		BYTE parms[] =
		VTS_BSTR VTS_I4 ;
	FireEvent(dispidOnMsgDownloadFile ,parms, strurl, whichpage
		  );
}

void CYYSpriteX2Ctrl::OnMsgSendFileMessage(LPCTSTR filesendtimepercent)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
		BYTE parms[] =
		VTS_BSTR;
		FireEvent(dispidOnMsgSendFileMessage ,parms, filesendtimepercent
		  );

	// TODO: Add your dispatch handler code here
}

LONG CYYSpriteX2Ctrl::PPT(LPCTSTR strUser)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CFileDialog dlg( TRUE );
	if( IDOK == dlg.DoModal() )
	{
		m_pMgr->GetMEMgr()->SendFileToHttp(dlg.GetPathName().GetBuffer(), PString(strUser).AsInteger());
	}
	// TODO: Add your dispatch handler code here

	return 0;
}

void CYYSpriteX2Ctrl::OnP2PFile(LPCTSTR value)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
		BYTE parms[] =
		VTS_BSTR ;
	FireEvent(dispidOnP2PFile ,parms, value 
		  );
	// TODO: Add your dispatch handler code here
}
void CYYSpriteX2Ctrl::OnP2PFile_WM(LPCTSTR value)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (m_hWndEx==NULL ) return;////MessageBox("Invalid hwnd ");
	::SendMessage(m_hWndEx, YY_OnP2PFile_WM, (WPARAM)value,0);
}

LONG CYYSpriteX2Ctrl::AcceptP2PFile(LPCTSTR value )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: Add your dispatch handler code here
	CFileDialog dlg( FALSE );
	if( IDOK == dlg.DoModal() )
	{
		m_pMgr->GetMEMgr()->P2PFileAccpet(PString(value), PString(dlg.GetPathName().GetBuffer() ) );		
	}

	return 0;
}
extern char* GetAppPath();
extern  PString  GetUserImgPath(unsigned int uid);
extern PString  GetUserShareDocPath( );
extern PString  GetUserRecordPath(unsigned int uid);
//  PString  GetUserImgHttpURL(unsigned int uid, const char* filename)
//{
//  PString str;
//  str.sprintf("%s\\YY/%u", GetAppPath(), uid);
//  if (PDirectory::Exists(str)==false )
//    PDirectory::Create(str);
//  str+="\\pic";
//  if (PDirectory::Exists(str)==false )
//    PDirectory::Create(str);
//
//  return str;
//}
BSTR  CYYSpriteX2Ctrl::P2PAFile(LPCTSTR uid_type)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  CString strResult="";
  PStringArray params= PString(uid_type).Tokenise(";");
  if (params.GetSize() < 2) return strResult.AllocSysString();
  unsigned int uid = params[0].AsUnsigned();
  unsigned int cmd = params[1].AsUnsigned();
  if (cmd == em_TransmitterService_inerceptpic){//p2p capture a pic to send peer
    //int subcmd=params[2].AsUnsigned();
    //if (subcmd==0)
    {
      CCatchScreenDlg dlg(m_pMgr);
      PString dstFileNameAndext =  PGloballyUniqueID().AsString()+ PString(".jpg") ;
      PString dstFile;
      dstFile.sprintf("%s\\%s ", (const char*)GetUserImgPath(m_pMgr->GetMEMgr()->GetID()),(const char*)dstFileNameAndext);
      dlg.SetFilePath(dstFile);
      dlg.DoModal();
      if (dlg.m_bDoneForCaptureScreen && PFile::Exists(dstFile )){
        //CString strFilePath = dlg.GetFilePath();
        PString str;str.sprintf("main/%u/pic/%s",m_pMgr->GetMEMgr()->GetID(),  (const char*)dstFileNameAndext );
       // if (g_AddHTTPFileResource(str, dstFile))
        {
          strResult.Format( (const char*)(GetHttpListenMainURL()+"%s"), (const char*)str);
        }
      }
     } 
  }
  else if (cmd == em_TransmitterService_sendpic){//p2p a file pictue
    int subcmd=params[2].AsUnsigned();
    if (subcmd==0)
    {
      //only get get a picture url ,and display it in IM
	    CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT|OFN_ALLOWMULTISELECT,  "jpg Files (*.jpg)|*.jpg|png Files (*.png)|*.png|All Files (*.*)|*.*||"   ,  NULL, 0); 
      if( IDOK == dlg.DoModal() && PFile::Exists( dlg.GetPathName().GetBuffer() ) )
	    {
        PString dstFile = "";// +PString("\\")+ PGloballyUniqueID().AsString() + dlg.GetFileExt().get;
        PString dstFileNameAndext =  PGloballyUniqueID().AsString()+ PString(".")+  dlg.GetFileExt().GetBuffer();
        dstFile.sprintf("%s\\%s ", (const char*)GetUserImgPath(m_pMgr->GetMEMgr()->GetID()),(const char*)dstFileNameAndext);
        if (PFile::Copy(dlg.GetPathName().GetBuffer(), dstFile, PTrue) == false){//take the pic into sender's pic dir
          return strResult.AllocSysString();
        }
        PString str;str.sprintf("main/%u/pic/%s",m_pMgr->GetMEMgr()->GetID(),  (const char*)dstFileNameAndext );
        //if (g_AddHTTPFileResource(str, dstFile))
        {
          strResult.Format( (const char*)(GetHttpListenMainURL()+"%s"), (const char*)str);
        }
        ///prepare send the file data to peer
        //wait to send im by user
        ///
	     }    
      }
     else if(subcmd==1){
      //real send a pic to peer
      PString strURL = params[3];
      PINDEX npos= strURL.FindLast("/");
      if (npos!= P_MAX_INDEX)
      {
        strURL = strURL.Mid(npos+1);
        PString dstFile = GetUserImgPath(m_pMgr->GetMEMgr()->GetID()) +"\\"+  strURL;
        PFile f;
        if (f.Open(dstFile, PFile::ReadOnly))
        { 
          PString fname =  f.GetFilePath().GetFileName();
          stDataTransmitter* pData = new stDataTransmitter();
          pData->contentservice = em_TransmitterService_sendpic;
          pData->bIssender=true;
          pData->receiver= uid;
		  //
	/*	  PStringArray  subparams= params[0].Tokenise(",");
		  if (subparams.GetSize()>=2)
			  pData->nRequestChanelID = subparams[1].AsUnsigned();
		  else*/
			  pData->nRequestChanelID =pData->receiver;
          pData->sender= m_pMgr->GetOPALMgr()->GetClientID();
          strncpy(pData->strFileName, (const char*) strURL, sizeof(pData->strFileName)-1);
          strResult = pData->strFileName;
          f.Close();

          if (uid > D_FREEGROUP_ID_UPPERLIMIT){
            //picture to peer
            m_pMgr->GetTransmiterMgr()->AddSession(*pData, true);
          }else if (uid > D_ROOM_ID_UPPERLIMIT && uid< D_FREEGROUP_ID_UPPERLIMIT){
            //picture to freegroup
            uint_list list;
            pData->nRoomID =  uid;
            if (  GetTreeSubTreeIDs(uid, list)){
              for(uint_list::iterator i = list.begin();i!= list.end();i++){
                if (*i != m_pMgr->GetMEMgr()->GetID() ){
                  stDataTransmitter* pMember= new stDataTransmitter();
                   
                  *pMember = *pData;
                  pMember->receiver= *i ;
                  m_pMgr->GetTransmiterMgr()->AddSession(*pMember, true);               
                }
              }
            }
            delete pData;pData=NULL;
          }
          else{
            //  picture send room members
            std::map<unsigned ,unsigned > members;
            pData->nRoomID =  uid;
            if (m_pMgr->GetMEMgr()->GetRoomMembers(uid, members) ){
              for (std::map<unsigned ,unsigned >::iterator i=  members.begin();i!= members.end();i++){
                stDataTransmitter* pMember= new stDataTransmitter();
                   
                  *pMember = *pData;
                  pMember->receiver= i->first;
                  m_pMgr->GetTransmiterMgr()->AddSession(*pMember, true);

                 
              }
            }
            delete pData;pData=NULL;
          }
      }
     
    }

  }
  }
  else if (cmd == em_TransmitterService_sendfile){//p2p a file to peer,1 --->send request, 2---> accpet 
       int subcmd=params[2].AsUnsigned();
       if (subcmd == 0){
	        CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,  "Files (*.*)|*.*|All Files (*.*)|*.*||"   ,  NULL, 0); 

          if( IDOK == dlg.DoModal() ){
            if  (PFile::Exists( dlg.GetPathName().GetBuffer() )) {
               strResult.Format("%s;%s",(const char*) PGloballyUniqueID().AsString(), (const char*)dlg.GetPathName());
               //strResult = dlg.GetPathName().GetBuffer();
            }
              else 
                 MessageBox("Incorrect  file.");
            }else{
            

            }
       }else if(subcmd==1){
        //real send a file to peer
        //PString strURL = params[3];
       // PINDEX npos= strURL.FindLast("/");
        if (true)
        {
          //strURL = strURL.Mid(npos+1);
          PString& strAFile =   params[3];//source dir for preparing send
          PString& strBFile =   params[4];//保存的新文件 
          PString& strKey =    params[5];//保存的新文件 
         PFile f;
          if (f.Open(strAFile, PFile::ReadOnly))
          { 
            PString fname =  f.GetFilePath().GetFileName();
            stDataTransmitter* pData = new stDataTransmitter();
            pData->contentservice = em_TransmitterService_sendfile;
            pData->bIssender=true;
            pData->receiver=pData->nRequestChanelID = uid;
            pData->sender= m_pMgr->GetOPALMgr()->GetClientID();
            strncpy(pData->strOragnalFileName, (const char*) strAFile, sizeof(pData->strOragnalFileName)-1);
            strncpy(pData->strFileName, (const char*) strBFile, sizeof(pData->strFileName)-1);
            strncpy(pData->GUID, (const char*) strKey, strKey.GetLength() );
            strResult = pData->strFileName;
            f.Close();
			 if (uid > D_FREEGROUP_ID_UPPERLIMIT){
				//file to peer
				 m_pMgr->GetTransmiterMgr()->AddSession(*pData, true);
			  }else if (uid > D_ROOM_ID_UPPERLIMIT && uid< D_FREEGROUP_ID_UPPERLIMIT){
				//picture to freegroup
				uint_list list;
				pData->nRoomID =  uid;
				PStringArray  subparams= params[0].Tokenise(",");
				if (subparams.GetSize()>=2)
					pData->receiver = subparams[1].AsUnsigned();//1010，2011
				/*if (  GetTreeSubTreeIDs(uid, list)){
				  for(uint_list::iterator i = list.begin();i!= list.end();i++){
					if (*i != m_pMgr->GetMEMgr()->GetID() ){
					  stDataTransmitter* pMember= new stDataTransmitter();
                   
					  *pMember = *pData;
					  pMember->receiver= *i ;
					  m_pMgr->GetTransmiterMgr()->AddSession(*pMember, true);               
					}
				  }
				}
				delete pData;pData=NULL;*/
				 m_pMgr->GetTransmiterMgr()->AddSession(*pData, true);
			  }
           /// m_pMgr->GetTransmiterMgr()->AddSession(*pData, true);
          }
       
         }
       }else if (subcmd==2){//get a save path
          const char* strFileName = (const char*)params[3];

	        CFileDialog dlg(FALSE, NULL,( strFileName) , OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,  " Files (*.*)|*.*|All Files (*.*)|*.*||"   ,  NULL, 0); 
         //  dlg.m_ofn.lpstrFile = (char*)((const char*)strFileName);//lpstrTitle
          if( IDOK == dlg.DoModal() ){
            //key and by send file
            strResult = dlg.GetPathName().GetBuffer();
            //strResult.Format("%s;%s",(const char*) PGloballyUniqueID().AsString(), (const char*)dlg.GetPathName());
          }
       }


  }
  else if (cmd == em_PlayrecordFile){//play a record file
    if (params.GetSize()>2  ){
      stRecordHeader header={0};
       if(false== m_pMgr->GetOPALMgr()->PlayRecordFile(params[2],header )){

          MessageBox("Incorrect record file.");
       }else{
          CString strResultParam;
          strResultParam.Format("%u;%u",  header.beginTime, header.endTime);
          return strResultParam.AllocSysString();
       }
    }else{
	    CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,  "Record Files (*.yym)|*.yym|All Files (*.*)|*.*||"   ,  NULL, 0); 

      if( IDOK == dlg.DoModal() && PFile::Exists( dlg.GetPathName().GetBuffer() ) ){
        stRecordHeader header={0};
        if(false== m_pMgr->GetOPALMgr()->PlayRecordFile(dlg.GetPathName(),header )){

          MessageBox("Incorrect record file.");
        }else{
          CString strResultParam;
          strResultParam.Format("%u;%u",  header.beginTime, header.endTime);
          return strResultParam.AllocSysString();

        }
      }
    }

  }
  else if (cmd == em_SharePPT){
       int subcmd=params[2].AsUnsigned();
       if (subcmd == 0){  
	        CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,  "Files (*.*)|*.*|All Files (*.*)|*.*||"   ,  NULL, 0); 

          if( IDOK == dlg.DoModal() ){
            if  (PFile::Exists( dlg.GetPathName().GetBuffer() )) {
                strResult.Format("%s;%s",(const char*) PGloballyUniqueID().AsString(), (const char*)dlg.GetPathName());
                //strResult = dlg.GetPathName().GetBuffer();
            }
            else 
              MessageBox("Incorrect  file.");
          }else{
          

          }
       }
       else if (subcmd == 1){
          params[3].Replace("\r\n","",true);
          //params[3].Replace("\n","",all);
          if(false==AutoPrintDocument( uid, CString((const char*)params[3])) ){
          }
       }
       else if (subcmd == 2){
          params[3].Replace("\r\n","",true);
          int roomid = uid;
          CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,  "Files (*.*)|*.*|All Files (*.*)|*.*||"   ,  NULL, 0); 
          if( IDOK == dlg.DoModal() ){
            if  (PFile::Exists( dlg.GetPathName().GetBuffer() )) {
              if(false==AutoPrintDocument( D_CLIENT_MCU_ID/*uid*/, dlg.GetPathName() ,roomid) ){
              }                //strResult = dlg.GetPathName().GetBuffer();
              }
            else 
              MessageBox("Incorrect  file.");
          }else{
          }


       }

  }
	// TODO: Add your dispatch handler code here


	return strResult.AllocSysString();
}
void CYYSpriteX2Ctrl::OnReceivingFileP2P_WM(LPCTSTR value)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (m_hWndEx==NULL ) return;////MessageBox("Invalid hwnd ");
	::SendMessage(m_hWndEx, YY_OnReceivingFileP2P_WM, (WPARAM)value,0);
}

void CYYSpriteX2Ctrl::OnReceivingFileP2P(LPCTSTR value)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
		 BYTE parms[] =
		VTS_BSTR ;
	FireEvent(dispidOnReceivingFileP2P ,parms, value 
		  );

	// TODO: Add your dispatch handler code here
}
void CYYSpriteX2Ctrl::OnSendingFileP2P_WM(LPCTSTR value)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (m_hWndEx==NULL ) return;////MessageBox("Invalid hwnd ");
	::SendMessage(m_hWndEx, YY_OnSendingFileP2P_WM, (WPARAM)value,0);
}
void CYYSpriteX2Ctrl::OnSendingFileP2P(LPCTSTR value)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
		 BYTE parms[] =
		VTS_BSTR ;
	FireEvent(dispidOnSendingFileP2P ,parms, value 
		  );

	// TODO: Add your dispatch handler code here
}

LONG CYYSpriteX2Ctrl::ChangeStatus(LONG state, LPCTSTR strState)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	// TODO: Add your dispatch handler code here

	return m_pMgr->GetMEMgr()->ChangeStatus(state, strState);
}

BOOL CYYSpriteX2Ctrl::DestroyWindow()
{
	// TODO: Add your specialized code here and/or call the base class
	//m_pMgr->Destroy();
	//delete m_pMgr;m_pMgr=NULL;

	return COleControl::DestroyWindow();
}
void CYYSpriteX2Ctrl::OnSETPIXELS_NOCONV_WM(LPCTSTR VARIANT_SAFEARRAY_UID_TAG, LONG pData )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (m_hWndEx==NULL ) return;////MessageBox("Invalid hwnd ");
	::SendMessage(m_hWndEx, YY_OnSETPIXELS_NOCONV_WM, (WPARAM)VARIANT_SAFEARRAY_UID_TAG, pData);
}
void CYYSpriteX2Ctrl::OnSETPIXELS_NOCONV(LPCTSTR VARIANT_SAFEARRAY_UID_TAG, LONG x, LONG y, LONG w, LONG h)
{
	//x= m_pMgr->GetOPALMgr()->m_nClientID;
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
//#if 0
//	BYTE parms[] =
//		VTS_BSTR VTS_I4 VTS_I4 VTS_I4 VTS_I4 ;
//
//		FireEvent(dispidOnSETPIXELS_NOCONV ,parms, buffer ,x, 0, 0, 0
//		  );
//#else
//	VARIANT *pVar=NULL ;
//	SAFEARRAY* psa=NULL; 
//	PStringArray arr = PString(VARIANT_SAFEARRAY_UID_TAG).Tokenise(";");
//	PString cmd;
//	for(int i=0;i< arr.GetSize();i++)
//	{
//		if (i==0) pVar =(VARIANT *) arr[i].AsUnsigned();
//		else if (i==1) psa = (SAFEARRAY*)arr[i].AsUnsigned();
//		else
//		{
//			cmd+= (arr[i]+";");
//		}
//
//	}
//			BYTE parms[] =
//		VTS_BSTR VTS_VARIANT ;
//	FireEvent(dispidOnSETPIXELS_NOCONV ,parms, (const char* )cmd ,pVar
//		  );
//	SafeArrayDestroy(psa);
//	VariantClear(pVar);
//#endif
	// TODO: Add your dispatch handler code here
}

LONG CYYSpriteX2Ctrl::FTOF(LPCTSTR uid, LPCTSTR pws)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  int ncmd = atoi(uid);
  if (ncmd == 0){
  #if 0
    m_pMgr->GetMEMgr()->TestViewer();

  #else
    PStringArray strMsg = PString(uid).Tokenise(";");
    uint32 nUID=0, type =0;
    PString strMeetingId;
   
    if (strMsg.GetSize() >0 ) 
      nUID = strMsg[0].AsUnsigned();
    if (strMsg.GetSize() >1 )
      type = strMsg[1].AsUnsigned();
    if (strMsg.GetSize() >2 )
      strMeetingId = strMsg[2];

    m_pMgr->GetMEMgr()->MakeRDCall(nUID, type,strMeetingId );
  #endif// TODO: Add your dispatch handler code here
  }else if (ncmd ==em_MCC_Remove_Room){
    //destroy conf
    PStringArray arr= PString(pws).Tokenise(";");
    m_pMgr->GetMEMgr()->NotifyMCCEventCMD(arr[0].AsInteger(),em_MCC_Remove_Room,"","");

  }else if (ncmd ==em_MCC_Add_participator)
  {
    //add a participator
    PStringArray arr= PString(pws).Tokenise(";");
    m_pMgr->GetMEMgr()->NotifyMCCEventCMD(arr[0].AsInteger(),em_MCC_Add_participator,arr[1],"");

  }
	return 0;
}
void CYYSpriteX2Ctrl::OnyFTOFState_WM(LONG code, LPCTSTR state )//state 0:0k, 1:user deny, 2:timeout, 3:request channel error
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (m_hWndEx==NULL ) return;////MessageBox("Invalid hwnd ");
	::SendMessage(m_hWndEx, YY_OnyFTOFState_WM, (WPARAM)code,(LPARAM) state);
}
void CYYSpriteX2Ctrl::OnyFTOFState(LONG code, LPCTSTR state)
{

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
		 BYTE parms[] =
		VTS_I4 VTS_BSTR ;
	FireEvent(dispidOnyFTOFState ,parms, code ,state
		  );
}
void CYYSpriteX2Ctrl::OnSetTlight_WM(LONG uid, LPCTSTR VARIANT_SAFEARRAY_UID_TAG )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (m_hWndEx==NULL ) return;////MessageBox("Invalid hwnd ");
	::SendMessage(m_hWndEx, YY_OnSetTlight_WM, (WPARAM)uid,(LPARAM) VARIANT_SAFEARRAY_UID_TAG);
}
void CYYSpriteX2Ctrl::OnCallState_WM(LONG state, LPCTSTR params )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (m_hWndEx==NULL ) return;////MessageBox("Invalid hwnd ");
	::SendMessage(m_hWndEx, YY_OnCallState_WM, (WPARAM)state,(LPARAM) params);
}
void CYYSpriteX2Ctrl::OnNotify_WM(LONG state, LPCTSTR params )
{
	///AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (m_hWndEx==NULL ) return;////MessageBox("Invalid hwnd ");
  LRESULT ret = 0;
  int nTrying =50;
  
  while (ret!=200 && nTrying-->0 ){
    PString * str = new PString();
    *str = params;
    if (state == em_mcc_user_state){
      ret = ::PostMessage(m_hWndEx, YY_OnNotify_WM, (WPARAM)state,(LPARAM) str);
      return;
    }
    else
      ret = ::SendMessage(m_hWndEx, YY_OnNotify_WM, (WPARAM)state,(LPARAM) str/*params*/);
   if (ret != 200)
    Sleep(100);
  }


}
void CYYSpriteX2Ctrl::OnYYClientState_WM(LONG state, LPCTSTR params )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (m_hWndEx==NULL ) return;////MessageBox("Invalid hwnd ");
	::SendMessage(m_hWndEx, YY_OnYYClientState_WM, (WPARAM)state,(LPARAM) params);
}
void CYYSpriteX2Ctrl::OnSetTlight(LONG uid, LPCTSTR value)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
#ifdef D_USE_VNC_VIEWER

#if 1
		 BYTE parms[] =
		VTS_I4 VTS_BSTR ;
	FireEvent(dispidOnSetTlight ,parms, uid ,value
		  );
#else
	VARIANT *pVar=NULL ;
	SAFEARRAY* psa=NULL; 
	PStringArray arr = PString(VARIANT_SAFEARRAY_UID_TAG).Tokenise(";");
	PString cmd;
	for(int i=0;i< arr.GetSize();i++)
	{
		if (i==0) pVar =(VARIANT *) arr[i].AsUnsigned();
		else if (i==1) psa = (SAFEARRAY*)arr[i].AsUnsigned();
		else
		{
			cmd+= (arr[i]+";");
		}

	}
			BYTE parms[] =
		VTS_BSTR VTS_VARIANT ;
	FireEvent(dispidOnSETPIXELS_NOCONV ,parms, (const char* )cmd ,pVar
		  );
	SafeArrayDestroy(psa);
	VariantClear(pVar);

#endif
	// TODO: Add your dispatch handler code here
#endif
}

LONG CYYSpriteX2Ctrl::WindowProc(LONG uid, LONG iMsg, LPCTSTR Params)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	// TODO: Add your dispatch handler code here
//	m_pMgr->GetMEMgr()->VNCClientWindowProc(uid, iMsg, Params);
	return 0;
}

void CYYSpriteX2Ctrl::OnSETPIXELS_NOCONV_WMEx(LPCTSTR tag, VARIANT* value)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());


	// TODO: Add your dispatch handler code here
}

void CYYSpriteX2Ctrl::OnSETPIXELS_NOCONV_WMEx2(LPCTSTR tag, VARIANT value)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
#ifdef D_USE_VNC_VIEWER

#if 0
     long nData[10]={1,2,3,4,5,6,7,8,9,10};
     SAFEARRAY* pArray=NULL;
     HRESULT hr=SafeArrayAllocDescriptor(1,&pArray);//创建SAFEARRAY结构的对象
     pArray->cbElements=sizeof(nData[0]);
     pArray->rgsabound[0].cElements=10;
     pArray->rgsabound[0].lLbound=0;
     SafeArrayAllocData(pArray);
     long* pData=NULL;
     SafeArrayAccessData(pArray,(void**)&pData);
     long l(0),h(0);

     SafeArrayGetLBound(pArray,1,&l);

     SafeArrayGetUBound(pArray,1,&h);

     long Size=h-l+1;

     SafeArrayAccessData(pArray,(void**)&pData);

     for(long Idx=l;Idx<Size;++Idx)
     {
          pData[Idx]=nData[Idx];

     }
     SafeArrayUnaccessData(pArray);
		 //CComSafeArray  dd;
		 //CComVariant var(pArray); 
    //var.Attach(val);

		 VARIANT varChunk;
		 VariantInit (&varChunk);
		 VARTYPE vt;
		SafeArrayGetVartype(pArray,&vt);   

		varChunk.vt =   VT_ARRAY| VT_VARIANT;
		varChunk.parray = pArray;

		 BYTE parms[] =
		VTS_BSTR VTS_VARIANT ;
	FireEvent(dispidOnSETPIXELS_NOCONV_WMEx ,parms, "tag" ,&varChunk
		  );
   SafeArrayDestroy(pArray);
VariantClear(&varChunk);
#else

#if 0
		 BYTE parms[] =
		VTS_BSTR VTS_VARIANT ;
	FireEvent(dispidOnSETPIXELS_NOCONV_WMEx ,parms, "tag" ,&var
		  );
#else
	long cTable[10]={7,6,5,4,5,6,7,8,9,10};
		VARIANT   retvalV;
		VariantInit(&retvalV);
		VARIANT*   retval=&retvalV;
      //返回数组 
      SAFEARRAY     FAR*     psa; 
      //数组维数 
      SAFEARRAYBOUND     rgsabound[1]; 

      rgsabound[0].lLbound   =   0; 
      rgsabound[0].cElements   =   3; 
      psa=SafeArrayCreate(VT_VARIANT,1,rgsabound);//我这里原来没用VT_VARIANT，而是用了VT_I4类型，结果会导致JS代码中new   VBArray出错，请一定注意 

      long   idx; 
      VARIANT   setdt;
      setdt.vt   =   VT_I4; 

      //赋值 
      idx   =   0; 
      setdt.lVal   =   1; 
      SafeArrayPutElement(psa,&idx,&setdt); 
      idx   =   1; 
      setdt.lVal   =   2; 
      SafeArrayPutElement(psa,&idx,&setdt); 
			void *pData=NULL;
			//SafeArrayAccessData(psa, &pData);
			//memcpy(pData, cTable, 4*3);
      //返回安全数组 
      V_VT(retval)     =     VT_ARRAY   |   VT_VARIANT; 
      V_ARRAY(retval)   =   psa; 

			BYTE parms[] =
		VTS_BSTR VTS_VARIANT ;
	FireEvent(dispidOnSETPIXELS_NOCONV_WMEx ,parms, "tag2" ,retval
		  );
	   SafeArrayDestroy(psa);
		 VariantClear(retval);
#endif
#endif

#endif
	// TODO: Add your dispatch handler code here
}

void CYYSpriteX2Ctrl::OnSETPIXELS_NOCONV2(LPCTSTR VARIANT_SAFEARRAY_UID_TAG, LONG x)
{

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
#ifdef D_USE_VNC_VIEWER
  VARIANT   retvalV;
	VariantInit(&retvalV);
	VARIANT *pVar=&retvalV ;
 	//VARIANT *pVar=NULL ;
	SAFEARRAY* psa=NULL; 
	PStringArray arr = PString(VARIANT_SAFEARRAY_UID_TAG).Tokenise(";");
	PString cmd;
	for(int i=0;i< arr.GetSize();i++)
	{
		if (i==0) continue;//pVar =(VARIANT *) arr[i].AsUnsigned();
		else if (i==1) psa = (SAFEARRAY*)arr[i].AsUnsigned();
		else
		{
			cmd+= (arr[i]+";");
		}

	}
	V_VT(pVar)     =     VT_ARRAY   |   VT_VARIANT; 
	V_ARRAY(pVar)   =   psa; 

			BYTE parms[] =
		VTS_BSTR VTS_VARIANT ;
	FireEvent(dispidOnSETPIXELS_NOCONV2 ,parms, (const char* )cmd ,pVar
		  );
	SafeArrayDestroy(psa);
	//VariantClear(pVar);
#endif
 }

void CYYSpriteX2Ctrl::OnCallState(LONG state, LPCTSTR Params2)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
		 BYTE parms[] =
		VTS_I4 VTS_BSTR ;
	FireEvent(dispidOnCallState ,parms, state ,Params2
		  );
	// TODO: Add your dispatch handler code here
}

void CYYSpriteX2Ctrl::OnYYClientState(LONG code, LPCTSTR Tag)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: Add your dispatch handler code here
}

LONG CYYSpriteX2Ctrl::SheduleMeet(LPCTSTR strV)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  
	// TODO: Add your dispatch handler code here
	BBQ_MeetingInfo Me ={0};
	unsigned char out[2048]={0};
	int ret = PKI_decode((const unsigned char*)strV, strlen(strV), out);

	if (ret> sizeof(BBQ_MeetingInfo))
	{
		MessageBox("SheduleMeet error for decode.");
		return -1;
	}
	
	memcpy(&Me, out, ret);
	bool bRet =m_pMgr->GetMEMgr()->SendSheduleMeet(&Me);
  //if (bRet){
  //  if (Me.command == 2 ){
  //    //reinit freegroup
  //    //loadcug
  //    CString str= GetTreeReturnCString(true);  
  //    OnNotify_WM(em_UserInfoFriendOrCUGOrFreeGroup, "2"+str);


  //  }
  //}
	return bRet? 0:1;
}

int CYYSpriteX2Ctrl::OnNotify(LONG type, LPCTSTR value)
{
 	AFX_MANAGE_STATE(AfxGetStaticModuleState());
 //first notify
  if ( type == em_ViewUserInfo){//send userinfo to sessioned IM dlgs 
    PStringArray arr = PString(value).Tokenise(";");//get sender
    unsigned int uidSender = arr[0].AsUnsigned();
    if (arr.GetSize() >0 ){
       COptionsExDlg* pDlg=NULL;
       for( MapIMDialogs::iterator i = m_mapIMDlgs.begin();i!= m_mapIMDlgs.end();i++){
         if (i->second->m_bXamlInit == false )
           continue;
         if (i->second->m_nUserID == uidSender || i->second->m_nYYmeetingUserID == uidSender || (i->first <D_FREEGROUP_ID_UPPERLIMIT/*D_ROOM_ID_UPPERLIMIT*/ )){
            
           if (i->second->m_bXamlInit&& i->second->GetSafeHwnd() &&  i->second->IsWindowVisible() ){
              pDlg = i->second; 
                
              CString strParam;
              strParam= value;
              //MessageBox(strParam);
              CStringArray strparams;

              strparams.Add(strParam);
              CComVariant ret=  pDlg->CallJScript("yOnNotifyUserInfo",strparams);
              return ret.intVal;
           }else
             return -1;
         }
       }
     
    }
  }
  if (type == em_MCC || type == em_mcc_user_state ) {
     PStringArray arr = PString(value).Tokenise(";");
     int nRoomID= arr[3].AsInteger();
     //if (m_pMgr->GetMEMgr()->GetMCCPrompted(nRoomID)== false)
     {
        if (type == em_MCC) {
          for( MapIMDialogs::iterator i = m_mapIMDlgs.begin();i!= m_mapIMDlgs.end();i++){
            if ( i->second->m_nUserID == nRoomID && i->second->GetSafeHwnd() !=NULL )
            {
              CString strParam =value;
              CStringArray strparams;
              strparams.Add(strParam);
              i->second->CallJScript("yOnNotifyMCC",strparams);
              return 200;
            }
          }
          //display the mcc notify dlg
         // PString strMsg ;strMsg.sprintf("14;%d", nRoomID);
         // IMDlgControl(strMsg );
        }else if (type == em_mcc_user_state){
          for( MapIMDialogs::iterator i = m_mapIMDlgs.begin();i!= m_mapIMDlgs.end();i++){
            if ( i->second->m_nUserID>0&& i->second->m_nUserID< D_ROOM_ID_UPPERLIMIT &&i->second->GetSafeHwnd() && i->second->IsWindowVisible() && i->second->m_bXamlInit  /* i->second->GetSafeHwnd() !=NULL*/ )
            {
              CString strParam =value;
              CStringArray strparams;
              strparams.Add(strParam);
              i->second->CallJScript("yOnNotifyEx",strparams);
            }else
              return -1;
          }
          return 200;
        }
     }
      
  }
  else if (type ==em_UserLogin ){//display welcome
    //if (value&& strlen(value) >0 && value[0] == '0'/* 0 ok,1 failed*/){
    //  char tmp[128]={0};
    //  g_GetSettingStringSafe("welcome_not_show",tmp, "0");
    //  if (atoi(tmp)== 0)
    //    IMDlgControl("11:0");
    //}
  }
  //second notify
  if (type == em_UserSendPicOK||type==em_UserShareDocOK||type== em_UserSendFileOK ){//for the case we call js VIA dalog
    PStringArray arr = PString(value).Tokenise(";");//get sender
    unsigned int uidSender =arr[0].AsInteger();
    if (arr.GetSize()>=3 && type == em_UserSendPicOK && arr[2].AsInteger() >0 /*if room id > 0*/) uidSender =arr[2].AsInteger();
    if (arr.GetSize() >0 ){
       COptionsExDlg* pDlg=NULL;
       MapIMDialogs::iterator ifound = m_mapIMDlgs.find(uidSender/*arr[0].AsUnsigned()*/);
      if (ifound ==  m_mapIMDlgs.end() ){
        //BBQUserInfo user={0};
        //m_pMgr->GetMEMgr()->ViewUserData(uidSender, user);
        pDlg = CreatePopIMDlg(uidSender, (const char*)UTF8ToPlainText( (const char*)m_pMgr->GetMEMgr()->GetAliasName(uidSender) /*user.basic.alias*/));
        
      }else
        pDlg = ifound->second ;
      //ifound = m_mapIMDlgs.find(arr[0].AsUnsigned());
      if (pDlg){
        CString strParam;
        strParam= value;
        //MessageBox(strParam);
        CStringArray strparams;

        strparams.Add(strParam);
        if (type == em_UserSendPicOK)
          pDlg->CallJScript("yOnNotify",strparams);
        else if (type == em_UserShareDocOK)
          pDlg->CallJScript("yOnNotifyPPT",strparams);
        else if (type == em_UserSendFileOK)
          pDlg->CallJScript("yOnNotifyFile",strparams);
        else if (type == em_UserSendDesktopOK)
          pDlg->CallJScript("yOnNotifyDesktop",strparams);
      }
    }
  }

  else if (type == em_OptionDialog){
    if (m_pOptdlg){
        CString strParam;
        strParam= value;
        CStringArray strparams;
        strparams.Add(strParam);
        m_pOptdlg->CallJScript("yOnNotify",strparams);
    }
  }
  else{
    //notefy it to main dlg
	  AFX_MANAGE_STATE(AfxGetStaticModuleState());
		  BYTE parms[] =
		  VTS_I4 VTS_BSTR ;
	  FireEvent(dispidOnNotify ,parms, type ,value
		    );
  }
  return 200;
}

LONG CYYSpriteX2Ctrl::EnterRoom(LONG roomid, LPCTSTR tag)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  m_pMgr->GetMEMgr()->EnterRoom(roomid);
  // TODO: Add your dispatch handler code here

  return 0;
}

BSTR CYYSpriteX2Ctrl::DlgLocalVideo(void)
{
  CString strResult;
  if (g_VideoLayout == em_videoLayoutAdvanced) return strResult.AllocSysString();
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  HWND hwnd =(HWND) m_pMgr->GetOPALMgr()->GetRemoteVideoWindowHWNDbyRemoteUserID("0", true);
  BOOL bAlreadyVisible = FALSE;
  m_pMgr->GetOPALMgr()->m_bIsAutoCloseLocalVideo = false;
  if (hwnd == 0) {
    m_pMgr->GetOPALMgr()->LoadLocalVideo();
     m_pMgr->GetOPALMgr()->SetLocalVideoVisiable(false );
    //m_pMgr->SetCamaraName(0);
    //return strResult.AllocSysString();;
  }else{
   // bAlreadyVisible = ::IsWindowVisible(hwnd);
   // m_pMgr->GetOPALMgr()->SetLocalVideoVisiable(!bAlreadyVisible );
  }

  strResult= bAlreadyVisible?"0":"1";
  return  strResult.AllocSysString();
  // TODO: Add your dispatch handler code here
}
extern void g_OnOptions(LPNMgr * pLPNMgr, CWnd* pWnd);

void CYYSpriteX2Ctrl::DlgOption(LONG nWhichTab)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
#if 0
  if (m_pOptdlg== NULL){
    m_pOptdlg= new COptionsExDlg();
    m_pOptdlg->Create(IDD_DIALOG_OPTIONS);
    m_pOptdlg->UpdateWindow();
  }
  switch (nWhichTab){
    case 0:
        m_pOptdlg->ShowWindow(SW_SHOW);
      break;
    case 1:
        m_pOptdlg->ShowWindow(SW_HIDE);
      break;    
  };
#else
  COptionsExDlg dlg;
  dlg.m_emStyle = em_optDlg;
  dlg.m_pMgr = m_pMgr;
  dlg.DoModal();
#endif
  //g_OnOptions( m_pMgr, this );

  // TODO: Add your dispatch handler code here
}

void CYYSpriteX2Ctrl::yDestroy(void)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  m_hwndMainContainerPanel =NULL;
  m_hwndLocalVideoPanel =NULL;
  delete m_pCUGTree;m_pCUGTree=NULL;
  if (  m_plistonlinedlg) {
    ::SendMessage( m_plistonlinedlg->GetSafeHwnd(), WM_CLOSE, 0,0 );
    ::SendMessage( m_plistonlinedlg->GetSafeHwnd(), WM_DESTROY, 0,0 );
    delete m_plistonlinedlg;m_plistonlinedlg=NULL;
  }
  if (  m_pviewuserdlg) {
    ::SendMessage( m_pviewuserdlg->GetSafeHwnd(), WM_CLOSE, 0,0 );
    ::SendMessage( m_pviewuserdlg->GetSafeHwnd(), WM_DESTROY, 0,0 );
    delete m_pviewuserdlg;m_pviewuserdlg=NULL;
  }
  if (  m_pcreatemeetdlg) {
    ::SendMessage( m_pcreatemeetdlg->GetSafeHwnd(), WM_CLOSE, 0,0 );
    ::SendMessage( m_pcreatemeetdlg->GetSafeHwnd(), WM_DESTROY, 0,0 );
    delete m_pcreatemeetdlg;m_pcreatemeetdlg=NULL;
  }
  if (  m_pcreatefreegroupdlg) {
    ::SendMessage( m_pcreatefreegroupdlg->GetSafeHwnd(), WM_CLOSE, 0,0 );
    ::SendMessage( m_pcreatefreegroupdlg->GetSafeHwnd(), WM_DESTROY, 0,0 );
    delete m_pcreatefreegroupdlg;m_pcreatefreegroupdlg=NULL;
  }
  if (  m_pOptdlg) {
    ::SendMessage( m_pOptdlg->GetSafeHwnd(), WM_CLOSE, 0,0 );
    ::SendMessage( m_pOptdlg->GetSafeHwnd(), WM_DESTROY, 0,0 );
    delete m_pOptdlg;m_pOptdlg=NULL;
  } 
  if (  m_pmessageboxdlg) {
    ::SendMessage( m_pmessageboxdlg->GetSafeHwnd(), WM_CLOSE, 0,0 );
    ::SendMessage( m_pmessageboxdlg->GetSafeHwnd(), WM_DESTROY, 0,0 );
    delete m_pmessageboxdlg;m_pmessageboxdlg=NULL;
  } 
  if (  m_pmwelcomedlg) {
    ::SendMessage( m_pmwelcomedlg->GetSafeHwnd(), WM_CLOSE, 0,0 );
    ::SendMessage( m_pmwelcomedlg->GetSafeHwnd(), WM_DESTROY, 0,0 );
    delete m_pmwelcomedlg;m_pmwelcomedlg=NULL;
  }
  if (  m_pguideAudioDlg) {
    ::SendMessage( m_pguideAudioDlg->GetSafeHwnd(), WM_CLOSE, 0,0 );
    ::SendMessage( m_pguideAudioDlg->GetSafeHwnd(), WM_DESTROY, 0,0 );
    delete m_pguideAudioDlg;m_pguideAudioDlg=NULL;
  }
  if (  m_pVideoPanelDlg) {
    ::SendMessage( m_pVideoPanelDlg->GetSafeHwnd(), WM_CLOSE, 0,0 );
    ::SendMessage( m_pVideoPanelDlg->GetSafeHwnd(), WM_DESTROY, 0,0 );
    delete m_pVideoPanelDlg;m_pVideoPanelDlg=NULL;
  } 
  for( MapIMDialogs::iterator i = m_mapIMDlgs.begin();i!= m_mapIMDlgs.end();i++){
    ::SendMessage( i->second->GetSafeHwnd(), WM_CLOSE, 0,0 );
    ::SendMessage( i->second->GetSafeHwnd(), WM_DESTROY, 0,0 );
     delete i->second;
    i->second=NULL;
  }
  for( MapIMDialogs::iterator i = m_mapMCCNotifyDlgs.begin();i!= m_mapMCCNotifyDlgs.end();i++){
      
    i->second->DestroyWindow();
     delete i->second;
    i->second=NULL;
   }  
  m_mapIMDlgs.clear();
  m_mapMCCNotifyDlgs.clear();
  /////////////////////////////
  ::SendMessage(m_hWndEx, WM_CLOSE, 0,0);
  HWND yyHwnd = m_hWndEx;m_hWndEx=NULL;
	for (std::list<CIncomingDlg* >::iterator i = m_listIncomingDlgs.begin();i!= m_listIncomingDlgs.end();i++)
	{
		delete (*i);
	}
  if (m_pMgr) 	m_pMgr->Destroy(); //in destroywindow
	delete m_pMgr;m_pMgr=NULL;
	m_listIncomingDlgs.clear();
	std::map<HWND, CYYSpriteX2Ctrl*>::iterator itor=  g_YYSpriteCtrls.find(yyHwnd);
	if (itor!= g_YYSpriteCtrls.end())
	{
		g_YYSpriteCtrls.erase(itor);
	}
  //g_YYSpriteCtrls.clear ,do not call this fun
  if (yyHwnd){
		::SendMessage(yyHwnd, WM_CLOSE, 0, 0);
		::SendMessage(yyHwnd, WM_DESTROY, 0, 0);
  }
	m_hWndEx =NULL;
	UnregisterClass(strwndClassName.GetBuffer(), GetModuleHandle(NULL));
	// TODO: Cleanup your control's instance data here.
  // TODO: Add your dispatch handler code here
  PString strRemovepath;
  strRemovepath.sprintf("%s\\YY\\doc", GetAppPath());
  gfx_DeleteDirectory( strRemovepath );

}
BOOL  CYYSpriteX2Ctrl::Pop_CreateProcess(unsigned int uid,const char* uidAlias)
{
  char cmd[512]={0};
  sprintf(cmd,"%syymeeting.exe -uid %u -uname %s -uyymeetingid %d",(const char*)PProcess::Current().GetFile().GetDirectory(), uid, (const char*)uidAlias,  m_pMgr->GetMEMgr()->GetID());
  // MessageBox(cmd);
  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  ZeroMemory( &si, sizeof(si) );
  si.cb = sizeof(si);
  ZeroMemory( &pi, sizeof(pi) );

  return ( CreateProcess(NULL,  cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi));

}
void CYYSpriteX2Ctrl::DestroyPopIMDlg(unsigned int  imDlgOwer)
{
  MapIMDialogs::iterator ifound = m_mapIMDlgs.find(imDlgOwer);
  if (ifound!= m_mapIMDlgs.end()){
    if (ifound->second ){
      //ifound->second->SendMessage(WM_CLOSE);
      ifound->second->Navigate("about:Tabs");
      ifound->second->CloseWindow();
      ifound->second->DestroyWindow();
      delete ifound->second;m_mapIMDlgs.erase(ifound);
    }
  } 
}
void CYYSpriteX2Ctrl::ShowOrHidPopIMDlg(unsigned int  imDlgOwer, bool bShow)
{
  MapIMDialogs::iterator ifound = m_mapIMDlgs.find(imDlgOwer);
  if (ifound!= m_mapIMDlgs.end()){
    if (ifound->second  && ifound->second->GetSafeHwnd() ){
      ifound->second->ShowWindow(bShow? SW_SHOW :SW_HIDE );
    }
  } 
}

bool  CYYSpriteX2Ctrl::DestroyPopDlg(unsigned int  imDlgOwer, int nStyle )
{
  if (nStyle == em_MCCNotifyDlg)
  {
    MapIMDialogs::iterator ifound = m_mapMCCNotifyDlgs.find(imDlgOwer);
    if (ifound!= m_mapMCCNotifyDlgs.end()){
      //ifound->second->CloseWindow( );
      ifound->second->DestroyWindow();
      delete ifound->second;
      m_mapMCCNotifyDlgs.erase(ifound);
      return true;
    }

  }else if (nStyle == em_popIM){
    MapIMDialogs::iterator ifound = m_mapIMDlgs.find(imDlgOwer);
    if (ifound!= m_mapIMDlgs.end()){
       
       // ifound->second->CloseWindow();
      ifound->second->DestroyWindow();
      delete ifound->second;
      m_mapIMDlgs.erase(ifound);
      return true;
    }
  }
  //em_optDlg, em_listonlineDlg,  em_viewuserDlg,  em_createmeetDlg, em_MsgBox=10, em_welcomeDlg=11,em_guideAudioDlg=12,em_createfreegroupDlg
  else if (nStyle == em_optDlg){//m_pOptdlg, *m_plistonlinedlg,*m_pviewuserdlg, *m_pcreatemeetdlg,*m_pmessageboxdlg ,*m_pmwelcomedlg,*m_pguideAudioDlg,*m_pcreatefreegroupdlg  ;//em_optDlg, em_listonlineDlg,  em_viewuserDlg,  em_createmeetDlg, 
    delete m_pOptdlg; m_pOptdlg=NULL;
  }else if (nStyle == em_listonlineDlg){
    delete m_plistonlinedlg; m_plistonlinedlg=NULL;
  }else if (nStyle == em_viewuserDlg){
    delete m_pviewuserdlg; m_pviewuserdlg=NULL;
  }else if (nStyle == em_createmeetDlg){
    delete m_pcreatemeetdlg; m_pcreatemeetdlg=NULL;
  }else if (nStyle == em_welcomeDlg){
    delete m_pmwelcomedlg; m_pmwelcomedlg=NULL;
  }else if (nStyle == em_guideAudioDlg){
    delete m_pguideAudioDlg; m_pguideAudioDlg=NULL;
  }else if (nStyle == em_createfreegroupDlg){
    delete m_pcreatefreegroupdlg; m_pcreatefreegroupdlg=NULL;
  } 
  return false;
}
COptionsExDlg* CYYSpriteX2Ctrl::CreatePopIMDlg(unsigned int  imDlgOwer,const char* imDlgOweraAlias, CWnd* pParent, bool bShow, bool bforcechild )
{
#if 1
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
 
    {

        MapIMDialogs::iterator ifound = m_mapIMDlgs.find(imDlgOwer);
        if (ifound!= m_mapIMDlgs.end()){
          ifound->second->DestroyWindow();
          delete ifound->second;
         // if (ifound->second && ifound->second->IsChild() ==FALSE){
         //   //ifound->second->ModifyStyle()
         // }
        } 
    }
    if (imDlgOwer> D_ROOM_ID_UPPERLIMIT ){
      pParent = FromHandle( m_hwndMainContainerPanel);
    }else
      pParent = m_pVideoPanelDlg;
    COptionsExDlg * p = new COptionsExDlg(pParent);
    p->m_pMgr= m_pMgr;
    p->m_emStyle= em_popIM;
    p->m_nUserID = imDlgOwer;
    p->m_nYYmeetingUserID =  m_pMgr->GetMEMgr()->GetID();
    if (strlen(imDlgOweraAlias)<=0 ){
     /* if (imDlgOwer < D_ROOM_ID_UPPERLIMIT ){
        BBQ_MeetingInfo info={0};
        m_pMgr->GetMEMgr()->GetMeetingInforByRoomID(imDlgOwer,info);
        p->m_strUserAlias = (const char*)UTF8ToPlainText( info.strMeetingName);
      }else*/
        p->m_strUserAlias =(const char*)m_pMgr->GetMEMgr()->GetAliasName(imDlgOwer);
    }
    else
      p->m_strUserAlias = (const char*) imDlgOweraAlias/*strValues[2]*/;
    
    p->Create(IDD_DIALOG_OPTIONS,pParent);
   //p-> SetWindowPos(&wndNoTopMost,   0,   0,   0,   0,   SWP_NOSIZE   |   SWP_NOMOVE); 
    if (bforcechild)
    {
      p->ModifyStyle(WS_POPUP|WS_BORDER,WS_CHILD,true);
      p->SetParent(pParent);
    }
 
   if (bShow)
      p->ShowWindowEx();
   //
    m_mapIMDlgs[imDlgOwer]= p;
    return p;
#else
  YYThreadDlg *pp= new YYThreadDlg(m_pMgr,imDlgOwer ,imDlgOweraAlias );
  pp->m_started.Wait();
  return pp->m_pDlg;
#endif
      
}
//void CYYSpriteX2Ctrl::RemoveIMDlg(unsigned int uid){
//    MapIMDialogs::iterator ifound = m_mapIMDlgs.find(uid);
//    if (ifound !=  m_mapIMDlgs.end() ){
//      ifound->second->DestroyWindow();
//      delete ifound->second;
//      m_mapIMDlgs.erase(ifound);
//    }
//}
BSTR CYYSpriteX2Ctrl::IMDlgControl(LPCTSTR ctrlcode)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
 // AfxMessageBox(ctrlcode, MB_OK, 0);
  CString strResult;
  PStringArray strValues = PString(ctrlcode).Tokenise(";");
  unsigned int cmd = strValues[0].AsUnsigned();
  unsigned int uid = strValues[1].AsUnsigned();
 // MessageBox(ctrlcode);
  if (cmd == 0){
    //close the pop dlg
    MapIMDialogs::iterator ifound = m_mapIMDlgs.find(uid);
    if (ifound!=  m_mapIMDlgs.end()){
      m_mapIMDlgs.erase(ifound);
    }
  }
  else if (cmd == 1){
    MapIMDialogs::iterator ifound = m_mapIMDlgs.find(uid);
    //if (ifound!=  m_mapIMDlgs.end()&& FALSE== ifound->second->IsWindowVisible() &&){
    //  RemoveIMDlg(uid);//always remove the IM dialog
    //  ifound = m_mapIMDlgs.find(uid);
    //}
    if (ifound ==  m_mapIMDlgs.end() ){
#if 1 
      COptionsExDlg *p = CreatePopIMDlg( uid, strValues[2]);//
      m_mapIMDlgs[uid]= p;
      //if (uid>=0 && uid < D_ROOM_ID_UPPERLIMIT ){
      //  ::PostMessage(  GetHWNDEX()  , 1021+33/*YY_CREATE_CCVIDEOLAYOUT_WM*/, 1, 0);//
      //}
#else
    //ifound = m_mapIMDlgs.find(uid);
    
    stThreadCommand * pCmd = new stThreadCommand();
    stUserID* pParam = new stUserID();
    pCmd->cmd = em_threadCreatePopDialog;
    PString strAlias = strValues[2]!=""?strValues[2]:m_pMgr->GetMEMgr()->GetAliasName(uid);
    pParam->m_uid = uid;strncpy( pParam->m_alias, (const char*)strValues[2], sizeof( pParam->m_alias)-1);
    pCmd->tag = pParam;
     
    m_pMgr->GetMEMgr()->PostPTThreadCommand(pCmd);
#endif
    }else{
     // MapIMDialogs::iterator ifound = m_mapIMDlgs.find(uid);
      if (  ::IsWindowVisible(ifound->second->GetSafeHwnd()) ){
        //ifound->second->FlashWindow();
       // ifound->second->ShowWindow(SW_SHOWMAXIMIZED);
        ifound->second->ShowWindow(SW_SHOW);
        ifound->second->SetForegroundWindow();
      }else  
      {
        /*if (ifound->second->GetSafeHwnd()!=NULL && ifound->second->m_nUserID < D_MAX_MEETING_MEMBER){
          ifound->second->ShowWindow(SW_SHOW);
        }else*/
          ifound->second->ShowWindowEx();
      }
    }
  }


  else if (cmd == 2)//em_optDlg, em_listonlineDlg,  em_viewuserDlg,  em_createmeetDlg, em_welcomeDlg,em_guideAudioDlg
  {
    if ( m_pOptdlg==NULL ){//em_optDlg
      COptionsExDlg * p= m_pOptdlg= new COptionsExDlg(GetDesktopWindow());
      p->m_pMgr= m_pMgr;
      p->m_emStyle= em_optDlg;
      p->m_nUserID = uid;
      p->m_nYYmeetingUserID =  m_pMgr->GetMEMgr()->GetID();
      p->m_strUserAlias = "";
      p->Create(IDD_DIALOG_OPTIONS);
      p->ShowWindowEx();
      
    }else{
#ifdef _DEBUG
      m_pOptdlg->ShowWindowEx();
      m_pOptdlg->ShowWindow(SW_SHOW);
#else
      m_pOptdlg->ShowWindow(SW_SHOW);

#endif
    }
  }
  else if (cmd == 3){//em_listonlineDlg
    if (m_plistonlinedlg== NULL){
      COptionsExDlg * p =m_plistonlinedlg= new COptionsExDlg();
     
      p->m_pMgr= m_pMgr;
      p->m_emStyle= em_listonlineDlg;
      //p->m_nUserID = uid;
      p->m_nYYmeetingUserID =  m_pMgr->GetMEMgr()->GetID();
      //p->m_strUserAlias = (const char*) strValues[2];
      p->Create(IDD_DIALOG_OPTIONS);
      //p->UpdateWindow();
      m_plistonlinedlg->ShowWindowEx();
    }else{
      m_plistonlinedlg->m_emSubStyle = em_dispListOnline;
      m_plistonlinedlg->ShowWindowEx();
      m_plistonlinedlg->ShowWindow(SW_SHOW);
    }
  }
  else if (cmd == 4){//em_viewuserDlg
    if (m_pviewuserdlg== NULL){
      COptionsExDlg * p =m_pviewuserdlg= new COptionsExDlg();
     
      p->m_pMgr= m_pMgr;
      p->m_emStyle= em_viewuserDlg;
      p->m_nUserID = uid;
      p->m_nYYmeetingUserID =  m_pMgr->GetMEMgr()->GetID();
      //p->m_strUserAlias = (const char*) strValues[2];
      p->Create(IDD_DIALOG_OPTIONS);
      //p->UpdateWindow();
      m_pviewuserdlg->ShowWindowEx();
    }else{
      m_pviewuserdlg->m_nUserID = uid;
      m_pviewuserdlg->m_nYYmeetingUserID =  m_pMgr->GetMEMgr()->GetID();
      m_pviewuserdlg->m_strUserinfodetail= (const char*)strValues[2];
      m_pviewuserdlg->ShowWindowEx();
    }
  }
  else if (cmd == 5){//em_createmeetDlg
    if (m_pcreatemeetdlg== NULL){
      COptionsExDlg * p =m_pcreatemeetdlg= new COptionsExDlg();
     
      p->m_pMgr= m_pMgr;
      p->m_emStyle= em_createmeetDlg;
      p->m_nUserID = uid;
      p->m_nYYmeetingUserID =  m_pMgr->GetMEMgr()->GetID();
      //p->m_strUserAlias = (const char*) strValues[2];
      p->Create(IDD_DIALOG_OPTIONS);
     // p->UpdateWindow();
       p->ShowWindowEx();
    }else{
      m_pcreatemeetdlg->ShowWindowEx();

      m_pcreatemeetdlg->ShowWindow(SW_SHOW);
    }
  }
   else if (cmd == 6 )//message box
  {
    PString&  strPrompt = strValues[2];
    if (m_pmessageboxdlg==NULL){
      
      COptionsExDlg * p = m_pmessageboxdlg= new COptionsExDlg();
      p->m_pMgr= m_pMgr;
      p->m_emStyle= em_MsgBox;
      p->m_nUserID = 0;
      p->m_nYYmeetingUserID =  m_pMgr->GetMEMgr()->GetID();
    }
    m_pmessageboxdlg->m_strMsgPrompt =(const char*) strPrompt;
    m_pmessageboxdlg->m_nMsgBoxStyle =uid;
    m_pmessageboxdlg->DoModal();
    strResult="123";
    

  } 
  else if (cmd == 7 )//get logined users from history xml 
  {
    if (strValues[2] =="0"){
      GetOptions(strValues, strResult);
    }else  if (strValues[2] =="1"){
      GetOptions(strValues, strResult);
    }
  }
  else if (cmd == 8 )//get logined users from history xml 
  {
    //GetOptions(strValues, strResult);
  } 
  else if (cmd == 11 )//em_welcomeDlg 
  {
    if (m_pmwelcomedlg== NULL){
      COptionsExDlg * p =m_pmwelcomedlg= new COptionsExDlg();
     
      p->m_pMgr= m_pMgr;
      p->m_emStyle= em_welcomeDlg;
      p->m_nUserID = uid;
      p->m_nYYmeetingUserID =  m_pMgr->GetMEMgr()->GetID();
      //p->m_strUserAlias = (const char*) strValues[2];
      p->Create(IDD_DIALOG_OPTIONS);
     // p->UpdateWindow();
       p->ShowWindowEx();
    }else{
      m_pmwelcomedlg->m_nUserID = uid;
      m_pmwelcomedlg->m_nYYmeetingUserID =  m_pMgr->GetMEMgr()->GetID();

#ifdef _DEBUG
      m_pmwelcomedlg->ShowWindowEx();
#endif
      if ( m_pmwelcomedlg->m_bLoadedXAML==false)
        m_pmwelcomedlg->ShowWindowEx();
      else
        m_pmwelcomedlg->ShowWindow(SW_SHOW);
    }
  }  // TODO: Add your dispatch handler code here
  else if (cmd== 12){//em_guideAudioDlg
    if (m_pguideAudioDlg== NULL){
      COptionsExDlg * p =m_pguideAudioDlg= new COptionsExDlg();
      
      p->m_pMgr= m_pMgr;
      p->m_emStyle= em_guideAudioDlg;
      p->m_nUserID = uid;
      p->m_nYYmeetingUserID =  m_pMgr->GetMEMgr()->GetID();
      //p->m_strUserAlias = (const char*) strValues[2];
      p->Create(IDD_DIALOG_OPTIONS);
      
     // p->UpdateWindow();
       p->ShowWindowEx();
    }else{
#ifdef _DEBUG
      m_pguideAudioDlg->ShowWindowEx();
#endif
      m_pguideAudioDlg->ShowWindow(SW_SHOW);
    }
  }
  else if (cmd == em_createfreegroupDlg){//em_createfreegroupDlg
    PString freegroupInfo;
    if (strValues.GetSize() >=3) {
      freegroupInfo = strValues[2];
    }
    if (m_pcreatefreegroupdlg== NULL){
      COptionsExDlg * p =m_pcreatefreegroupdlg= new COptionsExDlg();
     
      p->m_pMgr= m_pMgr;
      p->m_emStyle= em_createfreegroupDlg;
      p->m_nUserID = uid;
      p->m_nYYmeetingUserID =  m_pMgr->GetMEMgr()->GetID();
      p->m_strUserAlias = (const char*) freegroupInfo;
      p->Create(IDD_DIALOG_OPTIONS);
     // p->UpdateWindow();
       p->ShowWindowEx();
    }else{
      m_pcreatefreegroupdlg->m_strUserAlias = (const char*) freegroupInfo;
      m_pcreatefreegroupdlg->ShowWindowEx();

      m_pcreatefreegroupdlg->ShowWindow(SW_SHOW);
    }
  }
  else if (cmd == em_sipaccountDlg){//em_sipaccountDlg
    PString sipInfo ;
    if (strValues.GetSize() > 2) {
      //PString sipuser,sipdomain;
       
      sipInfo = m_pMgr->GetOPALMgr()-> SipGetInfo(strValues[2] );
    }
    if (m_plistonlinedlg== NULL){
      COptionsExDlg * p =m_plistonlinedlg= new COptionsExDlg();
     
      p->m_pMgr= m_pMgr;
      p->m_emStyle= em_listonlineDlg;
      //p->m_nUserID = uid;
      p->m_nYYmeetingUserID =  m_pMgr->GetMEMgr()->GetID();
      //p->m_strUserAlias = (const char*) strValues[2];
      p->Create(IDD_DIALOG_OPTIONS);
      p->m_strUserinfodetail =(const char*)sipInfo;
      p->m_emSubStyle = em_dispSipAccount;
      //p->UpdateWindow();
      m_plistonlinedlg->ShowWindowEx();
    }else{
      m_plistonlinedlg->m_emSubStyle = em_dispSipAccount;
      m_plistonlinedlg->m_strUserinfodetail =(const char*)sipInfo;
      m_plistonlinedlg->ShowWindowEx();
      m_plistonlinedlg->ShowWindow(SW_SHOW);
    }
  }
  else if (cmd == em_MCCNotifyDlg){//em_MCCNotifyDlg
    MapIMDialogs::iterator ifound = m_mapMCCNotifyDlgs.find(uid);
    
    if (ifound ==  m_mapMCCNotifyDlgs.end() ){   
      CWnd *pWnd =  FromHandle(m_hwndMainContainerPanel);
      COptionsExDlg *p = new COptionsExDlg(pWnd);
       p->m_pMgr= m_pMgr;
      p->m_emStyle= em_MCCNotifyDlg;
      p->m_nUserID = uid;
      p->m_nYYmeetingUserID =  m_pMgr->GetMEMgr()->GetID();

      p->Create(IDD_DIALOG_OPTIONS,pWnd );
      p->SetOwner(pWnd);
      p->ShowWindowEx();
      m_mapMCCNotifyDlgs[ uid]= p;
    }else{

    }
  }

  else if (cmd == em_msgfromcontainer )//em_msgfromcontainer 
  {
    
     int subcmd = uid;
     int nRemoteID =0;
     if (subcmd ==0 ){
      //set the save the HWND for local video panel 
       m_hwndLocalVideoPanel = (HWND) strValues[2].AsUnsigned();
     }else if (subcmd== 1){
     //get camera params
       std::list<PVideoFrameInfo> listVideoSizes;
       m_pMgr->GetOPALMgr()->GetVideoSizes(listVideoSizes);
       int nTmpIndex =0, tmpW,tmpH, j=0;
       m_pMgr->GetSettingXMLMgr()->GetLocalVideoSize(m_pMgr->GetMEMgr()->GetID(),tmpW,tmpH );
       for(std::list<PVideoFrameInfo>::iterator i =listVideoSizes.begin();i!=listVideoSizes.end();i++ ,j++){
         if (i->GetFrameWidth() == tmpW&& i->GetFrameHeight() == tmpH ){
            nTmpIndex = j;
         }
         CString str;str.Format("%u;%u,", i->GetFrameWidth(), i->GetFrameHeight());
         strResult+= str;
       }
       m_pMgr->m_nSelectSizeMenuIndex =nTmpIndex;
       CString strCurMediaInfo;strCurMediaInfo.Format("(%d|%d|%d)",nTmpIndex, m_pMgr->m_nSelectFPSMenuIndex,m_pMgr->m_nSelectBitrateMenuIndex );
       strResult+=strCurMediaInfo;
     }else if (subcmd ==2){// set wxh,bitrate 
        stMediaFormatInfo* pinfo = new stMediaFormatInfo();
        memset(pinfo, 0 ,sizeof(stMediaFormatInfo));
       stMediaFormatInfo& info= * pinfo;
       info.w = strValues[2].AsUnsigned();
       info.h = strValues[3].AsUnsigned();
       
      

       info.bitrate = strValues[4].AsUnsigned();
       m_pMgr->m_nSelectSizeMenuIndex =  strValues[5].AsUnsigned();
       m_pMgr->m_nSelectBitrateMenuIndex =  strValues[6].AsUnsigned();
       info.nRemoteID = nRemoteID;
    
      stThreadCommand * pCmd = new stThreadCommand();
      pCmd->cmd = en_threadSetMediaInfo;
      pCmd->tag =  (void*)pinfo;
      m_pMgr->GetMEMgr()->PostPTThreadCommand(pCmd);

       //m_pMgr->GetOPALMgr()->SetCurrectMediaInfobySessionID(info, "", 2, nRemoteID);
     }
     else if (subcmd ==3){//direct set bitrate
        stMediaFormatInfo* pinfo = new stMediaFormatInfo();
        memset(pinfo, 0 ,sizeof(stMediaFormatInfo));
       stMediaFormatInfo& info= * pinfo;   
       info.bitrate = strValues[2].AsUnsigned();
       m_pMgr->m_nSelectBitrateMenuIndex =  strValues[3].AsUnsigned();
       info.nRemoteID = nRemoteID;
    
      stThreadCommand * pCmd = new stThreadCommand();
      pCmd->cmd = en_threadSetMediaInfo;
      pCmd->tag =  (void*)pinfo;
      m_pMgr->GetMEMgr()->PostPTThreadCommand(pCmd);
      //m_pMgr->GetOPALMgr()->SetCurrectMediaInfobySessionID(info, "", 2, nRemoteID);
     }else if (subcmd ==4){//set FPS
       stMediaFormatInfo* pinfo = new stMediaFormatInfo();
       memset(pinfo, 0 ,sizeof(stMediaFormatInfo));
       stMediaFormatInfo& info= * pinfo;

       info.framerate = strValues[2].AsUnsigned();
       m_pMgr->m_nSelectFPSMenuIndex =  strValues[3].AsUnsigned();
       m_pMgr->GetOPALMgr()->SetCurrectMediaInfobySessionID(info,"", 2, nRemoteID);
     }else if (subcmd ==5){//set screencapture size
        CCatchScreenDlg dlg(0);
        m_wndCaptureDlg= &dlg ;
        dlg.DoModal();
        m_wndCaptureDlg=NULL;	  
        //m_pMgr->GetOPALMgr()->SetCameraName(C_SCREEN_CAPTURE_NAME);
        stMediaFormatInfo* pinfo = new stMediaFormatInfo();
        memset(pinfo, 0 ,sizeof(stMediaFormatInfo));
        stMediaFormatInfo& info= * pinfo;

        
        info.w =	dlg.m_RectScreenCapture.Width();
        info.h =	dlg.m_RectScreenCapture.Height();
        info.x = dlg.m_RectScreenCapture.left;
        info.y = dlg.m_RectScreenCapture.top;
        info.pVF =NULL;
        info.nRemoteID = nRemoteID;
    
        stThreadCommand * pCmd = new stThreadCommand();
        pCmd->cmd = en_threadSetMediaInfo;
        pCmd->tag =  (void*)pinfo;
        m_pMgr->GetMEMgr()->PostPTThreadCommand(pCmd);
      //  m_pMgr->GetOPALMgr()->SetCurrectMediaInfobySessionID(info, "", 2, nRemoteID);

       // if (m_bOrignalWin==true)
       //   SendMessage(   WM_VIDEOWND_DBCLICK,0,0);
     }else if (subcmd ==6){
       ::SendMessage(m_hWndEx, 1021+33/*YY_CREATE_CCVIDEOLAYOUT_WM*/, 0, 0);//
        g_VideoLayout = em_videoLayoutAdvanced;

     }
     else if (subcmd ==7){
      // ::SendMessage(m_hWndEx, 1021+33/*YY_CREATE_CCVIDEOLAYOUT_WM*/, 0, 0);//
      //  g_VideoLayout = em_videoLayoutAdvanced;
      HWND hwnd =(HWND) m_pMgr->GetOPALMgr()->GetRemoteVideoWindowHWNDbyRemoteUserID("0", true);
      ::PostMessage(hwnd, 1100/*WM_REMOTE_1*/, 1,0);

     }  
    else if (subcmd ==8){
      // show direct show video filter dialog
      if (strValues.GetSize()>2)
      {
         int nsubsubcmd= strValues[2].AsUnsigned();
         if (nsubsubcmd == 0)
           m_pMgr->GetOPALMgr()->ShowVideoFilterConfig();
         else if (nsubsubcmd == 1)
         {
           g_CanTransmitVideo=!g_CanTransmitVideo;
           m_pMgr->GetOPALMgr()->SetCameraName(  (const char*)m_pMgr->GetOPALMgr()->GetVideoInputDevice().deviceName );

         }
      }
     
      

   }else if (subcmd ==9){
     if ( m_pVideoPanelDlg&& m_pVideoPanelDlg->IsWindowVisible() && m_pVideoPanelDlg->IsMCCNormalLayout() ==false){
        m_pVideoPanelDlg->OnMenuItem( ID_Menu_Layout_normal);
        
      }
   }else if (subcmd ==10){
     m_hwndMainContainerPanel = (HWND) strValues[2].AsUnsigned();
   }
   else if (subcmd == 11 && m_pVideoPanelDlg ){
    unsigned int  nKeyCode = strValues[2].AsUnsigned();
    if ( nKeyCode == 1 ){
      m_pVideoPanelDlg->OnMenuItem(ID_Normal);
    }
    else if (nKeyCode == 2){
      m_pVideoPanelDlg->OnMenuItem(ID__2);
    }else if (nKeyCode == 3){
      m_pVideoPanelDlg->OnMenuItem(ID__4);
    }
    else if (nKeyCode == 4){
      m_pVideoPanelDlg->OnMenuItem(ID__6);
    }else if (nKeyCode == 5){
      m_pVideoPanelDlg->OnMenuItem(ID__12);
    }else if (nKeyCode == 0x53 - 0x30){//ctr+s sync

      m_pMgr->GetOPALMgr()->SyncVideoPanelLayout(m_pVideoPanelDlg ->GetLayoutStyle() );
      //m_pVideoPanelDlg->OnMenuItem(ID__12);
    }
   }
   else if (subcmd ==12)
   {
     m_pMgr->GetMEMgr()->m_strAutoTask = strValues[2];
     
     //m_pMgr->GetMEMgr()->CheckAutoDoTask();
     
   }
  }  // TODO: Add your dispatch handler code here
  else if (cmd == em_DTMF){
    m_pMgr->GetOPALMgr()-> SendUserInput( strValues[1]);
  }
  else if (cmd == em_controlmedia){
    bool bVideo=strValues[2].AsInteger() == 1 , bAudio =strValues[3].AsInteger() == 1 ;
     
    m_pMgr->GetOPALMgr()->SetVideoOrAudioTransmit(bVideo, bAudio);
  }
  else if (cmd == 100){
    int x=strValues[2].AsInteger(), y= strValues[3].AsInteger();
    ::PostMessage(this->GetHWNDMainContainer(), WM_NCLBUTTONDOWN,HTCAPTION,MAKELPARAM( x,y)); 
    
  }  
  return strResult.AllocSysString();
}

void CYYSpriteX2Ctrl::OnClose()
{
  COleControl::OnClose(3);
}
void CYYSpriteX2Ctrl::GetOptions(PStringArray& strValues, CString & retValue)
{
  //PStringArray strValues = PString(ctrlcode).Tokenise(";");
  //0 is  command,1 is subcommand
  int subCmd = strValues[2].AsInteger();
  switch (subCmd)
  {
  case 0://get login setting 
    {
    unsigned int uid = strValues[2].AsUnsigned();
   // PString pws;
    //int status= 0; bool bremenberpws= false, bautologin=false;
#if 0
    if (!m_pMgr->GetSettingXMLMgr()->GetLoginInfomation(uid, m_pMgr->GetMEMgr()->m_stLogin.pws,m_pMgr->GetMEMgr()->m_stLogin.status, m_pMgr->GetMEMgr()->m_stLogin.bremenberpws, m_pMgr->GetMEMgr()->m_stLogin.bautologin))
    {//default it
      m_pMgr->GetMEMgr()->m_stLogin.pws="";m_pMgr->GetMEMgr()->m_stLogin.status=0;m_pMgr->GetMEMgr()->m_stLogin.bremenberpws=0; m_pMgr->GetMEMgr()->m_stLogin.bautologin=0;
    }
    retValue.Format("%u;%s;%u;%u;%u", uid, (const char*)m_pMgr->GetMEMgr()->m_stLogin.GetRawPWS(),m_pMgr->GetMEMgr()->m_stLogin.status, (m_pMgr->GetMEMgr()->m_stLogin.bremenberpws?1:0), (m_pMgr->GetMEMgr()->m_stLogin.bautologin?1:0) );
#else
    PString strRet ;
    m_pMgr->GetSettingXMLMgr()->GetLoginInfomation(strRet,uid,m_pMgr->GetMEMgr()->m_stLogin.pws,m_pMgr->GetMEMgr()->m_stLogin.status, m_pMgr->GetMEMgr()->m_stLogin.bremenberpws, m_pMgr->GetMEMgr()->m_stLogin.bautologin);
    retValue = (const char*)strRet;
#endif
    }
    break;
   
  case 1://clear history
     m_pMgr->GetSettingXMLMgr()->ClearHistory();

    break;
  };
  
}
BOOL CYYSpriteX2Ctrl::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
  // TODO: Add your message handler code here and/or call default

  return COleControl::OnCopyData(pWnd, pCopyDataStruct);
}
#if 1
bool CYYSpriteX2Ctrl::AutoPrintDocument(unsigned int uid, const CString& strPathFile, int roomid)
{
   TCHAR*   sRoot= _T("SOFTWARE\\YYMeeting");
    HKEY hKey;
    LONG ret;
    DWORD dwSize=0;
   // 　RegCreateKeyEx(hCompanyKey,　_T("testreg"),　0,　REG_NONE,　EG_OPTION_NON_VOLATILE,　KEY_WRITE|KEY_READ,　NULL,&hAppKey,　&dw); 　
    ret = RegCreateKeyEx(HKEY_CURRENT_USER,sRoot,0,REG_NONE, REG_OPTION_NON_VOLATILE,KEY_WRITE|KEY_READ, NULL, &hKey, &dwSize);
  if(RegOpenKeyEx(HKEY_CURRENT_USER,sRoot,0,KEY_SET_VALUE,&hKey) == ERROR_SUCCESS){
    PString strPPTkey =PGloballyUniqueID().AsString();
    PString docPath = GetUserShareDocPath()+"\\"+ strPPTkey+"\\";
	    CString strValue;                     // 0                    1   2                        3                                  4
      strValue.Format("%s;%u;%s;%s\\%s\\;", D_ACTIVE_WIN_TITLE, uid, (const char*) strPPTkey, (const char*)GetUserShareDocPath(),(const char*) strPPTkey );
      PDirectory::Create(docPath);
      //TCHAR sPath[MAX_PATH]=_T("path");
	    dwSize=strValue.GetLength();
	    ret=RegSetValueEx(hKey,(const char*)"path",0,REG_SZ ,(BYTE*)strValue.GetBuffer(),dwSize);
	    if(ret==ERROR_SUCCESS)  {
        char szDefaultPrinter[256]={0};
        DWORD n=256;
        GetDefaultPrinter(szDefaultPrinter,& n);

        SetDefaultPrinter("YYPrinter");
        HINSTANCE ret= ShellExecute(GetDesktopWindow()->GetSafeHwnd(),   "print",   (const char*)strPathFile,NULL,NULL,SW_HIDE);
        stShareDocParam* pDoc= new stShareDocParam();
        stShareDocParam& stDoc= *pDoc;
        stDoc.m_strDefaultPrinter=szDefaultPrinter;
        stDoc.m_bMcc=false;stDoc.m_strDocFilePath = (const char*)strPathFile;stDoc.m_strDocGUID = strPPTkey;stDoc.m_uPeerid= uid;
        stDoc.roomid= roomid;

        m_pMgr->GetMEMgr()->ThreadShareDoc(stDoc);
       //
	  }
    RegCloseKey (hKey);
	  return true;
	}
  return false;
}


 
#else
bool CYYSpriteX2Ctrl::AutoPrintDocument(unsigned int uid, const CString& strPath){

CApplication objWord;

      // Convenient values declared as ColeVariants.
      COleVariant covTrue((short)TRUE),
                  covFalse((short)FALSE),
                  covOptional((long)DISP_E_PARAMNOTFOUND, VT_ERROR);

      // Get the IDispatch pointer and attach it to the objWord object.
      if (!objWord.CreateDispatch("Word.Application"))
      {
         AfxMessageBox("Couldn't get Word object.");
         return false;
      }

     // objWord.(SetVisible(TRUE);  //This shows the application.

      CDocuments docs(objWord.get_Documents());
      CDocument0 testDoc;

      
      testDoc.AttachDispatch(docs.Open(
                             COleVariant((const char*)strPath,VT_BSTR),
                             covFalse,    // Confirm Conversion.
                             covFalse,    // ReadOnly.
                             covFalse,    // AddToRecentFiles.
                             covOptional, // PasswordDocument.
                             covOptional, // PasswordTemplate.
                             covFalse,    // Revert.
                             covOptional, // WritePasswordDocument.
                             covOptional, // WritePasswordTemplate.
                             covOptional,  // Format. // Last argument for Word 97
                                covOptional, // Encoding // New for Word 2000/2002
                                covTrue,     // Visible
                                covOptional, // OpenConflictDocument
                                covOptional, // OpenAndRepair
                                (long)0,     // DocumentDirection wdDocumentDirection LeftToRight
                                covOptional  // NoEncodingDialog
                                )  // Close Open parameters
                                ); // Close AttachDispatch(…)

       AfxMessageBox("Now printing 2 copies on the active printer");

       testDoc.PrintOut(covFalse,              // Background.
                        covOptional,           // Append.
                        covOptional,           // Range.
                        covOptional,           // OutputFileName.
                        covOptional,           // From.
                        covOptional,           // To.
                        covOptional,           // Item.
                        COleVariant((long)2),  // Copies.
                        covOptional,           // Pages.
                        covOptional,           // PageType.
                        covOptional,           // PrintToFile.
                        covOptional,           // Collate.
                        covOptional,           // ActivePrinterMacGX.
                        covOptional,            // ManualDuplexPrint.
                        covOptional,           // PrintZoomColumn  New with Word 2002
                        covOptional,           // PrintZoomRow          ditto
                        covOptional,           // PrintZoomPaperWidth   ditto
                        covOptional);          // PrintZoomPaperHeight  ditto

       // If you wish to Print Preview the document rather than print it,
       // you can use the PrintPreview member function instead of the
       // PrintOut member function:
       //    testDoc[i].PrintPreview.

      objWord.Quit(covFalse,  // SaveChanges.
                   covTrue,   // OriginalFormat.
                   covFalse   // RouteDocument.
                   );

      return true;
}
#endif

HWND CYYSpriteX2Ctrl::GetVideoPanelHWND()
{
  if (m_pVideoPanelDlg) 
    return m_pVideoPanelDlg->GetSafeHwnd();
  else{
    return 0;
    //AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //if (m_pVideoPanelDlg==NULL)
    //{
    //  m_pVideoPanelDlg = new CVideoPanelDlg(0, this->m_pMgr);
    //  m_pVideoPanelDlg->Create(IDD_DIALOG_VIDEO_PANEL);
    //}
    //m_pVideoPanelDlg->ShowWindow(SW_SHOW);
    //return m_pVideoPanelDlg->GetSafeHwnd();
  }
}


LONG CYYSpriteX2Ctrl::OnNotifyEx(LONG CMD, LPCTSTR tag)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  // TODO: Add your dispatch handler code here

  return 0;
}
void CYYSpriteX2Ctrl::OnNotifyVideoPanel(unsigned int uid, int pos,  const char* createordestrop  )
{
  PString strNotify;
  strNotify.sprintf("%d;%d;%u;%d;%s;%s", (int)em_MCC_VideoPanelCreateOrDestroy, pos,uid, m_pMgr->GetMEMgr()->m_nCurectEnteredRoom ,(const char*) "GetCall().GetConfToken()", (const char*)createordestrop);
//   strNotify.sprintf("2;0;%u;%s",uid, (const char*) GetCall().GetConfToken());
	 OnNotify_WM(em_MCC, strNotify);
  
}
BOOL CYYSpriteX2Ctrl::PreTranslateMessage(MSG* pMsg)
{
  // TODO: Add your specialized code here and/or call the base class
  if(pMsg->message==WM_KEYDOWN&&pMsg->wParam==VK_ESCAPE)     
  {
    MessageBox("key");
     //OnMenuItem(ID_Menu_Layout_normal);
     // return TRUE;
  }else if (pMsg->message==WM_KEYUP)
    MessageBox("key up");
  return COleControl::PreTranslateMessage(pMsg);
}
