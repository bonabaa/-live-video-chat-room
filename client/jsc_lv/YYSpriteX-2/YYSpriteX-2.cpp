// YYSpriteX-2.cpp : Implementation of CYYSpriteX2App and DLL registration.

#include "stdafx.h"
#include "YYSpriteX-2.h"
#define D_SAFE_ACTIVEX

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//#pragma link warning(disable:4099)
CYYSpriteX2App NEAR theApp;
#ifdef D_SAFE_ACTIVEX
#include "comcat.h"
#include "strsafe.h"
#include "objsafe.h"
class CYYSpriteX2Ctrl;
//extern CYYSpriteX2Ctrl* g_pSpriteX2Ctrl;

////const GUID CDECL BASED_CODE _tlid =
////		{ 0x68B0DA90, 0xA01B, 0x41E2, { 0x84, 0xDE, 0x81, 0x71, 0xB5, 0x68, 0xCE, 0x15 } };
const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;
//const CATID CATID_SafeForScripting     =
//      {0x7dd95801,0x9882,0x11cf,{0x9f,0xa9,0x00,0xaa,0x00,0x6c,0x42,0xc4}};
//      const CATID CATID_SafeForInitializing  =
//      {0x7dd95802,0x9882,0x11cf,{0x9f,0xa9,0x00,0xaa,0x00,0x6c,0x42,0xc4}};

const CATID CLSID_SafeItem =
		{ 0x979C0907, 0xD1B5, 0x4830, { 0x87, 0x3, 0xC6, 0x5D, 0x8D, 0x75, 0xF4, 0x4D } };
//{0x74e59815, 0x8639, 0x4e51, {0x99, 0xa1, 0x47, 0xf1, 0xa5, 0x71, 0x60, 0xd9}};
//{ 0x68B0DA90, 0xA01B, 0x41E2, { 0x84, 0xDE, 0x81, 0x71, 0xB5, 0x68, 0xCE, 0x15 } };
//
//const GUID CDECL BASED_CODE _tlid =
//		{  0x74e59815, 0x8639, 0x4e51, { 0x99, 0xa1, 0x47, 0xf1, 0xa5, 0x71, 0x60, 0xd9 } };
const GUID CDECL BASED_CODE _tlid =
		{ 0x979C0907, 0xD1B5, 0x4830, { 0x87, 0x3, 0xC6, 0x5D, 0x8D, 0x75, 0xF4, 0x4D } };

const GUID CDECL BASED_CODE _ctlid =
		{  0x74e59815, 0x8639, 0x4e51, { 0x99, 0xa1, 0x47, 0xf1, 0xa5, 0x71, 0x60, 0xd9 } };//from IMPLEMENT_OLECREATE_EX

//IMPLEMENT_OLECREATE_EX(CYYSpriteX2Ctrl, "YYSPRITEX2.YYSpriteX2Ctrl.1",
//	0x74e59815, 0x8639, 0x4e51, 0x99, 0xa1, 0x47, 0xf1, 0xa5, 0x71, 0x60, 0xd9)

//const WORD _wVerMajor = 1;
//const WORD _wVerMinor = 0;


#else//not D_SAFE_ACTIVEX
const GUID CDECL BASED_CODE _tlid =
		{ 0x979C0907, 0xD1B5, 0x4830, { 0x87, 0x3, 0xC6, 0x5D, 0x8D, 0x75, 0xF4, 0x4D } };
const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;

#endif

// CYYSpriteX2App::InitInstance - DLL initialization
#include "ExceptionHandler.h"
int extern __cdecl WriteMiniDump(LPEXCEPTION_POINTERS pExceptPtrs, int nRet);


#ifdef _WIN32

LONG WINAPI BBQUnhandledExceptionFilter( EXCEPTION_POINTERS * ExceptionInfo )
{
#if 0
	char pszVersion[129], pszBuildNumber[129] = "build";
	sprintf( pszVersion, "%d.%d.%d", MAJOR_VERSION, MINOR_VERSION, BUILD_NUMBER );
	RecordExceptionInfo( ExceptionInfo, "bbqsvr.exe", pszVersion, pszBuildNumber );
#else
	WriteMiniDump( ExceptionInfo );
#endif

	return EXCEPTION_CONTINUE_SEARCH;
}

#endif
BOOL CYYSpriteX2App::InitInstance()
{
	BOOL bInit = COleControlModule::InitInstance();
		//g_pSpriteX2Ctrl =NULL;

	if (bInit)
	{
		// TODO: Add your own module initialization code here.
	}
CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

//AfxMessageBox("CYYSpriteX2App::InitInstance");
  //if(!AfxOleInit())  // Your addition starts here
  //{
  //      AfxMessageBox("Could not initialize COM dll");
  //      return FALSE;
  //}     // End of your addition

  AfxEnableControlContainer();

SetUnhandledExceptionFilter( BBQUnhandledExceptionFilter );
 	return bInit;
}



// CYYSpriteX2App::ExitInstance - DLL termination

int CYYSpriteX2App::ExitInstance()
{
	// TODO: Add your own module termination code here.

	return COleControlModule::ExitInstance();
}
#ifdef D_SAFE_ACTIVEX
#if 1

#include "comcat.h"
#include ".\yyspritex-2.h"
//////HRESULT CreateComponentCategory(CATID catid, WCHAR *catDescription)
//////{
//////    ICatRegister *pcr = NULL ;
//////    HRESULT hr = S_OK ;
//////
//////    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, 
//////            NULL, CLSCTX_INPROC_SERVER, IID_ICatRegister, (void**)&pcr);
//////    if (FAILED(hr))
//////        return hr;
//////
//////    // Make sure the HKCR\Component Categories\{..catid...}
//////    // key is registered.
//////    CATEGORYINFO catinfo;
//////    catinfo.catid = catid;
//////    catinfo.lcid = 0x0409 ; // english
//////    size_t len;
//////    // Make sure the provided description is not too long.
//////    // Only copy the first 127 characters if it is.
//////    // The second parameter of StringCchLength is the maximum
//////    // number of characters that may be read into catDescription.
//////    // There must be room for a NULL-terminator. The third parameter
//////    // contains the number of characters excluding the NULL-terminator.
//////	hr = StringCchLengthW(catDescription, STRSAFE_MAX_CCH, &len);
//////	if (SUCCEEDED(hr))
//////	   {
//////        if (len>127)
//////		   {
//////           len = 127;
//////		   }
//////		}   
//////    else
//////	    {
//////	//	TODO: Write an error handler;
//////		}
//////	// The second parameter of StringCchCopy is 128 because you need 
//////    // room for a NULL-terminator.    
//////	hr = StringCchCopyW(catinfo.szDescription, len + 1, 
//////           catDescription);
//////	// Make sure the description is null terminated.
//////        catinfo.szDescription[len + 1] = '\0';
//////
//////    hr = pcr->RegisterCategories(1, &catinfo);
//////        pcr->Release();
//////
//////    return hr;
//////}
//////HRESULT RegisterCLSIDInCategory(REFCLSID clsid, CATID catid)
//////{
//////// Register your component categories information.
//////    ICatRegister *pcr = NULL ;
//////    HRESULT hr = S_OK ;
//////    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, 
//////                NULL, CLSCTX_INPROC_SERVER, IID_ICatRegister, (void**)&pcr);
//////    if (SUCCEEDED(hr))
//////    {
//////       // Register this category as being "implemented" by the class.
//////       CATID rgcatid[1] ;
//////       rgcatid[0] = catid;
//////       hr = pcr->RegisterClassImplCategories(clsid, 1, rgcatid);
//////    }
//////
//////    if (pcr != NULL)
//////        pcr->Release();
//////		
//////    return hr;
//////}
//////STDAPI DllRegisterServer(void)
//////{
//////    HRESULT  hr;    // return for safety functions
//////
//////    AFX_MANAGE_STATE(_afxModuleAddrThis);
//////
//////    if (!AfxOleRegisterTypeLib(AfxGetInstanceHandle(), _tlid)) 
//////        return ResultFromScode(SELFREG_E_TYPELIB);
//////
//////    if (!COleObjectFactoryEx::UpdateRegistryAll(TRUE))
//////        return ResultFromScode(SELFREG_E_CLASS);
//////
//////    // Mark the control as safe for initializing.
//////
//////    hr = CreateComponentCategory(CATID_SafeForInitializing, L"Controls safely initializable from persistent data!");
//////    if (FAILED(hr))
//////        return hr;
//////
//////    hr = RegisterCLSIDInCategory(CLSID_SafeItem, CATID_SafeForInitializing);
//////    if (FAILED(hr))
//////        return hr;
//////
//////    // Mark the control as safe for scripting.
//////
//////    hr = CreateComponentCategory(CATID_SafeForScripting, L"Controls  safely scriptable!");
//////    if (FAILED(hr))
//////        return hr;
//////
//////    hr = RegisterCLSIDInCategory(CLSID_SafeItem, CATID_SafeForScripting);
//////    if (FAILED(hr))
//////        return hr;
//////
//////    return NOERROR;
//////}
//////HRESULT UnRegisterCLSIDInCategory(REFCLSID clsid, CATID catid)
//////{
//////    ICatRegister *pcr = NULL ;
//////    HRESULT hr = S_OK ;
//////
//////    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, 
//////            NULL, CLSCTX_INPROC_SERVER, IID_ICatRegister, (void**)&pcr);
//////    if (SUCCEEDED(hr))
//////    {
//////       // Unregister this category as being "implemented" by the class.
//////       CATID rgcatid[1] ;
//////       rgcatid[0] = catid;
//////       hr = pcr->UnRegisterClassImplCategories(clsid, 1, rgcatid);
//////    }
//////
//////    if (pcr != NULL)
//////        pcr->Release();
//////
//////    return hr;
//////}
//////STDAPI DllUnregisterServer(void)
//////{
//////AFX_MANAGE_STATE(_afxModuleAddrThis);
//////// Remove entries from the registry.
//////HRESULT hr; // return for safety functions
//////hr = UnRegisterCLSIDInCategory(CLSID_SafeItem, CATID_SafeForInitializing);
//////if (SUCCEEDED(hr))
////// hr = UnRegisterCLSIDInCategory(CLSID_SafeItem, CATID_SafeForScripting);
//////if (!AfxOleUnregisterTypeLib(_tlid, _wVerMajor, _wVerMinor))
////// return ResultFromScode(SELFREG_E_TYPELIB);
//////if (!COleObjectFactoryEx::UpdateRegistryAll(FALSE))
////// return ResultFromScode(SELFREG_E_CLASS);
//////return hr;
//////}

// 创建组件种类
HRESULT CreateComponentCategory(CATID catid, WCHAR* catDescription)
{
ICatRegister* pcr = NULL ;
HRESULT hr = S_OK ;

hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER, IID_ICatRegister, (void**)&pcr);
if (FAILED(hr))
return hr;

// Make sure the HKCR\Component Categories\{..catid}
// key is registered.
CATEGORYINFO catinfo;
catinfo.catid = catid;
catinfo.lcid = 0x0409 ; // english

// Make sure the provided description is not too long.
// Only copy the first 127 characters if it is.
int len = wcslen(catDescription);
if (len>127)
len = 127;
wcsncpy(catinfo.szDescription, catDescription, len);

// Make sure the description is null terminated.
catinfo.szDescription[len] = '\0';

hr = pcr->RegisterCategories(1, &catinfo);
pcr->Release();

return hr;
}

// 注册组件种类
HRESULT RegisterCLSIDInCategory(REFCLSID clsid, CATID catid)
{

// Register your component categories information.
ICatRegister* pcr = NULL ;
HRESULT hr = S_OK ;
hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER, IID_ICatRegister, (void**)&pcr);
if (SUCCEEDED(hr))
{

// Register this category as being implemented by the class.
CATID rgcatid[1] ;
rgcatid[0] = catid;
hr = pcr->RegisterClassImplCategories(clsid, 1, rgcatid);
}
if (pcr != NULL)
pcr->Release();
return hr;
}

// 卸载组件种类
HRESULT UnRegisterCLSIDInCategory(REFCLSID clsid, CATID catid)
{
ICatRegister* pcr = NULL ;
HRESULT hr = S_OK ;

hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER, IID_ICatRegister, (void**)&pcr);
if (SUCCEEDED(hr))
{

// Unregister this category as being implemented by the class.
CATID rgcatid[1] ;
rgcatid[0] = catid;
hr = pcr->UnRegisterClassImplCategories(clsid, 1, rgcatid);
}

if (pcr != NULL)
pcr->Release();

return hr;
}

//修改以下函数：
// DllRegisterServer - Adds entries to the system registry
STDAPI DllRegisterServer(void)
{
HRESULT hr;

AFX_MANAGE_STATE(_afxModuleAddrThis);

if (!AfxOleRegisterTypeLib(AfxGetInstanceHandle(), _tlid))
return ResultFromScode(SELFREG_E_TYPELIB);

if (!COleObjectFactoryEx::UpdateRegistryAll(TRUE))
return ResultFromScode(SELFREG_E_CLASS);

// 标记控件初始化安全.
// 创建初始化安全组件种类
hr = CreateComponentCategory(CATID_SafeForInitializing, L"Controls safely initializable from persistent data!");
if (FAILED(hr))
return hr;

// 注册初始化安全
hr = RegisterCLSIDInCategory(CLSID_SafeItem, CATID_SafeForInitializing);
if (FAILED(hr))
return hr;

// 标记控件脚本安全
// 创建脚本安全组件种类 
hr = CreateComponentCategory(CATID_SafeForScripting, L"Controls safely scriptable!");
if (FAILED(hr))
return hr;

// 注册脚本安全组件种类
hr = RegisterCLSIDInCategory(CLSID_SafeItem, CATID_SafeForScripting);
if (FAILED(hr))
return hr;

return NOERROR;
}

/**///////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
HRESULT hr;
AFX_MANAGE_STATE(_afxModuleAddrThis);

if (!AfxOleUnregisterTypeLib(_tlid, _wVerMajor, _wVerMinor))
return ResultFromScode(SELFREG_E_TYPELIB);

if (!COleObjectFactoryEx::UpdateRegistryAll(FALSE))
return ResultFromScode(SELFREG_E_CLASS);

// 删除控件初始化安全入口.
hr=UnRegisterCLSIDInCategory(CLSID_SafeItem, CATID_SafeForInitializing);
if (FAILED(hr))
return hr;

// 删除控件脚本安全入口
hr=UnRegisterCLSIDInCategory(CLSID_SafeItem, CATID_SafeForScripting);
if (FAILED(hr))
return hr;

/**///////////////////////////
return NOERROR;
} 



 
/////////////////////////////////////
#else
#include "comcat.h"

// Helper function to create a component category and associated
// description
HRESULT CreateComponentCategory(CATID catid, WCHAR* catDescription)
{
    ICatRegister* pcr = NULL ;
    HRESULT hr = S_OK ;

    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_ICatRegister,
                          (void**)&pcr);
    if (FAILED(hr))
      return hr;

    // Make sure the HKCR\Component Categories\{..catid...}
    // key is registered
    CATEGORYINFO catinfo;
    catinfo.catid = catid;
    catinfo.lcid = 0x0409 ; // english

    // Make sure the provided description is not too long.
    // Only copy the first 127 characters if it is
    int len = wcslen(catDescription);
    if (len>127)
      len = 127;
    wcsncpy(catinfo.szDescription, catDescription, len);
    // Make sure the description is null terminated
    catinfo.szDescription[len] = '\0';

    hr = pcr->RegisterCategories(1, &catinfo);
    pcr->Release();

    return hr;
}

// Helper function to register a CLSID as belonging to a component
// category
HRESULT RegisterCLSIDInCategory(REFCLSID clsid, CATID catid)
{
    // Register your component categories information.
    ICatRegister* pcr = NULL ;
    HRESULT hr = S_OK ;
    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_ICatRegister,
                          (void**)&pcr);
    if (SUCCEEDED(hr))
    {
      // Register this category as being "implemented" by
      // the class.
      CATID rgcatid[1] ;
      rgcatid[0] = catid;
      hr = pcr->RegisterClassImplCategories(clsid, 1, rgcatid);
    }

    if (pcr != NULL)
      pcr->Release();

    return hr;
}
STDAPI DllRegisterServer(void)
      {
          AFX_MANAGE_STATE(_afxModuleAddrThis);

          if (!AfxOleRegisterTypeLib(AfxGetInstanceHandle(), _tlid))
              return ResultFromScode(SELFREG_E_TYPELIB);

          if (!COleObjectFactoryEx::UpdateRegistryAll(TRUE))
              return ResultFromScode(SELFREG_E_CLASS);

          if (FAILED( CreateComponentCategory(
                  CATID_SafeForScripting,
                  L"Controls that are safely scriptable") ))
                return ResultFromScode(SELFREG_E_CLASS);

          if (FAILED( CreateComponentCategory(
                  CATID_SafeForInitializing,
                  L"Controls safely initializable from persistent data") ))
                return ResultFromScode(SELFREG_E_CLASS);

          if (FAILED( RegisterCLSIDInCategory(
                  _ctlid, CATID_SafeForScripting) ))
                return ResultFromScode(SELFREG_E_CLASS);

          if (FAILED( RegisterCLSIDInCategory(
                  _ctlid, CATID_SafeForInitializing) ))
                return ResultFromScode(SELFREG_E_CLASS);

          return NOERROR;
      }
			HRESULT UnRegisterCLSIDInCategory(REFCLSID clsid, CATID catid)
{
    ICatRegister *pcr = NULL ;
    HRESULT hr = S_OK ;

    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, 
            NULL, CLSCTX_INPROC_SERVER, IID_ICatRegister, (void**)&pcr);
    if (SUCCEEDED(hr))
    {
       // Unregister this category as being "implemented" by the class.
       CATID rgcatid[1] ;
       rgcatid[0] = catid;
       hr = pcr->UnRegisterClassImplCategories(clsid, 1, rgcatid);
    }

    if (pcr != NULL)
        pcr->Release();

    return hr;
}
STDAPI DllUnregisterServer(void)
{
    HRESULT hr;    // HResult used by Safety Functions

    AFX_MANAGE_STATE(_afxModuleAddrThis);

    if (!AfxOleUnregisterTypeLib(_tlid)) 
        return ResultFromScode(SELFREG_E_TYPELIB);

    if (!COleObjectFactoryEx::UpdateRegistryAll(FALSE))
        return ResultFromScode(SELFREG_E_CLASS);

    // Remove entries from the registry.

    hr=UnRegisterCLSIDInCategory(CLSID_SafeItem, CATID_SafeForInitializing);
    if (FAILED(hr))
        return hr;

    hr=UnRegisterCLSIDInCategory(CLSID_SafeItem, CATID_SafeForScripting);
    if (FAILED(hr))
        return hr;

    return NOERROR;
}
#endif

#else//not D_SAFE_ACTIVEX
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleRegisterTypeLib(AfxGetInstanceHandle(), _tlid))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(TRUE))
		return ResultFromScode(SELFREG_E_CLASS);

	return NOERROR;
}



// DllUnregisterServer - Removes entries from the system registry
  
STDAPI DllUnregisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleUnregisterTypeLib(_tlid, _wVerMajor, _wVerMinor))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(FALSE))
		return ResultFromScode(SELFREG_E_CLASS);

	return NOERROR;
}
#endif //D_SAFE_ACTIVEX
