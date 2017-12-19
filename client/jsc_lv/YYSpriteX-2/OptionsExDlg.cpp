// OptionsExDlg.cpp : implementation file
//
#ifndef LPN_SDK

#include "stdafx.h"
#include <afxwin.h>

#include "YYSpriteX-2.h"
#include "YYSpriteX-2ctrl.h"
#include "OptionsExDlg.h"
#include "remotedlg.h"
#include ".\sqldb.h"
#include "ptlib.h"
#include "lpnmgr.h"
#include "myclient.h"
#include "main.h"
#include ".\optionsexdlg.h"
#include ".\videopaneldlg.h"
#include "yusertousertransmitdatamgr.h"
#include <ptclib/pxml.h>
#include "sfcallsmanager.h"
#include ".\sfvideownd.h"
extern PString  GetUserImgPath(unsigned int uid);
extern PString GetHttpListenMainURLFromContainer();
extern int SaveCaptureTojpegFile(const char* filename,HDC hDCmem,int width, int height, int quality);
extern int  g_VideoLayout  ;
extern PString g_GetAliasName(uint32 uid, bool bUTF8);
extern PString PlainTextToUTF8(const char* lpszContent);
#define D_DEFAULT_W   600
#define D_DEFAULT_H   540
// COptionsExDlg dialog
#define D_SETTING_OPTIONS "D_SAVE_SETTING_OPTIONS"
IMPLEMENT_DYNCREATE(COptionsExDlg, CDHtmlDialog)


#define ERASE_BKGND_BEGIN \
CRect bgRect;\
GetWindowRect(&bgRect);\
CRgn bgRgn;\
bgRgn.CreateRectRgnIndirect(bgRect);
//#define ERASE_BKGND_BEGIN 
// Marco parameter 'IDC' specifies the identifier of the control 
#define ADD_NOERASE_CONTROL(IDC)\
{\
　CRect controlRect;\
　GetDlgItem(IDC)->GetWindowRect(&controlRect);\
　CRgn controlRgn;\
　controlRgn.CreateRectRgnIndirect(controlRect);\
　if(bgRgn.CombineRgn(&bgRgn, &controlRgn, RGN_XOR)==ERROR)\
　　return  ;\
}

// Marco parameter 'noEraseRect' specifies a screen coordinates based RECT, 
// which needn't erase.
#define ADD_NOERASE_RECT(noEraseRect)\
{\
  CRgn noEraseRgn;\
  noEraseRgn.CreateRectRgnIndirect(noEraseRect);\
  if(bgRgn.CombineRgn(&bgRgn, &noEraseRgn, RGN_XOR)==0 )\
    return  ;\
}

// Marco parameter 'pDC' is a kind of (CDC *) type.
// Marco parameter 'clBrushColor' specifies the color to brush the area.
#define ERASE_BKGND_END(pDC, clBrushColor)\
CBrush brush;\
brush.CreateSolidBrush(clBrushColor);\
CPoint saveOrg = (pDC)->GetWindowOrg();\
(pDC)->SetWindowOrg(bgRect.TopLeft());\
(pDC)->FillRgn(&bgRgn, &brush);\
(pDC)->SetWindowOrg(saveOrg);\
brush.DeleteObject();\
//#define ERASE_BKGND_END

COptionsExDlg::COptionsExDlg(CWnd* pParent /*=NULL*/)
	: CDHtmlDialog(COptionsExDlg::IDD, COptionsExDlg::IDH, pParent),m_bXamlInit(false),m_pLocalVideo(0)
{
  m_strMeetingid=" ";m_bFlashWindow= FALSE;
   EnableAutomation();
	  RECT workrect;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workrect, 0);
	m_workwidth = workrect.right -  workrect.left;
	m_workheight = workrect.bottom - workrect.top;
  m_bLoadedXAML = false;
}

COptionsExDlg::~COptionsExDlg()
{

}

void COptionsExDlg::DoDataExchange(CDataExchange* pDX)
{
	CDHtmlDialog::DoDataExchange(pDX);
}

 

BEGIN_MESSAGE_MAP(COptionsExDlg, CDHtmlDialog)
  ON_WM_TIMER()
  ON_WM_CLOSE()
  ON_WM_SHOWWINDOW()
 // ON_WM_PAINT()
 // ON_WM_ERASEBKGND()
  ON_WM_SIZE()
  ON_BN_CLICKED(IDOK, &COptionsExDlg::OnBnClickedOk)
  ON_WM_CREATE()
  ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

BEGIN_DHTML_EVENT_MAP(COptionsExDlg)
	DHTML_EVENT_ONCLICK(_T("ButtonOK"), OnButtonOK)
	DHTML_EVENT_ONCLICK(_T("ButtonCancel"), OnButtonCancel)
END_DHTML_EVENT_MAP()



BEGIN_DISPATCH_MAP(COptionsExDlg, CDHtmlDialog)
   // DISP_FUNCTION(COptionsExDlg, "Func1", Func1, VT_EMPTY, VTS_NONE) 

    DISP_FUNCTION(COptionsExDlg, "OnSendJSMsg", OnSendJSMsg, VT_I4, VTS_BSTR)
   DISP_FUNCTION(COptionsExDlg, "OnSendJSMsgEx", OnSendJSMsgEx, VT_BSTR, VTS_BSTR) 
    // example:VTS_NONE
    // DISP_FUNCTION(CMyDHTMLDialog,"Func2",TestFunc,VT_BOOL,VTS_BSTR VTS_I4 VTS_I4)
    //                                                                        ^return,        ^parameters type list
    //每个方法都需要在这里添加映射
END_DISPATCH_MAP()
// COptionsExDlg message handlers
BOOL COptionsExDlg::CanAccessExternal()
{
 		return TRUE;
}
//CString GetAppPath()
//{
//	char szBuff[_MAX_PATH];
//	::GetModuleFileName(NULL, szBuff, _MAX_PATH);
//	char* pszFind = strrchr(szBuff, '\\');
//	if(pszFind!=0 && pszFind != szBuff) {
//		//*(pszFind-1) = '\0';
//		*pszFind = '\0';
//	}
//	return szBuff;
//}
//#define D_MAIN_HTML "C:\\Program Files\\YYMeeting\\index.html" 
enum em_Dialog_style
{
  D_POP_HTML,D_OPT_HTML,D_MSGBOX_HTML,D_LIST_ONLINE_HTML,D_VIEW_USER_HTML,D_CREATE_MEET_HTML, D_WELCOME_HTML, D_GUIDE_AUDIO,D_CREATE_CC_HTML,D_MCC_NOTIFY_HTML,
};
#if 0
//#define D_POP_HTML "http://127.0.0.1:5999/main/pop.html?userid=%u|%d|%s|%s|%s"//"D:\\chy\\projects\\my\\eqcores\\YYMeeting-pop\\Controls.Samples\\pop.html" 
//
//#define D_OPT_HTML "http://127.0.0.1:5999/main/popOption.html?type=%u" 
//#define D_MSGBOX_HTML "http://127.0.0.1:5999/main/popOption.html?type=%u|%u|%s" //1 par dlg style,2,subcommand ,3 prompt text 
//#define D_LIST_ONLINE_HTML "http://127.0.0.1:5999/main/popOption.html?type=%u" 
//#define D_VIEW_USER_HTML "http://127.0.0.1:5999/main/popOption.html?type=%u|%u|%u" // dlg style|userid 
//#define D_CREATE_MEET_HTML "http://127.0.0.1:5999/main/popOption.html?type=%u|%u|%s" //dlg style| meetingid 
#endif
 WORD  g_GetUserDefaultUILanguage()
 {
   return GetUserDefaultUILanguage();
 }
PString GetDialogHTTPURL(em_Dialog_style style)
{
  PString strResult;
  LANGID  id = GetUserDefaultUILanguage();
  PString strLang ;

  //if (id != 2052)
  //  strLang="_en";

  switch(style)
  {
  case D_POP_HTML:
    strResult=  GetHttpListenMainURL() +"yy/pop"+strLang+ ".html?userid=%u|%d|%s|%s|%s|%s";
    break;
   case D_OPT_HTML:
    strResult=  GetHttpListenMainURL() +"yy/popOption"+strLang+ ".html?type=%u";
    break;
 case D_MSGBOX_HTML:
    strResult=  GetHttpListenMainURL() +"yy/popOption"+strLang+ ".html?type=%u|%u|%s";
    break;
  case D_LIST_ONLINE_HTML:
    strResult=  GetHttpListenMainURL() +"yy/popOption"+strLang+ ".html?type=%u|%d|%s";
    break;
  case D_VIEW_USER_HTML:
    strResult=  GetHttpListenMainURL() +"yy/popOption"+strLang+ ".html?type=%u|%u|%u";
    break;
  case D_CREATE_MEET_HTML:
    strResult=  GetHttpListenMainURL() +"yy/popOption"+strLang+ ".html?type=%u|%u|%s";
    break;
  case D_WELCOME_HTML:
    strResult=  GetHttpListenMainURL() +"yy/popOption"+strLang+ ".html?type=%u|";
    break;
  case D_GUIDE_AUDIO:
    strResult=  GetHttpListenMainURL() +"yy/popOption"+strLang+ ".html?type=%u|%u|%u|%s|%s";
    break;
  case D_CREATE_CC_HTML:
    strResult=  GetHttpListenMainURL() +"yy/popOption"+strLang+ ".html?type=%u|%u|%s |%s";
    break;
  case D_MCC_NOTIFY_HTML:
    strResult=  GetHttpListenMainURL() +"yy/popOption"+strLang+ ".html?type=%u|%s";
    break;
  };
  return strResult;
};

int COptionsExDlg::OnSendJSMsg(LPCTSTR p )
{
 
  //char *pSplit = strstr(p,";");
  //if (pSplit==NULL ) return 0;
  //char cmd[24]={0};
  //memcpy(cmd,p, pSplit- p);
  //CString strBody(&(pSplit[1]));
  CString strResult;
  CString strBody;
  char *pSplit = (char*)strstr(p,";");
  char cmd[24]={0};
  if (pSplit==NULL )  {
    memcpy(cmd,p,  strlen(p));
    strBody="";
  }
  else{
    memcpy(cmd,p, pSplit- p);
  strBody=(&(pSplit[1])); 
  }

  try{
    
    
    // we trust all com object (haha, you can make virus)
    switch(atoi(cmd))
    {
    case em_SaveLocalOptionsInfo:
      {
        SaveSetting(strBody);
      }
      break;
     case em_IMMessage_received://pop send a IM
      {
        m_pMgr->GetSpriteX2Ctrl()->SendOfflineIM(strBody);// (strBody);
         PTRACE(3, "strBody="  << (const char*)strBody);
      }
      break; 
     case em_pop_GetUserID:
      {
          return m_nUserID;
      }
      break;   
    case em_pop_GetYYMeetingUserID:
      {//get yymeetingID from IM Dialog
        return m_pMgr->GetMEMgr()->GetID();
      }
    };

     
  }
  catch(...){
  }
  return 1;
}
BSTR COptionsExDlg::OnSendJSMsgEx(LPCTSTR p )
{
  CString strResult;
  CString strBody;
  char *pSplit =(char*) strstr(p,";");
  char cmd[24]={0};
  if (pSplit==NULL )  {
    memcpy(cmd,p,  strlen(p));
    strBody="";
  }
  else{
    memcpy(cmd,p, pSplit- p);
  strBody=(&(pSplit[1])); 
  }
  try{
    
    
    // we trust all com object (haha, you can make virus)
    switch(atoi(cmd))
    {
    case em_SaveLocalOptionsInfo:
      {
        SaveSetting(strBody);
      }
      break;
    case em_meeting_ctrl2:
      {
        m_pMgr->GetMEMgr()->SendMeetingCtrl(1, strBody);
      }
      break;    
    case em_mouse_down:
      {
        PStringArray arr = PString((const char*)strBody.GetBuffer()).Tokenise(";");
        if (arr.GetSize()==2 ){
             int x=arr[0].AsInteger(), y= arr[1].AsInteger();
             ::PostMessage( this->GetParent()->GetSafeHwnd(), WM_NCLBUTTONDOWN,HTCAPTION,MAKELPARAM( x,y)); 
        }else if (arr.GetSize()==3 ){
             int x=arr[0].AsInteger(), y= arr[1].AsInteger();
             ::PostMessage(  GetSafeHwnd(), WM_NCLBUTTONDOWN,HTCAPTION,MAKELPARAM( x,y)); 
        }
      }
      break;    
    case em_SipCommand:
      {
		 PAssertAlways("sip forbiden");
        m_pMgr->GetOPALMgr()->SipCommandFromSL(strBody);
      }
      break;   
    case em_DlgWelcomeMsg:
      {
        PStringArray arr = PString(strBody.GetBuffer()).Tokenise(";");
        int subcmd = arr[0].AsInteger();
        if (subcmd ==0 ){
          //view user detail info
          PString strValue; strValue.sprintf("4;%u", m_pMgr->GetMEMgr()->GetID());
          m_pMgr->GetSpriteX2Ctrl()->IMDlgControl(strValue);
        }else  if (subcmd ==1 ){
          //tuning wizard
        }else  if (subcmd ==2 ){
          //save if display 
          g_SetSettingString("welcome_not_show", arr[1] );
        }else  if (subcmd ==3 ){
          //get the display 
          strResult = g_GetSettingString("welcome_not_show", "0");
        }
      }
      break;
    case em_LoadLocalOptionsInfo:
      {
        strResult= LoadSetting();
      }
      break ;
    case em_SendvideoCall:
      {
        PString strUID(m_nUserID);
        if (m_pMgr->GetOPALMgr()->IsInCallSession(strUID ) ==false){
          m_pMgr->GetSpriteX2Ctrl()->OnNotify(em_SendvideoCall,strUID);// (strBody);
          strResult ="1";
        }
        else
          strResult="0";
      }break;
    case em_SendAudiocall:
      {         
        PString strUID(m_nUserID);
        if (m_pMgr->GetOPALMgr()->IsInCallSession(strUID ) ==false){//ok to continue
          m_pMgr->GetSpriteX2Ctrl()->OnNotify(em_SendAudiocall,strUID);// (strBody);
          strResult ="1";
        }
        else
          strResult="0";
      }break;
    case em_SendPPT:
    case em_SendFile:
    case em_SendWhiteBoard :
    case em_SendDesktop:
    case em_SendPicture:
      {
        strResult = m_pMgr->GetSpriteX2Ctrl()->P2PAFile(strBody);
      }
      break ; 
    case em_ySaveSheduleMeet:
      {
        
        if (m_pMgr->GetSpriteX2Ctrl()->SheduleMeet(strBody)){
        }
      }
      break ;    
    case em_yGetFriends:
      {
        return m_pMgr->GetSpriteX2Ctrl()->GetFriendsList("3");//3: uid and alias 
      }
      break ; 
    case em_yGetCUGContacts:
      {
       return  m_pMgr->GetSpriteX2Ctrl()->GetTree(false);
      }
      break ; 
    case em_ySaveUseinfo://update user info to server
      {
        m_pMgr->GetSpriteX2Ctrl()->UpdateUserData(strBody);
      }
      break ;      
    case em_yLoadUseinfo://update user info to server
      {
        strResult = m_pMgr->GetSpriteX2Ctrl()->ViewUserDataDirect((const char*)strBody);
      }
      break ;
    case em_yListOnline://update user info to server
      {
        strResult = m_pMgr->GetSpriteX2Ctrl()->ListOnlineUser();
      }
      break ; 
    case em_CallClearFromPop://update user info to server
      {
        if (m_nUserID> D_ROOM_ID_UPPERLIMIT)
          m_pMgr->GetOPALMgr()->HangupCallSafe(m_nUserID);
        else
          m_pMgr->GetOPALMgr()->HangupCallSafe(0);
      }
      break ;
    case em_SwitchToVideoPanel://update user info to server
      {
#if 0
        this->ShowWindow(SW_SHOWMINIMIZED);
        PString strURL;strURL.sprintf("z:%u", D_CLIENT_MCU_ID);
        LONG ret = m_pMgr->GetSpriteX2Ctrl()->MakeCall(strURL,  m_pMgr->GetMEMgr()->GetMeetingIDByRoomID(m_nUserID) ) ;
        if (ret !=0 &&  m_pMgr->GetSpriteX2Ctrl()->m_pVideoPanelDlg){
          //g_nl
          // m_pMgr->GetSpriteX2Ctrl()->m_pVideoPanelDlg->ShowWindow(SW_SHOW);
          m_pMgr->GetSpriteX2Ctrl()->m_pVideoPanelDlg->SwitchToTheVideoAdcanceWindow();
         
        }
#else
        /*  case  ID__2:  case  ID__4:
  case  ID__6:
  case  ID__9:
  case  ID__12:
  case  ID__16:
  case  ID__32:
  case ID_Normal*/
        if (m_pMgr->GetSpriteX2Ctrl()->m_pVideoPanelDlg) {
           
            m_pMgr->GetSpriteX2Ctrl()->m_pVideoPanelDlg->OnMenuItem(atoi(strBody) );
        }
    
#endif   
      }
      break ; 
    case em_yAddFriend:// 
      {
        unsigned uidadd = PString( strBody).AsUnsigned();
        m_pMgr->GetMEMgr()->AddFriend( uidadd, "hello!");

        m_pMgr->GetSpriteX2Ctrl()->OnNotify_WM(em_UserLoadFriend, "");

        stViewUserParam* pParam = new stViewUserParam(1, true);
        PIntArray* data = pParam->data ;//new PIntArray(list.size());
        (*data)[0]=  uidadd;
		     m_pMgr->GetMEMgr()->ThreadViewUserData(*pParam);
	           
      }
      break ;     
    case em_yViewUserINFO:// 
      {
        PStringArray arr = PString( strBody).Tokenise(",");
        stViewUserParam* pParam = new stViewUserParam(arr.GetSize(), false);
        PIntArray* data = pParam->data ;//new PIntArray(list.size());
        for (int i=0;i< arr.GetSize();i++){
          (*data)[i]=  arr[i].AsUnsigned();
        }
		     m_pMgr->GetMEMgr()->ThreadViewUserData(*pParam);
	           
      }
      break ;     
    case em_yManageRoomINFO:// 
      {
        PStringArray arr = PString( strBody).Tokenise(",");
        stViewUserParam* pParam = new stViewUserParam(arr.GetSize(), false);
        PIntArray* data = pParam->data ;//new PIntArray(list.size());
        for (int i=0;i< arr.GetSize();i++){
          (*data)[i]=  arr[i].AsUnsigned();
        }
		     m_pMgr->GetMEMgr()->ThreadViewUserData(*pParam);
	           
      }
      break ;    
    case em_clipboard:// 
      {
        //PStringArray arr = PString( strBody).Tokenise();
        int nsubCmd = atoi(strBody.Left(1));

        if (nsubCmd==0){//ctrl+c
          if(OpenClipboard())
          {
	          HGLOBAL clipbuffer;
	          char * buffer;
	          EmptyClipboard();
	          clipbuffer = GlobalAlloc(GMEM_DDESHARE, strBody.GetLength()+1);
	          buffer = (char*)GlobalLock(clipbuffer);
	          strcpy(buffer, ((const char*)strBody)+1);
	          GlobalUnlock(clipbuffer);
	          SetClipboardData(CF_TEXT,clipbuffer);
	          CloseClipboard();
          }

        }else if (nsubCmd==1){//ctrl+v
          if ( OpenClipboard() )
          {
            bool bClipText= false;
            CString strContexnt;
            //if ( OpenClipboard() ) 
            {
	            HANDLE hData = GetClipboardData( CF_TEXT );
              if (hData!=NULL){
	              char * buffer = (char*)GlobalLock( hData );
	              strContexnt = buffer;
	              GlobalUnlock( hData );
	              CloseClipboard();
                bClipText =true;
                strResult="0"+ strContexnt;
                //EmptyClipboard();

              }
            }
            if (bClipText ==false){
	            //Get the clipboard data
	            HBITMAP handle = (HBITMAP)GetClipboardData(CF_BITMAP);
              if (handle!=NULL){
	              CBitmap * bm = CBitmap::FromHandle(handle);
                BITMAP   bmp; 
                //CBitmap   m_bitmap; 
                bm->GetBitmap(&bmp); 
                long   nWidth=bmp.bmWidth; 
                long   nHeight=bmp.bmHeight;
                PString strSave; 
                PString dstFileNameAndext =  PGloballyUniqueID().AsString()+ PString(".jpg") ;
                strSave.sprintf("%s\\%s ", (const char*)GetUserImgPath(m_pMgr->GetMEMgr()->GetID()),(const char*)dstFileNameAndext);
                {
                  CClientDC cdc(this);
	                CDC dc;
	                dc.CreateCompatibleDC(&cdc);
	                dc.SelectObject(bm);
                    
	                //cdc.BitBlt(0,0,nWidth,nHeight,&dc,0,0,SRCCOPY);
                  
                  SaveCaptureTojpegFile(strSave, dc,nWidth, nHeight, 80);
                  //DeleteDC(hMemDC)
                //  this->d();
                }
                 
                if ( PFile::Exists(strSave )){
                  //CString strFilePath = dlg.GetFilePath();
                  PString str;str.sprintf("main/%u/pic/%s",m_pMgr->GetMEMgr()->GetID(),  (const char*)dstFileNameAndext );
                  //if (g_AddHTTPFileResource(str, strSave))
                  {
                    strContexnt.Format( (const char*)(GetHttpListenMainURL()+"%s"), (const char*)str);
                    strResult="1"+ strContexnt;
                  }
                }
              }
            }
	          CloseClipboard();

          }
        }

	           
      }
      break ;       
    case em_CloseMe:
      {
        ShowWindow(SW_HIDE);
        PostMessage(WM_CLOSE);
        //PostMessage(WM_DESTROY);
       // SendMessage(WM_CLOSE);
       // OnCancel();
        //DestroyWindow();
      }
      break ; 
   case em_yAddorEditSIPAccount:
      {
       /* PString user ;
        PString domain ;
        PString password ;
        PString display ;
        PString auth ,sipcallee;*/
        PXML xml;
        strResult ="0";
        if (!xml.Load((const char*)strBody))
        {
          PTRACE(3, "failed to load  the SIPAccounts ." );
        }else{
          //check root
          PXMLElement* root = xml.GetRootElement();
          strResult ="0";
          if (root)
          {
            //root = new PXMLElement(NULL, "root");
          /// m_config.SetRootElement(root);
            stAppNode info;
            PXMLElement* p= root->GetElement("f");
            if (p!=NULL){
              info.key = p->GetAttribute("key");
              if (info.key.GetLength()<=0 )
                info.key = PGloballyUniqueID().AsString();
              info.user = p->GetAttribute("user");
              info.domain = p->GetAttribute("domain");
              info.display = p->GetAttribute("display");
              info.auth = p->GetAttribute("auth");
              info.password = p->GetAttribute("password");
              info.sipcallee = p->GetAttribute("sipcallee");
			  info.proxy = p->GetAttribute("proxy");
              strResult= m_pMgr->GetOPALMgr()->AddOrModifySipAccount(info )?"1":"0";

            }
        }
        }


      }
      break ;     
   case em_yEnterMeeting:
      {
        int nRoomid = atoi(strBody);
        if (nRoomid>0 ){
          BBQ_MeetingInfo info={0};
          if (m_pMgr->GetMEMgr()->GetMeetingInforByRoomID(nRoomid, info)){
            char szMake[128]={0};
            sprintf(szMake, "z:%d", info.nMcuId); 
            m_pMgr->GetSpriteX2Ctrl()->MakeCall(szMake,info.strMeetingId);
          }

        }
      }
      break ;    
    case em_ySendFileOpenfolder:
      {
        ShellExecute(NULL,NULL,_T("explorer"), "/select, "+strBody,NULL,SW_SHOW); 

      }
      break ;      
    case em_popOptionsDlg:
      {
        m_pMgr->GetSpriteX2Ctrl()->IMDlgControl("2;0");
      }
      break ;
    case em_yGetIMHistory:
      {
        PString strIM;
        PStringArray arrParams= PString((const char*)strBody).Tokenise(";");
        if (arrParams.GetSize() >=3){
          int nStart=arrParams[0].AsInteger(),nEnd=arrParams[1].AsInteger(), nUDWanted=arrParams[2].AsInteger();
          //strResult =""
          if ( m_pMgr->GetDBMgr()->GetIM(nStart, nEnd,nUDWanted, strIM) ){
            strResult.Format("0%s", (const char*) strIM);
          }else{
            strResult.Format("1");
          }
         
        }
      }
      break ;    
    case em_popRequestSpeak:
      {
        //MCU控制，发给主席，然后主席同意后，mcu就把申请人的声音播放给所有成员
       // m_pMgr->GetSpriteX2Ctrl()->IMDlgControl("2;0")  ;
        m_pMgr->GetMEMgr()->Mcc_Req_speaker( this->m_nUserID,strBody );
      }
      break ;    
     case em_popResponseSpeak:
     {
        //MCU控制，发给主席，然后主席同意后，mcu就把申请人的声音播放给所有成员
        PStringArray arr= PString((const char*)strBody).Tokenise(";");
        if (arr.GetSize() >=2)
          m_pMgr->GetMEMgr()->Mcc_Res_speaker(arr[0].AsInteger(), this->m_nUserID, "response", arr[1].AsInteger() );
      }
      break ;     
     case em_popRecording:
     {
       //MCU控制，发给主席，然后主席同意后，mcu就把申请人的声音播放给所有成员
       PStringArray arr= PString((const char*)strBody).Tokenise(";");
       bool bRecord =(arr[0].AsInteger() == 1);
       strResult=(const char*) m_pMgr->GetOPALMgr()->StartOrStopRecordMediabyCallID( bRecord ,true);
      }
      break ; 
     case em_yPopSaveIMSetting:
     {
       stIMSetting setting;
       setting.FromString(PString((const char*) strBody) );
       m_pMgr->GetSettingXMLMgr()->SaveIMSettingInfomation(m_nYYmeetingUserID, setting);
     }
      break ;      
     case em_popRequestMCUMeeting:
     {
        //MCU控制，发给主席，然后主席同意后，mcu就把申请人的声音播放给所有成员
        //"z:" + nCalleeID.ToString(), info.strMeetingId 
       PStringArray arr= PString((const char*)strBody).Tokenise(";");
       if (arr[0] == "1"){
          PString strURL;strURL.sprintf("z:%u", D_CLIENT_MCU_ID);
          m_pMgr->GetSpriteX2Ctrl()->MakeCall(strURL,m_pMgr->GetMEMgr()->GetMeetingIDByRoomID(m_nUserID));
       }else if (arr[0] == "0"){
         m_pMgr->GetOPALMgr()->HangupCallSafe();
       }else if (arr[0] == "2"){
         m_pMgr->GetMEMgr()->UpdateSheduleMeet(m_nUserID,  &((const char*)strBody)[2]);
       }else if (arr[0] == "5"){
         this->OnCancel();
       }else if (arr[0] == "10"){
          //m_pMgr->GetOPALMgr()->SetVideoOrAudioTransmit(true, true);
         m_pMgr->GetOPALMgr()->SetCameraNameEx (arr[1] );
        /* if (m_pMgr->GetOPALMgr()->m_gSourceStream && m_pMgr->GetOPALMgr()->m_gSourceStream->IsOpen() ==false ){
              m_pMgr->GetOPALMgr()->m_gSourceStream->Open();
            }*/
       }else if (arr[0] == "3"){
        if ( this->GetParent())
        {
          this->GetParent()-> ShowWindow(SW_SHOWMINIMIZED);
        }
       }
        else if (arr[0] == "4"){//maxnize
          if ( this->GetParent())
          {
            ((CVideoPanelDlg* )GetParent())-> SetWindowDefaultSize(); 
          }
       }     
     }
      break ; 
     case em_yLayoutExtendToRight:
     {
       if (m_pMgr->GetSpriteX2Ctrl()->m_pVideoPanelDlg  ){
         m_pMgr->GetSpriteX2Ctrl()->m_pVideoPanelDlg->SetExtendtoRight( strBody=="1" );
       }
     }
      break ;      
     case em_popTuningDlg:
     {
        //MCU控制，发给主席，然后主席同意后，mcu就把申请人的声音播放给所有成员
        //"z:" + nCalleeID.ToString(), info.strMeetingId 
       PStringArray arr= PString((const char*)strBody).Tokenise(";");
       int subcmd= arr[0].AsInteger();
       if (subcmd == 1){
         //display tuning dlg
          m_pMgr->GetSpriteX2Ctrl()->IMDlgControl("12;0");
           
       }else if (subcmd== 2){
         //test record
         PString &strRecord =arr[1], &strPlay=arr[2];
         m_pMgr->GetOPALMgr()->StartSoundTest(true, strRecord, strPlay);
       }
       else if (subcmd== 3){
         //test play
        PString &strRecord =arr[1], &strPlay=arr[2];
         m_pMgr->GetOPALMgr()->StartSoundTest(false, strRecord, strPlay);
       }
       else if (subcmd== 4){
         //test play
        PString &strRecord =arr[1], &strPlay=arr[2];
         m_pMgr->GetOPALMgr()->StopSoundTest( true, strRecord,strPlay);
       }  
       else if (subcmd== 5){
         //set microphone valume
         
        m_pMgr->GetOPALMgr()->SetMicrophoneVolume( arr[1].AsUnsigned());
       }  
       else if (subcmd== 6){
         //set speaker valume
         
        m_pMgr->GetOPALMgr()->SetSpeakerVolume( arr[1].AsUnsigned());
       }  
     }
      break ;
     case em_xamlInitOK:
      {

        //CloseWindow();
        //SendMessage(WM_CLOSE);
        //OnCancel();
        PTRACE(5, "DLG::\t      case em_xamlInitOK: m_emStyle=" << m_emStyle);
        if (m_emStyle == em_popIM){
          stIMSetting setting;
          m_pMgr->GetSettingXMLMgr()->LoadIMSettingInfomation(m_nYYmeetingUserID, setting);
          strResult= (const char*)setting.ToString();
          if (m_nUserID < D_ROOM_ID_UPPERLIMIT){
            SIMD_CS_CONFERENCE in={0};
            in.action = em_conf_enter;
            in.ActionUId= m_nYYmeetingUserID/*m_nUserID*/;// i am came in to notify
            in.roomid= m_nUserID;
             
            m_pMgr->GetMEMgr()->NotifyServerConferenceChange(&in);
            BBQ_MeetingInfo info={0};
            m_pMgr->GetMEMgr()->GetMeetingInforByRoomID(m_nUserID, info);
            //notify member's info to mcc panel
            stViewUserParam *pParam= new stViewUserParam(info.nIdCount,false, 3000);
            for(int i=0;i<info.nIdCount ;i++){
              (*pParam->data)[i] = info.localIdList[i];
            }
            m_pMgr->GetMEMgr()->ThreadViewUserData(*pParam);            
           //ShowWindow(SW_SHOWMAXIMIZED);
          }else if (m_nUserID>= D_ROOM_ID_UPPERLIMIT && m_nUserID< D_FREEGROUP_ID_UPPERLIMIT){//in freegroup pop im dlg
       //     std::list<unsigned int> lst ;
       //     m_pMgr->GetSpriteX2Ctrl()->GetTreeSubTreeIDs( m_nUserID, lst);

       ///*     for( std::list<unsigned int>::iterator iter= lst.begin();iter!= lst.end() ;iter++){
       //       if (*iter == m_nUserID) {
       //         lst.erase(iter);
       //         break;
       //       }
       //     }*/ 
       //     stViewUserParam *pParam= new stViewUserParam(lst.size() ,false, 5000);
       //     int i=0;
       //     for( std::list<unsigned int>::iterator iter= lst.begin();iter!= lst.end() ;iter++){
       //       (*pParam->data)[i++] = *iter;
       //     }
       //     m_pMgr->GetMEMgr()->ThreadViewUserData(*pParam);   
            ShowWindow(SW_SHOW);
          }
          else{
            ShowWindow(SW_SHOW);
          }
          
          stViewUserParam *pParam= new stViewUserParam(2,false, 3000);
          (*pParam->data)[0] = m_nUserID;(*pParam->data)[1] = m_nYYmeetingUserID;
          m_pMgr->GetMEMgr()->ThreadViewUserData(*pParam);

        }else{
          ShowWindow(SW_SHOW);
        }
        m_bXamlInit =true;
      }
      break ; 
    };

    
     
  }
  catch(...){
  }
  return strResult.AllocSysString();
}
void COptionsExDlg::OnNavigateComplete(LPDISPATCH pDisp, LPCTSTR szUrl)
{
	CDHtmlDialog::OnNavigateComplete(pDisp, szUrl);
    PTRACE(5, "DLG::\t  em_viewuserDlg: OnNavigateComplete beging 1 m_emStyle=" << m_emStyle   );
	CString strUrl = CString(szUrl).MakeLower();
	if ( strUrl.Find("pop.html")>=0){
		//this->SetWindowPos(NULL, 0, 0,  560,504, SWP_NOMOVE);
		CString param;
		param.Format("%d;%u;%s%u;", (int)em_SetUserInfo, m_nUserID, m_strUserAlias , m_nYYmeetingUserID);
   //  ShowWindow(SW_SHOW);
		CStringArray params;
		// strParam.Format("%d;%d", /*em_sizeSLLeftPanelWindow,*/ 0, 0);
		params.Add(param);
		CallJScript("yCallFromPop",params);
 
  }else if (strUrl.Find("index.html")>=0 ){
      ;//ShowWindow(SW_SHOW);

  }
  PTRACE(5, "DLG::\t  em_viewuserDlg: OnNavigateComplete beging 2 m_emStyle=" << m_emStyle   );
}
void COptionsExDlg::OnDocumentComplete(LPDISPATCH pDisp, LPCTSTR szUrl)
{
	CDHtmlDialog::OnDocumentComplete(pDisp, szUrl);
  PTRACE(5, "DLG::\t  em_viewuserDlg: OnDocumentComplete beging 2 m_emStyle=" << m_emStyle   );
}

HRESULT COptionsExDlg::OnButtonOK(IHTMLElement* /*pElement*/)
{
	OnOK();
	return S_OK;  // return TRUE  unless you set the focus to a control
}

HRESULT COptionsExDlg::OnButtonCancel(IHTMLElement* /*pElement*/)
{
	OnCancel();
	return S_OK;  // return TRUE  unless you set the focus to a control
}
PString BASE64Encode(const char* data)
{
  unsigned char out[1024]={0};
  PKI_encode((const unsigned char*)data, strlen(data), out);
  return  (const char*)out;
}
PString BASE64Encode(const char* data, int nlen)
{
  unsigned char out[2048]={0};
  PKI_encode((const unsigned char*)data, nlen, out);
  return  (const char*)out;
} 

BOOL COptionsExDlg::OnInitDialog()
{
   
	DWORD   dwStyle   =   GetWindowLong(m_hWnd,GWL_STYLE); 
  CRect rc;
  GetWindowRect(rc);
  m_winW=rc.Width();
  m_winH=rc.Height();
//if(dwStyle   &&   WS_CHILD) 
//{ 
//dwStyle   &=   ~WS_CHILD;//   去掉WS_CHILD风格 
//dwStyle   |=   WS_POPUP;//   设置WS_POPUP风格 
//} 
//else 
{ 
//dwStyle   &=   ~WS_POPUP; 
//dwStyle   |=   WS_CHILD; 
} 
//SetWindowLong(m_hWnd,GWL_STYLE,dwStyle);
    
  //GetDlgItem( IDOK)->ShowWindow(SW_HIDE);
	CDHtmlDialog::OnInitDialog();
   PTRACE(5, "DLG::\t  em_viewuserDlg: OnInitDialog beging 1 m_emStyle=" << m_emStyle   );
  m_emSubStyle = em_dispListOnline;
  int ret=0;
  if (m_emStyle ==em_popIM ){
    if (m_nUserID < D_ROOM_ID_UPPERLIMIT){
      ret=   ModifyStyle(WS_CAPTION|WS_EX_APPWINDOW, 0    );
      ModifyStyleEx(WS_EX_APPWINDOW,WS_EX_TOOLWINDOW);
      //  ret=   ModifyStyle(WS_CAPTION|WS_EX_APPWINDOW, WS_CHILD |WS_EX_TOOLWINDOW  );
     /// ShowWindowEx();
    // ret =ModifyStyleEx(WS_POPUP|WS_DLGFRAME|WS_CAPTION, WS_CHILD );
    }//else p2p IM
    
  }else{
     //ret=   ModifyStyle(WS_CHILD, WS_POPUP );
  }
 // m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
 // SetIcon(m_hIcon, FALSE);
   // SetTimer(2, 2000, NULL) ;                   // 启动窗口flash定时器

///////////////////////////////////////////////////////////
//WNDCLASS wc;

// Get the info for this class.
// #32770 is the default class name for dialogs boxes.
//::GetClassInfo(AfxGetInstanceHandle(), "#32770", &wc);
//
//// Change the name of the class.
//wc.lpszClassName = "YYContainer"; //这里请再次注意，一定要保证和rc资源文件里保存的类名相同！
//
//// Register this class so that MFC can use it.
//BOOL d = AfxRegisterClass(&wc); 
	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	//SetIcon(m_hIcon, TRUE);			// 设置大图标
	//SetIcon(m_hIcon, FALSE);		// 设置小图标
  SetExternalDispatch(GetIDispatch(TRUE));    //将浏览器控件的扩展接口设置为对话框自身的IDispatch
  //if ( m_emStyle == em_optDlg)
  //  ShowWindowEx();

  if (m_emStyle ==em_MsgBox) {
    PString strUrl;
    strUrl.sprintf((const char*)GetDialogHTTPURL(D_MSGBOX_HTML),(int)em_MsgBox, m_nMsgBoxStyle,(const char*) m_strMsgPrompt );
    this->SetWindowPos(NULL, 0, 0,  400,300,/*D_DEFAULT_W,D_DEFAULT_H, */SWP_NOMOVE);
    this->Navigate(strUrl);
  }else if (m_emStyle == em_optDlg){
     g_VideoLayout =em_videoLayoutSimpleInsilverlight;
    
  }
 // SetOwner(GetDesktopWindow());
    PTRACE(5, "DLG::\t  em_viewuserDlg: OnInitDialog beging 2 m_emStyle=" << m_emStyle   );
 	return TRUE;  // 除非设置了控件的焦点，否则返回 TRUE
}
void COptionsExDlg::ShowWindowEx()
{
  m_bLoadedXAML= true;
  if (m_emStyle == em_popIM){
    m_nUserID=  m_pMgr->GetMEMgr()-> m_nCurectEnteredRoom;
    //  SetWindowPos(&wndNoTopMost,   0,   0,   0,   0,   SWP_NOSIZE   |   SWP_NOMOVE); 
    PString strUrl, strMeetinginfo, strTag;
    int bInVideoRoom=0;
    if ( m_nUserID< D_ROOM_ID_UPPERLIMIT){
      BBQ_MeetingInfo info={0};
      if (m_pMgr->GetMEMgr()->GetMeetingInforByRoomID(m_nUserID, info)){
        strMeetinginfo = "";//BASE64Encode((const char*) &info, sizeof(info) );
      }
      bInVideoRoom =m_pMgr->GetOPALMgr()-> IsInMCCVideoRoom()?1:0;
      strTag.sprintf("%u,%u", m_pMgr->GetOPALMgr()->GetMicrophoneVolume(),  m_pMgr->GetOPALMgr()->GetSpeakerVolume() );
    }
    strUrl.sprintf( 
      (const char*)GetDialogHTTPURL(D_POP_HTML), 
      m_nUserID, bInVideoRoom,
      (const char*)BASE64Encode((const char*)PlainTextToUTF8(m_strUserAlias) ), 
      (const char*)BASE64Encode((const char*) m_pMgr->GetMEMgr()->GetAliasName( m_nYYmeetingUserID)) ,
      (const char*)strMeetinginfo,(const char*)strTag
        
      );
    if (m_nUserID > D_ROOM_ID_UPPERLIMIT){
      
      this->SetWindowPos(NULL,( m_workwidth - D_DEFAULT_W)/2, (m_workheight- D_DEFAULT_H)/2 ,  D_DEFAULT_W,D_DEFAULT_H, SWP_NOACTIVATE);
    }
    if (m_nUserID <D_ROOM_ID_UPPERLIMIT||m_nUserID> D_FREEGROUP_ID_UPPERLIMIT  ){
		  CString title;
		  title.Format("%s(%u)", m_strUserAlias,  m_nUserID );
		  this->SetWindowText(title);

    }else {
       this->SetWindowText(m_strUserAlias);
    }
    this->Navigate(strUrl);
  }
 else if (m_emStyle == em_optDlg){
   //Width="541" Height="360" YYDlg list online
   PString strUrl;
   strUrl.sprintf((const char*)GetDialogHTTPURL(D_OPT_HTML), (int)em_optDlg);
   CWnd *pWnd= FromHandle(m_pMgr->GetSpriteX2Ctrl()->GetHWNDMainContainer());
    
   if (pWnd!=NULL)//Width="329" Height="317"  
   {
     CRect rc;
     pWnd->GetWindowRect(rc);
     this->SetWindowPos(NULL, (rc.Width() - 329) /2, (rc.Height() - 317) /2,  329,317,/*D_DEFAULT_W,D_DEFAULT_H, */SWP_NOMOVE);
   }else
   {
    this->SetWindowPos(NULL, (1024-329)/2, (768-317)/2,  329,317,/*D_DEFAULT_W,D_DEFAULT_H, */SWP_NOMOVE);
   }
   this->Navigate(strUrl);
 
 }else if (m_emStyle == em_listonlineDlg){  
   this->SetWindowText("Online Users");
   PString strUrl;
   strUrl.sprintf((const char*)GetDialogHTTPURL(D_LIST_ONLINE_HTML), (int)em_listonlineDlg, m_emSubStyle, (const char*) m_strUserinfodetail );
   //this->SetWindowPos(NULL, 0, 0,   763,550,/*D_DEFAULT_W,D_DEFAULT_H, */SWP_NOMOVE);
   this->SetWindowPos(NULL,( m_workwidth - 763)/2, (m_workheight- 550)/2 ,  763,550, SWP_NOACTIVATE);
   this->Navigate(strUrl);
  }
 else if (m_emStyle == em_viewuserDlg){  
   // this->SetWindowText("User Detail");
    PTRACE(5, "DLG::\t  em_viewuserDlg: show beging 1 m_emStyle=" << m_emStyle);
   PString strUrl;
   strUrl.sprintf((const char*)GetDialogHTTPURL(D_VIEW_USER_HTML), (int)em_viewuserDlg, m_nUserID , m_nYYmeetingUserID);
   this->SetWindowPos(NULL, 0, 0,    682,460,/*D_DEFAULT_W,D_DEFAULT_H, */SWP_NOMOVE);
    PTRACE(5, "DLG::\t  em_viewuserDlg: show beging 2 m_emStyle=" << m_emStyle);
   this->Navigate(strUrl);
    PTRACE(5, "DLG::\t  em_viewuserDlg: show beging 3 m_emStyle=" << m_emStyle << strUrl);
   
 }
 else if (m_emStyle == em_createmeetDlg){  
  this->SetWindowText("Create Meeting");
   PString strUrl;
   strUrl.sprintf((const char*)GetDialogHTTPURL(D_CREATE_MEET_HTML), (int)em_createmeetDlg,m_nYYmeetingUserID,(const char*) m_strMeetingid );
   //this->SetWindowPos(NULL, 0, 0,   512,580,/*D_DEFAULT_W,D_DEFAULT_H, */SWP_NOMOVE);
   this->SetWindowPos(NULL,( m_workwidth - 512)/2, (m_workheight- 580)/2 ,  512,580, SWP_NOACTIVATE);
   this->Navigate(strUrl);
 }
 else if (m_emStyle == em_createfreegroupDlg){  
   if (m_strUserAlias.GetLength()>0 )
    this->SetWindowText("Edit CC Group");
   else
    this->SetWindowText("Create CC Group");
   PString strUrl;
   strUrl.sprintf((const char*)GetDialogHTTPURL(D_CREATE_CC_HTML), (int)em_createfreegroupDlg,m_nYYmeetingUserID, (const char*) g_GetAliasName(m_nYYmeetingUserID, false), (const char*) m_strUserAlias/* meeting info base64*/  );
   //this->SetWindowPos(NULL, 0, 0,   512,580,/*D_DEFAULT_W,D_DEFAULT_H, */SWP_NOMOVE);
  this->SetWindowPos(NULL,( m_workwidth - 512)/2, (m_workheight- 487)/2 ,  512,487, SWP_NOACTIVATE);
   this->Navigate(strUrl);
 } 
 else if (m_emStyle == em_MCCNotifyDlg){  
  this->SetWindowText("CCVision technology");
   PString strUrl;
   BBQ_MeetingInfo info={0};
   if (m_pMgr->GetMEMgr()->GetMeetingInforByRoomID( m_nUserID, info)){
    unsigned char out[4096]={0};
	  int ret = PKI_encode((const unsigned char*)&info, sizeof(BBQ_MeetingInfo), out);
    strUrl.sprintf((const char*)GetDialogHTTPURL(D_MCC_NOTIFY_HTML), (int)em_MCCNotifyDlg,  (const char*) out  );

    this->SetWindowPos(NULL,( m_workwidth - 372) , (m_workheight- 454)  ,  372,454, SWP_NOACTIVATE);
    this->Navigate(strUrl);
   }
 }
 else if (m_emStyle == em_welcomeDlg){  
    this->SetWindowText("Welcome");
   PString strUrl;
   strUrl.sprintf((const char*)GetDialogHTTPURL(D_WELCOME_HTML), (int)em_welcomeDlg);
   //this->SetWindowPos(NULL, workwidth -380 - r.Width() -20, (workheight-r.Height())/2,   829,595,/*D_DEFAULT_W,D_DEFAULT_H, */SWP_SHOWWINDOW);
   this->Navigate(strUrl);
   //ShowWindow(SW_SHOW);
 }
 else if (m_emStyle == em_guideAudioDlg){  
    this->SetWindowText("Voice setup wizard");
   PString strUrl;
   PString strDevicesMic, strDevicesSpeaker ; 
   PStringList decs ; m_pMgr->GetOPALMgr()->GetMicrophoneNames(decs);
   for(int i=0;i< decs.GetSize();i++){
    strDevicesMic +=( decs[i] +",");
   }
   m_pMgr->GetOPALMgr()->GetSpeakerNames(decs);
   for(int i=0;i< decs.GetSize();i++){
    strDevicesSpeaker +=( decs[i] +",");
   } 
   strUrl.sprintf((const char*)GetDialogHTTPURL(D_GUIDE_AUDIO), (int)em_guideAudioDlg, 0,0,(const char*)strDevicesMic ,(const char*)strDevicesSpeaker );
   this->SetWindowPos(NULL, 0, 0,   430,331,/*D_DEFAULT_W,D_DEFAULT_H, */SWP_NOMOVE);
   this->Navigate(strUrl);
 }
 //ShowWindow(SW_SHOW);
}
bool COptionsExDlg::GetJScript(CComPtr<IDispatch>& spDisp)
{
  IHTMLDocument2 *pDocument=NULL;
  HRESULT hr = GetDHtmlDocument(&pDocument); 
  if(FAILED(hr)){
    //MessageBox("Failed to get htmldocument. ");
    return false;
  }
  hr = pDocument->get_Script(&spDisp);
  ATLASSERT(SUCCEEDED(hr));
  return SUCCEEDED(hr);
}


CComVariant COptionsExDlg::CallJScript(const CString strFunc,
                                  const CStringArray&
                                        paramArray)
{
  CComVariant vaResult;
  if ( GetSafeHwnd() ==NULL /*|| IsWindowVisible() == FALSE*/) 
    return vaResult;
  //Getting IDispatch for Java Script objects
  CComPtr<IDispatch> spScript;
  if(!GetJScript(spScript))
  {
    //ShowError("Cannot GetScript");
    return false;
  }
  //Find dispid for given function in the object
  CComBSTR bstrMember(strFunc);
  DISPID dispid = NULL;
  HRESULT hr = spScript->GetIDsOfNames(IID_NULL,&bstrMember,1,
                         LOCALE_SYSTEM_DEFAULT,&dispid);
  if(FAILED(hr))
  {
    //ShowError(GetSystemErrorMessage(hr));
    return false;
  }

  const int arraySize = paramArray.GetSize();
  //Putting parameters
  DISPPARAMS dispparams;
  memset(&dispparams, 0, sizeof dispparams);
  dispparams.cArgs      = arraySize;
  dispparams.rgvarg     = new VARIANT[dispparams.cArgs];
  dispparams.cNamedArgs = 0;
  
  for( int i = 0; i < arraySize; i++)
  {
    CComBSTR  bstr = paramArray.GetAt(arraySize - 1 - i);
              // back reading
    bstr.CopyTo(&dispparams.rgvarg[i].bstrVal);
    dispparams.rgvarg[i].vt = VT_BSTR;
  }
  EXCEPINFO excepInfo;
  memset(&excepInfo, 0, sizeof excepInfo);
  
  UINT nArgErr = (UINT)-1;      // initialize to invalid arg
  //Call JavaScript function
  hr = spScript->Invoke(dispid,IID_NULL,0,
                        DISPATCH_METHOD,&dispparams,
                        &vaResult,&excepInfo,&nArgErr);
  delete [] dispparams.rgvarg;
  if(FAILED(hr))
  {
//    ShowError(GetSystemErrorMessage(hr));
    return false;
  }
  return vaResult;
}

 void COptionsExDlg::OnOK()
{

}

void COptionsExDlg::OnCancel()
{
	//ASSERT_VALID(this);
 // CloseWindow();
  //SendMessage(WM_CLOSE);
	// Common dialogs do not require ::EndDialog
	//Default();
 CDHtmlDialog::OnCancel();
 //if (m_emStyle == em_popIM|| m_emStyle == em_MCCNotifyDlg)
  m_pMgr->GetSpriteX2Ctrl()-> DestroyPopDlg(m_nUserID, m_emStyle);
 /*else{
   this->DestroyWindow();
   delete this;
 }*/
//  DestroyWindow();
}
CString COptionsExDlg::LoadSetting()
{
	char* caps[10]={0};
	//camera
	int nCount = m_pMgr->GetCamaraNames(caps, 10);
	CComboBox* p = (CComboBox*)this->GetDlgItem( IDC_COMBO_CAMERA);
  CString strCaps="";
	for (int i=0;i< nCount;i++)
	{
		strCaps += (caps[i]+ CString(","));
		 
	}
	//strCaps+=("none");
 

   
      CString strOptions = "";     
      //device opt
      //camera
      //speaker
      //microphone
      char camera[128]={0};
      char speaker[128]={0};
      char microphone[128]={0};

      CString strTmp;
      strTmp.Format ("cameralist=%s;camera=%s;speaker=%s;microphone=%s;", (const char*)strCaps, g_GetSettingStringSafe("camera",camera, "None"),g_GetSettingStringSafe("speaker",speaker ,"default"),g_GetSettingStringSafe("microphone", microphone, "default") );      
      //video opt
      
      //enabled codec
      strOptions += strTmp;
 
      strOptions += ("enablevideocodec="+CString(g_GetSettingString("enablevideocodec", "Dolphin,H.264") )+ ";");
      //availble codec
    
 
      strOptions += ("availblevideocodec=" + CString(g_GetSettingString("availblevideocodec","H.263") ) + ";");
      //channel type
      strOptions += ("videochanneltype=" + CString(g_GetSettingString("videochanneltype","AUTO_UDP") ) + ";");

      //audio opt
      //enabled codec
 
      strOptions += ("enableaudiocodec=" +  CString(g_GetSettingString("enableaudiocodec", "Dolphin,G.711-ulaw") )  + ";");
      //availble codec
 
      strOptions += ("availbleaudeocodec=" +  CString(g_GetSettingString("availbleaudeocodec", "G.711-alaw") ) + ";");
       //answer type
      strOptions += ("answertype=" +  CString(g_GetSettingString("answertype", "0") ) + ";");
       //expandsound type
      strOptions += ("expandsound=" +  CString(g_GetSettingString("expandsound", "0") ) + ";");
      //yvideorepairsocket
      strOptions += ("yvideorepairsocket=" +  CString(g_GetSettingString("yvideorepairsocket", "1") ) + ";");

      //sip opt
      CString strSIP;
      char user[128]={0};
      char domain[128]={0};
      char password[128]={0};
      char display[128]={0};
      char auth[128]={0};
      strSIP.Format("user=%s;domain=%s;password=%s;display=%s;auth=%s;", g_GetSettingStringSafe("user", user, "1000"), g_GetSettingStringSafe("domain", domain, "192.168.1.20"),g_GetSettingStringSafe("password", password,"123"),g_GetSettingStringSafe("display", display, "Catherine"  ),g_GetSettingStringSafe("auth",auth, "1000"));
      strOptions += strSIP;

  return strOptions;
}
void COptionsExDlg::SaveSetting(CString strOptions_d)
{
  PString strOptions  = strOptions_d;
  
  PStringArray stroptArr = strOptions.Tokenise(';');
  for (int i = 0; i < stroptArr.GetSize(); i++)
  {
    PString strValue = stroptArr[i];
    int nPos=0;
    if ((nPos = strValue.Find("camera=")) == 0)
    {
      nPos+= 7;
      PString v = strValue.Mid(nPos);
      PString str =g_GetSettingString("camera", "");
      if (str!=v){
        g_SetSettingString( "camera" , v);
       
      }
      // m_pMgr->SetCamaraName((const char*)v);
      // m_pMgr->GetOPALMgr()->m_bIsAutoCloseLocalVideo =false;
    }
    else if ((nPos = strValue.Find("speaker=")) == 0)
    {
      nPos += 8;
      PString v = strValue.Mid(nPos);
      g_SetSettingString("speaker" , v);
    }
    else if ((nPos = strValue.Find("expandsound=")) == 0)
    {
      nPos += 12;
      PString v = strValue.Mid(nPos);
      g_SetSettingString("expandsound" , v);
    }
    else if ((nPos = strValue.Find("microphone=")) == 0)
    {
      nPos += 11;
      PString v = strValue.Mid(nPos);
      g_SetSettingString("microphone"  , v);
    }
    else if ((nPos = strValue.Find("enablevideocodec=")) == 0)
    {
      nPos += PString("enablevideocodec=").GetLength();
      PString  strenablevideocodec= strValue.Mid(nPos) ;
      //enableVideoCodec.Items.Clear();
      //for (int j = 0; j < v.Length; j++)
      //  enableVideoCodec.Items.Add(v[j]);

      g_SetSettingString("enablevideocodec"  ,strenablevideocodec);
      
    }
    else if ((nPos = strValue.Find("videochanneltype=")) == 0)
    {
      nPos += PString("videochanneltype=").GetLength();
      PString  v = strValue.Mid(nPos) ;
      //SetComboxText( comVideoChannelType.SelectedItem, v);
      g_SetSettingString("videochanneltype"  ,v);
      g_SetSettingString("yVideoTransport",  v);
     
    }
    else if ((nPos = strValue.Find("enableaudiocodec=")) == 0)
    {
      nPos +=PString( "enableaudiocodec=").GetLength();//.Length;
      PString  v = strValue.Mid(nPos) ;
      g_SetSettingString("enableaudiocodec"  ,v);

    }
    else if ((nPos = strValue.Find("yvideorepairsocket=")) == 0)
    {
      nPos +=PString( "yvideorepairsocket=").GetLength();//.Length;
      PString  v = strValue.Mid(nPos) ;
      g_SetSettingString("yvideorepairsocket"  ,v);

    } 
    else if ((nPos = strValue.Find("availbleaudeocodec=")) == 0)
    {
      nPos += PString("availbleaudeocodec=").GetLength();//.Length;
      PString  v = strValue.Mid(nPos) ;
      g_SetSettingString("availbleaudeocodec"  ,v);

    }        
      //sip opt
    else if ((nPos = strValue.Find("availblevideocodec=")) == 0)
    {
      nPos +=PString( "availblevideocodec=").GetLength();//.Length;
      PString  v = strValue.Mid(nPos) ;       
      g_SetSettingString("availblevideocodec"  ,v);
    }
    //sip opt
    else if ((nPos = strValue.Find("user=")) == 0)
    {
      nPos += PString("user=").GetLength();
      PString v = strValue.Mid(nPos);
      g_SetSettingString("user"  ,v);


    }
    else if ((nPos = strValue.Find("domain=")) == 0)
    {
      nPos += PString("domain=").GetLength();
      PString v = strValue.Mid(nPos);
      g_SetSettingString("domain"  ,v);

    }
    else if ((nPos = strValue.Find("password=")) == 0)
    {
      nPos += PString("password=").GetLength();//.Length;
      PString v = strValue.Mid(nPos);
      g_SetSettingString("password"  ,v);

    }
    else if ((nPos = strValue.Find("display=")) == 0)
    {
      nPos += PString("display=").GetLength();//.Length;
      PString v = strValue.Mid(nPos);
      g_SetSettingString("display"  ,v);

    }
    else if ((nPos = strValue.Find("auth=")) == 0)
    {
      nPos += PString("auth=").GetLength();//.Length;
      PString v = strValue.Mid(nPos);
      g_SetSettingString("auth"  ,v);

    }
    else if ((nPos = strValue.Find("answertype=")) == 0)
    {
      nPos += PString("answertype=").GetLength();//.Length;
      PString v = strValue.Mid(nPos);
      g_SetSettingString("answertype"  ,v);

    }    
  }
}
void COptionsExDlg::OnNotifySLDlg(int cmd, const char* msg)
{
  if (GetSafeHwnd() == NULL )
    return ;
  CString strParam;
  strParam.Format("%u;%s", cmd,  msg);
  //MessageBox(strParam);
  CStringArray strparams;

  strparams.Add(strParam);
  CallJScript("yOnNotifyEx",strparams);

}

int COptionsExDlg::DLGNotifyOfflineIM(LONG index, LONG total, LPCTSTR msg)
{
  if (GetSafeHwnd() == NULL || m_bXamlInit == false )
    return -1;
 // this->SetFocus();
//  this->SetActiveWindow();
  if (m_bFlashWindow== FALSE){
    m_bFlashWindow =TRUE;
    PlaySound(MAKEINTRESOURCE(IDR_WAVE_MSG),AfxGetResourceHandle(),SND_ASYNC|SND_RESOURCE|SND_NODEFAULT);//将声音文件写入到程序中
  }
  if (GetActiveWindow() != this  )
  {
    FlashWindow( TRUE); 
  }//FlashWindow( TRUE);   

  CString strParam;
  strParam.Format("%u;%u;%s", index,total, msg);
  //MessageBox(strParam);
  CStringArray strparams;

  strparams.Add(strParam);
  CComVariant ret= CallJScript("yOnNotifyOfflineIM",strparams);
  
  return ret.dblVal;
 // int i=0;
}
BOOL COptionsExDlg::DestroyWindow()
{
  // TODO: Add your message handler code here and/or call default
 m_bFlashWindow =FALSE;
  if (m_emStyle == em_popIM ){
    if (m_nUserID < D_ROOM_ID_UPPERLIMIT){
      SIMD_CS_CONFERENCE in={0};
      in.action = em_conf_left;
      in.ActionUId= m_nYYmeetingUserID;
      in.roomid= m_nUserID;
      m_pMgr->GetMEMgr()->NotifyServerConferenceChange(&in);
      
    }else{
      //remove the transmmit file session
      m_pMgr->GetTransmiterMgr()->RemoveSession(m_nUserID);
    }

  }
 
  return CDHtmlDialog::DestroyWindow();
}
// 



void YYThreadDlg::Main( )
{
   {
    
#if 1
#ifdef LPN_ACTIVEX_PRJ
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    //AFX_MODULE_THREAD_STATE* mts=AfxGetModuleThreadState();
    //AFX_MODULE_THREAD_STATE backup_mts;
    //memcpy(&backup_mts,mts,sizeof(AFX_MODULE_THREAD_STATE));
    //memcpy(mts,(AFX_MODULE_THREAD_STATE*)ref_mts,sizeof(AFX_MODULE_THREAD_STATE));
    //CoInitialize(NULL);
    //AfxEnableControlContainer(NULL);
#else
    //AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CoInitialize(NULL);
    //AfxEnableControlContainer(NULL);
#endif
    if (!m_pDlg)
    {
      COptionsExDlg * p =m_pDlg= new COptionsExDlg();
      p->m_pMgr= m_pLPNMgr;
      p->m_emStyle= em_popIM;
      p->m_nUserID = m_nUserID;
      p->m_nYYmeetingUserID =  m_pLPNMgr->GetMEMgr()->GetID();
      p->m_strUserAlias = m_strUserAlias;
      p->Create(IDD_DIALOG_OPTIONS);
      //p->UpdateWindow();
      p->ShowWindowEx();
    }
    
#else
    m_hWnd = CreateWindow(wndClassName,
                          windowTitle, 
                          dwStyle|WS_SIZEBOX,
                          m_lastX, m_lastY, frameWidth, frameHeight,
                          hParent, NULL, GetModuleHandle(NULL), this);
#endif
    //SetWindowSize();
		//this->SetWindowPos(0,0,176,144);
  }
  
  m_started.Signal();
  MSG msg;
  while (/*m_hWnd!=NULL &&*/ GetMessage(&msg, NULL, 0, 0) && msg.message != WM_QUIT  )
	{
    // m_pDlg->ProcessMessageFilter(&msg);
    //if (!AfxPreTranslateMessage(&msg) )
    {
      TranslateMessage(&msg); 
      DispatchMessage(&msg); 
    }
 
    
	}
	if (m_pDlg ){
		//m_pDlg->m_hWnd = NULL;
		delete m_pDlg;m_pDlg=NULL;
		//UnregisterClass(wndClassName, GetModuleHandle(NULL));
    
	}
// memcpy(mts,&backup_mts,sizeof(AFX_MODULE_THREAD_STATE));


}
   
YYThreadDlg::	YYThreadDlg(LPNMgr*pLPNMgr ,unsigned int owerID, const char* imDlgOweraAlias)
#ifdef PT_THREAD
:PThread(20000,PThread::AutoDeleteThread, PThread::NormalPriority, "YYThreadD" + PString(owerID) )
#else
:CYYWin32Thread()
#endif
 ,m_pLPNMgr(pLPNMgr)
,m_pDlg(0)
 {
    m_nUserID = owerID;
    m_emStyle= em_popIM;
    m_nYYmeetingUserID =  m_pLPNMgr->GetMEMgr()->GetID();
    m_strUserAlias = imDlgOweraAlias;
#ifdef PT_THREAD
ref_mts = AfxGetModuleThreadState();

   Resume();
#else
    this->CreateThread();
 //AfxBeginThread((AFX_THREADPROC)ThreadFunc,this);
#endif
 }
YYThreadDlg::~YYThreadDlg()
{
 
	
 
}
//DWORD YYThreadDlg::ThreadFunc(PVOID pThis)
//{
//  YYThreadDlg* pThread = (YYThreadDlg*) pThis;
//  COptionsExDlg* m_pDlg = pThread->m_pDlg;
//  {
//#if 1
//#ifdef LPN_ACTIVEX_PRJ
//    AFX_MANAGE_STATE(AfxGetStaticModuleState());
//    //CoInitialize(NULL);
//    //AfxEnableControlContainer(NULL);
//#else
//    //AFX_MANAGE_STATE(AfxGetStaticModuleState());
//    CoInitialize(NULL);
//    //AfxEnableControlContainer(NULL);
//#endif
//    if (!m_pDlg)
//    {
//      COptionsExDlg * p =m_pDlg= new COptionsExDlg();
//      p->m_pMgr= pThread->m_pLPNMgr;
//      p->m_emStyle= em_popIM;
//      p->m_nUserID = pThread->m_nUserID;
//      p->m_nYYmeetingUserID =  pThread->m_pLPNMgr->GetMEMgr()->GetID();
//      p->m_strUserAlias = pThread->m_strUserAlias;
//      p->Create(IDD_DIALOG_OPTIONS);
//      //p->UpdateWindow();
//      p->ShowWindowEx();
//    }
//    
//#else
//    m_hWnd = CreateWindow(wndClassName,
//                          windowTitle, 
//                          dwStyle|WS_SIZEBOX,
//                          m_lastX, m_lastY, frameWidth, frameHeight,
//                          hParent, NULL, GetModuleHandle(NULL), this);
//#endif
//    //SetWindowSize();
//		//this->SetWindowPos(0,0,176,144);
//  }
//  
//  pThread->m_started.Signal();
//  MSG msg;
//  while (/*m_hWnd!=NULL &&*/ GetMessage(&msg, NULL, 0, 0) && msg.message != WM_QUIT  )
//	{
//    // m_pDlg->ProcessMessageFilter(&msg);
//    //if (!AfxPreTranslateMessage(&msg) )
//    {
//      TranslateMessage(&msg); 
//      DispatchMessage(&msg); 
//    }
// 
//    
//	}
//	if (m_pDlg ){
//		//m_pDlg->m_hWnd = NULL;
//		delete m_pDlg;m_pDlg=NULL;
//		//UnregisterClass(wndClassName, GetModuleHandle(NULL));
//    
//	}
//  return 0;
//}
//
 
 
 

BOOL COptionsExDlg::PreTranslateMessage(MSG* pMsg)
{
  // TODO: Add your specialized code here and/or call the base class
  if(pMsg->message==WM_KEYDOWN&&pMsg->wParam==VK_RETURN){
    //BOOL b=FALSE;
        return FALSE;
  }
  if(pMsg->message==WM_KEYDOWN&&pMsg->wParam==VK_ESCAPE)     
      return TRUE;
  return CDHtmlDialog::PreTranslateMessage(pMsg);
}
// 窗口振动函数
void COptionsExDlg::ShakeWindow()
{
    CRect rcWnd ;
    GetWindowRect(rcWnd) ;
    m_ptPosBeforeShake = rcWnd.TopLeft() ;    // 记录窗口振动之前的位置
    SetTimer(1, 40, NULL) ;                   // 启动窗口振动定时器
}

#define OFFSET    3

void COptionsExDlg::OnTimer(UINT nIDEvent)
{
  // TODO: Add your message handler code here and/or call default
    static int s_nIndex = 0 ;
    static int s_anOffset[][2] = {
        {OFFSET, - OFFSET}, {0, - OFFSET - OFFSET}, {- OFFSET, - OFFSET}, {0, 0},
        {OFFSET, - OFFSET}, {0, - OFFSET - OFFSET}, {- OFFSET, - OFFSET}, {0, 0},
        {OFFSET, - OFFSET}, {0, - OFFSET - OFFSET}, {- OFFSET, - OFFSET}, {0, 0},
        {OFFSET, - OFFSET}, {0, - OFFSET - OFFSET}, {- OFFSET, - OFFSET}, {0, 0},
    } ;    // 振动轨迹为4次菱形循环
    if (nIDEvent == 1)
    {
        SetWindowPos(
            NULL,
            m_ptPosBeforeShake.x + s_anOffset[s_nIndex][0], m_ptPosBeforeShake.y + s_anOffset[s_nIndex][1],
            0, 0,
            SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE
            ) ;
        ++ s_nIndex ;
        if (s_nIndex == 16)
        {
            s_nIndex = 0 ;
            KillTimer(1) ;
        }
    }else if (nIDEvent==2)
    {
      if (GetActiveWindow() == this)
        return ;
      else{
        FlashWindow(m_bFlashWindow);
        

      }
    }

  CDHtmlDialog::OnTimer(nIDEvent);
}

void COptionsExDlg::OnClose()
{
  if (m_emStyle == em_optDlg){
     g_VideoLayout =em_videoLayoutAdvanced;
     m_pMgr->GetOPALMgr() ->SetCameraName( m_pMgr->GetOPALMgr()->GetVideoInputDevice().deviceName);
  }  
  if (m_pLocalVideo){
    m_pLocalVideo->DestroyWindow();
    delete m_pLocalVideo;m_pLocalVideo=0;
  }

  CDHtmlDialog::OnClose();
}
//#endif //sdk
void COptionsExDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
  CDHtmlDialog::OnShowWindow(bShow, nStatus);
  CVideoPanelDlg *pPanel =m_pMgr->GetSpriteX2Ctrl()->m_pVideoPanelDlg;
  if (bShow && m_emStyle == em_popIM&& (m_nUserID >0 && m_nUserID < D_ROOM_ID_UPPERLIMIT)   && pPanel &&  pPanel->GetSafeHwnd() ){
    //if (pPanel->GetLayoutStyle() ==  ID_Normal)
      pPanel->OnMenuItem(ID_Normal);//如果 文档panel存在了，必须是 ID_Normal 布局
    pPanel->SwitchToTheVideoAdcanceWindow();
    
    
  }
  // TODO: Add your message handler code here
}


 
 
 


void COptionsExDlg::OnSize(UINT nType, int cx, int cy)
{
   CDHtmlDialog::OnSize(nType, cx, cy);
  //InvalidateRect(NULL,FALSE);
  // TODO: Add your message handler code here 
  if (cy>0){
      CStringArray params;
      CString strParam = "";
      strParam.Format("100;%d;%d", /*em_sizeSLLeftPanelWindow,*/ cx , cy );
      params.Add(strParam);
      CallJScript("yOnNotifyEx",params);
  }
}


void COptionsExDlg::OnBnClickedOk()
{
  // TODO: Add your control notification handler code here
}


LRESULT COptionsExDlg::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
  // TODO: Add your specialized code here and/or call the base class
  stMsgVideoChild* pChildMsg= (stMsgVideoChild*) wParam; 
  if (message  == WM_VIDEO_CHILD){
	   
      if (m_pLocalVideo ==NULL){
   
        m_pLocalVideo = new CSFVideoWnd();
        //m_pLocalVideo->m_pMyParent = this;
         
          CRect rect(27, 104, 27+176,104+144);
	      //video window
	        m_pLocalVideo->Create(WS_CHILD|WS_VISIBLE,rect,this,AFX_IDW_PANE_FIRST);
       
          m_pLocalVideo->BringWindowToTop();
          m_pLocalVideo->ShowWindow(SW_SHOW);
  
        //
      } 
 

      //m_pLocalVideo->SetFrameSize(pChildMsg->w, pChildMsg->h);
      //m_pLocalVideo->SetFrameData(0,0,pChildMsg->w, pChildMsg->h,(const BYTE*) pChildMsg->pData);
      m_pLocalVideo->Draw("", pChildMsg->pData, 0, pChildMsg->w,pChildMsg->h);
      return TRUE;
    }
    
    
   
  return CDHtmlDialog::DefWindowProc(message, wParam, lParam);
}


int COptionsExDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  if (CDHtmlDialog::OnCreate(lpCreateStruct) == -1)
    return -1;
  //::SetClassLong(this->m_hWnd, GCL_HBRBACKGROUND, (LONG)GetStockObject(BLACK_BRUSH));
  // TODO:  Add your specialized creation code here

  return 0;
}


BOOL COptionsExDlg::OnEraseBkgnd(CDC* pDC)
{
  // TODO: Add your message handler code here and/or call default
      CBrush   backBrush(RGB(0,   0,   0));   
      CBrush*   pOldBrush   =   pDC->SelectObject(&backBrush);   
      CRect   rect;   
      pDC->GetClipBox(&rect);             
      pDC->PatBlt(rect.left,   rect.top,   rect.Width(),     
      rect.Height(),   PATCOPY);   
      pDC->SelectObject(pOldBrush);   
			 return TRUE;
  return CDHtmlDialog::OnEraseBkgnd(pDC);
}
