// YYContainer.h : PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error 在包含用于 PCH 的此文件之前包含“stdafx.h”
#endif

//#define D_PRINT(x) {printf(x);fflush(stream); }
//extern FILE *stream  ;

#include "resource.h"		// 主符号
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
// 有关此类的实现，请参阅 YYContainer.cpp
//

class CYYContainerApp : public CWinApp
{
public:
	CYYContainerApp();

// 重写
	public:
	virtual BOOL InitInstance();

// 实现

	DECLARE_MESSAGE_MAP()
public:
    
};

extern CYYContainerApp theApp;
