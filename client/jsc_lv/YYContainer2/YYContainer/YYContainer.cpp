// YYContainer.cpp : 定义应用程序的类行为。
//

#include "stdafx.h"
#include "YYContainer.h"
#include "YYContainerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CYYContainerApp

BEGIN_MESSAGE_MAP(CYYContainerApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()


// CYYContainerApp 构造

CYYContainerApp::CYYContainerApp()
{
	// TODO: 在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
}


// 唯一的一个 CYYContainerApp 对象

CYYContainerApp theApp;


// CYYContainerApp 初始化

BOOL CYYContainerApp::InitInstance()
{
	// 如果一个运行在 Windows XP 上的应用程序清单指定要
	// 使用 ComCtl32.dll 版本 6 或更高版本来启用可视化方式，
	//则需要 InitCommonControls()。否则，将无法创建窗口。
	InitCommonControls();

	CWinApp::InitInstance();
  
	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();

	// 标准初始化
	// 如果未使用这些功能并希望减小
	// 最终可执行文件的大小，则应移除下列
	// 不需要的特定初始化例程
	// 更改用于存储设置的注册表项
	// TODO: 应适当修改该字符串，
	// 例如修改为公司或组织名
	SetRegistryKey(_T("应用程序向导生成的本地应用程序"));
  CString strCmd = GetCommandLine();
  int nstart = 0;
   
  CString strTokened = strCmd.Tokenize(" ", nstart);
  
	CYYContainerDlg dlg;
   
  //
  while(strTokened!=""){
    if (strTokened== "-uid"){
      dlg.m_emStyle= em_popIM;
      dlg.m_nUserID =   atoi(strCmd.Tokenize(" ",nstart ));  
    }else if (strTokened== "-uyymeetingid") {
      dlg.m_nYYmeetingUserID = atoi(strCmd.Tokenize(" ",nstart ));  
    }else if (strTokened== "-uname") {
      dlg.m_strUserAlias = strCmd.Tokenize(" ",nstart );  
    }    strTokened = strCmd.Tokenize(" ", nstart);  
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
    while (/*m_hWnd!=NULL &&*/ GetMessage(&msg, NULL, 0, 0) && msg.message != WM_QUIT  )
	  {
      // m_pDlg->ProcessMessageFilter(&msg);
      //if (!AfxPreTranslateMessage(&msg) )
      if (msg.message == WM_HOTKEY)
      {
        dlg.OnHotKey(msg.wParam, msg.lParam);
      }else
      {
        TranslateMessage(&msg); 
        DispatchMessage(&msg); 
      }
   
      
	  }
  }

	// 由于对话框已关闭，所以将返回 FALSE 以便退出应用程序，
	// 而不是启动应用程序的消息泵。
	return FALSE;
}
