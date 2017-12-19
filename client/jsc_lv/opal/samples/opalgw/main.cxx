/*
 * main.cxx
 *
 * PWLib application source file for OPAL Gateway
 *
 * Main program entry point.
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 21283 $
 * $Author: rjongbloed $
 * $Date: 2008-10-11 07:10:58 +0000 (Sat, 11 Oct 2008) $
 */

#include "precompile.h"
#include "main.h"
#include "custom.h"


PCREATE_PROCESS(OpalGw);


const WORD DefaultHTTPPort = 1719;
static const char UsernameKey[] = "Username";
static const char PasswordKey[] = "Password";
static const char LogLevelKey[] = "Log Level";
#if OPAL_PTLIB_SSL
static const char HTTPCertificateFileKey[]  = "HTTP Certificate";
#endif
static const char HttpPortKey[] = "HTTP Port";
static const char PreferredMediaKey[] = "Preferred Media";
static const char RemovedMediaKey[] = "Removed Media";
static const char MinJitterKey[] = "Minimum Jitter";
static const char MaxJitterKey[] = "Maximum Jitter";
static const char TCPPortBaseKey[] = "TCP Port Base";
static const char TCPPortMaxKey[] = "TCP Port Max";
static const char UDPPortBaseKey[] = "UDP Port Base";
static const char UDPPortMaxKey[] = "UDP Port Max";
static const char RTPPortBaseKey[] = "RTP Port Base";
static const char RTPPortMaxKey[] = "RTP Port Max";
static const char RTPTOSKey[] = "RTP Type of Service";

static const char H323AliasesKey[] = "H.323 Aliases";
static const char DisableFastStartKey[] = "Disable Fast Start";
static const char DisableH245TunnelingKey[] = "Disable H.245 Tunneling";
static const char DisableH245inSetupKey[] = "Disable H.245 in Setup";
static const char H323BandwidthKey[] = "H.323 Bandwidth";
static const char H323ListenersKey[] = "H.323 Interfaces";
static const char GatekeeperEnableKey[] = "Remote Gatekeeper Enable";
static const char GatekeeperAddressKey[] = "Remote Gatekeeper Address";
static const char GatekeeperIdentifierKey[] = "Remote Gatekeeper Identifier";
static const char GatekeeperInterfaceKey[] = "Remote Gatekeeper Interface";
static const char GatekeeperPasswordKey[] = "Remote Gatekeeper Password";
static const char GatekeeperTokenOIDKey[] = "Remote Gatekeeper Token OID";

static const char SIPUsernameKey[] = "SIP User Name";
static const char SIPProxyKey[] = "SIP Proxy URL";
static const char SIPRegistrarKey[] = "SIP Registrar";
static const char SIPListenersKey[] = "SIP Interfaces";

static const char LIDKey[] = "Line Interface Devices";

static const char VXMLKey[] = "VXML URL";

static const char DialPeerKey[] = "Dial Peers";
static const char DefaultIVRDialPeerKey[] = "Default IVR Alias";
static const char DefaultPOTSDialPeerKey[] = "Default POTS Dial Peer";
static const char DefaultPSTNDialPeerKey[] = "Default PSTN Dial Peer";
static const char * const DialPeerDestination[] = { "None", "H.323", "SIP" };
static const char DefaultH323DialPeerKey[] = "Default H.323 Dial Peer";
static const char * const H323DialPeerDestination[] = { "None", "POTS", "PSTN", "SIP" };
static const char DefaultSIPDialPeerKey[] = "Default SIP Dial Peer";
static const char * const SIPDialPeerDestination[] = { "None", "POTS", "PSTN", "H.323" };


///////////////////////////////////////////////////////////////////////////////

OpalGw::OpalGw()
  : OpalGwProcessAncestor(ProductInfo)
{
}


PBoolean OpalGw::OnStart()
{
  // change to the default directory to the one containing the executable
  PDirectory exeDir = GetFile().GetDirectory();

#if defined(_WIN32) && defined(_DEBUG)
  // Special check to aid in using DevStudio for debugging.
  if (exeDir.Find("\\Debug\\") != P_MAX_INDEX)
    exeDir = exeDir.GetParent();
#endif
  exeDir.Change();

  httpNameSpace.AddResource(new PHTTPDirectory("data", "data"));
  httpNameSpace.AddResource(new PServiceHTTPDirectory("html", "html"));

  return PHTTPServiceProcess::OnStart();
}


void OpalGw::OnStop()
{
  PHTTPServiceProcess::OnStop();
}


void OpalGw::OnControl()
{
  // This function get called when the Control menu item is selected in the
  // tray icon mode of the service.
  PStringStream url;
  url << "http://";

  PString host = PIPSocket::GetHostName();
  PIPSocket::Address addr;
  if (PIPSocket::GetHostAddress(host, addr))
    url << host;
  else
    url << "localhost";

  url << ':' << DefaultHTTPPort;

  PURL::OpenBrowser(url);
}


void OpalGw::OnConfigChanged()
{
}



PBoolean OpalGw::Initialise(const char * initMsg)
{
  PConfig cfg("Parameters");

  // Sert log level as early as possible
  SetLogLevel((PSystemLog::Level)cfg.GetInteger(LogLevelKey, GetLogLevel()));
#if PTRACING
  if (GetLogLevel() >= PSystemLog::Warning)
    PTrace::SetLevel(GetLogLevel()-PSystemLog::Warning);
  else
    PTrace::SetLevel(0);
  PTrace::ClearOptions(PTrace::Timestamp);
  PTrace::SetOptions(PTrace::DateAndTime);
#endif

  // Get the HTTP basic authentication info
  PString username = cfg.GetString(UsernameKey);
  PString password = PHTTPPasswordField::Decrypt(cfg.GetString(PasswordKey));

  PHTTPSimpleAuth authority(GetName(), username, password);

  // Create the parameters URL page, and start adding fields to it
  PConfigPage * rsrc = new PConfigPage(*this, "Parameters", "Parameters", authority);

  // HTTP authentication username/password
  rsrc->Add(new PHTTPStringField(UsernameKey, 25, username));
  rsrc->Add(new PHTTPPasswordField(PasswordKey, 25, password));

  // Log level for messages
  rsrc->Add(new PHTTPIntegerField(LogLevelKey,
                                  PSystemLog::Fatal, PSystemLog::NumLogLevels-1,
                                  GetLogLevel(),
                                  "1=Fatal only, 2=Errors, 3=Warnings, 4=Info, 5=Debug"));

#if OPAL_PTLIB_SSL
  // SSL certificate file.
  PString certificateFile = cfg.GetString(HTTPCertificateFileKey, "server.pem");
  rsrc->Add(new PHTTPStringField(HTTPCertificateFileKey, 25, certificateFile));
  if (certificateFile.IsEmpty())
    disableSSL = true;
  else if (!SetServerCertificate(certificateFile, PTrue)) {
    PSYSTEMLOG(Fatal, "OpalGw\tCould not load certificate \"" << certificateFile << '"');
    return PFalse;
  }
#endif

  // HTTP Port number to use.
  WORD httpPort = (WORD)cfg.GetInteger(HttpPortKey, DefaultHTTPPort);
  rsrc->Add(new PHTTPIntegerField(HttpPortKey, 1, 32767, httpPort));

  // Initialise the core of the system
  if (!manager.Initialise(cfg, rsrc))
    return PFalse;

  // Finished the resource to add, generate HTML for it and add to name space
  PServiceHTML html("System Parameters");
  rsrc->BuildHTML(html);
  httpNameSpace.AddResource(rsrc, PHTTPSpace::Overwrite);


#if OPAL_H323
  // Create the status page
  httpNameSpace.AddResource(new MainStatusPage(*this, authority), PHTTPSpace::Overwrite);
#endif // OPAL_H323


  // Create the home page
  static const char welcomeHtml[] = "welcome.html";
  if (PFile::Exists(welcomeHtml))
    httpNameSpace.AddResource(new PServiceHTTPFile(welcomeHtml, PTrue), PHTTPSpace::Overwrite);
  else {
    PHTML html;
    html << PHTML::Title("Welcome to " + GetName())
         << PHTML::Body()
         << GetPageGraphic()
         << PHTML::Paragraph() << "<center>"

         << PHTML::HotLink("Parameters") << "Parameters" << PHTML::HotLink()
         << PHTML::Paragraph();

    if (!systemLogFileName && systemLogFileName != "-")
      html << PHTML::HotLink("logfile.txt") << "Full Log File" << PHTML::HotLink()
           << PHTML::BreakLine()
           << PHTML::HotLink("tail_logfile") << "Tail Log File" << PHTML::HotLink()
           << PHTML::Paragraph();
 
    html << PHTML::HRule()
         << GetCopyrightText()
         << PHTML::Body();
    httpNameSpace.AddResource(new PServiceHTTPString("welcome.html", html), PHTTPSpace::Overwrite);
  }

  // set up the HTTP port for listening & start the first HTTP thread
  if (ListenForHTTP(httpPort))
    PSYSTEMLOG(Info, "Opened master socket for HTTP: " << httpListeningSocket->GetPort());
  else {
    PSYSTEMLOG(Fatal, "Cannot run without HTTP port: " << httpListeningSocket->GetErrorText());
    return PFalse;
  }

  PSYSTEMLOG(Info, "Service " << GetName() << ' ' << initMsg);
  return PTrue;
}


void OpalGw::Main()
{
  Suspend();
}


///////////////////////////////////////////////////////////////

MyManager::MyManager()
{
#if OPAL_H323
  h323EP = NULL;
  gkServer = NULL;
#endif
#if OPAL_SIP
  sipEP = NULL;
#endif
#if OPAL_LID
  potsEP = NULL;
#endif
#if OPAL_PTLIB_EXPAT
  ivrEP = NULL;
#endif

#if OPAL_VIDEO
  autoStartReceiveVideo = autoStartTransmitVideo = PFalse;
#endif
}


MyManager::~MyManager()
{
#if OPAL_LID
  // Must do this before we destroy the manager or a crash will result
  if (potsEP != NULL)
    potsEP->RemoveAllLines();
#endif

#if OPAL_H323
  delete gkServer;
#endif
}


PBoolean MyManager::Initialise(PConfig & cfg, PConfigPage * rsrc)
{
  PHTTPFieldArray * fieldArray;

  // Create all the endpoints

#if OPAL_H323
  if (h323EP == NULL)
    h323EP = new H323EndPoint(*this);
  if (gkServer == NULL && h323EP != NULL)
    gkServer = new MyGatekeeperServer(*h323EP);
#endif

#if OPAL_SIP
  if (sipEP == NULL)
    sipEP = new SIPEndPoint(*this);
#endif

#if OPAL_LID
  if (potsEP == NULL)
    potsEP = new OpalLineEndPoint(*this);
#endif

#if OPAL_PTLIB_EXPAT
  if (ivrEP == NULL)
    ivrEP = new OpalIVREndPoint(*this);
#endif

  // General parameters for all endpoint types
  fieldArray = new PHTTPFieldArray(new PHTTPStringField(PreferredMediaKey, 25), PTrue);
  PStringArray formats = fieldArray->GetStrings(cfg);
  if (formats.GetSize() > 0)
    SetMediaFormatOrder(formats);
  else
    fieldArray->SetStrings(cfg, GetMediaFormatOrder());
  rsrc->Add(fieldArray);

  fieldArray = new PHTTPFieldArray(new PHTTPStringField(RemovedMediaKey, 25), PTrue);
  SetMediaFormatMask(fieldArray->GetStrings(cfg));
  rsrc->Add(fieldArray);

  SetAudioJitterDelay(cfg.GetInteger(MinJitterKey, GetMinAudioJitterDelay()),
                      cfg.GetInteger(MaxJitterKey, GetMaxAudioJitterDelay()));
  rsrc->Add(new PHTTPIntegerField(MinJitterKey, 20, 2000, GetMinAudioJitterDelay(), "ms"));
  rsrc->Add(new PHTTPIntegerField(MaxJitterKey, 20, 2000, GetMaxAudioJitterDelay(), "ms"));

  SetTCPPorts(cfg.GetInteger(TCPPortBaseKey, GetTCPPortBase()),
              cfg.GetInteger(TCPPortMaxKey, GetTCPPortMax()));
  SetUDPPorts(cfg.GetInteger(UDPPortBaseKey, GetUDPPortBase()),
              cfg.GetInteger(UDPPortMaxKey, GetUDPPortMax()));
  SetRtpIpPorts(cfg.GetInteger(RTPPortBaseKey, GetRtpIpPortBase()),
                cfg.GetInteger(RTPPortMaxKey, GetRtpIpPortMax()));

  rsrc->Add(new PHTTPIntegerField(TCPPortBaseKey, 0, 65535, GetTCPPortBase()));
  rsrc->Add(new PHTTPIntegerField(TCPPortMaxKey,  0, 65535, GetTCPPortMax()));
  rsrc->Add(new PHTTPIntegerField(UDPPortBaseKey, 0, 65535, GetUDPPortBase()));
  rsrc->Add(new PHTTPIntegerField(UDPPortMaxKey,  0, 65535, GetUDPPortMax()));
  rsrc->Add(new PHTTPIntegerField(RTPPortBaseKey, 0, 65535, GetRtpIpPortBase()));
  rsrc->Add(new PHTTPIntegerField(RTPPortMaxKey,  0, 65535, GetRtpIpPortMax()));

  SetRtpIpTypeofService(cfg.GetInteger(RTPTOSKey, GetRtpIpTypeofService()));
  rsrc->Add(new PHTTPIntegerField(RTPTOSKey,  0, 255, GetRtpIpTypeofService()));

#if OPAL_H323

  // Add H.323 parameters
  fieldArray = new PHTTPFieldArray(new PHTTPStringField(H323AliasesKey, 25), PTrue);
  PStringArray aliases = fieldArray->GetStrings(cfg);
  if (aliases.IsEmpty())
    fieldArray->SetStrings(cfg, h323EP->GetAliasNames());
  else {
    h323EP->SetLocalUserName(aliases[0]);
    for (PINDEX i = 1; i < aliases.GetSize(); i++)
      h323EP->AddAliasName(aliases[i]);
  }
  rsrc->Add(fieldArray);

  h323EP->DisableFastStart(cfg.GetBoolean(DisableFastStartKey, h323EP->IsFastStartDisabled()));
  rsrc->Add(new PHTTPBooleanField(DisableFastStartKey,  h323EP->IsFastStartDisabled()));

  h323EP->DisableH245Tunneling(cfg.GetBoolean(DisableH245TunnelingKey, h323EP->IsH245TunnelingDisabled()));
  rsrc->Add(new PHTTPBooleanField(DisableH245TunnelingKey,  h323EP->IsH245TunnelingDisabled()));

  h323EP->DisableH245inSetup(cfg.GetBoolean(DisableH245inSetupKey, h323EP->IsH245inSetupDisabled()));
  rsrc->Add(new PHTTPBooleanField(DisableH245inSetupKey,  h323EP->IsH245inSetupDisabled()));

  h323EP->SetInitialBandwidth(cfg.GetInteger(H323BandwidthKey, h323EP->GetInitialBandwidth()/10)*10);
  rsrc->Add(new PHTTPIntegerField(H323BandwidthKey, 1, UINT_MAX/10, h323EP->GetInitialBandwidth()/10, "kb/s"));
  
  fieldArray = new PHTTPFieldArray(new PHTTPStringField(H323ListenersKey, 25), PFalse);
  if (!h323EP->StartListeners(fieldArray->GetStrings(cfg))) {
    PSYSTEMLOG(Error, "Could not open any H.323 listeners!");
  }
  rsrc->Add(fieldArray);

  bool gkEnable = cfg.GetBoolean(GatekeeperEnableKey, true);
  rsrc->Add(new PHTTPBooleanField(GatekeeperEnableKey, gkEnable));

  PString gkAddress = cfg.GetString(GatekeeperAddressKey);
  rsrc->Add(new PHTTPStringField(GatekeeperAddressKey, 25, gkAddress));

  PString gkIdentifier = cfg.GetString(GatekeeperIdentifierKey);
  rsrc->Add(new PHTTPStringField(GatekeeperIdentifierKey, 25, gkIdentifier));

  PString gkInterface = cfg.GetString(GatekeeperInterfaceKey);
  rsrc->Add(new PHTTPStringField(GatekeeperInterfaceKey, 25, gkInterface));

  PString gkPassword = PHTTPPasswordField::Decrypt(cfg.GetString(GatekeeperPasswordKey));
  if (!gkPassword)
    h323EP->SetGatekeeperPassword(gkPassword);
  rsrc->Add(new PHTTPPasswordField(GatekeeperPasswordKey, 25, gkPassword));

  h323EP->SetGkAccessTokenOID(cfg.GetString(GatekeeperTokenOIDKey));
  rsrc->Add(new PHTTPStringField(GatekeeperTokenOIDKey, 25, h323EP->GetGkAccessTokenOID()));

  if (gkEnable) {
    if (h323EP->UseGatekeeper(gkAddress, gkIdentifier, gkInterface)) {
      PSYSTEMLOG(Info, "Register with gatekeeper " << *h323EP->GetGatekeeper());
    }
    else {
      PSYSTEMLOG(Error, "Could not register with gatekeeper!");
    }
  }
  else {
    PSYSTEMLOG(Info, "Not using gatekeeper.");
    h323EP->RemoveGatekeeper();
  }

  if (!gkServer->Initialise(cfg, rsrc))
    return PFalse;
#endif

#if OPAL_SIP
  // Add SIP parameters
  sipEP->SetDefaultLocalPartyName(cfg.GetString(SIPUsernameKey, sipEP->GetDefaultLocalPartyName()));
  rsrc->Add(new PHTTPStringField(SIPUsernameKey, 25, sipEP->GetDefaultLocalPartyName()));

  PString proxy = sipEP->GetProxy().AsString();
  sipEP->SetProxy(cfg.GetString(SIPProxyKey, proxy));
  rsrc->Add(new PHTTPStringField(SIPProxyKey, 25, proxy));

  PString registrar = cfg.GetString(SIPRegistrarKey);
  rsrc->Add(new PHTTPStringField(SIPRegistrarKey, 25, registrar));

  fieldArray = new PHTTPFieldArray(new PHTTPStringField(SIPListenersKey, 25), PFalse);
  if (!sipEP->StartListeners(fieldArray->GetStrings(cfg))) {
    PSYSTEMLOG(Error, "Could not open any SIP listeners!");
  }
  rsrc->Add(fieldArray);

  if (!registrar) {
    if (sipEP->Register(registrar)) {
      PSYSTEMLOG(Info, "Registered with registrar " << registrar);
    }
    else {
      PSYSTEMLOG(Error, "Could not register with registrar!");
    }
  }

#endif

#if OPAL_LID
  // Add POTS and PSTN endpoints
  fieldArray = new PHTTPFieldArray(new PHTTPSelectField(LIDKey, OpalLineInterfaceDevice::GetAllDevices()), PFalse);
  PStringArray devices = fieldArray->GetStrings(cfg);
  if (!potsEP->AddDeviceNames(devices)) {
    PSYSTEMLOG(Error, "No LID devices!");
  }
  rsrc->Add(fieldArray);
#endif // OPAL_LID


#if OPAL_PTLIB_EXPAT
  // Create IVR protocol handler
  PString vxml = cfg.GetString(VXMLKey);
  rsrc->Add(new PHTTPStringField(VXMLKey, 25, vxml));
  if (!vxml)
    ivrEP->SetDefaultVXML(vxml);
#endif


  // Routing
  PString ivrAlias = cfg.GetString(DefaultIVRDialPeerKey, "#");
  rsrc->Add(new PHTTPStringField(DefaultIVRDialPeerKey, 10, ivrAlias));

  static const PStringArray dialPeerDestination(
                 PARRAYSIZE(DialPeerDestination),
                            DialPeerDestination);
  PINDEX potsRoute = dialPeerDestination.GetValuesIndex(cfg.GetString(DefaultPOTSDialPeerKey));
  if (potsRoute == P_MAX_INDEX)
    potsRoute = 1;
  rsrc->Add(new PHTTPRadioField(DefaultPOTSDialPeerKey, dialPeerDestination, potsRoute));

  PINDEX pstnRoute = dialPeerDestination.GetValuesIndex(cfg.GetString(DefaultPSTNDialPeerKey));
  if (pstnRoute == P_MAX_INDEX)
    pstnRoute = 0;
  rsrc->Add(new PHTTPRadioField(DefaultPSTNDialPeerKey, dialPeerDestination, pstnRoute));
  
#if OPAL_H323
  static const PStringArray h323DialPeerDestination(
                 PARRAYSIZE(H323DialPeerDestination),
                            H323DialPeerDestination);
  PINDEX h323Route = h323DialPeerDestination.GetValuesIndex(cfg.GetString(DefaultH323DialPeerKey));
  if (h323Route == P_MAX_INDEX)
    h323Route = 0;
  rsrc->Add(new PHTTPRadioField(DefaultH323DialPeerKey, h323DialPeerDestination, h323Route));
#endif
  
#if OPAL_SIP
  static const PStringArray sipDialPeerDestination(
                 PARRAYSIZE(SIPDialPeerDestination),
                            SIPDialPeerDestination);
  PINDEX sipRoute = sipDialPeerDestination.GetValuesIndex(cfg.GetString(DefaultSIPDialPeerKey));
  if (sipRoute == P_MAX_INDEX)
    sipRoute = 0;
  rsrc->Add(new PHTTPRadioField(DefaultSIPDialPeerKey, sipDialPeerDestination, sipRoute));
#endif
  
  fieldArray = new PHTTPFieldArray(new PHTTPStringField(DialPeerKey, 25), PTrue);
  PStringArray routes = fieldArray->GetStrings(cfg);
  rsrc->Add(fieldArray);

  if (!ivrAlias)
    routes += ".*:" + ivrAlias + "  = ivr:";

  switch (potsRoute) {
#if OPAL_H323
    case 1 :
      routes += "pots:.*\\*.*\\*.* = h323:<dn2ip>";
      routes += "pots:.*           = h323:<da>";
      break;
#endif
#if OPAL_SIP
    case 2 :
      routes += "pots:.*\\*.*\\*.* = sip:<dn2ip>";
      routes += "pots:.*           = sip:<da>";
#endif
  }

  switch (pstnRoute) {
#if OPAL_H323
    case 1 :
      routes += "pstn:.*\\*.*\\*.* = h323:<dn2ip>";
      routes += "pstn:.*           = h323:<da>";
      break;
#endif
#if OPAL_SIP
    case 2 :
      routes += "pstn:.*\\*.*\\*.* = sip:<dn2ip>";
      routes += "pstn:.*           = sip:<da>";
#endif
  }

#if OPAL_H323
  switch (h323Route) {
    case 1 :
      routes += "h323:.*           = pots:<da>";
      break;
    case 2 :
      routes += "h323:.*           = pstn:<da>";
      break;
    case 3 :
      routes += "h323:.*           = sip:<da>";
  }
#endif

#if OPAL_SIP
  switch (sipRoute) {
    case 1 :
      routes += "sip:.*           = pots:<da>";
      break;
    case 2 :
      routes += "sip:.*           = pstn:<da>";
      break;
    case 3 :
      routes += "sip:.*           = h323:<da>";
  }
#endif

  if (!SetRouteTable(routes)) {
    PSYSTEMLOG(Error, "No legal entries in dial peers!");
  }


  return PTrue;
}


// End of File ///////////////////////////////////////////////////////////////
