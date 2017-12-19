// VideoPanelDlg.cpp : implementation file
//

#include "stdafx.h"
#include "YYSpriteX-2.h"
#include "VideoPanelDlg.h"
#include ".\videopaneldlg.h"
#include ".\VideoPanelChildDlg.h"
#include "lpnmgr.h"
#include "YYSpriteX-2Ctrl.h"
#include <algorithm>
#include "OptionsExDlg.h"
#include "myclient.h"
#include "remotedlg.h" 
#include "main.h"
#define D_MCC_DEFAULT_RIGHT_VIDEO_PANEL_LEFTOFFSET 11//(320-4*20) 
#define D_MCC_DEFAULT_RIGHT_VIDEO_PANEL_TOPOFFSET 61//(320-4*20) 
#define D_MCC_DEFAULT_RIGHT_VIDEO_PANEL_TOPHEIGHT 35//(320-4*20) 
#define D_MCC_DEFAULT_RIGHT_VIDEO_PANEL_W 224//(320-4*20) 
#define D_MCC_DEFAULT_RIGHT_VIDEO_PANEL_H 176//(240-3*20)
extern int  g_VideoLayout ;
int GetTrayWindowHeight()
{
  HWND TastWnd=FindWindow( "shell_traywnd", NULL); 
  CRect recttray(0,0,0,0); 
  if (TastWnd){
 //任务栏的尺寸 
    ::GetWindowRect(TastWnd, &recttray); 
  }
  return recttray.Height();
}
CDialog* GetCwnd(stChildDlg* p)
{
  if (p->m_pChild1) return p->m_pChild1;
  else
    return p->m_pChild2;
};
void SetCwnd(stChildDlg* p ,CDialog* pDlg)
{
  if (p->m_pChild1)  p->m_pChild1 = (CVideoPanelChildDlg*)pDlg;
  else if (p->m_pChild2)
    p->m_pChild2 = (COptionsExDlg*)pDlg;
};
#define D_ROOM_WIN_W 1024
#define D_ROOM_WIN_H 728

void CVideoPanelDlg::SetWindowDefaultSize()
{
    CRect DesktoPrect ; 
   HWND hDesktop =  ::GetDesktopWindow();
  ::GetWindowRect( hDesktop,&DesktoPrect);
  int x,y,w=D_ROOM_WIN_W,h=D_ROOM_WIN_H;
  int workwidth = DesktoPrect.Width();// workrect.right -  workrect.left;
  int workheight = DesktoPrect.Height()-GetTrayWindowHeight();// workrect.bottom - workrect.top;

  //
  CRect rc;
  this->GetWindowRect(rc);
  if (workwidth == rc.Width() && workheight == rc.Height()){
    if (workwidth <  D_ROOM_WIN_W && workheight <  D_ROOM_WIN_H){
      MoveWindow(0,0,D_ROOM_WIN_W,D_ROOM_WIN_H);
    }else{
       
      x = (workwidth- D_ROOM_WIN_W)/2;
      y = (workheight- D_ROOM_WIN_H )/2;

      MoveWindow(x, y, D_ROOM_WIN_W, D_ROOM_WIN_H);
    }    
  }else{ 
    MoveWindow(0, 0, workwidth, workheight);
   // ShowWindow(SW_SHOWMINIMIZED);
  }


}
 
void  SetFullScreen(bool bfull, HWND h)     
{
   CRect DesktoPrect; /* temporary variable */

  /* Next function copies the dimensions of 
  the bounding rectangle of the desktop */
  HWND hDesktop =  GetDesktopWindow();
  ::GetWindowRect( hDesktop,&DesktoPrect);
  int workwidth = DesktoPrect.Width();// workrect.right -  workrect.left;
  int workheight = DesktoPrect.Height()-GetTrayWindowHeight();// workrect.bottom - workrect.top;
#if 0
  HWND m_hwnd1 =h;
  //return;
	// Find how large the desktop work area is
	RECT workrect;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workrect, 0);

	RECT fullwinrect;
	

		SetRect(&fullwinrect, 0, 0,
				workwidth, workheight);


	//AdjustWindowRectEx(&fullwinrect, 
	//		   GetWindowLong(m_hwnd, GWL_STYLE ), 
	//		   FALSE, GetWindowLong(m_hwnd, GWL_EXSTYLE));

	int m_fullwinwidth = fullwinrect.right - fullwinrect.left;
	int m_fullwinheight = fullwinrect.bottom - fullwinrect.top;

 fullwinrect.bottom = fullwinrect.bottom + rtb.bottom - rtb.top - 3;
	//}

	int m_winwidth  =  min(fullwinrect.right - fullwinrect.left,  workwidth);
	int m_winheight =  min(fullwinrect.bottom - fullwinrect.top, workheight);
	if ((fullwinrect.right - fullwinrect.left > workwidth) &&
		(workheight - m_winheight >= 16)) {
		m_winheight = m_winheight + 16;
	} 
	if ((fullwinrect.bottom - fullwinrect.top > workheight) && 
		(workwidth - m_winwidth >= 16)) {
		m_winwidth = m_winwidth + 16;
	}
#endif
	int x =0 ,y= 0;
	WINDOWPLACEMENT winplace;
	winplace.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(h, &winplace);
	if (bfull==false) {
    x =  workwidth/3 *2;		
		y = 20;		
    workwidth = workwidth/2;
    workwidth = workheight/2;
	} else {
		// Try to preserve current position if possible
	/*	GetWindowPlacement( h, &winplace);
		if ((winplace.showCmd == SW_SHOWMAXIMIZED) || (winplace.showCmd == SW_SHOWMINIMIZED)) {
			x = winplace.rcNormalPosition.left;
			y = winplace.rcNormalPosition.top;
		} else {
			RECT tmprect;
			GetWindowRect(h,  &tmprect);
			x = tmprect.left;
			y = tmprect.top;
		}
		if (x + m_winwidth > workrect.right)
			x = workrect.right - m_winwidth;
		if (y + m_winheight > workrect.bottom)
			y = workrect.bottom - m_winheight;*/
	}
	winplace.rcNormalPosition.top = y;
	winplace.rcNormalPosition.left = x;
	winplace.rcNormalPosition.right = x + workwidth;
  winplace.rcNormalPosition.bottom = y + workheight;
	SetWindowPlacement( h, &winplace);
  
	//PositionChildWindow();
} 

// CVideoPanelDlg dialog

IMPLEMENT_DYNAMIC(CVideoPanelDlg, CDialog)
CVideoPanelDlg::CVideoPanelDlg(CWnd* pParent  , LPNMgr* pMgr)
	: CDialog(CVideoPanelDlg::IDD, pParent),m_pMgr(pMgr), m_bMCCLayout(false),m_pMCCDocFlg(NULL)
{
  //m_rcPrepareForChild.SetRectEmpty();
  m_pChildMouseDown =0;
  m_stAspectRatio.den=3;
  m_stAspectRatio.num = 4;
}

CVideoPanelDlg::~CVideoPanelDlg()
{
  m_pMgr->GetSpriteX2Ctrl()->m_pVideoPanelDlg = 0;
   
}
void CVideoPanelDlg::HungupCall()
{
  ::PostMessage(m_pMgr->GetSpriteX2Ctrl()->GetHWNDEX(), YY_HUNGUP_WM, 0,0);
        g_VideoLayout =0;// em_videoLayoutSimple;
        PostMessage(WM_CLOSE);
        PostMessage(WM_DESTROY);

}
void CVideoPanelDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CVideoPanelDlg, CDialog)
  ON_WM_CONTEXTMENU()
  ON_WM_DESTROY()
  ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
    ON_COMMAND_RANGE(ID_Menu_Layout_normal, ID_Normal, OnMenuItem)

    ON_BN_CLICKED(IDCANCEL2, OnBnClickedCancel2)
    ON_WM_ERASEBKGND()
    ON_WM_PAINT()
    ON_WM_KEYUP()
    ON_WM_SYSKEYDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_CREATE()
    ON_WM_SHOWWINDOW()
    ON_WM_CLOSE()
    ON_WM_SIZE()
END_MESSAGE_MAP()

void CVideoPanelDlg::OnMenuItem(UINT nID) 
{
    // TODO: Add your command handler code here
	switch (nID)
  {
  case ID_Menu_Layout_normal:
    {
      g_VideoLayout =0;// em_videoLayoutSimple;
#if 0
      PostMessage(WM_CLOSE);
      PostMessage(WM_DESTROY);
#else
      //m_pMgr->GetSpriteX2Ctrl()->ShowOrHidPopIMDlg(m_nRoomID, false);
      ShowWindow(SW_HIDE);
#endif
    
    }
    break;
  case ID_VIEOLAYOUT_HUNGUP:
    {
      HungupCall();
    }
    break;
  case  ID__0:
  case  ID__2:
  case  ID__4:
  case  ID__6:
  case  ID__9:
  case  ID__12:
  case  ID__16:
  case  ID__32:
  case ID_Normal:
    {
      //m_nLayoutmenuID = nID;
       SetChildLayout(nID) ; 
    }
    break;
  case ID_4_3:
    {
      m_stAspectRatio.den =3;m_stAspectRatio.num=4;
      SetChildLayout(m_nLayoutmenuID);
    }break;
  case ID_5_4:
    {
      m_stAspectRatio.den =4;m_stAspectRatio.num=5;SetChildLayout(m_nLayoutmenuID);
    }break;
  case ID_16_9:
    {
      m_stAspectRatio.den =9;m_stAspectRatio.num=16;SetChildLayout(m_nLayoutmenuID);
    }break;
  case ID_1_1:
    {
      m_stAspectRatio.den =1;m_stAspectRatio.num=1;SetChildLayout(m_nLayoutmenuID);
    }break;
  case ID_VIEOLAYOUT_ADDMCCPANEL:
    {

    }break;
  case ID_VIEOLAYOUT_SWITCH_CC_PANNEL:
    {
      if (m_bMCCLayout){
        SetChildLayout(ID_Normal);
      //  //ShowWindow(SW_HIDE);
      //  //SetWindowPos(&wndNoTopMost,   0,   0,   0,   0,   SWP_NOSIZE   |   SWP_NOMOVE); 
      // // ShowWindow(SW_SHOW);
      //  m_pMgr->GetMEMgr()->SwitchToMCCPanel();
      //g_VideoLayout =0;// em_videoLayoutSimple;
      //PostMessage(WM_CLOSE);
      //PostMessage(WM_DESTROY);

      }
    }break;  
  };
}

// CVideoPanelDlg message handlers

BOOL CVideoPanelDlg::OnInitDialog()
{
  CDialog::OnInitDialog();
#ifdef _DEBUG
  ModifyStyle( WS_EX_TOPMOST,0); 
#else
#endif
  // TODO:  Add extra initialization here
//#if 0
//  SetFullScreen(true,GetSafeHwnd() );
//#else
  HWND hDesktop =  ::GetDesktopWindow();
  CRect DesktoPrect;
  
  ::GetClientRect( hDesktop,&DesktoPrect);
  int x,y,w=D_ROOM_WIN_W,h=D_ROOM_WIN_H;
  int workwidth = DesktoPrect.Width();// workrect.right -  workrect.left;
  int workheight = DesktoPrect.Height();// workrect.bottom - workrect.top;
    x = (workwidth- D_ROOM_WIN_W)/2;
      y =  (workheight- D_ROOM_WIN_H-GetTrayWindowHeight())/2;

      MoveWindow(x, y, D_ROOM_WIN_W, D_ROOM_WIN_H);
  //SetWindowDefaultSize();
//#endif
 // SetWindowPos(&wndTopMost,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE);
  
 
 //SetWindowPos(&wndNoTopMost,   0,   0,   0,   0,   SWP_NOSIZE   |   SWP_NOMOVE); 
  CRect rc;
  GetWindowRect(&rc);
  if (m_bMCCLayout){
     m_nLayoutmenuID= ID_Normal/*ID__2*/;
     COptionsExDlg* pDlg = m_pMCCDocFlg = m_pMgr->GetSpriteX2Ctrl()->CreatePopIMDlg(m_nRoomID, (const char*)m_pMgr->GetMEMgr()->m_nCurectEnteredRoom_alias, this, true, true);
    if (pDlg){
      //pDlg->ShowWindowEx();
     /* stChildDlg* p = new stChildDlg(this);
      p->m_pChild2= pDlg;*/
     // m_mapChildren.insert(m_mapChildren.begin(), p);
      //pDlg->ShowWindow(SW_SHOW);
      OnMenuItem(m_nLayoutmenuID);
    }
  }else{
     m_nLayoutmenuID=  ID__2 ;
  }
  //for(int i=0;i< 0;i++){
  //      CVideoPanelChildDlg*p = new CVideoPanelChildDlg(this);
  //      stChildDlg* p1 = new stChildDlg(this);
  //      p1->m_pChild1= p;
  //      p->m_nUserID = i+400 /*pChildMsg->uid*/;
  //      p->Create(IDD_DIALOG_VIDEO_CHILD,this);
  //      m_mapChildren.push_back( p1);
  //      p->ShowWindow(SW_SHOW);
  //      //p->SetWindowText(pChildMsg->m_strAlias);
  //      //p->SetVideoWindowTitle(true, pChildMsg->m_strAlias);
  //}

  return TRUE;  // return TRUE unless you set the focus to a control
  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CVideoPanelDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message  == WM_VIDEO_CHILD){
	  return TRUE;
	  }
  //if(pMsg->message==WM_KEYDOWN&&pMsg->wParam==VK_ESCAPE)     
  //{
  //   OnMenuItem(ID_Menu_Layout_normal);
  //    return TRUE;
  //}
  // TODO: Add your specialized code here and/or call the base class
  return CDialog::PreTranslateMessage(pMsg);
}
void CVideoPanelDlg::GetApectRatioedHxW ( int& childW,  int& childH, int& wOffset , int& hOffset)
{
 // if ( childW*100 / childH*100 > m_stAspectRatio.den*100/m_stAspectRatio.num*100){
    int tmpH  = childW*m_stAspectRatio.den/m_stAspectRatio.num;//4:3
    hOffset =  (childH - tmpH)/2;
    childH =tmpH;
 // }else{
  //  int tmpW = childH*m_stAspectRatio.den/m_stAspectRatio.num;//4:3
  //  wOffset =  (childW - tmpW)/2;
  //  childW= tmpW;
  //}

}

BOOL CVideoPanelDlg::SetChildLayout(int menuID)
{
   
  if (ID_Normal == menuID && m_pMCCDocFlg==NULL)
    return TRUE;
  if (m_pMCCDocFlg && ID_Normal != menuID)
    m_pMCCDocFlg->ShowWindow(SW_HIDE);
  m_nLayoutmenuID = menuID;
  m_lstChildrect.clear();
  //for(MAP_PanelChildren::iterator i= m_mapChildren.begin();i!= m_mapChildren.end(); i++)
  //{
  //  CDialog *p =GetCwnd(*i);
  //  if (p)
  //  {
  //    p->ShowWindow(SW_HIDE);
  //  }
  //}
	switch (menuID)
  {
  case ID_Normal://mcc+video
    {
//#define D_OLD_MCC_LAYOUT
      //CRect mccLeft(0,0 ,rc.Width(), rc.Height());
      if (m_pMCCDocFlg){
        CRect rc; GetClientRect(&rc);
       // if (m_pMCCDocFlg->IsWindowVisible() == FALSE)
#ifdef D_OLD_MCC_LAYOUT
          m_pMCCDocFlg->SetWindowPos(0, 0,0, rc.Width() -D_MCC_DEFAULT_RIGHT_VIDEO_PANEL_W , rc.Height() , SWP_SHOWWINDOW|SWP_NOACTIVATE);
#else
          m_pMCCDocFlg->SetWindowPos(0, 0,0, rc.Width()/*-D_MCC_DEFAULT_RIGHT_VIDEO_PANEL_W*/, rc.Height() , SWP_SHOWWINDOW|SWP_NOACTIVATE);
#endif

        //m_pMCCDocFlg->ShowWindowEx();
        int i=0;
        int nCount = rc.Height()/ D_MCC_DEFAULT_RIGHT_VIDEO_PANEL_H+1;
        for(MAP_PanelChildren::iterator itor= m_mapChildren.begin();itor!= m_mapChildren.end()&&i< nCount; i++)
        {
          POINT pt; SIZE sz; 
          i= (*itor)->pos;
#ifdef D_OLD_MCC_LAYOUT
          pt.x =  rc.Width()-D_MCC_DEFAULT_RIGHT_VIDEO_PANEL_W ; pt.y= i*D_MCC_DEFAULT_RIGHT_VIDEO_PANEL_H;
#else
          pt.x = D_MCC_DEFAULT_RIGHT_VIDEO_PANEL_LEFTOFFSET/*rc.Width()-D_MCC_DEFAULT_RIGHT_VIDEO_PANEL_W*/; 
          if (i==0)
            pt.y= i*D_MCC_DEFAULT_RIGHT_VIDEO_PANEL_H+D_MCC_DEFAULT_RIGHT_VIDEO_PANEL_TOPOFFSET;
          else
            pt.y= i*D_MCC_DEFAULT_RIGHT_VIDEO_PANEL_H+D_MCC_DEFAULT_RIGHT_VIDEO_PANEL_TOPOFFSET
            +i*D_MCC_DEFAULT_RIGHT_VIDEO_PANEL_TOPHEIGHT;
 
#endif
          sz.cx = D_MCC_DEFAULT_RIGHT_VIDEO_PANEL_W; sz.cy = D_MCC_DEFAULT_RIGHT_VIDEO_PANEL_H;
          CRect rc(pt, sz);
          m_lstChildrect.push_back(rc);
          if (itor!= m_mapChildren.end() ){
            CDialog *p =GetCwnd(*itor);
			      p->UpdateWindow();
			      p->RedrawWindow(NULL, NULL, RDW_ALLCHILDREN);
            if ( p!= m_pMCCDocFlg ){
               
              p->SetWindowPos(0,pt.x,pt.y ,sz.cx ,sz.cy, SWP_NOZORDER |SWP_SHOWWINDOW);
             // m_pMCCDocFlg->InvalidateRect(rc);
             
            }else 
            {
				/*p->ShowWindow( SW_MAXIMIZE);
				p->ShowWindow(SW_MINIMIZE);
				p->ShowWindow( SW_MAXIMIZE);*/
              //GetCwnd(*itor)->ShowWindow(SW_HIDE);
            }
            itor++;
          }else{
            //cal rects;
          }
           
        }
        m_pMCCDocFlg->RedrawWindow(NULL, NULL, RDW_ALLCHILDREN|RDW_INVALIDATE|RDW_VALIDATE);
      }
     
    }
    break;
  case  ID__2://2 layout
    {
      int nOffset =2;
      CRect rc;
      GetWindowRect(rc);
      int childW=rc.Width()/2,childH = rc.Height();
      //rc.left += rc.Width()/2;
      int hOffset=0, wOffset=0;
      GetApectRatioedHxW( childW, childH,  wOffset,hOffset);
      //int hOffset =  (rc.Height() - childH)/2;
      //childH = childW*m_stAspectRatio.den/m_stAspectRatio.num;//4:3
      int i=0;
      for(MAP_PanelChildren::iterator itor= m_mapChildren.begin();i<2 ;i++)
      {
        POINT pt; SIZE sz;
        pt.x = wOffset+i* childW; pt.y= hOffset;
        sz.cx = childW; sz.cy = childH;
        CRect rc(pt, sz);
        m_lstChildrect.push_back(rc);
        if (itor!= m_mapChildren.end()){
          if (i<2){
            GetCwnd(*itor)->SetWindowPos(0,wOffset+i* childW,hOffset,childW ,childH, SWP_NOZORDER |SWP_SHOWWINDOW);
          }else 
          {
            GetCwnd(*itor)->ShowWindow(SW_HIDE);
          }
          itor++;
        }
        
      }
       
    }
    break;
  case  ID__4://4 layout
    {
      CRect rc;
      GetWindowRect(rc);
      int childW=rc.Width()/2,childH = rc.Height()/2;
      int i=0;
      for(MAP_PanelChildren::iterator itor= m_mapChildren.begin();i<4 ;i++)
      { 
        POINT pt; SIZE sz;
        pt.x = (i%2* childW); pt.y= i/2* childH;
        sz.cx = childW; sz.cy = childH;
        CRect rc(pt, sz);
        m_lstChildrect.push_back(rc);
        if (itor!= m_mapChildren.end()){
          if( i<4){
            GetCwnd(*itor)->SetWindowPos( 0,(i%2* childW),i/2* childH,childW ,childH,  SWP_NOZORDER |SWP_SHOWWINDOW);
          }else 
          {
            GetCwnd(*itor)->ShowWindow(SW_HIDE);
          }
          itor++;
        }
         
      }     
    }
    break;
  case  ID__6://6 layout
    {
#ifdef D_NORMAL6_LAYOUT
      CRect rc;
      GetWindowRect(rc);
      int childW=rc.Width()/3,childH = rc.Height()/2;
      int i=0;
      for(MAP_PanelChildren::iterator itor= m_mapChildren.begin();itor!= m_mapChildren.end();itor++)
      {
        if( i<6)
          GetCwnd(*itor)->SetWindowPos( 0,(i%3* childW),i/3* childH,childW ,childH,  SWP_NOZORDER |SWP_SHOWWINDOW);
        else
          GetCwnd(*itor)->ShowWindow(SW_HIDE);
        i++;
      }  
#else
      CRect rc;
      GetWindowRect(rc);
      int childW=rc.Width()/3,childH = rc.Height()/3;
      int i=0;
      for(MAP_PanelChildren::iterator itor= m_mapChildren.begin();i<6;i++)
      {
        POINT pt; SIZE sz;
        
        {
          if( i== 0){
            pt.x = 0; pt.y=0;
            sz.cx = childW*2; sz.cy = childH*2;
            CRect rc(pt, sz);
            m_lstChildrect.push_back(rc);
            if (itor!= m_mapChildren.end()){
              GetCwnd(*itor)->SetWindowPos( 0,0,0,childW*2 ,childH*2,  SWP_NOZORDER |SWP_SHOWWINDOW);
              itor++;
            }
          }else if (i>=1&& i<=3){
            pt.x = childW * (i-1); pt.y=childH*2;
            sz.cx = childW; sz.cy = childH;
            CRect rc(pt, sz);
            m_lstChildrect.push_back(rc);
            if (itor!= m_mapChildren.end()){
              GetCwnd(*itor)->SetWindowPos( 0, childW * (i-1) ,  childH*2, childW  ,childH ,  SWP_NOZORDER |SWP_SHOWWINDOW);
              itor++;
            }
          }
          else if (i>=4 &&i<=6){
            pt.x = childW*2; pt.y= childH * (i-4);
            sz.cx = childW; sz.cy = childH;
            CRect rc(pt, sz);
            m_lstChildrect.push_back(rc);
            if (itor!= m_mapChildren.end()){
              GetCwnd(*itor)->SetWindowPos( 0, childW*2 , childH * (i-4), childW  ,childH ,  SWP_NOZORDER |SWP_SHOWWINDOW);
              itor++;
            }
          }else{
            if (itor!= m_mapChildren.end()){
              GetCwnd(*itor)->ShowWindow(SW_HIDE);
              itor++;
            }
          }
           
        }
      }  

#endif
    }
    break;
  case  ID__9://9 layout
    {
      CRect rc;
      GetWindowRect(rc);
      int childW=rc.Width()/3,childH = rc.Height()/3;
      int i=0;
      for(MAP_PanelChildren::iterator itor= m_mapChildren.begin();i< 9;i++)
      {
        POINT pt; SIZE sz;
        pt.x = (i%3* childW); pt.y= i/3* childH;
        sz.cx = childW; sz.cy = childH;
        CRect rc(pt, sz);
        m_lstChildrect.push_back(rc);
        if (itor!= m_mapChildren.end()){
          if ( i<9)
            GetCwnd(*itor)->SetWindowPos( 0, (i%3* childW),i/3* childH,childW ,childH, SWP_NOZORDER |SWP_SHOWWINDOW);
          else
            GetCwnd(*itor)->ShowWindow(SW_HIDE);
          itor++;
        }
      }       
    }
    break;
  case  ID__12://12 layout
    {
#ifdef D_NORMAL12_LAYOUT
      CRect rc;
      GetWindowRect(rc);
      int childW=rc.Width()/4,childH = rc.Height()/3;
      int i=0;
      for(MAP_PanelChildren::iterator itor= m_mapChildren.begin();itor!= m_mapChildren.end();itor++)
      {
        if (i<12)
          GetCwnd(*itor)->SetWindowPos(0, (i%4* childW),i/4* childH,childW ,childH,  SWP_NOZORDER |SWP_SHOWWINDOW);
        else
           GetCwnd(*itor)->ShowWindow(SW_HIDE);
        i++;
      }      
#else
      CRect rc;
      GetWindowRect(rc);
      int childW=rc.Width()/4,childH = rc.Height()/4;
      int i=0;
      for(MAP_PanelChildren::iterator itor= m_mapChildren.begin();i< 13;i++)
      {
        POINT pt; SIZE sz;
        //
        {
          if (i==0){
            pt.x = ( childW); pt.y=   childH;
            sz.cx = childW*2; sz.cy = childH*2;
            CRect rc(pt, sz);
            m_lstChildrect.push_back(rc);
            if (itor!= m_mapChildren.end()){
              GetCwnd(*itor)->SetWindowPos(0, childW, childH,   childW*2 ,childH*2,  SWP_NOZORDER |SWP_SHOWWINDOW);
              itor++;
            }
          }
          else if (i>=1 && i<=13){
            int j =i-1;
            if (j>=5&& j< 7 ) 
              j+=2;
            else if (  j>=7)
              j+=4;
            pt.x = (j%4* childW); pt.y=   j/4* childH;
            sz.cx = childW; sz.cy = childH;
            CRect rc(pt, sz);
            m_lstChildrect.push_back(rc);
            if (itor!= m_mapChildren.end()){
              GetCwnd(*itor)->SetWindowPos(0, (j%4* childW),j/4* childH ,childW ,childH,  SWP_NOZORDER |SWP_SHOWWINDOW);
              itor++;
            }
          }
          else{
            if (itor!= m_mapChildren.end()){
              GetCwnd(*itor)->ShowWindow(SW_HIDE);
              itor++;
            }
          }
        }
      }      

#endif
    }
    break;
  case  ID__16://16 layout
    {
       CRect rc;
      GetWindowRect(rc);
      int childW=rc.Width()/4,childH = rc.Height()/4;
      int i=0;
      for(MAP_PanelChildren::iterator itor= m_mapChildren.begin();i<16;i++)
      {
        POINT pt; SIZE sz;
        pt.x = (i%4* childW); pt.y= i/4* childH;
        sz.cx = childW; sz.cy = childH;
        CRect rc(pt, sz);
        m_lstChildrect.push_back(rc);
        if (itor!= m_mapChildren.end()){

          if ( i<16)
            GetCwnd(*itor)->SetWindowPos( 0,(i%4* childW),i/4* childH,childW ,childH, SWP_NOZORDER |SWP_SHOWWINDOW);
          else
            GetCwnd(*itor)->ShowWindow(SW_HIDE);
          itor++;
        }
      }         
    }
    break;
  case  ID__32:////32 layout
    {
        CRect  rc;
      GetWindowRect(rc);
        
      int childW=rc.Width()/8,childH = rc.Height()/4;
       CRect  rc1(0,0,childW, childH);
      int i=0;
      for(MAP_PanelChildren::iterator itor= m_mapChildren.begin();i< 32;i++)
      {
        POINT pt; SIZE sz;
        pt.x = (i%8* childW); pt.y= i/8* childH;
        sz.cx = childW; sz.cy = childH;
        CRect rc(pt, sz);
        m_lstChildrect.push_back(rc);
        if (itor!= m_mapChildren.end()){
          if ( i<32){
          // GetApectRatioedHxW(  childW1, childH1, wOffset, hOffset);
            GetCwnd(*itor)->SetWindowPos( 0,(i%8* childW),i/8* childH,childW ,childH,SWP_NOZORDER |SWP_SHOWWINDOW);
          }
          else
            GetCwnd (*itor)->ShowWindow(SW_HIDE);
          itor++;
        }
      }      
    }
    break;
  case  ID__0:////1 big layout + 12 small layout
    {
      CRect rc, halfRightrc;
      GetWindowRect(rc);
      halfRightrc =rc;
      halfRightrc.left=0;
      halfRightrc.right=  rc.Width()/2/3;
      halfRightrc.top = 0;
      halfRightrc.bottom =  rc.Height()/4;
      int childW=rc.Width()/2,childH = rc.Height() ;
      int i=0;
      for(MAP_PanelChildren::iterator itor= m_mapChildren.begin();i< 13;i++)
      {
        POINT pt; SIZE sz;
        //
        {
          if (i==0){
            int hOffset=0, wOffset=0,childW1=childW,childH1 =childH;
            CRect rc1 =rc;
            rc1.left += rc1.Width()/2;
            GetApectRatioedHxW(  childW1, childH1, wOffset, hOffset);
            
            pt.x = wOffset+(i%8* childW1); pt.y= hOffset+i/8* childH1;
            sz.cx = childW1; sz.cy = childH1;
            CRect rc(pt, sz);
            m_lstChildrect.push_back(rc);
            if (itor!= m_mapChildren.end()){
              GetCwnd(*itor)->SetWindowPos( 0,wOffset+(i%8* childW1),hOffset+i/8* childH1,childW1 ,childH1,SWP_NOZORDER |SWP_SHOWWINDOW);
              itor++;
            }
          }
          else if ( i>=1&&i<=13){
            int hOffset=0, wOffset=0;
#define D_COLS 3
#define D_ROWS 4

            int subW= childW/D_COLS  ,subH=childH/D_ROWS;
            
            GetApectRatioedHxW(  subW, subH, wOffset, hOffset);
            int nIndex  = i-1;

            pt.x = wOffset + childW+(nIndex%3* subW); pt.y= (hOffset * (1+ nIndex/ D_ROWS) +hOffset*(nIndex/3))+nIndex/3* subH;
            sz.cx = subW; sz.cy = subH;
            CRect rc(pt, sz);
            m_lstChildrect.push_back(rc); 
            if (itor!= m_mapChildren.end()){
              GetCwnd(*itor)->SetWindowPos( 0, pt.x /*wOffset+ childW+(nIndex%3* subW)*/, pt.y /*(hOffset+hOffset*(nIndex/3))+nIndex/3* subH*/,subW ,subH,SWP_NOZORDER |SWP_SHOWWINDOW);
              itor++;
            }
          }
          else{
            if (itor!= m_mapChildren.end()){
              GetCwnd(*itor)->ShowWindow(SW_HIDE);
              itor++;
            }
          }
        }
      }      
    }
    break; 
 };
  this->Invalidate();
  return TRUE;
}
CVideoPanelChildDlg* CVideoPanelDlg::GetChildByUid(unsigned int uid)
{
  for(MAP_PanelChildren::iterator itor = m_mapChildren.begin();itor!= m_mapChildren.end();itor++)//text
  {//
    if ( (*itor)->GetUserID() == uid) return (*itor)->m_pChild1;//GetCwnd(*itor);
  }
  return NULL;
}
 int  CVideoPanelDlg::GetChildUIDByCWnd(CDialog * pDlg)
{
  for(MAP_PanelChildren::iterator itor = m_mapChildren.begin();itor!= m_mapChildren.end();itor++)//text
  {//
    if ( (*itor)->m_pChild1 == pDlg) return (*itor)->m_pChild1->m_nUserID;//GetCwnd(*itor);
    else if ( (*itor)->m_pChild2 == pDlg) return (*itor)->m_pChild2->m_nUserID;
  }
  return -1;
}
 bool compare(stChildDlg* a,stChildDlg* b)
{
  return a->pos < b->pos;
}
LRESULT CVideoPanelDlg::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
  // TODO: Add your specialized code here and/or call the base class
  if (message  == WM_VIDEO_CHILD){
	 // return TRUE;
    stMsgVideoChild* pChildMsg= (stMsgVideoChild*) wParam;
    if (pChildMsg->Cmd == emMsgVideo_draw){
      //MAP_PanelChildren::iterator itor = m_mapChildren.find( pChildMsg->uid);
      CVideoPanelChildDlg *p=GetChildByUid( pChildMsg->uid);

      if ( p == NULL){
 // #define D_TEST 3
  #ifdef D_TEST
        for(int i=0;i< D_TEST;i++)//text
        {//
          stChildDlg* p1 = new stChildDlg(this);
          p1->m_pChild1= p = new CVideoPanelChildDlg(this);
          p->m_nUserID = pChildMsg->uid;
          p->Create(IDD_DIALOG_VIDEO_CHILD,this);
          m_mapChildren.push_back(p1);
          p->m_nUserID =(i);
          p->ShowWindow(SW_SHOW);
          p->SetWindowText(pChildMsg->m_strAlias);
          p->BringWindowToTop();
        }
        //OnMenuItem( ID_Normal);
		m_nLayoutmenuID=ID_Normal;
  #else
      if (m_pMCCDocFlg){
        int nIndex=-1;
        int uid =pChildMsg->uid>0?pChildMsg->uid: m_pMgr->GetOPALMgr()->GetClientID();
        PStringArray& arr=m_pMgr->GetOPALMgr()->m_arrShowersOrder;
        for(int i=0;i< m_pMgr->GetOPALMgr()->m_arrShowersOrder.GetSize();i++){
          if (arr[i].AsUnsigned() == uid){
            nIndex=i;
            break;
          }
          if (nIndex>=0) 
            break;
        }
#define D_VIDEO_MAX 100
        if (nIndex<0){
          //not found we use tail order to fill pos
          nIndex=-1;
         
          for (int i=0;i<3;i++){
             bool bFound= false;
            for( MAP_PanelChildren::iterator itor = m_mapChildren.begin(); itor!=m_mapChildren.end();itor++)//text
            {//
              if  ((*itor)->pos ==i){
                bFound = true;
                break;
              }
               
            }
            if (bFound ==false) {
              nIndex=i;
              break;
            }
          }
          //nIndex =  m_mapChildren.size();
           
        }
         if (nIndex<0|| nIndex==D_VIDEO_MAX){
            nIndex=0;
         }
          p = new CVideoPanelChildDlg(m_pMCCDocFlg);
          stChildDlg* p1 = new stChildDlg(this);
          p1->pos =nIndex;
          p1->m_pChild1= p;
          p->m_nUserID = pChildMsg->uid;
          p->Create(IDD_DIALOG_VIDEO_CHILD,m_pMCCDocFlg);
          m_mapChildren.push_back ( p1);
          m_mapChildren.sort(compare);
        //  std::sort(m_mapChildren.begin(),m_mapChildren.end(),  compare );
          p->ShowWindow(SW_SHOW);
          p->SetWindowText(pChildMsg->m_strAlias);
          p->SetVideoWindowTitle(true, pChildMsg->m_strAlias);
            p->BringWindowToTop(); 
          int  pos= nIndex+1;//m_mapChildren.size() ;order is 1-3
            ///
            m_pMgr->GetSpriteX2Ctrl()->OnNotifyVideoPanel( (p->m_nUserID==0? m_pMgr->GetMEMgr()->GetID():p->m_nUserID ),pos, "1");
            ///
       }else
         return TRUE;
		/* m_pMCCDocFlg->UpdateUI();
		  
		 m_pMCCDocFlg->UpdateWindow();
		 m_pMCCDocFlg->RedrawWindow();
		 m_pMCCDocFlg->SetFocus();
		 m_pMCCDocFlg->SetActiveWindow(); */
  #endif
        int nChild = m_mapChildren.size();

   
       if (m_bMCCLayout)
           OnMenuItem(m_nLayoutmenuID);
        else{
       
        }
        
        //
      }else{//already drawing
        //draw it 
  #ifdef D_TEST
        for( MAP_PanelChildren::iterator itor = m_mapChildren.begin(); itor!=m_mapChildren.end();itor++)//text
        {//
          (*itor)->m_pChild1->SetFrameSize(pChildMsg->w, pChildMsg->h);
          (*itor)->m_pChild1->SetFrameData(0,0,pChildMsg->w, pChildMsg->h,(const BYTE*) pChildMsg->pData);
        }
        return 0;
  #else
  //     p= itor->second;
  #endif
        //if (p->m_nFrameW != w|| p->m_nFrameH)
      }
      //draw it 
 

      p->SetFrameSize(pChildMsg->w, pChildMsg->h);
      p->SetFrameData(0,0,pChildMsg->w, pChildMsg->h,(const BYTE*) pChildMsg->pData);
      if (pChildMsg->m_pCallback ){
        //pChildMsg->m_pCallback->UnLockMsgBuffer();
      }
      return TRUE;
    }
    else if (pChildMsg->Cmd == emMsgVideo_close){
      stChildDlg *pChild =0;
      int pos=0;
        for( MAP_PanelChildren::iterator itor = m_mapChildren.begin(); itor!=m_mapChildren.end();itor++)//text
        {//
          if (pChildMsg->uid == (*itor)->GetUserID()) 
          {
            pChild = (*itor); m_mapChildren.erase(itor);
            pos++;
            break;
          }else
            pos++;
          
          
        }
        if (pChild){
          //delete it 
          m_pMgr->GetSpriteX2Ctrl()->OnNotifyVideoPanel((pChild->GetUserID()==0? m_pMgr->GetMEMgr()->GetID():pChild->GetUserID() ),
            pos, "0");
          GetCwnd(pChild)->DestroyWindow();delete pChild;pChild =0;
        }
        delete pChildMsg;pChildMsg=0;// it is a postmessage
       
        m_pChildMouseDown=NULL;
        return TRUE;
    }
   
    
  } 
  //else if(message ==WM_KEYDOWN&& wParam==VK_ESCAPE)     
  //    return TRUE;


  return CDialog::DefWindowProc(message, wParam, lParam);
}

void CVideoPanelDlg::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
  // TODO: Add your message handler code here
	CMenu menu; 
  LANGID  id = GetUserDefaultUILanguage();
   
  if (id != 2052)
	  VERIFY(menu.LoadMenu(IDR_MENU_VIDEO_LAYOUT_CTRL_EN));
  else
	  VERIFY(menu.LoadMenu(IDR_MENU_VIDEO_LAYOUT_CTRL));

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);
	CWnd* pWndPopupOwner = this;
  pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,
		pWndPopupOwner);
}

void CVideoPanelDlg::OnDestroy()
{

  CDialog::OnDestroy();
  
  // TODO: Add your message handler code here
}

void CVideoPanelDlg::OnBnClickedCancel()
{
  // TODO: Add your control notification handler code here
 // OnMenuItem(ID_Menu_Layout_normal);
  OnCancel();
}

void CVideoPanelDlg::OnBnClickedCancel2()
{
  // TODO: Add your control notification handler code here
}

BOOL CVideoPanelDlg::OnEraseBkgnd(CDC* pDC)
{
  // TODO: Add your message handler code here and/or call default
    /*  CBrush   backBrush(RGB(0,   0,   0));   
      CBrush*   pOldBrush   =   pDC->SelectObject(&backBrush);   
      CRect   rect;   
      pDC->GetClipBox(&rect);             
      pDC->PatBlt(rect.left,   rect.top,   rect.Width(),     
      rect.Height(),   PATCOPY);   
      pDC->SelectObject(pOldBrush);   
			return TRUE;*/
  return CDialog::OnEraseBkgnd(pDC);
}

void CVideoPanelDlg::OnPaint()
{
  CPaintDC dc(this); // device context for painting
  ///m_pMCCDocFlg->SendMessage(WM_PAINT);
				/*m_pMCCDocFlg->SendMessage(WM_PAINT);
				m_pMCCDocFlg->PostMessage(WM_PAINT);
		m_pMCCDocFlg->UpdateWindow();*/
		
#if 0
	CRect vRect;
	GetWindowRect(&vRect);
 
  int nRows, nCols, GridH,GridW;
  if (m_nLayoutmenuID == ID__2 ){
    return ;
    nRows=1, nCols=2, GridH = vRect.Height(),GridW=vRect.Width()/2;
  }else if (m_nLayoutmenuID == ID__4 ){
    nRows=2, nCols=2, GridH = vRect.Height()/2,GridW=vRect.Width()/2;
  }else if (m_nLayoutmenuID ==ID__6 ){
    nRows=3, nCols=3, GridH = vRect.Height()/3,GridW=vRect.Width()/3;
  }else if (m_nLayoutmenuID == ID__9 ){
    nRows=3, nCols=3, GridH = vRect.Height()/3,GridW=vRect.Width()/3;
  }else if (m_nLayoutmenuID == ID__12 ){
    nRows=4, nCols=4, GridH = vRect.Height()/4,GridW=vRect.Width()/4;
  }else if (m_nLayoutmenuID== ID__16 ){
    nRows=4, nCols=4, GridH = vRect.Height()/4,GridW=vRect.Width()/4;
  }else if (m_nLayoutmenuID == ID__32 ){
    nRows=4, nCols=8, GridH = vRect.Height()/4,GridW=vRect.Width()/8;
  }else if (m_nLayoutmenuID == ID__0){
    nRows=1, nCols=2, GridH = vRect.Height() ,GridW=vRect.Width()/2;
  }else if (m_nLayoutmenuID == ID_Normal){
   nRows=1, nCols=2, GridH = vRect.Height() ,GridW=vRect.Width() - D_MCC_DEFAULT_RIGHT_VIDEO_PANEL_W;
  }
  CPoint pointbegin, pointEnd;
	//网格线 水平 start
	//nInterval =  nKeduMaxH/D_KEDU_COUNT*1.6;
	CPen   pen(PS_SOLID,1,(COLORREF) RGB(136,90,44)); 
	CPen*   pOldPen   =   dc.SelectObject(&pen); 
	pointbegin.x= 0;
	pointEnd.x= vRect.Width();
  //int nKeduMaxH = vRect.Height();
  //int nKeduMaxW = vRect.Width();
	for (int i=0;i<= nRows;i++)
	{
		pointbegin.y=   (i * GridH );
		pointEnd.y= pointbegin.y;
		dc.MoveTo(pointbegin);
		dc.LineTo(pointEnd);


	}
	//网格线 垂直
	pointbegin.y= 0;
	pointEnd.y= vRect.Height();
	for (int i=0;i<=nCols;i++)
	{
		pointbegin.x= (i * GridW /**fAspect*/);
		pointEnd.x= pointbegin.x;
		dc.MoveTo(pointbegin);
		dc.LineTo(pointEnd);

	}	

	//网格线 end
	//for (int i=0;i< m_xhvec.size() ;i++)
	//	m_xhvec[i]->OnPaint(dc, i, vRect, fAspect);
	//draw 刻度数字
	 
  CRect textRect;
  int j=0, i=0;
	for (  i=0;i< nRows ;i++)
	{
    for(  j=0;j<nCols;j++){
    }
		pointbegin.y= (i * GridH );
		pointEnd.y= pointbegin.y + GridH;
		//dc.MoveTo(pointbegin);
		//dc.LineTo(pointEnd);
		textRect.top =  i * GridH ;
		textRect.bottom =  i * GridH +GridH ;
    textRect.left =  j * GridW ;
    textRect.right =  j * GridW +GridH ;
		char tmp[25]={0};
    if (pointbegin.y>0){
			dc.DrawText(itoa(i*nCols+j, tmp, 10)  ,&textRect, DT_CENTER);
    }
	}
  //draw extern style
  if (m_nLayoutmenuID == ID__0){//经典画面
    int subRows= 4,subCols=3,subH = GridH/subRows, subW= GridW/subCols;
	  pointbegin.x= GridW;
	  pointEnd.x= vRect.Width();
	  for (int i=0;i<= subRows;i++)
	  {
		  pointbegin.y=   (i * subH );
		  pointEnd.y= pointbegin.y;
		  dc.MoveTo(pointbegin);
		  dc.LineTo(pointEnd);
	  }
    //cols
	  pointbegin.y= 0;
	  pointEnd.y= vRect.Height();
	  for (int i=0;i<=subCols;i++)
	  {
		  pointbegin.x= GridW+(i * subW /**fAspect*/);
		  pointEnd.x= pointbegin.x;
		  dc.MoveTo(pointbegin);
		  dc.LineTo(pointEnd);

	  }	
  }

  else if (m_nLayoutmenuID == ID_Normal ){
    int subRows= vRect.Height() / D_MCC_DEFAULT_RIGHT_VIDEO_PANEL_H ;
    pointbegin.x= vRect.Width()-D_MCC_DEFAULT_RIGHT_VIDEO_PANEL_W ;
	  pointEnd.x= vRect.Width();
	  for (int i=0;i<= subRows;i++)
	  {
		  pointbegin.y=   (i * D_MCC_DEFAULT_RIGHT_VIDEO_PANEL_H );
		  pointEnd.y= pointbegin.y;
		  dc.MoveTo(pointbegin);
		  dc.LineTo(pointEnd);
	  }
  
  }
  else if (m_nLayoutmenuID == ID__6){
	  CPen   pen2(PS_SOLID,2,(COLORREF) RGB(0,0,0)); 
	  CPen*   pOldPen2   =   dc.SelectObject(&pen2); 
    //1
    pointbegin.x =0;
    pointbegin.y=   GridH;
		pointEnd.x= GridW*2;
		pointEnd.y= GridH;
		dc.MoveTo(pointbegin);
		dc.LineTo(pointEnd);
    //2
    pointbegin.x =GridW;
    pointbegin.y=   0;
		pointEnd.x= GridW;
		pointEnd.y= GridH*2;
		dc.MoveTo(pointbegin);
		dc.LineTo(pointEnd);
    dc.SelectObject(pOldPen2);	
  }
 else if (m_nLayoutmenuID == ID__12){
	  CPen   pen2(PS_SOLID,2,(COLORREF) RGB(0,0,0)); 
	  CPen*   pOldPen2   =   dc.SelectObject(&pen2); 
    //1
    pointbegin.x =GridW;
    pointbegin.y=   GridH*2;
		pointEnd.x= GridW*3;
		pointEnd.y= pointbegin.y;
		dc.MoveTo(pointbegin);
		dc.LineTo(pointEnd);
    //2
    pointbegin.x =GridW*2;
    pointbegin.y=   GridH;
		pointEnd.x= GridW*2;
		pointEnd.y= GridH*3;
		dc.MoveTo(pointbegin);
		dc.LineTo(pointEnd);
    dc.SelectObject(pOldPen2);	
  } 	
 dc.SelectObject(pOldPen);	
 /* CRect noEraseRect(0, 0, 200, 200);
  dc.ExcludeClipRect(noEraseRect);*/
#endif
}

void CVideoPanelDlg::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
  // TODO: Add your message handler code here and/or call default
  if (nChar == VK_ESCAPE)
  {
    OnMenuItem(ID_Menu_Layout_normal);
    return ;
  }
  CDialog::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CVideoPanelDlg::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
  // TODO: Add your message handler code here and/or call default

  CDialog::OnSysKeyDown(nChar, nRepCnt, nFlags);
}
void CVideoPanelDlg::DoSwapWindow(CDialog* pSrc, CDialog* pDst, CPoint& mouseDstPos )
{ 
   if (pSrc ==NULL || pDst == NULL|| pSrc ==pDst ) 
     return ;
  
   unsigned int uidSrc =GetChildUIDByCWnd( pSrc) ;
   unsigned int uidDst= GetChildUIDByCWnd(pDst);
  //CVideoPanelChildDlg *pDst =0;
  //MAP_PanelChildren::iterator  iSrc= m_mapChildren.end(),iDst= m_mapChildren.end();
  CRect rc;
  for(MAP_PanelChildren::iterator itor = m_mapChildren.begin();itor!= m_mapChildren.end();itor++)//text
  {//
    GetCwnd(*itor)->GetWindowRect(&rc);
    ClientToScreen(&rc);
    if ((*itor)->GetUserID()!= uidSrc&& rc.PtInRect(mouseDstPos)) 
    {pDst =  GetCwnd(*itor) ; break;}
  }
  //
  CRect rcSrc,rcDst;
  pSrc->GetWindowRect(&rcSrc);
  pDst->GetWindowRect(&rcDst);
  for(MAP_PanelChildren::iterator itor = m_mapChildren.begin();itor!= m_mapChildren.end();itor++)//text
  {//
    if ((*itor)->GetUserID() == uidSrc)  {
       
      //GetCwnd(*itor) = pDst;
      SetCwnd(*itor, pDst);
      pDst->MoveWindow(&rcSrc);
    }
    else if ((*itor)->GetUserID() == uidDst ) {
      SetCwnd(*itor, pSrc);
      //*itor = pSrc;
      pSrc->MoveWindow(&rcDst);
    }
  }
 // SetChildLayout(m_nLayoutmenuID);
  m_pChildMouseDown=0;
 // std::swap(*uidSrc, *uidDst);
 // iter_swap(&m_mapChildren, &m_mapChildren);
  //m_mapChildren.iter.iter_swap(uidSrc, uidDst);
}

 void CVideoPanelDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
  // TODO: Add your message handler code here and/or call default
  if (m_pChildMouseDown) {
    for(LST_PanelChildRects::iterator i= m_lstChildrect.begin();i!= m_lstChildrect.end();i++){
      if (i->PtInRect(point)){
        m_pChildMouseDown->MoveWindow(*i);
        m_pChildMouseDown=0;
        break;
      }
    }
    
  }
  
  CDialog::OnLButtonUp(nFlags, point);
}
stChildDlg::~stChildDlg()
{
  if (m_pChild1) {delete m_pChild1;m_pChild1=0;}

  if (m_pChild2) {
    m_pParent->m_pMgr->GetSpriteX2Ctrl()->DestroyPopIMDlg(m_pParent->m_nRoomID);
    m_pChild2=0;
  }
  

} 
unsigned int stChildDlg::GetUserID()
{
  if (m_pChild1) return m_pChild1->m_nUserID;
  else if (m_pChild2) return m_pChild2->m_nUserID;
  else
    return 0;
};

int CVideoPanelDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  if (CDialog::OnCreate(lpCreateStruct) == -1)
    return -1;

  // TODO:  Add your specialized creation code here
  lpCreateStruct->dwExStyle|= (WS_EX_TOPMOST|WS_CAPTION);
  return 0;
}
void CVideoPanelDlg::SwitchToTheVideoAdcanceWindow()
{
  g_VideoLayout = 1/*em_videoLayoutAdvanced*/;
  ShowWindow(SW_SHOW);
  SetWindowPos(&wndTopMost,   0,   0,   0,   0,   SWP_NOSIZE   |   SWP_NOMOVE); 

}
void CVideoPanelDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
  CDialog::OnShowWindow(bShow, nStatus);
  m_pMgr->GetSpriteX2Ctrl()->ShowOrHidPopIMDlg(m_nRoomID, bShow?true: false); 
  // TODO: Add your message handler code here
}
void  CVideoPanelDlg::SetExtendtoRight(bool b)
{
  if (m_bMCCLayout && m_pMCCDocFlg&&m_pMCCDocFlg->IsWindowVisible() &&m_nLayoutmenuID == ID_Normal){
    CRect rc;
    GetWindowRect(rc);
    if (b== true){
      m_pMCCDocFlg->SetWindowPos(0, 0,0, rc.Width(), rc.Height() , SWP_SHOWWINDOW|SWP_NOACTIVATE);
    }else
     m_pMCCDocFlg->SetWindowPos(0, 0,0, rc.Width()-D_MCC_DEFAULT_RIGHT_VIDEO_PANEL_W, rc.Height() , SWP_SHOWWINDOW|SWP_NOACTIVATE);
  }
}
BOOL CVideoPanelDlg::DestroyWindow()
{
  // TODO: Add your specialized code here and/or call the base class
  for( MAP_PanelChildren::iterator itor = m_mapChildren.begin(); itor!=m_mapChildren.end();itor++){
    GetCwnd(*itor)->DestroyWindow();delete (*itor);//->second;
  }
  m_mapChildren.clear();
  if (m_bMCCLayout &&m_pMCCDocFlg ){
    m_pMCCDocFlg =NULL;
     //m_pMCCDocFlg->DestroyWindow();
      m_pMgr->GetSpriteX2Ctrl()->DestroyPopIMDlg(m_nRoomID); //detroy it in yyctrl
   // delete m_pMCCDocFlg;
  }
  return CDialog::DestroyWindow();
}
void CVideoPanelDlg::SetChildWinSize(bool bFull, CVideoPanelChildDlg * pChild)
{
 if (bFull ){
  /*  pChild->GetWindowRect(& pChild->m_rcPreRect);
     SetFullScreen(true,pChild->GetSafeHwnd() );
     if (m_pMCCDocFlg)
        m_pMCCDocFlg->ShowWindow(SW_HIDE);
      
    for(MAP_PanelChildren::iterator i=  m_mapChildren.begin();i!=m_mapChildren.end();i++  ){
      if (GetCwnd(*i)!= pChild)
        GetCwnd(*i)->ShowWindow(SW_HIDE);
    }*/
    //hide all children
  }else
  {
    
     if ( m_pMCCDocFlg&&  IsMCCNormalLayout()){
        m_pMCCDocFlg->ShowWindow(SW_SHOW);
     }    
     pChild->SetWindowPos(0,pChild->m_rcPreRect.left,pChild->m_rcPreRect.top ,pChild->m_rcPreRect.Width() ,pChild->m_rcPreRect.Height(), SWP_NOZORDER |SWP_SHOWWINDOW);
     OnMenuItem(m_nLayoutmenuID);
  }
}

void CVideoPanelDlg::OnClose()
{
  // TODO: Add your message handler code here and/or call default
 // HungupCall();
  CDialog::OnClose();
}


void CVideoPanelDlg::OnSize(UINT nType, int cx, int cy)
{
  CDialog::OnSize(nType, cx, cy);
  if (m_pMCCDocFlg)
    m_pMCCDocFlg->MoveWindow(0,0,cx,cy);
  // TODO: Add your message handler code here
}
void CVideoPanelDlg::Notify(int cmd, int uid)
{
  if (uid == 0 ) 
    uid= m_pMgr->GetMEMgr()->GetID();
  ///cmd
  if (cmd ==1){
    PString strNotify;
    strNotify.sprintf("%d;0;%u;%d", (int)em_MCC_Nofify, uid, m_pMgr->GetMEMgr()->m_nCurectEnteredRoom );
	  m_pMgr->GetSpriteX2Ctrl()->OnNotify_WM(em_MCC, strNotify);
  }
  else if (cmd ==2){
    PString strNotify;
    strNotify.sprintf("%d;1;%u;%d", (int)em_MCC_Nofify, uid, m_pMgr->GetMEMgr()->m_nCurectEnteredRoom );
	  m_pMgr->GetSpriteX2Ctrl()->OnNotify_WM(em_MCC, strNotify);
  }
  // TODO: Add your message handler code here
}