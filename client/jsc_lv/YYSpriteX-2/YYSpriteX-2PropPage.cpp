// YYSpriteX-2PropPage.cpp : Implementation of the CYYSpriteX2PropPage property page class.

#include "stdafx.h"
#include "YYSpriteX-2.h"
#include "YYSpriteX-2PropPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE(CYYSpriteX2PropPage, COlePropertyPage)



// Message map

BEGIN_MESSAGE_MAP(CYYSpriteX2PropPage, COlePropertyPage)
END_MESSAGE_MAP()



// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CYYSpriteX2PropPage, "YYSPRITEX2.YYSpriteX2PropPage.1",
	0xdf78588f, 0x6096, 0x4dfb, 0x8d, 0xea, 0x57, 0x21, 0x1, 0x23, 0x3e, 0xc)



// CYYSpriteX2PropPage::CYYSpriteX2PropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CYYSpriteX2PropPage

BOOL CYYSpriteX2PropPage::CYYSpriteX2PropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_YYSPRITEX2_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}



// CYYSpriteX2PropPage::CYYSpriteX2PropPage - Constructor

CYYSpriteX2PropPage::CYYSpriteX2PropPage() :
	COlePropertyPage(IDD, IDS_YYSPRITEX2_PPG_CAPTION)
{
}



// CYYSpriteX2PropPage::DoDataExchange - Moves data between page and properties

void CYYSpriteX2PropPage::DoDataExchange(CDataExchange* pDX)
{
	DDP_PostProcessing(pDX);
}



// CYYSpriteX2PropPage message handlers
