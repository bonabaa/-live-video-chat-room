#pragma once


#include <afxwin.h>
// CYYWin32Thread

class CYYWin32ThreadDlg : public CWinThread
{
	DECLARE_DYNCREATE(CYYWin32Thread)

public:
	CYYWin32ThreadDlg();           // protected constructor used by dynamic creation
	virtual ~CYYWin32ThreadDlg();
	virtual int Run( );

public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual void Main( ) {};
	bool WaitForTermination() const;
  void Destroy();
public:
  //CString m_strAlias;
  //int m_uid, m_status, m_image;
  //CYYContainerDlg *m_ower;
  //CPromptDlg *m_pPromptDlg;

protected:
	DECLARE_MESSAGE_MAP()
};


