// ToolTipWnd.cpp : implementation file
//

#include "stdafx.h"
#include ".\sfvideownd.h"

/////////////////////////////////////////////////////////////////////////////
// CSFVideoWnd

CSFVideoWnd::CSFVideoWnd()
{
  m_bEnableDragMouse= true;
  OSVERSIONINFO info;
  info.dwOSVersionInfoSize = sizeof(info);
  GetVersionEx(&info);
  m_nWinver = info.dwMajorVersion;
//#if WINVER >= D_WIN7_VER
  if (m_nWinver>= D_WIN7_VER){
	  m_hDibDraw = DrawDibOpen();
	  m_savedRect.SetRectEmpty();
	  memset( &m_destinfo, 0, sizeof( m_destinfo ) );
	  m_destinfo.biSize = sizeof( m_destinfo );
	  m_destinfo.biBitCount = 12;
	  m_destinfo.biCompression = MAKEFOURCC( 'I', '4', '2', '0' );
	  m_destinfo.biPlanes = 1;
	  m_destinfo.biWidth = 176;
	  m_destinfo.biHeight = 144;
	  m_destinfo.biSizeImage = m_destinfo.biWidth * m_destinfo.biHeight * m_destinfo.biBitCount / 8;
  }else{
//#else

	  int frameWidth = 176,frameHeight =144;
#ifndef D_LOCAL_PANCEL_CONTAINER
	  m_converter= PColourConverter::Create("YUV420P", "BGR24", frameWidth, frameHeight);
#endif
	  m_hDibDraw = DrawDibOpen();
	  m_savedRect.SetRectEmpty();
	  memset( &m_destinfo, 0, sizeof( m_destinfo ) );
	  m_destinfo.biSize = sizeof(m_destinfo);
	  m_destinfo.biWidth = frameWidth;
	  m_destinfo.biHeight = -(int)frameHeight;
	  m_destinfo.biPlanes = 1;
	  m_destinfo.biBitCount = 24;
	  m_destinfo.biCompression = BI_RGB;
	  m_destinfo.biXPelsPerMeter = 0;
	  m_destinfo.biYPelsPerMeter = 0;
	  m_destinfo.biClrImportant = 0;
	  m_destinfo.biClrUsed = 0;
	  m_destinfo.biSizeImage =  frameWidth*frameHeight*3;
  }
//#endif
}

CSFVideoWnd::~CSFVideoWnd()
{
//#if WINVER >= D_WIN7_VER
  /*if (m_nWinver>= D_WIN7_VER)*/{
	  if( m_hDibDraw ) 
		  DrawDibClose( m_hDibDraw );
  }
//#endif
	if( GetSafeHwnd() ){
		DestroyWindow();
	}
}



BEGIN_MESSAGE_MAP(CSFVideoWnd, CWnd)
	//{{AFX_MSG_MAP(CSFVideoWnd)
	//}}AFX_MSG_MAP
	ON_WM_CREATE()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_MBUTTONDBLCLK()
	ON_WM_LBUTTONDBLCLK()
  ON_WM_NCLBUTTONUP()
  ON_WM_KEYUP()
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSFVideoWnd message handlers

int CSFVideoWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;


	return 0;
}

BOOL CSFVideoWnd::DestroyWindow() 
{
	// TODO: Add your specialized code here and/or call the base class
	return CWnd::DestroyWindow();
}

BOOL CSFVideoWnd::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
	// TODO: Add your specialized code here and/or call the base class
	CString strMyClass;

	// load stock cursor, brush, and icon for
	// my own window class

	try
	{
	   strMyClass = AfxRegisterWndClass(
		  CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS
		  );
	}
	catch (CResourceException* pEx)
	{
		  AfxMessageBox(
			 _T("Couldn't register class! (Already registered?)"));
		  pEx->Delete();
		  return FALSE;
	}

	BOOL bRet =  CWnd::Create(strMyClass, "VideoW", dwStyle, rect, pParentWnd, nID, pContext);

	m_savedRect.SetRect(0, 0, rect.right-rect.left, rect.bottom-rect.top);
//#if WINVER >= D_WIN7_VER
  /*if (m_nWinver>= D_WIN7_VER)*/{
    {
	    HDC hdc = ::GetDC(GetSafeHwnd());
	    DrawDibEnd(m_hDibDraw);
	    DrawDibBegin (
		    m_hDibDraw,
		    hdc,
		    m_savedRect.Width(),
		    m_savedRect.Height(),
		    &m_destinfo,
		    m_destinfo.biWidth,
		    m_destinfo.biHeight,
		    DDF_SAME_DRAW | DDF_HALFTONE
		    ); 
	    DrawDibRealize( m_hDibDraw, hdc, FALSE);
	    ::ReleaseDC(GetSafeHwnd(),hdc);
    }
  }
//#endif
	return bRet;
}
//#if WINVER >= D_WIN7_VER
void CSFVideoWnd::Draw(const char* handle,const void* data, int nColorSpace,unsigned w, unsigned h)
{
 	  if (m_hWnd==NULL) {Sleep(10);return;} 
  if (m_nWinver>= D_WIN7_VER){

	  CDC* pDC = GetDC();
	   
	  CRect rc;
	  GetClientRect(rc);
	  if( m_destinfo.biWidth != w || m_destinfo.biHeight != h || m_savedRect.Height() != rc.Height() || m_savedRect.Width() != rc.Width() ) {
		  m_destinfo.biWidth = w;
		  m_destinfo.biHeight = h;
		  m_destinfo.biSizeImage = w * h * m_destinfo.biBitCount / 8;
		  m_savedRect = rc;
  		
		  if (!DrawDibEnd(m_hDibDraw))
		  {
			  int d = ::GetLastError();
			  d= 100;
		  }
		  if (! DrawDibBegin (
			  m_hDibDraw,
			  *pDC,
			  m_savedRect.Width(),
			  m_savedRect.Height(),
			  &m_destinfo,
			  m_destinfo.biWidth,
			  m_destinfo.biHeight,
			  DDF_SAME_DRAW | DDF_HALFTONE 
			  )
			  )
		  {
			  int d = ::GetLastError();
			  d= 100;

		  }
		  if (!
			  DrawDibRealize( m_hDibDraw, *pDC, FALSE)
			  ){
			  int d = ::GetLastError();
			  d = 100;
			  }
	  }
#if 0
	  BOOL b = ::DrawDibDraw(m_hDibDraw, *pDC, m_savedRect.left, m_savedRect.top, m_savedRect.Width(), m_savedRect.Height(), &m_destinfo, (LPVOID)data, 
		  0, 0, m_destinfo.biWidth, m_destinfo.biHeight, DDF_SAME_DRAW  | DDF_HALFTONE );// | DDF_SAME_HDC); 
#else
	  BOOL b = ::DrawDibDraw(m_hDibDraw, *pDC, m_savedRect.left, m_savedRect.top, m_savedRect.Width(), m_savedRect.Height(), &m_destinfo, (LPVOID)data, 
		  0, 0, m_destinfo.biWidth, m_destinfo.biHeight, DDF_SAME_DRAW  | DDF_HALFTONE   | DDF_SAME_HDC); 
#endif
	  if (!b) 
	  {
		  int d = ::GetLastError();
		  d= 1000;
	  }
	  ReleaseDC(pDC);
  }else
    DrawBGR24(handle, data, nColorSpace, w, h);
//#endif

}

//void CSFVideoWnd::DrawBGR24(const char* handle,const void* data, int nColorSpace,unsigned w, unsigned h)
//{
//	CRect rc;
//	GetClientRect(rc);
//	HDC hDC = GetDC()->m_hDC;
//	if( m_bitmap.bmiHeader.biWidth != w || -m_bitmap.bmiHeader.biHeight != h || m_savedRect.Height() != rc.Height() || m_savedRect.Width() != rc.Width() ) {
//		m_bitmap.bmiHeader.biSize = sizeof(m_bitmap.bmiHeader);
//		m_bitmap.bmiHeader.biWidth = w;
//		m_bitmap.bmiHeader.biHeight = -(int)h;
//		m_bitmap.bmiHeader.biPlanes = 1;
//		m_bitmap.bmiHeader.biBitCount = 24;
//		m_bitmap.bmiHeader.biCompression = BI_RGB;
//		m_bitmap.bmiHeader.biXPelsPerMeter = 0;
//		m_bitmap.bmiHeader.biYPelsPerMeter = 0;
//		m_bitmap.bmiHeader.biClrImportant = 0;
//		m_bitmap.bmiHeader.biClrUsed = 0;
//		m_bitmap.bmiHeader.biSizeImage =  w*h*3;
//		m_savedRect = rc;
//		m_converter->SetFrameSize(w, h);
//		m_tempPixelBuffer.SetSize(w*h*m_bitmap.bmiHeader.biBitCount / 8);
// 
//	}
//	PINDEX byteReturn =-1;
//	m_converter->Convert((const BYTE*)data , m_tempPixelBuffer.GetPointer(), &byteReturn);
//	int result =0;
//	if (m_savedRect.Width() == w&& m_savedRect.Height() == h)
//    result = SetDIBitsToDevice(hDC,
//	 m_savedRect.left, m_savedRect.top, m_savedRect.Width(), m_savedRect.Height(),
//	0, 0, 0, m_savedRect.Height(),
//                               m_tempPixelBuffer.GetPointer(), &m_bitmap, DIB_RGB_COLORS);
//  else
//    result = StretchDIBits(hDC,
//                            m_savedRect.left, m_savedRect.top, m_savedRect.Width(), m_savedRect.Height(),
//							0, 0, w,h,
//                           m_tempPixelBuffer.GetPointer(), &m_bitmap, DIB_RGB_COLORS, SRCCOPY);
// 
//  if (result == 0) {
//   int  lastError = ::GetLastError();
//   lastError=0;
//    //PTRACE(2, "VidOut\tDrawing image failed, error=" << lastError);
//  }
//}
//
//#else
void*  CSFVideoWnd::ConvertYUV420PToRGB24(const char* handle,const void* data, int nColorSpace,unsigned w, unsigned h)
{
#ifndef D_LOCAL_PANCEL_CONTAINER
	CRect rc;
	GetClientRect(rc);
  CDC* pDC =  GetDC();
  //if (pDC==NULL) return 0;
	HDC hDC = pDC->m_hDC;
	if( m_destinfo.biWidth != w || m_destinfo.biHeight != h || m_savedRect.Height() != rc.Height() || m_savedRect.Width() != rc.Width() ) {
		m_destinfo.biSize = sizeof(m_destinfo);
		m_destinfo.biWidth = w;
		m_destinfo.biHeight = (int)h;
		m_destinfo.biPlanes = 1;
		m_destinfo.biBitCount = 24;
		m_destinfo.biCompression = BI_RGB;
		m_destinfo.biXPelsPerMeter = 0;
		m_destinfo.biYPelsPerMeter = 0;
		m_destinfo.biClrImportant = 0;
		m_destinfo.biClrUsed = 0;
		m_destinfo.biSizeImage =  w*h*m_destinfo.biBitCount / 8;
		m_arrRGBline.SetSize(w*m_destinfo.biBitCount / 8);//for rgb a line
		m_savedRect = rc;
		m_converter->SetFrameSize(w, h);
		m_tempPixelBuffer.SetSize(w*h*m_destinfo.biBitCount / 8);
//
		DrawDibEnd(m_hDibDraw);
		  
		DrawDibBegin (
			  m_hDibDraw,
			  hDC,
			  m_savedRect.Width(),
			  m_savedRect.Height(),
			  &m_destinfo,
			  m_destinfo.biWidth,
			  m_destinfo.biHeight,
			  DDF_SAME_DRAW | DDF_HALFTONE 
			  );
		DrawDibRealize( m_hDibDraw, hDC, FALSE);
			   
 
	}
	PINDEX byteReturn =-1;
	//PBYTEArray& line = m_arrRGBline;
	int nWidthByte =w*m_destinfo.biBitCount / 8;
	//line.SetSize(nWidthByte);
	m_converter->Convert((const BYTE*)data , m_tempPixelBuffer.GetPointer(), &byteReturn);
	//正立RGB
	BYTE *p =m_tempPixelBuffer.GetPointer();
	BYTE *pHeader = NULL, *pButtom= NULL;
	for( int i=0;i< h/2;i++)
	{
		pHeader = p+ nWidthByte*i;pButtom = p+ (h -i-1)* nWidthByte;

		memcpy(m_arrRGBline.GetPointer(), pButtom, nWidthByte);
		//swap
		memcpy(pButtom, pHeader,   nWidthByte);
		memcpy( pHeader,  m_arrRGBline.GetPointer(), nWidthByte);
	}
  return  (void*)m_tempPixelBuffer.GetPointer();
#else
  return 0;
#endif
}
void CSFVideoWnd::DrawWithBGR24Data(const char* handle,const void* data, int nColorSpace,unsigned w, unsigned h)
{
 //#ifndef D_LOCAL_PANCEL_CONTAINER

	CRect rc;
	GetClientRect(rc);
  CDC* pDC =  GetDC();
  if (pDC==NULL) return ;
	HDC hDC = pDC->m_hDC;
	if( m_destinfo.biWidth != w || m_destinfo.biHeight != h || m_savedRect.Height() != rc.Height() || m_savedRect.Width() != rc.Width() ) {
		m_destinfo.biSize = sizeof(m_destinfo);
		m_destinfo.biWidth = w;
		m_destinfo.biHeight = (int)h;
		m_destinfo.biPlanes = 1;
		m_destinfo.biBitCount = 24;
		m_destinfo.biCompression = BI_RGB;
		m_destinfo.biXPelsPerMeter = 0;
		m_destinfo.biYPelsPerMeter = 0;
		m_destinfo.biClrImportant = 0;
		m_destinfo.biClrUsed = 0;
		m_destinfo.biSizeImage =  w*h*m_destinfo.biBitCount / 8;
//		m_arrRGBline.SetSize(w*m_destinfo.biBitCount / 8);//for rgb a line
		m_savedRect = rc;
		//m_converter->SetFrameSize(w, h);
		//m_tempPixelBuffer.SetSize(w*h*m_destinfo.biBitCount / 8);
//
		DrawDibEnd(m_hDibDraw);
		  
		DrawDibBegin (
			  m_hDibDraw,
			  hDC,
			  m_savedRect.Width(),
			  m_savedRect.Height(),
			  &m_destinfo,
			  m_destinfo.biWidth,
			  m_destinfo.biHeight,
			  DDF_SAME_DRAW | DDF_HALFTONE 
			  );
		DrawDibRealize( m_hDibDraw, hDC, FALSE);
			   
 
	}

   //
  ::DrawDibDraw(m_hDibDraw, hDC, m_savedRect.left, m_savedRect.top, m_savedRect.Width(), m_savedRect.Height(), &m_destinfo, (LPVOID) data , 
		  0, 0, m_destinfo.biWidth, h/*m_destinfo.biHeight*/, DDF_SAME_DRAW  | DDF_HALFTONE );// | DDF_SAME_HDC); 
     ReleaseDC(pDC);
//#endif
 
}

void CSFVideoWnd::DrawBGR24(const char* handle,const void* data, int nColorSpace,unsigned w, unsigned h)
{
#ifndef D_LOCAL_PANCEL_CONTAINER

	CRect rc;
	GetClientRect(rc);
  CDC* pDC =  GetDC();
  if (pDC==NULL) return ;
	HDC hDC = pDC->m_hDC;
	if( m_destinfo.biWidth != w || m_destinfo.biHeight != h || m_savedRect.Height() != rc.Height() || m_savedRect.Width() != rc.Width() ) {
		m_destinfo.biSize = sizeof(m_destinfo);
		m_destinfo.biWidth = w;
		m_destinfo.biHeight = (int)h;
		m_destinfo.biPlanes = 1;
		m_destinfo.biBitCount = 24;
		m_destinfo.biCompression = BI_RGB;
		m_destinfo.biXPelsPerMeter = 0;
		m_destinfo.biYPelsPerMeter = 0;
		m_destinfo.biClrImportant = 0;
		m_destinfo.biClrUsed = 0;
		m_destinfo.biSizeImage =  w*h*m_destinfo.biBitCount / 8;
		m_arrRGBline.SetSize(w*m_destinfo.biBitCount / 8);//for rgb a line
		m_savedRect = rc;
		m_converter->SetFrameSize(w, h);
		m_tempPixelBuffer.SetSize(w*h*m_destinfo.biBitCount / 8);
//
		DrawDibEnd(m_hDibDraw);
		  
		DrawDibBegin (
			  m_hDibDraw,
			  hDC,
			  m_savedRect.Width(),
			  m_savedRect.Height(),
			  &m_destinfo,
			  m_destinfo.biWidth,
			  m_destinfo.biHeight,
			  DDF_SAME_DRAW | DDF_HALFTONE 
			  );
		DrawDibRealize( m_hDibDraw, hDC, FALSE);
			   
 
	}
	PINDEX byteReturn =-1;
	//PBYTEArray& line = m_arrRGBline;
	int nWidthByte =w*m_destinfo.biBitCount / 8;
	//line.SetSize(nWidthByte);
	m_converter->Convert((const BYTE*)data , m_tempPixelBuffer.GetPointer(), &byteReturn);
	//正立RGB
	BYTE *p =m_tempPixelBuffer.GetPointer();
	BYTE *pHeader = NULL, *pButtom= NULL;
	for( int i=0;i< h/2;i++)
	{
		pHeader = p+ nWidthByte*i;pButtom = p+ (h -i-1)* nWidthByte;

		memcpy(m_arrRGBline.GetPointer(), pButtom, nWidthByte);
		//swap
		memcpy(pButtom, pHeader,   nWidthByte);
		memcpy( pHeader,  m_arrRGBline.GetPointer(), nWidthByte);
	}
#if 0
	::DrawDibDraw(m_hDibDraw, hDC, m_savedRect.left, m_savedRect.top, m_savedRect.Width(), m_savedRect.Height(), &m_destinfo, (LPVOID)m_tempPixelBuffer.GetPointer()/*data*/, 
		  0, 0, m_destinfo.biWidth, h/*m_destinfo.biHeight*/, DDF_SAME_DRAW  | DDF_HALFTONE );// | DDF_SAME_HDC); 
#else
  //
  ::DrawDibDraw(m_hDibDraw, hDC, m_savedRect.left, m_savedRect.top, m_savedRect.Width(), m_savedRect.Height(), &m_destinfo, (LPVOID)m_tempPixelBuffer.GetPointer()/*data*/, 
		  0, 0, m_destinfo.biWidth, h/*m_destinfo.biHeight*/, DDF_SAME_DRAW  | DDF_HALFTONE );// | DDF_SAME_HDC); 
#endif
    ReleaseDC(pDC);
#endif
}

//#endif
void CSFVideoWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
  //if (m_bEnableDragMouse)
  {
	  // TODO: Add your message handler code here and/or call default
	  //GetParent()->ClientToScreen(&point);
	  ClientToScreen(&point);
	  GetParent()->ClientToScreen(&point);

	  long l = point.y << 16| point.x;

	  ::SendMessage( GetParent()->GetSafeHwnd(), WM_LBUTTONUP, nFlags, l);
  }
//	CWnd::OnLButtonUp(nFlags, point);
}

void CSFVideoWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
  if (m_bEnableDragMouse)
  {
	  ::PostMessage(this->GetParent()->GetSafeHwnd(),WM_NCLBUTTONDOWN,HTCAPTION,MAKELPARAM(point.x,point.y)); 
  }
  ::PostMessage(this->GetParent()->GetSafeHwnd(),WM_LBUTTONDOWN,0,MAKELPARAM(point.x,point.y)); 
	CWnd::OnLButtonDown(nFlags, point);
}

void CSFVideoWnd::OnMButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	CWnd::OnMButtonDblClk(nFlags, point);
}

void CSFVideoWnd::OnLButtonDblClk(UINT nFlags, CPoint point)
{
  //if (m_bEnableDragMouse)
  {
	// TODO: Add your message handler code here and/or call default
	  ::SendMessage(this->GetParent()->GetSafeHwnd(), WM_VIDEOWND_DBCLICK,0,0);
  }

	CWnd::OnLButtonDblClk(nFlags, point);
}

void CSFVideoWnd::OnNcLButtonUp(UINT nHitTest, CPoint point)
{
  // TODO: Add your message handler code here and/or call default

  CWnd::OnNcLButtonUp(nHitTest, point);
}

void CSFVideoWnd::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
  // TODO: Add your message handler code here and/or call default
 
  CWnd::OnKeyUp(nChar, nRepCnt, nFlags);
}

BOOL CSFVideoWnd::PreTranslateMessage(MSG* pMsg)
{
  // TODO: Add your specialized code here and/or call the base class
  if(pMsg->message==WM_KEYUP&&pMsg->wParam==VK_ESCAPE)     
    return false;
  return CWnd::PreTranslateMessage(pMsg);
}
