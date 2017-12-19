#pragma once

// YYSpriteX-2PropPage.h : Declaration of the CYYSpriteX2PropPage property page class.


// CYYSpriteX2PropPage : See YYSpriteX-2PropPage.cpp for implementation.

class CYYSpriteX2PropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CYYSpriteX2PropPage)
	DECLARE_OLECREATE_EX(CYYSpriteX2PropPage)

// Constructor
public:
	CYYSpriteX2PropPage();

// Dialog Data
	enum { IDD = IDD_PROPPAGE_YYSPRITEX2 };

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	DECLARE_MESSAGE_MAP()
};

