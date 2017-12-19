// MyEdit.cpp : implementation file
// Download by http://www.codefans.net

#include "stdafx.h"
#include "MyEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Resource.h"

/////////////////////////////////////////////////////////////////////////////
// CMyEdit


CMyEdit::CMyEdit()
{
	m_bMove=TRUE;	
  BOOL b=	bitmap.LoadBitmap(IDB_BITMAP_CAPTURE);
  	BITMAP bmp;
	bitmap.GetBitmap(&bmp);
}

CMyEdit::~CMyEdit()
{
}


BEGIN_MESSAGE_MAP(CMyEdit, CEdit)
	//{{AFX_MSG_MAP(CMyEdit)
	ON_WM_CREATE()
	ON_WM_MOUSEMOVE()
	ON_WM_SETFOCUS()
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
  ON_WM_PAINT()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMyEdit message handlers




int CMyEdit::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{

	if (CEdit::OnCreate(lpCreateStruct) == -1)
		return -1;
		
	// TODO: Add your specialized creation code here
	return 0;
}

void CMyEdit::OnMouseMove(UINT nFlags, CPoint point) 
{
	
	
	// TODO: Add your message handler code here and/or call default
	//MessageBox("Mouse");
//***********************************************************************************
	
	CRect rect;
	GetWindowRect(&rect);

	int xScreen = GetSystemMetrics(SM_CXSCREEN);
	//int ySCreen = GetSystemMetrics(SM_CYSCREEN);

	if(m_bMove)
	{
		//移动到左上角
		MoveWindow(10,10,rect.Width(),rect.Height());
		m_bMove=FALSE;
	}
	else
	{
		//移动到右上角
		MoveWindow(xScreen-180,10,rect.Width(),rect.Height());
		m_bMove=TRUE;
	}
//**************************************************************************************
	CEdit::OnMouseMove(nFlags, point);
}

void CMyEdit::OnSetFocus(CWnd* pOldWnd) 
{
	CEdit::OnSetFocus(pOldWnd);
//**********************************************************************************
   //隐藏光标提示符,其实也用不着
	this->HideCaret();
//*********************************************************************************
	// TODO: Add your message handler code here	
}



HBRUSH CMyEdit::CtlColor(CDC* pDC, UINT nCtlColor) 
{
	// TODO: Change any attributes of the DC here
	
	// TODO: Return a non-NULL brush if the parent's handler should not be called
//***************************************************************************************
	//设置文字背景透明
 	pDC->SetBkMode(TRANSPARENT);
//**************************************************************************************
	return NULL;
}

BOOL CMyEdit::OnEraseBkgnd(CDC* pDC) 
{
	// TODO: Add your message handler code here and/or call default
//**************************************************************************************
    
	//取消文字复选
	this->SetSel(0,0);
	
	//设置位图背景

#if 1

	CDC dcCompatible;
	dcCompatible.CreateCompatibleDC(pDC);

	dcCompatible.SelectObject(&bitmap);

	CRect rect;
	GetClientRect(&rect);
  //pDC->StretchBlt(0,0,rect.Width(),rect.Height(),&dcCompatible,0,0, bmp.bmWidth, bmp.bmHeight,SRCCOPY);
	BOOL b= pDC->BitBlt(0,0,rect.Width(),rect.Height(),&dcCompatible,0,0,SRCCOPY);
   dcCompatible.DeleteDC();
#else
      CDC memDC;
    memDC.CreateCompatibleDC(pDC);
    memDC.SelectObject(&bitmap);
    BITMAP m_Bmp;
    bitmap.GetBitmap(&m_Bmp);
    CRect rect;
    GetClientRect(&rect);
//拉伸贴图
    pDC->StretchBlt(0,0,rect.Width() ,rect.Height(),&memDC,0,0,m_Bmp.bmWidth,m_Bmp.bmHeight,SRCCOPY);
    memDC.DeleteDC();
    return TRUE;

#endif
	return TRUE;
//*********************************************************************************
	//CEdit::OnEraseBkgnd(pDC);
}
void CMyEdit::OnEnChange()
{
//使无效
   // Invalidate();
}


void CMyEdit::OnPaint()
{
//  CPaintDC dc(this); // device context for painting
//        CDC memDC;
//    memDC.CreateCompatibleDC(&dc);
//    memDC.SelectObject(&bitmap);
//    BITMAP m_Bmp;
//    bitmap.GetBitmap(&m_Bmp);
//    CRect rect;
//    GetClientRect(&rect);
////拉伸贴图
//    dc.StretchBlt(0,0,rect.Width() ,rect.Height(),&memDC,0,0,m_Bmp.bmWidth,m_Bmp.bmHeight,SRCCOPY);
//    memDC.DeleteDC();
  // TODO: Add your message handler code here
  // Do not call CEdit::OnPaint() for painting messages
}
