// ..\czlibexe\YYWin32Thread.cpp : implementation file
//
#include "stdafx.h"

//#ifdef LPN_ACTIVEX_PRJ
////#include "..\YYSpriteX-2\resource.h"
////#include "..\lpMultiPeopleVideo\resource.h"
////#define IDD_DIALOG_REMOTE               130
//
//#include "RemoteDlg.h"
//
//#else
//#include "..\lpMultiPeopleVideo\resource.h"
//#include "RemoteDlg.h"
//#endif

#include "YYWin32Thread.h"
 
#include "promptdlg.h"

// CYYWin32ThreadDlg

IMPLEMENT_DYNCREATE(CYYWin32ThreadDlg, CWinThread)

CYYWin32ThreadDlg::CYYWin32ThreadDlg()
:CWinThread()
{
  m_bAutoDelete = FALSE;
  //CreateThread();
  //SuspendThread();
}

CYYWin32ThreadDlg::~CYYWin32ThreadDlg()
{
  
}

BOOL CYYWin32ThreadDlg::InitInstance()
{
	// TODO:  perform and per-thread initialization here
	return TRUE;
}
void CYYWin32ThreadDlg::Destroy()
{

}

int CYYWin32ThreadDlg::ExitInstance()
{
	// TODO:  perform any per-thread cleanup here
	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CYYWin32ThreadDlg, CWinThread)
END_MESSAGE_MAP()
int CYYWin32ThreadDlg:: Run( )
{
  m_pPromptDlg =new CPromptDlg();

	 //Main();
	 return 0;
}
bool CYYWin32ThreadDlg::WaitForTermination( ) const
{
	
  if (  m_hThread == NULL) {
    //PTRACE(3, "PWLib\tWaitForTermination short circuited");
    return true;
  }

  DWORD result;
  int retries = 10;
  while ((result = WaitForSingleObject(m_hThread, 0xffffffff)) != WAIT_TIMEOUT) {
    if (result == WAIT_OBJECT_0){
     // m_hThread =NULL;
      return true;
    }

    if (::GetLastError() != ERROR_INVALID_HANDLE) {
     // m_hThread =NULL;
      
      return true;
    }

    if (retries == 0){
     // m_hThread=NULL;
      return true;
    }

    retries--;
  }
 // PProcess::Current().m
//  m_hThread =NULL;
  return false;
}
// CYYWin32ThreadDlg message handlers
