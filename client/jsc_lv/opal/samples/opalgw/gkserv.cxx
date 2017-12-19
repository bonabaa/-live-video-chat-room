/*
 * gkerv.cxx
 *
 * PWLib application source file for OPAL Gateway
 *
 * Main program entry point.
 *
 * Copyright (c) 2007 Equivalence Pty. Ltd.
 *
 * $Revision: 21283 $
 * $Author: rjongbloed $
 * $Date: 2008-10-11 07:10:58 +0000 (Sat, 11 Oct 2008) $
 */

#include "precompile.h"
#include "main.h"

#if OPAL_H323

#include <ptclib/random.h>

#ifdef H323_TRANSNEXUS_OSP
#include <opalosp.h>
#endif

//#define TEST_TOKEN

static const char GatekeeperIdentifierKey[] = "Server Gatekeeper Identifier";
static const char AvailableBandwidthKey[] = "Total Bandwidth";
static const char DefaultBandwidthKey[] = "Default Bandwidth Allocation";
static const char MaximumBandwidthKey[] = "Maximum Bandwidth Allocation";
static const char DefaultTimeToLiveKey[] = "Default Time To Live";
static const char CallHeartbeatTimeKey[]    = "Call Heartbeat Time";
static const char OverwriteOnSameSignalAddressKey[] = "Overwrite EP On Same Signal Address";
static const char CanHaveDuplicateAliasKey[] = "Can Have Duplicate Alias";
static const char CanOnlyCallRegisteredEPKey[] = "Can Only Call Registered EP";
static const char CanOnlyAnswerRegisteredEPKey[] = "Can Only Answer Registered EP";
static const char AnswerCallPreGrantedARQKey[] = "Answer Call Pregranted ARQ";
static const char MakeCallPreGrantedARQKey[] = "Make Call Pregranted ARQ";
static const char IsGatekeeperRoutedKey[] = "Gatekeeper Routed";
static const char AliasCanBeHostNameKey[] = "Alias Can Be Host Name";
static const char RequireH235Key[] = "Require H.235";
static const char UsernameKey[] = "Username";
static const char PasswordKey[] = "Password";
static const char RouteAliasKey[] = "Alias";
static const char RouteHostKey[] = "Host";
static const char ClearingHouseKey[] = "H501 Clearing House";
static const char H501InterfaceKey[] = "H501 Interface";
static const char GkServerListenersKey[] = "Server Gatekeeper Interfaces";

#ifdef H323_TRANSNEXUS_OSP
static const char OSPRoutingURLKey[] = "OSP Routing URL";
static const char OSPPrivateKeyFileKey[] = "OSP Private Key";
static const char OSPPublicKeyFileKey[] = "OSP Public Key";
static const char OSPServerKeyFileKey[] = "OSP Server Key";
#endif

#define LISTENER_INTERFACE_KEY        "Interface"

#define USERS_SECTION "Authentication"
#define USERS_KEY     "Credentials"

#define ROUTES_SECTION "Route Maps"
#define ROUTES_KEY     "Mapping"


#define new PNEW


///////////////////////////////////////////////////////////////

MainStatusPage::MainStatusPage(OpalGw & _app, PHTTPAuthority & auth)
  : PServiceHTTPString("Status", "", "text/html; charset=UTF-8", auth),
    app(_app)
{
  PHTML html;

  html << PHTML::Title("OpenH323 Gatekeeper Server Status")
       << "<meta http-equiv=\"Refresh\" content=\"30\">\n"
       << PHTML::Body()
       << app.GetPageGraphic()
       << PHTML::Paragraph() << "<center>"

       << PHTML::Form("POST")

       << PHTML::TableStart("border=1")
       << PHTML::TableRow()
       << PHTML::TableHeader()
       << "&nbsp;End&nbsp;Point&nbsp;Identifier&nbsp;"
       << PHTML::TableHeader()
       << "&nbsp;Call&nbsp;Signal&nbsp;Addresses&nbsp;"
       << PHTML::TableHeader()
       << "&nbsp;Aliases&nbsp;"
       << PHTML::TableHeader()
       << "&nbsp;Application&nbsp;"
       << PHTML::TableHeader()
       << "&nbsp;Active&nbsp;Calls&nbsp;"
       << "<!--#macrostart EndPointStatus-->"
         << PHTML::TableRow()
         << PHTML::TableData()
         << "<!--#status EndPointIdentifier-->"
         << PHTML::TableData()
         << "<!--#status CallSignalAddresses-->"
         << PHTML::TableData("NOWRAP")
         << "<!--#status EndPointAliases-->"
         << PHTML::TableData("NOWRAP")
         << "<!--#status Application-->"
         << PHTML::TableData("align=center")
         << "<!--#status ActiveCalls-->"
         << PHTML::TableData()
         << PHTML::SubmitButton("Unregister", "!--#status EndPointIdentifier--")
       << "<!--#macroend EndPointStatus-->"
       << PHTML::TableEnd()

       << PHTML::Paragraph()

       << PHTML::TableStart("border=1")
       << PHTML::TableRow()
       << PHTML::TableHeader()
       << "&nbsp;Call&nbsp;Identifier&nbsp;"
       << PHTML::TableHeader()
       << "&nbsp;End&nbsp;Point&nbsp;"
       << PHTML::TableHeader()
       << "&nbsp;Source/Destination" << PHTML::BreakLine() << "Signalling&nbsp;Addresse&nbsp;"
       << PHTML::TableHeader()
       << "&nbsp;Last&nbsp;IRR&nbsp;"
       << PHTML::TableHeader()
       << "&nbsp;Connected&nbsp;"
       << "<!--#macrostart CallStatus-->"
         << PHTML::TableRow()
         << PHTML::TableData("NOWRAP")
         << "<!--#status CallIdentifier-->"
         << PHTML::TableData("NOWRAP")
         << "<!--#status EndPointIdentifier-->"
         << PHTML::TableData("NOWRAP")
         << "<!--#status SourceAddress-->"
         << PHTML::BreakLine()
         << "<!--#status DestinationAddress-->"
         << PHTML::TableData()
         << "<!--#status LastIRR-->"
         << PHTML::TableData()
         << "<!--#status ConnectedTime-->"
         << PHTML::TableData()
         << PHTML::SubmitButton("Clear", "!--#status CallIdentifier--")
       << "<!--#macroend CallStatus-->"
       << PHTML::TableEnd()

       << PHTML::Form()
       << PHTML::HRule()

       << app.GetCopyrightText()
       << PHTML::Body();

  string = html;
}


PBoolean MainStatusPage::Post(PHTTPRequest & request,
                          const PStringToString & data,
                          PHTML & msg)
{
  PTRACE(2, "Main\tClear call POST received " << data);

  msg << PHTML::Title() << "Accepted Control Command" << PHTML::Body()
      << PHTML::Heading(1) << "Accepted Control Command" << PHTML::Heading(1);

  if (!app.manager.OnPostControl(data, msg))
    msg << PHTML::Heading(2) << "No calls or endpoints!" << PHTML::Heading(2);

  msg << PHTML::Paragraph()
      << PHTML::HotLink(request.url.AsString()) << "Reload page" << PHTML::HotLink()
      << "&nbsp;&nbsp;&nbsp;&nbsp;"
      << PHTML::HotLink("/") << "Home page" << PHTML::HotLink();

  PServiceHTML::ProcessMacros(request, msg, "html/status.html",
                              PServiceHTML::LoadFromFile|PServiceHTML::NoSignatureForFile);
  return TRUE;
}


PBoolean MyManager::OnPostControl(const PStringToString & data, PHTML & msg)
{
  bool gotOne = FALSE;

  for (PINDEX i = 0; i < data.GetSize(); i++) {
    PString key = data.GetKeyAt(i);
    PString value = data.GetDataAt(i);
    if (value == "Unregister") {
      PSafePtr<H323RegisteredEndPoint> ep = gkServer->FindEndPointByIdentifier(key);
      if (ep != NULL) {
        msg << PHTML::Heading(2) << "Unregistered endpoint " << *ep << PHTML::Heading(2);
        ep->Unregister();
        gotOne = TRUE;
      }
    }
    else if (value == "Clear") {
      PSafePtr<H323GatekeeperCall> call = gkServer->FindCall(key);
      if (call != NULL) {
        msg << PHTML::Heading(2) << "Cleared call " << *call << PHTML::Heading(2);
        call->Disengage();
        gotOne = TRUE;
      }
    }
  }

  return gotOne;
}


static void SpliceMacro(PString & text, const PString & token, const PString & value)
{
  PRegularExpression RegEx("<?!--#status[ \t\r\n]+" + token + "[ \t\r\n]*-->?",
                           PRegularExpression::Extended|PRegularExpression::IgnoreCase);
  PINDEX pos, len;
  while (text.FindRegEx(RegEx, pos, len))
    text.Splice(value, pos, len);
}


PString MyManager::OnLoadEndPointStatus(const PString & htmlBlock)
{
  PINDEX i;
  PString substitution;

  for (PSafePtr<H323RegisteredEndPoint> ep = gkServer->GetFirstEndPoint(PSafeReadOnly); ep != NULL; ep++) {
    // make a copy of the repeating html chunk
    PString insert = htmlBlock;

    SpliceMacro(insert, "EndPointIdentifier", ep->GetIdentifier());

    PStringStream addresses;
    for (i = 0; i < ep->GetSignalAddressCount(); i++) {
      if (i > 0)
        addresses << "<br>";
      addresses << ep->GetSignalAddress(i);
    }
    SpliceMacro(insert, "CallSignalAddresses", addresses);

    PStringStream aliases;
    for (i = 0; i < ep->GetAliasCount(); i++) {
      if (i > 0)
        aliases << "<br>";
      aliases << ep->GetAlias(i);
    }
    SpliceMacro(insert, "EndPointAliases", aliases);

    PString str = "<i>Name:</i> " + ep->GetApplicationInfo();
    str.Replace("\t", "<BR><i>Version:</i> ");
    str.Replace("\t", " <i>Vendor:</i> ");
    SpliceMacro(insert, "Application", str);

    SpliceMacro(insert, "ActiveCalls", ep->GetCallCount());

    // Then put it into the page, moving insertion point along after it.
    substitution += insert;
  }

  return substitution;
}


PString MyManager::OnLoadCallStatus(const PString & htmlBlock)
{
  PString substitution;

  for (PSafePtr<H323GatekeeperCall> call = gkServer->GetFirstCall(PSafeReadOnly); call != NULL; call++) {
    // make a copy of the repeating html chunk
    PString insert = htmlBlock;

    SpliceMacro(insert, "CallIdentifier",     call->GetCallIdentifier().AsString());
    SpliceMacro(insert, "EndPointIdentifier", call->GetEndPoint().GetIdentifier());
    SpliceMacro(insert, "SourceAddress",      call->GetSourceAddress());
    SpliceMacro(insert, "DestinationAddress", call->GetDestinationAddress());
    SpliceMacro(insert, "LastIRR",            call->GetLastInfoResponseTime().AsString("hh:mm:ss"));
    if (call->GetConnectedTime().GetTimeInSeconds() != 0)
      SpliceMacro(insert, "ConnectedTime",    call->GetConnectedTime().AsString("hh:mm:ss"));

    // Then put it into the page, moving insertion point along after it.
    substitution += insert;
  }

  return substitution;
}


PCREATE_SERVICE_MACRO_BLOCK(EndPointStatus,P_EMPTY,P_EMPTY,block)
{
  return OpalGw::Current().manager.OnLoadEndPointStatus(block);
}


PCREATE_SERVICE_MACRO_BLOCK(CallStatus,P_EMPTY,P_EMPTY,block)
{
  return OpalGw::Current().manager.OnLoadCallStatus(block);
}


///////////////////////////////////////////////////////////////

MyGatekeeperServer::MyGatekeeperServer(H323EndPoint & ep)
  : H323GatekeeperServer(ep),
    endpoint(ep)
{
#ifdef H323_TRANSNEXUS_OSP
  ospProvider = NULL;
#endif
}


PBoolean MyGatekeeperServer::Initialise(PConfig & cfg, PConfigPage * rsrc)
{
  PINDEX i;

  PWaitAndSignal mutex(reconfigurationMutex);

#ifdef H323_H501

  // set clearing house address
  PString clearingHouse = cfg.GetString(ClearingHouseKey);
  rsrc->Add(new PHTTPStringField(ClearingHouseKey, 25, clearingHouse));

  PString h501Interface = cfg.GetString(H501InterfaceKey);
  rsrc->Add(new PHTTPStringField(H501InterfaceKey, 25, h501Interface));

  if (clearingHouse.IsEmpty())
    SetPeerElement(NULL);
  else {
    if (!h501Interface)
      CreatePeerElement(h501Interface);
    if (!OpenPeerElement(clearingHouse)) {
      PSYSTEMLOG(Error, "Main\tCould not open clearing house at: " << clearingHouse);
    }
  }
#endif

#ifdef H323_TRANSNEXUS_OSP
  PString oldOSPServer = ospRoutingURL;
  ospRoutingURL = cfg.GetString(OSPRoutingURLKey, ospRoutingURL);
  rsrc->Add(new PHTTPStringField(OSPRoutingURLKey, 25, ospRoutingURL));
  bool ospChanged = oldOSPServer != ospRoutingURL;

  ospPrivateKeyFileName = cfg.GetString(OSPPrivateKeyFileKey, ospPrivateKeyFileName);
  rsrc->Add(new PHTTPStringField(OSPPrivateKeyFileKey, 25, ospPrivateKeyFileName));

  ospPublicKeyFileName = cfg.GetString(OSPPublicKeyFileKey, ospPublicKeyFileName);
  rsrc->Add(new PHTTPStringField(OSPPublicKeyFileKey, 25, ospPublicKeyFileName));

  ospServerKeyFileName = cfg.GetString(OSPServerKeyFileKey, ospServerKeyFileName);
  rsrc->Add(new PHTTPStringField(OSPServerKeyFileKey, 25, ospServerKeyFileName));

  if (!ospRoutingURL.IsEmpty()) {
    if (ospProvider != NULL && ospProvider->IsOpen() && ospChanged) {
      delete ospProvider;
      ospProvider = NULL;
    }
    ospProvider = new OpalOSP::Provider();
    int status;
    if (ospPrivateKeyFileName.IsEmpty() && ospPublicKeyFileName.IsEmpty() && ospServerKeyFileName.IsEmpty())
      status = ospProvider->Open(ospRoutingURL);
    else
      status = ospProvider->Open(ospRoutingURL, ospPrivateKeyFileName, ospPublicKeyFileName, ospServerKeyFileName);
    if (status != 0) {
      delete ospProvider;
      ospProvider = NULL;
    }
  } 
  
  else if (ospProvider != NULL) {
    ospProvider->Close();
    delete ospProvider;
    ospProvider = NULL;
  }

#endif

  PString gkid = cfg.GetString(GatekeeperIdentifierKey, OpalGw::Current().GetName() + " on " + PIPSocket::GetHostName());
  rsrc->Add(new PHTTPStringField(GatekeeperIdentifierKey, 25, gkid));
  SetGatekeeperIdentifier(gkid);

  // Interfaces to listen on
  PHTTPFieldArray * fieldArray = new PHTTPFieldArray(new PHTTPStringField(GkServerListenersKey, 25), PFalse);
  H323TransportAddressArray interfaces = fieldArray->GetStrings(cfg);
  AddListeners(interfaces);
  rsrc->Add(fieldArray);

  SetAvailableBandwidth(cfg.GetInteger(AvailableBandwidthKey, GetAvailableBandwidth()/10)*10);
  rsrc->Add(new PHTTPIntegerField(AvailableBandwidthKey, 1, INT_MAX, GetAvailableBandwidth()/10, "kb/s"));

  defaultBandwidth = cfg.GetInteger(DefaultBandwidthKey, defaultBandwidth/10)*10;
  rsrc->Add(new PHTTPIntegerField(DefaultBandwidthKey, 1, INT_MAX, defaultBandwidth/10, "kb/s"));

  maximumBandwidth = cfg.GetInteger(MaximumBandwidthKey, maximumBandwidth/10)*10;
  rsrc->Add(new PHTTPIntegerField(MaximumBandwidthKey, 1, INT_MAX, maximumBandwidth/10, "kb/s"));

  defaultTimeToLive = cfg.GetInteger(DefaultTimeToLiveKey, defaultTimeToLive);
  rsrc->Add(new PHTTPIntegerField(DefaultTimeToLiveKey, 10, 86400, defaultTimeToLive, "seconds"));

  defaultInfoResponseRate = cfg.GetInteger(CallHeartbeatTimeKey, defaultInfoResponseRate);
  rsrc->Add(new PHTTPIntegerField(CallHeartbeatTimeKey, 0, 86400, defaultInfoResponseRate, "seconds"));

  overwriteOnSameSignalAddress = cfg.GetBoolean(OverwriteOnSameSignalAddressKey, overwriteOnSameSignalAddress);
  rsrc->Add(new PHTTPBooleanField(OverwriteOnSameSignalAddressKey, overwriteOnSameSignalAddress));

  canHaveDuplicateAlias = cfg.GetBoolean(CanHaveDuplicateAliasKey, canHaveDuplicateAlias);
  rsrc->Add(new PHTTPBooleanField(CanHaveDuplicateAliasKey, canHaveDuplicateAlias));

  canOnlyCallRegisteredEP = cfg.GetBoolean(CanOnlyCallRegisteredEPKey, canOnlyCallRegisteredEP);
  rsrc->Add(new PHTTPBooleanField(CanOnlyCallRegisteredEPKey, canOnlyCallRegisteredEP));

  canOnlyAnswerRegisteredEP = cfg.GetBoolean(CanOnlyAnswerRegisteredEPKey, canOnlyAnswerRegisteredEP);
  rsrc->Add(new PHTTPBooleanField(CanOnlyAnswerRegisteredEPKey, canOnlyAnswerRegisteredEP));

  answerCallPreGrantedARQ = cfg.GetBoolean(AnswerCallPreGrantedARQKey, answerCallPreGrantedARQ);
  rsrc->Add(new PHTTPBooleanField(AnswerCallPreGrantedARQKey, answerCallPreGrantedARQ));

  makeCallPreGrantedARQ = cfg.GetBoolean(MakeCallPreGrantedARQKey, makeCallPreGrantedARQ);
  rsrc->Add(new PHTTPBooleanField(MakeCallPreGrantedARQKey, makeCallPreGrantedARQ));

  aliasCanBeHostName = cfg.GetBoolean(AliasCanBeHostNameKey, aliasCanBeHostName);
  rsrc->Add(new PHTTPBooleanField(AliasCanBeHostNameKey, aliasCanBeHostName));

  isGatekeeperRouted = cfg.GetBoolean(IsGatekeeperRoutedKey, isGatekeeperRouted);
  rsrc->Add(new PHTTPBooleanField(IsGatekeeperRoutedKey, isGatekeeperRouted));

  routes.RemoveAll();
  PINDEX arraySize = cfg.GetInteger(ROUTES_SECTION, ROUTES_KEY" Array Size");
  for (i = 0; i < arraySize; i++) {
    cfg.SetDefaultSection(psprintf(ROUTES_SECTION"\\"ROUTES_KEY" %u", i+1));
    RouteMap map(cfg.GetString(RouteAliasKey), cfg.GetString(RouteHostKey));
    if (map.IsValid())
      routes.Append(new RouteMap(map));
  }

  PHTTPCompositeField * routeFields = new PHTTPCompositeField(ROUTES_SECTION"\\"ROUTES_KEY" %u\\", "Alias Route Maps");
  routeFields->Append(new PHTTPStringField(RouteAliasKey, 20));
  routeFields->Append(new PHTTPStringField(RouteHostKey, 30));
  rsrc->Add(new PHTTPFieldArray(routeFields, TRUE));

  requireH235 = cfg.GetBoolean(RequireH235Key, requireH235);
  rsrc->Add(new PHTTPBooleanField(RequireH235Key, requireH235));

  passwords.RemoveAll();
  arraySize = cfg.GetInteger(USERS_SECTION, USERS_KEY" Array Size");
  for (i = 0; i < arraySize; i++) {
    cfg.SetDefaultSection(psprintf(USERS_SECTION"\\"USERS_KEY" %u", i+1));
    PString username = cfg.GetString(UsernameKey);
    if (!username) {
      PString password = PHTTPPasswordField::Decrypt(cfg.GetString(PasswordKey));
      passwords.SetAt(username, password);
    }
  }

  PHTTPCompositeField * security = new PHTTPCompositeField(USERS_SECTION"\\"USERS_KEY" %u\\", "Authentication Credentials");
  security->Append(new PHTTPStringField(UsernameKey, 25));
  security->Append(new PHTTPPasswordField(PasswordKey, 25));
  rsrc->Add(new PHTTPFieldArray(security, FALSE));

  return TRUE;
}


H323GatekeeperCall * MyGatekeeperServer::CreateCall(const OpalGloballyUniqueID & id,
                                                    H323GatekeeperCall::Direction dir)
{
  return new MyGatekeeperCall(*this, id, dir);
}


PBoolean MyGatekeeperServer::TranslateAliasAddress(const H225_AliasAddress & alias,
                                               H225_ArrayOf_AliasAddress & aliases,
                                               H323TransportAddress & address,
                                               PBoolean & isGkRouted,
                                               H323GatekeeperCall * call)
{
#ifdef H323_TRANSNEXUS_OSP
  if (ospProvider != NULL) {
    address = "127.0.0.1";
    return TRUE;
  }
#endif

  if (H323GatekeeperServer::TranslateAliasAddress(alias, aliases, address, isGkRouted, call))
    return TRUE;

  PString aliasString = H323GetAliasAddressString(alias);
  PTRACE(4, "Translating \"" << aliasString << "\" through route maps");

  PWaitAndSignal mutex(reconfigurationMutex);

  for (PINDEX i = 0; i < routes.GetSize(); i++) {
    PTRACE(4, "Checking route map " << routes[i]);
    if (routes[i].IsMatch(aliasString)) {
      address = routes[i].GetHost();
      PTRACE(3, "Translated \"" << aliasString << "\" to " << address);
      break;
    }
  }

  return TRUE;
}


MyGatekeeperServer::RouteMap::RouteMap(const PString & theAlias, const PString & theHost)
  : alias(theAlias),
    regex('^' + theAlias + '$'),
    host(theHost)
{
}


void MyGatekeeperServer::RouteMap::PrintOn(ostream & strm) const
{
  strm << '"' << alias << "\" => " << host;
}


PBoolean MyGatekeeperServer::RouteMap::IsValid() const
{
  return !alias && regex.GetErrorCode() == PRegularExpression::NoError && !host;
}


PBoolean MyGatekeeperServer::RouteMap::IsMatch(const PString & alias) const
{
  PINDEX start;
  return regex.Execute(alias, start);
}


///////////////////////////////////////////////////////////////

MyGatekeeperCall::MyGatekeeperCall(MyGatekeeperServer & gk,
                                   const OpalGloballyUniqueID & id,
                                   Direction dir)
  : H323GatekeeperCall(gk, id, dir)
{
#ifdef H323_TRANSNEXUS_OSP
  ospTransaction = NULL;
#endif
}

#ifdef H323_TRANSNEXUS_OSP
static bool GetE164Alias(const H225_ArrayOf_AliasAddress & aliases, H225_AliasAddress & destAlias)
{
  PINDEX i;
  for (i = 0; i < aliases.GetSize(); ++i) {
    if (aliases[i].GetTag() == H225_AliasAddress::e_dialedDigits) {
      destAlias = aliases[i];
      return TRUE;
    }
  }
  return FALSE;
}

PBoolean MyGatekeeperCall::AuthoriseOSPCall(H323GatekeeperARQ & info)
{
  int result;

  // if making call, authorise the call and insert the token
  if (!info.arq.m_answerCall) {

    OpalOSP::Transaction::AuthorisationInfo authInfo;

    // get the source call signalling address
    if (info.arq.HasOptionalField(H225_AdmissionRequest::e_srcCallSignalAddress))
      authInfo.ospvSource = OpalOSP::TransportAddressToOSPString(info.arq.m_srcCallSignalAddress);
    else
      authInfo.ospvSource = OpalOSP::TransportAddressToOSPString(info.endpoint->GetSignalAddress(0));

    // get the source number
    if (!GetE164Alias(info.arq.m_srcInfo, authInfo.callingNumber)) {
      PTRACE(1, "OSP\tNo E164 source address in ARQ");
      info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_incompleteAddress);
      return FALSE;
    }

    // get the dest number
    if (!info.arq.HasOptionalField(H225_AdmissionRequest::e_destinationInfo)) {
      PTRACE(1, "OSP\tNo dest aliases in ARQ");
      info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_incompleteAddress);
      return FALSE;
    }
    if (!GetE164Alias(info.arq.m_destinationInfo, authInfo.calledNumber)) {
      PTRACE(1, "OSP\tNo E164 source address in ARQ");
      info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_incompleteAddress);
      return FALSE;
    }

    // get the call ID
    authInfo.callID           = this->GetCallIdentifier();

    // authorise the call
    unsigned numberOfDestinations = 1;
    if ((result = ospTransaction->Authorise(authInfo, numberOfDestinations)) != 0) {
      PTRACE(1, "OSP\tCannot authorise ARQ - result = " << result);
      info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_incompleteAddress);
      return FALSE;
    } 
    if (numberOfDestinations == 0) {
      PTRACE(1, "OSP\tNo destinations available");
      info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_noRouteToDestination);
      return FALSE;
    }

    // get the destination
    OpalOSP::Transaction::DestinationInfo destInfo;
    if ((result = ospTransaction->GetFirstDestination(destInfo)) != 0) {
      PTRACE(1, "OSP\tCannot get first destination - result = " << result);
      info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_undefinedReason);
      return FALSE;
    } 

    // insert destination into the ACF
    if (!destInfo.Insert(info.acf)) {
      PTRACE(1, "OSP\tCannot insert information info ACF");
      info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_undefinedReason);
      return FALSE;
    }

    PTRACE(4, "OSP routed call to " << destInfo.calledNumber << "@" << destInfo.destinationAddress);
    return TRUE;
  }

  // if answering call, validate the token
  OpalOSP::Transaction::ValidationInfo valInfo;

  // get the token
  if (!info.arq.HasOptionalField(H225_AdmissionRequest::e_tokens) || 
     !valInfo.ExtractToken(info.arq.m_tokens)) {
    PTRACE(1, "OSP\tNo tokens in in ARQ");
    info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_invalidPermission);
    return FALSE;
  }

  // get the source call signalling address
  if (info.arq.HasOptionalField(H225_AdmissionRequest::e_srcCallSignalAddress))
    valInfo.ospvSource = OpalOSP::TransportAddressToOSPString(info.arq.m_srcCallSignalAddress);
  else
    valInfo.ospvSource = OpalOSP::TransportAddressToOSPString(info.endpoint->GetSignalAddress(0));

  // get the dest call signalling address
  if (info.arq.HasOptionalField(H225_AdmissionRequest::e_destCallSignalAddress))
    valInfo.ospvDest = OpalOSP::TransportAddressToOSPString(info.arq.m_destCallSignalAddress);
  else
    valInfo.ospvDest = OpalOSP::TransportAddressToOSPString(info.endpoint->GetSignalAddress(0));

  // get the source number
  if (!GetE164Alias(info.arq.m_srcInfo, valInfo.callingNumber)) {
    PTRACE(1, "OSP\tNo E164 source address in ARQ");
    info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_incompleteAddress);
    return FALSE;
  }

  // get the dest number
  if (!info.arq.HasOptionalField(H225_AdmissionRequest::e_destinationInfo)) {
    PTRACE(1, "OSP\tNo dest aliases in ARQ");
    info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_incompleteAddress);
    return FALSE;
  }
  if (!GetE164Alias(info.arq.m_destinationInfo, valInfo.calledNumber)) {
    PTRACE(1, "OSP\tNo E164 source address in ARQ");
    info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_incompleteAddress);
    return FALSE;
  }

  // get the call ID
  valInfo.callID = this->GetCallIdentifier();

  // validate the token
  bool authorised;
  unsigned timeLimit;
  if ((result = ospTransaction->Validate(valInfo, authorised, timeLimit)) != 0) {
    PTRACE(1, "OSP\tCannot validate ARQ - result = " << result);
    info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_invalidPermission);
    return FALSE;
  }

  if (!authorised) {
    PTRACE(1, "OSP\tCall not authorised");
    info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_requestDenied);
    return FALSE;
  }

  PTRACE(4, "OSP authorised call with time limit of " << timeLimit);
  return TRUE;
}

#endif

H323GatekeeperRequest::Response MyGatekeeperCall::OnAdmission(H323GatekeeperARQ & info)
{
#ifdef TEST_TOKEN
  info.acf.IncludeOptionalField(H225_AdmissionConfirm::e_tokens);
  info.acf.m_tokens.SetSize(1);
  info.acf.m_tokens[0].m_tokenOID = "1.2.36.76840296.1";
  info.acf.m_tokens[0].IncludeOptionalField(H235_ClearToken::e_nonStandard);
  info.acf.m_tokens[0].m_nonStandard.m_nonStandardIdentifier = "1.2.36.76840296.1.1";
  info.acf.m_tokens[0].m_nonStandard.m_data = "SnfYt0jUuZ4lVQv8umRYaH2JltXDRW6IuYcnASVU";
#endif

#ifdef H323_TRANSNEXUS_OSP
  OpalOSP::Provider * ospProvider = ((MyGatekeeperServer &)gatekeeper).GetOSPProvider();
  if (ospProvider != NULL) {
    // need to add RIP as clearing house is involved
    if (info.IsFastResponseRequired())
      return H323GatekeeperRequest::InProgress(30000);

    ospTransaction = new OpalOSP::Transaction();
    int result;
    if ((result = ospTransaction->Open(*ospProvider)) != 0) {
      delete ospTransaction;
      ospTransaction = NULL;
      PTRACE(1, "OSP\tCannot open transaction - result = " << result);
      info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_exceedsCallCapacity);
      return H323GatekeeperRequest::Reject;
    }

    if (!AuthoriseOSPCall(info)) {
      delete ospTransaction;
      ospTransaction = NULL;
      return H323GatekeeperRequest::Reject;
    }

    return H323GatekeeperRequest::Confirm;
#endif

  return H323GatekeeperCall::OnAdmission(info);
}


MyGatekeeperCall::~MyGatekeeperCall()
{
#ifdef H323_TRANSNEXUS_OSP
  if (ospTransaction != NULL) {
    ospTransaction->SetEndReason(callEndReason);
    PTimeInterval duration;
    if (connectedTime.GetTimeInSeconds() != 0 && callEndTime.GetTimeInSeconds() != 0)
      duration = callEndTime - connectedTime;
    ospTransaction->CallEnd(callEndTime.GetTimeInSeconds());
    delete ospTransaction;
  }
#endif
}

#endif // OPAL_H323


// End of File ///////////////////////////////////////////////////////////////
