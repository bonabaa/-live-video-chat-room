// YYContainer.cpp : ����Ӧ�ó��������Ϊ��
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


// CYYContainerApp ����

CYYContainerApp::CYYContainerApp()
{
	// TODO: �ڴ˴���ӹ�����룬
	// ��������Ҫ�ĳ�ʼ�������� InitInstance ��
}


// Ψһ��һ�� CYYContainerApp ����

CYYContainerApp theApp;


// CYYContainerApp ��ʼ��

BOOL CYYContainerApp::InitInstance()
{
	// ���һ�������� Windows XP �ϵ�Ӧ�ó����嵥ָ��Ҫ
	// ʹ�� ComCtl32.dll �汾 6 ����߰汾�����ÿ��ӻ���ʽ��
	//����Ҫ InitCommonControls()�����򣬽��޷��������ڡ�
	InitCommonControls();

	CWinApp::InitInstance();
  
	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();

	// ��׼��ʼ��
	// ���δʹ����Щ���ܲ�ϣ����С
	// ���տ�ִ���ļ��Ĵ�С����Ӧ�Ƴ�����
	// ����Ҫ���ض���ʼ������
	// �������ڴ洢���õ�ע�����
	// TODO: Ӧ�ʵ��޸ĸ��ַ�����
	// �����޸�Ϊ��˾����֯��
	SetRegistryKey(_T("Ӧ�ó��������ɵı���Ӧ�ó���"));
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

	// ���ڶԻ����ѹرգ����Խ����� FALSE �Ա��˳�Ӧ�ó���
	// ����������Ӧ�ó������Ϣ�á�
	return FALSE;
}
