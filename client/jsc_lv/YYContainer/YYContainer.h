// YYContainer.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error �ڰ������� PCH �Ĵ��ļ�֮ǰ������stdafx.h��
#endif

//#define D_PRINT(x) {printf(x);fflush(stream); }
//extern FILE *stream  ;

#include "resource.h"		// ������
class CLocalVideoDlg;

struct dk_window 
	{
		CLocalVideoDlg*	pWnd;
		BOOL		bDocked;
		DWORD		nEdge;
		DWORD		dwMagType;
		int			delta;
	};
// CYYContainerApp:
// �йش����ʵ�֣������ YYContainer.cpp
//

class CYYContainerApp : public CWinApp
{
public:
	CYYContainerApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
public:
    
};

extern CYYContainerApp theApp;
