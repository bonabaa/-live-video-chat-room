// YYContainerDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "YYContainer.h"
#include "YYContainerDlg.h"
#include ".\yycontainerdlg.h"
#include "LocalVideoDlg.h"
#include "PromptDlg.h"
#include "YYWin32Thread.h"
//#include "../miniweb/httpapi.h"
//extern FILE *stream ;

#define D_PRINT(x) ;//{printf(x);fflush(stream); }


#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#define D_DEFAULT_W   1024
#define D_DEFAULT_H   728
#define D_DEFAULT_SL_LEFTPANEL_W   260
extern CString GetAppPath();

// CYYContainerDlg 消息处理程序
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
CString UTF8ToPlainText(const char* lpszContent)
{
  //return lpszContent;
  CString strResult= lpszContent;
	//CString strContent(lpszContent);
	int nSrcLen = strlen(lpszContent);
	int nLen = ::MultiByteToWideChar(CP_UTF8,  0, lpszContent, nSrcLen, NULL, 0);
	if (nLen > 0) {
		//CTempBuffer<WCHAR> pszW;
		//ATLTRY(pszW.Allocate(nLen));	
    wchar_t *pszW=new wchar_t[nLen];
    memset(pszW, 0, nLen);
		// do the UTF8 to UNICODE conversion
		nLen = ::MultiByteToWideChar(CP_UTF8, 0, lpszContent, nSrcLen,pszW, nLen);
		nSrcLen = nLen;
    
		nLen = ::WideCharToMultiByte(CP_ACP, 0, pszW, nSrcLen, NULL, 0, NULL, NULL);	
		if(nLen > 0)
		{
			//CString strConvert;
			//LPSTR sz = strConvert.GetBuffer(nLen+1);
      char *sz= new char[nLen+1];
      memset(sz, 0, nLen+1);
			nLen = ::WideCharToMultiByte(CP_ACP, 0, pszW, nSrcLen, sz, nLen, NULL, NULL);	
			sz[nLen] = '\0';
			//strConvert.ReleaseBuffer(nLen);
      strResult = sz;
      delete[] sz;
			//return sz;
		} 
    delete[] pszW;
	} 
	return strResult;
}

enum em_JSMsgType{
  em_sizeContainerWindow,
  em_sendIM,
  em_ExitProgram=10,
  em_sizeSLLeftPanelWindow= 100,
   em_pop_GetUserID= 103,
   em_pop_GetYYMeetingUserID= 104,
  em_setWindowTitle= 102,
  em_PrepareLogin=105,
  em_SendMsgFromContainer=10000

};

// CYYContainerDlg 对话框

BEGIN_DHTML_EVENT_MAP(CYYContainerDlg)
	DHTML_EVENT_ONCLICK(_T("ButtonOK"), OnButtonOK)
	DHTML_EVENT_ONCLICK(_T("ButtonCancel"), OnButtonCancel)

END_DHTML_EVENT_MAP()
BEGIN_DISPATCH_MAP(CYYContainerDlg, CDHtmlDialog)
   // DISP_FUNCTION(CYYContainerDlg, "Func1", Func1, VT_EMPTY, VTS_NONE) 

    DISP_FUNCTION(CYYContainerDlg, "OnSendJSMsg", OnSendJSMsg, VT_BSTR, VTS_BSTR) 
  //   DISP_FUNCTION(COptionsExDlg, "OnSendJSMsgEx", OnSendJSMsgEx, VT_BSTR, VTS_BSTR) 
    // example:VTS_NONE
    // DISP_FUNCTION(CMyDHTMLDialog,"Func2",TestFunc,VT_BOOL,VTS_BSTR VTS_I4 VTS_I4)
    //                                                                        ^return,        ^parameters type list
    //每个方法都需要在这里添加映射
END_DISPATCH_MAP()

unsigned int CYYContainerDlg::ThreadminiWeb( LPVOID pParam )
{
   CYYContainerDlg* pDlg =(CYYContainerDlg*) pParam;
    
   while ( !pDlg->m_hp.bKillWebserver   )
	  {
 
      mwHttpLoop(& pDlg->m_hp);
      Sleep(0);
        
    }
	  
  
    return 0;
}
//unsigned int CYYContainerDlg::ThreadFunc( LPVOID pParam )
//{
//   CPromptDlg* pDlg =(CPromptDlg*) pParam;
//   pDlg->Create(IDD_DIALOG_PROMPT_ONLINE);
//   pDlg->ShowWindow(SW_SHOW);
//    MSG msg;
//    while (/*m_hWnd!=NULL &&*/ GetMessage(&msg, NULL, 0, 0) && msg.message != WM_QUIT  )
//	  {
// 
//      TranslateMessage(&msg); 
//      DispatchMessage(&msg); 
//        
//    }
//	  
//   pDlg->DestroyWindow();
//    return 0;
//}
BOOL CYYContainerDlg::CanAccessExternal()
{
 		return TRUE;
}
HWND CYYContainerDlg::GetYYMeetingWindow()
{
    CString strWinTitle ;strWinTitle.Format("YYMeeting-[%u]", m_nYYmeetingUserID );
    return ::FindWindow( NULL, (const char*)strWinTitle );
}
void CYYContainerDlg::SetFullScreen(bool bfull)     
{
   
  HWND m_hwnd1 = GetSafeHwnd();
  //return;
	// Find how large the desktop work area is
	RECT workrect;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workrect, 0);
	int workwidth = workrect.right -  workrect.left;
	int workheight = workrect.bottom - workrect.top;

	RECT fullwinrect;
	

		SetRect(&fullwinrect, 0, 0,
				workwidth, workheight);


	//AdjustWindowRectEx(&fullwinrect, 
	//		   GetWindowLong(m_hwnd, GWL_STYLE ), 
	//		   FALSE, GetWindowLong(m_hwnd, GWL_EXSTYLE));

	m_fullwinwidth = fullwinrect.right - fullwinrect.left;
	m_fullwinheight = fullwinrect.bottom - fullwinrect.top;

	//AdjustWindowRectEx(&fullwinrect, 
	//		   GetWindowLong(m_hwndscroll, GWL_STYLE ) & ~WS_HSCROLL & 
	//		   ~WS_VSCROLL & ~WS_BORDER, 
	//		   FALSE, GetWindowLong(m_hwndscroll, GWL_EXSTYLE));
	AdjustWindowRectEx(&fullwinrect, 
			   GetWindowLong(m_hwnd1, GWL_STYLE ), 
			   FALSE, GetWindowLong(m_hwnd1, GWL_EXSTYLE));

	//if (GetMenuState(GetSystemMenu(m_hwnd1, FALSE),
	//				 ID_TOOLBAR, MF_BYCOMMAND) == MF_CHECKED) {
	//	RECT rtb;
	//	GetWindowRect(m_hToolbar, &rtb);
	//	fullwinrect.bottom = fullwinrect.bottom + rtb.bottom - rtb.top - 3;
	//}

	m_winwidth  =  min(fullwinrect.right - fullwinrect.left,  workwidth);
	m_winheight =  min(fullwinrect.bottom - fullwinrect.top, workheight);
	if ((fullwinrect.right - fullwinrect.left > workwidth) &&
		(workheight - m_winheight >= 16)) {
		m_winheight = m_winheight + 16;
	} 
	if ((fullwinrect.bottom - fullwinrect.top > workheight) && 
		(workwidth - m_winwidth >= 16)) {
		m_winwidth = m_winwidth + 16;
	}

	int x,y;
	WINDOWPLACEMENT winplace;
	winplace.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement( &winplace);
	if (bfull==false) {
    x =  workwidth/3 *2;		
		y = 20;		
    m_winwidth = D_DEFAULT_W;
    m_winheight = D_DEFAULT_H;
	} else {
		// Try to preserve current position if possible
		GetWindowPlacement(  &winplace);
		if ((winplace.showCmd == SW_SHOWMAXIMIZED) || (winplace.showCmd == SW_SHOWMINIMIZED)) {
			x = winplace.rcNormalPosition.left;
			y = winplace.rcNormalPosition.top;
		} else {
			RECT tmprect;
			GetWindowRect(  &tmprect);
			x = tmprect.left;
			y = tmprect.top;
		}
		if (x + m_winwidth > workrect.right)
			x = workrect.right - m_winwidth;
		if (y + m_winheight > workrect.bottom)
			y = workrect.bottom - m_winheight;
	}
	winplace.rcNormalPosition.top = y;
	winplace.rcNormalPosition.left = x;
	winplace.rcNormalPosition.right = x + m_winwidth;
	winplace.rcNormalPosition.bottom = y + m_winheight;
	SetWindowPlacement(  &winplace);
 
	//PositionChildWindow();
}




BSTR CYYContainerDlg::OnSendJSMsg(LPCTSTR p )
{
  CString str=p, strResult;int nStart=0;
  try{
    CString strMsgID =  str.Tokenize(";",nStart);
    
    // we trust all com object (haha, you can make virus)
    switch(atoi(strMsgID))
    {
    case em_sizeContainerWindow:
      {
        //int w= atoi(str.Tokenize(";",nStart));
        //int h= atoi(str.Tokenize(";",nStart));
        CRect rect;
        GetWindowRect(rect);
        if (rect.Width()!= D_DEFAULT_W){
          CStringArray params;
          CString strParam = "";
          strParam.Format("%d;%d;%d", em_sizeSLLeftPanelWindow, D_DEFAULT_SL_LEFTPANEL_W, 0);
          params.Add(strParam);
          CallJScript("ySizeWindowForLeftPanel",params);
          SetFullScreen(false);
        }else{
            SetFullScreen(true);
        }

      }
      break;
    case em_sendIM:
      {
        //CString strWinTitle ;strWinTitle.Format("YYMeeting-[%u]", m_nYYmeetingUserID );
        HWND hWndReceived = GetYYMeetingWindow();//::FindWindow( NULL, (const char*)strWinTitle );      //进程B的接收数据窗口对象
        //CString sstemp = strWinTitle+ p;
       // MessageBox(strWinTitle );
        //COPYDATASTRUCT结构是WM_COPYDATA传递的数据结构对象
        COPYDATASTRUCT cpd;
        cpd.dwData =  em_IMMessage_sending;
        cpd.cbData  =  strlen(p) ;            //传递的数据长度
        char* pp = (char*)p;
        //pp[0] = em_IMMessage_sending;
        cpd.lpData    =  (void*)  (pp+2);  //传递的数据地址
        ::SendMessage( hWndReceived, WM_COPYDATA, 0, (LPARAM) & cpd );

      }
      break;   
    case em_setWindowTitle:
      {
        char tmp[32]={0};
        //CString strTitle= UTF8ToPlainText(&p[4]) ;
        const char * strTitle = (const char*)&p[4];
        //SetWindowText(&p[4]);
        if (m_emStyle == em_standard)
          m_pTrayIcon->SetTooltip(  strTitle );


      }
      break;   
    case em_pop_GetUserID:
      {
        strResult.Format("%u", m_nUserID);
      }
      break;   
    case em_PrepareLogin:
      {
        ShowWindow(SW_SHOW);
        strResult=  ( m_strTalker);
      }
      break;   
    case em_ExitProgram:
      {
       // PostMessage(WM_COMMAND, 0, ID_POPMENU_EXIT);
        CString strsub = str.Tokenize(";",nStart);
        if (strsub=="1"){
          ShowWindow(SW_SHOWMINIMIZED);
        }else if ( strsub=="3"){
          SetWindowDefaultSize();
        }else
         OnCancel();
         
      }
      break;     
    case em_pop_GetYYMeetingUserID:
      {
      strResult.Format("%u", m_nYYmeetingUserID);
      }
      break;
    case em_SendMsgFromContainer:
      {
        
        
        if (m_pDlgLocalVideo->IsWindowVisible() == FALSE ){
          CStringArray params;
          CString strParam = "";
          strParam.Format("20;0;%u", (unsigned int)m_pDlgLocalVideo->GetSafeHwnd() );//em_msgfromcontainer
          params.Add(strParam);
          CallJScript("yIMDlgControl",params);

          CRect container;
          GetWindowRect(container);
          CRect localpanel(container.left - m_pDlgLocalVideo->C_W, container.top+container.Height()/3,0,0  );
          localpanel.bottom = localpanel.top +m_pDlgLocalVideo->C_H ;
          localpanel.right =localpanel.left+m_pDlgLocalVideo->C_W;
          m_pDlgLocalVideo->MoveWindow(&localpanel);
         
           m_pDlgLocalVideo->m_bOpened = true;
          ShowLocalVideoDlg(true);
        } else{
          ShowLocalVideoDlg(false);
           m_pDlgLocalVideo->m_bOpened = false;
         // m_pDlgLocalVideo->ShowWindow(SW_HIDE);
        }
        strResult= "1";
      }break;
    };

     
  }
  catch(...){
  }
 // return 1;
 return  strResult.AllocSysString();
}
//BSTR COptionsExDlg::OnSendJSMsgEx(LPCTSTR p )
//{
//  CString strResult="";
//  CString strBody;
//  char *pSplit = strstr(p,";");
//  char cmd[24]={0};
//  if (pSplit==NULL )  {
//    memcpy(cmd,p,  strlen(p));
//    strBody="";
//  }
//  else{
//    memcpy(cmd,p, pSplit- p);
//  strBody=(&(pSplit[1])); 
//  }
//  try{
//    
//    
//    // we trust all com object (haha, you can make virus)
//    switch(atoi(cmd))
//    {
//    case em_SaveLocalOptionsInfo:
//      {
//        CStringArray params;
//        params.Add(strBody);
//        CComVariant result= CallJScript("yOptSaveSetting",params);//get from avtivex center
//        //strResult =result.bstrVal; 
//       // SaveSetting(strBody);
//      }
//      break;
//    case em_LoadLocalOptionsInfo:
//      {
//        //CStringArray params;
//        //params.Add("");
//        //CComVariant result= CallJScript("yOptLoadSetting",params);//get from avtivex center
//        //strResult =result.bstrVal;
//        //1 send message to yymeeting container to tell it's activex , i want to get options
//        //2 yymeeting container recv it ,the call js funtion to get it 
//        //3 activex recv it ,fill and direct return
//        //4 rely a msg to option dlg use call js 
//        //CString strWinTitle ;strWinTitle.Format("YYMeeting-[%u]", m_nYYmeetingUserID );
//        HWND hWndReceived = GetYYMeetingWindow();//::FindWindow( NULL, (const char*)strWinTitle );      //进程B的接收数据窗口对象
//        COPYDATASTRUCT cpd={0};
//        cpd.dwData =  em_LoadSettingessage_request;
//        cpd.cbData  =  strlen(p) ;            //传递的数据长度
//        //char* pp = (char*)p;
//        //pp[0] = em_IMMessage_sending;
//        //cpd.lpData    =  (void*)  (pp+2);  //传递的数据地址
//        ::SendMessage( hWndReceived, WM_COPYDATA, 0, (LPARAM) & cpd );
//       
//      }
//      break ;
//    };
//     
//  }
//  catch(...){
//  }
//  return strResult.AllocSysString();
//}
//
 BOOL CheckFileRelation(const char *strExt, const char *strAppKey)  
{  
    int nRet=FALSE;  
    HKEY hExtKey;  
    char szPath[_MAX_PATH];    
    DWORD dwSize=sizeof(szPath);    
    if(RegOpenKey(HKEY_CLASSES_ROOT,strExt,&hExtKey)==ERROR_SUCCESS)  
    {  
        RegQueryValueEx(hExtKey,NULL,NULL,NULL,(LPBYTE)szPath,&dwSize);  
        if(_stricmp(szPath,strAppKey)==0)  
        {  
            nRet=TRUE;  
        }  
        RegCloseKey(hExtKey);  
        return nRet;  
    }  
    return nRet;  
}  
  
// 注册文件关联   
// strExe: 要检测的扩展名(例如: ".txt")   
// strAppName: 要关联的应用程序名(例如: "C:/MyApp/MyApp.exe")   
// strAppKey: ExeName扩展名在注册表中的键值(例如: "txtfile")   
// strDefaultIcon: 扩展名为strAppName的图标文件(例如: "C:/MyApp/MyApp.exe,0")   
// strDescribe: 文件类型描述   
void  RegisterFileRelation(char *strExt, char *strAppName, char *strAppKey, char *strDefaultIcon, char *strDescribe)  
{  
    char strTemp[_MAX_PATH];  
    HKEY hKey;  
      
    RegCreateKey(HKEY_CLASSES_ROOT,strExt,&hKey);  
    RegSetValue(hKey,"",REG_SZ,strAppKey,strlen(strAppKey)+1);  
    RegCloseKey(hKey);  
      
    RegCreateKey(HKEY_CLASSES_ROOT,strAppKey,&hKey);  
    RegSetValue(hKey,"",REG_SZ,strDescribe,strlen(strDescribe)+1);  
    RegCloseKey(hKey);  
      
    sprintf(strTemp,"%s\\DefaultIcon",strAppKey);  
    RegCreateKey(HKEY_CLASSES_ROOT,strTemp,&hKey);  
    RegSetValue(hKey,"",REG_SZ,strDefaultIcon,strlen(strDefaultIcon)+1);  
    RegCloseKey(hKey);  
      
    sprintf(strTemp,"%s\\Shell",strAppKey);  
    RegCreateKey(HKEY_CLASSES_ROOT,strTemp,&hKey);  
    RegSetValue(hKey,"",REG_SZ,"Open",strlen("Open")+1);  
    RegCloseKey(hKey);  
      
    sprintf(strTemp,"%s\\Shell\\Open\\Command",strAppKey);  
    RegCreateKey(HKEY_CLASSES_ROOT,strTemp,&hKey);  
    sprintf(strTemp,"%s \"%%1\"",strAppName);  
    RegSetValue(hKey,"",REG_SZ,strTemp,strlen(strTemp)+1);  
    RegCloseKey(hKey);  
}  
CYYContainerDlg::CYYContainerDlg(CWnd* pParent /*=NULL*/)
	: CDHtmlDialog(CYYContainerDlg::IDD, CYYContainerDlg::IDH, pParent)
{
  //const char*  str= "æŽ¨èä¸“åŒº";
  //CString stre= UTF8ToPlainText(str);
  m_pTrayIcon =NULL;
  m_emStyle = em_standard;
  m_nYYmeetingUserID= m_nUserID = 0;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
  m_pDlgLocalVideo = new CLocalVideoDlg(0, this);
  m_pDlgLocalVideo->Create(IDD_DIALOG_LOCAL_VIDEO);
  EnableAutomation();
  m_pMagParentDlg =0;
  mwInitParam(&m_hp);
  m_hp.hlBindIP =ntohl( 0x7f000000);
  strcpy(m_hp.strBindIP , "127.0.0.1");
  /*m_hp.hlBindIP =ntohl( 0);
  strcpy(m_hp.strBindIP , "0.0.0.0")*/;
  strncpy( m_hp.pchWebPath, (const char*)GetAppPath(), sizeof(m_hp.pchWebPath)-1);
  m_hp.httpPort= 5666;
  while ( mwServerStart(&m_hp)!=0 )
    m_hp.httpPort++;

  AfxBeginThread(ThreadminiWeb, (LPVOID)this);


  /////////////////
  CString strAppRecordExe = GetAppPath()+"\\CCVisionRecord.exe";
  CString strAppMainExe   = GetAppPath()+"\\CCVision.exe,0";
   
  if (CheckFileRelation(".yym", "yymfile" )== FALSE){
    RegisterFileRelation(".yym",  strAppRecordExe.GetBuffer(), "yymfile", strAppMainExe.GetBuffer() , "CC Vision Recording file.");
  }

}

void CYYContainerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDHtmlDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CYYContainerDlg, CDHtmlDialog)
	//}}AFX_MSG_MAP
  ON_WM_KEYDOWN()
  ON_WM_SIZE()
  ON_WM_CLOSE()
  ON_WM_DESTROY()
  ON_WM_COPYDATA()

  ON_MESSAGE(WM_TRAYNOTIFY, OnTrayNotify)
  ON_COMMAND_RANGE(ID_POPMENU_SHOWME, ID_POPMENU_EXIT,OnMenuItem)
  ON_WM_SYSCOMMAND()
  ON_WM_NCPAINT()
  ON_WM_CREATE()
  ON_WM_TIMER()
  ON_MESSAGE(WM_HOTKEY,OnHotKey)

	ON_WM_MOVING()
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_SHOWWINDOW()

  ON_WM_LBUTTONDOWN()
  ON_WM_LBUTTONUP()
  ON_WM_ERASEBKGND()
END_MESSAGE_MAP()



 /*#define D_MAIN_HTML "D:\\chy\\projects\\my\\eqcores\\YYMeeting\\Controls.Samples\\index.html" 
 #define D_OPT_HTML "D:\\chy\\projects\\my\\eqcores\\YYMeeting-opt\\Controls.Samples\\popoption.html" */

LONG CYYContainerDlg::OnHotKey(WPARAM wParam,LPARAM lParam) 
{  
  
    UINT fuModifiers = (UINT) LOWORD(lParam);  // key-modifier flags    
    UINT uVirtKey = (UINT) HIWORD(lParam);     // virtual-key code    
  
    //判断响应了什么热键   
    if( MOD_CONTROL|MOD_ALT == fuModifiers && ('x' == uVirtKey||'X'== uVirtKey) )  
    {  
      bool bNeedVisible = !IsWindowVisible();
      ShowWindow(bNeedVisible==false ? SW_HIDE: SW_SHOW);
      ShowLocalVideoDlg(bNeedVisible);
    }  else if( MOD_CONTROL|MOD_ALT == fuModifiers && ('a' == uVirtKey||'A'== uVirtKey) )  {
      CString strParam = "0;0";
      CStringArray params;
      //strParam.Format("0;0", /*em_sizeSLLeftPanelWindow,*/ cx, cy);
      params.Add(strParam);
      CallJScript("yP2PAFile",params);
    }
     
                           
    return 0;          
}  

void CYYContainerDlg::ShowLocalVideoDlg(bool b)
{
  if (m_pDlgLocalVideo->m_bOpened )
    m_pDlgLocalVideo->ShowWindow(b? SW_SHOW: SW_HIDE );
}

void CYYContainerDlg::OnMenuItem(UINT nID) 
{
  switch(nID)
  {
  case ID_POPMENU_SHOWME:
    {
	    SetForegroundWindow();
	    ShowWindow( SW_SHOW );

        // if it is minimized restore it
	    if ( IsIconic() )
        {
		    ShowWindow( SW_RESTORE );
	    }

    }
    break;
  case ID_POPMENU_EXIT:
    {
     // PostQuitMessage(0);
		  OnCancel();
      //CloseWindow();
     // CDHtmlDialog::OnOK();
     // PostMessage(WM_CLOSE);
     // PostMessage(WM_DESTROY);
     // PostQuitMessage(0);
    }
    break;

  };
}

void CYYContainerDlg::OnNavigateComplete(LPDISPATCH pDisp, LPCTSTR szUrl)
{
  
  D_PRINT("CYYContainerDlg::OnNavigateComplete Begin\r\n" );
  D_PRINT( ("szUrl=[%s]  \r\n",  szUrl ));
	CDHtmlDialog::OnNavigateComplete(pDisp, szUrl);
	CString strUrl = CString(szUrl).MakeLower();
	if (/*strUrl.Find(D_MAIN_HTML)>=0 ||*/ strUrl.Find("pop.html")>=0){
		this->SetWindowPos(NULL, 0, 0,  552,504, SWP_NOMOVE);
		CString title;
		title.Format("%s(%u)", m_strUserAlias,  m_nUserID );
		this->SetWindowText(title);
		CString param;
		param.Format("%d;%u;%s%u;", (int)em_SetUserInfo, m_nUserID, m_strUserAlias , m_nYYmeetingUserID);
     ShowWindow(SW_SHOW);
		CStringArray params;
		// strParam.Format("%d;%d", /*em_sizeSLLeftPanelWindow,*/ 0, 0);
		params.Add(param);
		CallJScript("yCallFromPop",params);
  }else if (strUrl.Find("index.html")>=0|| strUrl.Find("index_en.html")>=0 ){
     //dock dlg
 	  AddMagneticDialog(m_pDlgLocalVideo );

 	  m_pDlgLocalVideo->EnableMagnetic(DKDLG_LEFT,this);
    //m_pDlgLocalVideo->ShowWindow(SW_SHOW);
    CStringArray params;
    CString strParam;
    strParam.Format("20;12;%s", (const char*) m_strTalker );
    params.Add(strParam);
   // MessageBox(strParam);
    CallJScript("yIMDlgControl",params);
    //CallJScript("yCallFromPop",params);

   //end

  }

  D_PRINT("CYYContainerDlg::OnNavigateComplete End\n");
}
void CYYContainerDlg::OnDocumentComplete(LPDISPATCH pDisp, LPCTSTR szUrl)
{
  D_PRINT(  "CYYContainerDlg::OnDocumentComplete begin" );
  D_PRINT( ("CYYContainerDlg::OnDocumentComplete %s\n", szUrl) );
	CDHtmlDialog::OnDocumentComplete(pDisp, szUrl);
  D_PRINT("CYYContainerDlg::OnDocumentComplete end \n");
}
BOOL CYYContainerDlg::OnInitDialog()
{
	CDHtmlDialog::OnInitDialog();
  D_PRINT("CYYContainerDlg::OnInitDialog\n");
   
  ModifyStyle(WS_MAXIMIZEBOX, 0); 
  ModifyStyleEx( WS_EX_TOPMOST,0); 
  ////CStringArray params;
  ////CString strParam = "";
  ////strParam.Format("20;10;%u", (unsigned int)GetSafeHwnd() );//em_msgfromcontainer
  ////params.Add(strParam);
  ////CallJScript("yIMDlgControl",params);

  //ShowWindow(SW_HIDE);
//ModifyStyleEx(WS_EX_APPWINDOW, WS_EX_TOOLWINDOW);

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
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	//SetIcon(m_hIcon, FALSE);		// 设置小图标
  SetExternalDispatch(GetIDispatch(TRUE));    //将浏览器控件的扩展接口设置为对话框自身的IDispatch
  if ( m_emStyle == em_standard){
    m_pTrayIcon = new CSimpleTray();
    m_pTrayIcon->SetIcon( m_hIcon ); 
    m_pTrayIcon->SetTooltip("未登录!");
    m_pTrayIcon->Show();
	  // TODO: 在此添加额外的初始化代码
 	  //this->Navigate(D_MAIN_HTML);
    CString strURL;// "http://127.0.0.1:5666/index.html";
    //LANGID  id = GetUserDefaultUILanguage();
    //if (id == 2052)
      strURL.Format("http://127.0.0.1:%d/index.html", m_hp.httpPort);
    //else
    //  strURL.Format("http://127.0.0.1:%d/index_en.html", m_hp.httpPort);

   this->Navigate(strURL );
	  
    this->SetWindowPos(NULL, 0, 0,  D_DEFAULT_W,D_DEFAULT_H, SWP_NOMOVE);
  } 
 

	RECT workrect;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workrect, 0);
	int workwidth = workrect.right -  workrect.left;
	int workheight = workrect.bottom - workrect.top ;
  CRect r;
  GetWindowRect(r);
  MoveWindow(( workwidth -r.Width()) /2,(workheight- r.Height())/2, r.Width(), r.Height());

  D_PRINT("CYYContainerDlg::OnInitDialog OK \n");
  //ShowWindow(SW_HIDE);
 	return TRUE;  // 除非设置了控件的焦点，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。
LRESULT CYYContainerDlg::OnTrayNotify(WPARAM wParam, LPARAM lParam)
{	// switch lParam, which contains the event that occured
	// wParam would hold the icon's ID, but since we have
	// only one icon, we don't need to check that
	switch ( lParam )
	{
	case WM_LBUTTONDOWN:
		{	// left click, activate window
			// we would do here the same thing as if
			// the user clicked the "show dialog" menu
			// so don't duplicate the code
      // activate our window
	      SetForegroundWindow();
	      ShowWindow( SW_SHOW );

          // if it is minimized restore it
	      if ( IsIconic() )
          {
		      ShowWindow( SW_RESTORE );
	      }
		}
		break;
	case WM_RBUTTONUP:
		{	// right click, show popup menu
			CMenu temp;
			CMenu *popup;
			CPoint loc;
			GetCursorPos( &loc );
          LANGID  id = GetUserDefaultUILanguage();
   
      if (id != 2052)
        temp.LoadMenu( IDR_MENU1_EN );
      else
			  temp.LoadMenu( IDR_MENU1 );
			popup = temp.GetSubMenu( 0 );
			popup->TrackPopupMenu( TPM_LEFTALIGN, loc.x, loc.y, this );
		}
		break;
	default:
		// do nothing
		break;
	}
  return 0;
}


void CYYContainerDlg::OnPaint() 
{
	if (IsIconic())
	{
		//CPaintDC dc(this); // 用于绘制的设备上下文

		//SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		//// 使图标在工作矩形中居中
		//int cxIcon = GetSystemMetrics(SM_CXICON);
		//int cyIcon = GetSystemMetrics(SM_CYICON);
		//CRect rect;
		//GetClientRect(&rect);
		//int x = (rect.Width() - cxIcon + 1) / 2;
		//int y = (rect.Height() - cyIcon + 1) / 2;

		//// 绘制图标
		//dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDHtmlDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标显示。
HCURSOR CYYContainerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
HRESULT CYYContainerDlg::OnButtonOK(IHTMLElement* /*pElement*/)
{
//	OnOK();
	return S_OK;
}

HRESULT CYYContainerDlg::OnButtonCancel(IHTMLElement* pElement)
{
	//OnCancel();
  //pElement->Release();
	return S_OK;
}
bool CYYContainerDlg::GetJScript(CComPtr<IDispatch>& spDisp)
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


CComVariant CYYContainerDlg::CallJScript(const CString strFunc,
                                  const CStringArray&
                                        paramArray)
{
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
  CComVariant vaResult;
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

void CYYContainerDlg::OnOK()
{

}

void CYYContainerDlg::OnCancel()
{
	//ASSERT_VALID(this);
 // CloseWindow();
  SendMessage(WM_CLOSE);
  DestroyWindow();
	// Common dialogs do not require ::EndDialog
	//Default();
 // CDHtmlDialog::OnCancel();
}
void CYYContainerDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
  // TODO: Add your message handler code here and/or call default

  CDHtmlDialog::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CYYContainerDlg::OnSize(UINT nType, int cx, int cy)
{
  CDHtmlDialog::OnSize(nType, cx, cy);
  //if (m_emStyle == em_standard) {
	 // RECT workrect;
	 // SystemParametersInfo(SPI_GETWORKAREA, 0, &workrect, 0);
	 // int workwidth = workrect.right -  workrect.left;
	 // //int workheight = workrect.bottom - workrect.top;
  //  CRect rc;
  //  GetWindowRect(rc);
  //  if (workwidth != rc.Width()){
  //    CStringArray params;
  //    CString strParam = "";
  //    strParam.Format("%d;%d", /*em_sizeSLLeftPanelWindow,*/ cx, cy);
  //    params.Add(strParam);
  //    CallJScript("ySizeWindowForLeftPanel",params);
  //  }
  // 
  //}
    // TODO: Add your message handler code here
}

void CYYContainerDlg::OnClose()
{
  if (m_emStyle == em_standard)
  {

    //nothing todo
    SetWindowText("");
    if (m_pTrayIcon){
      m_pTrayIcon->Hide();
      delete m_pTrayIcon;m_pTrayIcon =NULL;
    }
    //CStringArray params;
    //params.Add("");
    //CallJScript("yDestroy",params);

  }else if (m_emStyle== em_popIM){
    
    //COPYDATASTRUCT cpd;
    //cpd.dwData =  em_ClosingPop;
    //cpd.cbData  =  m_nUserID ;            //传递的数据长度
    // cpd.lpData    =  0;  //传递的数据地址
    //::SendMessage( GetYYMeetingWindow(), WM_COPYDATA, 0, (LPARAM) & cpd );

  }
  if (m_pDlgLocalVideo &&m_pDlgLocalVideo->GetSafeHwnd() ){
    m_pDlgLocalVideo->ShowWindow(SW_HIDE);
  }
  ShowWindow(SW_HIDE);
  
  CDHtmlDialog::OnClose();
}

void CYYContainerDlg::OnDestroy()
{

  CDHtmlDialog::OnDestroy();
  // TODO: Add your message handler code here and/or call default

 }

BOOL CYYContainerDlg::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* p)
{
   //MessageBox(((const char*)p->lpData)+1);
  // TODO: Add your message handler code here and/or call default
  switch(p->dwData ){
    case em_IMMessage_received://execute in pop dlg
      {
        CStringArray params;
        CString strParam = "";
        char* pp = new char[p->cbData+1];  
        memcpy(pp, p->lpData, p->cbData);
        pp[ p->cbData]=0;
        strParam.Format("%d;%s", 1  /*em_IMMessage*/, pp );
        params.Add(strParam);
        CallJScript("yCallFromPop",params);
        delete[] pp;
      }break;
    case em_IMMessage_sending://execute in standard dlg
      {
        CStringArray params;
        CString strParam = (const char*)p->lpData;
        params.Add(strParam);
        CallJScript("ySendIM",params);
      }break;
    case em_LoadSettingessage_request://execute in standard dlg
      {
    //    CComVariant result= CallJScript("yOptLoadSetting",params);//get from avtivex center
      //  CString strResult =result.bstrVal;

      }
    case em_ClosePop://execute in pop dlg
      {
        this->CloseWindow();
      }break;
    case em_ClosingPop://execute in standarl dlg
      {
        CStringArray params;
        CString strParam;
        strParam.Format("%u;%u", 0/*IMDlgControl 0*/, p->cbData);
        params.Add(strParam);
        CallJScript("yIMDlgControl",params);
         
      }break;
  };
  
  return CDHtmlDialog::OnCopyData(pWnd, p);
}

void CYYContainerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	// TODO: Add your message handler code here and/or call default
	switch(nID)
	{
	case SC_CLOSE:
		{

      if (m_emStyle == em_standard){
   // CStringArray params;
   // CString strParam="6;0;#0";//提示是否退出程序
   //// strParam.Format("2000;6;#1", 0/*IMDlgControl 0*/, p->cbData);
   // params.Add(strParam);
   // CallJScript("yIMDlgControl",params);
        if (MessageBox( "是否退出程序?","提示", MB_YESNO|MB_ICONINFORMATION) == IDYES)
        {
          OnCancel();
        }
			 // ShowWindow(SW_HIDE);
			  return;
      }
		}
		break;
  case SC_MINIMIZE:
    {
      ShowWindow(SW_HIDE);
      ShowLocalVideoDlg(false);
    //  m_pDlgLocalVideo->ShowWindow(SW_HIDE);
      return ;
    }break;
	};
	CDHtmlDialog::OnSysCommand(nID, lParam);
}

BOOL CYYContainerDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
  if(pMsg->message==WM_KEYDOWN&&pMsg->wParam==VK_RETURN){
    //BOOL b=FALSE;
        return FALSE;
  }
  if(pMsg->message==WM_KEYDOWN&&pMsg->wParam==VK_ESCAPE)     
      return TRUE;
 /* if (pMsg->message==WM_KEYDOWN) 
    return TRUE;*/
	return CDHtmlDialog::PreTranslateMessage(pMsg);
}

 

BOOL CYYContainerDlg::DestroyWindow()
{
  if (m_emStyle == em_standard)
  {//nothing todo
#ifdef _DEBUG   
    int nRet = UnregisterHotKey(GetSafeHwnd(), m_nHotKeyID[0]);   
    if(!nRet)  
        AfxMessageBox(_T("faile to UnregisterHotKey  "));  
    nRet = UnregisterHotKey(GetSafeHwnd(), m_nHotKeyID[1]);   
    //if(!nRet)  
    //    AfxMessageBox(_T("UnregisterHotKey 1 false"));  
#else   
    UnregisterHotKey(NULL, m_nHotKeyID[0]);   
   // UnregisterHotKey(GetSafeHwnd(), m_nHotKeyID[1]);   
#endif 
    GlobalDeleteAtom(m_atom1);

    CStringArray params;
    params.Add("");
    CallJScript("yDestroy",params);
  }
  // TODO: Add your specialized code here and/or call the base class
  for(MAP_PROMPTS::iterator i= m_mapPrompts.begin();i!= m_mapPrompts.end();i++){
   delete i->second ;
  }
  m_mapPrompts.clear();
  delete m_pDlgLocalVideo;
  m_pDlgLocalVideo=0;
  return CDHtmlDialog::DestroyWindow();
}

 

int CYYContainerDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  if (CDHtmlDialog::OnCreate(lpCreateStruct) == -1)
    return -1;
  // ::SetClassLong(this->m_hWnd, GCL_HBRBACKGROUND, (LONG)GetStockObject(BLACK_BRUSH));
 
 m_nHotKeyID[0] =m_atom1=  GlobalAddAtom("my first hotkey");
 m_nHotKeyID[1]=2000;
 m_nHotKeyID[2]=3000;
  SetTimer(1,8000,NULL);
#ifdef _DEBUG //debug版本   
    int nRet =     RegisterHotKey(NULL,m_nHotKeyID[0],MOD_CONTROL|MOD_ALT,'X'); //热键 ctrl + d   
    //if(!nRet){  
   //     AfxMessageBox(_T("Failed to register HotKey."));  
   // }
  #else //release版本   
    RegisterHotKey(GetSafeHwnd(),m_nHotKeyID[0],MOD_CONTROL|MOD_ALT,'X'); //热键 ctrl + d   
    //RegisterHotKey(GetSafeHwnd(),m_nHotKeyID[1],MOD_CONTROL|MOD_ALT,'c'); //热键 ctrl + d   
    //RegisterHotKey(GetSafeHwnd(),m_nHotKeyID[1],MOD_ALT,'M'); //热键 alt + m   
#endif   
        RegisterHotKey(NULL,m_nHotKeyID[0],MOD_CONTROL|MOD_ALT,'A'); //热键 ctrl + d   

  // TODO:  Add your specialized creation code here
  return 0;
}

void CYYContainerDlg::OnTimer(UINT nIDEvent)
{
  // TODO: Add your message handler code here and/or call default
  if (nIDEvent == 1){
    if (this->IsWindowVisible()== FALSE){
      this->ShowWindow(SW_SHOW); KillTimer(1);
    }else{
      KillTimer(1);
    }
  }
  CDHtmlDialog::OnTimer(nIDEvent);
}

LRESULT CYYContainerDlg::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
  // TODO: Add your specialized code here and/or call the base class
  if (message == WM_HOTKEY)
  {
    OnHotKey(wParam, lParam);
  }else if ( message == 2002){
//    //prompt online status
    CString str = (const char*)wParam;
    int nStart =0;
   // str.Tokenize(";", nStart);
    CString resToken=   str.Tokenize(";",nStart);  
    int uid=0, status=255, image;
    CString strAlias;
    int i=0;
    while   (resToken   !=   "")   
    {   
      //printf("Resulting   token:   %s\n",   resToken);   
      if (i== 0){
        uid = atoi(resToken);
      }else if (i==1){
        status = atoi(resToken);
      }else if (i==2){
        image = atoi(resToken);
      }else if (i==3){
        strAlias =resToken;
      }
      i++;
      resToken=   str.Tokenize(";",nStart);   
    };   
    CString strTitle;strTitle.Format("%s(%u)",(const char*)strAlias ,uid );
     m_pTrayIcon->SetTooltip(  strTitle );
//    if (uid>0 && status == CLIENT_NORMAL){
//      CPromptDlg* pDlg=NULL;
//      MAP_PROMPTS::iterator iter= m_mapPrompts.find(uid);
//      if (iter!= m_mapPrompts.end()  ){
//      iter->second->m_status= status;
//        iter->second->SetPrompt(strAlias);
//        //iter->second->ShowWindow(SW_SHOW);
//        return TRUE;//delete iter->second ;
//
//      }
//      //
//       
//      pDlg = new CPromptDlg();
//      m_mapPrompts[uid] = pDlg;
//      pDlg->m_uid = uid;
//      pDlg->m_status= status;
//      pDlg->m_image= image;
//      pDlg->m_strAlias = strAlias;
//      pDlg->m_ower= this;
//#if 0
//      AfxBeginThread(ThreadFunc, (LPVOID)pDlg);
//#else
//      
//      pDlg->Create(IDD_DIALOG_PROMPT_ONLINE);
//      pDlg->ShowWindow(SW_SHOW);
//#endif
//
//
       
 //   }
  }
  return CDHtmlDialog::DefWindowProc(message, wParam, lParam);
}
void CYYContainerDlg::OnMoving(UINT fwSide, LPRECT pRect)
{
	CDialog::OnMoving(fwSide, pRect);

	CRect dlgRect;
	for (size_t nIndex=0;nIndex < m_dkDialogs.size();nIndex++)
	{
		if ((m_dkDialogs[nIndex].bDocked) &&
			m_dkDialogs[nIndex].pWnd->IsWindowVisible())
		{
		
			m_dkDialogs[nIndex].pWnd->GetWindowRect(dlgRect);
			
			switch(m_dkDialogs[nIndex].nEdge) 
			{
			case DKDLG_LEFT:
				if (m_dkDialogs[nIndex].delta == 0)
					m_dkDialogs[nIndex].delta = pRect->top - dlgRect.top;
				dlgRect.MoveToXY(pRect->left-(dlgRect.right-dlgRect.left),(int)pRect->top-m_dkDialogs[nIndex].delta);
				break;
			case DKDLG_RIGHT:
				if (m_dkDialogs[nIndex].delta == 0)
					m_dkDialogs[nIndex].delta = pRect->top - dlgRect.top;
				dlgRect.MoveToXY(pRect->right,(int)pRect->top-m_dkDialogs[nIndex].delta);
				break;
			case DKDLG_TOP:
				if (m_dkDialogs[nIndex].delta == 0)
					m_dkDialogs[nIndex].delta = pRect->left - dlgRect.left;
				dlgRect.MoveToXY((int)pRect->left-m_dkDialogs[nIndex].delta,pRect->top-(dlgRect.bottom-dlgRect.top));
				break;
			case DKDLG_BOTTOM:
				if (m_dkDialogs[nIndex].delta == 0)
					m_dkDialogs[nIndex].delta = pRect->left - dlgRect.left;
				dlgRect.MoveToXY((int)pRect->left-m_dkDialogs[nIndex].delta,pRect->bottom);
				break;
			default:
				dlgRect;
			}

      ((CLocalVideoDlg*)m_dkDialogs[nIndex].pWnd)->MoveMagDialog(dlgRect,TRUE,TRUE);
		}
	}


	// Magnetic field

	if (m_pMagParentDlg != NULL)
	{
		CPoint curPl,curPr,curPt,curPb;
		int rHight,rWidth;
		CRect rectParent,rectLeft,rectRight,rectTop,rectBottom;

		rHight	= 50;
		rWidth	= 50;

		m_pMagParentDlg->GetWindowRect(rectParent);

		// Magnetic fields!
		rectRight	= CRect(rectParent.right-rHight,rectParent.top-rWidth,
			rectParent.right+rHight,rectParent.bottom+rWidth);
		rectLeft	= CRect(rectParent.left-rHight,rectParent.top-rWidth,
			rectParent.left+rHight,rectParent.bottom+rWidth);
		rectTop		= CRect(rectParent.left-rWidth,rectParent.top-rHight,
			rectParent.right+rWidth,rectParent.top+rHight);
		rectBottom	= CRect(rectParent.left-rWidth,rectParent.bottom-rHight,
			rectParent.right+rWidth,rectParent.bottom+rHight);

		curPl		= CPoint(pRect->left,pRect->top+(pRect->bottom-pRect->top)/2);		//Left
		curPr		= CPoint(pRect->right,pRect->top+(pRect->bottom-pRect->top)/2);		//Right
		curPt		= CPoint(pRect->left+(pRect->right-pRect->left)/2,pRect->bottom);	//Top
		curPb		= CPoint(pRect->left+(pRect->right-pRect->left)/2,pRect->top);		//Bottom

		if (m_bDocked)
		{
			if ((nEdge == DKDLG_RIGHT) && 
				!rectRight.PtInRect(curPl))
			{
				m_pMagParentDlg->UnDockMagneticDialog(this);
				nEdge		= DKDLG_NONE;
				m_bDocked	= FALSE;
				m_iDelta	= 0;
			}
			if ((nEdge == DKDLG_LEFT )&& 
				!rectLeft.PtInRect(curPr))
			{
				m_pMagParentDlg->UnDockMagneticDialog(this);
				nEdge		= DKDLG_NONE;
				m_bDocked	= FALSE;
				m_iDelta	= 0;
			}
			if ((nEdge == DKDLG_TOP )&& 
				!rectTop.PtInRect(curPt))
			{
				m_pMagParentDlg->UnDockMagneticDialog(this);
				nEdge		= DKDLG_NONE;
				m_bDocked	= FALSE;
				m_iDelta	= 0;
			}
			if ((nEdge == DKDLG_BOTTOM )&& 
				!rectBottom.PtInRect(curPb))
			{
				m_pMagParentDlg->UnDockMagneticDialog(this);
				nEdge		= DKDLG_NONE;
				m_bDocked	= FALSE;
				m_iDelta	= 0;
			}

		}
		else
		{
			if ((m_dwMagType == DKDLG_RIGHT || m_dwMagType == DKDLG_ANY) && 
				rectRight.PtInRect(curPl))
			{
				m_pMagParentDlg->DockMagneticDialog(this,DKDLG_RIGHT);
				nEdge		= DKDLG_RIGHT;	
				m_bDocked	= TRUE;
			}

			if ((m_dwMagType == DKDLG_LEFT || m_dwMagType == DKDLG_ANY) && 
				rectLeft.PtInRect(curPr))
			{
				m_pMagParentDlg->DockMagneticDialog(this,DKDLG_LEFT);
				nEdge		= DKDLG_LEFT;	
				m_bDocked	= TRUE;
			}

			if ((m_dwMagType == DKDLG_TOP || m_dwMagType == DKDLG_ANY) && 
				rectTop.PtInRect(curPt))
			{
				m_pMagParentDlg->DockMagneticDialog(this,DKDLG_TOP);
				nEdge		= DKDLG_TOP;	
				m_bDocked	= TRUE;
			}
			if ((m_dwMagType == DKDLG_BOTTOM || m_dwMagType == DKDLG_ANY) && 
				rectBottom.PtInRect(curPb))
			{
				m_pMagParentDlg->DockMagneticDialog(this,DKDLG_BOTTOM);
				nEdge		= DKDLG_BOTTOM;	
				m_bDocked	= TRUE;
			}


		}

	}

}

//void CYYContainerDlg::EnableMagnetic(DWORD dwMagType,CYYContainerDlg* pMagParentDlg)
//{
//	m_dwMagType		= dwMagType;
//	m_pMagParentDlg	= pMagParentDlg;
//}

void CYYContainerDlg::AddMagneticDialog(CLocalVideoDlg* pDialog,BOOL bDocked,DWORD dwMagWhere)
{
	dk_window dkWnd;

	dkWnd.pWnd		= pDialog;
	dkWnd.dwMagType = DKDLG_ANY;
	dkWnd.bDocked	= bDocked;
	dkWnd.nEdge		= dwMagWhere;
	dkWnd.delta		= 0;
	m_dkDialogs.push_back(dkWnd);

	pDialog->UpdateDockData(dkWnd);
				DockMagneticDialog(pDialog,DKDLG_LEFT);
				//pDialog->nEdge		= DKDLG_LEFT;	
				pDialog->m_bDocked	= TRUE;

}

void CYYContainerDlg::DockMagneticDialog(CDialog* pDialog,DWORD nEdge)
{
	for (size_t nIndex=0;nIndex < m_dkDialogs.size();nIndex++)
	{
		if (m_dkDialogs[nIndex].pWnd == pDialog)
		{
			m_dkDialogs[nIndex].nEdge	= nEdge;
			m_dkDialogs[nIndex].bDocked = TRUE;
		}
	}
}

void CYYContainerDlg::UnDockMagneticDialog(CDialog* pDialog)
{
	for (size_t nIndex=0;nIndex < m_dkDialogs.size();nIndex++)
	{
		if (m_dkDialogs[nIndex].pWnd == pDialog)
		{
			m_dkDialogs[nIndex].nEdge	= DKDLG_NONE;
			m_dkDialogs[nIndex].bDocked = FALSE;
			m_dkDialogs[nIndex].delta	= 0;
		}
	}
}


void CYYContainerDlg::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
	if ((m_bDocked) &&					// This is a fix on movement... 
		(m_pMagParentDlg != NULL) &&	// I need it only if the windows is docked and I'm trying to move it:
		(m_bDisablePosFix == FALSE) &&	// when this window is dragged by parent window I don't need this fix
		IsWindowVisible())
	{
		CRect tmpRect,rectParent;
		GetWindowRect(tmpRect);
		m_pMagParentDlg->GetWindowRect(rectParent);
		if (nEdge == DKDLG_RIGHT)
		{
			m_iDelta = rectParent.top - tmpRect.top;
			tmpRect.MoveToXY(rectParent.right,lpwndpos->y);
		}
		if (nEdge == DKDLG_LEFT )
		{
			m_iDelta = rectParent.top - tmpRect.top;
			tmpRect.MoveToXY(rectParent.left-(tmpRect.right-tmpRect.left),lpwndpos->y);
		}
		if (nEdge == DKDLG_TOP )
		{
			m_iDelta = rectParent.left - tmpRect.left;
			tmpRect.MoveToXY(lpwndpos->x,rectParent.top-(tmpRect.bottom-tmpRect.top));
		}
		if (nEdge == DKDLG_BOTTOM )
		{
			m_iDelta = rectParent.left - tmpRect.left;
			tmpRect.MoveToXY(lpwndpos->x,rectParent.bottom);
		}
				
		lpwndpos->x		= tmpRect.left;
		lpwndpos->y		= tmpRect.top;
		lpwndpos->cx	= tmpRect.Width();
		lpwndpos->cy	= tmpRect.Height();

		m_pMagParentDlg->UpdateMagPosition(this);
	}
	m_bDisablePosFix = FALSE;

	CDialog::OnWindowPosChanging(lpwndpos);

}

void CYYContainerDlg::MoveMagDialog(LPCRECT lpRect, BOOL bRepaint, BOOL bDisablePosFix)
{
	m_bDisablePosFix = bDisablePosFix;
	MoveWindow(lpRect,bRepaint);
}

void CYYContainerDlg::UpdateMagPosition(CDialog* pDialog)
{
	for (size_t nIndex=0;nIndex < m_dkDialogs.size();nIndex++)
	{
		if (m_dkDialogs[nIndex].pWnd == pDialog)
		{
			m_dkDialogs[nIndex].delta	= 0;
		}
	}
}
void CYYContainerDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialog::OnShowWindow(bShow, nStatus);
	if (bShow && m_bDocked &&
		(m_pMagParentDlg != NULL))
	{
		CRect paRect,dlgRect;
		m_pMagParentDlg->GetWindowRect(paRect);
		GetWindowRect(dlgRect);

		switch(nEdge) 
		{
		case DKDLG_LEFT:
			dlgRect.MoveToXY(paRect.left-(dlgRect.right-dlgRect.left),(int)paRect.top-m_iDelta);
			break;
		case DKDLG_RIGHT:
			dlgRect.MoveToXY(paRect.right,(int)paRect.top-m_iDelta);
			break;
		case DKDLG_TOP:
			dlgRect.MoveToXY((int)paRect.left-m_iDelta,paRect.top-(dlgRect.bottom-dlgRect.top));
			break;
		case DKDLG_BOTTOM:
			dlgRect.MoveToXY((int)paRect.left-m_iDelta,paRect.bottom);
			break;
		default:
			dlgRect;
		}

		MoveWindow(dlgRect);
	}
}

void CYYContainerDlg::UpdateDockData(dk_window dkdata)
{
	m_dwMagType	= dkdata.dwMagType;
	nEdge		= dkdata.nEdge;
	m_bDocked	= dkdata.bDocked;
}
void CYYContainerDlg::OnKeyDownVK_YYKey( WPARAM wParam , LPARAM lParam)
{


  CStringArray params;
  
  if (wParam == VK_ESCAPE )
  {
    CString strParam = "20;9";
    //strParam.Format("20;0;%u", (unsigned int)m_pDlgLocalVideo->GetSafeHwnd() );//em_msgfromcontainer
    params.Add(strParam);
    CallJScript("yIMDlgControl",params);	 
     
  }else  //ctrl 0-9
  {
    CString strParam;
    strParam.Format( "20;11;%d", wParam-0x30);
    params.Add(strParam);
    CallJScript("yIMDlgControl",params);	 
    
  }


 
}
void CYYContainerDlg::DestroyPrompt(unsigned uid)
{
  MAP_PROMPTS::iterator i= m_mapPrompts.find(uid);
  if (i!= m_mapPrompts.end() ){
    i->second->ShowWindow(SW_HIDE);
    //delete i->second ;
    //m_mapPrompts.erase(i);

  }
}

CYYContainerDlg::~CYYContainerDlg()
{
  mwServerShutdown(&m_hp);

}
void CYYContainerDlg::OnFinalRelease()
{
  // TODO: Add your specialized code here and/or call the base class
  D_PRINT(" CYYContainerDlg::OnFinalRelease\n");
  CDHtmlDialog::OnFinalRelease();
}

void CYYContainerDlg::OnBeforeNavigate(LPDISPATCH pDisp, LPCTSTR szUrl)
{
  // TODO: Add your specialized code here and/or call the base class

  CDHtmlDialog::OnBeforeNavigate(pDisp, szUrl);
}


void CYYContainerDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
  // TODO: Add your message handler code here and/or call default
 

  CDHtmlDialog::OnLButtonDown(nFlags, point);
}


void CYYContainerDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
  // TODO: Add your message handler code here and/or call default

  CDHtmlDialog::OnLButtonUp(nFlags, point);
}
int GetTrayWindowHeight()
{
  HWND TastWnd=FindWindow( "shell_traywnd", NULL); 
  CRect recttray(0,0,0,0); 
  if (TastWnd){
 //任务栏的尺寸 
    ::GetWindowRect(TastWnd, &recttray); 
  }
  return recttray.Height();
}
#define D_ROOM_WIN_W D_DEFAULT_W
#define D_ROOM_WIN_H D_DEFAULT_H
void CYYContainerDlg::SetWindowDefaultSize()
{
    CRect DesktoPrect ; 
   HWND hDesktop =  ::GetDesktopWindow();
  ::GetWindowRect( hDesktop,&DesktoPrect);
  int x,y,w=D_ROOM_WIN_W,h=D_ROOM_WIN_H;
  int workwidth = DesktoPrect.Width();// workrect.right -  workrect.left;
  int workheight = DesktoPrect.Height()-GetTrayWindowHeight();// workrect.bottom - workrect.top;

  //
  CRect rc;
  this->GetWindowRect(rc);
  if (workwidth == rc.Width() && workheight == rc.Height()){
    if (workwidth <  D_ROOM_WIN_W && workheight <  D_ROOM_WIN_H){
      MoveWindow(0,0,D_ROOM_WIN_W,D_ROOM_WIN_H);
    }else{

      x = (workwidth- D_ROOM_WIN_W)/2;
      y = (workheight- D_ROOM_WIN_H )/2  ;

      MoveWindow(x, y, D_ROOM_WIN_W, D_ROOM_WIN_H);
    }    
  }else{
    MoveWindow(0, 0, workwidth, workheight);
   // ShowWindow(SW_SHOWMINIMIZED);
  }


}

BOOL CYYContainerDlg::OnEraseBkgnd(CDC* pDC)
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
