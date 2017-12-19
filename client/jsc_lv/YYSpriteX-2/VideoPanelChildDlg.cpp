// VideoPanelChildDlg.cpp : implementation file
//

#include "stdafx.h"
#include "YYSpriteX-2.h"
#include "VideoPanelChildDlg.h"
#include ".\videopanelchilddlg.h"
#include ".\VideoPanelDlg.h"
#include "OptionsExDlg.h"
#include "myclient.h"

extern void  SetFullScreen(bool bfull, HWND h)    ;
extern int g_VideoLayout;
extern int jpegCapture(char* filename, int quality, HWND h);
extern  PString  GetUserImgPath(unsigned int uid);
extern PString UTF8ToPlainText(const char* lpszContent);
int CreateBMPFile2(LPTSTR pszFile, HBITMAP hbitmap, HDC hdc)
{	
  PBITMAPINFO pbi;
//λͼ��Ϣ	
BITMAP bitmap;
//λͼ����	
int cClrBits;
//ÿ�����ص���ɫλ��	
BITMAPFILEHEADER bitmapfileheader;
	FILE *fp;
	BYTE *pimagedata ;
//���λͼ��Ϣ��������ͼ������   
  if (!GetObject(hbitmap, sizeof(BITMAP), (LPSTR)&bitmap))//���ʧ�ܷ���0	
  {        MessageBox(NULL,TEXT("GetObject����"),NULL,MB_OK);
		return 0;
	}		//ɫ��ƽ����*ÿͼ�ص�ɫ��λ��,  1<< cClrBits��ʾ����ɫ��	
  cClrBits = (WORD)(bitmap.bmPlanes*bitmap.bmBitsPixel);
		//�����ڴ�	
  if(cClrBits != 24)	{		pbi=(PBITMAPINFO)malloc(sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)*(1<< cClrBits));
	}	else	{		pbi=(PBITMAPINFO)malloc(sizeof(BITMAPINFOHEADER));
//24λλͼû�е�ɫ��	
  }	
if(!pbi)	{		MessageBox(NULL,TEXT("�ڴ治��"),TEXT("ERROR��"),MB_OK);
		return 0;
	}//���λͼ��Ϣͷ   
pbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
//λͼ��Ϣͷ�Ĵ�С   
pbi->bmiHeader.biWidth = bitmap.bmWidth;
     pbi->bmiHeader.biHeight = bitmap.bmHeight;
     pbi->bmiHeader.biPlanes = bitmap.bmPlanes;
     pbi->bmiHeader.biBitCount = bitmap.bmBitsPixel;
    pbi->bmiHeader.biClrUsed = (1<<cClrBits);
 //ʹ�õ���ɫ��	
    pbi->bmiHeader.biCompression = BI_RGB;
//λͼû��ѹ�������ñ�־		//����λͼ���ݴ�С��ÿһ����ռ���ֽ�Ϊ4�ı���   
    pbi->bmiHeader.biSizeImage = ((pbi->bmiHeader.biWidth*cClrBits/8+3) & ~3)* pbi->bmiHeader.biHeight;
	pbi->bmiHeader.biClrImportant = 0;
//����ļ�ͷ	
  bitmapfileheader.bfType=0x4d42;
//��ʶ�ļ������ͣ��涨ΪBM	
  bitmapfileheader.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+pbi->bmiHeader.biClrUsed*sizeof(RGBQUAD);
//ͼ������ƫ��	
  bitmapfileheader.bfSize=bitmapfileheader.bfOffBits+pbi->bmiHeader.biSizeImage;
//���ļ���С	
  bitmapfileheader.bfReserved1=0;
//�����֣�δʹ��	
  bitmapfileheader.bfReserved2=0;
//�����ļ�	
  fp=_tfopen(pszFile,TEXT("wb+"));
    if(!fp)	{		MessageBox(NULL,TEXT("�޷������ļ�"),TEXT("ERROR��"),MB_OK);
		return 0;
	}//д���ļ�ͷ	
    if(fwrite(&bitmapfileheader,sizeof(BITMAPFILEHEADER),1,fp)==0)	{       MessageBox(NULL,TEXT("��������"),TEXT("ERROR!"),MB_OK);
	   return 0;
    }//д��λͼ��Ϣ	
    if(fwrite(pbi,sizeof(BITMAPINFOHEADER)+pbi->bmiHeader.biClrUsed*sizeof(RGBQUAD),1,fp)==0)	{       MessageBox(NULL,TEXT("��������"),TEXT("ERROR!"),MB_OK);
	   return 0;
    }//д��λͼ����	
pimagedata=(BYTE *)malloc(pbi->bmiHeader.biSizeImage);
	if(!pimagedata)	{		MessageBox(NULL,TEXT("�ڴ治��"),TEXT("ERROR��"),MB_OK);
		return 0;
	}    if (!GetDIBits(hdc,hbitmap,0,pbi->bmiHeader.biHeight,pimagedata,pbi,DIB_RGB_COLORS))	{		MessageBox(NULL,TEXT("��ȡλͼ����ʧ��"),TEXT("ERROR��"),MB_OK);
		return 0;
    } 	if(fwrite(pimagedata,pbi->bmiHeader.biSizeImage,1,fp)==0)	{       MessageBox(NULL,TEXT("��������"),TEXT("ERROR!"),MB_OK);
	   return 0;
    }//�ر��ļ�	
  if(fclose(fp))	{		MessageBox(NULL,TEXT("�ļ��޷��ر�"),TEXT("ERROR��"),MB_OK);

		return 0;

	}//�ͷ��ڴ�	
  free(pbi);

	free(pimagedata);

	return 1;

}
 

// CVideoPanelChildDlg dialog
int CVideoPanelChildDlg::m_nTitleHeight = 0/*20*/;
IMPLEMENT_DYNAMIC(CVideoPanelChildDlg, CDialog)
CVideoPanelChildDlg::CVideoPanelChildDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CVideoPanelChildDlg::IDD, pParent),m_nFrameW(0),m_nFrameH(0),m_nUserID(0),m_bFullScreen(false)
{
}

CVideoPanelChildDlg::~CVideoPanelChildDlg()
{
}

void CVideoPanelChildDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CVideoPanelChildDlg, CDialog)
  ON_WM_CONTEXTMENU()
  ON_COMMAND_RANGE(ID_Menu_Layout_normal, ID_VIEOLAYOUT_HUNGUP, OnMenuItem)

  ON_WM_SIZE()
  ON_WM_KEYUP()
  
  ON_WM_LBUTTONUP()
  ON_WM_LBUTTONDOWN()
  ON_WM_SETCURSOR()
  ON_WM_CTLCOLOR()
  ON_BN_CLICKED(IDOK, OnBnClickedOk)
  ON_WM_ERASEBKGND()
  ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()

void CVideoPanelChildDlg::OnMenuItem(UINT nID) 
{
    // TODO: Add your command handler code here
	switch (nID)
  {
  case ID_VIEOLAYOUT_USERINFO:
    {
      ((CVideoPanelDlg*)( GetParent()->GetParent()))->Notify(2, m_nUserID);
    }
    break;
  case ID_VIEOLAYOUT_COPYALIAS:
    {
      if(OpenClipboard())
          {
            PString str=  UTF8ToPlainText(g_MyClient->GetAliasName(g_MyClient->GetID()));
	          HGLOBAL clipbuffer;
	          char * buffer;
	          EmptyClipboard();
	          clipbuffer = GlobalAlloc(GMEM_DDESHARE, str.GetLength()+1);
	          buffer = (char*)GlobalLock(clipbuffer);
	          strcpy(buffer, (const char*)str); 
	          GlobalUnlock(clipbuffer);
	          SetClipboardData(CF_TEXT,clipbuffer);
	          CloseClipboard();
          }
      //SetClipboardData(CF_TEXT,(const char*)g_MyClient->GetAliasName(g_MyClient->GetID()));
    }
    break;
  case ID_VIEOLAYOUT_COPYID:
    { 
      if(OpenClipboard())
          {
        PString str=PString(g_MyClient->GetID());
	          HGLOBAL clipbuffer;
	          char * buffer;
	          EmptyClipboard();
	          clipbuffer = GlobalAlloc(GMEM_DDESHARE, str.GetLength()+1);
	          buffer = (char*)GlobalLock(clipbuffer);
	          strcpy(buffer, (const char*)str);
	          GlobalUnlock(clipbuffer);
	          SetClipboardData(CF_TEXT,clipbuffer);
	          CloseClipboard();
      }
    }
    break;
  case ID_VIEOLAYOUT_CLOSECAMERA:
    {
     
    }
    break;
  case ID_VIEOLAYOUT_CLOSEAUDIO:
    {
        
        
       
    }
    break;  
  case ID_VIEOLAYOUT_TAKEPIC:
    {
      CRect rcSrc;
      m_wndVideo.GetWindowRect(&rcSrc);
      PTime t;
     
      PString strpath =GetUserImgPath(g_MyClient->GetID())+ "\\cc_" +t.AsString(PTime::ShortISO8601)+".jpg"; 
#if 0
      HDC hdc,hmemdc;
HBITMAP  hbitmap=0;;
hdc=m_wndVideo.GetDC()->m_hDC;
hmemdc=CreateCompatibleDC(hdc);
hbitmap = CreateCompatibleBitmap(hdc, rcSrc.Width(),rcSrc.Height());
//������hdc���ݵ��ڴ�DC
SelectObject(hmemdc,hbitmap);
//��hdc��ͼ���Ƶ��ڴ�DC,����hbitmap����ʶ��λͼ��LPVOIDbmBits����ͼ��������
BitBlt(hmemdc,0,0,rcSrc.Width(),rcSrc.Height(),hdc,0,0,SRCCOPY);
CreateBMPFile2(strpath,hbitmap,hdc);
DeleteDC (hmemdc);
DeleteObject(hbitmap); 
#else
       
      jpegCapture((char*)(const char*)strpath, 90, m_wndVideo.GetSafeHwnd());
#endif
    }
    break;   
   
  default:
   ;// ((CVideoPanelDlg*) GetParent())->OnMenuItem(nID);

  };
}
BOOL CVideoPanelChildDlg::OnInitDialog()
{
  CDialog::OnInitDialog();

  // TODO:  Add extra initialization here
	//return TRUE;
	//load bitmap button
	// video Bitmap button
    m_hCursor=AfxGetApp()->LoadCursor(IDC_CURSOR4);  
    m_hDefaultCursor=GetCursor();
  CRect rect;
  GetClientRect(rect);
 // rect.top+= m_nTitleHeight;
  //rect.bottom+= m_nTitleHeight;
 	m_wndVideo.Create(WS_CHILD|WS_VISIBLE,rect,this,AFX_IDW_PANE_FIRST);
	m_wndVideo.ShowWindow(SW_SHOW);
  m_wndVideo.m_bEnableDragMouse = false;


	//
 // ::MoveWindow
 
	//m_IncomingNotify.GetDC()->SetTextColor(RGB(255,255,255));
	//CString str = m_strRemoteUserID;
	//str.Format("Incoming from : %s ", m_strRemoteUserID.GetBuffer());
	//CString strTitle;
	////if (m_strRemoteUserID=="0")
 ////   strTitle= g_GetStrings( "local");
	////else
	////	strTitle=m_strRemoteUserID +"("+ m_strRemoteUserDisplayName+")";
	// 
	//this->SetWindowText(strTitle);
 	//SetWindowPos(NULL, 0, 0, D_OREIGNAL_W, D_OREIGNAL_H, SWP_NOMOVE);
	//SetWindowPosInScreen();
 
  return TRUE;  // return TRUE unless you set the focus to a control
  // EXCEPTION: OCX Property Pages should return FALSE
}
void CVideoPanelChildDlg::SetFrameSize(int w,int h)
{
  //if (m_nFrameW != w || m_nFrameH != h){
	 // m_nFrameW = w;m_nFrameH =h;
  //  #ifdef LPN_ACTIVEX_PRJ
	 //   AFX_MANAGE_STATE(AfxGetStaticModuleState());
  //  #else
  //  #endif
  //  SetWindowPos(NULL, 0, 0, w,h,SWP_NOMOVE);
  //   m_wndVideo.SetWindowPos(NULL, 0,0,cx,cy, SW_SHOWNORMAL);
  //}
 
	 
}
bool CVideoPanelChildDlg::SetFrameData(unsigned x, unsigned y,
                                        unsigned w, unsigned h,
                                        const BYTE * data
                                        )
{
    //  PostMessage(WM_SHOWWINDOW, TRUE,SW_PARENTOPENING );

	if(IsWindowVisible())
	{
		m_wndVideo.Draw("", data, 0, w,h);
		return true;
	}else
		return false;
}


void CVideoPanelChildDlg::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
  // TODO: Add your message handler code here
	CMenu menu; 
  LANGID  id = GetUserDefaultUILanguage();
   
  if (id != 2052)
	  VERIFY(menu.LoadMenu(IDR_MENU_VIDEO_LAYOUT_CTRL_EN));
  else
	  VERIFY(menu.LoadMenu(IDR_MENU_VIDEO_LAYOUT_POP));

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);
	CWnd* pWndPopupOwner = this;
  pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,
		pWndPopupOwner);
}

void CVideoPanelChildDlg::OnSize(UINT nType, int cx, int cy)
{
  CDialog::OnSize(nType, cx, cy);
 //  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  if (m_wndVideo.GetSafeHwnd() !=NULL){
    m_wndVideo.SetWindowPos(NULL, 0, m_nTitleHeight,cx,cy-m_nTitleHeight, 0);
   // CRect rc(0,0,0,rect.Height() - m_nTitleHeight);
    GetDlgItem( IDC_STATIC_VIDEO_TITLE)->MoveWindow(0,0, cx, m_nTitleHeight);

  }
  // TODO: Add your message handler code here
}

void CVideoPanelChildDlg::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
  // TODO: Add your message handler code here and/or call default
  if (nChar == VK_ESCAPE)//VK_F11
  {
    OnMenuItem(ID_Menu_Layout_normal);
    return ;
  }
  CDialog::OnKeyUp(nChar, nRepCnt, nFlags);
}

 

void CVideoPanelChildDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
  // TODO: Add your message handler code here and/or call default
  //((CVideoPanelDlg*) GetParent())->DoSwapWindow(((CVideoPanelDlg*) GetParent())->m_pChildMouseDown, this, point);
  CDialog::OnLButtonUp(nFlags, point);
}

void CVideoPanelChildDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
 
  // TODO: Add your message handler code here and/or call default
  //((CVideoPanelDlg*) GetParent())->m_pChildMouseDown = this;
  ((CVideoPanelDlg*)( GetParent()->GetParent()))->Notify(1, this->m_nUserID);
  ///notify message to pop
 
  CDialog::OnLButtonDown(nFlags, point);
}

BOOL CVideoPanelChildDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
  // TODO: Add your message handler code here and/or call default
	// TODO: Add your message handler code here and/or call default
//***********************************************************************
	//���øı��ȡ���δ�Сʱ���
  
  if (((CVideoPanelDlg*) GetParent())->m_pChildMouseDown ){
	  //���ò�ɫ���
	  SetCursor(m_hCursor);
	  return TRUE;
  }else{
    SetCursor(m_hDefaultCursor);
    return TRUE;
  }

  return CDialog::OnSetCursor(pWnd, nHitTest, message);
}

BOOL CVideoPanelChildDlg::PreTranslateMessage(MSG* pMsg)
{
  // TODO: Add your specialized code here and/or call the base class
  if(pMsg->message==WM_KEYDOWN&&pMsg->wParam==VK_ESCAPE)     
  {
     OnMenuItem(ID_Menu_Layout_normal);
      return TRUE;
  }
  return CDialog::PreTranslateMessage(pMsg);
}

HBRUSH CVideoPanelChildDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
  HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	if(nCtlColor == CTLCOLOR_STATIC)
	{
		pDC->SetBkMode(TRANSPARENT);   
		pDC->SetTextColor(RGB(255,255,255)); 

		return   (HBRUSH)::GetStockObject(NULL_BRUSH);   
	}
  // TODO:  Change any attributes of the DC here

  // TODO:  Return a different brush if the default is not desired
  return hbr;
}

void CVideoPanelChildDlg::OnBnClickedOk()
{
  // TODO: Add your control notification handler code here
  //OnOK();
}
void CVideoPanelChildDlg::SetVideoWindowTitle(bool bForce, const char* txt)
{
  if (m_nUserID == 0){
    GetDlgItem( IDC_STATIC_VIDEO_TITLE)->SetWindowText("Local");
  }else
  {
    CString strTitle;strTitle.Format("%s(%d)",txt, m_nUserID );
    GetDlgItem( IDC_STATIC_VIDEO_TITLE)->SetWindowText(strTitle);
  }
  /*if (bForce ){
    GetDlgItem( IDC_STATIC_VIDEO_TITLE)->SetWindowText(txt);
  }else if (m_nUserID==0) {
    GetDlgItem( IDC_STATIC_VIDEO_TITLE)->SetWindowText(txt);
  }*/
}


BOOL CVideoPanelChildDlg::OnEraseBkgnd(CDC* pDC)
{
  // TODO: Add your message handler code here and/or call default
// TODO: Add your message handler code here and/or call default CRect rect;
	//m_wndVideo.();
#if 0
 
#else 
       CBrush   backBrush(RGB(0,   0,   0));   
      CBrush*   pOldBrush   =   pDC->SelectObject(&backBrush);   
      CRect   rect;   
      pDC->GetClipBox(&rect);             
      pDC->PatBlt(rect.left,   rect.top,   rect.Width(),     
      rect.Height(),   PATCOPY);   
      pDC->SelectObject(pOldBrush);   
			 return TRUE;
#endif
	 
	//return TRUE;
  return CDialog::OnEraseBkgnd(pDC);
}

void CVideoPanelChildDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
  // TODO: Add your message handler code here and/or call default
  m_bFullScreen = !m_bFullScreen;
  CVideoPanelDlg *p = (CVideoPanelDlg *)GetParent();
  if (p ){
    p->m_pChildMouseDown = NULL;
    p->SetChildWinSize(m_bFullScreen, this);
  }


 
  CDialog::OnLButtonDblClk(nFlags, point);
}
