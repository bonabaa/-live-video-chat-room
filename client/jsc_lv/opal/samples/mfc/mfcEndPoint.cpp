// MfcEndPoint.cpp: implementation of the CMfcEndPoint class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "mfc.h"
#include "MfcEndPoint.h"
#include "MfcDlg.h"

#include <h323/h323ep.h>
#include <sip/sipep.h>


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMfcManager::CMfcManager()
{
  m_dialog = NULL;
}

CMfcManager::~CMfcManager()
{

}

PBoolean CMfcManager::Initialise(CMfcDlg *dlg)
{
  m_dialog = dlg;

#if OPAL_H323
  new H323EndPoint(*this);
#endif // OPAL_H323
#if OPAL_SIP
  new SIPEndPoint(*this);
#endif // OPAL_SIP

  return true;
}


#if 0
void CMfcEndPoint::OnConnectionEstablished(H323Connection & connection, const PString & token)
{
  m_dialog->m_token = token;
  m_dialog->m_call.EnableWindow(PFalse);
  m_dialog->m_answer.EnableWindow(PFalse);
  m_dialog->m_refuse.EnableWindow(PFalse);
  m_dialog->m_hangup.EnableWindow();
  m_dialog->m_caller.SetWindowText("In call with " + connection.GetRemotePartyName());
}

void CMfcEndPoint::OnConnectionCleared(H323Connection &, const PString &)
{
  m_dialog->m_answer.EnableWindow(PFalse);
  m_dialog->m_refuse.EnableWindow(PFalse);
  m_dialog->m_hangup.EnableWindow(PFalse);
  m_dialog->m_call.EnableWindow();
  m_dialog->m_caller.SetWindowText("");
}

H323Connection::AnswerCallResponse CMfcEndPoint::OnAnswerCall(H323Connection & connection, const PString &caller, const H323SignalPDU &, H323SignalPDU &)
{
  m_dialog->m_token = connection.GetCallToken();
  m_dialog->m_caller.SetWindowText(caller + " is calling.");
  m_dialog->m_answer.EnableWindow();
  m_dialog->m_refuse.EnableWindow();
  m_dialog->m_call.EnableWindow(PFalse);
  return H323Connection::AnswerCallPending;
}
#endif
