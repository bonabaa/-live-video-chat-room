// CatchScreenDlg.cpp : implementation file
// Download by http://www.codefans.net

#include "stdafx.h"
#include "CatchScreen.h"
#include "CatchScreenDlg.h"
#include "remotedlg.h"
#include "lpncoremgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
extern int SaveCaptureTojpegFile(const char* filename,HDC hDCmem,int width, int height, int quality);
/////////////////////////////////////////////////////////////////////////////
// CCatchScreenDlg dialog
CCatchScreenDlg::CCatchScreenDlg(LPNMgr* pMgr, CWnd* pParent /*=NULL*/)
	: CDialog(CCatchScreenDlg::IDD, pParent), m_pMgr(pMgr), m_nScreenCapture(0), m_bDoneForCaptureScreen(false)
{
//*******************************************************************************
    //��ʼ����Ƥ����,������resizeMiddle ����
	m_rectTracker.m_nStyle=CMyTracker::resizeMiddle|CMyTracker::solidLine;  
	m_rectTracker.m_rect.SetRect(-1,-2,-3,-4);
	//���þ�����ɫ
	m_rectTracker.SetRectColor(RGB(10,100,130));
	//���þ��ε���ʱ���,Ĭ�ϵ�̫С��,Ū�˸�����
	m_rectTracker.SetResizeCursor(IDC_CURSOR6,IDC_CURSOR5,IDC_CURSOR2,IDC_CURSOR3,IDC_CURSOR4);

    m_hCursor=AfxGetApp()->LoadCursor(IDC_CURSOR1);  
    
	
	m_bDraw=FALSE;
	m_bFirstDraw=FALSE;
	m_bQuit=FALSE;
	m_bShowMsg=FALSE;
    m_startPt=0;
    
	//��ȡ��Ļ�ֱ���
	m_xScreen = GetSystemMetrics(SM_CXSCREEN);
	m_yScreen = GetSystemMetrics(SM_CYSCREEN);

	//��ȡ��Ļ��λͼ��
	CRect rect(0, 0,m_xScreen,m_yScreen);
	m_pBitmap=CBitmap::FromHandle(CopyScreenToBitmap(&rect));
    
	//��ʼ��ˢ�´������� m_rgn
    m_rgn.CreateRectRgn(0,0,50,50);
//*******************************************************************************

//	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}
CCatchScreenDlg::CCatchScreenDlg(int  pRemote, CWnd* pParent /*=NULL*/)
	: CDialog(CCatchScreenDlg::IDD, pParent), m_nScreenCapture(pRemote),m_pMgr(0), m_bDoneForCaptureScreen(false)
{
	//{{AFX_DATA_INIT(CCatchScreenDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32

//*******************************************************************************
    //��ʼ����Ƥ����,������resizeMiddle ����
	m_rectTracker.m_nStyle=CMyTracker::resizeMiddle|CMyTracker::solidLine;  
	m_rectTracker.m_rect.SetRect(-1,-2,-3,-4);
	//���þ�����ɫ
	m_rectTracker.SetRectColor(RGB(10,100,130));
	//���þ��ε���ʱ���,Ĭ�ϵ�̫С��,Ū�˸�����
	m_rectTracker.SetResizeCursor(IDC_CURSOR6,IDC_CURSOR5,IDC_CURSOR2,IDC_CURSOR3,IDC_CURSOR4);

    m_hCursor=AfxGetApp()->LoadCursor(IDC_CURSOR1);  
    
	
	m_bDraw=FALSE;
	m_bFirstDraw=FALSE;
	m_bQuit=FALSE;
	m_bShowMsg=FALSE;
    m_startPt=0;
    
	//��ȡ��Ļ�ֱ���
	m_xScreen = GetSystemMetrics(SM_CXSCREEN);
	m_yScreen = GetSystemMetrics(SM_CYSCREEN);

	//��ȡ��Ļ��λͼ��
	CRect rect(0, 0,m_xScreen,m_yScreen);
	m_pBitmap=CBitmap::FromHandle(CopyScreenToBitmap(&rect));
    
	//��ʼ��ˢ�´������� m_rgn
    m_rgn.CreateRectRgn(0,0,50,50);
//*******************************************************************************

//	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCatchScreenDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCatchScreenDlg)
	DDX_Control(pDX, IDC_EDIT1, m_tipEdit);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CCatchScreenDlg, CDialog)
	//{{AFX_MSG_MAP(CCatchScreenDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_ERASEBKGND()
	ON_WM_SETCURSOR()
	ON_WM_RBUTTONUP()
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCatchScreenDlg message handlers

BOOL CCatchScreenDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
//**************************************************************************
	//�ѶԻ������ó�ȫ�����㴰��
 	SetWindowPos(&wndTopMost,0,0,m_xScreen,m_yScreen,SWP_SHOWWINDOW);
 	//�ƶ�������ʾ����
	CRect rect;
	m_tipEdit.GetWindowRect(&rect);
    m_tipEdit.MoveWindow(10,10,rect.Width(),rect.Height());

	//��ʾ������ʾ��������
	DrawTip();
	
	//���񰴼���Ϣ����,���Ի���ľ�����ݵ�CCatchScreenApp��
	//((CCatchScreenApp *)AfxGetApp())->m_hwndDlg=m_hWnd;
//**************************************************************************

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	//KillTimer(1001);
 // this->ShowWindow(SW_HIDE);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CCatchScreenDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
//**************************************************************************
		CPaintDC dc(this);
         
		//��ʾ��ȡ���δ�С��Ϣ
		if(m_bShowMsg&&m_bFirstDraw)
		{
			//�õ���ǰ���δ�С
			CRect rect;
			m_rectTracker.GetTrueRect(&rect);
			//����CPaintDC ��Ϊ�˲����˴����ϻ���Ϣ
     // GetDlgItem(IDC_EDIT1)->Invalidate();
			DrawMessage(rect,&dc);
		}

		//������Ƥ�����
		if(m_bFirstDraw)
		{
			m_rectTracker.Draw(&dc);
		}

//*************************************************************************
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CCatchScreenDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CCatchScreenDlg::OnOK() 
{
	// TODO: Add extra validation here
	
	//CDialog::OnOK();
}

void CCatchScreenDlg::OnCancel() 
{
	// TODO: Add extra cleanup here
//***************************************************************
	if(m_bFirstDraw)
	{
		//ȡ���ѻ����α���
		m_bFirstDraw=FALSE;
		m_bDraw=FALSE;
		m_rectTracker.m_rect.SetRect(-1,-1,-1,-1);
		PaintWindow();
	}
	else
	{
		CDialog::OnCancel();
	}
//*******************************************************************
}

void CCatchScreenDlg::OnMouseMove(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
//**************************************************************************************
	   if(m_bDraw)
	   {
			//��̬�������δ�С,��ˢ�»���
		    m_rectTracker.m_rect.SetRect(m_startPt.x+4,m_startPt.y+4,point.x,point.y);
			PaintWindow();
	   }
	   
	   //�ֲ�������С��λ��ʱ,���ղ���MouseMove��Ϣ
	   CRect rect;
	   m_tipEdit.GetWindowRect(&rect);
	   if(rect.PtInRect(point))
	   m_tipEdit.SendMessage(WM_MOUSEMOVE);
       
	   ChangeRGB();
//*****************************************************************************************
	CDialog::OnMouseMove(nFlags, point);
}

void CCatchScreenDlg::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
//*****************************************************************************************
	int nHitTest;
	nHitTest=m_rectTracker.HitTest(point);

    //�жϻ���λ��
	if(nHitTest<0)
	{
		if(!m_bFirstDraw)
		{
			//��һ�λ�����
			m_startPt=point;
			m_bDraw=TRUE;
			m_bFirstDraw=TRUE;
			//���õ�����갴��ʱ��С�ľ��δ�С
			m_rectTracker.m_rect.SetRect(point.x,point.y,point.x+4,point.y+4);	
			
			//��֤����굱��ʱ������ʾ��Ϣ
			if(m_bFirstDraw)
			  m_bShowMsg=TRUE;		
			DrawTip();
			PaintWindow();
		}
	}
	else
	{
		//��֤����굱��ʱ������ʾ��Ϣ
		m_bShowMsg=TRUE;		
		PaintWindow();
		
		if(m_bFirstDraw)
		{
			//������Сʱ,Track���Զ��������δ�С,��Щ�ڼ�,��Ϣ��CRectTracker�ڲ�����
			m_rectTracker.Track(this,point,TRUE);
			//SendMessage(WM_LBUTTONUP,NULL,NULL);
			PaintWindow();

		}
	}
//****************************************************************************************
	CDialog::OnLButtonDown(nFlags, point);
}

void CCatchScreenDlg::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
//****************************************************************************************
    
	m_bShowMsg=FALSE;
	m_bDraw=FALSE;
	DrawTip();

	PaintWindow();
//****************************************************************************************
	CDialog::OnLButtonUp(nFlags, point);
}

void CCatchScreenDlg::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
 	// TODO: Add your message handler code here and/or call default
	int nHitTest;
	nHitTest=m_rectTracker.HitTest(point);
    //������Ǿ����ڲ�˫��
	if(nHitTest==8)      
	{
        //����λͼ��ճ������,bSave ΪTRUE,
    if ( m_pMgr!=NULL ){
      CopyScreenToBitmapEx(m_rectTracker.m_rect,TRUE);
    }else /*if ( m_nScreenCapture)*/{
      //::PostMessage(
	    //  PostQuitMessage(0);
      SIZE nfixedSZ;

      CRect rGot= m_rectTracker.m_rect;
      //if (rGot.Width() % 16 !=0 )
        nfixedSZ.cx = rGot.Width() - rGot.Width() % 16;
      //else
      //   nfixedSZ.cx = rGot.Width();
      //
      //if (rGot.Height() % 16 !=0 )
        nfixedSZ.cy = rGot.Height() - rGot.Height() % 16;
 
       
      m_RectScreenCapture = CRect(rGot.TopLeft(), nfixedSZ);
      //m_nScreenCapture->SetCaptureFrameSize(r);
    }
    CDialog::OnCancel();
	}
	CDialog::OnLButtonDblClk(nFlags, point);
}

void CCatchScreenDlg::OnRButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	//****************************************************************************************
	if(m_bFirstDraw)
	{
		//����Ѿ���ȡ���������ȡ����
		m_bFirstDraw=FALSE;
		//������δ�С
		m_rectTracker.m_rect.SetRect(-1,-1,-1,-1);
		DrawTip();
		PaintWindow();
	}
	else
	{
		 //�رճ���
		 //PostQuitMessage(0);
     CDialog::OnCancel();
	}
//****************************************************************************************
	CDialog::OnRButtonUp(nFlags, point);
}

HBRUSH CCatchScreenDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	
	// TODO: Change any attributes of the DC here
	// TODO: Return a different brush if the default is not desired
//***********************************************************
	//���ò�����ʾ�����ı���ɫ
	if(pWnd->GetDlgCtrlID()==IDC_EDIT1)
	{
		pDC->SetTextColor(RGB(255,255,255));
	}
//***************************************************************
	// TODO: Return a different brush if the default is not desired
	return hbr;
}

BOOL CCatchScreenDlg::OnEraseBkgnd(CDC* pDC) 
{
	// TODO: Add your message handler code here and/or call default
//**************************************************************************************
	//�������������ȫ���Ի��򱳾�
	BITMAP bmp;
	m_pBitmap->GetBitmap(&bmp);

	CDC dcCompatible;
	dcCompatible.CreateCompatibleDC(pDC);

	dcCompatible.SelectObject(m_pBitmap);

	CRect rect;
	GetClientRect(&rect);
	pDC->BitBlt(0,0,rect.Width(),rect.Height(),&dcCompatible,0,0,SRCCOPY);

	return TRUE;
//**************************************************************************************
	//return CDialog::OnEraseBkgnd(pDC);
}

BOOL CCatchScreenDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	// TODO: Add your message handler code here and/or call default
//***********************************************************************
	//���øı��ȡ���δ�Сʱ���
	if (pWnd == this &&m_rectTracker.SetCursor(this, nHitTest)
		             &&!m_bDraw &&m_bFirstDraw) //�˴��жϱ���ȡʱ����ʼ��Ϊ��ɫ���
    {
		return TRUE; 
	}

	//���ò�ɫ���
	SetCursor(m_hCursor);
	return TRUE;

//*******************************************************************
	//return CDialog::OnSetCursor(pWnd, nHitTest, message);
}


//*********************���ӵĺ���**********************************************************
//������Ļ,��δ������������� �Ǻ�
HBITMAP CCatchScreenDlg::CopyScreenToBitmap(LPRECT lpRect,BOOL bSave)
//lpRect ����ѡ������
{
	HDC       hScrDC, hMemDC;      
	// ��Ļ���ڴ��豸������
	HBITMAP    hBitmap, hOldBitmap;   
	// λͼ���
	int       nX, nY, nX2, nY2;      
	// ѡ����������
	int       nWidth, nHeight;
	
	// ȷ��ѡ������Ϊ�վ���
	if (IsRectEmpty(lpRect))
		return NULL;
	//Ϊ��Ļ�����豸������
	hScrDC = CreateDC("DISPLAY", NULL, NULL, NULL);

	//Ϊ��Ļ�豸�����������ݵ��ڴ��豸������
	hMemDC = CreateCompatibleDC(hScrDC);
	// ���ѡ����������
	nX = lpRect->left;
	nY = lpRect->top;
	nX2 = lpRect->right;
	nY2 = lpRect->bottom;

	//ȷ��ѡ�������ǿɼ���
	if (nX < 0)
		nX = 0;
	if (nY < 0)
		nY = 0;
	if (nX2 > m_xScreen)
		nX2 = m_xScreen;
	if (nY2 > m_yScreen)
		nY2 = m_yScreen;
	nWidth = nX2 - nX;
	nHeight = nY2 - nY;
	// ����һ������Ļ�豸��������ݵ�λͼ
	hBitmap = CreateCompatibleBitmap
		(hScrDC, nWidth, nHeight);
	// ����λͼѡ���ڴ��豸��������
	hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
	// ����Ļ�豸�����������ڴ��豸��������
	if(bSave)
	{
		//����������DC,��bSaveΪ��ʱ�ѿ�ʼ�����ȫ��λͼ,����ȡ���δ�С����
		CDC dcCompatible;
		dcCompatible.CreateCompatibleDC(CDC::FromHandle(hMemDC));
		dcCompatible.SelectObject(m_pBitmap);
        
		BitBlt(hMemDC, 0, 0, nWidth, nHeight,
			dcCompatible, nX, nY, SRCCOPY);

	}
	else
	{
		BitBlt(hMemDC, 0, 0, nWidth, nHeight,
			hScrDC, nX, nY, SRCCOPY);
	}

	hBitmap = (HBITMAP)SelectObject(hMemDC, hOldBitmap);
	//�õ���Ļλͼ�ľ��
	//��� 
	DeleteDC(hScrDC);
	DeleteDC(hMemDC);
	
	if(bSave)
	{
				
		if (OpenClipboard()) 
		{
        //��ռ�����
        EmptyClipboard();
        //����Ļ����ճ������������,
        //hBitmap Ϊ�ղŵ���Ļλͼ���
        SetClipboardData(CF_BITMAP, hBitmap);
        
        //�رռ�����
        CloseClipboard();
      }
	}
	// ����λͼ���

	return hBitmap;
}

HBITMAP CCatchScreenDlg::CopyScreenToBitmapEx(LPRECT lpRect,BOOL bSave)
//lpRect ����ѡ������
{
	HDC       hScrDC, hMemDC;      
	// ��Ļ���ڴ��豸������
	HBITMAP    hBitmap, hOldBitmap;   
	// λͼ���
	int       nX, nY, nX2, nY2;      
	// ѡ����������
	int       nWidth, nHeight;
	
	// ȷ��ѡ������Ϊ�վ���
	if (IsRectEmpty(lpRect))
		return NULL;
	//Ϊ��Ļ�����豸������
	hScrDC = CreateDC("DISPLAY", NULL, NULL, NULL);

	//Ϊ��Ļ�豸�����������ݵ��ڴ��豸������
	hMemDC = CreateCompatibleDC(hScrDC);
	// ���ѡ����������
	nX = lpRect->left;
	nY = lpRect->top;
	nX2 = lpRect->right;
	nY2 = lpRect->bottom;

	//ȷ��ѡ�������ǿɼ���
	if (nX < 0)
		nX = 0;
	if (nY < 0)
		nY = 0;
	if (nX2 > m_xScreen)
		nX2 = m_xScreen;
	if (nY2 > m_yScreen)
		nY2 = m_yScreen;
	nWidth = nX2 - nX;
	nHeight = nY2 - nY;
	// ����һ������Ļ�豸��������ݵ�λͼ
	hBitmap = CreateCompatibleBitmap
		(hScrDC, nWidth, nHeight);
	// ����λͼѡ���ڴ��豸��������
	hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
	// ����Ļ�豸�����������ڴ��豸��������
	if(bSave)
	{
		//����������DC,��bSaveΪ��ʱ�ѿ�ʼ�����ȫ��λͼ,����ȡ���δ�С����
		CDC dcCompatible;
		dcCompatible.CreateCompatibleDC(CDC::FromHandle(hMemDC));
		dcCompatible.SelectObject(m_pBitmap);
        
		BitBlt(hMemDC, 0, 0, nWidth, nHeight,
			dcCompatible, nX, nY, SRCCOPY);

	}
	else
	{
		BitBlt(hMemDC, 0, 0, nWidth, nHeight,
			hScrDC, nX, nY, SRCCOPY);
	}

	//hBitmap = (HBITMAP)SelectObject(hScrDC, hOldBitmap);
  SaveCaptureTojpegFile( m_strFile /*"c:\\1.jpg"*/, hMemDC,  nWidth, nHeight, 70);
  m_bDoneForCaptureScreen = true;

	
	if(bSave)
	{
				
		if (OpenClipboard()) 
		{
        //��ռ�����
        EmptyClipboard();
        //����Ļ����ճ������������,
        //hBitmap Ϊ�ղŵ���Ļλͼ���
        SetClipboardData(CF_BITMAP, hBitmap);
        //�رռ�����
        CloseClipboard();
         
    }
//    if ( OpenClipboard() )
//    {
//	    EmptyClipboard();
//	    //create some data
//	   // CBitmap * junk = new CBitmap();
//      CBitmap*   junk   =   CBitmap::FromHandle(hBitmap); 
//      if (junk== NULL) MessageBox("null");
//	    CClientDC cdc(this);
//	    CDC dc;
//	    dc.CreateCompatibleDC(&cdc);
//	    CRect client(0,0, nWidth, nHeight);
//	    junk->CreateCompatibleBitmap(&cdc,client.Width(),client.Height());
//	    dc.SelectObject(junk);
//
//	    //call draw routine here that makes GDI calls
////	    DrawImage(&dc,CString("Bitmap"));
//
//	    //put the data on the clipboard
//	    SetClipboardData(CF_BITMAP,junk->m_hObject);
//	    CloseClipboard();
//
//	    //copy has been made on clipboard so we can delete
//	    //delete junk;
//    }

	}
	// ����λͼ���
	//�õ���Ļλͼ�ľ��
	//��� 
  hBitmap = (HBITMAP)SelectObject(hScrDC, hOldBitmap);
	DeleteDC(hScrDC);
	DeleteDC(hMemDC);
  return hBitmap;
}

//��ʾ������ʾ��Ϣ
void CCatchScreenDlg::DrawTip()
{
    
    //�õ�ǰ��������,
	CPoint pt;
	GetCursorPos(&pt);
	
	//������ǰR,G,B,������ֵ
	COLORREF color;
	CClientDC dc(this);
	color=dc.GetPixel(pt);
	BYTE rValue,gValue,bValue;
	rValue=GetRValue(color);
	gValue=GetGValue(color);
	bValue=GetGValue(color);
	
	//����ʽ�ŷ��ַ���
	CString string;
	CString strTemp;
	string.Format("\r\n\r\n\r\n ��RGB (%d,%d,%d)\r\n",rValue,gValue,bValue);
    
	if(!m_bDraw&&!m_bFirstDraw)
	{
		strTemp="\r\n ����������������ѡ���ȡ\r\n ��Χ\r\n\r\n ����ESC��������Ҽ��˳�";
	}
	else
	if(m_bDraw&&m_bFirstDraw)
	{
		strTemp="\r\n ���ɿ�������ȷ����ȡ��Χ\r\n\r\n ����ESC���˳�";
	}
	else
	if(m_bFirstDraw)
	{
		strTemp="\r\n ����������������ȡ��Χ��\r\n ��С��λ��\r\n\r\n ����ȡ��Χ��˫����������\r\n ��ͼ�񣬽�������\r\n\r\n ���������Ҽ�����ѡ��";
	}
	string+=strTemp;
	//��ʾ���༩����,������ʾ����
	m_tipEdit.SetWindowText(string);
}

//��ʾ��ȡ������Ϣ
void CCatchScreenDlg::DrawMessage(CRect &inRect,CDC * pDC)
{
	//��ȡ���δ�С��Ϣ�������
	const int space=3;
    
	//����������ɫ��С
	
	CPoint pt;
	CPen pen(PS_SOLID,1,RGB(147,147,147));

	//dc.SetTextColor(RGB(147,147,147));
	CFont font;
	CFont * pOldFont;
	font.CreatePointFont(90,"����");
	pOldFont=pDC->SelectObject(&font);

	//�õ������Ⱥ͸߶�
	GetCursorPos(&pt);
	int OldBkMode;
	OldBkMode=pDC->SetBkMode(TRANSPARENT);

	TEXTMETRIC tm;
	int charHeight;
	CSize size;
	int	lineLength;
	pDC->GetTextMetrics(&tm);
	charHeight = tm.tmHeight+tm.tmExternalLeading;
	size=pDC->GetTextExtent("Top Position  ",strlen("Top Position  "));
	lineLength=size.cx;
    
	//��ʼ������, �Ա�֤д����������
	CRect rect(pt.x+space,pt.y-charHeight*6-space,pt.x+lineLength+space,pt.y-space);
    
    //������ʱ����
    CRect rectTemp;
	//�����ε��������Եʱ��������ʹ�С
	if((pt.x+rect.Width())>=m_xScreen)
	{
		//�����Ϸ���ʾ���¾���
		rectTemp=rect;
		rectTemp.left=rect.left-rect.Width()-space*2;
		rectTemp.right=rect.right-rect.Width()-space*2;;
		rect=rectTemp;
	}

	if((pt.y-rect.Height())<=0)
	{
		//�����ҷ���ʾ���¾���
		rectTemp=rect;
		rectTemp.top=rect.top+rect.Height()+space*2;;
		rectTemp.bottom=rect.bottom+rect.Height()+space*2;;
		rect=rectTemp;
		
	}

	//�����ջ�ˢ������
	CBrush * pOldBrush;
    pOldBrush=pDC->SelectObject(CBrush::FromHandle((HBRUSH)GetStockObject(NULL_BRUSH)));
	
	pDC->Rectangle(rect);
   	rect.top+=2;
	//�ھ�������ʾ����
	CRect outRect(rect.left,rect.top,rect.left+lineLength,rect.top+charHeight);
	CString string("Top Position");
	pDC->DrawText(string,outRect,DT_CENTER);
    
	outRect.SetRect(rect.left,rect.top+charHeight,rect.left+lineLength,charHeight+rect.top+charHeight);
	string.Format("(%d,%d)",inRect.left,inRect.top);
	pDC->DrawText(string,outRect,DT_CENTER);
	

	outRect.SetRect(rect.left,rect.top+charHeight*2,rect.left+lineLength,charHeight+rect.top+charHeight*2);
	string="Rectangle size";
	pDC->DrawText(string,outRect,DT_CENTER);

	outRect.SetRect(rect.left,rect.top+charHeight*3,rect.left+lineLength,charHeight+rect.top+charHeight*3);
	string.Format("(%d,%d)",inRect.Width(),inRect.Height());
    pDC->DrawText(string,outRect,DT_CENTER);

	outRect.SetRect(rect.left,rect.top+charHeight*4,rect.left+lineLength,charHeight+rect.top+charHeight*4);
	string="Cursor coordinates";
    pDC->DrawText(string,outRect,DT_CENTER);

	outRect.SetRect(rect.left,rect.top+charHeight*5,rect.left+lineLength,charHeight+rect.top+charHeight*5);
	string.Format("(%d,%d)",pt.x,pt.y);
	pDC->DrawText(string,outRect,DT_CENTER);
    
	pDC->SetBkMode(OldBkMode);
	pDC->SelectObject(pOldFont);
	pDC->SelectObject(pOldBrush);
	
}
//�ػ�����
void CCatchScreenDlg::PaintWindow()
{
	//��ȡ��ȫ���Ի��򴰿ڴ�С
	CRect rect1;
	GetWindowRect(rect1);

	//��ȡ�༭�򴰿ڴ�С
	CRect rect2;
	m_tipEdit.GetWindowRect(rect2);

	CRgn rgn1,rgn2;
	rgn1.CreateRectRgnIndirect(rect1);
	rgn2.CreateRectRgnIndirect(rect2);

	//��ȡ��������,���ǳ��˱༭�򴰿ڲ�����
	m_rgn.CombineRgn(&rgn1,&rgn2,RGN_DIFF);
	
	InvalidateRgn(&m_rgn);
}
//�ı������ʾ���ڵ�RGBֵ
void CCatchScreenDlg::ChangeRGB()
{
	//����ɵ�RGBֵ�ַ���
	static CString strOld("");

	CPoint pt;
	GetCursorPos(&pt);

	//������ǰR,G,B,������ֵ
	COLORREF color;
	CClientDC dc(this);
	color=dc.GetPixel(pt);
	BYTE rValue,gValue,bValue;
	rValue=GetRValue(color);
	gValue=GetGValue(color);
	bValue=GetGValue(color);
	
	//����ʽ�ŷ��ַ���
	CString string;
	string.Format("(%d,%d,%d)",rValue,gValue,bValue);
	//�����ǰ��ɫû����ˢ��RGBֵ,���ⴰ���и�����˸
    if(strOld!=string)
	{
	    //�õ�RGB�ı���һ�е��ı�����
		int LineLength=m_tipEdit.LineLength(6);
		//��ѡRGBֵ�ı�,Ҳ����(255,255,255)��ʽ
	    m_tipEdit.SetSel(20,LineLength+6);
        
		//�滻RGB����
		m_tipEdit.ReplaceSel(string);
    }
	
	strOld=string;

}

//*******************************************************************************************

