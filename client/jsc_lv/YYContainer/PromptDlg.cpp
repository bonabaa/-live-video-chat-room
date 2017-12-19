// PromptDlg.cpp : implementation file
//

#include "stdafx.h"
#include "YYContainer.h"
#include "PromptDlg.h"
#include ".\promptdlg.h"
#include <windows.h>
#include "YYContainerDlg.h"




#include "Winuser.h"

// CPromptDlg dialog

IMPLEMENT_DYNAMIC(CPromptDlg, CDialog)
CPromptDlg::CPromptDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPromptDlg::IDD, pParent)
{
  m_bEraseBkgGroup = false;
  m_nTimeOutDestroy=0;
}

CPromptDlg::~CPromptDlg()
{
}

void CPromptDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK_CLOSE, m_btnClose);
}


BEGIN_MESSAGE_MAP(CPromptDlg, CDialog)
  ON_WM_ERASEBKGND()
  ON_WM_CREATE()
  ON_BN_CLICKED(IDOK, OnBnClickedOk)
  ON_WM_TIMER()
  ON_WM_CTLCOLOR()
  ON_BN_CLICKED(IDC_CHECK_CLOSE, OnBnClickedCheckClose)
  ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


// CPromptDlg message handlers

BOOL CPromptDlg::OnInitDialog()
{
  CDialog::OnInitDialog();
  
  m_bkBitmap.LoadBitmap(IDB_BITMAP_PROMPT);
  CBitmap& bitmap =m_bkBitmap; 
 
  //得到图片大小并调整窗口大小适应图片 
  bitmap.GetBitmap(&bitInfo); 
	//memset( &m_destinfo, 0, sizeof( m_destinfo ) );
	//m_destinfo.biSize = sizeof( m_destinfo );
	//m_destinfo.biBitCount = bitInfo.bmBitsPixel ;
	//m_destinfo.biCompression = MAKEFOURCC( 'I', '4', '2', '0' );
	//m_destinfo.biPlanes = 1;
	//m_destinfo.biWidth = bitInfo.bmWidth;
	//m_destinfo.biHeight = bitInfo.bmHeight;
	//m_destinfo.biSizeImage = m_destinfo.biWidth * m_destinfo.biHeight  *  8;/* m_destinfo.biBitCount*/
	m_btnClose.SetBitmaps(IDB_BITMAP_CLOSE2, RGB(255, 0, 255),IDB_BITMAP_CLOSE, RGB(255, 0, 255));
	m_btnClose.DrawTransparent(TRUE);
	m_btnClose.SizeToContent();


  //创建并保存DC 
  m_DC.CreateCompatibleDC(GetDC()); 
  m_oldBitmap = m_DC.SelectObject(&bitmap); 
  //设置窗口掩码颜色和模式 
  //首先获得掩码颜色 
  //COLORREF maskColor = RGB(153, 255, 0);//m_DC.GetPixel(0,0); 
  COLORREF maskColor = RGB(255, 0, 255);//m_DC.GetPixel(0,0); 

  #define LWA_COLORKEY  0x00000001 
  #define WS_EX_LAYERED  0x00080000 

  typedef BOOL (WINAPI *lpfnSetLayeredWindowAttributes)(HWND hWnd, 
                                  COLORREF crKey, 
                                  BYTE bAlpha, 
                                  DWORD dwFlags); 

  lpfnSetLayeredWindowAttributes SetLayeredWindowAttributes; 

  HMODULE hUser32 = GetModuleHandle("user32.dll"); 
  SetLayeredWindowAttributes = (lpfnSetLayeredWindowAttributes)GetProcAddress(hUser32,           "SetLayeredWindowAttributes");   SetWindowLong(GetSafeHwnd(), 
                            GWL_EXSTYLE, 
                            GetWindowLong(GetSafeHwnd(), 
                            GWL_EXSTYLE) | WS_EX_LAYERED); 

  SetLayeredWindowAttributes(GetSafeHwnd(), 
                              maskColor, 
                              0, 
                              LWA_COLORKEY); 

  FreeLibrary(hUser32); 
  // TODO:  Add extra initialization here
  SetTimer(1, 100,NULL);
  int w= bitInfo.bmWidth,h=bitInfo.bmHeight;
	RECT rect;
	SystemParametersInfo(SPI_GETWORKAREA,0,&rect,0);
	int x= m_x=rect.right-rect.left - w;
	int y=m_y=rect.bottom-rect.top - h;
  SetWindowPos(NULL, x,y, bitInfo.bmWidth, 0/*bitInfo.bmHeight*/, SWP_NOACTIVATE);
  SetPrompt(m_strAlias);
  GetDlgItem( IDC_STATIC_TITLE)->SetWindowPos(NULL, bitInfo.bmWidth/3, (bitInfo.bmHeight)/2,0,0, SWP_NOSIZE);
	CRect btnClose;
	GetDlgItem(IDC_CHECK_CLOSE)->GetClientRect(btnClose);
	//CRect NewbtnClose(w- btnClose.Width() -2 ), 0, );
	m_btnClose.SetWindowPos(NULL, w- btnClose.Width() -2 , 2, 0,0, SW_SHOW);
  m_bEraseBkgGroup = true;
  return TRUE;  // return TRUE unless you set the focus to a control
  // EXCEPTION: OCX Property Pages should return FALSE
}
void CPromptDlg::SetPrompt(CString strAlias)
{
  CString strPrompt= strAlias + " is online!";
  GetDlgItem( IDC_STATIC_TITLE)->SetWindowText(strPrompt);
  m_nTimeOutDestroy=0; 
  SetTimer(1, 100,NULL);
  SetWindowPos(NULL, m_x, m_y, bitInfo.bmWidth, 0/*bitInfo.bmHeight*/, SWP_NOACTIVATE);
  ShowWindow(SW_SHOW);
 

}
BOOL CPromptDlg::OnEraseBkgnd(CDC* pDC)
{
  // TODO: Add your message handler code here and/or call default
  if (m_bEraseBkgGroup){
	  BITMAP bmpInfo; 
	  m_bkBitmap.GetBitmap(&bmpInfo); 
	  CRect rect; 
    GetClientRect(&rect); 
        //////////////////////////////////// 
    pDC->BitBlt(0,0,rect.Width(),rect.Height(),&m_DC,0,0,SRCCOPY); 
	  return TRUE;
  }
	//if (m_DC.m_hDC !=NULL)
	//{
	//	BITMAP bmpInfo; 
	//	m_bkBitmap.GetBitmap(&bmpInfo); 
	//	CRect rect; 
	//	GetClientRect(&rect); 
	//			//////////////////////////////////// 
	//	//pDC->BitBlt(0,0,rect.Width(),rect.Height(),&m_DC,0,0,SRCCOPY); 
	//	pDC->StretchBlt(0,0,rect.Width(),rect.Height(),&m_DC,0,0, bmpInfo.bmWidth, bmpInfo.bmHeight,SRCCOPY);
 // 
 //   return TRUE;
	//}
  return CDialog::OnEraseBkgnd(pDC);
}

int CPromptDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  if (CDialog::OnCreate(lpCreateStruct) == -1)
    return -1;

  // TODO:  Add your specialized creation code here
  //AnimateWindow(2000,AW_ACTIVATE|AW_CENTER|AW_BLEND);

  return 0;
}

void CPromptDlg::OnBnClickedOk()
{
  // TODO: Add your control notification handler code here
  OnOK();
}

void CPromptDlg::OnTimer(UINT nIDEvent)
{
#define D_INTERVAL_DISP 9//h is 99
  // TODO: Add your message handler code here and/or call default
  if (nIDEvent == 1){
    if (m_nTimeOutDestroy++> 80){
      //this->DestroyWindow();
      KillTimer(nIDEvent);
      m_ower->DestroyPrompt(m_uid);
      return ;
    }else{
      CRect rc;
      GetWindowRect(rc);
      int y=rc.Height()+D_INTERVAL_DISP;
      if (rc.Height()< bitInfo.bmHeight){
        SetWindowPos(NULL, m_x, m_y +bitInfo.bmHeight- y, bitInfo.bmWidth, rc.Height()+D_INTERVAL_DISP/*bitInfo.bmHeight*/, SWP_NOACTIVATE);
      }
      
    }
  }
  CDialog::OnTimer(nIDEvent);
}

HBRUSH CPromptDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
  HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

  // TODO:  Change any attributes of the DC here
	if(nCtlColor == CTLCOLOR_STATIC)
	{
		pDC->SetBkMode(TRANSPARENT);   
		pDC->SetTextColor(RGB(255,255,255)); 

		return   (HBRUSH)::GetStockObject(NULL_BRUSH);   
	}
  // TODO:  Return a different brush if the default is not desired
  return hbr;
}

void CPromptDlg::OnBnClickedCheckClose()
{
  // TODO: Add your control notification handler code here
  m_ower->DestroyPrompt(m_uid);
}

void CPromptDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
  // TODO: Add your message handler code here and/or call default
  PostMessage(  WM_NCLBUTTONDOWN,HTCAPTION,MAKELPARAM(point.x,point.y)); 
  CDialog::OnLButtonDown(nFlags, point);
}
