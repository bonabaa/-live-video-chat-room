#pragma once

// YYSpriteX-2.h : main header file for YYSpriteX-2.DLL

#if !defined( __AFXCTL_H__ )
#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols


// CYYSpriteX2App : See YYSpriteX-2.cpp for implementation.

class CYYSpriteX2App : public COleControlModule
{
public:
	BOOL InitInstance();
	int ExitInstance();
};

extern const GUID CDECL _tlid;
extern const WORD _wVerMajor;
extern const WORD _wVerMinor;

