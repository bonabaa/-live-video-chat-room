// LocalVideoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "YYContainer.h"
#include "LocalVideoDlg.h"
#include "yycontainerdlg.h"
#include ".\localvideodlg.h"
#include<windows.h>
#define IDC_MENEU_VIDESIZE 10000

CString g_CallJsScript(CYYContainerDlg* pContainer, CString & strParam)
{
  CStringArray params;
 // CString strParam = "";
  //strParam.Format("20;0;%u", (unsigned int)GetSafeHwnd() );//em_msgfromcontainer
  params.Add(strParam);
 return CString( pContainer-> CallJScript("yIMDlgControl",params));

}
// CLocalVideoDlg dialog

IMPLEMENT_DYNAMIC(CLocalVideoDlg, CDialog)
CLocalVideoDlg::CLocalVideoDlg(CWnd* pParent /*=NULL*/,CYYContainerDlg* pContainer )
	: CDialog(CLocalVideoDlg::IDD, pParent), m_pContainer(pContainer)
{
   C_W=200,C_H=235;
   m_bOpened= false;
   m_popupMenu=0;
  // m_nSelectSize=-1; m_nSelectFPS = ID_VIDEOFRAME_15; m_nSelectBitrate=ID_VIDEORATE_64K;
}

CLocalVideoDlg::~CLocalVideoDlg()
{
  
}

void CLocalVideoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CLocalVideoDlg, CDialog)
	ON_WM_MOVING()
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_SHOWWINDOW()
  ON_WM_ERASEBKGND()
  ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
  ON_WM_LBUTTONDOWN()
  ON_BN_CLICKED(IDOK, OnBnClickedOk)
  ON_WM_CONTEXTMENU()
  ON_WM_INITMENUPOPUP()
	ON_COMMAND_RANGE(ID_VIDEORATE_64K, ID__OPENVIDEO,OnMenuItem)
	//ON_UPDATE_COMMAND_UI_RANGE(ID_VIDEORATE_64K, ID_VIDEOFRAME_60, OnUpdateOnViewNumRange)
	ON_COMMAND_RANGE(10000, 10009, OnMenuItem)

END_MESSAGE_MAP()
void CLocalVideoDlg::OnMenuItem(UINT nID) 
{
 // int nRemoteID = atoi(m_strRemoteUserID.GetBuffer());
    // TODO: Add your command handler code here
    int framerate=10;
	if (nID >=10000&& nID <= 10030)
	{
    //m_nSelectSize = nID;
		//frame size
		UINT nIndex=nID-10000 ;
    int nIndexBitrate=0;
		int j=0;
    for(std::list<stMsgVideoFrame>::iterator i = m_listVideoSize.begin(); i !=m_listVideoSize.end(); i++,j++)
		{
			if (j >= nIndex)
			{
			 
        int w=i->frameWidth,h=i->frameHeight,bitrate=0;
			 
				//m_pLPNMgr->GetOPALMgr()->SetCurrectMediaInfobySessionID(info, m_strHandle.GetBuffer(), 2, nRemoteID);
        if ( w> 640 &&w<1024 ){
		       
		      bitrate =	256*1024  ;nIndexBitrate =2;
		      //m_pLPNMgr->GetOPALMgr()->SetCurrectMediaInfobySessionID(info, m_strHandle.GetBuffer(), 2, nRemoteID);
        }else if ( w>= 1024&&w<=1280 ){
		       
		      bitrate =	420*1024  ;nIndexBitrate = 4;
          //m_nSelectBitrate = ID_VIDEORATE_512K;
		      //m_pLPNMgr->GetOPALMgr()->SetCurrectMediaInfobySessionID(info, m_strHandle.GetBuffer(), 2, nRemoteID);
        }else if ( w>= 1280&&w<1920 ){
          //m_nSelectBitrate = ID_VIDEORATE_1024K;
          nIndexBitrate  =5;
		      bitrate =	800*1024  ;
		      //m_pLPNMgr->GetOPALMgr()->SetCurrectMediaInfobySessionID(info, m_strHandle.GetBuffer(), 2, nRemoteID);
        }else if ( w<=640 && w> 320 ){
          //m_nSelectBitrate = ID_VIDEORATE_128K;
		      bitrate =	128*1024  ;nIndexBitrate =1;
        }else {
          //m_nSelectBitrate = ID_VIDEORATE_64K;
          bitrate =	64*1024  ;nIndexBitrate=0;
        }
        CString strParam = "";
        strParam.Format("20;2;%d;%d;%d;%d;%d",  w,h, bitrate ,nIndex,nIndexBitrate );//em_msgfromcontainer
        g_CallJsScript(m_pContainer, strParam);
		     // m_pLPNMgr->GetOPALMgr()->SetCurrectMediaInfobySessionID(info, m_strHandle.GetBuffer(), 2, nRemoteID);
        break;
			}
		}
		return ;
	}
	else if (nID>=ID_VIDEORATE_64K && nID <= ID_VIDEORATE_2048K)
	{
    int nIndex =0;
    //m_nSelectBitrate = nID;
		//bitrate
		int bitrate=64000;
    switch(nID)
    {
    case ID_VIDEORATE_64K:
      bitrate = 64*1024;
      break;
     case ID_VIDEORATE_128K:
      bitrate = 128*1024;nIndex=1;
      break;
    case ID_VIDEORATE_256K:
      bitrate = 256*1024;nIndex=2;
      break;
    case ID_VIDEORATE_384K:
      bitrate = 366*1024;nIndex=3;
      break;
    case ID_VIDEORATE_512K:
       bitrate = 512*1024;nIndex=4;
     break;
    case ID_VIDEORATE_1024K:
      bitrate = 1024*1024;nIndex=5;
      break;
    case ID_VIDEORATE_2048K:
      bitrate = 2048*1024;nIndex=6;
      break;
   };
		//bitrate =	64*1024 * pow(2,nID - ID_VIDEORATE_64K); 
      CString strParam = "";
      strParam.Format("20;3;%d;%d",   bitrate, nIndex );//em_msgfromcontainer
      g_CallJsScript(m_pContainer, strParam);

		//m_pLPNMgr->GetOPALMgr()->SetCurrectMediaInfobySessionID(info, m_strHandle.GetBuffer(), 2, nRemoteID);
	
	}else if( nID >= ID_VIDEOFRAME_5&& nID <= ID_VIDEOFRAME_15)
	{
     
		//FPS 5+
    int nIndex = nID - ID_VIDEOFRAME_5;
    //m_nSelectFPS = nID;
		framerate =	5* (nID-ID_VIDEOFRAME_5+1); 
    CString strParam = "";
    strParam.Format("20;4;%d;%d", framerate,nIndex );//em_msgfromcontainer
    g_CallJsScript(m_pContainer, strParam);

		//m_pLPNMgr->GetOPALMgr()->SetCurrectMediaInfobySessionID(info, m_strHandle.GetBuffer(), 2, nRemoteID);
		
	}else if (nID == ID_VIDEOFRAME_25)
	{
    int nIndex = nID - ID_VIDEOFRAME_5;
		framerate =	25 ; 
//    m_nSelectFPS = nID;
    CString strParam = "";
    strParam.Format("20;4;%d;%d", framerate,  nIndex);//em_msgfromcontainer
    g_CallJsScript(m_pContainer, strParam);
		//m_pLPNMgr->GetOPALMgr()->SetCurrectMediaInfobySessionID(info, m_strHandle.GetBuffer(), 2, nRemoteID);
	}
	else if (nID == ID_VIDEOFRAME_30)
	{
    int nIndex = nID - ID_VIDEOFRAME_5;
		framerate =	30; 
//    m_nSelectFPS = nID;
    CString strParam = "";
    strParam.Format("20;4;%d;%d", framerate ,nIndex);//em_msgfromcontainer
    g_CallJsScript(m_pContainer, strParam);
		//m_pLPNMgr->GetOPALMgr()->SetCurrectMediaInfobySessionID(info, m_strHandle.GetBuffer(), 2, nRemoteID);
	}
	else if (nID == ID_VIDEOFRAME_60)
	{
    int nIndex = nID - ID_VIDEOFRAME_5;
    //m_nSelectFPS = nID;
		framerate =	60; 
		//m_pLPNMgr->GetOPALMgr()->SetCurrectMediaInfobySessionID(info, m_strHandle.GetBuffer(), 2, nRemoteID);
    CString strParam = "";
    strParam.Format("20;4;%d;%d", framerate,nIndex  );//em_msgfromcontainer
    g_CallJsScript(m_pContainer, strParam);
	}
	else if (nID == ID_OPTIONS)
	{
    CString strParam = "20;6";
    g_CallJsScript(m_pContainer, strParam);
		//OnOptions();
	}
	else if (nID == ID_SCREEN_AEAR)
	{
		//CCatchScreenDlg dlg(this);
  //  m_wndCaptureDlg= &dlg ;
  //  dlg.DoModal();
  //  m_wndCaptureDlg=NULL;	  
  //  if (m_bOrignalWin==true)
  //    SendMessage(   WM_VIDEOWND_DBCLICK,0,0);
   
    CString strParam = "";
    strParam.Format("20;5;%d", framerate );//em_msgfromcontainer
    g_CallJsScript(m_pContainer, strParam);

	}
	else if (nID == ID_ORIGINAL_SIZE)
	{
	  SendMessage(   WM_VIDEOWND_DBCLICK,0,0);

	}
	else if (nID == ID__MAXIMIZE)
	{
    CString strParam = "20;7";
    g_CallJsScript(m_pContainer, strParam);
    //  m_wndVideo.Invalidate();
      this->Invalidate();
  //SetFullScreen(true);
	}else if (nID== ID__VIDEO_OPTION)
  {
   // CString strParam = "";
    //strParam.Format("20;6;" );//em_msgfromcontainer
    g_CallJsScript(m_pContainer, CString("20;8;"));

  }
  else if (nID== ID__OPENVIDEO)
  {
   // CString strParam = "";
    //strParam.Format("20;6;" );//em_msgfromcontainer
    g_CallJsScript(m_pContainer, CString("20;8;1"));

  }}

void CLocalVideoDlg::OnMoving(UINT fwSide, LPRECT pRect)
{
	CDialog::OnMoving(fwSide, pRect);

	CRect dlgRect;
	for (size_t nIndex=0;nIndex < m_dkDialogs.size();nIndex++)
	{
		if ((m_dkDialogs[nIndex].bDocked) &&
			m_dkDialogs[nIndex].pWnd->IsWindowVisible())
		{
		
			m_dkDialogs[nIndex].pWnd->GetWindowRect(dlgRect);
			
			switch(m_dkDialogs[nIndex].nEdge) 
			{
			case DKDLG_LEFT:
				if (m_dkDialogs[nIndex].delta == 0)
					m_dkDialogs[nIndex].delta = pRect->top - dlgRect.top;
				dlgRect.MoveToXY(pRect->left-(dlgRect.right-dlgRect.left),(int)pRect->top-m_dkDialogs[nIndex].delta);
				break;
			case DKDLG_RIGHT:
				if (m_dkDialogs[nIndex].delta == 0)
					m_dkDialogs[nIndex].delta = pRect->top - dlgRect.top;
				dlgRect.MoveToXY(pRect->right,(int)pRect->top-m_dkDialogs[nIndex].delta);
				break;
			case DKDLG_TOP:
				if (m_dkDialogs[nIndex].delta == 0)
					m_dkDialogs[nIndex].delta = pRect->left - dlgRect.left;
				dlgRect.MoveToXY((int)pRect->left-m_dkDialogs[nIndex].delta,pRect->top-(dlgRect.bottom-dlgRect.top));
				break;
			case DKDLG_BOTTOM:
				if (m_dkDialogs[nIndex].delta == 0)
					m_dkDialogs[nIndex].delta = pRect->left - dlgRect.left;
				dlgRect.MoveToXY((int)pRect->left-m_dkDialogs[nIndex].delta,pRect->bottom);
				break;
			default:
				dlgRect;
			}

			((CLocalVideoDlg*)m_dkDialogs[nIndex].pWnd)->MoveMagDialog(dlgRect,TRUE,TRUE);
		}
	}


	// Magnetic field

	if (m_pMagParentDlg != NULL)
	{
		CPoint curPl,curPr,curPt,curPb;
		int rHight,rWidth;
		CRect rectParent,rectLeft,rectRight,rectTop,rectBottom;

		rHight	= 50;
		rWidth	= 50;

		m_pMagParentDlg->GetWindowRect(rectParent);

		// Magnetic fields!
		rectRight	= CRect(rectParent.right-rHight,rectParent.top-rWidth,
			rectParent.right+rHight,rectParent.bottom+rWidth);
		rectLeft	= CRect(rectParent.left-rHight,rectParent.top-rWidth,
			rectParent.left+rHight,rectParent.bottom+rWidth);
		rectTop		= CRect(rectParent.left-rWidth,rectParent.top-rHight,
			rectParent.right+rWidth,rectParent.top+rHight);
		rectBottom	= CRect(rectParent.left-rWidth,rectParent.bottom-rHight,
			rectParent.right+rWidth,rectParent.bottom+rHight);

		curPl		= CPoint(pRect->left,pRect->top+(pRect->bottom-pRect->top)/2);		//Left
		curPr		= CPoint(pRect->right,pRect->top+(pRect->bottom-pRect->top)/2);		//Right
		curPt		= CPoint(pRect->left+(pRect->right-pRect->left)/2,pRect->bottom);	//Top
		curPb		= CPoint(pRect->left+(pRect->right-pRect->left)/2,pRect->top);		//Bottom

		if (m_bDocked)
		{
			if ((nEdge == DKDLG_RIGHT) && 
				!rectRight.PtInRect(curPl))
			{
				m_pMagParentDlg->UnDockMagneticDialog(this);
				nEdge		= DKDLG_NONE;
				m_bDocked	= FALSE;
				m_iDelta	= 0;
			}
			if ((nEdge == DKDLG_LEFT )&& 
				!rectLeft.PtInRect(curPr))
			{
				m_pMagParentDlg->UnDockMagneticDialog(this);
				nEdge		= DKDLG_NONE;
				m_bDocked	= FALSE;
				m_iDelta	= 0;
			}
			if ((nEdge == DKDLG_TOP )&& 
				!rectTop.PtInRect(curPt))
			{
				m_pMagParentDlg->UnDockMagneticDialog(this);
				nEdge		= DKDLG_NONE;
				m_bDocked	= FALSE;
				m_iDelta	= 0;
			}
			if ((nEdge == DKDLG_BOTTOM )&& 
				!rectBottom.PtInRect(curPb))
			{
				m_pMagParentDlg->UnDockMagneticDialog(this);
				nEdge		= DKDLG_NONE;
				m_bDocked	= FALSE;
				m_iDelta	= 0;
			}

		}
		else
		{
			if ((m_dwMagType == DKDLG_RIGHT || m_dwMagType == DKDLG_ANY) && 
				rectRight.PtInRect(curPl))
			{
				m_pMagParentDlg->DockMagneticDialog(this,DKDLG_RIGHT);
				nEdge		= DKDLG_RIGHT;	
				m_bDocked	= TRUE;
			}

			if ((m_dwMagType == DKDLG_LEFT || m_dwMagType == DKDLG_ANY) && 
				rectLeft.PtInRect(curPr))
			{
				m_pMagParentDlg->DockMagneticDialog(this,DKDLG_LEFT);
				nEdge		= DKDLG_LEFT;	
				m_bDocked	= TRUE;
			}

			if ((m_dwMagType == DKDLG_TOP || m_dwMagType == DKDLG_ANY) && 
				rectTop.PtInRect(curPt))
			{
				m_pMagParentDlg->DockMagneticDialog(this,DKDLG_TOP);
				nEdge		= DKDLG_TOP;	
				m_bDocked	= TRUE;
			}
			if ((m_dwMagType == DKDLG_BOTTOM || m_dwMagType == DKDLG_ANY) && 
				rectBottom.PtInRect(curPb))
			{
				m_pMagParentDlg->DockMagneticDialog(this,DKDLG_BOTTOM);
				nEdge		= DKDLG_BOTTOM;	
				m_bDocked	= TRUE;
			}


		}

	}

}

void CLocalVideoDlg::EnableMagnetic(DWORD dwMagType,CYYContainerDlg* pMagParentDlg)
{
	m_dwMagType		= dwMagType;
	m_pMagParentDlg	= pMagParentDlg;
}

//void CLocalVideoDlg::AddMagneticDialog(CLocalVideoDlg* pDialog,BOOL bDocked,DWORD dwMagWhere)
//{
//	dk_window dkWnd;
//
//	dkWnd.pWnd		= pDialog;
//	dkWnd.dwMagType = DKDLG_ANY;
//	dkWnd.bDocked	= bDocked;
//	dkWnd.nEdge		= dwMagWhere;
//	dkWnd.delta		= 0;
//	m_dkDialogs.push_back(dkWnd);
//
//	pDialog->UpdateDockData(dkWnd);
//}
//
//void CLocalVideoDlg::DockMagneticDialog(CLocalVideoDlg* pDialog,DWORD nEdge)
//{
//	for (size_t nIndex=0;nIndex < m_dkDialogs.size();nIndex++)
//	{
//		if (m_dkDialogs[nIndex].pWnd == pDialog)
//		{
//			m_dkDialogs[nIndex].nEdge	= nEdge;
//			m_dkDialogs[nIndex].bDocked = TRUE;
//		}
//	}
//}
//
//void CLocalVideoDlg::UnDockMagneticDialog(CLocalVideoDlg* pDialog)
//{
//	for (size_t nIndex=0;nIndex < m_dkDialogs.size();nIndex++)
//	{
//		if (m_dkDialogs[nIndex].pWnd == pDialog)
//		{
//			m_dkDialogs[nIndex].nEdge	= DKDLG_NONE;
//			m_dkDialogs[nIndex].bDocked = FALSE;
//			m_dkDialogs[nIndex].delta	= 0;
//		}
//	}
//}
//
//
void CLocalVideoDlg::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
	if ((m_bDocked) &&					// This is a fix on movement... 
		(m_pMagParentDlg != NULL) &&	// I need it only if the windows is docked and I'm trying to move it:
		(m_bDisablePosFix == FALSE) &&	// when this window is dragged by parent window I don't need this fix
		IsWindowVisible())
	{
		CRect tmpRect,rectParent;
		GetWindowRect(tmpRect);
		m_pMagParentDlg->GetWindowRect(rectParent);
		if (nEdge == DKDLG_RIGHT)
		{
			m_iDelta = rectParent.top - tmpRect.top;
			tmpRect.MoveToXY(rectParent.right,lpwndpos->y);
		}
		if (nEdge == DKDLG_LEFT )
		{
			m_iDelta = rectParent.top - tmpRect.top;
			tmpRect.MoveToXY(rectParent.left-(tmpRect.right-tmpRect.left),lpwndpos->y);
		}
		if (nEdge == DKDLG_TOP )
		{
			m_iDelta = rectParent.left - tmpRect.left;
			tmpRect.MoveToXY(lpwndpos->x,rectParent.top-(tmpRect.bottom-tmpRect.top));
		}
		if (nEdge == DKDLG_BOTTOM )
		{
			m_iDelta = rectParent.left - tmpRect.left;
			tmpRect.MoveToXY(lpwndpos->x,rectParent.bottom);
		}
				
		lpwndpos->x		= tmpRect.left;
		lpwndpos->y		= tmpRect.top;
		lpwndpos->cx	= tmpRect.Width();
		lpwndpos->cy	= tmpRect.Height();

		m_pMagParentDlg->UpdateMagPosition(this);
	}
	m_bDisablePosFix = FALSE;

	CDialog::OnWindowPosChanging(lpwndpos);

}

void CLocalVideoDlg::MoveMagDialog(LPCRECT lpRect, BOOL bRepaint, BOOL bDisablePosFix)
{
	m_bDisablePosFix = bDisablePosFix;
	MoveWindow(lpRect,bRepaint);
}

//void CLocalVideoDlg::UpdateMagPosition(CLocalVideoDlg* pDialog)
//{
//	for (size_t nIndex=0;nIndex < m_dkDialogs.size();nIndex++)
//	{
//		if (m_dkDialogs[nIndex].pWnd == pDialog)
//		{
//			m_dkDialogs[nIndex].delta	= 0;
//		}
//	}
//}
void CLocalVideoDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialog::OnShowWindow(bShow, nStatus);

  if (bShow) 
    m_wndVideo.ShowWindow(SW_SHOW);
	if (bShow && m_bDocked &&
		(m_pMagParentDlg != NULL))
	{
		CRect paRect,dlgRect;
		m_pMagParentDlg->GetWindowRect(paRect);
		GetWindowRect(dlgRect);

		switch(nEdge) 
		{
		case DKDLG_LEFT:
			dlgRect.MoveToXY(paRect.left-(dlgRect.right-dlgRect.left),(int)paRect.top-m_iDelta);
			break;
		case DKDLG_RIGHT:
			dlgRect.MoveToXY(paRect.right,(int)paRect.top-m_iDelta);
			break;
		case DKDLG_TOP:
			dlgRect.MoveToXY((int)paRect.left-m_iDelta,paRect.top-(dlgRect.bottom-dlgRect.top));
			break;
		case DKDLG_BOTTOM:
			dlgRect.MoveToXY((int)paRect.left-m_iDelta,paRect.bottom);
			break;
		default:
			dlgRect;
		}

		MoveWindow(dlgRect);
	}
}

void CLocalVideoDlg::UpdateDockData(dk_window dkdata)
{
	m_dwMagType	= dkdata.dwMagType;
	nEdge		= dkdata.nEdge;
	m_bDocked	= dkdata.bDocked;
}
// CLocalVideoDlg message handlers

BOOL CLocalVideoDlg::OnEraseBkgnd(CDC* pDC)
{
  // TODO: Add your message handler code here and/or call default
///////////////////////////////////////////////////////////
	//if (m_DC.m_hDC !=NULL)
	{
		BITMAP bmpInfo; 
		m_bkBitmap.GetBitmap(&bmpInfo); 
		CRect rect; 
		GetClientRect(&rect); 
				//////////////////////////////////// 
		//pDC->BitBlt(0,0,rect.Width(),rect.Height(),&m_DC,0,0,SRCCOPY); 
		pDC->StretchBlt(0,0,rect.Width(),rect.Height(),&m_DC,0,0, bmpInfo.bmWidth, bmpInfo.bmHeight,SRCCOPY);
  
    return TRUE;
	}

	//////////////////////////////////////// 
  return CDialog::OnEraseBkgnd(pDC);
}
 
BOOL CLocalVideoDlg::OnInitDialog()
{
  CDialog::OnInitDialog();
	//video window
  CRect rect(0,0,0,0);
  SetWindowPos(0,0,0,C_W,C_H, SWP_NOMOVE);
	m_wndVideo.Create(WS_CHILD|WS_VISIBLE,rect,this,AFX_IDW_PANE_FIRST);
  m_wndVideo.MoveWindow(20,32,160,120); 
	m_wndVideo.ShowWindow(SW_SHOW);
  ModifyStyleEx(WS_EX_APPWINDOW,WS_EX_TOOLWINDOW);//
	//ModifyStyleEx(WS_EX_APPWINDOW,WS_EX_TOOLWINDOW);//
//	m_hDibDraw = DrawDibOpen();
  m_bkBitmap.LoadBitmap(IDB_BITMAP_LOCAL_PANEL);
  CBitmap& bitmap =m_bkBitmap; 
  BITMAP bitInfo; 
  ////得到图片大小并调整窗口大小适应图片 
  //bitmap.GetBitmap(&bitInfo); 
	//memset( &m_destinfo, 0, sizeof( m_destinfo ) );
	//m_destinfo.biSize = sizeof( m_destinfo );
	//m_destinfo.biBitCount = bitInfo.bmBitsPixel ;
	//m_destinfo.biCompression = MAKEFOURCC( 'I', '4', '2', '0' );
	//m_destinfo.biPlanes = 1;
	//m_destinfo.biWidth = bitInfo.bmWidth;
	//m_destinfo.biHeight = bitInfo.bmHeight;
	//m_destinfo.biSizeImage = m_destinfo.biWidth * m_destinfo.biHeight  *  8;/* m_destinfo.biBitCount*/


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
	//////////////////////////////////////// 
	//BITMAPINFOHEADER destinfo;
	//memset( &destinfo, 0, sizeof( destinfo ) );
	//destinfo.biSize = sizeof( destinfo );
	//destinfo.biBitCount = 32;
	//destinfo.biCompression = BI_RGB;
	//destinfo.biPlanes = 1;
	//destinfo.biWidth = bitInfo.bmWidth;
	//destinfo.biHeight = bitInfo.bmHeight;
	//destinfo.biSizeImage = 0/*destinfo.biWidth * destinfo.biHeight * destinfo.biBitCount*/ ;
	//memset( &m_destinfoBMP, 0, sizeof( m_destinfoBMP ) );

	//m_destinfoBMP.bmiHeader = destinfo;
  // TODO:  Add extra initialization here
  return TRUE;  // return TRUE unless you set the focus to a control
  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CLocalVideoDlg::DestroyWindow()
{
  // TODO: Add your specialized code here and/or call the base class
  m_wndVideo.DestroyWindow();

  return CDialog::DestroyWindow();
}

void CLocalVideoDlg::OnBnClickedCancel()
{
  // TODO: Add your control notification handler code here
  OnCancel();
}

BOOL CLocalVideoDlg::PreTranslateMessage(MSG* pMsg)
{
  // TODO: Add your specialized code here and/or call the base class
  //if(pMsg->message==WM_KEYDOWN&&pMsg->wParam==VK_ESCAPE)     
  //    return TRUE;

  return CDialog::PreTranslateMessage(pMsg);
}

void CLocalVideoDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
  // TODO: Add your message handler code here and/or call default
	PostMessage(WM_NCLBUTTONDOWN,HTCAPTION,MAKELPARAM(point.x,point.y)); 

  CDialog::OnLButtonDown(nFlags, point);
}

LRESULT CLocalVideoDlg::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
  // TODO: Add your specialized code here and/or call the base class
  if (message == 2001 && this->IsWindowVisible() ){
    stMsgVideoChild* p = (stMsgVideoChild*) wParam;
    //if (m_wndVideo.IsWindowVisible() ==FALSE) m_wndVideo.ShowWindow()
    if (m_wndVideo.m_nWinver >= D_WIN7_VER)
      m_wndVideo.Draw("", p->pData, 0, p->w, p->h);
    else
      m_wndVideo.DrawWithBGR24Data("", p->pData, 0, p->w, p->h);
    return TRUE;
    
  }else if (message ==WM_VIDEOWND_DBCLICK){
      OnMenuItem(ID__MAXIMIZE);
  }
  return CDialog::DefWindowProc(message, wParam, lParam);
}

void CLocalVideoDlg::OnBnClickedOk()
{
  // TODO: Add your control notification handler code here
  CStringArray params;
  CString strParam = "";
  strParam.Format("20;0;%u", (unsigned int)GetSafeHwnd() );//em_msgfromcontainer
  params.Add(strParam);
  m_pContainer-> CallJScript("yIMDlgControl",params);
 // OnOK();
}
//#define D_ENABLE_16 
void CLocalVideoDlg::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	CMenu menu;
    LANGID  id = GetUserDefaultUILanguage();
   
  if (id != 2052)
	  VERIFY(menu.LoadMenu(IDR_MENU_VIDEO_CTRL_EN));
  else
	  VERIFY(menu.LoadMenu(IDR_MENU_VIDEO_CTRL));

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);
	CWnd* pWndPopupOwner = this;
   // 添加弹出式子菜单
		m_listVideoSize.clear();
  CStringArray params;
  CString strParam ="20;1";
  //strParam.Format("20;1", (unsigned int)GetSafeHwnd() );//em_msgfromcontainer
  params.Add(strParam);
  CComVariant ret= m_pContainer-> CallJScript("yIMDlgControl",params);
  CString strVideoSizes, strCurMediaInfo, strtmp = CString(ret.bstrVal);
  
  int nstart=0, i=0;
  nstart= strtmp.Find("(");
  if (nstart>0 ){
    strVideoSizes = strtmp.Mid(0, nstart);
    strCurMediaInfo = strtmp.Mid(nstart+1, strtmp.GetLength()- nstart-2);
  }
  CString strValue;
  nstart=0;
  char tmp[24]={0};
  int  nSelectSizeMenuIndex =-1, nSelectFPSMenuIndex =2,  nSelectBitrateMenuIndex=0;
  while ( (strValue=strVideoSizes.Tokenize(",", nstart) )!="" ){
    int mpos =strValue.Find(";");
    if (mpos>0){
      stMsgVideoFrame f;
      f.frameWidth = strtoul(strValue.Mid(0,mpos ), 0,10);
      f.frameHeight = strtoul(strValue.Mid( mpos+1,strValue.GetLength() - mpos-1), 0,10);
#ifdef D_ENABLE_16
      if (f.frameWidth% 16 ==0 &&   f.frameHeight %16==0)
        m_listVideoSize.push_back(f);
#else
        m_listVideoSize.push_back(f);
      //if (f.frameWidth==1920)
#endif
       
    }
  }
  
  nstart =0;i=0;
  while ( (strValue=strCurMediaInfo.Tokenize("|", nstart) )!="" ){
    if (i==0){
      nSelectSizeMenuIndex= atoi(strValue);
    }else if (i==1){
       nSelectFPSMenuIndex= atoi(strValue);
    }else if (i==2){
       nSelectBitrateMenuIndex= atoi(strValue);
    }

    i++;
  }



  if (m_listVideoSize.size()<=1 )
  {
    stMsgVideoFrame f1; 
    f1.frameWidth=1280;f1.frameHeight=720;
    m_listVideoSize.insert(m_listVideoSize.begin(), f1);
    f1.frameWidth=1920/2;f1.frameHeight=1080/2;
    m_listVideoSize.insert(m_listVideoSize.begin(), f1);
    f1.frameWidth=1920/3;f1.frameHeight=1080/3;
    m_listVideoSize.insert(m_listVideoSize.begin(), f1);
    f1.frameWidth=640;f1.frameHeight=480;
    m_listVideoSize.insert(m_listVideoSize.begin(), f1);
    f1.frameWidth=320;f1.frameHeight=240;
    m_listVideoSize.insert(m_listVideoSize.begin(), f1);
    //m_listVideoSize.push_back(f1);
  }
	//m_pLPNMgr->GetOPALMgr()->GetVideoSizes(listVideoSizes);
 	if (m_popupMenu) delete m_popupMenu;
	m_popupMenu = new CMenu();
	m_popupMenu->CreateMenu();
	int j=0;
	for(std::list<stMsgVideoFrame>::iterator i = m_listVideoSize.begin(); i !=m_listVideoSize.end(); i++ )
	{
 #ifdef D_ENABLE_16
   if (i->frameWidth% 16 ==0 &&   i->frameHeight%16==0)
#endif
		{    
			CString message = "";
      message.Format("%dx%d", i->frameWidth, i->frameHeight );
      if (i->frameWidth ==320&&  i->frameHeight == 240&& nSelectSizeMenuIndex == -1){
        nSelectSizeMenuIndex =j;
      }
      int nMenuID = IDC_MENEU_VIDESIZE   + j++;
			m_popupMenu->AppendMenuA(MF_STRING, nMenuID, message.GetBuffer() );
     /* if (m_nSelectSize< 0  &&  i->frameWidth == 320 ){
        m_nSelectSize = nMenuID;
        
      }*/

		}
	}
  m_popupMenu->CheckMenuRadioItem(IDC_MENEU_VIDESIZE, IDC_MENEU_VIDESIZE+j-1, IDC_MENEU_VIDESIZE+nSelectSizeMenuIndex ,  MF_BYCOMMAND);
  pPopup->AppendMenuA(MF_POPUP, (UINT_PTR)m_popupMenu->operator HMENU(),  (id != 2052? "Video Size":"视频大小") );
  //select FPS menu item
  CMenu* menuFPS= pPopup->GetSubMenu(7);
  if (menuFPS ) menuFPS->CheckMenuRadioItem(ID_VIDEOFRAME_5, ID_VIDEOFRAME_60,   ID_VIDEOFRAME_5+ nSelectFPSMenuIndex, MF_BYCOMMAND);
  //select bitrate menu item
  CMenu* menuBitrate= pPopup->GetSubMenu(6);
  if (menuBitrate ) menuBitrate->CheckMenuRadioItem(ID_VIDEORATE_64K, ID_VIDEORATE_2048K,  ID_VIDEORATE_64K+ nSelectBitrateMenuIndex, MF_BYCOMMAND);

  
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,
			pWndPopupOwner);
}

void CLocalVideoDlg::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
  CDialog::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

  // TODO: Add your message handler code here
}
