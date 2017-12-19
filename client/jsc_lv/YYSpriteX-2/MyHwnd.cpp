// MyHwnd.cpp : implementation file
//

#include "stdafx.h"
#include "YYSpriteX-2.h"
#include "MyHwnd.h"
#include ".\myhwnd.h"
#include "YYSpriteX-2Ctrl.h"

// CMyHwnd

IMPLEMENT_DYNAMIC(CMyHwnd, CWnd)
CMyHwnd::CMyHwnd()
{
}

CMyHwnd::~CMyHwnd()
{
}


BEGIN_MESSAGE_MAP(CMyHwnd, CWnd)
END_MESSAGE_MAP()



// CMyHwnd message handlers


BOOL CMyHwnd::PreTranslateMessage(MSG* pMsg)
{
  // TODO: Add your specialized code here and/or call the base class

  return CWnd::PreTranslateMessage(pMsg);
}

LRESULT CMyHwnd::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
  // TODO: Add your specialized code here and/or call the base class
  CYYSpriteX2Ctrl::WndProc(GetSafeHwnd(), message, wParam,lParam);
  return CWnd::WindowProc(message, wParam, lParam);
}
