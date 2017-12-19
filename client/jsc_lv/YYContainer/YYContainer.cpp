// YYContainer.cpp : 定义应用程序的类行为。
//

#include "stdafx.h"
#include "YYContainer.h"
#include "YYContainerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



char* GetAppPathForDoc()
{
  static bool bInit=false;
	static char MyDir[512]; 
  if (bInit == false){
    
  SHGetSpecialFolderPath(0,MyDir,CSIDL_PERSONAL,0); 
  bInit= true;
  //SHGetSpecialFolderPath(0,MyDir,CSIDL_COMMON_DOCUMENTS,0); 
  }
  return MyDir;

}
// CYYContainerApp

BEGIN_MESSAGE_MAP(CYYContainerApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()


// CYYContainerApp 构造

CYYContainerApp::CYYContainerApp()
{
	// TODO: 在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
  //CString str;
  //str.Format("%s\\yy\\cc.log", GetAppPathForDoc());
  //if((stream = freopen(str.GetBuffer(), "w", stdout)) == NULL)
  //   MessageBox(NULL, "stdout failed!","", MB_OK);


  // printf("init winmain\n");
}


// 唯一的一个 CYYContainerApp 对象

CYYContainerApp theApp;
CString GetAppPath()
{
	char szBuff[_MAX_PATH];
	::GetModuleFileName(NULL, szBuff, _MAX_PATH);
	char* pszFind = strrchr(szBuff, '\\');
	if(pszFind!=0 && pszFind != szBuff) {
		//*(pszFind-1) = '\0';
		*pszFind = '\0';
	}
	return szBuff;
}

// CYYContainerApp 初始化
bool  AutoWriteCCVisionINFO( )
{
   TCHAR*   sRoot= _T("SOFTWARE\\YYMeeting");
    HKEY hKey;
    LONG ret;
    DWORD dwSize=0;
   // 　RegCreateKeyEx(hCompanyKey,　_T("testreg"),　0,　REG_NONE,　EG_OPTION_NON_VOLATILE,　KEY_WRITE|KEY_READ,　NULL,&hAppKey,　&dw); 　
    ret = RegCreateKeyEx(HKEY_CURRENT_USER,sRoot,0,REG_NONE, REG_OPTION_NON_VOLATILE,KEY_WRITE|KEY_READ, NULL, &hKey, &dwSize);
  if(RegOpenKeyEx(HKEY_CURRENT_USER,sRoot,0,KEY_SET_VALUE,&hKey) == ERROR_SUCCESS){
 	  CString strValue = GetAppPath();                     // 0                    1   2                        3                                  4

    dwSize=strValue.GetLength();
	  ret=RegSetValueEx(hKey,(const char*)"CC_PATH",0,REG_SZ ,(BYTE*)strValue.GetBuffer(),dwSize);
	  if(ret==ERROR_SUCCESS)  {
      //
	  }
    RegCloseKey (hKey);
	  return true;
	}
  return false;
}

BOOL CYYContainerApp::InitInstance()
{
   printf("  InitInstance 1 \n");
	// 如果一个运行在 Windows XP 上的应用程序清单指定要
	// 使用 ComCtl32.dll 版本 6 或更高版本来启用可视化方式，
	//则需要 InitCommonControls()。否则，将无法创建窗口。
	InitCommonControls();

	CWinApp::InitInstance();
  printf("  InitInstance 2 \n");
	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		//return FALSE;
	}
	AfxEnableControlContainer();

	// 标准初始化
	// 如果未使用这些功能并希望减小
	// 最终可执行文件的大小，则应移除下列
	// 不需要的特定初始化例程
	// 更改用于存储设置的注册表项
	// TODO: 应适当修改该字符串，
	// 例如修改为公司或组织名
	SetRegistryKey(_T("CCVision应用程序"));
  CString strCmd = GetCommandLine();
  int nstart = 0;
   
  CString strTokened = strCmd.Tokenize(" ", nstart);
  
	CYYContainerDlg dlg;
   AutoWriteCCVisionINFO();
  //
  while(strTokened!=""){
   /* if (strTokened== "-uid"){
      dlg.m_emStyle= em_popIM;
      dlg.m_nUserID =   atoi(strCmd.Tokenize(" ",nstart ));  
    }else if (strTokened== "-uyymeetingid") {
      dlg.m_nYYmeetingUserID = atoi(strCmd.Tokenize(" ",nstart ));  
    }else if (strTokened== "-uname") {
      dlg.m_strUserAlias = strCmd.Tokenize(" ",nstart );  
    }else*/ 
    if (strTokened == "-talker" ){
      dlg.m_strTalker = strCmd.Tokenize(" ",nstart );  
    }
    //AfxMessageBox(dlg.m_strTalker);
    strTokened = strCmd.Tokenize(" ", nstart);  
  }

  m_pMainWnd = &dlg;
  if ( dlg.m_emStyle == em_popIM){
	  INT_PTR nResponse = dlg.DoModal();
	  if (nResponse == IDOK)
	  {
		  // TODO: 在此放置处理何时用“确定”来关闭
		  //对话框的代码
	  }
	  else if (nResponse == IDCANCEL)
	  {
		  // TODO: 在此放置处理何时用“取消”来关闭
		  //对话框的代码
	  }
  }else{
    dlg.Create(IDD_YYCONTAINER_DIALOG);

    HWND h = dlg.GetSafeHwnd();
    //dlg.UpdateWindow();
   // dlg.SetForegroundWindow();
    dlg.ShowWindow(SW_HIDE);
    MSG msg;
    
    while (/*m_hWnd!=NULL &&*/ GetMessage(&msg, NULL, 0, 0) /*&& msg.message != WM_QUIT*/  )
	  {
    //  if ( msg.message == WM_QUIT){
  /*      printf("  InitInstance WM_QUIT \n");
        fflush(stream);
 */ //    }
      // m_pDlg->ProcessMessageFilter(&msg);
      //if (!AfxPreTranslateMessage(&msg) )
      {
         
    /*    if (msg.message == WM_HOTKEY)
        {
          dlg.OnHotKey(msg.wParam, msg.lParam);
        }else if( msg.message==WM_KEYDOWN )
         {
           if (msg.wParam==VK_ESCAPE ||  
             ( (::GetKeyState   (   VK_CONTROL)   &   0x8000)  &&(msg.wParam == 0x53||(  msg.wParam> 0x30&& msg.wParam<= 0x39))    ) )
             dlg.OnKeyDownVK_YYKey(msg.wParam , msg.lParam );
         }*/
        
       
        /////
        { 
          TranslateMessage(&msg); 
          DispatchMessage(&msg); 
        }
      }
   
      
	  }
  } 
  //D_PRINT("  InitInstance ended \n");
  //if ()
  //fclose( stream);
	// 由于对话框已关闭，所以将返回 FALSE 以便退出应用程序，
	// 而不是启动应用程序的消息泵。
	return FALSE;
}
