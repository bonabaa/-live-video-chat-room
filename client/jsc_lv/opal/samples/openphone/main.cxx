/*
 * main.cpp
 *
 * An OPAL GUI phone application.
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *    
 * Copyright (c) 2007 Vox Lucida
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
 * The Original Code is Open Phone client.
 *
 * The Initial Developer of the Original Code is Robert Jongbloed
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 21283 $
 * $Author: rjongbloed $
 * $Date: 2008-10-11 07:10:58 +0000 (Sat, 11 Oct 2008) $
 */

//#ifdef __GNUG__
//#pragma implementation
//#pragma interface
//#endif

#include "main.h"

#include "version.h"

#include <wx/config.h>
#include <wx/tokenzr.h>
#include <wx/clipbrd.h>
#include <wx/accel.h>
#include <wx/filesys.h>
#include <wx/gdicmn.h>     //Required for icons on linux. 
#include <wx/image.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/spinctrl.h>
#include <wx/splitter.h>
#include <wx/valgen.h>
#include <wx/notebook.h>
#undef LoadMenu // Bizarre but necessary before the xml code
#include <wx/xrc/xmlres.h>

#include <opal/transcoders.h>
#include <opal/ivr.h>
#include <lids/lidep.h>
#include <ptclib/pstun.h>
#include <t38/t38proto.h>
#include <codec/vidcodec.h>


#if defined(__WXGTK__)   || \
    defined(__WXMOTIF__) || \
    defined(__WXX11__)   || \
    defined(__WXMAC__)   || \
    defined(__WXMGL__)   || \
    defined(__WXCOCOA__)
#include "openphone.xpm"
#include "h323phone.xpm"
#include "sipphone.xpm"
#include "otherphone.xpm"
#include "smallphone.xpm"

#define USE_SDL 1

#else

#define USE_SDL 0

#endif


extern void InitXmlResource(); // From resource.cpp whichis compiled openphone.xrc

// Definitions of the configuration file section and key names

#define DEF_FIELD(name) static const wxChar name##Key[] = wxT(#name)

static const wxChar AppearanceGroup[] = wxT("/Appearance");
DEF_FIELD(MainFrameX);
DEF_FIELD(MainFrameY);
DEF_FIELD(MainFrameWidth);
DEF_FIELD(MainFrameHeight);
DEF_FIELD(SashPosition);
DEF_FIELD(ActiveView);

static const wxChar GeneralGroup[] = wxT("/General");
DEF_FIELD(Username);
DEF_FIELD(DisplayName);
DEF_FIELD(RingSoundDeviceName);
DEF_FIELD(RingSoundFileName);
DEF_FIELD(AutoAnswer);
DEF_FIELD(IVRScript);
DEF_FIELD(SpeakerVolume);
DEF_FIELD(MicrophoneVolume);
DEF_FIELD(SpeakerMute);
DEF_FIELD(MicrophoneMute);
DEF_FIELD(LastDialed);
DEF_FIELD(LastReceived);

static const wxChar NetworkingGroup[] = wxT("/Networking");
DEF_FIELD(Bandwidth);
DEF_FIELD(TCPPortBase);
DEF_FIELD(TCPPortMax);
DEF_FIELD(UDPPortBase);
DEF_FIELD(UDPPortMax);
DEF_FIELD(RTPPortBase);
DEF_FIELD(RTPPortMax);
DEF_FIELD(RTPTOS);
DEF_FIELD(NATHandling);
DEF_FIELD(NATRouter);
DEF_FIELD(STUNServer);

static const wxChar LocalInterfacesGroup[] = wxT("/Networking/Interfaces");

static const wxChar AudioGroup[] = wxT("/Audio");
DEF_FIELD(SoundPlayer);
DEF_FIELD(SoundRecorder);
DEF_FIELD(SoundBuffers);
DEF_FIELD(LineInterfaceDevice);
DEF_FIELD(AEC);
DEF_FIELD(Country);
DEF_FIELD(MinJitter);
DEF_FIELD(MaxJitter);
DEF_FIELD(SilenceSuppression);
DEF_FIELD(SilenceThreshold);
DEF_FIELD(SignalDeadband);
DEF_FIELD(SilenceDeadband);
DEF_FIELD(DisableDetectInBandDTMF);

static const wxChar VideoGroup[] = wxT("/Video");
DEF_FIELD(VideoGrabber);
DEF_FIELD(VideoGrabFormat);
DEF_FIELD(VideoGrabSource);
DEF_FIELD(VideoGrabFrameRate);
DEF_FIELD(VideoGrabFrameSize);
DEF_FIELD(VideoGrabPreview);
DEF_FIELD(VideoFlipLocal);
DEF_FIELD(VideoAutoTransmit);
DEF_FIELD(VideoAutoReceive);
DEF_FIELD(VideoFlipRemote);
DEF_FIELD(VideoMinFrameSize);
DEF_FIELD(VideoMaxFrameSize);
DEF_FIELD(LocalVideoFrameX);
DEF_FIELD(LocalVideoFrameY);
DEF_FIELD(RemoteVideoFrameX);
DEF_FIELD(RemoteVideoFrameY);

static const wxChar FaxGroup[] = wxT("/Fax");
DEF_FIELD(FaxStationIdentifier);
DEF_FIELD(FaxReceiveDirectory);
DEF_FIELD(FaxSpanDSP);

static const wxChar CodecsGroup[] = wxT("/Codecs");
static const wxChar CodecNameKey[] = wxT("Name");

static const wxChar SecurityGroup[] = wxT("/Security");
DEF_FIELD(SecureH323);
DEF_FIELD(SecureSIP);
DEF_FIELD(RTPSecurityModeH323);
DEF_FIELD(RTPSecurityModeSIP);

static const wxChar H323Group[] = wxT("/H.323");
DEF_FIELD(GatekeeperMode);
DEF_FIELD(GatekeeperAddress);
DEF_FIELD(GatekeeperIdentifier);
DEF_FIELD(GatekeeperTTL);
DEF_FIELD(GatekeeperLogin);
DEF_FIELD(GatekeeperPassword);
DEF_FIELD(DTMFSendMode);
DEF_FIELD(CallIntrusionProtectionLevel);
DEF_FIELD(DisableFastStart);
DEF_FIELD(DisableH245Tunneling);
DEF_FIELD(DisableH245inSETUP);

static const wxChar H323AliasesGroup[] = wxT("/H.323/Aliases");

static const wxChar SIPGroup[] = wxT("/SIP");
DEF_FIELD(SIPProxyUsed);
DEF_FIELD(SIPProxy);
DEF_FIELD(SIPProxyUsername);
DEF_FIELD(SIPProxyPassword);

static const wxChar RegistrarGroup[] = wxT("/SIP/Registrars");
DEF_FIELD(RegistrarUsed);
DEF_FIELD(RegistrarName);
DEF_FIELD(RegistrarDomain);
DEF_FIELD(RegistrarAuthID);
DEF_FIELD(RegistrarUsername);
DEF_FIELD(RegistrarPassword);
DEF_FIELD(RegistrarTimeToLive);
DEF_FIELD(SubscribeMWI);
DEF_FIELD(SubscribePresence);

static const wxChar RoutingGroup[] = wxT("/Routes");

#if PTRACING
static const wxChar TracingGroup[] = wxT("/Tracing");
DEF_FIELD(EnableTracing);
DEF_FIELD(TraceLevelThreshold);
DEF_FIELD(TraceLevelNumber);
DEF_FIELD(TraceFileLine);
DEF_FIELD(TraceBlocks);
DEF_FIELD(TraceDateTime);
DEF_FIELD(TraceTimestamp);
DEF_FIELD(TraceThreadName);
DEF_FIELD(TraceThreadAddress);
DEF_FIELD(TraceFileName);
DEF_FIELD(TraceOptions);
#endif

static const wxChar SpeedDialsGroup[] = wxT("/Speed Dials");
static const wxChar SpeedDialAddressKey[] = wxT("Address");
static const wxChar SpeedDialNumberKey[] = wxT("Number");
static const wxChar SpeedDialDescriptionKey[] = wxT("Description");

static const wxChar RecentCallsGroup[] = wxT("/Recent Calls");
static const size_t MaxSavedRecentCalls = 10;

static const wxChar SIPonly[] = wxT(" (SIP only)");
static const wxChar H323only[] = wxT(" (H.323 only)");

static const wxChar AllRouteSources[] = wxT("<ALL>");

static const char * const DefaultRoutes[] = {
#if OPAL_IVR
    ".*:#  = ivr:", // A hash from anywhere goes to IVR
#endif
    "pots:.*\\*.*\\*.* = sip:<dn2ip>",
    "pots:.*           = sip:<da>",
    "pc:.*             = sip:<da>",

#if OPAL_FAX
    "t38:.*            = sip:<da>",
#endif

    "h323:.*           = pots:<dn>",
    "h323:.*           = pc:<du>",

    "h323s:.*          = pots:<dn>",
    "h323s:.*          = pc:<du>",

    "sip:.*            = pots:<dn>",
    "sip:.*            = pc:<du>",

    "sips:.*           = pots:<dn>",
    "sips:.*           = pc:<du>"
};


enum {
  ID_RETRIEVE_MENU_BASE = wxID_HIGHEST+1,
  ID_RETRIEVE_MENU_TOP = ID_RETRIEVE_MENU_BASE+999,
  ID_TRANSFER_MENU_BASE,
  ID_TRANSFER_MENU_TOP = ID_TRANSFER_MENU_BASE+999,
  ID_AUDIO_CODEC_MENU_BASE,
  ID_AUDIO_CODEC_MENU_TOP = ID_AUDIO_CODEC_MENU_BASE+99,
  ID_VIDEO_CODEC_MENU_BASE,
  ID_VIDEO_CODEC_MENU_TOP = ID_VIDEO_CODEC_MENU_BASE+99,
  ID_LOG_MESSAGE,
  ID_STATE_CHANGE,
  ID_STREAMS_CHANGED,
};

DECLARE_EVENT_TYPE(wxEvtLogMessage, -1)
DEFINE_EVENT_TYPE(wxEvtLogMessage)

DECLARE_EVENT_TYPE(wxEvtStreamsChanged, -1)
DEFINE_EVENT_TYPE(wxEvtStreamsChanged)

DECLARE_EVENT_TYPE(wxEvtStateChange, -1)
DEFINE_EVENT_TYPE(wxEvtStateChange)


///////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4100)
#endif

template <class cls> cls * FindWindowByNameAs(wxWindow * window, const wxChar * name)
{
  wxWindow * baseChild = window->FindWindowByName(name);
  if (PAssert(baseChild != NULL, "Windows control not found")) {
    cls * derivedChild = dynamic_cast<cls *>(baseChild);
    if (PAssert(derivedChild != NULL, "Cannot cast window object to selected class"))
      return derivedChild;
  }

  return NULL;
}


void RemoveNotebookPage(wxWindow * window, const char * name)
{
  wxNotebook * book = FindWindowByNameAs<wxNotebook>(window, wxT("OptionsNotebook"));
  for (size_t i = 0; i < book->GetPageCount(); ++i) {
    if (PwxString(book->GetPageText(i)) == name) {
      book->DeletePage(i);
      break;
    }
  }
}

#ifdef _MSC_VER
#pragma warning(default:4100)
#endif


///////////////////////////////////////////////////////////////////////////////

class TextCtrlChannel : public PChannel
{
    PCLASSINFO(TextCtrlChannel, PChannel)
  public:
    TextCtrlChannel()
      : m_frame(NULL)
      { }

    virtual PBoolean Write(
      const void * buf, /// Pointer to a block of memory to write.
      PINDEX len        /// Number of bytes to write.
    ) {
      if (m_frame == NULL)
        return PFalse;

      wxCommandEvent theEvent(wxEvtLogMessage, ID_LOG_MESSAGE);
      theEvent.SetEventObject(m_frame);
      theEvent.SetString(wxString((const wxChar *)buf, (size_t)len));
      m_frame->GetEventHandler()->AddPendingEvent(theEvent);
      return PTrue;
    }

    void SetFrame(
      wxFrame * frame
    ) { m_frame = frame; }

    static TextCtrlChannel & Instance()
    {
      static TextCtrlChannel instance;
      return instance;
    }

  protected:
    wxFrame * m_frame;
};

#define LogWindow TextCtrlChannel::Instance()


///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_APP(OpenPhoneApp)

OpenPhoneApp::OpenPhoneApp()
  : PProcess(MANUFACTURER_TEXT, PRODUCT_NAME_TEXT,
             MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER)
{
  wxConfig::Set(new wxConfig(PwxString(GetName()), PwxString(GetManufacturer())));
}


void OpenPhoneApp::Main()
{
  // Dummy function
}

//////////////////////////////////

bool OpenPhoneApp::OnInit()
{
  // make sure various URL types are registered to this application
  {
    PString urlTypes("sip\nh323\nsips\nh323s");
    PProcess::HostSystemURLHandlerInfo::RegisterTypes(urlTypes, true);
  }

  // Create the main frame window
  MyManager * main = new MyManager();
  SetTopWindow(main);
  wxBeginBusyCursor();
  bool ok = main->Initialise();
  wxEndBusyCursor();
  return ok;
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(MyManager, wxFrame)
  EVT_CLOSE(MyManager::OnClose)

  EVT_MENU_OPEN(MyManager::OnAdjustMenus)

  EVT_MENU(XRCID("MenuQuit"),            MyManager::OnMenuQuit)
  EVT_MENU(XRCID("MenuAbout"),           MyManager::OnMenuAbout)
  EVT_MENU(XRCID("MenuCall"),            MyManager::OnMenuCall)
  EVT_MENU(XRCID("MenuCallLastDialed"),  MyManager::OnMenuCallLastDialed)
  EVT_MENU(XRCID("MenuCallLastReceived"),MyManager::OnMenuCallLastReceived)
  EVT_MENU(XRCID("CallSpeedDialAudio"),  MyManager::OnCallSpeedDialAudio)
  EVT_MENU(XRCID("CallSpeedDialHandset"),MyManager::OnCallSpeedDialHandset)
  EVT_MENU(XRCID("MenuSendFax"),         MyManager::OnSendFax)
  EVT_MENU(XRCID("SendFaxSpeedDial"),    MyManager::OnSendFaxSpeedDial)
  EVT_MENU(XRCID("MenuAnswer"),          MyManager::OnMenuAnswer)
  EVT_MENU(XRCID("MenuHangUp"),          MyManager::OnMenuHangUp)
  EVT_MENU(XRCID("NewSpeedDial"),        MyManager::OnNewSpeedDial)
  EVT_MENU(XRCID("ViewLarge"),           MyManager::OnViewLarge)
  EVT_MENU(XRCID("ViewSmall"),           MyManager::OnViewSmall)
  EVT_MENU(XRCID("ViewList"),            MyManager::OnViewList)
  EVT_MENU(XRCID("ViewDetails"),         MyManager::OnViewDetails)
  EVT_MENU(XRCID("EditSpeedDial"),       MyManager::OnEditSpeedDial)
  EVT_MENU(XRCID("MenuCut"),             MyManager::OnCutSpeedDial)
  EVT_MENU(XRCID("MenuCopy"),            MyManager::OnCopySpeedDial)
  EVT_MENU(XRCID("MenuPaste"),           MyManager::OnPasteSpeedDial)
  EVT_MENU(XRCID("MenuDelete"),          MyManager::OnDeleteSpeedDial)
  EVT_MENU(XRCID("MenuOptions"),         MyManager::OnOptions)
  EVT_MENU(XRCID("MenuHold"),            MyManager::OnRequestHold)
  EVT_MENU_RANGE(ID_RETRIEVE_MENU_BASE,ID_RETRIEVE_MENU_TOP, MyManager::OnRetrieve)
  EVT_MENU(XRCID("MenuTransfer"),        MyManager::OnTransfer)
  EVT_MENU_RANGE(ID_TRANSFER_MENU_BASE,ID_TRANSFER_MENU_TOP, MyManager::OnTransfer)
  EVT_MENU(XRCID("MenuStartRecording"),  MyManager::OnStartRecording)
  EVT_MENU(XRCID("MenuStopRecording"),   MyManager::OnStopRecording)
  EVT_MENU_RANGE(ID_AUDIO_CODEC_MENU_BASE, ID_AUDIO_CODEC_MENU_TOP, MyManager::OnNewCodec)
  EVT_MENU_RANGE(ID_VIDEO_CODEC_MENU_BASE, ID_VIDEO_CODEC_MENU_TOP, MyManager::OnNewCodec)
  EVT_MENU(XRCID("MenuStartVideo"),      MyManager::OnStartVideo)
  EVT_MENU(XRCID("MenuStopVideo"),       MyManager::OnStopVideo)
  EVT_MENU(XRCID("MenuSendVFU"),         MyManager::OnVFU)
  EVT_MENU(XRCID("MenuVideoControl"),    MyManager::OnVideoControl)

  EVT_SPLITTER_SASH_POS_CHANGED(SplitterID, MyManager::OnSashPositioned)
  EVT_LIST_ITEM_ACTIVATED(SpeedDialsID, MyManager::OnSpeedDialActivated)
  EVT_LIST_COL_END_DRAG(SpeedDialsID, MyManager::OnSpeedDialColumnResize)
  EVT_LIST_ITEM_RIGHT_CLICK(SpeedDialsID, MyManager::OnRightClick) 

  EVT_COMMAND(ID_LOG_MESSAGE, wxEvtLogMessage, MyManager::OnLogMessage)
  EVT_COMMAND(ID_STATE_CHANGE, wxEvtStateChange, MyManager::OnStateChange)
  EVT_COMMAND(ID_STREAMS_CHANGED, wxEvtStreamsChanged, MyManager::OnStreamsChanged)
END_EVENT_TABLE()

MyManager::MyManager()
  : wxFrame(NULL, -1, wxT("OpenPhone"), wxDefaultPosition, wxSize(640, 480))
  , m_AnswerMode(AnswerDetect)
  , m_speedDials(NULL)
  , pcssEP(NULL)
  , potsEP(NULL)
#if OPAL_H323
  , h323EP(NULL)
#endif
#if OPAL_SIP
  , sipEP(NULL)
#endif
#if OPAL_IVR
  , ivrEP(NULL)
#endif
  , m_autoAnswer(false)
  , m_VideoGrabPreview(true)
  , m_localVideoFrameX(INT_MIN)
  , m_localVideoFrameY(INT_MIN)
  , m_remoteVideoFrameX(INT_MIN)
  , m_remoteVideoFrameY(INT_MIN)
#if PTRACING
  , m_enableTracing(false)
#endif
  , m_callState(IdleState)
{
  // Give it an icon
  SetIcon(wxICON(AppIcon));

  // Make an image list containing large icons
  m_imageListNormal = new wxImageList(32, 32, true);

  // Order here is important!!
  m_imageListNormal->Add(wxICON(OtherPhone));
  m_imageListNormal->Add(wxICON(H323Phone));
  m_imageListNormal->Add(wxICON(SIPPhone));

  m_imageListSmall = new wxImageList(16, 16, true);
  m_imageListSmall->Add(wxICON(SmallPhone));

  m_RingSoundTimer.SetNotifier(PCREATE_NOTIFIER(OnRingSoundAgain));

  LogWindow.SetFrame(this);
}


MyManager::~MyManager()
{
  LogWindow.SetFrame(NULL);

  ShutDownEndpoints();

  wxMenuBar * menubar = GetMenuBar();
  SetMenuBar(NULL);
  delete menubar;

  delete m_imageListNormal;
  delete m_imageListSmall;

  delete wxXmlResource::Set(NULL);
}


bool MyManager::Initialise()
{
  wxImage::AddHandler(new wxGIFHandler);
  wxXmlResource::Get()->InitAllHandlers();
  InitXmlResource();

  // Make a menubar
  wxMenuBar * menubar;
  {
    PMEMORY_IGNORE_ALLOCATIONS_FOR_SCOPE;
    if ((menubar = wxXmlResource::Get()->LoadMenuBar(wxT("MenuBar"))) == NULL)
      return false;
    SetMenuBar(menubar);
  }

  wxAcceleratorEntry accelEntries[] = {
      wxAcceleratorEntry(wxACCEL_CTRL,  'E',         XRCID("EditSpeedDial")),
      wxAcceleratorEntry(wxACCEL_CTRL,  'X',         XRCID("MenuCut")),
      wxAcceleratorEntry(wxACCEL_CTRL,  'C',         XRCID("MenuCopy")),
      wxAcceleratorEntry(wxACCEL_CTRL,  'V',         XRCID("MenuPaste")),
      wxAcceleratorEntry(wxACCEL_NORMAL, WXK_DELETE, XRCID("MenuDelete"))
  };
  wxAcceleratorTable accel(PARRAYSIZE(accelEntries), accelEntries);
  SetAcceleratorTable(accel);

  wxConfigBase * config = wxConfig::Get();
  config->SetPath(PwxString(AppearanceGroup));

  wxPoint initalPosition = wxDefaultPosition;
  if (config->Read(MainFrameXKey, &initalPosition.x) && config->Read(MainFrameYKey, &initalPosition.y))
    Move(initalPosition);

  wxSize initialSize(512, 384);
  if (config->Read(MainFrameWidthKey, &initialSize.x) && config->Read(MainFrameHeightKey, &initialSize.y))
    SetSize(initialSize);

  // Make the content of the main window, speed dial and log panes inside a splitter
  m_splitter = new wxSplitterWindow(this, SplitterID, wxPoint(), initialSize, wxSP_3D);

  // Log window - gets informative text
  initialSize.y /= 2;
  m_logWindow = new wxTextCtrl(m_splitter, -1, wxEmptyString, wxPoint(), wxSize(512, 128), wxTE_MULTILINE | wxTE_DONTWRAP | wxSUNKEN_BORDER);
  m_logWindow->SetForegroundColour(wxColour(0,255,0)); // Green
  m_logWindow->SetBackgroundColour(wxColour(0,0,0)); // Black

  // Speed dial window - icons for each speed dial
  int i;
  if (!config->Read(ActiveViewKey, &i) || i < 0 || i >= e_NumViews)
    i = e_ViewList;
  static const wxChar * const ViewMenuNames[e_NumViews] = {
    wxT("ViewLarge"), wxT("ViewSmall"), wxT("ViewList"), wxT("ViewDetails")
  };
  menubar->Check(wxXmlResource::GetXRCID(ViewMenuNames[i]), true);
  RecreateSpeedDials((SpeedDialViews)i);

  // Speed dial panel switches to answer panel on ring
  m_answerPanel = new AnswerPanel(*this, m_splitter);
  m_answerPanel->Show(false);

  // Speed dial panel switches to calling panel on dial
  m_callingPanel = new CallingPanel(*this, m_splitter);
  m_callingPanel->Show(false);

  // Speed dial/Answer/Calling panel switches to "in call" panel on successful call establishment
  m_inCallPanel = new InCallPanel(*this, m_splitter);
  m_inCallPanel->Show(false);

  // Set up sizer to automatically resize the splitter to size of window
  wxBoxSizer * sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(m_splitter, 1, wxGROW, 0);
  SetSizer(sizer);

  // Show the frame window
  Show(PTrue);

  LogWindow << PProcess::Current().GetName()
            << " Version " << PProcess::Current().GetVersion(TRUE)
            << " by " << PProcess::Current().GetManufacturer()
            << " on " << PProcess::Current().GetOSClass() << ' ' << PProcess::Current().GetOSName()
            << " (" << PProcess::Current().GetOSVersion() << '-' << PProcess::Current().GetOSHardware() << ')' << endl;

  m_ClipboardFormat.SetId(wxT("OpenPhone Speed Dial"));

#if PTRACING
  ////////////////////////////////////////
  // Tracing fields
  config->SetPath(TracingGroup);
  if (config->Read(EnableTracingKey, &m_enableTracing, false) && m_enableTracing &&
      config->Read(TraceFileNameKey, &m_traceFileName) && !m_traceFileName.empty()) {
    int traceLevelThreshold = 3;
    config->Read(TraceLevelThresholdKey, &traceLevelThreshold);
    int traceOptions = PTrace::DateAndTime|PTrace::Thread|PTrace::FileAndLine;
    config->Read(TraceOptionsKey, &traceOptions);
    PTrace::Initialise(traceLevelThreshold, PString(m_traceFileName), traceOptions);
  }
#endif

  ////////////////////////////////////////
  // Creating the endpoints
#if OPAL_H323
  h323EP = new MyH323EndPoint(*this);
#endif

#if OPAL_SIP
  sipEP = new MySIPEndPoint(*this);
#endif

#if OPAL_IVR
  ivrEP = new OpalIVREndPoint(*this);
#endif

#if OPAL_FAX
  m_faxEP = new OpalT38EndPoint(*this);
#endif

  potsEP = new OpalLineEndPoint(*this);
  pcssEP = new MyPCSSEndPoint(*this);

  PwxString str;
  bool onoff;
  int value1 = 0, value2 = 0;

  ////////////////////////////////////////
  // General fields
  config->SetPath(GeneralGroup);
  if (config->Read(UsernameKey, &str) && !str.IsEmpty())
    SetDefaultUserName(str);
  if (config->Read(DisplayNameKey, &str) && !str.IsEmpty())
    SetDefaultDisplayName(str);

  if (!config->Read(RingSoundDeviceNameKey, &m_RingSoundDeviceName))
    m_RingSoundDeviceName = PSoundChannel::GetDefaultDevice(PSoundChannel::Player);
  config->Read(RingSoundFileNameKey, &m_RingSoundFileName);

  config->Read(AutoAnswerKey, &m_autoAnswer);
#if OPAL_IVR
  if (config->Read(IVRScriptKey, &str))
    ivrEP->SetDefaultVXML(str);
#endif

  ////////////////////////////////////////
  // Networking fields
  PIPSocket::InterfaceTable interfaceTable;
  if (PIPSocket::GetInterfaceTable(interfaceTable))
    LogWindow << "Detected " << interfaceTable.GetSize() << " network interfaces:\n"
              << setfill('\n') << interfaceTable << setfill(' ') << flush;

  config->SetPath(NetworkingGroup);
#if OPAL_H323
  if (config->Read(BandwidthKey, &value1))
    h323EP->SetInitialBandwidth(value1);
#endif
  if (config->Read(TCPPortBaseKey, &value1) && config->Read(TCPPortMaxKey, &value2))
    SetTCPPorts(value1, value2);
  if (config->Read(UDPPortBaseKey, &value1) && config->Read(UDPPortMaxKey, &value2))
    SetUDPPorts(value1, value2);
  if (config->Read(RTPPortBaseKey, &value1) && config->Read(RTPPortMaxKey, &value2))
    SetRtpIpPorts(value1, value2);
  if (config->Read(RTPTOSKey, &value1))
    SetRtpIpTypeofService(value1);
  config->Read(NATRouterKey, &m_NATRouter);
  config->Read(STUNServerKey, &m_STUNServer);
  if (!config->Read(NATHandlingKey, &m_NATHandling))
    m_NATHandling = m_STUNServer.IsEmpty() ? (m_NATRouter.IsEmpty() ? 0 : 1) : 2;

  SetNATHandling();

  config->SetPath(LocalInterfacesGroup);
  wxString entryName;
  long entryIndex;
  if (config->GetFirstEntry(entryName, entryIndex)) {
    do {
      wxString localInterface;
      if (config->Read(entryName, &localInterface) && !localInterface.empty())
        m_LocalInterfaces.AppendString(localInterface.c_str());
    } while (config->GetNextEntry(entryName, entryIndex));
  }

  StartAllListeners();

  ////////////////////////////////////////
  // Sound fields
  config->SetPath(AudioGroup);
  if (config->Read(SoundPlayerKey, &str))
    pcssEP->SetSoundChannelPlayDevice(str);
  if (config->Read(SoundRecorderKey, &str))
    pcssEP->SetSoundChannelRecordDevice(str);
  if (config->Read(SoundBuffersKey, &value1))
    pcssEP->SetSoundChannelBufferDepth(value1);

  if (config->Read(MinJitterKey, &value1)) {
    config->Read(MaxJitterKey, &value2, value1);
    SetAudioJitterDelay(value1, value2);
  }

  OpalSilenceDetector::Params silenceParams = GetSilenceDetectParams();
  if (config->Read(SilenceSuppressionKey, &value1) && value1 >= 0 && value1 < OpalSilenceDetector::NumModes)
    silenceParams.m_mode = (OpalSilenceDetector::Mode)value1;
  if (config->Read(SilenceThresholdKey, &value1))
    silenceParams.m_threshold = value1;
  if (config->Read(SignalDeadbandKey, &value1))
    silenceParams.m_signalDeadband = value1*8;
  if (config->Read(SilenceDeadbandKey, &value1))
    silenceParams.m_silenceDeadband = value1*8;
  SetSilenceDetectParams(silenceParams);

  if (config->Read(DisableDetectInBandDTMFKey, &onoff))
    DisableDetectInBandDTMF(onoff);

  StartLID();


  ////////////////////////////////////////
  // Video fields
  config->SetPath(VideoGroup);
  PVideoDevice::OpenArgs videoArgs = GetVideoInputDevice();
  if (config->Read(VideoGrabberKey, &str))
    videoArgs.deviceName = (PString)PString(str.c_str());
  if (config->Read(VideoGrabFormatKey, &value1) && value1 >= 0 && value1 < PVideoDevice::NumVideoFormats)
    videoArgs.videoFormat = (PVideoDevice::VideoFormat)value1;
  if (config->Read(VideoGrabSourceKey, &value1))
    videoArgs.channelNumber = value1;
  if (config->Read(VideoGrabFrameRateKey, &value1))
    videoArgs.rate = value1;
  config->Read(VideoFlipLocalKey, &videoArgs.flip);
  SetVideoInputDevice(videoArgs);

  config->Read(VideoGrabFrameSizeKey, &m_VideoGrabFrameSize,  wxT("CIF"));
  config->Read(VideoMinFrameSizeKey,  &m_VideoMinFrameSize, wxT("SQCIF"));
  config->Read(VideoMaxFrameSizeKey,  &m_VideoMaxFrameSize,   wxT("CIF16"));

  config->Read(VideoGrabPreviewKey, &m_VideoGrabPreview);
  if (config->Read(VideoAutoTransmitKey, &onoff))
    SetAutoStartTransmitVideo(onoff);
  if (config->Read(VideoAutoReceiveKey, &onoff))
    SetAutoStartReceiveVideo(onoff);

  videoArgs = GetVideoPreviewDevice();
#if USE_SDL
  videoArgs.driverName = "SDL";
#else
  videoArgs.driverName = "Window";
#endif
  SetVideoPreviewDevice(videoArgs);

  videoArgs = GetVideoOutputDevice();
#if USE_SDL
  videoArgs.driverName = "SDL";
#else
  videoArgs.driverName = "Window";
#endif
  config->Read(VideoFlipRemoteKey, &videoArgs.flip);
  SetVideoOutputDevice(videoArgs);

  config->Read(LocalVideoFrameXKey, &m_localVideoFrameX);
  config->Read(LocalVideoFrameYKey, &m_localVideoFrameY);
  config->Read(RemoteVideoFrameXKey, &m_remoteVideoFrameX);
  config->Read(RemoteVideoFrameYKey, &m_remoteVideoFrameY);

  ////////////////////////////////////////
  // Fax fields
#if OPAL_FAX
  config->SetPath(FaxGroup);
  if (config->Read(FaxStationIdentifierKey, &str))
    m_faxEP->SetDefaultDisplayName(str);
  if (config->Read(FaxReceiveDirectoryKey, &str))
    m_faxEP->SetDefaultDirectory(str.c_str());
  if (config->Read(FaxSpanDSPKey, &str))
    m_faxEP->SetSpanDSP(str.c_str());
#endif

  ////////////////////////////////////////
  // Codec fields
  InitMediaInfo(PwxString(pcssEP->GetPrefixName()), pcssEP->GetMediaFormats());
  InitMediaInfo(PwxString(potsEP->GetPrefixName()), potsEP->GetMediaFormats());
#if OPAL_IVR
  InitMediaInfo(PwxString(ivrEP->GetPrefixName()), ivrEP->GetMediaFormats());
#endif

  OpalMediaFormatList mediaFormats;
  mediaFormats += pcssEP->GetMediaFormats();
  mediaFormats += potsEP->GetMediaFormats();
#if OPAL_IVR
  mediaFormats += ivrEP->GetMediaFormats();
#endif
#if OPAL_FAX
  mediaFormats += m_faxEP->GetMediaFormats();
#endif
  InitMediaInfo(wxT("sw"), OpalTranscoder::GetPossibleFormats(mediaFormats));

  config->SetPath(CodecsGroup);
  int codecIndex = 0;
  for (;;) {
    wxString groupName;
    groupName.sprintf(wxT("%04u"), codecIndex);
    if (!config->HasGroup(groupName))
      break;

    config->SetPath(groupName);
    PwxString codecName;
    if (config->Read(CodecNameKey, &codecName) && !codecName.empty()) {
      for (MyMediaList::iterator mm = m_mediaInfo.begin(); mm != m_mediaInfo.end(); ++mm) {
        if (codecName == mm->mediaFormat) {
          mm->preferenceOrder = codecIndex;
          bool changedSomething = false;
          for (PINDEX i = 0; i < mm->mediaFormat.GetOptionCount(); i++) {
            const OpalMediaOption & option = mm->mediaFormat.GetOption(i);
            if (!option.IsReadOnly()) {
              PwxString codecOptionName = option.GetName();
              PwxString codecOptionValue;
              PString oldOptionValue;
              mm->mediaFormat.GetOptionValue(codecOptionName, oldOptionValue);
              if (config->Read(codecOptionName, &codecOptionValue) &&
                              !codecOptionValue.empty() && codecOptionValue != oldOptionValue) {
                if (mm->mediaFormat.SetOptionValue(codecOptionName, codecOptionValue))
                  changedSomething = true;
              }
            }
          }
          if (changedSomething)
            OpalMediaFormat::SetRegisteredMediaFormat(mm->mediaFormat);
        }
      }
    }
    config->SetPath(wxT(".."));
    codecIndex++;
  }

  if (codecIndex > 0)
    ApplyMediaInfo();
  else {
    PStringArray mediaFormatOrder = GetMediaFormatOrder();
    for (PINDEX i = 0; i < mediaFormatOrder.GetSize(); i++) {
      for (MyMediaList::iterator mm = m_mediaInfo.begin(); mm != m_mediaInfo.end(); ++mm) {
        if (mm->mediaFormat == mediaFormatOrder[i])
          mm->preferenceOrder = codecIndex++;
      }
    }
    for (MyMediaList::iterator mm = m_mediaInfo.begin(); mm != m_mediaInfo.end(); ++mm) {
      if (mm->preferenceOrder < 0)
        mm->preferenceOrder = codecIndex++;
    }
    m_mediaInfo.sort();
  }
  AdjustFrameSize();

#if PTRACING
  mediaFormats = OpalMediaFormat::GetAllRegisteredMediaFormats();
  ostream & traceStream = PTrace::Begin(3, __FILE__, __LINE__);
  traceStream << "OpenPhone\tRegistered media formats:\n";
  for (PINDEX i = 0; i < mediaFormats.GetSize(); i++)
    mediaFormats[i].PrintOptions(traceStream);
  traceStream << PTrace::End;
#endif

  ////////////////////////////////////////
  // Security fields
  config->SetPath(SecurityGroup);
  if (config->Read(SecureH323Key, &onoff) && !onoff)
    DetachEndPoint("h323s");
  if (config->Read(SecureSIPKey, &onoff) && !onoff)
    DetachEndPoint("sips");
#if OPAL_H323
  if (config->Read(RTPSecurityModeH323Key, &str) && str != "None")
    h323EP->SetDefaultSecurityMode(str);
#endif
#if OPAL_SIP
  if (config->Read(RTPSecurityModeSIPKey, &str) && str != "None")
    sipEP->SetDefaultSecurityMode(str);
#endif

  PwxString username, password;

#if OPAL_H323
  ////////////////////////////////////////
  // H.323 fields
  config->SetPath(H323AliasesGroup);
  if (config->GetFirstEntry(entryName, entryIndex)) {
    do {
      wxString alias;
      if (config->Read(entryName, &alias) && !alias.empty())
        h323EP->AddAliasName(alias.c_str());
    } while (config->GetNextEntry(entryName, entryIndex));
  }

  config->SetPath(H323Group);

  if (config->Read(DTMFSendModeKey, &value1) && value1 >= 0 && value1 < H323Connection::NumSendUserInputModes)
    h323EP->SetSendUserInputMode((H323Connection::SendUserInputModes)value1);
#if OPAL_450
  if (config->Read(CallIntrusionProtectionLevelKey, &value1))
    h323EP->SetCallIntrusionProtectionLevel(value1);
#endif
  if (config->Read(DisableFastStartKey, &onoff))
    h323EP->DisableFastStart(onoff);
  if (config->Read(DisableH245TunnelingKey, &onoff))
    h323EP->DisableH245Tunneling(onoff);
  if (config->Read(DisableH245inSETUPKey, &onoff))
    h323EP->DisableH245inSetup(onoff);

  config->Read(GatekeeperModeKey, &m_gatekeeperMode, 0);
  if (m_gatekeeperMode > 0) {
    if (config->Read(GatekeeperTTLKey, &value1))
      h323EP->SetGatekeeperTimeToLive(PTimeInterval(0, value1));

    config->Read(GatekeeperLoginKey, &username, wxT(""));
    config->Read(GatekeeperPasswordKey, &password, wxT(""));
    h323EP->SetGatekeeperPassword(password, username);

    config->Read(GatekeeperAddressKey, &m_gatekeeperAddress, wxT(""));
    config->Read(GatekeeperIdentifierKey, &m_gatekeeperIdentifier, wxT(""));
    if (!StartGatekeeper())
      return false;
  }
#endif

#if OPAL_SIP
  ////////////////////////////////////////
  // SIP fields
  config->SetPath(SIPGroup);
  const SIPURL & proxy = sipEP->GetProxy();
  PwxString hostname;
  config->Read(SIPProxyUsedKey, &m_SIPProxyUsed, false);
  config->Read(SIPProxyKey, &hostname, PwxString(proxy.GetHostName()));
  config->Read(SIPProxyUsernameKey, &username, PwxString(proxy.GetUserName()));
  config->Read(SIPProxyPasswordKey, &password, PwxString(proxy.GetPassword()));
  if (m_SIPProxyUsed)
    sipEP->SetProxy(hostname, username, password);

  if (config->Read(RegistrarTimeToLiveKey, &value1))
    sipEP->SetRegistrarTimeToLive(PTimeInterval(0, value1));

  // Original backward compatibility entry
  RegistrarInfo registrar;
  if (config->Read(RegistrarUsedKey, &registrar.m_Active, false) &&
      config->Read(RegistrarNameKey, &registrar.m_Domain) &&
      config->Read(RegistrarUsernameKey, &registrar.m_User) &&
      config->Read(RegistrarPasswordKey, &registrar.m_Password))
    m_registrars.push_back(registrar);

  config->SetPath(RegistrarGroup);
  wxString groupName;
  long groupIndex;
  if (config->GetFirstGroup(groupName, groupIndex)) {
    do {
      config->SetPath(groupName);
      if (config->Read(RegistrarUsedKey, &registrar.m_Active, false) &&
          config->Read(RegistrarUsernameKey, &registrar.m_User) &&
          config->Read(RegistrarDomainKey, &registrar.m_Domain)) {
        config->Read(RegistrarAuthIDKey, &registrar.m_AuthID);
        config->Read(RegistrarPasswordKey, &registrar.m_Password);
        config->Read(RegistrarTimeToLiveKey, &registrar.m_TimeToLive);
        config->Read(SubscribeMWIKey, &registrar.m_MWI, true);
        config->Read(SubscribePresenceKey, &registrar.m_Presence, true);
        m_registrars.push_back(registrar);
      }
      config->SetPath(wxT(".."));
    } while (config->GetNextGroup(groupName, groupIndex));
  }

  StartRegistrars();
#endif // OPAL_SIP


  ////////////////////////////////////////
  // Routing fields
  config->SetPath(RoutingGroup);
  if (config->GetFirstEntry(entryName, entryIndex)) {
    do {
      wxString routeSpec;
      if (config->Read(entryName, &routeSpec))
        AddRouteEntry(routeSpec.c_str());
    } while (config->GetNextEntry(entryName, entryIndex));
  }
  else {
    for (PINDEX i = 0; i < PARRAYSIZE(DefaultRoutes); i++)
      AddRouteEntry(DefaultRoutes[i]);
  }

  return true;
}


void MyManager::StartLID()
{
  wxConfigBase * config = wxConfig::Get();

  PwxString device;
  if (!config->Read(LineInterfaceDeviceKey, &device) ||
      device.IsEmpty() || (device.StartsWith(wxT("<<")) && device.EndsWith(wxT(">>"))))
    return;

  if (!potsEP->AddDeviceName(device)) {
    LogWindow << "Line Interface Device \"" << device << "\" has been unplugged!" << endl;
    return;
  }

  OpalLine * line = potsEP->GetLine("*");
  if (PAssertNULL(line) == NULL)
    return;

  int aec;
  if (config->Read(AECKey, &aec) && aec >= 0 && aec < OpalLineInterfaceDevice::AECError)
    line->SetAEC((OpalLineInterfaceDevice::AECLevels)aec);

  PwxString country;
  if (config->Read(CountryKey, &country) && !country.IsEmpty()) {
    if (line->GetDevice().SetCountryCodeName(country))
      LogWindow << "Using Line Interface Device \"" << line->GetDevice().GetDescription() << '"' << endl;
    else
      LogWindow << "Could not configure Line Interface Device to country \"" << country << '"' << endl;
  }
}


void MyManager::SetNATHandling()
{
  switch (m_NATHandling) {
    case 1 :
      if (!m_NATRouter.IsEmpty())
        SetTranslationHost(m_NATRouter);
      SetSTUNServer(PString::Empty());
      break;
      
    case 2 :
      if (!m_STUNServer.IsEmpty()) {
        LogWindow << "STUN server \"" << m_STUNServer << "\" being contacted ..." << endl;
        GetEventHandler()->ProcessPendingEvents();
        Update();

        PSTUNClient::NatTypes nat = SetSTUNServer(m_STUNServer);

        LogWindow << "STUN server \"" << stun->GetServer() << "\" replies " << nat;
        PIPSocket::Address externalAddress;
        if (nat != PSTUNClient::BlockedNat && GetSTUNClient()->GetExternalAddress(externalAddress))
          LogWindow << " with address " << externalAddress;
        LogWindow << endl;
      }
      SetTranslationHost(PString::Empty());
      break;

    default :
      SetTranslationHost(PString::Empty());
      SetSTUNServer(PString::Empty());
  }
}


static void StartListenerForEP(OpalEndPoint * ep, const PStringArray & allInterfaces)
{
  if (ep == NULL)
    return;

  PStringArray interfacesForEP;
  PString prefixAndColon = ep->GetPrefixName() + ':';

  for (PINDEX i = 0; i < allInterfaces.GetSize(); i++) {
    PCaselessString iface = allInterfaces[i];
    if (iface.NumCompare("all:", 4) == PObject::EqualTo)
      interfacesForEP += iface.Mid(4);
    else if (iface.NumCompare(prefixAndColon) == PObject::EqualTo)
      interfacesForEP.AppendString(iface.Mid(prefixAndColon.GetLength()));
  }

  ep->RemoveListener(NULL);
  if (ep->StartListeners(interfacesForEP))
    LogWindow << ep->GetPrefixName().ToUpper() << " listening on " << setfill(',') << ep->GetListeners() << setfill(' ') << endl;
  else {
    LogWindow << ep->GetPrefixName().ToUpper() << " listen failed";
    if (!interfacesForEP.IsEmpty())
      LogWindow << " with interfaces" << setfill(',') << interfacesForEP << setfill(' ');
    LogWindow << endl;
  }
}


void MyManager::StartAllListeners()
{
#if OPAL_H323
  StartListenerForEP(h323EP, m_LocalInterfaces);
#endif
#if OPAL_SIP
  StartListenerForEP(sipEP, m_LocalInterfaces);
#endif
}


void MyManager::RecreateSpeedDials(SpeedDialViews view)
{
  wxConfigBase * config = wxConfig::Get();
  config->SetPath(AppearanceGroup);

  config->Write(ActiveViewKey, view);

  wxListCtrl * oldSpeedDials = m_speedDials;

  static DWORD const ListCtrlStyle[e_NumViews] = {
    wxLC_ICON, wxLC_SMALL_ICON, wxLC_LIST, wxLC_REPORT
  };

  m_speedDials = new wxListCtrl(m_splitter, SpeedDialsID,
                               wxDefaultPosition, wxSize(512, 128),
                               ListCtrlStyle[view] | wxLC_EDIT_LABELS | wxSUNKEN_BORDER);

  if (view != e_ViewDetails) {
    m_speedDials->SetImageList(m_imageListNormal, wxIMAGE_LIST_NORMAL);
    m_speedDials->SetImageList(m_imageListSmall, wxIMAGE_LIST_SMALL);
  }

  int width;
  static const wxChar * const titles[e_NumColumns] = { wxT("Name"), wxT("Number"), wxT("Address"), wxT("Description") };

  for (int i = 0; i < e_NumColumns; i++) {
    m_speedDials->InsertColumn(i, titles[i]);
    wxString key;
    key.sprintf(wxT("ColumnWidth%u"), i);
    if (config->Read(key, &width))
      m_speedDials->SetColumnWidth(i, width);
  }

  // Now either replace the top half of the splitter or set it for the first time
  if (oldSpeedDials == NULL) {
    width = 0;
    config->Read(SashPositionKey, &width);
    m_splitter->SplitHorizontally(m_speedDials, m_logWindow, width);
  }
  else {
    m_splitter->ReplaceWindow(oldSpeedDials, m_speedDials);
    delete oldSpeedDials;
  }

  // Read the speed dials from the configuration
  config->SetPath(SpeedDialsGroup);
  wxString groupName;
  long groupIndex;
  if (config->GetFirstGroup(groupName, groupIndex)) {
    do {
      config->SetPath(groupName);
      wxString number, address, description;
      if (config->Read(SpeedDialAddressKey, &address) && !address.empty()) {
        int icon = 0;
        if (view == e_ViewLarge) {
          if (address.StartsWith(wxT("h323")))
            icon = 1;
          else if (address.StartsWith(wxT("sip")))
            icon = 2;
        }

        int pos = m_speedDials->InsertItem(INT_MAX, groupName, icon);
        m_speedDials->SetItem(pos, e_NumberColumn, config->Read(SpeedDialNumberKey, wxT("")));
        m_speedDials->SetItem(pos, e_AddressColumn, address);
        m_speedDials->SetItem(pos, e_DescriptionColumn, config->Read(SpeedDialDescriptionKey, wxT("")));
      }
      config->SetPath(wxT(".."));
    } while (config->GetNextGroup(groupName, groupIndex));
  }
}


void MyManager::OnClose(wxCloseEvent& /*event*/)
{
  ::wxBeginBusyCursor();

  wxConfigBase * config = wxConfig::Get();
  config->SetPath(AppearanceGroup);

  int x, y;
  GetPosition(&x, &y);
  config->Write(MainFrameXKey, x);
  config->Write(MainFrameYKey, y);

  int w, h;
  GetSize(&w, &h);
  config->Write(MainFrameWidthKey, w);
  config->Write(MainFrameHeightKey, h);

  potsEP = NULL;
  m_activeCall.SetNULL();
  m_callsOnHold.clear();
  ShutDownEndpoints();

  Destroy();
}


void MyManager::OnLogMessage(wxCommandEvent & theEvent)
{
    m_logWindow->WriteText(theEvent.GetString());
}


bool MyManager::CanDoFax() const
{
#if OPAL_FAX
  return m_callState != InCallState &&
         GetMediaFormatMask().GetValuesIndex(OpalT38.GetName()) == P_MAX_INDEX &&
         PFile::Exists(m_faxEP->GetSpanDSP());
#else
  return false;
#endif
}


void MyManager::OnAdjustMenus(wxMenuEvent& WXUNUSED(event))
{
  wxMenuBar * menubar = GetMenuBar();
  menubar->Enable(XRCID("MenuCall"),            m_callState == IdleState);
  menubar->Enable(XRCID("MenuCallLastDialed"),  m_callState == IdleState && !m_LastDialed.IsEmpty());
  menubar->Enable(XRCID("MenuCallLastReceived"),m_callState == IdleState && !m_LastReceived.IsEmpty());
  menubar->Enable(XRCID("MenuAnswer"),          m_callState == RingingState);
  menubar->Enable(XRCID("MenuHangUp"),          m_callState == InCallState);
  menubar->Enable(XRCID("MenuHold"),            m_callState == InCallState);
  menubar->Enable(XRCID("MenuTransfer"),        m_callState == InCallState);
  menubar->Enable(XRCID("MenuStartRecording"),  m_callState == InCallState && !m_activeCall->IsRecording());
  menubar->Enable(XRCID("MenuStopRecording"),   m_callState == InCallState &&  m_activeCall->IsRecording());

  menubar->Enable(XRCID("MenuSendFax"),         CanDoFax());

  for (list<CallsOnHold>::iterator it = m_callsOnHold.begin(); it != m_callsOnHold.end(); ++it) {
    menubar->Enable(it->m_retrieveMenuId, m_callState != InCallState);
    menubar->Enable(it->m_transferMenuId, m_callState == InCallState);
  }

  int count = m_speedDials->GetSelectedItemCount();
  menubar->Enable(XRCID("MenuCut"),       count >= 1);
  menubar->Enable(XRCID("MenuCopy"),      count >= 1);
  menubar->Enable(XRCID("MenuDelete"),    count >= 1);
  menubar->Enable(XRCID("EditSpeedDial"), count == 1);

  bool hasFormat = false;
  if (wxTheClipboard->Open()) {
    hasFormat = wxTheClipboard->IsSupported(m_ClipboardFormat);
    wxTheClipboard->Close();
  }
  menubar->Enable(XRCID("MenuPaste"), hasFormat);

  bool hasStartVideo = false;
  bool hasStopVideo = false;
  bool hasRxVideo = false;
  wxString audioFormat, videoFormat;

  PSafePtr<OpalConnection> connection = GetConnection(false, PSafeReadOnly);
  if (connection != NULL) {
    // Get ID of open audio to check the menu item
    OpalMediaStreamPtr stream = connection->GetMediaStream(OpalMediaType::Audio(), true);
    if (stream != NULL)
      audioFormat = PwxString(stream->GetMediaFormat());

    stream = connection->GetMediaStream(OpalMediaType::Video(), false);
    hasStopVideo = stream != NULL && stream->Open();

    stream = connection->GetMediaStream(OpalMediaType::Video(), true);
    if (stream != NULL) {
      videoFormat = PwxString(stream->GetMediaFormat());
      hasRxVideo = stream->Open();
    }

    // Determine if video is startable, or is already started
    OpalMediaFormatList availableFormats = connection->GetMediaFormats();
    for (PINDEX idx = 0; idx < availableFormats.GetSize(); idx++) {
      if (availableFormats[idx].GetMediaType() == OpalMediaType::Video()) {
        hasStartVideo = !hasStopVideo;
        break;
      }
    }
  }

  menubar->Enable(XRCID("SubMenuAudio"), m_callState == InCallState);
  for (int id = ID_AUDIO_CODEC_MENU_BASE; id <= ID_AUDIO_CODEC_MENU_TOP; id++) {
    wxMenuItem * item = menubar->FindItem(id);
    if (item != NULL)
      item->Check(item->GetLabel() == audioFormat);
  }

  menubar->Enable(XRCID("SubMenuVideo"), !videoFormat.IsEmpty() && m_callState == InCallState);
  for (int id = ID_VIDEO_CODEC_MENU_BASE; id <= ID_VIDEO_CODEC_MENU_TOP; id++) {
    wxMenuItem * item = menubar->FindItem(id);
    if (item != NULL)
      item->Check(item->GetLabel() == videoFormat);
  }

  menubar->Enable(XRCID("MenuStartVideo"), hasStartVideo);
  menubar->Enable(XRCID("MenuStopVideo"), hasStopVideo);
  menubar->Enable(XRCID("MenuSendVFU"), hasRxVideo);
  menubar->Enable(XRCID("MenuVideoControl"), hasStopVideo);

  menubar->Enable(XRCID("SubMenuRetrieve"), !m_callsOnHold.empty());
}


void MyManager::OnMenuQuit(wxCommandEvent& WXUNUSED(event))
{
    Close(PTrue);
}


void MyManager::OnMenuAbout(wxCommandEvent& WXUNUSED(event))
{
  wxString text;
  text.sprintf(PRODUCT_NAME_TEXT " %s\n""\n"
               "Copyright © 2007-2008 " COPYRIGHT_HOLDER ", All rights reserved.\n"
               "\n"
               "This application may be used for any purpose so long as it is not sold "
               "or distributed for profit on it's own, or it's ownership by " COPYRIGHT_HOLDER
               " disguised or hidden in any way.\n"
               "\n"
               "Part of the Open Phone Abstraction Library, http://www.opalvoip.org\n",
               (const char *)PProcess::Current().GetVersion());
  wxMessageDialog dialog(this, text, wxT("About " PRODUCT_NAME_TEXT), wxOK);
  dialog.ShowModal();
}


void MyManager::OnMenuCall(wxCommandEvent& WXUNUSED(event))
{
  CallDialog dlg(this, false);
  if (dlg.ShowModal() == wxID_OK)
    MakeCall(dlg.m_Address, dlg.m_UseHandset ? "pots:*" : "pc:*");
}


void MyManager::OnMenuCallLastDialed(wxCommandEvent& WXUNUSED(event))
{
  MakeCall(m_LastDialed);
}


void MyManager::OnMenuCallLastReceived(wxCommandEvent& WXUNUSED(event))
{
  MakeCall(m_LastReceived);
}


void MyManager::OnCallSpeedDialAudio(wxCommandEvent & /*event*/)
{
  wxListItem item;
  item.m_itemId = m_speedDials->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  if (item.m_itemId < 0)
    return;

  item.m_col = e_AddressColumn;
  item.m_mask = wxLIST_MASK_TEXT;
  if (m_speedDials->GetItem(item))
    MakeCall(item.m_text);
}


void MyManager::OnCallSpeedDialHandset(wxCommandEvent & /*event*/)
{
  wxListItem item;
  item.m_itemId = m_speedDials->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  if (item.m_itemId < 0)
    return;

  item.m_col = e_AddressColumn;
  item.m_mask = wxLIST_MASK_TEXT;
  if (m_speedDials->GetItem(item))
    MakeCall(item.m_text, "pots:*");
}


void MyManager::OnSendFax(wxCommandEvent & /*event*/)
{
  wxFileDialog faxDlg(this,
                      wxT("Send FAX file"),
                      wxEmptyString,
                      wxEmptyString,
                      wxT("*.tif"));
  if (faxDlg.ShowModal() == wxID_OK) {
    CallDialog callDlg(this, true);
    if (callDlg.ShowModal() == wxID_OK)
      MakeCall(callDlg.m_Address, wxString(wxT("t38:")) + faxDlg.GetPath());
  }
}


void MyManager::OnSendFaxSpeedDial(wxCommandEvent & /*event*/)
{
  wxListItem item;
  item.m_itemId = m_speedDials->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  if (item.m_itemId < 0)
    return;

  item.m_col = e_AddressColumn;
  item.m_mask = wxLIST_MASK_TEXT;
  if (m_speedDials->GetItem(item)) {
    wxFileDialog faxDlg(this,
                        wxT("Send FAX file"),
                        wxEmptyString,
                        wxEmptyString,
                        wxT("*.tif"));
    if (faxDlg.ShowModal() == wxID_OK)
      MakeCall(item.m_text, wxString(wxT("t38:")) + faxDlg.GetPath());
  }
}


void MyManager::OnMenuAnswer(wxCommandEvent& WXUNUSED(event))
{
  AnswerCall();
}


void MyManager::OnMenuHangUp(wxCommandEvent& WXUNUSED(event))
{
  HangUpCall();
}


static wxString MakeUniqueSpeedDialName(wxListCtrl * speedDials, const wxChar * baseName)
{
  wxString name(baseName);
  unsigned tieBreaker = 0;
  int count = speedDials->GetItemCount();
  int i = 0;
  while (i < count) {
    if (speedDials->GetItemText(i).CmpNoCase(name) != 0)
      i++;
    else {
      name.Printf(wxT("%s (%u)"), baseName, ++tieBreaker);
      i = 0;
    }
  }
  return name;
}


void MyManager::OnNewSpeedDial(wxCommandEvent& WXUNUSED(event))
{
  wxString groupName = MakeUniqueSpeedDialName(m_speedDials, wxT("New Speed Dial"));

  int pos = m_speedDials->InsertItem(INT_MAX, groupName);
  m_speedDials->SetItem(pos, e_NumberColumn, wxT(""));
  m_speedDials->SetItem(pos, e_AddressColumn, wxT(""));
  m_speedDials->SetItem(pos, e_DescriptionColumn, wxT(""));
  EditSpeedDial(pos, true);
}


void MyManager::OnViewLarge(wxCommandEvent& event)
{
  GetMenuBar()->Check(event.GetId(), true);
  RecreateSpeedDials(e_ViewLarge);
}


void MyManager::OnViewSmall(wxCommandEvent& event)
{
  GetMenuBar()->Check(event.GetId(), true);
  RecreateSpeedDials(e_ViewSmall);
}


void MyManager::OnViewList(wxCommandEvent& event)
{
  GetMenuBar()->Check(event.GetId(), true);
  RecreateSpeedDials(e_ViewList);
}


void MyManager::OnViewDetails(wxCommandEvent& event)
{
  GetMenuBar()->Check(event.GetId(), true);
  RecreateSpeedDials(e_ViewDetails);
}


void MyManager::OnEditSpeedDial(wxCommandEvent& WXUNUSED(event))
{
  EditSpeedDial(m_speedDials->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED), false);
}


void MyManager::OnCutSpeedDial(wxCommandEvent& event)
{
  OnCopySpeedDial(event);
  OnDeleteSpeedDial(event);
}


void MyManager::OnCopySpeedDial(wxCommandEvent& WXUNUSED(event))
{
  if (!wxTheClipboard->Open())
    return;

  wxString tabbedText;

  wxListItem item;
  item.m_itemId = -1;
  while ((item.m_itemId = m_speedDials->GetNextItem(item.m_itemId, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) >= 0) {
    item.m_mask = wxLIST_MASK_TEXT;
    item.m_col = e_NameColumn;
    if (m_speedDials->GetItem(item)) {
      tabbedText += item.m_text;
      tabbedText += '\t';

      item.m_col = e_NumberColumn;
      if (m_speedDials->GetItem(item))
        tabbedText += item.m_text;
      tabbedText += '\t';

      item.m_col = e_AddressColumn;
      if (m_speedDials->GetItem(item))
        tabbedText += item.m_text;
      tabbedText += '\t';

      item.m_col = e_DescriptionColumn;
      if (m_speedDials->GetItem(item))
        tabbedText += item.m_text;
      tabbedText += wxT("\r\n");
    }
  }

  // Want pure text so can copy to notepad or something, and our built
  // in format, even though the latter is exactly the same string. This
  // just guarantees the format of teh string, where just using CF_TEXT
  // coupld provide anything.
  wxDataObjectComposite * multiFormatData = new wxDataObjectComposite;
  wxTextDataObject * myFormatData = new wxTextDataObject(tabbedText);
  myFormatData->SetFormat(m_ClipboardFormat);
  multiFormatData->Add(myFormatData);
  multiFormatData->Add(new wxTextDataObject(tabbedText));
  wxTheClipboard->SetData(multiFormatData);
  wxTheClipboard->Close();
}


void MyManager::OnPasteSpeedDial(wxCommandEvent& WXUNUSED(event))
{
  if (wxTheClipboard->Open()) {
    if (wxTheClipboard->IsSupported(m_ClipboardFormat)) {
      wxTextDataObject myFormatData;
      myFormatData.SetFormat(m_ClipboardFormat);
      if (wxTheClipboard->GetData(myFormatData)) {
        wxConfigBase * config = wxConfig::Get();
        wxStringTokenizer tabbedLines(myFormatData.GetText(), wxT("\r\n"));
        while (tabbedLines.HasMoreTokens()) {
          wxStringTokenizer tabbedText(tabbedLines.GetNextToken(), wxT("\t"), wxTOKEN_RET_EMPTY_ALL);
          wxString name = MakeUniqueSpeedDialName(m_speedDials, tabbedText.GetNextToken());
          wxString number = tabbedText.GetNextToken();
          wxString address = tabbedText.GetNextToken();
          wxString description = tabbedText.GetNextToken();

          int pos = m_speedDials->InsertItem(INT_MAX, name);
          m_speedDials->SetItem(pos, e_NumberColumn, number);
          m_speedDials->SetItem(pos, e_AddressColumn, address);
          m_speedDials->SetItem(pos, e_DescriptionColumn, description);

          config->SetPath(SpeedDialsGroup);
          config->SetPath(name);
          config->Write(SpeedDialNumberKey, number);
          config->Write(SpeedDialAddressKey, address);
          config->Write(SpeedDialDescriptionKey, description);
        }
      }
    }
    wxTheClipboard->Close();
  }
}


void MyManager::OnDeleteSpeedDial(wxCommandEvent& WXUNUSED(event))
{
  int count = m_speedDials->GetSelectedItemCount();
  if (count == 0)
    return;

  wxString str;
  str.Printf(wxT("Delete %u item%s?"), count, count != 1 ? wxT("s") : wxT(""));
  wxMessageDialog dlg(this, str, wxT("OpenPhone Speed Dials"), wxYES_NO);
  if (dlg.ShowModal() != wxID_YES)
    return;

  wxListItem item;
  while ((item.m_itemId = m_speedDials->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) >= 0) {
    item.m_mask = wxLIST_MASK_TEXT;
    item.m_col = e_NameColumn;
    if (m_speedDials->GetItem(item)) {
      wxConfigBase * config = wxConfig::Get();
      config->SetPath(SpeedDialsGroup);
      config->DeleteGroup(item.m_text);
      m_speedDials->DeleteItem(item.m_itemId);
    }
  }
}


void MyManager::OnSashPositioned(wxSplitterEvent& event)
{
  wxConfigBase * config = wxConfig::Get();
  config->SetPath(AppearanceGroup);
  config->Write(SashPositionKey, event.GetSashPosition());
}


void MyManager::OnSpeedDialActivated(wxListEvent& event)
{
  wxListItem item;
  item.m_itemId = event.GetIndex();
  if (item.m_itemId < 0)
    return;

  item.m_col = e_AddressColumn;
  item.m_mask = wxLIST_MASK_TEXT;
  if (m_speedDials->GetItem(item))
    MakeCall(item.m_text);
}


void MyManager::OnSpeedDialColumnResize(wxListEvent& event)
{
  wxConfigBase * config = wxConfig::Get();
  config->SetPath(AppearanceGroup);
  wxString key;
  key.sprintf(wxT("ColumnWidth%u"), event.GetColumn());
  config->Write(key, m_speedDials->GetColumnWidth(event.GetColumn()));
}


void MyManager::OnRightClick(wxListEvent& event)
{
  wxMenuBar * menuBar = wxXmlResource::Get()->LoadMenuBar(wxT("SpeedDialMenu"));
  menuBar->Enable(XRCID("CallSpeedDialHandset"), HasHandset());
  menuBar->Enable(XRCID("SendFaxSpeedDial"),     CanDoFax());
  PopupMenu(menuBar->GetMenu(0), event.GetPoint());
  delete menuBar;
}


void MyManager::EditSpeedDial(int index, bool newItem)
{
  if (index < 0)
    return;

  wxListItem item;
  item.m_itemId = index;
  item.m_mask = wxLIST_MASK_TEXT;

  item.m_col = e_NameColumn;
  if (!m_speedDials->GetItem(item))
    return;

  // Should display a menu, but initially just allow editing
  SpeedDialDialog dlg(this);

  wxString originalName = dlg.m_Name = item.m_text;

  item.m_col = e_NumberColumn;
  if (m_speedDials->GetItem(item))
    dlg.m_Number = item.m_text;

  item.m_col = e_AddressColumn;
  if (m_speedDials->GetItem(item))
    dlg.m_Address = item.m_text;

  item.m_col = e_DescriptionColumn;
  if (m_speedDials->GetItem(item))
    dlg.m_Description = item.m_text;

  if (dlg.ShowModal() == wxID_CANCEL) {
    if (newItem)
      m_speedDials->DeleteItem(item);
    return;
  }

  item.m_col = e_NameColumn;
  item.m_text = dlg.m_Name;
  m_speedDials->SetItem(item);

  item.m_col = e_NumberColumn;
  item.m_text = dlg.m_Number;
  m_speedDials->SetItem(item);

  item.m_col = e_AddressColumn;
  item.m_text = dlg.m_Address;
  m_speedDials->SetItem(item);

  item.m_col = e_DescriptionColumn;
  item.m_text = dlg.m_Description;
  m_speedDials->SetItem(item);

  wxConfigBase * config = wxConfig::Get();
  config->SetPath(SpeedDialsGroup);
  config->DeleteGroup(originalName);
  config->SetPath(dlg.m_Name);
  config->Write(SpeedDialNumberKey, dlg.m_Number);
  config->Write(SpeedDialAddressKey, dlg.m_Address);
  config->Write(SpeedDialDescriptionKey, dlg.m_Description);
}


bool MyManager::HasSpeedDialName(const wxString & name) const
{
  return m_speedDials->FindItem(-1, name) >= 0;
}


bool MyManager::HasSpeedDialNumber(const wxString & number, const wxString & ignore) const
{
  int count = m_speedDials->GetItemCount();
  wxListItem item;
  item.m_mask = wxLIST_MASK_TEXT;
  item.m_col = e_NumberColumn;
  for (item.m_itemId = 0; item.m_itemId < count; item.m_itemId++) {
    if (m_speedDials->GetItem(item) && item.m_text == number && item.m_text != ignore)
      return true;
  }

  return false;
}


void MyManager::MakeCall(const PwxString & address, const PwxString & local)
{
  if (address.IsEmpty())
    return;

  m_LastDialed = address;
  wxConfigBase * config = wxConfig::Get();
  config->SetPath(GeneralGroup);
  config->Write(LastDialedKey, m_LastDialed);

  PwxString from = local;
  if (from.empty())
    from = "pc:*";

  PString token;
  if (SetUpCall(from, address, token)) {
    LogWindow << "Calling \"" << address << '"' << endl;
    m_activeCall = FindCallWithLock(token, PSafeReference);
    SetState(CallingState);
  }
  else {
    LogWindow << "Could not call \"" << address << '"' << endl;
    SetState(IdleState);
  }
}


void MyManager::AnswerCall()
{
  if (PAssert(m_callState == RingingState && !m_incomingToken.IsEmpty(), PLogicError)) {
    StopRingSound();

    // Must do this before AcceptIncomingConnection or InCallState arrives before AnsweringState!
    SetState(AnsweringState);

    pcssEP->AcceptIncomingConnection(m_incomingToken);
    m_incomingToken.MakeEmpty();
  }
}


void MyManager::RejectCall()
{
  if (PAssert(m_callState == RingingState && !m_incomingToken.IsEmpty(), PLogicError)) {
    StopRingSound();
    pcssEP->RejectIncomingConnection(m_incomingToken);
    m_incomingToken.MakeEmpty();
  }
}


void MyManager::HangUpCall()
{
  if (PAssert(m_callState != IdleState && m_callState != AnsweringState && m_activeCall != NULL, PLogicError)) {
    LogWindow << "Hanging up \"" << *m_activeCall << '"' << endl;
    m_activeCall->Clear();
  }
}


void MyManager::OnRinging(const OpalPCSSConnection & connection)
{
  m_incomingToken = connection.GetToken();

  PTime now;
  LogWindow << "\nIncoming call at " << now.AsString("w h:mma")
            << " from " << connection.GetRemotePartyName() << endl;

  m_LastReceived = connection.GetRemotePartyAddress();
  wxConfigBase * config = wxConfig::Get();
  config->SetPath(GeneralGroup);
  config->Write(LastReceivedKey, m_LastDialed);

  if (!m_autoAnswer && !m_RingSoundFileName.empty()) {
    m_RingSoundChannel.Open(m_RingSoundDeviceName, PSoundChannel::Player);
    m_RingSoundChannel.PlayFile(m_RingSoundFileName, PFalse);
    m_RingSoundTimer.RunContinuous(5000);
  }

  SetState(RingingState);
}


void MyManager::OnRingSoundAgain(PTimer &, INT)
{
  m_RingSoundChannel.PlayFile(m_RingSoundFileName, PFalse);
}


void MyManager::StopRingSound()
{
  m_RingSoundTimer.Stop();
  m_RingSoundChannel.Close();
}


PBoolean MyManager::OnIncomingConnection(OpalConnection & connection, unsigned options, OpalConnection::StringOptions * stringOptions)
{
  bool usingHandset = connection.GetEndPoint().GetPrefixName() == "pots";
  if (usingHandset)
    LogWindow << "Line interface device \"" << connection.GetRemotePartyName() << "\" has gone off hook." << endl;

  if (!OpalManager::OnIncomingConnection(connection, options, stringOptions))
    return false;

  if (usingHandset) {
    m_activeCall = &connection.GetCall();
    SetState(CallingState);
  }

  return true;
}


void MyManager::OnEstablishedCall(OpalCall & call)
{
  m_activeCall = &call;

  LogWindow << "Established call from " << call.GetPartyA() << " to " << call.GetPartyB() << endl;
  SetState(InCallState);

  if (m_AnswerMode == AnswerFax)
    SwitchToFax();
}


void MyManager::OnClearedCall(OpalCall & call)
{
  StopRingSound();

  PString name = call.GetPartyB().IsEmpty() ? call.GetPartyA() : call.GetPartyB();

  switch (call.GetCallEndReason()) {
    case OpalConnection::EndedByRemoteUser :
      LogWindow << '"' << name << "\" has cleared the call";
      break;
    case OpalConnection::EndedByCallerAbort :
      LogWindow << '"' << name << "\" has stopped calling";
      break;
    case OpalConnection::EndedByRefusal :
      LogWindow << '"' << name << "\" did not accept your call";
      break;
    case OpalConnection::EndedByNoAnswer :
      LogWindow << '"' << name << "\" did not answer your call";
      break;
    case OpalConnection::EndedByTransportFail :
      LogWindow << "Call with \"" << name << "\" ended abnormally";
      break;
    case OpalConnection::EndedByCapabilityExchange :
      LogWindow << "Could not find common codec with \"" << name << '"';
      break;
    case OpalConnection::EndedByNoAccept :
      LogWindow << "Did not accept incoming call from \"" << name << '"';
      break;
    case OpalConnection::EndedByAnswerDenied :
      LogWindow << "Refused incoming call from \"" << name << '"';
      break;
    case OpalConnection::EndedByNoUser :
      LogWindow << "Gatekeeper could find user \"" << name << '"';
      break;
    case OpalConnection::EndedByNoBandwidth :
      LogWindow << "Call to \"" << name << "\" aborted, insufficient bandwidth.";
      break;
    case OpalConnection::EndedByUnreachable :
      LogWindow << '"' << name << "\" could not be reached.";
      break;
    case OpalConnection::EndedByNoEndPoint :
      LogWindow << "No phone running for \"" << name << '"';
      break;
    case OpalConnection::EndedByHostOffline :
      LogWindow << '"' << name << "\" is not online.";
      break;
    case OpalConnection::EndedByConnectFail :
      LogWindow << "Transport error calling \"" << name << '"';
      break;
    default :
      LogWindow << "Call with \"" << name << "\" completed";
  }
  PTime now;
  LogWindow << ", on " << now.AsString("w h:mma") << ". Duration "
            << setprecision(0) << setw(5) << (now - call.GetStartTime())
            << "s." << endl;

  SetState(ClearingCallState, call.GetToken());
}


void MyManager::OnHold(OpalConnection & connection, bool fromRemote, bool onHold)
{
  OpalManager::OnHold(connection, fromRemote, onHold);

  if (fromRemote) {
    LogWindow << "Remote " << connection.GetRemotePartyName() << " has "
              << (onHold ? "put you on" : "released you from") << " hold." << endl;
    return;
  }

  LogWindow << "Remote " << connection.GetRemotePartyName() << " has been "
            << (onHold ? "put on" : "released from") << " hold." << endl;
  SetState(onHold ? IdleState : InCallState, connection.GetCall().GetToken());
}


static void LogMediaStream(const char * stopStart, const OpalMediaStream & stream, const PString & epPrefix)
{
  if (epPrefix == "pc" || epPrefix == "pots")
    return;

  OpalMediaFormat mediaFormat = stream.GetMediaFormat();
  LogWindow << stopStart << (stream.IsSource() ? " receiving " : " sending ") << mediaFormat;

  if (!stream.IsSource() && mediaFormat.GetMediaType() == OpalMediaType::Audio())
    LogWindow << " (" << mediaFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption())*mediaFormat.GetFrameTime()/mediaFormat.GetTimeUnits() << "ms)";

  LogWindow << (stream.IsSource() ? " from " : " to ")
            << epPrefix << " endpoint"
            << endl;
}


PBoolean MyManager::OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream)
{
  if (!OpalManager::OnOpenMediaStream(connection, stream))
    return false;

  LogMediaStream("Started", stream, connection.GetEndPoint().GetPrefixName());

  wxCommandEvent theEvent(wxEvtStreamsChanged, ID_STREAMS_CHANGED);
  theEvent.SetEventObject(this);
  GetEventHandler()->AddPendingEvent(theEvent);

  return true;
}


void MyManager::OnClosedMediaStream(const OpalMediaStream & stream)
{
  OpalManager::OnClosedMediaStream(stream);

  LogMediaStream("Stopped", stream, stream.GetConnection().GetEndPoint().GetPrefixName());

  wxCommandEvent theEvent(wxEvtStreamsChanged, ID_STREAMS_CHANGED);
  theEvent.SetEventObject(this);
  GetEventHandler()->AddPendingEvent(theEvent);

  if (PIsDescendant(&stream, OpalVideoMediaStream)) {
    PVideoOutputDevice * device = ((const OpalVideoMediaStream &)stream).GetVideoOutputDevice();
    if (device != NULL) {
      int x, y;
      if (device->GetPosition(x, y)) {
        wxConfigBase * config = wxConfig::Get();
        config->SetPath(VideoGroup);

        if (stream.IsSource()) {
          if (x != m_localVideoFrameX || y != m_localVideoFrameY) {
            config->Write(LocalVideoFrameXKey, m_localVideoFrameX = x);
            config->Write(LocalVideoFrameYKey, m_localVideoFrameY = y);
          }
        }
        else {
          if (x != m_remoteVideoFrameX || y != m_remoteVideoFrameY) {
            config->Write(RemoteVideoFrameXKey, m_remoteVideoFrameX = x);
            config->Write(RemoteVideoFrameYKey, m_remoteVideoFrameY = y);
          }
        }
      }
    }
  }
}


PSafePtr<OpalCall> MyManager::GetCall(PSafetyMode mode)
{
  if (m_activeCall == NULL)
    return NULL;

  PSafePtr<OpalCall> call = m_activeCall;
  return call.SetSafetyMode(mode) ? call : NULL;
}


PSafePtr<OpalConnection> MyManager::GetConnection(bool user, PSafetyMode mode)
{
  if (m_activeCall == NULL)
    return NULL;

  PSafePtr<OpalConnection> connection = m_activeCall->GetConnection(0, PSafeReference);
  while (connection != NULL && connection->IsNetworkConnection() == user)
    ++connection;

  return connection.SetSafetyMode(mode) ? connection : NULL;
}


bool MyManager::HasHandset() const
{
  return potsEP != NULL && potsEP->GetLine("*") != NULL;
}


MyManager::CallsOnHold::CallsOnHold(OpalCall & call)
  : m_call(&call, PSafeReference)
{
  static int lastMenuId = ID_RETRIEVE_MENU_BASE;
  m_retrieveMenuId = lastMenuId++;
  m_transferMenuId = m_retrieveMenuId + (ID_TRANSFER_MENU_BASE - ID_RETRIEVE_MENU_BASE);
}


void MyManager::AddCallOnHold(OpalCall & call)
{
  m_callsOnHold.push_back(call);

  PwxString otherParty = call.GetPartyA();

  wxMenuBar * menubar = GetMenuBar();
  wxMenuItem * item = PAssertNULL(menubar)->FindItem(XRCID("SubMenuRetrieve"));
  wxMenu * menu = PAssertNULL(item)->GetSubMenu();
  PAssertNULL(menu)->Append(m_callsOnHold.back().m_retrieveMenuId, otherParty);
  item = menu->FindItemByPosition(0);
  if (item->IsSeparator())
    menu->Delete(item);

  item = menubar->FindItem(XRCID("SubMenuTransfer"));
  menu = PAssertNULL(item)->GetSubMenu();
  PAssertNULL(menu)->Append(m_callsOnHold.back().m_transferMenuId, otherParty);
}


bool MyManager::RemoveCallOnHold(const PString & token)
{
  list<CallsOnHold>::iterator it = m_callsOnHold.begin();
  for (;;) {
    if (it == m_callsOnHold.end())
      return false;
    if (it->m_call->GetToken() == token)
      break;
    ++it;
  }

  wxMenuBar * menubar = GetMenuBar();
  wxMenu * menu;
  wxMenuItem * item = PAssertNULL(menubar)->FindItem(it->m_retrieveMenuId, &menu);
  if (PAssert(menu != NULL && item != NULL, PLogicError)) {
    if (m_callsOnHold.size() == 1)
      menu->AppendSeparator();
    menu->Delete(item);
  }

  item = menubar->FindItem(it->m_transferMenuId, &menu);
  if (PAssert(menu != NULL && item != NULL, PLogicError))
    menu->Delete(item);

  m_callsOnHold.erase(it);

  m_inCallPanel->OnHoldChanged(false);
  return true;
}


void MyManager::SendUserInput(char tone)
{
  PSafePtr<OpalConnection> connection = GetConnection(true, PSafeReadWrite);
  if (connection != NULL)
    connection->OnUserInputTone(tone, 100);
}


void MyManager::OnUserInputString(OpalConnection & connection, const PString & value)
{
  LogWindow << "User input \"" << value << "\" received from \"" << connection.GetRemotePartyName() << '"' << endl;
  OpalManager::OnUserInputString(connection, value);
}


void MyManager::OnUserInputTone(OpalConnection & connection, char tone, int duration)
{
  if (toupper(tone) == 'X' && m_AnswerMode == AnswerDetect)
    SwitchToFax();

  OpalManager::OnUserInputTone(connection, tone, duration);
}


void MyManager::SwitchToFax()
{
  if (m_activeCall == NULL)
    return; // Huh?

  if (!m_activeCall->IsNetworkOriginated())
    return; // We originated call

  if (m_activeCall->GetPartyB().NumCompare("t38") == EqualTo)
    return; // Already switched

  PSafePtr<OpalConnection> connection = m_activeCall->GetConnection(1);
  if (connection == NULL)
    return; // Huh? again!

  if (m_activeCall->Transfer(*connection, "t38:*;receive"))
    LogWindow << "Switching to T.38 fax mode." << endl;
  else
    LogWindow << "Could not switch to T.38 fax mode." << endl;
}


void MyManager::OnRequestHold(wxCommandEvent& /*event*/)
{
  PSafePtr<OpalCall> call = GetCall(PSafeReadWrite);
  if (call != NULL)
    call->Hold();
}


void MyManager::OnRetrieve(wxCommandEvent& theEvent)
{
  if (PAssert(m_activeCall == NULL, PLogicError)) {
    for (list<CallsOnHold>::iterator it = m_callsOnHold.begin(); it != m_callsOnHold.end(); ++it) {
      if (theEvent.GetId() == it->m_retrieveMenuId) {
        it->m_call->Retrieve();
        break;
      }
    }
  }
}


void MyManager::OnTransfer(wxCommandEvent& theEvent)
{
  if (PAssert(m_activeCall != NULL, PLogicError)) {
    for (list<CallsOnHold>::iterator it = m_callsOnHold.begin(); it != m_callsOnHold.end(); ++it) {
      if (theEvent.GetId() == it->m_transferMenuId) {
        PSafePtr<OpalConnection> connection = GetConnection(false, PSafeReference);
        if (connection != NULL)
          m_activeCall->Transfer(*connection, it->m_call->GetToken());
        return;
      }
    }

    CallDialog dlg(this, true);
    dlg.SetTitle(wxT("Transfer Call"));
    if (dlg.ShowModal() == wxID_OK) {
      PSafePtr<OpalConnection> connection = GetConnection(false, PSafeReference);
      if (connection != NULL)
        m_activeCall->Transfer(*connection, dlg.m_Address);
    }
  }
}


void MyManager::OnStartRecording(wxCommandEvent & /*event*/)
{
  wxFileDialog dlg(this,
                   wxT("Save call to file"),
                   wxEmptyString,
                   PwxString(m_lastRecordFile),
                   wxT("*.wav"),
                   wxFD_SAVE);
  if (dlg.ShowModal() == wxID_OK && m_activeCall != NULL) {
    m_lastRecordFile = PFilePath(dlg.GetPath().mb_str(wxConvUTF8));
    m_activeCall->StartRecording(m_lastRecordFile);
  }
}


void MyManager::OnStopRecording(wxCommandEvent & /*event*/)
{
  if (m_activeCall != NULL)
    m_activeCall->StopRecording();
}


void MyManager::OnNewCodec(wxCommandEvent& theEvent)
{
  OpalMediaFormat mediaFormat(GetMenuBar()->FindItem(theEvent.GetId())->GetLabel().mb_str(wxConvUTF8));
  if (mediaFormat.IsValid()) {
    PSafePtr<OpalConnection> connection = GetConnection(true, PSafeReadWrite);
    if (connection != NULL) {
      OpalMediaStreamPtr stream = connection->GetMediaStream(mediaFormat.GetMediaType(), true);
      if (!connection->GetCall().OpenSourceMediaStreams(*connection,
                                                        mediaFormat.GetMediaType(),
                                                        stream != NULL ? stream->GetSessionID() : 0,
                                                        mediaFormat))
        LogWindow << "Could not change codec to " << mediaFormat << endl;
    }
  }
}


void MyManager::OnStartVideo(wxCommandEvent & /*event*/)
{
  PSafePtr<OpalConnection> connection = GetConnection(true, PSafeReadWrite);
  if (connection != NULL) {
    OpalMediaStreamPtr stream = connection->GetMediaStream(OpalMediaType::Video(), true);
    if (stream == NULL) {
      if (!connection->GetCall().OpenSourceMediaStreams(*connection, OpalMediaType::Video()))
        LogWindow << "Could not open video to remote!" << endl;
    }
  }
}


void MyManager::OnStopVideo(wxCommandEvent & /*event*/)
{
  PSafePtr<OpalConnection> connection = GetConnection(true, PSafeReadWrite);
  if (connection != NULL) {
    OpalMediaStreamPtr stream = connection->GetMediaStream(OpalMediaType::Video(), true);
    if (stream != NULL)
      connection->CloseMediaStream(*stream);
  }
}


void MyManager::OnVFU(wxCommandEvent& /*event*/)
{
  PSafePtr<OpalConnection> connection = GetConnection(false, PSafeReadOnly);
  if (connection != NULL)  {
    OpalVideoUpdatePicture cmd;
    connection->OnMediaCommand(cmd, 0);
  }
}


void MyManager::OnVideoControl(wxCommandEvent& /*event*/)
{
  VideoControlDialog dlg(this);
  dlg.ShowModal();
}


PString MyManager::ReadUserInput(OpalConnection & connection,
                                 const char *,
                                 unsigned,
                                 unsigned firstDigitTimeout)
{
  // The usual behaviour is to read until a '#' or timeout and that yields the
  // entire destination address. However for this application we want to disable
  // the timeout and short circuit the need for '#' as the speed dial number is
  // always unique.

  PTRACE(3, "OpalPhone\tReadUserInput from " << connection);

  connection.PromptUserInput(PTrue);
  PString digit = connection.GetUserInput(firstDigitTimeout);
  connection.PromptUserInput(PFalse);

  if (digit == "#")
    return digit;

  PString input;
  while (!digit.IsEmpty()) {
    input += digit;

    int count = m_speedDials->GetItemCount();
    wxListItem item;
    item.m_mask = wxLIST_MASK_TEXT;
    item.m_col = e_NumberColumn;
    for (item.m_itemId = 0; item.m_itemId < count; item.m_itemId++) {
      if (!m_speedDials->GetItem(item))
        continue;

      size_t specialCharPos = item.m_text.find_first_of(wxT("-+"));
      if (specialCharPos == wxString::npos) {
        if (PwxString(input)  != item.m_text)
          continue;
      }
      else {
        if (digit != "#" || strncmp(item.m_text.mb_str(wxConvUTF8), input, specialCharPos) != 0)
          continue;
        if (item.m_text[specialCharPos] == '-')
          input.Delete(0, specialCharPos);    // Using '-' so strip the prefix off
        input.Delete(input.GetLength()-1, 1); // Also get rid of the '#' at the end
      }

      item.m_col = e_AddressColumn;
      if (!m_speedDials->GetItem(item))
        continue;

      PString address = item.m_text.c_str();
      address.Replace("<dn>", input);

      item.m_col = e_NameColumn;
      m_speedDials->GetItem(item);

      LogWindow << "Calling \"" << item.m_text << "\" using \"" << address << '"' << endl;
      return address;
    }

    digit = connection.GetUserInput(firstDigitTimeout);
  }

  PTRACE(2, "OpalPhone\tReadUserInput timeout (" << firstDigitTimeout << " seconds) on " << *this);
  return PString::Empty();
}


static const PVideoDevice::OpenArgs & AdjustVideoArgs(PVideoDevice::OpenArgs & videoArgs, const char * title, int x, int y)
{
#if USE_SDL
  videoArgs.deviceName = "SDL";
#else
  videoArgs.deviceName = psprintf("MSWIN STYLE=0x%08X", WS_POPUP|WS_BORDER|WS_SYSMENU|WS_CAPTION);
#endif

  videoArgs.deviceName.sprintf(" TITLE=\"%s\" X=%i Y=%i", title, x, y);

  return videoArgs;
}

PBoolean MyManager::CreateVideoOutputDevice(const OpalConnection & connection,
                                        const OpalMediaFormat & mediaFormat,
                                        PBoolean preview,
                                        PVideoOutputDevice * & device,
                                        PBoolean & autoDelete)
{
  if (preview && !m_VideoGrabPreview)
    return PFalse;

  if (m_localVideoFrameX == INT_MIN) {
    wxRect rect(GetPosition(), GetSize());
    m_localVideoFrameX = rect.GetLeft() + mediaFormat.GetOptionInteger(OpalVideoFormat::FrameWidthOption(), PVideoFrameInfo::QCIFWidth);
    m_localVideoFrameY = rect.GetBottom();
    m_remoteVideoFrameX = rect.GetLeft();
    m_remoteVideoFrameY = rect.GetBottom();
  }

  PVideoDevice::OpenArgs videoArgs;
  if (preview)
    SetVideoPreviewDevice(AdjustVideoArgs(videoArgs = GetVideoPreviewDevice(), "Local", m_localVideoFrameX, m_localVideoFrameY));
  else
    SetVideoOutputDevice(AdjustVideoArgs(videoArgs = GetVideoOutputDevice(), "Remote", m_remoteVideoFrameX, m_remoteVideoFrameY));

  return OpalManager::CreateVideoOutputDevice(connection, mediaFormat, preview, device, autoDelete);
}


ostream & operator<<(ostream & strm, MyManager::CallState state)
{
  static const char * const names[] = {
    "Idle", "Calling", "Ringing", "Answering", "InCall", "ClearingCall"
  };
  return strm << names[state];
}


void MyManager::SetState(CallState newState, const char * token)
{
  wxCommandEvent theEvent(wxEvtStateChange, ID_STATE_CHANGE);
  theEvent.SetEventObject(this);
  theEvent.SetInt(newState);
  if (token != NULL)
    theEvent.SetString(PwxString(token));
  GetEventHandler()->AddPendingEvent(theEvent);
}


void MyManager::OnStateChange(wxCommandEvent & theEvent)
{
  CallState newState = (CallState)theEvent.GetInt();

  if (m_callState == newState)
    return;

  PTRACE(3, "OpenPhone\tGUI state changed from " << m_callState << " to " << newState);

  wxWindow * newWindow;
  switch (newState) {
    case RingingState :
      if (!IsActive())
        RequestUserAttention();
      Raise();

      if (!m_autoAnswer) {
        // Want the network side connection to get calling and called party names.
        PSafePtr<OpalConnection> connection = pcssEP->GetConnectionWithLock(m_incomingToken, PSafeReadOnly);
        if (connection != NULL) {
          connection = connection->GetCall().GetConnection(0, PSafeReadOnly);
          if (connection != NULL)
            m_answerPanel->SetPartyNames(connection->GetRemotePartyURL(), connection->GetDestinationAddress());
        }
        newWindow = m_answerPanel;
        break;
      }

      pcssEP->AcceptIncomingConnection(m_incomingToken);
      m_incomingToken.MakeEmpty();

      newState = AnsweringState;
      // Do next state

    case AnsweringState :
    case CallingState :
      newWindow = m_callingPanel;
      break;

    case ClearingCallState :
      if (m_activeCall == NULL || PwxString(m_activeCall->GetToken()) != theEvent.GetString()) {
        // A call on hold got cleared
        if (RemoveCallOnHold(PwxString(theEvent.GetString())))
          return;
      }

      m_activeCall.SetNULL();
      newState = IdleState;
      newWindow = m_speedDials;
      break;

    case InCallState :
      if (m_activeCall == NULL) {
        // Retrieve call from hold
        RemoveCallOnHold(PwxString(theEvent.GetString()));
        m_activeCall = FindCallWithLock(theEvent.GetString().c_str(), PSafeReference);
      }
      newWindow = m_inCallPanel;
      break;

    case IdleState :
      if (m_activeCall != NULL) {
        AddCallOnHold(*m_activeCall);
        m_activeCall.SetNULL();
        m_inCallPanel->OnHoldChanged(true);
      }
      newWindow = m_speedDials;
      break;

    default :
      PAssertAlways(PLogicError);
      return;
  }

  m_callState = newState;

  m_speedDials->Hide();
  m_answerPanel->Hide();
  m_callingPanel->Hide();
  m_inCallPanel->Hide();
  newWindow->Show();

  m_splitter->ReplaceWindow(m_splitter->GetWindow1(), newWindow);
}


void MyManager::OnStreamsChanged(wxCommandEvent &)
{
  m_inCallPanel->OnStreamsChanged();
}


bool MyManager::StartGatekeeper()
{
#if OPAL_H323
  if (m_gatekeeperMode == 0)
    h323EP->RemoveGatekeeper();
  else {
    PString gkDesc = m_gatekeeperIdentifier;
    if (!m_gatekeeperIdentifier.IsEmpty() || !m_gatekeeperAddress.IsEmpty())
      gkDesc += "@";
    gkDesc += m_gatekeeperAddress.c_str();

    if (h323EP->UseGatekeeper(m_gatekeeperAddress, m_gatekeeperIdentifier)) {
      LogWindow << "H.323 registration started for " << *h323EP->GetGatekeeper() << endl;
      return true;
    }

    LogWindow << "H.323 registration failed for " << gkDesc << endl;
  }

  return m_gatekeeperMode < 2;
#else
   return false;
#endif
}


#if OPAL_SIP
void MyManager::StartRegistrars()
{
  if (sipEP == NULL)
    return;

  for (RegistrarList::iterator iter = m_registrars.begin(); iter != m_registrars.end(); ++iter) {
    if (iter->m_Active) {
      SIPRegister::Params param;
      param.m_addressOfRecord = PString(iter->m_User.c_str());
      if (param.m_addressOfRecord.Find('@') == P_MAX_INDEX) {
        param.m_addressOfRecord += '@';
        param.m_addressOfRecord += iter->m_Domain.mb_str(wxConvUTF8);
      }
      param.m_authID = iter->m_AuthID.mb_str(wxConvUTF8);
      if (param.m_authID.IsEmpty())
        param.m_authID = iter->m_User.mb_str(wxConvUTF8);
      param.m_password = iter->m_Password.mb_str(wxConvUTF8);
      param.m_expire = iter->m_TimeToLive;
      bool ok = sipEP->Register(param);
      LogWindow << "SIP registration " << (ok ? "start" : "fail") << "ed for " << iter->m_User << '@' << iter->m_Domain << endl;

      if (iter->m_MWI) {
        SIPSubscribe::Params mwiParam(SIPSubscribe::MessageSummary);
        mwiParam.m_targetAddress = param.m_addressOfRecord;
        mwiParam.m_authID = param.m_authID;
        mwiParam.m_password = param.m_password;
        mwiParam.m_expire = iter->m_TimeToLive;
        ok = sipEP->Subscribe(mwiParam);
        LogWindow << "SIP MWI subscribe " << (ok ? "start" : "fail") << "ed for " << iter->m_User << '@' << iter->m_Domain << endl;
      }

      if (iter->m_Presence) {
        SIPSubscribe::Params presenceParam(SIPSubscribe::Presence);
        presenceParam.m_targetAddress = param.m_addressOfRecord;
        presenceParam.m_authID = param.m_authID;
        presenceParam.m_password = param.m_password;
        presenceParam.m_expire = iter->m_TimeToLive;
        ok = sipEP->Subscribe(presenceParam);
        LogWindow << "SIP Presence subscribe " << (ok ? "start" : "fail") << "ed for " << iter->m_User << '@' << iter->m_Domain << endl;
      }
    }
  }
}


void MyManager::StopRegistrars()
{
  if (sipEP != NULL) {
    sipEP->UnregisterAll();
    sipEP->UnsubcribeAll(SIPSubscribe::MessageSummary);
    sipEP->UnsubcribeAll(SIPSubscribe::Presence);
  }
}
#endif // OPAL_SIP


bool MyManager::AdjustFrameSize()
{
  unsigned width, height;
  if (!PVideoFrameInfo::ParseSize(m_VideoGrabFrameSize, width, height)) {
    width = PVideoFrameInfo::CIFWidth;
    height = PVideoFrameInfo::CIFWidth;
  }

  unsigned minWidth, minHeight;
  if (!PVideoFrameInfo::ParseSize(m_VideoMinFrameSize, minWidth, minHeight)) {
    minWidth = PVideoFrameInfo::SQCIFWidth;
    minHeight = PVideoFrameInfo::SQCIFHeight;
  }

  unsigned maxWidth, maxHeight;
  if (!PVideoFrameInfo::ParseSize(m_VideoMaxFrameSize, maxWidth, maxHeight)) {
    maxWidth = PVideoFrameInfo::CIF16Width;
    maxHeight = PVideoFrameInfo::CIF16Height;
  }

  OpalMediaFormatList allMediaFormats;
  OpalMediaFormat::GetAllRegisteredMediaFormats(allMediaFormats);
  for (PINDEX i = 0; i < allMediaFormats.GetSize(); i++) {
    OpalMediaFormat mediaFormat = allMediaFormats[i];
    if (mediaFormat.GetMediaType() == OpalMediaType::Video()) {
      mediaFormat.SetOptionInteger(OpalVideoFormat::FrameWidthOption(), width);
      mediaFormat.SetOptionInteger(OpalVideoFormat::FrameHeightOption(), height);
      mediaFormat.SetOptionInteger(OpalVideoFormat::MinRxFrameWidthOption(), minWidth);
      mediaFormat.SetOptionInteger(OpalVideoFormat::MinRxFrameHeightOption(), minHeight);
      mediaFormat.SetOptionInteger(OpalVideoFormat::MaxRxFrameWidthOption(), maxWidth);
      mediaFormat.SetOptionInteger(OpalVideoFormat::MaxRxFrameHeightOption(), maxHeight);
      OpalMediaFormat::SetRegisteredMediaFormat(mediaFormat);
    }
  }

  return true;
}


void MyManager::InitMediaInfo(const wxChar * source, const OpalMediaFormatList & mediaFormats)
{
  for (PINDEX i = 0; i < mediaFormats.GetSize(); i++) {
    const OpalMediaFormat & mediaFormat = mediaFormats[i];
    if (mediaFormat.IsTransportable())
      m_mediaInfo.push_back(MyMedia(source, mediaFormat));
  }
}


void MyManager::ApplyMediaInfo()
{
  PStringList mediaFormatOrder, mediaFormatMask;

  m_mediaInfo.sort();

  wxMenuBar * menubar = GetMenuBar();
  wxMenuItem * item = PAssertNULL(menubar)->FindItem(XRCID("SubMenuAudio"));
  wxMenu * audioMenu = PAssertNULL(item)->GetSubMenu();
  while (audioMenu->GetMenuItemCount() > 0)
    audioMenu->Delete(audioMenu->FindItemByPosition(0));

  item = PAssertNULL(menubar)->FindItem(XRCID("SubMenuVideo"));
  wxMenu * videoMenu = PAssertNULL(item)->GetSubMenu();
  while (videoMenu->GetMenuItemCount() > 0)
    videoMenu->Delete(videoMenu->FindItemByPosition(0));

  for (MyMediaList::iterator mm = m_mediaInfo.begin(); mm != m_mediaInfo.end(); ++mm) {
    if (mm->preferenceOrder < 0)
      mediaFormatMask.AppendString(mm->mediaFormat);
    else {
      mediaFormatOrder.AppendString(mm->mediaFormat);
      if (mm->mediaFormat.GetMediaType() == OpalMediaType::Audio())
        audioMenu->Append(mm->preferenceOrder+ID_AUDIO_CODEC_MENU_BASE,
                          PwxString(mm->mediaFormat.GetName()),
                          wxEmptyString,
                          true);
      else if (mm->mediaFormat.GetMediaType() == OpalMediaType::Video())
        videoMenu->Append(mm->preferenceOrder+ID_VIDEO_CODEC_MENU_BASE,
                          PwxString(mm->mediaFormat.GetName()),
                          wxEmptyString,
                          true);
    }
  }

  if (!mediaFormatOrder.IsEmpty()) {
    PTRACE(3, "OpenPhone\tMedia order:\n"<< setfill('\n') << mediaFormatOrder << setfill(' '));
    SetMediaFormatOrder(mediaFormatOrder);
    PTRACE(3, "OpenPhone\tMedia mask:\n"<< setfill('\n') << mediaFormatMask << setfill(' '));
    SetMediaFormatMask(mediaFormatMask);
  }
}


///////////////////////////////////////////////////////////////////////////////

MyMedia::MyMedia()
  : sourceProtocol(NULL)
  , validProtocols(NULL)
  , preferenceOrder(-1) // -1 indicates disabled
  , dirty(false)
{
}


MyMedia::MyMedia(const wxChar * source, const PString & format)
  : sourceProtocol(source)
  , mediaFormat(format)
  , preferenceOrder(-1) // -1 indicates disabled
  , dirty(false)
{
  bool hasSIP = mediaFormat.IsValidForProtocol("sip");
  bool hasH323 = mediaFormat.IsValidForProtocol("h.323");
  if (hasSIP && !hasH323)
    validProtocols = SIPonly;
  else if (!hasSIP && hasH323)
    validProtocols = H323only;
  else
    validProtocols = NULL;
}


///////////////////////////////////////////////////////////////////////////////

class wxFrameSizeValidator: public wxGenericValidator
{
public:
  wxFrameSizeValidator(wxString* val)
    : wxGenericValidator(val)
  {
  }

  virtual wxObject *Clone() const
  {
    return new wxFrameSizeValidator(*this);
  }

  virtual bool Validate(wxWindow *)
  {
    unsigned width, height;
    if (PVideoFrameInfo::ParseSize(GetWindow()->GetLabel().c_str(), width, height))
      return true;

    wxMessageBox(wxT("Illegal value for video size."), wxT("Error"), wxCANCEL|wxICON_EXCLAMATION);
    return false;
  }
};

///////////////////////////////////////////////////////////////////////////////

void MyManager::OnOptions(wxCommandEvent& /*event*/)
{
  OptionsDialog dlg(this);
  dlg.ShowModal();
}


BEGIN_EVENT_TABLE(OptionsDialog, wxDialog)
  ////////////////////////////////////////
  // General fields
  EVT_BUTTON(XRCID("BrowseSoundFile"), OptionsDialog::BrowseSoundFile)
  EVT_BUTTON(XRCID("PlaySoundFile"), OptionsDialog::PlaySoundFile)

  ////////////////////////////////////////
  // Networking fields
  EVT_CHOICE(XRCID("BandwidthClass"), OptionsDialog::BandwidthClass)
  EVT_RADIOBUTTON(XRCID("NoNATUsed"), OptionsDialog::NATHandling)
  EVT_RADIOBUTTON(XRCID("UseNATRouter"), OptionsDialog::NATHandling)
  EVT_RADIOBUTTON(XRCID("UseSTUNServer"), OptionsDialog::NATHandling)
  EVT_LISTBOX(XRCID("LocalInterfaces"), OptionsDialog::SelectedLocalInterface)
  EVT_RADIOBOX(XRCID("InterfaceProtocol"), OptionsDialog::ChangedInterfaceInfo)
  EVT_TEXT(XRCID("InterfaceAddress"), OptionsDialog::ChangedInterfaceInfo)
  EVT_TEXT(XRCID("InterfacePort"), OptionsDialog::ChangedInterfaceInfo)
  EVT_BUTTON(XRCID("AddInterface"), OptionsDialog::AddInterface)
  EVT_BUTTON(XRCID("RemoveInterface"), OptionsDialog::RemoveInterface)

  ////////////////////////////////////////
  // Audio fields
  EVT_COMBOBOX(wxXmlResource::GetXRCID(LineInterfaceDeviceKey), OptionsDialog::SelectedLID)

  ////////////////////////////////////////
  // Fax fields
  EVT_BUTTON(XRCID("FaxBrowseReceiveDirectory"), OptionsDialog::BrowseFaxDirectory)
  EVT_BUTTON(XRCID("FaxBrowseSpanDSP"), OptionsDialog::BrowseFaxSpanDSP)

  ////////////////////////////////////////
  // Codec fields
  EVT_BUTTON(XRCID("AddCodec"), OptionsDialog::AddCodec)
  EVT_BUTTON(XRCID("RemoveCodec"), OptionsDialog::RemoveCodec)
  EVT_BUTTON(XRCID("MoveUpCodec"), OptionsDialog::MoveUpCodec)
  EVT_BUTTON(XRCID("MoveDownCodec"), OptionsDialog::MoveDownCodec)
  EVT_LISTBOX(XRCID("AllCodecs"), OptionsDialog::SelectedCodecToAdd)
  EVT_LISTBOX(XRCID("SelectedCodecs"), OptionsDialog::SelectedCodec)
  EVT_LIST_ITEM_SELECTED(XRCID("CodecOptionsList"), OptionsDialog::SelectedCodecOption)
  EVT_LIST_ITEM_DESELECTED(XRCID("CodecOptionsList"), OptionsDialog::DeselectedCodecOption)
  EVT_TEXT(XRCID("CodecOptionValue"), OptionsDialog::ChangedCodecOptionValue)

  ////////////////////////////////////////
  // SIP fields
  EVT_BUTTON(XRCID("AddRegistrar"), OptionsDialog::AddRegistrar)
  EVT_BUTTON(XRCID("ChangeRegistrar"), OptionsDialog::ChangeRegistrar)
  EVT_BUTTON(XRCID("RemoveRegistrar"), OptionsDialog::RemoveRegistrar)
  EVT_LIST_ITEM_SELECTED(XRCID("Registrars"), OptionsDialog::SelectedRegistrar)
  EVT_LIST_ITEM_DESELECTED(XRCID("Registrars"), OptionsDialog::DeselectedRegistrar)
  EVT_TEXT(XRCID("RegistrarDomain"), OptionsDialog::ChangedRegistrarInfo)
  EVT_TEXT(XRCID("RegistrarUsername"), OptionsDialog::ChangedRegistrarInfo)
  EVT_TEXT(XRCID("RegistrarPassword"), OptionsDialog::ChangedRegistrarInfo)
  EVT_TEXT(XRCID("RegistrarTimeToLive"), OptionsDialog::ChangedRegistrarInfo)
  EVT_BUTTON(XRCID("RegistrarUsed"), OptionsDialog::ChangedRegistrarInfo)

  ////////////////////////////////////////
  // Routing fields
  EVT_BUTTON(XRCID("AddRoute"), OptionsDialog::AddRoute)
  EVT_BUTTON(XRCID("RemoveRoute"), OptionsDialog::RemoveRoute)
  EVT_BUTTON(XRCID("MoveUpRoute"), OptionsDialog::MoveUpRoute)
  EVT_BUTTON(XRCID("MoveDownRoute"), OptionsDialog::MoveDownRoute)
  EVT_LIST_ITEM_SELECTED(XRCID("Routes"), OptionsDialog::SelectedRoute)
  EVT_LIST_ITEM_DESELECTED(XRCID("Routes"), OptionsDialog::DeselectedRoute)
  EVT_TEXT(XRCID("RouteDevice"), OptionsDialog::ChangedRouteInfo)
  EVT_TEXT(XRCID("RoutePattern"), OptionsDialog::ChangedRouteInfo)
  EVT_TEXT(XRCID("RouteDestination"), OptionsDialog::ChangedRouteInfo)
  EVT_BUTTON(XRCID("RestoreDefaultRoutes"), OptionsDialog::RestoreDefaultRoutes)

  ////////////////////////////////////////
  // H.323 fields
  EVT_LISTBOX(XRCID("Aliases"), OptionsDialog::SelectedAlias)
  EVT_BUTTON(XRCID("AddAlias"), OptionsDialog::AddAlias)
  EVT_BUTTON(XRCID("RemoveAlias"), OptionsDialog::RemoveAlias)
  EVT_TEXT(XRCID("NewAlias"), OptionsDialog::ChangedNewAlias)

  ////////////////////////////////////////
  // Tracing fields
#if PTRACING
  EVT_BUTTON(XRCID("BrowseTraceFile"), OptionsDialog::BrowseTraceFile)
#endif
END_EVENT_TABLE()


#define INIT_FIELD(name, value) \
  m_##name = value; \
  FindWindowByName(name##Key)->SetValidator(wxGenericValidator(&m_##name))

OptionsDialog::OptionsDialog(MyManager * manager)
  : m_manager(*manager)
{
  PINDEX i;

  SetExtraStyle(GetExtraStyle() | wxWS_EX_VALIDATE_RECURSIVELY);
  wxXmlResource::Get()->LoadDialog(this, manager, wxT("OptionsDialog"));

  ////////////////////////////////////////
  // General fields
  INIT_FIELD(Username, m_manager.GetDefaultUserName());
  INIT_FIELD(DisplayName, m_manager.GetDefaultDisplayName());

  PStringList devices = PSoundChannel::GetDeviceNames(PSoundChannel::Player);
  wxChoice * choice = FindWindowByNameAs<wxChoice>(this, RingSoundDeviceNameKey);
  choice->SetValidator(wxGenericValidator(&m_RingSoundDeviceName));
  for (i = 0; i < devices.GetSize(); i++) {
    PwxString str = devices[i];
    str.Replace(wxT("\t"), wxT(": "));
    choice->Append(str);
  }
  m_RingSoundDeviceName = m_manager.m_RingSoundDeviceName;
  m_RingSoundDeviceName.Replace(wxT("\t"), wxT(": "));
  INIT_FIELD(RingSoundFileName, m_manager.m_RingSoundFileName);

  INIT_FIELD(AutoAnswer, m_manager.m_autoAnswer);
#if OPAL_IVR
  INIT_FIELD(IVRScript, m_manager.ivrEP->GetDefaultVXML());
#endif

  ////////////////////////////////////////
  // Networking fields
#if OPAL_H323
  int bandwidth = m_manager.h323EP->GetInitialBandwidth();
  m_Bandwidth.sprintf(bandwidth%10 == 0 ? wxT("%u") : wxT("%u.%u"), bandwidth/10, bandwidth%10);
  FindWindowByName(BandwidthKey)->SetValidator(wxTextValidator(wxFILTER_NUMERIC, &m_Bandwidth));
  int bandwidthClass;
  if (bandwidth <= 144)
    bandwidthClass = 0;
  else if (bandwidth <= 288)
    bandwidthClass = 1;
  else if (bandwidth <= 640)
    bandwidthClass = 2;
  else if (bandwidth <= 1280)
    bandwidthClass = 3;
  else if (bandwidth <= 15000)
    bandwidthClass = 4;
  else
    bandwidthClass = 5;
  FindWindowByNameAs<wxChoice>(this, wxT("BandwidthClass"))->SetSelection(bandwidthClass);
#endif

  INIT_FIELD(TCPPortBase, m_manager.GetTCPPortBase());
  INIT_FIELD(TCPPortMax, m_manager.GetTCPPortMax());
  INIT_FIELD(UDPPortBase, m_manager.GetUDPPortBase());
  INIT_FIELD(UDPPortMax, m_manager.GetUDPPortMax());
  INIT_FIELD(RTPPortBase, m_manager.GetRtpIpPortBase());
  INIT_FIELD(RTPPortMax, m_manager.GetRtpIpPortMax());
  INIT_FIELD(RTPTOS, m_manager.GetRtpIpTypeofService());

  m_NoNATUsedRadio = FindWindowByNameAs<wxRadioButton>(this, wxT("NoNATUsed"));
  m_NATRouterRadio = FindWindowByNameAs<wxRadioButton>(this, wxT("UseNATRouter"));
  m_STUNServerRadio= FindWindowByNameAs<wxRadioButton>(this, wxT("UseSTUNServer"));
  m_NATRouter = m_manager.m_NATRouter;
  m_NATRouterWnd = FindWindowByNameAs<wxTextCtrl>(this, wxT("NATRouter"));
  m_NATRouterWnd->SetValidator(wxGenericValidator(&m_NATRouter));
  m_STUNServer = m_manager.m_STUNServer;
  m_STUNServerWnd = FindWindowByNameAs<wxTextCtrl>(this, wxT("STUNServer"));
  m_STUNServerWnd->SetValidator(wxGenericValidator(&m_STUNServer));
  switch (m_manager.m_NATHandling) {
    case 2 :
      m_STUNServerRadio->SetValue(true);
      m_NATRouterWnd->Disable();
      break;
    case 1 :
      m_NATRouterRadio->SetValue(true);
      m_STUNServerWnd->Disable();
      break;
    default :
      m_NoNATUsedRadio->SetValue(true);
      m_NATRouterWnd->Disable();
      m_STUNServerWnd->Disable();
  }

  m_AddInterface = FindWindowByNameAs<wxButton>(this, wxT("AddInterface"));
  m_AddInterface->Disable();
  m_RemoveInterface = FindWindowByNameAs<wxButton>(this, wxT("RemoveInterface"));
  m_RemoveInterface->Disable();
  m_InterfaceProtocol = FindWindowByNameAs<wxRadioBox>(this, wxT("InterfaceProtocol"));
  m_InterfacePort = FindWindowByNameAs<wxTextCtrl>(this, wxT("InterfacePort"));
  m_InterfaceAddress = FindWindowByNameAs<wxComboBox>(this, wxT("InterfaceAddress"));
  m_InterfaceAddress->Append(wxT("*"));
  PIPSocket::InterfaceTable ifaces;
  if (PIPSocket::GetInterfaceTable(ifaces)) {
    for (i = 0; i < ifaces.GetSize(); i++) {
      PwxString addr = ifaces[i].GetAddress().AsString();
      PwxString name = wxT("%");
      name += PwxString(ifaces[i].GetName());
      m_InterfaceAddress->Append(addr);
      m_InterfaceAddress->Append(name);
      m_InterfaceAddress->Append(addr + name);
    }
  }
  m_LocalInterfaces = FindWindowByNameAs<wxListBox>(this, wxT("LocalInterfaces"));
  for (i = 0; i < m_manager.m_LocalInterfaces.GetSize(); i++)
    m_LocalInterfaces->Append(PwxString(m_manager.m_LocalInterfaces[i]));

  ////////////////////////////////////////
  // Sound fields
  INIT_FIELD(SoundBuffers, m_manager.pcssEP->GetSoundChannelBufferDepth());
  INIT_FIELD(MinJitter, m_manager.GetMinAudioJitterDelay());
  INIT_FIELD(MaxJitter, m_manager.GetMaxAudioJitterDelay());
  INIT_FIELD(SilenceSuppression, m_manager.GetSilenceDetectParams().m_mode);
  INIT_FIELD(SilenceThreshold, m_manager.GetSilenceDetectParams().m_threshold);
  INIT_FIELD(SignalDeadband, m_manager.GetSilenceDetectParams().m_signalDeadband/8);
  INIT_FIELD(SilenceDeadband, m_manager.GetSilenceDetectParams().m_silenceDeadband/8);
  INIT_FIELD(DisableDetectInBandDTMF, m_manager.DetectInBandDTMFDisabled());

  // Fill sound player combo box with available devices and set selection
  wxComboBox * combo = FindWindowByNameAs<wxComboBox>(this, SoundPlayerKey);
  combo->SetValidator(wxGenericValidator(&m_SoundPlayer));
  for (i = 0; i < devices.GetSize(); i++) {
    PwxString str = devices[i];
    str.Replace(wxT("\t"), wxT(": "));
    combo->Append(str);
  }
  m_SoundPlayer = m_manager.pcssEP->GetSoundChannelPlayDevice();
  m_SoundPlayer.Replace(wxT("\t"), wxT(": "));

  // Fill sound recorder combo box with available devices and set selection
  combo = FindWindowByNameAs<wxComboBox>(this, SoundRecorderKey);
  combo->SetValidator(wxGenericValidator(&m_SoundRecorder));
  devices = PSoundChannel::GetDeviceNames(PSoundChannel::Recorder);
  for (i = 0; i < devices.GetSize(); i++) {
    PwxString str = devices[i];
    str.Replace(wxT("\t"), wxT(": "));
    combo->Append(str);
  }
  m_SoundRecorder = m_manager.pcssEP->GetSoundChannelRecordDevice();
  m_SoundRecorder.Replace(wxT("\t"), wxT(": "));

  // Fill line interface combo box with available devices and set selection
  m_selectedAEC = FindWindowByNameAs<wxChoice>(this, AECKey);
  m_selectedCountry = FindWindowByNameAs<wxComboBox>(this, CountryKey);
  m_selectedCountry->SetValidator(wxGenericValidator(&m_Country));
  m_selectedLID = FindWindowByNameAs<wxComboBox>(this, LineInterfaceDeviceKey);
  m_selectedLID->SetValidator(wxGenericValidator(&m_LineInterfaceDevice));
  devices = OpalLineInterfaceDevice::GetAllDevices();
  if (devices.IsEmpty()) {
    m_LineInterfaceDevice = "<< None available >>";
    m_selectedLID->Append(m_LineInterfaceDevice);
    m_selectedAEC->Disable();
    m_selectedCountry->Disable();
  }
  else {
    static const wxChar UseSoundCard[] = wxT("<< Use sound card only >>");
    m_selectedLID->Append(UseSoundCard);
    for (i = 0; i < devices.GetSize(); i++)
      m_selectedLID->Append(PwxString(devices[i]));

    OpalLine * line = m_manager.potsEP->GetLine("*");
    if (line != NULL) {
      m_LineInterfaceDevice = line->GetDevice().GetDeviceType() + ": " + line->GetDevice().GetDeviceName();
      for (i = 0; i < devices.GetSize(); i++) {
        if (m_LineInterfaceDevice == devices[i])
          break;
      }
      if (i >= devices.GetSize()) {
        for (i = 0; i < devices.GetSize(); i++) {
          if (devices[i].Find(m_LineInterfaceDevice.c_str()) == 0)
            break;
        }
        if (i >= devices.GetSize())
          m_LineInterfaceDevice = devices[0];
      }

      INIT_FIELD(AEC, line->GetAEC());

      PStringList countries = line->GetDevice().GetCountryCodeNameList();
      for (PStringList::iterator country = countries.begin(); country != countries.end(); ++country)
        m_selectedCountry->Append(PwxString(*country));
      INIT_FIELD(Country, line->GetDevice().GetCountryCodeName());
    }
    else {
      m_LineInterfaceDevice = UseSoundCard;
      m_selectedAEC->Disable();
      m_selectedCountry->Disable();
    }
  }

  ////////////////////////////////////////
  // Video fields
  INIT_FIELD(VideoGrabber, m_manager.GetVideoInputDevice().deviceName);
  INIT_FIELD(VideoGrabFormat, m_manager.GetVideoInputDevice().videoFormat);
  INIT_FIELD(VideoGrabSource, m_manager.GetVideoInputDevice().channelNumber);
  INIT_FIELD(VideoGrabFrameRate, m_manager.GetVideoInputDevice().rate);
  INIT_FIELD(VideoFlipLocal, m_manager.GetVideoInputDevice().flip != PFalse);
  INIT_FIELD(VideoGrabPreview, m_manager.m_VideoGrabPreview);
  INIT_FIELD(VideoAutoTransmit, m_manager.CanAutoStartTransmitVideo() != PFalse);
  INIT_FIELD(VideoAutoReceive, m_manager.CanAutoStartReceiveVideo() != PFalse);
  INIT_FIELD(VideoFlipRemote, m_manager.GetVideoOutputDevice().flip != PFalse);

  m_VideoGrabFrameSize = m_manager.m_VideoGrabFrameSize;
  FindWindowByName(VideoGrabFrameSizeKey)->SetValidator(wxFrameSizeValidator(&m_VideoGrabFrameSize));
  m_VideoMinFrameSize = m_manager.m_VideoMinFrameSize;
  FindWindowByName(VideoMinFrameSizeKey)->SetValidator(wxFrameSizeValidator(&m_VideoMinFrameSize));
  m_VideoMaxFrameSize = m_manager.m_VideoMaxFrameSize;
  FindWindowByName(VideoMaxFrameSizeKey)->SetValidator(wxFrameSizeValidator(&m_VideoMaxFrameSize));

  combo = FindWindowByNameAs<wxComboBox>(this, wxT("VideoGrabber"));
  devices = PVideoInputDevice::GetDriversDeviceNames("*");
  for (i = 0; i < devices.GetSize(); i++)
    combo->Append(PwxString(devices[i]));

  ////////////////////////////////////////
  // Fax fields
#if OPAL_FAX
  INIT_FIELD(FaxStationIdentifier, (const char *)m_manager.m_faxEP->GetDefaultDisplayName());
  INIT_FIELD(FaxReceiveDirectory, (const char *)m_manager.m_faxEP->GetDefaultDirectory());
  INIT_FIELD(FaxSpanDSP, (const char *)m_manager.m_faxEP->GetSpanDSP());
#else
  RemoveNotebookPage(this, "Fax");
#endif

  ////////////////////////////////////////
  // Codec fields
  m_AddCodec = FindWindowByNameAs<wxButton>(this, wxT("AddCodec"));
  m_AddCodec->Disable();
  m_RemoveCodec = FindWindowByNameAs<wxButton>(this, wxT("RemoveCodec"));
  m_RemoveCodec->Disable();
  m_MoveUpCodec = FindWindowByNameAs<wxButton>(this, wxT("MoveUpCodec"));
  m_MoveUpCodec->Disable();
  m_MoveDownCodec = FindWindowByNameAs<wxButton>(this, wxT("MoveDownCodec"));
  m_MoveDownCodec->Disable();

  m_allCodecs = FindWindowByNameAs<wxListBox>(this, wxT("AllCodecs"));
  m_selectedCodecs = FindWindowByNameAs<wxListBox>(this, wxT("SelectedCodecs"));
  for (MyMediaList::iterator mm = m_manager.m_mediaInfo.begin(); mm != m_manager.m_mediaInfo.end(); ++mm) {
    PwxString str(mm->sourceProtocol);
    str += wxT(": ");
    str += PwxString(mm->mediaFormat);
    str += mm->validProtocols;
    m_allCodecs->Append(str, &*mm);

    str = PwxString(mm->mediaFormat);
    if (mm->preferenceOrder >= 0 && m_selectedCodecs->FindString(str) < 0)
      m_selectedCodecs->Append(str, &*mm);
  }
  m_codecOptions = FindWindowByNameAs<wxListCtrl>(this, wxT("CodecOptionsList"));
  int columnWidth = (m_codecOptions->GetClientSize().GetWidth()-30)/2;
  m_codecOptions->InsertColumn(0, wxT("Option"), wxLIST_FORMAT_LEFT, columnWidth);
  m_codecOptions->InsertColumn(1, wxT("Value"), wxLIST_FORMAT_LEFT, columnWidth);
  m_codecOptionValue = FindWindowByNameAs<wxTextCtrl>(this, wxT("CodecOptionValue"));
  m_codecOptionValue->Disable();
  m_CodecOptionValueLabel = FindWindowByNameAs<wxStaticText>(this, wxT("CodecOptionValueLabel"));
  m_CodecOptionValueLabel->Disable();
  m_CodecOptionValueError = FindWindowByNameAs<wxStaticText>(this, wxT("CodecOptionValueError"));
  m_CodecOptionValueError->Show(false);

  ////////////////////////////////////////
  // Security fields
#if OPAL_PTLIB_SSL
  INIT_FIELD(SecureH323, m_manager.FindEndPoint("h323s") != NULL);
  INIT_FIELD(SecureSIP, m_manager.FindEndPoint("sips") != NULL);
#else
  FindWindowByName(SecureH323Key)->Disable();
  FindWindowByName(SecureSIPKey)->Disable();
#endif
#if (defined OPAL_SRTP) || (defined OPAL_ZRTP)
#if OPAL_H323
  INIT_FIELD(RTPSecurityModeH323, m_manager.h323EP->GetDefaultSecurityMode());
#endif // OPAL_H323
#if OPAL_SIP
  INIT_FIELD(RTPSecurityModeSIP, m_manager.sipEP->GetDefaultSecurityMode());
#endif
#ifndef OPAL_SRTP
  choice = FindWindowByNameAs<wxChoice>(this, RTPSecurityModeH323Key);
  choice->Delete(choice->FindString("SRTP"));
  choice = FindWindowByNameAs<wxChoice>(this, RTPSecurityModeSIPKey);
  choice->Delete(choice->FindString("SRTP"));
#endif
#ifndef OPAL_ZRTP
  choice = FindWindowByNameAs<wxChoice>(this, RTPSecurityModeH323Key);
  choice->Delete(choice->FindString("ZRTP"));
  choice = FindWindowByNameAs<wxChoice>(this, RTPSecurityModeSIPKey);
  choice->Delete(choice->FindString("ZRTP"));
#endif
#else
  FindWindowByName(RTPSecurityModeH323Key)->Disable();
  FindWindowByName(RTPSecurityModeSIPKey)->Disable();
#endif // OPAL_SRTP || OPAL_ZRTP

#if OPAL_H323
  ////////////////////////////////////////
  // H.323 fields
  m_AddAlias = FindWindowByNameAs<wxButton>(this, wxT("AddAlias"));
  m_AddAlias->Disable();
  m_RemoveAlias = FindWindowByNameAs<wxButton>(this, wxT("RemoveAlias"));
  m_RemoveAlias->Disable();
  m_NewAlias = FindWindowByNameAs<wxTextCtrl>(this, wxT("NewAlias"));
  m_Aliases = FindWindowByNameAs<wxListBox>(this, wxT("Aliases"));
  PStringList aliases = m_manager.h323EP->GetAliasNames();
  for (i = 1; i < aliases.GetSize(); i++)
    m_Aliases->Append(PwxString(aliases[i]));

  INIT_FIELD(DTMFSendMode, m_manager.h323EP->GetSendUserInputMode());
  if (m_DTMFSendMode > OpalConnection::SendUserInputAsInlineRFC2833)
    m_DTMFSendMode = OpalConnection::SendUserInputAsString;
#if OPAL_450
  INIT_FIELD(CallIntrusionProtectionLevel, m_manager.h323EP->GetCallIntrusionProtectionLevel());
#endif
  INIT_FIELD(DisableFastStart, m_manager.h323EP->IsFastStartDisabled() != PFalse);
  INIT_FIELD(DisableH245Tunneling, m_manager.h323EP->IsH245TunnelingDisabled() != PFalse);
  INIT_FIELD(DisableH245inSETUP, m_manager.h323EP->IsH245inSetupDisabled() != PFalse);
  INIT_FIELD(GatekeeperMode, m_manager.m_gatekeeperMode);
  INIT_FIELD(GatekeeperAddress, m_manager.m_gatekeeperAddress);
  INIT_FIELD(GatekeeperIdentifier, m_manager.m_gatekeeperIdentifier);
  INIT_FIELD(GatekeeperTTL, m_manager.h323EP->GetGatekeeperTimeToLive().GetSeconds());
  INIT_FIELD(GatekeeperLogin, m_manager.h323EP->GetGatekeeperUsername());
  INIT_FIELD(GatekeeperPassword, m_manager.h323EP->GetGatekeeperPassword());
#endif

#if OPAL_SIP
  ////////////////////////////////////////
  // SIP fields
  INIT_FIELD(SIPProxyUsed, m_manager.m_SIPProxyUsed);
  INIT_FIELD(SIPProxy, m_manager.sipEP->GetProxy().GetHostName());
  INIT_FIELD(SIPProxyUsername, m_manager.sipEP->GetProxy().GetUserName());
  INIT_FIELD(SIPProxyPassword, m_manager.sipEP->GetProxy().GetPassword());

  m_SelectedRegistrar = INT_MAX;

  m_Registrars = FindWindowByNameAs<wxListCtrl>(this, wxT("Registrars"));
  m_Registrars->InsertColumn(0, _T("Domain"));
  m_Registrars->InsertColumn(1, _T("User"));
  m_Registrars->InsertColumn(2, _T("Refresh"));
  m_Registrars->InsertColumn(3, _T("Status"));
  m_Registrars->InsertColumn(4, _T("MWI"));
  m_Registrars->InsertColumn(5, _T("Presence"));
  for (RegistrarList::iterator registrar = m_manager.m_registrars.begin(); registrar != m_manager.m_registrars.end(); ++registrar)
    RegistrarToList(false, new RegistrarInfo(*registrar), INT_MAX);
  m_Registrars->SetColumnWidth(0, 160);
  m_Registrars->SetColumnWidth(1, 120);
  m_Registrars->SetColumnWidth(2, 50);
  m_Registrars->SetColumnWidth(3, 60);
  m_Registrars->SetColumnWidth(4, 60);
  m_Registrars->SetColumnWidth(5, 60);

  m_AddRegistrar = FindWindowByNameAs<wxButton>(this, wxT("AddRegistrar"));
  m_AddRegistrar->Disable();

  m_ChangeRegistrar = FindWindowByNameAs<wxButton>(this, wxT("ChangeRegistrar"));
  m_ChangeRegistrar->Disable();

  m_RemoveRegistrar = FindWindowByNameAs<wxButton>(this, wxT("RemoveRegistrar"));
  m_RemoveRegistrar->Disable();

  m_RegistrarUser = FindWindowByNameAs<wxTextCtrl>(this, RegistrarUsernameKey);
  m_RegistrarDomain = FindWindowByNameAs<wxTextCtrl>(this, RegistrarDomainKey);
  m_RegistrarAuthID = FindWindowByNameAs<wxTextCtrl>(this, RegistrarAuthIDKey);
  m_RegistrarPassword = FindWindowByNameAs<wxTextCtrl>(this, RegistrarPasswordKey);
  m_RegistrarTimeToLive = FindWindowByNameAs<wxSpinCtrl>(this, RegistrarTimeToLiveKey);
  m_RegistrarActive = FindWindowByNameAs<wxCheckBox>(this, RegistrarUsedKey);
  m_SubscribeMWI = FindWindowByNameAs<wxCheckBox>(this, SubscribeMWIKey);
  m_SubscribePresence = FindWindowByNameAs<wxCheckBox>(this, SubscribePresenceKey);
#endif // OPAL_SIP


  ////////////////////////////////////////
  // Routing fields
  m_SelectedRoute = INT_MAX;

  m_RouteDevice = FindWindowByNameAs<wxTextCtrl>(this, wxT("RouteDevice"));
  m_RoutePattern = FindWindowByNameAs<wxTextCtrl>(this, wxT("RoutePattern"));
  m_RouteDestination = FindWindowByNameAs<wxTextCtrl>(this, wxT("RouteDestination"));

  m_AddRoute = FindWindowByNameAs<wxButton>(this, wxT("AddRoute"));
  m_AddRoute->Disable();
  m_RemoveRoute = FindWindowByNameAs<wxButton>(this, wxT("RemoveRoute"));
  m_RemoveRoute->Disable();
  m_MoveUpRoute = FindWindowByNameAs<wxButton>(this, wxT("MoveUpRoute"));
  m_MoveUpRoute->Disable();
  m_MoveDownRoute = FindWindowByNameAs<wxButton>(this, wxT("MoveDownRoute"));
  m_MoveDownRoute->Disable();

  // Fill list box with active routes
  m_Routes = FindWindowByNameAs<wxListCtrl>(this, wxT("Routes"));
  m_Routes->InsertColumn(0, _T("Source"));
  m_Routes->InsertColumn(1, _T("Dev/If"));
  m_Routes->InsertColumn(2, _T("Pattern"));
  m_Routes->InsertColumn(3, _T("Destination"));
  const OpalManager::RouteTable & routeTable = m_manager.GetRouteTable();
  for (i = 0; i < routeTable.GetSize(); i++)
    AddRouteTableEntry(routeTable[i]);

  for (i = 0; i < m_Routes->GetColumnCount(); i++)
    m_Routes->SetColumnWidth(i, wxLIST_AUTOSIZE_USEHEADER);

  // Fill combo box with possible protocols
  m_RouteSource = FindWindowByNameAs<wxComboBox>(this, wxT("RouteSource"));
  m_RouteSource->Append(AllRouteSources);
  PList<OpalEndPoint> endpoints = m_manager.GetEndPoints();
  for (i = 0; i < endpoints.GetSize(); i++)
    m_RouteSource->Append(PwxString(endpoints[i].GetPrefixName()));
  m_RouteSource->SetSelection(0);


#if PTRACING
  ////////////////////////////////////////
  // Tracing fields
  INIT_FIELD(EnableTracing, m_manager.m_enableTracing);
  INIT_FIELD(TraceLevelThreshold, PTrace::GetLevel());
  INIT_FIELD(TraceLevelNumber, (PTrace::GetOptions()&PTrace::TraceLevel) != 0);
  INIT_FIELD(TraceFileLine, (PTrace::GetOptions()&PTrace::FileAndLine) != 0);
  INIT_FIELD(TraceBlocks, (PTrace::GetOptions()&PTrace::Blocks) != 0);
  INIT_FIELD(TraceDateTime, (PTrace::GetOptions()&PTrace::DateAndTime) != 0);
  INIT_FIELD(TraceTimestamp, (PTrace::GetOptions()&PTrace::Timestamp) != 0);
  INIT_FIELD(TraceThreadName, (PTrace::GetOptions()&PTrace::Thread) != 0);
  INIT_FIELD(TraceThreadAddress, (PTrace::GetOptions()&PTrace::ThreadAddress) != 0);
  INIT_FIELD(TraceFileName, m_manager.m_traceFileName);
#else
  RemoveNotebookPage(this, "Tracing");
#endif // PTRACING
}


OptionsDialog::~OptionsDialog()
{
  for (int i = 0; i < m_Registrars->GetItemCount(); ++i)
    delete (RegistrarInfo *)m_Registrars->GetItemData(i);
}


#define SAVE_FIELD(name, set) \
  set(m_##name); \
  config->Write(name##Key, m_##name)

#define SAVE_FIELD_POST(name, set, post) \
  set(m_##name post); \
  config->Write(name##Key, m_##name)

#define SAVE_FIELD2(name1, name2, set) \
  set(m_##name1, m_##name2); \
  config->Write(name1##Key, m_##name1); \
  config->Write(name2##Key, m_##name2)

bool OptionsDialog::TransferDataFromWindow()
{
  if (!wxDialog::TransferDataFromWindow())
    return false;

  double floatBandwidth;
  if (!m_Bandwidth.ToDouble(&floatBandwidth) || floatBandwidth < 10)
    return false;

  ::wxBeginBusyCursor();

  wxConfigBase * config = wxConfig::Get();

  ////////////////////////////////////////
  // General fields
  config->SetPath(GeneralGroup);
  SAVE_FIELD(Username, m_manager.SetDefaultUserName);
  SAVE_FIELD(DisplayName, m_manager.SetDefaultDisplayName);
  m_RingSoundDeviceName.Replace(wxT(": "), wxT("\t"));
  SAVE_FIELD(RingSoundDeviceName, m_manager.m_RingSoundDeviceName = );
  SAVE_FIELD(RingSoundFileName, m_manager.m_RingSoundFileName = );
  SAVE_FIELD(AutoAnswer, m_manager.m_autoAnswer = );
#if OPAL_IVR
  SAVE_FIELD(IVRScript, m_manager.ivrEP->SetDefaultVXML);
#endif

  ////////////////////////////////////////
  // Networking fields
  config->SetPath(NetworkingGroup);
  int adjustedBandwidth = (int)(floatBandwidth*10);
#if OPAL_H323
  m_manager.h323EP->SetInitialBandwidth(adjustedBandwidth);
#endif
  config->Write(BandwidthKey, adjustedBandwidth);
  SAVE_FIELD2(TCPPortBase, TCPPortMax, m_manager.SetTCPPorts);
  SAVE_FIELD2(UDPPortBase, UDPPortMax, m_manager.SetUDPPorts);
  SAVE_FIELD2(RTPPortBase, RTPPortMax, m_manager.SetRtpIpPorts);
  SAVE_FIELD(RTPTOS, m_manager.SetRtpIpTypeofService);
  m_manager.m_NATHandling = m_STUNServerRadio->GetValue() ? 2 : m_NATRouterRadio->GetValue() ? 1 : 0;
  config->Write(NATHandlingKey, m_manager.m_NATHandling);
  SAVE_FIELD(STUNServer, m_manager.m_STUNServer = );
  SAVE_FIELD(NATRouter, m_manager.m_NATRouter = );
  m_manager.SetNATHandling();

  config->DeleteGroup(LocalInterfacesGroup);
  config->SetPath(LocalInterfacesGroup);
  PStringArray newInterfaces(m_LocalInterfaces->GetCount());
  bool changed = m_manager.m_LocalInterfaces.GetSize() != newInterfaces.GetSize();
  for (int i = 0; i < newInterfaces.GetSize(); i++) {
    newInterfaces[i] = (PString)m_LocalInterfaces->GetString(i);
    PINDEX oldIndex = m_manager.m_LocalInterfaces.GetValuesIndex(newInterfaces[i]);
    if (oldIndex == P_MAX_INDEX || newInterfaces[i] != m_manager.m_LocalInterfaces[oldIndex])
      changed = true;
    wxString key;
    key.sprintf(wxT("%u"), i+1);
    config->Write(key, (const char *)newInterfaces[i]);
  }
  if (changed) {
    m_manager.m_LocalInterfaces = newInterfaces;
    m_manager.StartAllListeners();
  }

  ////////////////////////////////////////
  // Sound fields
  config->SetPath(AudioGroup);
  m_SoundPlayer.Replace(wxT(": "), wxT("\t"));
  m_SoundRecorder.Replace(wxT(": "), wxT("\t"));
  SAVE_FIELD(SoundPlayer, m_manager.pcssEP->SetSoundChannelPlayDevice);
  SAVE_FIELD(SoundRecorder, m_manager.pcssEP->SetSoundChannelRecordDevice);
  SAVE_FIELD(SoundBuffers, m_manager.pcssEP->SetSoundChannelBufferDepth);
  SAVE_FIELD2(MinJitter, MaxJitter, m_manager.SetAudioJitterDelay);

  OpalSilenceDetector::Params silenceParams;
  SAVE_FIELD(SilenceSuppression, silenceParams.m_mode=(OpalSilenceDetector::Mode));
  SAVE_FIELD(SilenceThreshold, silenceParams.m_threshold=);
  SAVE_FIELD(SignalDeadband, silenceParams.m_signalDeadband=8*);
  SAVE_FIELD(SilenceDeadband, silenceParams.m_silenceDeadband=8*);
  m_manager.SetSilenceDetectParams(silenceParams);

  SAVE_FIELD(DisableDetectInBandDTMF, m_manager.DisableDetectInBandDTMF);

  if (m_LineInterfaceDevice.StartsWith(wxT("<<")) && m_LineInterfaceDevice.EndsWith(wxT(">>")))
    m_LineInterfaceDevice.Empty();
  config->Write(LineInterfaceDeviceKey, m_LineInterfaceDevice);
  config->Write(AECKey, m_AEC);
  config->Write(CountryKey, m_Country);
  m_manager.StartLID();

  ////////////////////////////////////////
  // Video fields
  config->SetPath(VideoGroup);
  PVideoDevice::OpenArgs grabber = m_manager.GetVideoInputDevice();
  SAVE_FIELD_POST(VideoGrabber, grabber.deviceName = (PString), .mb_str(wxConvUTF8));   /// XXXXX
  SAVE_FIELD(VideoGrabFormat, grabber.videoFormat = (PVideoDevice::VideoFormat));
  SAVE_FIELD(VideoGrabSource, grabber.channelNumber = );
  SAVE_FIELD(VideoGrabFrameRate, grabber.rate = );
  SAVE_FIELD(VideoGrabFrameSize, m_manager.m_VideoGrabFrameSize = );
  SAVE_FIELD(VideoFlipLocal, grabber.flip = );
  m_manager.SetVideoInputDevice(grabber);
  SAVE_FIELD(VideoGrabPreview, m_manager.m_VideoGrabPreview = );
  SAVE_FIELD(VideoAutoTransmit, m_manager.SetAutoStartTransmitVideo);
  SAVE_FIELD(VideoAutoReceive, m_manager.SetAutoStartReceiveVideo);
//  SAVE_FIELD(VideoFlipRemote, );
  SAVE_FIELD(VideoMinFrameSize, m_manager.m_VideoMinFrameSize = );
  SAVE_FIELD(VideoMaxFrameSize, m_manager.m_VideoMaxFrameSize = );
  m_manager.AdjustFrameSize();

  ////////////////////////////////////////
  // Fax fields
#if OPAL_FAX
  config->SetPath(FaxGroup);
  SAVE_FIELD(FaxStationIdentifier, m_manager.m_faxEP->SetDefaultDisplayName);
  SAVE_FIELD(FaxReceiveDirectory, m_manager.m_faxEP->SetDefaultDirectory);
  SAVE_FIELD(FaxSpanDSP, m_manager.m_faxEP->SetSpanDSP);
#endif

  ////////////////////////////////////////
  // Codec fields
  MyMediaList::iterator mm;
  for (mm = m_manager.m_mediaInfo.begin(); mm != m_manager.m_mediaInfo.end(); ++mm)
    mm->preferenceOrder = -1;

  size_t codecIndex;
  for (codecIndex = 0; codecIndex < m_selectedCodecs->GetCount(); codecIndex++) {
    PwxString selectedFormat = m_selectedCodecs->GetString(codecIndex);
    for (mm = m_manager.m_mediaInfo.begin(); mm != m_manager.m_mediaInfo.end(); ++mm) {
      if (selectedFormat == mm->mediaFormat) {
        mm->preferenceOrder = codecIndex;
        break;
      }
    }
  }

  m_manager.ApplyMediaInfo();

  config->DeleteGroup(CodecsGroup);
  for (mm = m_manager.m_mediaInfo.begin(); mm != m_manager.m_mediaInfo.end(); ++mm) {
    if (mm->preferenceOrder >= 0) {
      wxString groupName;
      groupName.sprintf(wxT("%s/%04u"), CodecsGroup, mm->preferenceOrder);
      config->SetPath(groupName);
      config->Write(CodecNameKey, (const char *)mm->mediaFormat);
      for (PINDEX i = 0; i < mm->mediaFormat.GetOptionCount(); i++) {
        const OpalMediaOption & option = mm->mediaFormat.GetOption(i);
        if (!option.IsReadOnly())
          config->Write(PwxString(option.GetName()), PwxString(option.AsString()));
      }
      if (mm->dirty) {
        OpalMediaFormat::SetRegisteredMediaFormat(mm->mediaFormat);
        mm->dirty = false;
      }
    }
  }


  ////////////////////////////////////////
  // Security fields
  config->SetPath(SecurityGroup);
  if (m_SecureH323)
    m_manager.AttachEndPoint(m_manager.FindEndPoint("h323"), "h323s");
  else
    m_manager.DetachEndPoint("h323s");
  config->Write(SecureH323Key, m_SecureH323);

  if (m_SecureSIP)
    m_manager.AttachEndPoint(m_manager.FindEndPoint("sip"), "sips");
  else
    m_manager.DetachEndPoint("sips");
  config->Write(SecureSIPKey, m_SecureSIP);

  if (m_RTPSecurityModeH323 == "None")
    m_RTPSecurityModeH323.erase();
  if (m_RTPSecurityModeSIP == "None")
    m_RTPSecurityModeSIP.erase();
#if OPAL_H323
  SAVE_FIELD(RTPSecurityModeH323, m_manager.h323EP->SetDefaultSecurityMode);
#endif
#if OPAL_SIP
  SAVE_FIELD(RTPSecurityModeSIP, m_manager.sipEP->SetDefaultSecurityMode);
#endif


#if OPAL_H323
  ////////////////////////////////////////
  // H.323 fields
  config->DeleteGroup(H323AliasesGroup);
  config->SetPath(H323AliasesGroup);
  m_manager.h323EP->SetLocalUserName(m_Username);
  PStringList aliases = m_manager.h323EP->GetAliasNames();
  for (size_t i = 0; i < m_Aliases->GetCount(); i++) {
    wxString alias = m_Aliases->GetString(i);
    m_manager.h323EP->AddAliasName(alias.c_str());
    wxString key;
    key.sprintf(wxT("%u"), i+1);
    config->Write(key, alias);
  }

  config->SetPath(H323Group);
  m_manager.h323EP->SetSendUserInputMode((H323Connection::SendUserInputModes)m_DTMFSendMode);
  config->Write(DTMFSendModeKey, m_DTMFSendMode);
#if OPAL_450
  SAVE_FIELD(CallIntrusionProtectionLevel, m_manager.h323EP->SetCallIntrusionProtectionLevel);
#endif
  SAVE_FIELD(DisableFastStart, m_manager.h323EP->DisableFastStart);
  SAVE_FIELD(DisableH245Tunneling, m_manager.h323EP->DisableH245Tunneling);
  SAVE_FIELD(DisableH245inSETUP, m_manager.h323EP->DisableH245inSetup);

  config->Write(GatekeeperTTLKey, m_GatekeeperTTL);
  m_manager.h323EP->SetGatekeeperTimeToLive(PTimeInterval(0, m_GatekeeperTTL));

  if (m_manager.m_gatekeeperMode != m_GatekeeperMode ||
      m_manager.m_gatekeeperAddress != m_GatekeeperAddress ||
      m_manager.m_gatekeeperIdentifier != m_GatekeeperIdentifier ||
      PwxString(m_manager.h323EP->GetGatekeeperUsername()) != m_GatekeeperLogin ||
      PwxString(m_manager.h323EP->GetGatekeeperPassword()) != m_GatekeeperPassword) {
    SAVE_FIELD(GatekeeperMode, m_manager.m_gatekeeperMode = );
    SAVE_FIELD(GatekeeperAddress, m_manager.m_gatekeeperAddress = );
    SAVE_FIELD(GatekeeperIdentifier, m_manager.m_gatekeeperIdentifier = );
    SAVE_FIELD2(GatekeeperPassword, GatekeeperLogin, m_manager.h323EP->SetGatekeeperPassword);

    if (!m_manager.StartGatekeeper())
      m_manager.Close();
  }
#endif

#if OPAL_SIP
  ////////////////////////////////////////
  // SIP fields
  config->SetPath(SIPGroup);
  SAVE_FIELD(SIPProxyUsed, m_manager.m_SIPProxyUsed =);
  m_manager.sipEP->SetProxy(m_SIPProxy, m_SIPProxyUsername, m_SIPProxyPassword);
  config->Write(SIPProxyKey, m_SIPProxy);
  config->Write(SIPProxyUsernameKey, m_SIPProxyUsername);
  config->Write(SIPProxyPasswordKey, m_SIPProxyPassword);

  RegistrarList newRegistrars;

  for (int i = 0; i < m_Registrars->GetItemCount(); ++i)
    newRegistrars.push_back(*(RegistrarInfo *)m_Registrars->GetItemData(i));

  if (newRegistrars != m_manager.m_registrars) {
    config->DeleteEntry(RegistrarUsedKey);
    config->DeleteEntry(RegistrarNameKey);
    config->DeleteEntry(RegistrarUsernameKey);
    config->DeleteEntry(RegistrarPasswordKey);
    config->DeleteGroup(RegistrarGroup);

    int registrarIndex = 1;
    for (RegistrarList::iterator iterReg = newRegistrars.begin(); iterReg != newRegistrars.end(); ++iterReg) {
      wxString group;
      group.sprintf(wxT("%s/%04u"), RegistrarGroup, registrarIndex++);
      config->SetPath(group);
      config->Write(RegistrarUsedKey, iterReg->m_Active);
      config->Write(RegistrarUsernameKey, iterReg->m_User);
      config->Write(RegistrarDomainKey, iterReg->m_Domain);
      config->Write(RegistrarAuthIDKey, iterReg->m_AuthID);
      config->Write(RegistrarPasswordKey, iterReg->m_Password);
      config->Write(RegistrarTimeToLiveKey, iterReg->m_TimeToLive);
      config->Write(SubscribeMWIKey, iterReg->m_MWI);
      config->Write(SubscribePresenceKey, iterReg->m_Presence);
    }

    m_manager.StopRegistrars();
    m_manager.m_registrars = newRegistrars;
    m_manager.StartRegistrars();
  }
#endif // OPAL_SIP

  ////////////////////////////////////////
  // Routing fields

  config->DeleteGroup(RoutingGroup);
  config->SetPath(RoutingGroup);
  PStringArray routeSpecs;
  for (int i = 0; i < m_Routes->GetItemCount(); i++) {
    PwxString spec;
    wxListItem item;
    item.m_itemId = i;
    item.m_mask = wxLIST_MASK_TEXT;
    m_Routes->GetItem(item);
    spec += (item.m_text == wxT("<ALL>")) ? wxT(".*") : item.m_text;
    spec += ':';
    item.m_col++;
    m_Routes->GetItem(item);
    spec += item.m_text.empty() ? wxT(".*") : item.m_text;
    spec += '\t';
    item.m_col++;
    m_Routes->GetItem(item);
    spec += item.m_text;
    spec += '=';
    item.m_col++;
    m_Routes->GetItem(item);
    spec += item.m_text;
    routeSpecs.AppendString(spec);

    wxString key;
    key.sprintf(wxT("%04u"), i+1);
    config->Write(key, spec);
  }
  m_manager.SetRouteTable(routeSpecs);


#if PTRACING
  ////////////////////////////////////////
  // Tracing fields
  config->SetPath(TracingGroup);
  int traceOptions = 0;
  if (m_TraceLevelNumber)
    traceOptions |= PTrace::TraceLevel;
  if (m_TraceFileLine)
    traceOptions |= PTrace::FileAndLine;
  if (m_TraceBlocks)
    traceOptions |= PTrace::Blocks;
  if (m_TraceDateTime)
    traceOptions |= PTrace::DateAndTime;
  if (m_TraceTimestamp)
    traceOptions |= PTrace::Timestamp;
  if (m_TraceThreadName)
    traceOptions |= PTrace::Thread;
  if (m_TraceThreadAddress)
    traceOptions |= PTrace::ThreadAddress;

  config->Write(EnableTracingKey, m_EnableTracing);
  config->Write(TraceLevelThresholdKey, m_TraceLevelThreshold);
  config->Write(TraceFileNameKey, m_TraceFileName);
  config->Write(TraceOptionsKey, traceOptions);

  // Check for stopping tracing
  if (m_manager.m_enableTracing && (!m_EnableTracing || m_TraceFileName.empty()))
    PTrace::SetStream(NULL);
  else if (m_EnableTracing && (!m_manager.m_enableTracing || m_manager.m_traceFileName != m_TraceFileName))
    PTrace::Initialise(m_TraceLevelThreshold, PFilePath(m_TraceFileName.c_str()), traceOptions);
  else {
    PTrace::SetLevel(m_TraceLevelThreshold);
    PTrace::SetOptions(traceOptions);
    PTrace::ClearOptions(~traceOptions);
  }

  m_manager.m_enableTracing = m_EnableTracing;
  m_manager.m_traceFileName = m_TraceFileName;
#endif // PTRACING

  ::wxEndBusyCursor();

  return true;
}


////////////////////////////////////////
// General fields

void OptionsDialog::BrowseSoundFile(wxCommandEvent & /*event*/)
{
  wxString newFile = wxFileSelector(wxT("Sound file to play on incoming calls"),
                                    wxT(""),
                                    m_RingSoundFileName,
                                    wxT(".wav"),
                                    wxT("WAV files (*.wav)|*.wav"),
                                    wxOPEN|wxFILE_MUST_EXIST);
  if (!newFile.empty()) {
    m_RingSoundFileName = newFile;
    TransferDataToWindow();
  }
}


void OptionsDialog::PlaySoundFile(wxCommandEvent & /*event*/)
{
  PSoundChannel speaker(m_manager.m_RingSoundDeviceName, PSoundChannel::Player);
  speaker.PlayFile(m_RingSoundFileName);
}


////////////////////////////////////////
// Networking fields

void OptionsDialog::BandwidthClass(wxCommandEvent & event)
{
  static const wxChar * bandwidthClasses[] = {
    wxT("14.4"), wxT("28.8"), wxT("64.0"), wxT("128"), wxT("1500"), wxT("10000")
  };

  m_Bandwidth = bandwidthClasses[event.GetSelection()];
  TransferDataToWindow();
}


void OptionsDialog::NATHandling(wxCommandEvent &)
{
  if (m_STUNServerRadio->GetValue()) {
    m_STUNServerWnd->Enable();
    m_NATRouterWnd->Disable();
  }
  else if (m_NATRouterRadio->GetValue()) {
    m_STUNServerWnd->Disable();
    m_NATRouterWnd->Enable();
  }
  else {
    m_STUNServerWnd->Disable();
    m_NATRouterWnd->Disable();
  }
}


void OptionsDialog::SelectedLocalInterface(wxCommandEvent & /*event*/)
{
  m_RemoveInterface->Enable(m_LocalInterfaces->GetSelection() != wxNOT_FOUND);
}


void OptionsDialog::ChangedInterfaceInfo(wxCommandEvent & /*event*/)
{
  bool enab = true;
  PString iface = m_InterfaceAddress->GetValue().c_str();
  if (iface.IsEmpty())
    enab = false;
  else if (iface != "*") {
    PIPSocket::Address test(iface);
    if (!test.IsValid())
      enab = false;
  }

  if (m_InterfaceProtocol->GetSelection() == 0)
    m_InterfacePort->Disable();
  else {
    m_InterfacePort->Enable();
    if (m_InterfacePort->GetValue().IsEmpty())
      enab = false;
  }

  m_AddInterface->Enable(enab);
}


static const char * const InterfacePrefixes[] = {
  "all:", "h323:tcp$", "sip:udp$", "sip:tcp$"
};

void OptionsDialog::AddInterface(wxCommandEvent & /*event*/)
{
  int proto = m_InterfaceProtocol->GetSelection();
  wxString iface = PwxString(InterfacePrefixes[proto]);
  iface += m_InterfaceAddress->GetValue();
  if (proto > 0) 
    iface += wxT(":") + m_InterfacePort->GetValue();
  m_LocalInterfaces->Append(iface);
}


void OptionsDialog::RemoveInterface(wxCommandEvent & /*event*/)
{
  wxString iface = m_LocalInterfaces->GetStringSelection();

  for (int i = 0; i < PARRAYSIZE(InterfacePrefixes); i++) {
    if (iface.StartsWith(PwxString(InterfacePrefixes[i]))) {
      m_InterfaceProtocol->SetSelection(i);
      iface.Remove(0, strlen(InterfacePrefixes[i]));
    }
  }

  size_t colon = iface.find(':');
  if (colon != string::npos) {
    m_InterfacePort->SetValue(iface.Mid(colon+1));
    iface.Remove(colon);
  }

  m_InterfaceAddress->SetValue(iface);
  m_LocalInterfaces->Delete(m_LocalInterfaces->GetSelection());
  m_RemoveInterface->Disable();
}


////////////////////////////////////////
// Audio fields

void OptionsDialog::SelectedLID(wxCommandEvent & /*event*/)
{
  bool enabled = m_selectedLID->GetSelection() > 0;
  m_selectedAEC->Enable(enabled);
  m_selectedCountry->Enable(enabled);

  if (enabled) {
    PwxString devName = m_selectedLID->GetValue();
    OpalLineInterfaceDevice * lidToDelete = NULL; 
    const OpalLineInterfaceDevice * lid = m_manager.potsEP->GetDeviceByName(devName);
    if (lid == NULL)
      lid = lidToDelete = OpalLineInterfaceDevice::CreateAndOpen(devName);
    if (lid != NULL) {
      m_selectedAEC->SetSelection(lid->GetAEC(0));

      m_selectedCountry->Clear();
      PStringList countries = lid->GetCountryCodeNameList();
      for (PStringList::iterator country = countries.begin(); country != countries.end(); ++country)
        m_selectedCountry->Append(PwxString(*country));
      m_selectedCountry->SetValue(PwxString(lid->GetCountryCodeName()));
    }
    delete lidToDelete;
  }
}


////////////////////////////////////////
// Fax fields

void OptionsDialog::BrowseFaxDirectory(wxCommandEvent & /*event*/)
{
  wxDirDialog dlg(this, wxT("Select Receive Directory for Faxes"), m_FaxReceiveDirectory);
  if (dlg.ShowModal() == wxID_OK) {
    m_FaxReceiveDirectory = dlg.GetPath();
    FindWindowByNameAs<wxTextCtrl>(this, wxT("FaxReceiveDirectory"))->SetLabel(m_FaxReceiveDirectory);
  }
}


void OptionsDialog::BrowseFaxSpanDSP(wxCommandEvent &)
{
  wxString newFile = wxFileSelector(wxT("Select location of Span DSP Utility executable"),
                                    wxT(""),
                                    m_FaxSpanDSP,
#ifdef _WIN32
                                    wxT(".exe"),
                                    wxT("EXE files (*.exe)|*.exe"),
#else
                                    wxEmptyString,
                                    wxEmptyString,
#endif
                                    wxOPEN|wxFILE_MUST_EXIST);
  if (!newFile.empty()) {
    m_FaxSpanDSP = newFile;
    FindWindowByNameAs<wxTextCtrl>(this, wxT("FaxSpanDSP"))->SetLabel(newFile);
  }
}


////////////////////////////////////////
// Codec fields

void OptionsDialog::AddCodec(wxCommandEvent & /*event*/)
{
  int insertionPoint = -1;
  wxArrayInt destinationSelections;
  if (m_selectedCodecs->GetSelections(destinationSelections) > 0)
    insertionPoint = destinationSelections[0];

  wxArrayInt sourceSelections;
  m_allCodecs->GetSelections(sourceSelections);
  for (size_t i = 0; i < sourceSelections.GetCount(); i++) {
    int sourceSelection = sourceSelections[i];
    wxString value = m_allCodecs->GetString(sourceSelection);
    void * data = m_allCodecs->GetClientData(sourceSelection);
    value.Remove(0, value.Find(':')+2);
    value.Replace(SIPonly, wxT(""));
    value.Replace(H323only, wxT(""));
    if (m_selectedCodecs->FindString(value) < 0) {
      if (insertionPoint < 0)
        m_selectedCodecs->Append(value, data);
      else {
        m_selectedCodecs->InsertItems(1, &value, insertionPoint);
        m_selectedCodecs->SetClientData(insertionPoint, data);
      }
    }
    m_allCodecs->Deselect(sourceSelections[i]);
  }

  m_AddCodec->Enable(false);
}


void OptionsDialog::RemoveCodec(wxCommandEvent & /*event*/)
{
  wxArrayInt selections;
  m_selectedCodecs->GetSelections(selections);
  for (int i = selections.GetCount()-1; i >= 0; i--)
    m_selectedCodecs->Delete(selections[i]);
  m_RemoveCodec->Enable(false);
  m_MoveUpCodec->Enable(false);
  m_MoveDownCodec->Enable(false);
}


void OptionsDialog::MoveUpCodec(wxCommandEvent & /*event*/)
{
  wxArrayInt selections;
  m_selectedCodecs->GetSelections(selections);
  int selection = selections[0];
  wxString value = m_selectedCodecs->GetString(selection);
  MyMedia * media = (MyMedia *)m_selectedCodecs->GetClientData(selection);
  m_selectedCodecs->Delete(selection);
  m_selectedCodecs->InsertItems(1, &value, --selection);
  m_selectedCodecs->SetClientData(selection, media);
  m_selectedCodecs->SetSelection(selection);
  m_MoveUpCodec->Enable(selection > 0);
  m_MoveDownCodec->Enable(true);
}


void OptionsDialog::MoveDownCodec(wxCommandEvent & /*event*/)
{
  wxArrayInt selections;
  m_selectedCodecs->GetSelections(selections);
  int selection = selections[0];
  wxString value = m_selectedCodecs->GetString(selection);
  MyMedia * media = (MyMedia *)m_selectedCodecs->GetClientData(selection);
  m_selectedCodecs->Delete(selection);
  m_selectedCodecs->InsertItems(1, &value, ++selection);
  m_selectedCodecs->SetClientData(selection, media);
  m_selectedCodecs->SetSelection(selection);
  m_MoveUpCodec->Enable(true);
  m_MoveDownCodec->Enable(selection < (int)m_selectedCodecs->GetCount()-1);
}


void OptionsDialog::SelectedCodecToAdd(wxCommandEvent & /*event*/)
{
  wxArrayInt selections;
  m_AddCodec->Enable(m_allCodecs->GetSelections(selections) > 0);
}


void OptionsDialog::SelectedCodec(wxCommandEvent & /*event*/)
{
  wxArrayInt selections;
  size_t count = m_selectedCodecs->GetSelections(selections);
  m_RemoveCodec->Enable(count > 0);
  m_MoveUpCodec->Enable(count == 1 && selections[0] > 0);
  m_MoveDownCodec->Enable(count == 1 && selections[0] < (int)m_selectedCodecs->GetCount()-1);

  m_codecOptions->DeleteAllItems();
  m_codecOptionValue->SetValue(wxT(""));
  m_codecOptionValue->Disable();
  m_CodecOptionValueLabel->Disable();

  if (count == 1) {
    MyMedia * media = (MyMedia *)m_selectedCodecs->GetClientData(selections[0]);
    PAssert(media != NULL, PLogicError);
    for (PINDEX i = 0; i < media->mediaFormat.GetOptionCount(); i++) {
      const OpalMediaOption & option = media->mediaFormat.GetOption(i);
      wxListItem item;
      item.m_mask = wxLIST_MASK_TEXT|wxLIST_MASK_DATA;
      item.m_itemId = LONG_MAX;
      item.m_text = PwxString(option.GetName());
      item.m_data = option.IsReadOnly();
      long index = m_codecOptions->InsertItem(item);
      m_codecOptions->SetItem(index, 1, PwxString(option.AsString()));
    }
  }
}


void OptionsDialog::SelectedCodecOption(wxListEvent & /*event*/)
{
  wxListItem item;
  item.m_mask = wxLIST_MASK_TEXT|wxLIST_MASK_DATA;
  item.m_itemId = m_codecOptions->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  item.m_col = 1;
  m_codecOptions->GetItem(item);
  m_codecOptionValue->Enable(!item.m_data);
  m_CodecOptionValueLabel->Enable(!item.m_data);
  if (!item.m_data)
    m_codecOptionValue->SetValue(item.m_text);
}


void OptionsDialog::DeselectedCodecOption(wxListEvent & /*event*/)
{
  m_codecOptionValue->SetValue(wxT(""));
  m_codecOptionValue->Disable();
  m_CodecOptionValueLabel->Disable();
}


void OptionsDialog::ChangedCodecOptionValue(wxCommandEvent & /*event*/)
{
  wxListItem item;
  item.m_itemId = m_codecOptions->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  if (item.m_itemId < 0)
    return;

  item.m_mask = wxLIST_MASK_TEXT;
  item.m_col = 1;
  m_codecOptions->GetItem(item);

  PwxString newValue = m_codecOptionValue->GetValue();
  if (item.m_text == newValue)
    return;

  wxArrayInt selections;
  PAssert(m_selectedCodecs->GetSelections(selections) == 1, PLogicError);
  MyMedia * media = (MyMedia *)m_selectedCodecs->GetClientData(selections[0]);
  if (!PAssert(media != NULL, PLogicError))
    return;

  item.m_col = 0;
  m_codecOptions->GetItem(item);
  bool ok = media->mediaFormat.SetOptionValue(item.m_text.c_str(), newValue);
  if (ok) {
    media->dirty = true;

    item.m_col = 1;
    item.m_text = newValue;
    m_codecOptions->SetItem(item);
  }

  m_CodecOptionValueError->Show(!ok);
}


////////////////////////////////////////
// H.323 fields

void OptionsDialog::SelectedAlias(wxCommandEvent & /*event*/)
{
  m_RemoveAlias->Enable(m_Aliases->GetSelection() != wxNOT_FOUND);
}


void OptionsDialog::ChangedNewAlias(wxCommandEvent & /*event*/)
{

  m_AddAlias->Enable(!m_NewAlias->GetValue().IsEmpty());
}


void OptionsDialog::AddAlias(wxCommandEvent & /*event*/)
{
  m_Aliases->Append(m_NewAlias->GetValue());
}


void OptionsDialog::RemoveAlias(wxCommandEvent & /*event*/)
{
  m_NewAlias->SetValue(m_Aliases->GetStringSelection());
  m_Aliases->Delete(m_Aliases->GetSelection());
  m_RemoveAlias->Disable();
}


////////////////////////////////////////
// SIP fields

void OptionsDialog::FieldsToRegistrar(RegistrarInfo & registrar)
{
  registrar.m_User = m_RegistrarUser->GetValue();
  registrar.m_Domain = m_RegistrarDomain->GetValue();
  registrar.m_AuthID = m_RegistrarAuthID->GetValue();
  registrar.m_Password = m_RegistrarPassword->GetValue();
  registrar.m_TimeToLive = m_RegistrarTimeToLive->GetValue();
  registrar.m_Active = m_RegistrarActive->GetValue();
  registrar.m_MWI = m_SubscribeMWI->GetValue();
  registrar.m_Presence = m_SubscribePresence->GetValue();
}


void OptionsDialog::RegistrarToList(bool overwrite, RegistrarInfo * registrar, int position)
{
  if (overwrite)
    m_Registrars->SetItem(position, 0, registrar->m_Domain);
  else {
    position = m_Registrars->InsertItem(position, registrar->m_Domain);
    m_Registrars->SetItemPtrData(position, (wxUIntPtr)registrar);
  }

  m_Registrars->SetItem(position, 1, registrar->m_User);

  wxString str;
  str.sprintf(wxT("%u:%02u"), registrar->m_TimeToLive/60, registrar->m_TimeToLive%60);
  m_Registrars->SetItem(position, 2, str);

  m_Registrars->SetItem(position, 3, registrar->m_Active   ? wxT("ACTIVE")   : wxT("disabled"));
  m_Registrars->SetItem(position, 4, registrar->m_MWI      ? wxT("subcribe") : wxT("disabled"));
  m_Registrars->SetItem(position, 5, registrar->m_Presence ? wxT("subcribe") : wxT("disabled"));
}


void OptionsDialog::AddRegistrar(wxCommandEvent & event)
{
  RegistrarInfo * registrar = new RegistrarInfo();
  FieldsToRegistrar(*registrar);
  RegistrarToList(false, registrar, m_SelectedRegistrar);
  if (m_SelectedRegistrar != INT_MAX) {
    m_Registrars->SetItemState(m_SelectedRegistrar, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    m_Registrars->SetItemState(m_SelectedRegistrar+1, 0, wxLIST_STATE_SELECTED);
  }
  ChangedRegistrarInfo(event);
}


void OptionsDialog::ChangeRegistrar(wxCommandEvent & event)
{
  RegistrarInfo * registrar = (RegistrarInfo *)m_Registrars->GetItemData(m_SelectedRegistrar);
  FieldsToRegistrar(*registrar);
  RegistrarToList(true, registrar, m_SelectedRegistrar);
  ChangedRegistrarInfo(event);
}


void OptionsDialog::RemoveRegistrar(wxCommandEvent & event)
{
  delete (RegistrarInfo *)m_Registrars->GetItemData(m_SelectedRegistrar);
  m_Registrars->DeleteItem(m_SelectedRegistrar);
  ChangedRegistrarInfo(event);
}


void OptionsDialog::SelectedRegistrar(wxListEvent & event)
{
  m_SelectedRegistrar = event.GetIndex();
  m_ChangeRegistrar->Enable(true);
  m_RemoveRegistrar->Enable(true);

  RegistrarInfo & registrar = *(RegistrarInfo *)m_Registrars->GetItemData(m_SelectedRegistrar);
  m_RegistrarUser->SetValue(registrar.m_User);
  m_RegistrarDomain->SetValue(registrar.m_Domain);
  m_RegistrarAuthID->SetValue(registrar.m_AuthID);
  m_RegistrarPassword->SetValue(registrar.m_Password);
  m_RegistrarTimeToLive->SetValue(registrar.m_TimeToLive);
  m_RegistrarActive->SetValue(registrar.m_Active);
  m_SubscribeMWI->SetValue(registrar.m_MWI);
  m_SubscribePresence->SetValue(registrar.m_Presence);

  ChangedRegistrarInfo(event);
}


void OptionsDialog::DeselectedRegistrar(wxListEvent & event)
{
  m_SelectedRegistrar = INT_MAX;
  m_ChangeRegistrar->Enable(false);
  m_RemoveRegistrar->Enable(false);
  ChangedRegistrarInfo(event);
}


void OptionsDialog::ChangedRegistrarInfo(wxCommandEvent & /*event*/)
{
  RegistrarInfo registrar;
  FieldsToRegistrar(registrar);

  bool add = !registrar.m_Domain.IsEmpty() && !registrar.m_User.IsEmpty();
  bool change = add && m_SelectedRegistrar != INT_MAX;

  // Check for uniqueness
  for (int i = 0; i < m_Registrars->GetItemCount(); ++i) {
    if (m_Registrars->GetItemText(i) == registrar.m_Domain) {
      add = false;
      if (i != m_SelectedRegistrar)
        change = false;
      break;
    }
  }

  m_AddRegistrar->Enable(add);
  m_ChangeRegistrar->Enable(change);
}


////////////////////////////////////////
// Routing fields

void OptionsDialog::AddRoute(wxCommandEvent & /*event*/)
{
  int pos = m_Routes->InsertItem(m_SelectedRoute, m_RouteSource->GetValue());
  m_Routes->SetItem(pos, 1, m_RouteDevice->GetValue());
  m_Routes->SetItem(pos, 2, m_RoutePattern->GetValue());
  m_Routes->SetItem(pos, 3, m_RouteDestination->GetValue());
}


void OptionsDialog::RemoveRoute(wxCommandEvent & /*event*/)
{
  wxListItem item;
  item.m_itemId = m_SelectedRoute;
  item.m_mask = wxLIST_MASK_TEXT;
  m_Routes->GetItem(item);
  m_RouteSource->SetValue(item.m_text);
  item.m_col++;
  m_Routes->GetItem(item);
  m_RouteDevice->SetValue(item.m_text);
  item.m_col++;
  m_Routes->GetItem(item);
  m_RoutePattern->SetValue(item.m_text);
  item.m_col++;
  m_Routes->GetItem(item);
  m_RouteDestination->SetValue(item.m_text);

  m_Routes->DeleteItem(m_SelectedRoute);
}


static int MoveRoute(wxListCtrl * routes, int selection, int delta)
{
  wxStringList cols;
  wxListItem item;
  item.m_itemId = selection;
  item.m_mask = wxLIST_MASK_TEXT;
  for (item.m_col = 0; item.m_col < routes->GetColumnCount(); item.m_col++) {
    routes->GetItem(item);
    cols.Add(item.m_text);
  }

  routes->DeleteItem(selection);
  selection += delta;
  routes->InsertItem(selection, cols.front());

  item.m_itemId = selection;
  for (item.m_col = 1; item.m_col < routes->GetColumnCount(); item.m_col++) {
    item.m_text = cols[item.m_col];
    routes->SetItem(item);
  }

  routes->SetItemState(selection, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
  return selection;
}


void OptionsDialog::MoveUpRoute(wxCommandEvent & /*event*/)
{
  m_SelectedRoute = MoveRoute(m_Routes, m_SelectedRoute, -1);
  m_MoveUpRoute->Enable(m_SelectedRoute > 0);
  m_MoveDownRoute->Enable(true);
}


void OptionsDialog::MoveDownRoute(wxCommandEvent & /*event*/)
{
  m_SelectedRoute = MoveRoute(m_Routes, m_SelectedRoute, 1);
  m_MoveUpRoute->Enable(true);
  m_MoveDownRoute->Enable(m_SelectedRoute < (int)m_Routes->GetItemCount()-1);
}


void OptionsDialog::SelectedRoute(wxListEvent & event)
{
  m_SelectedRoute = event.GetIndex();
  m_RemoveRoute->Enable(true);
  m_MoveUpRoute->Enable(m_SelectedRoute > 0);
  m_MoveDownRoute->Enable(m_SelectedRoute < (int)m_Routes->GetItemCount()-1);
}


void OptionsDialog::DeselectedRoute(wxListEvent & /*event*/)
{
  m_SelectedRoute = INT_MAX;
  m_RemoveRoute->Enable(false);
  m_MoveUpRoute->Enable(false);
  m_MoveDownRoute->Enable(false);
}


void OptionsDialog::ChangedRouteInfo(wxCommandEvent & /*event*/)
{
  m_AddRoute->Enable(!m_RoutePattern->GetValue().IsEmpty() && !m_RouteDestination->GetValue().IsEmpty());
}


void OptionsDialog::RestoreDefaultRoutes(wxCommandEvent &)
{
  m_Routes->DeleteAllItems();

  for (PINDEX i = 0; i < PARRAYSIZE(DefaultRoutes); i++) {
    PString spec = DefaultRoutes[i];
    PINDEX equal = spec.Find('=');
    if (equal != P_MAX_INDEX)
      AddRouteTableEntry(OpalManager::RouteEntry(spec.Left(equal).Trim(), spec.Mid(equal+1).Trim()));
  }
}


void OptionsDialog::AddRouteTableEntry(OpalManager::RouteEntry entry)
{
  PString expression = entry.pattern;

  PINDEX tab = expression.Find('\t');
  if (tab == P_MAX_INDEX)
    tab = expression.Find("\\t");

  PINDEX colon = expression.Find(':');

  PwxString source, device, pattern;
  if (colon >= tab) {
    source = AllRouteSources;
    device = (const char *)expression(colon+1, tab-1);
    pattern = expression.Mid(tab+1);
  }
  else {
    source = expression.Left(colon);
    if (source == ".*")
      source = AllRouteSources;
    if (tab == P_MAX_INDEX)
      pattern = expression.Mid(colon+1);
    else {
      device = expression(colon+1, tab-1);
      if (device == ".*")
        device = "";
      pattern = expression.Mid(tab + (expression[tab] == '\t' ? 1 : 2));
    }
  }

  int pos = m_Routes->InsertItem(INT_MAX, source);
  m_Routes->SetItem(pos, 1, device);
  m_Routes->SetItem(pos, 2, pattern);
  m_Routes->SetItem(pos, 3, PwxString(entry.destination));
}


#if PTRACING
////////////////////////////////////////
// Tracing fields

void OptionsDialog::BrowseTraceFile(wxCommandEvent & /*event*/)
{
  wxString newFile = wxFileSelector(wxT("Trace log file"),
                                    wxT(""),
                                    m_TraceFileName,
                                    wxT(".log"),
                                    wxT("Log Files (*.log)|*.log|Text Files (*.txt)|*.txt||"),
                                    wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
  if (!newFile.empty()) {
    m_TraceFileName = newFile;
    TransferDataToWindow();
  }
}
#endif



///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(VideoControlDialog, wxDialog)
END_EVENT_TABLE()

VideoControlDialog::VideoControlDialog(MyManager * manager)
  : m_manager(*manager)
{
  wxXmlResource::Get()->LoadDialog(this, manager, wxT("VideoControlDialog"));

  m_TargetBitRate = FindWindowByNameAs<wxSlider>(this, wxT("VideoBitRate"));

  PSafePtr<OpalConnection> connection = m_manager.GetConnection(false, PSafeReadOnly);
  if (connection != NULL) {
    OpalMediaStreamPtr stream = connection->GetMediaStream(OpalMediaType::Video(), false);
    if (stream != NULL) {
      OpalMediaFormat mediaFormat = stream->GetMediaFormat();
      m_TargetBitRate->SetMax(mediaFormat.GetOptionInteger(OpalMediaFormat::MaxBitRateOption())/1000);
      m_TargetBitRate->SetValue(mediaFormat.GetOptionInteger(OpalMediaFormat::TargetBitRateOption())/1000);
      m_TargetBitRate->SetTickFreq(m_TargetBitRate->GetMax()/10,1);
    }
  }
}


bool VideoControlDialog::TransferDataFromWindow()
{
  if (!wxDialog::TransferDataFromWindow())
    return false;

  PSafePtr<OpalConnection> connection = m_manager.GetConnection(false, PSafeReadOnly);
  if (connection != NULL) {
    OpalMediaStreamPtr stream = connection->GetMediaStream(OpalMediaType::Video(), false);
    if (stream != NULL) {
      OpalMediaFormat mediaFormat = stream->GetMediaFormat();
      mediaFormat.SetOptionInteger(OpalVideoFormat::TargetBitRateOption(), m_TargetBitRate->GetValue()*1000);
      stream->UpdateMediaFormat(mediaFormat);
    }
  }

  return true;
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(CallDialog, wxDialog)
  EVT_BUTTON(XRCID("wxID_OK"), CallDialog::OnOK)
  EVT_TEXT(XRCID("Address"), CallDialog::OnAddressChange)
END_EVENT_TABLE()

CallDialog::CallDialog(MyManager * manager, bool hideHandset)
  : m_UseHandset(manager->HasHandset())
{
  wxXmlResource::Get()->LoadDialog(this, manager, wxT("CallDialog"));

  m_ok = FindWindowByNameAs<wxButton>(this, wxT("wxID_OK"));
  m_ok->Disable();

  wxCheckBox * useHandset = FindWindowByNameAs<wxCheckBox>(this, wxT("UseHandset"));
  useHandset->SetValidator(wxGenericValidator(&m_UseHandset));
  if (hideHandset)
    useHandset->Hide();
  else
    useHandset->Enable(m_UseHandset);

  m_AddressCtrl = FindWindowByNameAs<wxComboBox>(this, wxT("Address"));
  m_AddressCtrl->SetValidator(wxGenericValidator(&m_Address));

  wxConfigBase * config = wxConfig::Get();
  config->SetPath(RecentCallsGroup);
  wxString entryName;
  long entryIndex;
  if (config->GetFirstEntry(entryName, entryIndex)) {
    do {
      wxString address;
      if (config->Read(entryName, &address))
        m_AddressCtrl->AppendString(address);
    } while (config->GetNextEntry(entryName, entryIndex));
  }
}


void CallDialog::OnOK(wxCommandEvent &)
{
  wxConfigBase * config = wxConfig::Get();
  config->DeleteGroup(RecentCallsGroup);
  config->SetPath(RecentCallsGroup);

  config->Write(wxT("1"), m_Address);

  unsigned keyNumber = 1;
  for (size_t i = 0; i < m_AddressCtrl->GetCount() && keyNumber < MaxSavedRecentCalls; ++i) {
    wxString entry = m_AddressCtrl->GetString(i);

    if (m_Address != entry) {
      wxString key;
      key.sprintf(wxT("%u"), ++keyNumber);
      config->Write(key, entry);
    }
  }

  EndModal(wxID_OK);
}


void CallDialog::OnAddressChange(wxCommandEvent & WXUNUSED(event))
{
  TransferDataFromWindow();
  m_ok->Enable(!m_Address.IsEmpty());
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(AnswerPanel, wxPanel)
  EVT_BUTTON(XRCID("AnswerCall"), AnswerPanel::OnAnswer)
  EVT_BUTTON(XRCID("RejectCall"), AnswerPanel::OnReject)
  EVT_RADIOBOX(XRCID("AnswerAs"), AnswerPanel::OnChangeAnswerMode)
END_EVENT_TABLE()

AnswerPanel::AnswerPanel(MyManager & manager, wxWindow * parent)
  : m_manager(manager)
{
  wxXmlResource::Get()->LoadPanel(this, parent, wxT("AnswerPanel"));
}


void AnswerPanel::SetPartyNames(const PwxString & calling, const PwxString & called)
{
  FindWindowByNameAs<wxStaticText>(this, wxT("CallingParty"))->SetLabel(calling);
  FindWindowByNameAs<wxStaticText>(this, wxT("CalledParty"))->SetLabel(called);
  FindWindowByNameAs<wxStaticText>(this, wxT("CalledParty"))->SetLabel(called);
}


void AnswerPanel::OnAnswer(wxCommandEvent & /*event*/)
{
  m_manager.AnswerCall();
}


void AnswerPanel::OnReject(wxCommandEvent & /*event*/)
{
  m_manager.RejectCall();
}


void AnswerPanel::OnChangeAnswerMode(wxCommandEvent & theEvent)
{
  m_manager.m_AnswerMode = (MyManager::AnswerModes)theEvent.GetSelection();
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(CallingPanel, wxPanel)
  EVT_BUTTON(XRCID("HangUpCall"), CallingPanel::OnHangUp)
END_EVENT_TABLE()

CallingPanel::CallingPanel(MyManager & manager, wxWindow * parent)
  : m_manager(manager)
{
  wxXmlResource::Get()->LoadPanel(this, parent, wxT("CallingPanel"));
}


void CallingPanel::OnHangUp(wxCommandEvent & /*event*/)
{
  m_manager.HangUpCall();
}


///////////////////////////////////////////////////////////////////////////////

const int VU_UPDATE_TIMER_ID = 1000;

BEGIN_EVENT_TABLE(InCallPanel, wxPanel)
  EVT_BUTTON(XRCID("HangUp"), InCallPanel::OnHangUp)
  EVT_BUTTON(XRCID("Hold"), InCallPanel::OnRequestHold)
  EVT_CHECKBOX(XRCID("SpeakerMute"), InCallPanel::OnSpeakerMute)
  EVT_CHECKBOX(XRCID("MicrophoneMute"), InCallPanel::OnMicrophoneMute)
  EVT_BUTTON(XRCID("Input1"), InCallPanel::OnUserInput1)
  EVT_BUTTON(XRCID("Input2"), InCallPanel::OnUserInput2)
  EVT_BUTTON(XRCID("Input3"), InCallPanel::OnUserInput3)
  EVT_BUTTON(XRCID("Input4"), InCallPanel::OnUserInput4)
  EVT_BUTTON(XRCID("Input5"), InCallPanel::OnUserInput5)
  EVT_BUTTON(XRCID("Input6"), InCallPanel::OnUserInput6)
  EVT_BUTTON(XRCID("Input7"), InCallPanel::OnUserInput7)
  EVT_BUTTON(XRCID("Input8"), InCallPanel::OnUserInput8)
  EVT_BUTTON(XRCID("Input9"), InCallPanel::OnUserInput9)
  EVT_BUTTON(XRCID("Input0"), InCallPanel::OnUserInput0)
  EVT_BUTTON(XRCID("InputStar"), InCallPanel::OnUserInputStar)
  EVT_BUTTON(XRCID("InputHash"), InCallPanel::OnUserInputHash)
  EVT_BUTTON(XRCID("InputFlash"), InCallPanel::OnUserInputFlash)

  EVT_COMMAND_SCROLL(XRCID("SpeakerVolume"), InCallPanel::SpeakerVolume)
  EVT_COMMAND_SCROLL(XRCID("MicrophoneVolume"), InCallPanel::MicrophoneVolume)

  EVT_TIMER(VU_UPDATE_TIMER_ID, InCallPanel::OnUpdateVU)
END_EVENT_TABLE()


InCallPanel::InCallPanel(MyManager & manager, wxWindow * parent)
  : m_manager(manager)
  , m_vuTimer(this, VU_UPDATE_TIMER_ID)
  , m_updateStatistics(0)
  , m_SwitchingHold(false)
{
  wxXmlResource::Get()->LoadPanel(this, parent, wxT("InCallPanel"));

  m_Hold = FindWindowByNameAs<wxButton>(this, wxT("Hold"));
  m_SpeakerHandset = FindWindowByNameAs<wxButton>(this, wxT("SpeakerHandset"));
  m_SpeakerMute = FindWindowByNameAs<wxCheckBox>(this, wxT("SpeakerMute"));
  m_MicrophoneMute = FindWindowByNameAs<wxCheckBox>(this, wxT("MicrophoneMute"));
  m_SpeakerVolume = FindWindowByNameAs<wxSlider>(this, wxT("SpeakerVolume"));
  m_MicrophoneVolume = FindWindowByNameAs<wxSlider>(this, wxT("MicrophoneVolume"));
  m_vuSpeaker = FindWindowByNameAs<wxGauge>(this, wxT("SpeakerGauge"));
  m_vuMicrophone = FindWindowByNameAs<wxGauge>(this, wxT("MicrophoneGauge"));

  m_vuTimer.Start(250);

  m_pages[RxAudio].Init(this, RxAudio, OpalMediaType::Audio(), true );
  m_pages[TxAudio].Init(this, TxAudio, OpalMediaType::Audio(), false);
  m_pages[RxVideo].Init(this, RxVideo, OpalMediaType::Video(), true );
  m_pages[TxVideo].Init(this, TxVideo, OpalMediaType::Video(), false);
  m_pages[RxFax  ].Init(this, RxFax  , OpalMediaType::Fax(),   true);
  m_pages[TxFax  ].Init(this, TxFax  , OpalMediaType::Fax(),   false);

  m_FirstTime = true;
}


bool InCallPanel::Show(bool show)
{
  wxConfigBase * config = wxConfig::Get();
  config->SetPath(AudioGroup);

  if (show || m_FirstTime) {
    m_FirstTime = false;

    int value = 50;
    config->Read(SpeakerVolumeKey, &value);
    m_SpeakerVolume->SetValue(value);
    bool mute = false;
    config->Read(SpeakerMuteKey, &mute);
    m_SpeakerMute->SetValue(!mute);
    SetVolume(false, value, mute);

    value = 50;
    config->Read(MicrophoneVolumeKey, &value);
    m_MicrophoneVolume->SetValue(value);
    mute = false;
    config->Read(MicrophoneMuteKey, &mute);
    m_MicrophoneMute->SetValue(!mute);
    SetVolume(true, value, mute);
  }
  else {
    config->Write(SpeakerVolumeKey, m_SpeakerVolume->GetValue());
    config->Write(SpeakerMuteKey, !m_SpeakerMute->GetValue());
    config->Write(MicrophoneVolumeKey, m_MicrophoneVolume->GetValue());
    config->Write(MicrophoneMuteKey, !m_MicrophoneMute->GetValue());
  }

  return wxPanel::Show(show);
}


void InCallPanel::OnStreamsChanged()
{
  // Must do this before getting lock on OpalCall to avoid deadlock
  m_SpeakerHandset->Enable(m_manager.HasHandset());

  PSafePtr<OpalConnection> connection = m_manager.GetConnection(false, PSafeReadOnly);

  for (PINDEX i = 0; i < NumPages; i++)
    m_pages[i].UpdateSession(connection);
}


void InCallPanel::OnHangUp(wxCommandEvent & /*event*/)
{
  m_manager.HangUpCall();
}


void InCallPanel::OnHoldChanged(bool onHold)
{
  bool anyCallsOnHols = !m_manager.m_callsOnHold.empty();
  if (onHold && m_SwitchingHold && anyCallsOnHols)
    m_manager.m_callsOnHold.front().m_call->Retrieve();
  m_SwitchingHold = false;

  m_Hold->SetLabel(anyCallsOnHols ? wxT("Switch") : wxT("Hold"));
  m_Hold->Enable(true);
}


void InCallPanel::OnRequestHold(wxCommandEvent & /*event*/)
{
  PSafePtr<OpalCall> call = m_manager.GetCall(PSafeReadWrite);
  if (call != NULL) {
    if (!call->Hold())
      return;

    m_SwitchingHold = !m_manager.m_callsOnHold.empty();
  }

  m_Hold->SetLabel(wxT("In Progress"));
  m_Hold->Enable(false);
}


void InCallPanel::OnSpeakerMute(wxCommandEvent & event)
{
  SetVolume(false, m_SpeakerVolume->GetValue(), !event.IsChecked());
}


void InCallPanel::OnMicrophoneMute(wxCommandEvent & event)
{
  SetVolume(true, m_MicrophoneVolume->GetValue(), !event.IsChecked());
}


#define ON_USER_INPUT_HANDLER(i,c) \
  void InCallPanel::OnUserInput##i(wxCommandEvent &) \
  { m_manager.SendUserInput(c); }

ON_USER_INPUT_HANDLER(1,'1')
ON_USER_INPUT_HANDLER(2,'2')
ON_USER_INPUT_HANDLER(3,'3')
ON_USER_INPUT_HANDLER(4,'4')
ON_USER_INPUT_HANDLER(5,'5')
ON_USER_INPUT_HANDLER(6,'6')
ON_USER_INPUT_HANDLER(7,'7')
ON_USER_INPUT_HANDLER(8,'8')
ON_USER_INPUT_HANDLER(9,'9')
ON_USER_INPUT_HANDLER(0,'0')
ON_USER_INPUT_HANDLER(Star,'*')
ON_USER_INPUT_HANDLER(Hash,'#')
ON_USER_INPUT_HANDLER(Flash,'!')


void InCallPanel::SpeakerVolume(wxScrollEvent & event)
{
  SetVolume(false, event.GetPosition(), !m_SpeakerMute->GetValue());
}


void InCallPanel::MicrophoneVolume(wxScrollEvent & event)
{
  SetVolume(true, event.GetPosition(), !m_MicrophoneMute->GetValue());
}


void InCallPanel::SetVolume(bool isMicrophone, int value, bool muted)
{
  PSafePtr<OpalConnection> connection = m_manager.GetConnection(true, PSafeReadOnly);
  if (connection != NULL)
    connection->SetAudioVolume(isMicrophone, muted ? 0 : value);
}


static void SetGauge(wxGauge * gauge, int level)
{
  if (level < 0 || level > 32767) {
    gauge->Show(false);
    return;
  }
  gauge->Show();
  gauge->SetValue((int)(100*log10(1.0 + 9.0*level/8192.0))); // Convert to logarithmic scale
}


void InCallPanel::OnUpdateVU(wxTimerEvent& WXUNUSED(event))
{
  if (IsShown()) {
    if (++m_updateStatistics > 8) {
      PSafePtr<OpalConnection> connection = m_manager.GetConnection(false, PSafeReadOnly);
      for (PINDEX i = 0; i < NumPages; i++)
        m_pages[i].UpdateSession(connection);
      m_updateStatistics = 0;
    }

    int micLevel = -1;
    int spkLevel = -1;
    PSafePtr<OpalConnection> connection = m_manager.GetConnection(true, PSafeReadOnly);
    if (connection != NULL) {
      spkLevel = connection->GetAudioSignalLevel(false);
      micLevel = connection->GetAudioSignalLevel(true);
    }

    SetGauge(m_vuSpeaker, spkLevel);
    SetGauge(m_vuMicrophone, micLevel);
  }
}


static vector<StatisticsField *> StatisticsFieldTemplates;

StatisticsField::StatisticsField(const wxChar * name, StatisticsPages page)
  : m_name(name)
  , m_page(page)
  , m_staticText(NULL)
  , m_lastBytes(0)
  , m_lastFrames(0)
{
  StatisticsFieldTemplates.push_back(this);
}


void StatisticsField::Init(wxWindow * panel)
{
  m_staticText = FindWindowByNameAs<wxStaticText>(panel, m_name);
  m_printFormat = m_staticText->GetLabel();
  Clear();
}


void StatisticsField::Clear()
{
  m_staticText->SetLabel(wxT("N/A"));
  m_lastBandwidthTick = 0;
}


double StatisticsField::CalculateBandwidth(DWORD bytes)
{
  PTimeInterval tick = PTimer::Tick();

  double value;
  if (m_lastBandwidthTick != 0)
    value = 8.0 * (bytes - m_lastBytes) / (tick - m_lastBandwidthTick).GetMilliSeconds(); // Ends up as kilobits/second
  else
    value = 0;

  m_lastBandwidthTick = tick;
  m_lastBytes = bytes;

  return value;
}


double StatisticsField::CalculateFrameRate(DWORD frames)
{
  PTimeInterval tick = PTimer::Tick();

  double value;
  if (m_lastFrameTick != 0)
    value = (frames - m_lastFrames) / (double)(tick - m_lastFrameTick).GetSeconds(); // Ends up as frames/second
  else
    value = 0;

  m_lastFrameTick = tick;
  m_lastFrames = frames;

  return value;
}


void StatisticsField::Update(const OpalConnection & connection, const OpalMediaStream & stream)
{
  wxString value;
  OpalMediaStatistics statistics;
  stream.GetStatistics(statistics);
  GetValue(connection, stream, statistics, value);
  m_staticText->SetLabel(value);
}


#define STATISTICS_FIELD_BEG(type, name) \
  class type##name##StatisticsField : public StatisticsField { \
  public: type##name##StatisticsField() : StatisticsField(wxT(#type #name), type) { } \
    virtual StatisticsField * Clone() const { return new type##name##StatisticsField(*this); } \
    virtual void GetValue(const OpalConnection & connection, const OpalMediaStream & stream, const OpalMediaStatistics & statistics, wxString & value) {

#define STATISTICS_FIELD_END(type, name) \
    } } Static##type##name##StatisticsField;

#ifdef _MSC_VER
#pragma warning(disable:4100)
#endif

STATISTICS_FIELD_BEG(RxAudio, Bandwidth)
  value.sprintf(m_printFormat, CalculateBandwidth(statistics.m_totalBytes));
STATISTICS_FIELD_END(RxAudio, Bandwidth)

STATISTICS_FIELD_BEG(RxAudio, MinTime)
  value.sprintf(m_printFormat, statistics.m_minimumPacketTime);
STATISTICS_FIELD_END(RxAudio, MinTime)

STATISTICS_FIELD_BEG(RxAudio, Bytes)
  value.sprintf(m_printFormat, statistics.m_totalBytes);
STATISTICS_FIELD_END(RxAudio, Bytes)

STATISTICS_FIELD_BEG(RxAudio, AvgTime)
  value.sprintf(m_printFormat, statistics.m_averagePacketTime);
STATISTICS_FIELD_END(RxAudio, AvgTime)

STATISTICS_FIELD_BEG(RxAudio, Packets)
  value.sprintf(m_printFormat, statistics.m_totalPackets);
STATISTICS_FIELD_END(RxAudio, Packets)

STATISTICS_FIELD_BEG(RxAudio, MaxTime)
  value.sprintf(m_printFormat, statistics.m_maximumPacketTime);
STATISTICS_FIELD_END(RxAudio, MaxTime)

STATISTICS_FIELD_BEG(RxAudio, Lost)
  value.sprintf(m_printFormat, statistics.m_packetsLost);
STATISTICS_FIELD_END(RxAudio, Lost)

STATISTICS_FIELD_BEG(RxAudio, AvgJitter)
  value.sprintf(m_printFormat, statistics.m_averageJitter);
STATISTICS_FIELD_END(RxAudio, AvgJitter)

STATISTICS_FIELD_BEG(RxAudio, OutOfOrder)
  value.sprintf(m_printFormat, statistics.m_packetsOutOfOrder);
STATISTICS_FIELD_END(RxAudio, OutOfOrder)

STATISTICS_FIELD_BEG(RxAudio, MaxJitter)
  value.sprintf(m_printFormat, statistics.m_maximumJitter);
STATISTICS_FIELD_END(RxAudio, MaxJitter)

STATISTICS_FIELD_BEG(RxAudio, TooLate)
  value.sprintf(m_printFormat, statistics.m_packetsTooLate);
STATISTICS_FIELD_END(RxAudio, TooLate)

STATISTICS_FIELD_BEG(RxAudio, Overruns)
  value.sprintf(m_printFormat, statistics.m_packetOverruns);
STATISTICS_FIELD_END(RxAudio, Overruns)

STATISTICS_FIELD_BEG(TxAudio, Bandwidth)
  value.sprintf(m_printFormat, CalculateBandwidth(statistics.m_totalBytes));
STATISTICS_FIELD_END(TxAudio, Bandwidth)

STATISTICS_FIELD_BEG(TxAudio, MinTime)
  value.sprintf(m_printFormat, statistics.m_minimumPacketTime);
STATISTICS_FIELD_END(TxAudio, MinTime)

STATISTICS_FIELD_BEG(TxAudio, Bytes)
  value.sprintf(m_printFormat, statistics.m_totalBytes);
STATISTICS_FIELD_END(TxAudio, Bytes)

STATISTICS_FIELD_BEG(TxAudio, AvgTime)
  value.sprintf(m_printFormat, statistics.m_averagePacketTime);
STATISTICS_FIELD_END(TxAudio, AvgTime)

STATISTICS_FIELD_BEG(TxAudio, Packets)
  value.sprintf(m_printFormat, statistics.m_totalPackets);
STATISTICS_FIELD_END(TxAudio, Packets)

STATISTICS_FIELD_BEG(TxAudio, MaxTime)
  value.sprintf(m_printFormat, statistics.m_maximumPacketTime);
STATISTICS_FIELD_END(TxAudio, MaxTime)

STATISTICS_FIELD_BEG(RxVideo, Bandwidth)
  value.sprintf(m_printFormat, CalculateBandwidth(statistics.m_totalBytes));
STATISTICS_FIELD_END(RxVideo, Bandwidth)

STATISTICS_FIELD_BEG(RxVideo, MinTime)
  value.sprintf(m_printFormat, statistics.m_minimumPacketTime);
STATISTICS_FIELD_END(RxVideo, MinTime)

STATISTICS_FIELD_BEG(RxVideo, Bytes)
  value.sprintf(m_printFormat, statistics.m_totalBytes);
STATISTICS_FIELD_END(RxVideo, Bytes)

STATISTICS_FIELD_BEG(RxVideo, AvgTime)
  value.sprintf(m_printFormat, statistics.m_averagePacketTime);
STATISTICS_FIELD_END(RxVideo, AvgTime)

STATISTICS_FIELD_BEG(RxVideo, Packets)
  value.sprintf(m_printFormat, statistics.m_totalPackets);
STATISTICS_FIELD_END(RxVideo, Packets)

STATISTICS_FIELD_BEG(RxVideo, MaxTime)
  value.sprintf(m_printFormat, statistics.m_maximumPacketTime);
STATISTICS_FIELD_END(RxVideo, MaxTime)

STATISTICS_FIELD_BEG(RxVideo, Lost)
  value.sprintf(m_printFormat, statistics.m_packetsLost);
STATISTICS_FIELD_END(RxVideo, Lost)

STATISTICS_FIELD_BEG(RxVideo, AvgJitter)
  value.sprintf(m_printFormat, statistics.m_averageJitter);
STATISTICS_FIELD_END(RxVideo, AvgJitter)

STATISTICS_FIELD_BEG(RxVideo, OutOfOrder)
  value.sprintf(m_printFormat, statistics.m_packetsOutOfOrder);
STATISTICS_FIELD_END(RxVideo, OutOfOrder)

STATISTICS_FIELD_BEG(RxVideo, MaxJitter)
  value.sprintf(m_printFormat, statistics.m_maximumJitter);
STATISTICS_FIELD_END(RxVideo, MaxJitter)

STATISTICS_FIELD_BEG(RxVideo, Frames)
  value.sprintf(m_printFormat, statistics.m_totalFrames);
STATISTICS_FIELD_END(RxVideo, Frames)

STATISTICS_FIELD_BEG(RxVideo, KeyFrames)
  value.sprintf(m_printFormat, statistics.m_keyFrames);
STATISTICS_FIELD_END(RxVideo, KeyFrames)

STATISTICS_FIELD_BEG(RxVideo, FrameRate)
  value.sprintf(m_printFormat, CalculateFrameRate(statistics.m_totalFrames));
STATISTICS_FIELD_END(RxVideo, FrameRate)

STATISTICS_FIELD_BEG(RxVideo, VFU)
  value.sprintf(m_printFormat, connection.GetVideoUpdateRequestsSent());
STATISTICS_FIELD_END(RxVideo, VFU)

STATISTICS_FIELD_BEG(TxVideo, Bandwidth)
  value.sprintf(m_printFormat, CalculateBandwidth(statistics.m_totalBytes));
STATISTICS_FIELD_END(TxVideo, Bandwidth)

STATISTICS_FIELD_BEG(TxVideo, MinTime)
  value.sprintf(m_printFormat, statistics.m_minimumPacketTime);
STATISTICS_FIELD_END(TxVideo, MinTime)

STATISTICS_FIELD_BEG(TxVideo, Bytes)
  value.sprintf(m_printFormat, statistics.m_totalBytes);
STATISTICS_FIELD_END(TxVideo, Bytes)

STATISTICS_FIELD_BEG(TxVideo, AvgTime)
  value.sprintf(m_printFormat, statistics.m_averagePacketTime);
STATISTICS_FIELD_END(TxVideo, AvgTime)

STATISTICS_FIELD_BEG(TxVideo, Packets)
  value.sprintf(m_printFormat, statistics.m_totalPackets);
STATISTICS_FIELD_END(TxVideo, Packets)

STATISTICS_FIELD_BEG(TxVideo, MaxTime)
  value.sprintf(m_printFormat, statistics.m_maximumPacketTime);
STATISTICS_FIELD_END(TxVideo, MaxTime)

STATISTICS_FIELD_BEG(TxVideo, Frames)
  value.sprintf(m_printFormat, statistics.m_totalFrames);
STATISTICS_FIELD_END(TxVideo, Frames)

STATISTICS_FIELD_BEG(TxVideo, KeyFrames)
  value.sprintf(m_printFormat, statistics.m_keyFrames);
STATISTICS_FIELD_END(TxVideo, KeyFrames)

STATISTICS_FIELD_BEG(TxVideo, FrameRate)
  value.sprintf(m_printFormat, CalculateFrameRate(statistics.m_totalFrames));
STATISTICS_FIELD_END(TxVideo, FrameRate)

STATISTICS_FIELD_BEG(RxFax, Bandwidth)
  value.sprintf(m_printFormat, CalculateBandwidth(statistics.m_totalBytes));
STATISTICS_FIELD_END(RxFax, Bandwidth)

STATISTICS_FIELD_BEG(RxFax, Bytes)
  value.sprintf(m_printFormat, statistics.m_totalBytes);
STATISTICS_FIELD_END(RxFax, Bytes)

STATISTICS_FIELD_BEG(RxFax, Packets)
  value.sprintf(m_printFormat, statistics.m_totalPackets);
STATISTICS_FIELD_END(RxFax, Packets)

STATISTICS_FIELD_BEG(TxFax, Bandwidth)
  value.sprintf(m_printFormat, CalculateBandwidth(statistics.m_totalBytes));
STATISTICS_FIELD_END(TxFax, Bandwidth)

STATISTICS_FIELD_BEG(TxFax, Bytes)
  value.sprintf(m_printFormat, statistics.m_totalBytes);
STATISTICS_FIELD_END(TxFax, Bytes)

STATISTICS_FIELD_BEG(TxFax, Packets)
  value.sprintf(m_printFormat, statistics.m_totalPackets);
STATISTICS_FIELD_END(TxFax, Packets)

#ifdef _MSC_VER
#pragma warning(default:4100)
#endif


StatisticsPage::StatisticsPage()
  : m_page(NumPages)
  , m_receiver(false)
  , m_isActive(false)
  , m_window(NULL)
{
}


StatisticsPage::~StatisticsPage()
{
  for (size_t i = 0; i < m_fields.size(); i++)
    delete m_fields[i];
}


void StatisticsPage::Init(InCallPanel * panel,
                          StatisticsPages page,
                          const OpalMediaType & mediaType,
                          bool receiver)
{
  m_page = page;
  m_mediaType = mediaType;
  m_receiver = receiver;
  m_isActive = false;

  wxNotebook * book = FindWindowByNameAs<wxNotebook>(panel, wxT("Statistics"));
  m_window = book->GetPage(page > RxTxFax ? RxTxFax : page);

  for (size_t i = 0; i < StatisticsFieldTemplates.size(); i++) {
    if (StatisticsFieldTemplates[i]->m_page == page) {
      StatisticsField * field = StatisticsFieldTemplates[i]->Clone();
      field->Init(panel);
      m_fields.push_back(field);
    }
  }
}


void StatisticsPage::UpdateSession(const OpalConnection * connection)
{
  if (connection == NULL)
    m_isActive = false;
  else {
    OpalMediaStreamPtr stream = connection->GetMediaStream(m_mediaType, m_receiver);
    m_isActive = stream != NULL && stream->Open();
    if (m_isActive) {
      for (size_t i = 0; i < m_fields.size(); i++)
        m_fields[i]->Update(*connection, *stream);
    }
  }

  m_window->Enable(m_isActive);

  if (!m_isActive) {
    for (size_t i = 0; i < m_fields.size(); i++)
      m_fields[i]->Clear();
  }
}


///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(SpeedDialDialog, wxDialog)
  EVT_TEXT(XRCID("SpeedDialName"), SpeedDialDialog::OnChange)
  EVT_TEXT(XRCID("SpeedDialNumber"), SpeedDialDialog::OnChange)
  EVT_TEXT(XRCID("SpeedDialAddress"), SpeedDialDialog::OnChange)
END_EVENT_TABLE()

SpeedDialDialog::SpeedDialDialog(MyManager * manager)
  : m_manager(*manager)
{
  wxXmlResource::Get()->LoadDialog(this, manager, wxT("SpeedDialDialog"));

  m_ok = FindWindowByNameAs<wxButton>(this, wxT("wxID_OK"));

  m_nameCtrl = FindWindowByNameAs<wxTextCtrl>(this, wxT("SpeedDialName"));
  m_nameCtrl->SetValidator(wxGenericValidator(&m_Name));

  m_numberCtrl = FindWindowByNameAs<wxTextCtrl>(this, wxT("SpeedDialNumber"));
  m_numberCtrl->SetValidator(wxGenericValidator(&m_Number));

  m_addressCtrl = FindWindowByNameAs<wxTextCtrl>(this, wxT("SpeedDialAddress"));
  m_addressCtrl->SetValidator(wxGenericValidator(&m_Address));

  FindWindowByName(wxT("SpeedDialDescription"))->SetValidator(wxGenericValidator(&m_Description));

  m_inUse = FindWindowByNameAs<wxStaticText>(this, wxT("SpeedDialInUse"));
  m_ambiguous = FindWindowByNameAs<wxStaticText>(this, wxT("SpeedDialAmbiguous"));
}


void SpeedDialDialog::OnChange(wxCommandEvent & WXUNUSED(event))
{
  wxString newName = m_nameCtrl->GetValue();
  bool inUse = newName != m_Name && m_manager.HasSpeedDialName(newName);
  m_inUse->Show(inUse);

  m_ok->Enable(!inUse && !newName.IsEmpty() && !m_addressCtrl->GetValue().IsEmpty());

  m_ambiguous->Show(m_manager.HasSpeedDialNumber(m_numberCtrl->GetValue(), m_Number));
}


///////////////////////////////////////////////////////////////////////////////

MyPCSSEndPoint::MyPCSSEndPoint(MyManager & manager)
  : OpalPCSSEndPoint(manager),
    m_manager(manager)
{
}


PBoolean MyPCSSEndPoint::OnShowIncoming(const OpalPCSSConnection & connection)
{
  m_manager.OnRinging(connection);
  return PTrue;

}


PBoolean MyPCSSEndPoint::OnShowOutgoing(const OpalPCSSConnection & connection)
{
  PTime now;
  LogWindow << connection.GetRemotePartyName() << " is ringing on "
            << now.AsString("w h:mma") << " ..." << endl;
  return PTrue;
}


#if OPAL_H323
///////////////////////////////////////////////////////////////////////////////

MyH323EndPoint::MyH323EndPoint(MyManager & manager)
  : H323EndPoint(manager),
    m_manager(manager)
{
}


void MyH323EndPoint::OnRegistrationConfirm()
{
  LogWindow << "H.323 registration successful." << endl;
}

#endif
///////////////////////////////////////////////////////////////////////////////

#if OPAL_SIP

MySIPEndPoint::MySIPEndPoint(MyManager & manager)
  : SIPEndPoint(manager),
    m_manager(manager)
{
}


void MySIPEndPoint::OnRegistrationStatus(const PString & aor,
                                         PBoolean wasRegistering,
                                         PBoolean reRegistering,
                                         SIP_PDU::StatusCodes reason)
{
  switch (reason) {
    default:
      break;
    case SIP_PDU::Failure_UnAuthorised :
    case SIP_PDU::Information_Trying :
      return;

    case SIP_PDU::Successful_OK :
      if (reRegistering)
        return;
  }

  LogWindow << "SIP ";
  if (!wasRegistering)
    LogWindow << "un";
  LogWindow << "registration of " << aor << ' ';
  switch (reason) {
    case SIP_PDU::Successful_OK :
      LogWindow << "successful";
      break;

    case SIP_PDU::Failure_RequestTimeout :
      LogWindow << "timed out";
      break;

    default :
      LogWindow << "failed (" << reason << ')';
  }
  LogWindow << '.' << endl;
}


void MySIPEndPoint::OnSubscriptionStatus(const PString & eventPackage,
                                         const SIPURL & uri,
                                         bool wasSubscribing,
                                         bool reSubscribing,
                                         SIP_PDU::StatusCodes reason)
{
  switch (reason) {
    default:
      break;
    case SIP_PDU::Failure_UnAuthorised :
    case SIP_PDU::Information_Trying :
      return;

    case SIP_PDU::Successful_OK :
      if (reSubscribing)
        return;
  }

  LogWindow << "SIP ";
  if (!wasSubscribing)
    LogWindow << "un";
  LogWindow << "subscription of " << uri << " to " << eventPackage << " events ";
  switch (reason) {
    case SIP_PDU::Successful_OK :
      LogWindow << "successful";
      break;

    case SIP_PDU::Failure_RequestTimeout :
      LogWindow << "timed out";
      break;

    default :
      LogWindow << "failed (" << reason << ')';
  }
  LogWindow << '.' << endl;
}

#endif // OPAL_SIP


// End of File ///////////////////////////////////////////////////////////////
