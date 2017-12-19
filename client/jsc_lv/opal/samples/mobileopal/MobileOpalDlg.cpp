/*
 * MobileOpalDlg.h
 *
 * Sample Windows Mobile application.
 *
 * Open Phone Abstraction Library
 *
 * Copyright (c) 2008 Vox Lucida
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Vox Lucida (Robert Jongbloed)
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 21326 $
 * $Author: rjongbloed $
 * $Date: 2008-10-14 07:22:43 +0000 (Tue, 14 Oct 2008) $
 */

#include "stdafx.h"
#include "MobileOPAL.h"
#include "MobileOpalDlg.h"
#include "OptionsGeneral.h"
#include "OptionsH323.h"
#include "OptionsSIP.h"

#include <winsock2.h>
#include <iphlpapi.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#define TRACE_OPTIONS "TraceLevel=4"
#else
#define TRACE_OPTIONS "TraceLevel=4 TraceAppend TraceFile=\\MobileOpalLog.txt"
#endif

#define SPEAKERMODE_METHOD 3


static UINT TimerID = 1;

static TCHAR const RecentCallsSection[]  = L"RecentCalls";
static TCHAR const OptionsSection[]      = L"Options";
static TCHAR const UserNameKey[]         = L"UserName";
static TCHAR const DisplayNameKey[]      = L"DisplayName";
static TCHAR const STUNServerKey[]       = L"STUNServer";
static TCHAR const InterfaceAddressKey[] = L"InterfaceAddress";
static TCHAR const GkTypeKey[]           = L"GatekeeperType";
static TCHAR const GkIdKey[]             = L"GatekeeperIdentifer";
static TCHAR const GkHostKey[]           = L"GatekeeperHost";
static TCHAR const GkAliasKey[]          = L"GatekeeperAlias";
static TCHAR const GkAuthUserKey[]       = L"GatekeeperUser";
static TCHAR const GkPasswordKey[]       = L"GatekeeperPassword";
static TCHAR const RegistrarAorKey[]     = L"RegistrarAOR";
static TCHAR const RegistrarHostKey[]    = L"RegistrarHost";
static TCHAR const RegistrarUserKey[]    = L"RegistrarUser";
static TCHAR const RegistrarPassKey[]    = L"RegistrarPassword";
static TCHAR const RegistrarRealmKey[]   = L"RegistrarRealm";



static CString GetOptionString(LPCTSTR szKey, LPCTSTR szDefault = NULL)
{
  return AfxGetApp()->GetProfileString(OptionsSection, szKey, szDefault);
}


static CStringA GetOptionStringA(LPCTSTR szKey, LPCTSTR szDefault = NULL)
{
  CStringA str((const TCHAR *)AfxGetApp()->GetProfileString(OptionsSection, szKey, szDefault));
  return str;
}


static UINT GetOptionInt(LPCTSTR szKey)
{
  return AfxGetApp()->GetProfileInt(OptionsSection, szKey, 0);
}


static void SetOptionString(LPCTSTR szKey, LPCTSTR szValue)
{
  AfxGetApp()->WriteProfileString(OptionsSection, szKey, szValue);
}


static void SetOptionInt(LPCTSTR szKey, UINT szValue)
{
  AfxGetApp()->WriteProfileInt(OptionsSection, szKey, szValue);
}


void GetNetworkInterfaces(CStringArray & interfaces, bool includeNames)
{
  // Get interfaces
  BYTE * buffer = NULL;
  ULONG size = 0;
  DWORD error = GetIpAddrTable(NULL, &size, FALSE);
  if (error == ERROR_INSUFFICIENT_BUFFER) {
    buffer = new BYTE[size];
    if (buffer != NULL)
      error = GetIpAddrTable((MIB_IPADDRTABLE *)buffer, &size, FALSE);
  }

  if (error == ERROR_SUCCESS) {
    const MIB_IPADDRTABLE * ipaddr = (const MIB_IPADDRTABLE *)buffer;
    for (unsigned i = 0; i < ipaddr->dwNumEntries; ++i) {
      in_addr ip;
      ip.S_un.S_addr = ipaddr->table[i].dwAddr;
      if (ntohl(ip.S_un.S_addr) != INADDR_LOOPBACK) {
        CStringA iface;
        if (ip.S_un.S_addr != INADDR_ANY)
          iface = inet_ntoa(ip);

        if (includeNames) {
          MIB_IFROW info;
          info.dwIndex = ipaddr->table[i].dwIndex;
          if (GetIfEntry(&info) == NO_ERROR && info.dwDescrLen > 0) {
            iface += '%';
            iface.Append((const char *)info.bDescr, info.dwDescrLen);
          }
        }

        if (!iface.IsEmpty())
          interfaces.Add(CString(iface));
      }
    }
  }
  delete [] buffer;
}


#if SPEAKERMODE_METHOD==1

  #define MM_WOM_FORCESPEAKER (WM_USER+2)

  static void SetSpeakerMode(bool speakerphone)
  {
    MMRESULT result = waveOutMessage((HWAVEOUT)0, MM_WOM_FORCESPEAKER, speakerphone, 0);
    if (result != MMSYSERR_NOERROR) {
      wchar_t buffer[200];
      wsprintf(buffer, L"Could not do speakerphone switch, error=%u", result);
      AfxMessageBox(buffer);
    }
  }

#elif SPEAKERMODE_METHOD==2

  /* Stuff from Wavedev.h */
  #define IOCTL_WAV_MESSAGE 0x001d000c
  typedef struct {
    UINT uDeviceId;
    UINT uMsg;
    DWORD dwUser;
    DWORD dwParam1;
    DWORD dwParam2;
  } MMDRV_MESSAGE_PARAMS;
  /* End of Wavedev.h extract */


  class WavDevice
  {
    private:
      HANDLE hWavDev;

    public:
      WavDevice()
      {
        hWavDev = CreateFile(TEXT("WAV1:"), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hWavDev == INVALID_HANDLE_VALUE) {
          wchar_t buffer[200];
          wsprintf(buffer, L"Could not open device, error=%u", GetLastError());
          AfxMessageBox(buffer);
        }
      }

      ~WavDevice()
      {
        if (hWavDev != INVALID_HANDLE_VALUE)
          CloseHandle(hWavDev);
      }

      bool SendMessage(UINT uMsg, DWORD dwParam1 = 0, DWORD dwParam2 = 0)
      {
        if (hWavDev == INVALID_HANDLE_VALUE)
          return false;

        DWORD dwRet, dwOut; 
        MMDRV_MESSAGE_PARAMS mp; 
        memset(&mp,0,sizeof(mp));
        mp.uMsg = uMsg;
        mp.dwParam1 = dwParam1;
        mp.dwParam2 = dwParam1;
        if (DeviceIoControl(hWavDev, IOCTL_WAV_MESSAGE, &mp, sizeof(mp), &dwOut, sizeof(dwOut), &dwRet, 0))
          return true;

        wchar_t buffer[200];
        wsprintf(buffer, L"Could not do speakerphone switch, error=%u", GetLastError());
        AfxMessageBox(buffer);
        return false;
      }
  };


  static void SetSpeakerMode(bool speakerphone)
  {
    WavDevice wd;
    if (!wd.SendMessage(1002, speakerphone ? 0 : 2))
      return;
    if (!wd.SendMessage(1012, speakerphone ? 0 : 1))
      return;
    if (speakerphone && !wd.SendMessage(1013))
      return;
    wd.SendMessage(1000, speakerphone ? 2 : 4, speakerphone ? 0 : 7);
  }

#elif SPEAKERMODE_METHOD==3

  class OsSvcsDll
  {
    private:
      HMODULE hDLL;
      typedef HRESULT (* SetSpeakerModeFn)(DWORD mode);
      SetSpeakerModeFn pfnSetSpeakerMode;

    public:
      OsSvcsDll()
      {
        hDLL = LoadLibrary(L"\\windows\\ossvcs.dll");
        if (hDLL == NULL) {
          wchar_t buffer[200];
          wsprintf(buffer, L"Could not open DLL, error=%u", GetLastError());
          AfxMessageBox(buffer);
        }
        pfnSetSpeakerMode = (SetSpeakerModeFn)GetProcAddress(hDLL, (LPCTSTR)218);
        if (pfnSetSpeakerMode == NULL) {
          wchar_t buffer[200];
          wsprintf(buffer, L"Could not open DLL, error=%u", GetLastError());
          AfxMessageBox(buffer);
        }
      }

      ~OsSvcsDll()
      {
        if (hDLL != NULL)
          FreeLibrary(hDLL);
      }

      bool SetSpeakerMode(DWORD speakerphone)
      {
        if (pfnSetSpeakerMode == NULL)
          return false;

        HRESULT result = pfnSetSpeakerMode(speakerphone);
        if (result == 0)
          return true;

        wchar_t buffer[200];
        wsprintf(buffer, L"Could not do speakerphone switch, error=0x%x", result);
        AfxMessageBox(buffer);
        return false;
      }
  };


  static void SetSpeakerMode(bool speakerphone)
  {
    OsSvcsDll dll;
    dll.SetSpeakerMode(speakerphone ? 1 : 0);
  }

#else

  static void SetSpeakerMode(bool /*speakerphone*/)
  {
  }

#endif // SPEAKERMODE_METHOD


///////////////////////////////////////////////////////////////////////////////

// CMobileOpalDlg dialog

CMobileOpalDlg::CMobileOpalDlg(CWnd* pParent /*=NULL*/)
  : CDialog(CMobileOpalDlg::IDD, pParent)
  , m_opal(NULL)
  , m_opalVersion(OPAL_C_API_VERSION)
  , m_speakerphone(false)
{
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}


CMobileOpalDlg::~CMobileOpalDlg()
{
  OpalShutDown(m_opal); // Fail safe shutdown, can handle NULL
}


void CMobileOpalDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Text(pDX, IDC_ADDRESS, m_callAddress);
  DDX_Text(pDX, IDC_LOCAL_ADDRESS, m_localAddress);
  DDX_Control(pDX, IDC_ADDRESS, m_ctrlCallAddress);
  DDX_Control(pDX, IDC_STATUS, m_ctrlStatus);
  DDX_Control(pDX, IDC_LOCAL_ADDRESS, m_ctrlLocalStatus);
}


BEGIN_MESSAGE_MAP(CMobileOpalDlg, CDialog)
  ON_WM_SIZE()
  ON_WM_TIMER()
  ON_COMMAND(ID_CALL_ANSWER, &CMobileOpalDlg::OnCallAnswer)
  ON_CBN_EDITCHANGE(IDC_ADDRESS, &CMobileOpalDlg::OnChangedAddress)
  ON_CBN_SELENDOK(IDC_ADDRESS, &CMobileOpalDlg::OnSelectedAddress)
  ON_COMMAND(IDM_OPTIONS_GENERAL, &CMobileOpalDlg::OnMenuOptionsGeneral)
  ON_COMMAND(IDM_OPTIONS_H323, &CMobileOpalDlg::OnMenuOptionsH323)
  ON_COMMAND(IDM_OPTIONS_SIP, &CMobileOpalDlg::OnMenuOptionsSIP)
  ON_COMMAND(IDM_EXIT, &CMobileOpalDlg::OnExit)
  ON_COMMAND(IDM_SPEAKERPHONE, &CMobileOpalDlg::OnSpeakerphone)
END_MESSAGE_MAP()


// CMobileOpalDlg message handlers

BOOL CMobileOpalDlg::OnInitDialog()
{
  CDialog::OnInitDialog();

  // Set the icon for this dialog.  The framework does this automatically
  //  when the application's main window is not a dialog
  SetIcon(m_hIcon, TRUE);			// Set big icon
  SetIcon(m_hIcon, FALSE);		// Set small icon

  if (!m_dlgCommandBar.Create(this) || !m_dlgCommandBar.InsertMenuBar(IDR_MAINFRAME)) {
    TRACE0("Failed to create CommandBar\n");
    EndDialog(IDCANCEL);
    return FALSE;      // fail to create
  }

  // Resize to screen width
  ShowWindow(SW_SHOWMAXIMIZED);

  // Start background check for OPAL messages
  SetTimer(TimerID, 100, NULL);

  // Recent calls list
  unsigned index = 1;
  for (;;) {
    CString key;
    key.Format(L"%04u", index++);
    CString uri = AfxGetApp()->GetProfileString(RecentCallsSection, key);
    if (uri.IsEmpty())
      break;
    m_ctrlCallAddress.AddString(uri);
  }

  // Speakerphone state
  m_speakerphone = false;
  SetSpeakerMode(m_speakerphone);

  // Get interfaces
  CStringArray interfaces;
  GetNetworkInterfaces(interfaces, false);
  for (int i = 0; i < interfaces.GetSize(); ++i) {
    if (!m_localAddress.IsEmpty())
      m_localAddress += ", ";
    m_localAddress += interfaces[i];
  }
  UpdateData(false);

  SetCallButton(false);

  return TRUE;  // return TRUE  unless you set the focus to a control
}


void CMobileOpalDlg::OnSize(UINT /*nType*/, int cx, int cy)
{
  // Adjust some controls to be full width of screen
  CRect box;
  m_ctrlCallAddress.GetWindowRect(&box);
  m_ctrlCallAddress.SetWindowPos(NULL, 0, 0, cx-box.left*2, box.Height(), SWP_NOMOVE|SWP_NOZORDER);
  m_ctrlLocalStatus.GetWindowRect(&box);
  m_ctrlLocalStatus.SetWindowPos(NULL, 0, 0, cx-box.left, box.Height(), SWP_NOMOVE|SWP_NOZORDER);
  m_ctrlStatus.GetWindowRect(&box);
  m_ctrlStatus.SetWindowPos(NULL, 0, 0, cx, box.Height(), SWP_NOMOVE|SWP_NOZORDER);
}


void CMobileOpalDlg::InitialiseOPAL()
{
  /////////////////////////////////////////////////////////////////////
  // Start up and initialise OPAL

  if (m_opal == NULL) {
    m_opal = OpalInitialise(&m_opalVersion, "pc h323 sip " TRACE_OPTIONS);
    if (m_opal == NULL) {
      ErrorBox(IDS_INIT_FAIL);
      EndDialog(IDCANCEL);
      return;
    }
  }

  OpalMessage command;
  OpalMessage * response;

  // SIP registrar un-regisration
  CStringA strAOR = GetOptionStringA(RegistrarAorKey);
  CStringA strHost = GetOptionStringA(RegistrarHostKey);
  if (!m_currentAOR.IsEmpty() && (m_currentAOR != strAOR || m_currentHost != strHost)) {
    // Registration changed, so unregister the previous name
    memset(&command, 0, sizeof(command));
    command.m_type = OpalCmdRegistration;
    command.m_param.m_registrationInfo.m_protocol = "sip";
    command.m_param.m_registrationInfo.m_identifier = m_currentAOR;
    command.m_param.m_registrationInfo.m_hostName = m_currentHost;
    response = OpalSendMessage(m_opal, &command);
    if (response != NULL && response->m_type == OpalCmdRegistration) {
      OpalFreeMessage(response);

      UpdateWindow();

      // Wait for unregister to complete.
      while ((response = OpalGetMessage(m_opal, 30000)) != NULL) {
        HandleMessage(*response);
        if (response->m_type == OpalIndRegistration && response->m_param.m_registrationStatus.m_status == OpalRegisterRemoved)
          break;
      }
    }
    OpalFreeMessage(response);
    m_currentAOR.Empty();
    m_currentHost.Empty();
  }

  // General options
  memset(&command, 0, sizeof(command));
  command.m_type = OpalCmdSetGeneralParameters;

  CStringA strStunServer = GetOptionStringA(STUNServerKey);
  command.m_param.m_general.m_stunServer = strStunServer;
  command.m_param.m_general.m_audioPlayerDevice = "Audio Output";
  command.m_param.m_general.m_audioRecordDevice = "Audio Input";
  command.m_param.m_general.m_mediaMask = "RFC4175*";

  if ((response = OpalSendMessage(m_opal, &command)) == NULL || response->m_type == OpalIndCommandError)
    ErrorBox(IDS_CONFIGURATION_FAIL, response);
  OpalFreeMessage(response);

  // Options across all protocols
  memset(&command, 0, sizeof(command));
  command.m_type = OpalCmdSetProtocolParameters;

  CStringA strUserName = GetOptionStringA(UserNameKey);
  command.m_param.m_protocol.m_userName = strUserName;

  CStringA strDisplayName = GetOptionStringA(UserNameKey);
  command.m_param.m_protocol.m_displayName = strDisplayName;

  CStringA interfaceAddress = GetOptionStringA(InterfaceAddressKey, L"*");
  command.m_param.m_protocol.m_interfaceAddresses = interfaceAddress;

  if ((response = OpalSendMessage(m_opal, &command)) == NULL || response->m_type == OpalIndCommandError)
    ErrorBox(IDS_LISTEN_FAIL, response);
  OpalFreeMessage(response);

  // H.323 gatekeeper registration
  memset(&command, 0, sizeof(command));
  command.m_type = OpalCmdRegistration;

  UINT gkType = GetOptionInt(GkTypeKey);
  if (gkType > 0) {
    command.m_param.m_registrationInfo.m_protocol = "h323";

    CStringA strAliasName = GetOptionStringA(GkAliasKey);
    command.m_param.m_registrationInfo.m_identifier = strAliasName;

    CStringA strGkId = GetOptionStringA(GkIdKey);
    command.m_param.m_registrationInfo.m_adminEntity = strGkId;

    CStringA strGkHost = GetOptionStringA(GkHostKey);
    command.m_param.m_registrationInfo.m_hostName = strGkHost;

    CStringA strGkAuthUser = GetOptionStringA(GkAuthUserKey);
    command.m_param.m_registrationInfo.m_authUserName = strGkAuthUser;

    CStringA strGkPassword = GetOptionStringA(GkPasswordKey);
    command.m_param.m_registrationInfo.m_password = strGkPassword;

    SetStatusText(IDS_REGISTERING);
    if ((response = OpalSendMessage(m_opal, &command)) == NULL || response->m_type == OpalIndCommandError)
      ErrorBox(IDS_REGISTRATION_FAIL, response);
    OpalFreeMessage(response);
  }

  // SIP registrar registration
  if (strAOR.IsEmpty())
    SetStatusText(IDS_READY);
  else {
    memset(&command, 0, sizeof(command));
    command.m_type = OpalCmdRegistration;

    command.m_param.m_registrationInfo.m_protocol = "sip";

    command.m_param.m_registrationInfo.m_identifier = strAOR;
    command.m_param.m_registrationInfo.m_hostName = strHost;

    CStringA strAuthUser = GetOptionStringA(RegistrarUserKey);
    command.m_param.m_registrationInfo.m_authUserName = strAuthUser;

    CStringA strPassword = GetOptionStringA(RegistrarPassKey);
    command.m_param.m_registrationInfo.m_password = strPassword;

    CStringA strRealm = GetOptionStringA(RegistrarRealmKey);
    command.m_param.m_registrationInfo.m_adminEntity = strRealm;

    command.m_param.m_registrationInfo.m_timeToLive = 300;
    command.m_param.m_registrationInfo.m_messageWaiting = 300;

    SetStatusText(IDS_REGISTERING);
    if ((response = OpalSendMessage(m_opal, &command)) != NULL && response->m_type != OpalIndCommandError) {
      m_currentAOR = strAOR;
      m_currentHost = strHost;
    }
    else {
      ErrorBox(IDS_REGISTRATION_FAIL, response);
      SetStatusText(IDS_READY);
    }
    OpalFreeMessage(response);
  }
}


void CMobileOpalDlg::ErrorBox(UINT ids, const OpalMessage * response)
{
  CString text;
  if (response != NULL && response->m_param.m_commandError != NULL || *response->m_param.m_commandError != '\0')
    text = response->m_param.m_commandError;
  else {
    if (ids != 0)
      text.LoadString(ids);
    else
      text = "Error!";
  }

  MessageBox(text, NULL, MB_OK|MB_ICONEXCLAMATION);
}


void CMobileOpalDlg::SetStatusText(UINT ids, const char * str)
{
  CString text;
  if (str != NULL && str[0] != '\0') {
    if (isdigit(*str)) {
      char * colon;
      unsigned code = strtoul(str, &colon, 10);
      if (*colon == ':') {
        str = colon+1;
        while (isspace(*str))
          str++;
        text.LoadString(ids+IDS_CUSTOM_MESSAGES);
      }
    }

    if (text.IsEmpty())
    text = str;
  }
  else {
    if (ids == 0)
      return;
    text.LoadString(ids);
  }

  m_ctrlStatus.SetWindowText(text);
  m_ctrlStatus.UpdateWindow();
}


void CMobileOpalDlg::SetCallButton(bool enabled, UINT strId)
{
  TBBUTTONINFO tbi;
  memset (&tbi, 0, sizeof (tbi));
  tbi.cbSize = sizeof (tbi);
  tbi.dwMask = TBIF_STATE;
  if (!::SendMessage(m_dlgCommandBar.m_hCommandBar, TB_GETBUTTONINFO, ID_CALL_ANSWER, (LPARAM)&tbi)) 
    return;

  if (enabled)
    tbi.fsState |= TBSTATE_ENABLED;
  else
    tbi.fsState &= ~TBSTATE_ENABLED;

  CString text;
  if (strId != 0 && text.LoadString(strId)) {
    tbi.dwMask |= TBIF_TEXT;
    tbi.pszText = (LPWSTR)(const TCHAR *)text;
  }
  ::SendMessage(m_dlgCommandBar.m_hCommandBar, TB_SETBUTTONINFO, ID_CALL_ANSWER, (LPARAM)&tbi);
}


void CMobileOpalDlg::AddRecentCall(const CString & uri)
{
  int index = m_ctrlCallAddress.FindStringExact(-1, uri);
  if (index == 0)
    return;

  if (index > 0)
    m_ctrlCallAddress.DeleteString(index);

  m_ctrlCallAddress.InsertString(0, uri);

  index = 0;
  while (index < m_ctrlCallAddress.GetCount()) {
    CString str;
    m_ctrlCallAddress.GetLBText(index, str);
    CString key;
    key.Format(L"%04u", ++index);
    AfxGetApp()->WriteProfileString(RecentCallsSection, key, str);
  }
}


void CMobileOpalDlg::OnTimer(UINT_PTR nIDEvent)
{
  if (m_opal == NULL)
    InitialiseOPAL();

  if (nIDEvent == TimerID && m_opal != NULL) {
    OpalMessage * message = OpalGetMessage(m_opal, 0);
    if (message != NULL) {
      HandleMessage(*message);
      OpalFreeMessage(message);
    }
  }

  CDialog::OnTimer(nIDEvent);
}


void CMobileOpalDlg::HandleMessage(OpalMessage & message)
{
  switch (message.m_type) {
    case OpalIndRegistration :
      switch (message.m_param.m_registrationStatus.m_status) {
        case OpalRegisterSuccessful :
        case OpalRegisterRestored :
          SetStatusText(IDS_REGISTERED);
          break;

        case OpalRegisterRemoved :
          SetStatusText(IDS_UNREGISTERED, message.m_param.m_registrationStatus.m_error);
          break;

        case OpalRegisterRetrying :
          SetStatusText(IDS_REGISTERING);
          break;

        case OpalRegisterFailed :
          SetStatusText(0, message.m_param.m_registrationStatus.m_error);
      }
      break;

    case OpalIndMessageWaiting :
      if (message.m_param.m_messageWaiting.m_type != NULL && *message.m_param.m_messageWaiting.m_type != '\0') {
        CStringA msg;
        msg.Format(IDS_MESSAGE_WAITING,
                   message.m_param.m_messageWaiting.m_type,
                   message.m_param.m_messageWaiting.m_party);
        SetStatusText(0, msg);
      }
      break;

    case OpalIndIncomingCall :
      if (m_incomingCallToken.IsEmpty() && m_currentCallToken.IsEmpty()) {
        m_incomingCallToken = message.m_param.m_incomingCall.m_callToken;
        SetStatusText(IDS_INCOMING_CALL);
        SetCallButton(true, IDS_ANSWER);
        m_ctrlCallAddress.EnableWindow(false);
        ShowWindow(true);
        BringWindowToTop();
      }
      else {
        OpalMessage command;
        memset(&command, 0, sizeof(command));
        command.m_type = OpalCmdClearCall;
        command.m_param.m_clearCall.m_callToken = message.m_param.m_incomingCall.m_callToken;
        command.m_param.m_clearCall.m_reason = OpalCallEndedByAnswerDenied;
        OpalMessage * response = OpalSendMessage(m_opal, &command);
        if (response != NULL)
          OpalFreeMessage(response);
        SetStatusText(IDS_BUSY);
      }
      break;

    case OpalIndAlerting :
      SetStatusText(IDS_RINGING);
      break;

    case OpalIndEstablished :
      SetSpeakerMode(m_speakerphone);
      SetStatusText(IDS_ESTABLISHED);
      break;

    case OpalIndUserInput :
      SetStatusText(0, message.m_param.m_userInput.m_userInput);
      break;

    case OpalIndCallCleared :
      if (m_currentCallToken  == message.m_param.m_callCleared.m_callToken ||
          m_incomingCallToken == message.m_param.m_callCleared.m_callToken) {
        m_incomingCallToken.Empty();
        m_currentCallToken.Empty();

        SetCallButton(true, IDS_CALL);
        m_ctrlCallAddress.EnableWindow(true);

        SetStatusText(IDS_READY, message.m_param.m_callCleared.m_reason);
      }
  }
}

void CMobileOpalDlg::OnOK()
{
#if WIN32_PLATFORM_WFSP
  // For Smartphone this is the centre button
  OnCallAnswer();
#else
  // For Pocket PC this is the OK button in the top line
  OnCancel();
#endif
}


void CMobileOpalDlg::OnCancel()
{
  CString msg(MAKEINTRESOURCE(IDS_ASK_EXIT));
  if (MessageBox(msg, NULL, MB_YESNO|MB_ICONQUESTION) == IDYES)
    OnExit();
}


void CMobileOpalDlg::OnExit()
{
  if (m_opal != NULL) {
    KillTimer(TimerID);
    m_ctrlCallAddress.EnableWindow(false);
    SetStatusText(IDS_SHUTTING_DOWN);
    UpdateWindow();

    OpalShutDown(m_opal);
    m_opal = NULL;
    SetStatusText(0, "Shut down completed.");
    Sleep(2000);
  }

  CDialog::OnCancel();
}


void CMobileOpalDlg::OnCallAnswer()
{
  UpdateData();

  OpalMessage * response = NULL;
  OpalMessage command;
  memset(&command, 0, sizeof(command));

  if (!m_incomingCallToken.IsEmpty()) {
    SetStatusText(IDS_ANSWERING);
    SetCallButton(true, IDS_HANGUP);
    command.m_type = OpalCmdAnswerCall;
    command.m_param.m_callToken = m_incomingCallToken;
    response = OpalSendMessage(m_opal, &command);
  }
  else if (!m_currentCallToken.IsEmpty()) {
    SetStatusText(IDS_HANGINGUP);
    SetCallButton(false, IDS_HANGUP);
    command.m_type = OpalCmdClearCall;
    command.m_param.m_callCleared.m_callToken = m_currentCallToken;
    response = OpalSendMessage(m_opal, &command);
  }
  else if (!m_callAddress.IsEmpty()) {
    SetStatusText(IDS_CALLING);
    SetCallButton(true, IDS_HANGUP);
    CStringA utfAddress((const TCHAR *)m_callAddress);
    command.m_type = OpalCmdSetUpCall;
    command.m_param.m_callSetUp.m_partyB = utfAddress;
    response = OpalSendMessage(m_opal, &command);
  }

  if (response == NULL)
    ErrorBox(IDS_CALL_START_FAIL);
  else {
    if (response->m_type == OpalIndCommandError) {
      ErrorBox(IDS_CALL_START_FAIL, response);
      SetCallButton(true, IDS_CALL);
    }
    else if (command.m_type == OpalCmdSetUpCall) {
      m_currentCallToken = response->m_param.m_callSetUp.m_callToken;
      AddRecentCall(m_callAddress);
    }
    else if (command.m_type == OpalCmdAnswerCall) {
      m_currentCallToken = m_incomingCallToken;
      m_incomingCallToken.Empty();
    }

    OpalFreeMessage(response);
  }
}


void CMobileOpalDlg::OnChangedAddress()
{
  SetCallButton(m_ctrlCallAddress.GetWindowTextLength() > 0);
}


void CMobileOpalDlg::OnSelectedAddress()
{
  SetCallButton(true);
}


void CMobileOpalDlg::OnMenuOptionsGeneral()
{
  COptionsGeneral dlg;
  dlg.m_strUsername = GetOptionString(UserNameKey);
  dlg.m_strDisplayName = GetOptionString(DisplayNameKey);
  dlg.m_strStunServer = GetOptionString(STUNServerKey);
  dlg.m_interfaceAddress = GetOptionString(InterfaceAddressKey, L"*");

  if (dlg.DoModal() == IDOK) {
    SetOptionString(UserNameKey, dlg.m_strUsername);
    SetOptionString(DisplayNameKey, dlg.m_strDisplayName);
    SetOptionString(STUNServerKey, dlg.m_strStunServer);
    SetOptionString(InterfaceAddressKey, dlg.m_interfaceAddress);
    InitialiseOPAL();
  }
}


void CMobileOpalDlg::OnMenuOptionsH323()
{
  COptionsH323 dlg;
  dlg.m_uiGatekeeperType = GetOptionInt(GkTypeKey);
  dlg.m_strGkId = GetOptionString(GkIdKey);
  dlg.m_strGkHost = GetOptionString(GkHostKey);
  dlg.m_strAlias = GetOptionString(GkAliasKey);
  dlg.m_strAuthUser = GetOptionString(GkAuthUserKey);
  dlg.m_strPassword = GetOptionString(GkPasswordKey);
  if (dlg.DoModal() == IDOK) {
    SetOptionInt(GkTypeKey, dlg.m_uiGatekeeperType);
    SetOptionString(GkIdKey, dlg.m_strGkId);
    SetOptionString(GkHostKey, dlg.m_strGkHost);
    SetOptionString(GkAliasKey, dlg.m_strAlias);
    SetOptionString(GkAuthUserKey, dlg.m_strAuthUser);
    SetOptionString(GkPasswordKey, dlg.m_strPassword);
    InitialiseOPAL();
  }
}


void CMobileOpalDlg::OnMenuOptionsSIP()
{
  COptionsSIP dlg;
  dlg.m_strAddressOfRecord = GetOptionString(RegistrarAorKey);
  dlg.m_strHostName = GetOptionString(RegistrarHostKey);
  dlg.m_strAuthUser = GetOptionString(RegistrarUserKey);
  dlg.m_strPassword = GetOptionString(RegistrarPassKey);
  dlg.m_strRealm = GetOptionString(RegistrarRealmKey);
  if (dlg.DoModal() == IDOK) {
    SetOptionString(RegistrarAorKey, dlg.m_strAddressOfRecord);
    SetOptionString(RegistrarHostKey, dlg.m_strHostName);
    SetOptionString(RegistrarUserKey, dlg.m_strAuthUser);
    SetOptionString(RegistrarPassKey, dlg.m_strPassword);
    SetOptionString(RegistrarRealmKey, dlg.m_strRealm);
    InitialiseOPAL();
  }
}


void CMobileOpalDlg::OnSpeakerphone()
{
  m_speakerphone = !m_speakerphone;
  SetSpeakerMode(m_speakerphone);

  HMENU hMenu = (HMENU)::SendMessage(m_dlgCommandBar.m_hCommandBar, SHCMBM_GETSUBMENU, 0, IDS_OPTIONS);
  CheckMenuItem(hMenu, IDM_SPEAKERPHONE, m_speakerphone ? MF_CHECKED : MF_UNCHECKED);
}
