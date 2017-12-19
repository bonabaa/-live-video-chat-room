// MfcEndPoint.h: interface for the CMfcEndPoint class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MFCENDPOINT_H__B1AF53B8_E3C6_4590_BBC8_6614922EC493__INCLUDED_)
#define AFX_MFCENDPOINT_H__B1AF53B8_E3C6_4590_BBC8_6614922EC493__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <opal/manager.h>

class CMfcDlg;

class CMfcManager : public OpalManager
{
public:
	PBoolean Initialise(CMfcDlg * dlg);
	CMfcManager();
	virtual ~CMfcManager();
protected:
	CMfcDlg * m_dialog;
};

#endif // !defined(AFX_MFCENDPOINT_H__B1AF53B8_E3C6_4590_BBC8_6614922EC493__INCLUDED_)
