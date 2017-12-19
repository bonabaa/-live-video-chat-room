#pragma once


// CMyHwnd

class CMyHwnd : public CWnd
{
	DECLARE_DYNAMIC(CMyHwnd)

public:
	CMyHwnd();
	virtual ~CMyHwnd();

protected:
	DECLARE_MESSAGE_MAP()
public:
  virtual BOOL PreTranslateMessage(MSG* pMsg);
protected:
  virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
};


