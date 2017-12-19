// YYContainer.cpp : ����Ӧ�ó��������Ϊ��
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


// CYYContainerApp ����

CYYContainerApp::CYYContainerApp()
{
	// TODO: �ڴ˴���ӹ�����룬
	// ��������Ҫ�ĳ�ʼ�������� InitInstance ��
  //CString str;
  //str.Format("%s\\yy\\cc.log", GetAppPathForDoc());
  //if((stream = freopen(str.GetBuffer(), "w", stdout)) == NULL)
  //   MessageBox(NULL, "stdout failed!","", MB_OK);


  // printf("init winmain\n");
}


// Ψһ��һ�� CYYContainerApp ����

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

// CYYContainerApp ��ʼ��
bool  AutoWriteCCVisionINFO( )
{
   TCHAR*   sRoot= _T("SOFTWARE\\YYMeeting");
    HKEY hKey;
    LONG ret;
    DWORD dwSize=0;
   // ��RegCreateKeyEx(hCompanyKey,��_T("testreg"),��0,��REG_NONE,��EG_OPTION_NON_VOLATILE,��KEY_WRITE|KEY_READ,��NULL,&hAppKey,��&dw); ��
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
	// ���һ�������� Windows XP �ϵ�Ӧ�ó����嵥ָ��Ҫ
	// ʹ�� ComCtl32.dll �汾 6 ����߰汾�����ÿ��ӻ���ʽ��
	//����Ҫ InitCommonControls()�����򣬽��޷��������ڡ�
	InitCommonControls();

	CWinApp::InitInstance();
  printf("  InitInstance 2 \n");
	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		//return FALSE;
	}
	AfxEnableControlContainer();

	// ��׼��ʼ��
	// ���δʹ����Щ���ܲ�ϣ����С
	// ���տ�ִ���ļ��Ĵ�С����Ӧ�Ƴ�����
	// ����Ҫ���ض���ʼ������
	// �������ڴ洢���õ�ע�����
	// TODO: Ӧ�ʵ��޸ĸ��ַ�����
	// �����޸�Ϊ��˾����֯��
	SetRegistryKey(_T("CCVisionӦ�ó���"));
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
		  // TODO: �ڴ˷��ô����ʱ�á�ȷ�������ر�
		  //�Ի���Ĵ���
	  }
	  else if (nResponse == IDCANCEL)
	  {
		  // TODO: �ڴ˷��ô����ʱ�á�ȡ�������ر�
		  //�Ի���Ĵ���
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
	// ���ڶԻ����ѹرգ����Խ����� FALSE �Ա��˳�Ӧ�ó���
	// ����������Ӧ�ó������Ϣ�á�
	return FALSE;
}
