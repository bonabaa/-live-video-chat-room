/*
 * h323pdu.cxx
 *
 * H.323 PDU definitions
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2000 Equivalence Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions of this code were written with the assisance of funding from
 * Vovida Networks, Inc. http://www.vovida.com.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23923 $
 * $Author: rjongbloed $
 * $Date: 2010-01-12 01:49:45 +0000 (Tue, 12 Jan 2010) $
 */

#include <ptlib.h>

#include <opal/buildopts.h>
#if OPAL_H323

#ifdef __GNUC__
#pragma implementation "h323pdu.h"
#endif

#include <h323/h323pdu.h>

#include <h323/h323ep.h>
#include <h323/h323con.h>
#include <h323/transaddr.h>
#include <h323/h225ras.h>
#include <h323/h235auth.h>

#if OPAL_H460
#include <h460/h460.h>
#endif

#define new PNEW

const unsigned H225_ProtocolID[] = { 0,0,8,2250,0,H225_PROTOCOL_VERSION };
const unsigned H245_ProtocolID[] = { 0,0,8,245 ,0,H245_PROTOCOL_VERSION };

static const char E164NumberPrefix[] = "E164:";
static const char PrivatePartyPrefix[] = "Private:";
static const char DataPartyPrefix[] = "Data:";
static const char TelexPartyPrefix[] = "Telex:";
static const char NSPNumberPrefix[] = "NSP:";


///////////////////////////////////////////////////////////////////////////////

#if PTRACING
void H323TraceDumpPDU(const char * proto,
                      PBoolean writing,
                      const PBYTEArray & rawData,
                      const PASN_Object & pdu,
                      const PASN_Choice & tags,
                      unsigned seqNum)
{
  if (!PTrace::CanTrace(3))
    return;

  ostream & trace = PTrace::Begin(3, __FILE__, __LINE__);
  trace << proto << '\t' << (writing ? "Send" : "Receiv") << "ing PDU:";

  if (PTrace::CanTrace(4)) {
    trace << "\n  "
          << resetiosflags(ios::floatfield);

    if (!PTrace::CanTrace(5))
      trace << setiosflags(ios::fixed); // Will truncate hex dumps to 32 bytes

    trace << setprecision(2) << pdu
          << resetiosflags(ios::floatfield);

    if (PTrace::CanTrace(6))
      trace << "\nRaw PDU:\n"
            << hex << setfill('0')
            << setprecision(2) << rawData
            << dec << setfill(' ');
  }
  else {
    trace << ' ' << tags.GetTagName();
    PASN_Object & next = tags.GetObject();
    if (PIsDescendant(&next, PASN_Choice))
      trace << ' ' << ((PASN_Choice &)next).GetTagName();
    if (seqNum > 0)
      trace << ' ' << seqNum;
  }

  trace << PTrace::End;
}
#endif


///////////////////////////////////////////////////////////////////////////////

void H323SetAliasAddresses(const H323TransportAddressArray & addresses, H225_ArrayOf_AliasAddress & aliases)
{
  aliases.SetSize(addresses.GetSize());
  for (PINDEX i = 0; i < addresses.GetSize(); i++)
    H323SetAliasAddress(addresses[i], aliases[i]);
}


void H323SetAliasAddresses(const PStringArray & names,
                           H225_ArrayOf_AliasAddress & aliases,
                           int tag)
{
  aliases.SetSize(names.GetSize());
  for (PINDEX i = 0; i < names.GetSize(); i++)
    H323SetAliasAddress(names[i], aliases[i], tag);
}


void H323SetAliasAddresses(const PStringList & names,
                           H225_ArrayOf_AliasAddress & aliases,
                           int tag)
{
  aliases.SetSize(names.GetSize());
  PINDEX i = 0;
  for (PStringList::const_iterator name = names.begin(); name != names.end(); ++name,++i)
    H323SetAliasAddress(*name, aliases[i], tag);
}


static PBoolean IsE164(const PString & str)
{
  return !str && str.FindSpan("1234567890*#") == P_MAX_INDEX;
}


void H323SetAliasAddress(const H323TransportAddress & address, H225_AliasAddress & alias)
{
  alias.SetTag(H225_AliasAddress::e_transportID);
  address.SetPDU(alias);
}

static struct {
  const char * name;
  int tag;
} aliasAddressTypes[5] = {
  { "e164",  H225_AliasAddress::e_dialedDigits },
  { "h323",  H225_AliasAddress::e_h323_ID },
  { "url",   H225_AliasAddress::e_url_ID },
  { "ip",    H225_AliasAddress::e_transportID },
  { "email", H225_AliasAddress::e_email_ID },
//  { "???",    H225_AliasAddresse_partyNumber },
//  { "???",    H225_AliasAddresse_mobileUIM }
};

void H323SetAliasAddress(const PString & _name, H225_AliasAddress & alias, int tag)
{
  PString name = _name;
  // See if alias type was explicitly specified
  if (tag < 0) {
    PINDEX colon = name.Find(':');
    if (colon != P_MAX_INDEX && colon > 0) {
      PString type = name.Left(colon);
      for (PINDEX i = 0; tag < 0 && i < 5; i++) {
        if (type == aliasAddressTypes[i].name) {
          tag = aliasAddressTypes[i].tag;
          name = name.Mid(colon+1);
        }
      }
    }
  }
  
  // otherwise guess it from the string: if all digits then assume an e164 address.
  if (tag < 0)
    tag = IsE164(name) ? H225_AliasAddress::e_dialedDigits : H225_AliasAddress::e_h323_ID;

  alias.SetTag(tag);
  switch (alias.GetTag()) {
    case H225_AliasAddress::e_dialedDigits :
    case H225_AliasAddress::e_url_ID :
    case H225_AliasAddress::e_email_ID :
      (PASN_IA5String &)alias = name;
      break;

    case H225_AliasAddress::e_h323_ID :
      (PASN_BMPString &)alias = name;
      break;

    case H225_AliasAddress::e_transportID :
    {
      H323TransportAddress addr = name;
      addr.SetPDU(alias);
      break;
    }
    case H225_AliasAddress::e_partyNumber :
    {
      H225_PartyNumber & party = alias;
      if (strncmp(name, E164NumberPrefix, sizeof(E164NumberPrefix)-1) == 0) {
        party.SetTag(H225_PartyNumber::e_e164Number);
        H225_PublicPartyNumber & number = party;
        number.m_publicNumberDigits = name.Mid(sizeof(E164NumberPrefix)-1);
      }
      else if (strncmp(name, PrivatePartyPrefix, sizeof(PrivatePartyPrefix)-1) == 0) {
        party.SetTag(H225_PartyNumber::e_privateNumber);
        H225_PrivatePartyNumber & number = party;
        number.m_privateNumberDigits = name.Mid(sizeof(PrivatePartyPrefix)-1);
      }
      else if (strncmp(name, DataPartyPrefix, sizeof(DataPartyPrefix)-1) == 0) {
        party.SetTag(H225_PartyNumber::e_dataPartyNumber);
        (H225_NumberDigits &)party = name.Mid(sizeof(DataPartyPrefix)-1);
      }
      else if (strncmp(name, TelexPartyPrefix, sizeof(TelexPartyPrefix)-1) == 0) {
        party.SetTag(H225_PartyNumber::e_telexPartyNumber);
        (H225_NumberDigits &)party = name.Mid(sizeof(TelexPartyPrefix)-1);
      }
      else if (strncmp(name, NSPNumberPrefix, sizeof(NSPNumberPrefix)-1) == 0) {
        party.SetTag(H225_PartyNumber::e_nationalStandardPartyNumber);
        (H225_NumberDigits &)party = name.Mid(sizeof(NSPNumberPrefix)-1);
      }
    }

    default :
      break;
  }
}


/////////////////////////////////////////////////////////////////////////////

PStringArray H323GetAliasAddressStrings(const H225_ArrayOf_AliasAddress & aliases)
{
  PStringArray strings(aliases.GetSize());

  for (PINDEX i = 0; i < aliases.GetSize(); i++)
    strings[i] = H323GetAliasAddressString(aliases[i]);

  return strings;
}


PString H323GetAliasAddressString(const H225_AliasAddress & alias)
{
  switch (alias.GetTag()) {
    case H225_AliasAddress::e_dialedDigits :
    case H225_AliasAddress::e_url_ID :
    case H225_AliasAddress::e_email_ID :
      return ((const PASN_IA5String &)alias).GetValue();

    case H225_AliasAddress::e_h323_ID :
      return ((const PASN_BMPString &)alias).GetValue();

    case H225_AliasAddress::e_transportID :
      return H323TransportAddress(alias);

    case H225_AliasAddress::e_partyNumber :
    {
      const H225_PartyNumber & party = alias;
      switch (party.GetTag()) {
        case H225_PartyNumber::e_e164Number :
        {
          const H225_PublicPartyNumber & number = party;
          return E164NumberPrefix + (PString)number.m_publicNumberDigits;
        }

        case H225_PartyNumber::e_privateNumber :
        {
          const H225_PrivatePartyNumber & number = party;
          return PrivatePartyPrefix + (PString)number.m_privateNumberDigits;
        }

        case H225_PartyNumber::e_dataPartyNumber :
          return DataPartyPrefix + (PString)(const H225_NumberDigits &)party;

        case H225_PartyNumber::e_telexPartyNumber :
          return TelexPartyPrefix + (PString)(const H225_NumberDigits &)party;

        case H225_PartyNumber::e_nationalStandardPartyNumber :
          return NSPNumberPrefix + (PString)(const H225_NumberDigits &)party;
      }
      break;
    }

    default :
      break;
  }

  return PString();
}


PString H323GetAliasAddressE164(const H225_AliasAddress & alias)
{
  PString str = H323GetAliasAddressString(alias);
  if (IsE164(str))
    return str;

  return PString();
}


PString H323GetAliasAddressE164(const H225_ArrayOf_AliasAddress & aliases)
{
  for (PINDEX i = 0; i < aliases.GetSize(); i++) {
    PString alias = H323GetAliasAddressE164(aliases[i]);
    if (!alias)
      return alias;
  }

  return PString();
}


///////////////////////////////////////////////////////////////////////////////

void H323GetApplicationInfo(OpalProductInfo & info, const H225_VendorIdentifier & vendor)
{
  info.name = vendor.m_productId.AsString();
  info.version = vendor.m_versionId.AsString();

  // Special case, Cisco IOS does not put in the product and version fields
  if (vendor.m_vendor.m_t35CountryCode == 181 &&
      vendor.m_vendor.m_t35Extension == 0 &&
      vendor.m_vendor.m_manufacturerCode == 18) {
    if (info.name.IsEmpty())
      info.name = "Cisco IOS";
    if (info.version.IsEmpty())
      info.version = "12.2";
  }

  info.t35CountryCode   = (BYTE)vendor.m_vendor.m_t35CountryCode.GetValue();
  info.t35Extension     = (BYTE)vendor.m_vendor.m_t35Extension.GetValue();
  info.manufacturerCode = (WORD)vendor.m_vendor.m_manufacturerCode.GetValue();
}


///////////////////////////////////////////////////////////////////////////////

void H323SetRTPPacketization(H245_ArrayOf_RTPPayloadType & rtpPacketizations,
                             PINDEX & rtpPacketizationCount,
                             const OpalMediaFormat & mediaFormat,
                             RTP_DataFrame::PayloadTypes payloadType)
{
  PString mediaPacketization = mediaFormat.GetOptionString(OpalMediaFormat::MediaPacketizationsOption(),
                               mediaFormat.GetOptionString(OpalMediaFormat::MediaPacketizationOption()));
  if (mediaPacketization.IsEmpty())
    return;
  
  // Special case handling by Product Id
  PString h323ProductId = mediaFormat.GetOptionString("h323ProductId");
  if (h323ProductId == "NetMeeting")
    return;

  PStringArray packetizationStrings = mediaPacketization.Tokenise(",");
  for (PINDEX i = 0; i < packetizationStrings.GetSize(); i++) {
    PString & packetizationString = packetizationStrings[i];
    
    rtpPacketizations.SetSize(rtpPacketizationCount+1);
    if (H323SetRTPPacketization(rtpPacketizations[rtpPacketizationCount], packetizationString, mediaFormat, payloadType)) {
      
      // Check if already in list
      PINDEX test;
      for (test = 0; test < rtpPacketizationCount; test++) {
        if (rtpPacketizations[test] == rtpPacketizations[rtpPacketizationCount])
          break;
      }
      if (test == rtpPacketizationCount)
        rtpPacketizationCount++;
    }
  }
}

bool H323SetRTPPacketization(H245_RTPPayloadType & rtpPacketization,
                             const OpalMediaFormat & mediaFormat,
                             RTP_DataFrame::PayloadTypes payloadType)
{
  PString mediaPacketization = mediaFormat.GetOptionString(OpalMediaFormat::MediaPacketizationsOption(),
                               mediaFormat.GetOptionString(OpalMediaFormat::MediaPacketizationOption()));
  if (mediaPacketization.IsEmpty())
    return PFalse;

  // Special case handling by Product Id
  PString h323ProductId = mediaFormat.GetOptionString("h323ProductId");
  if (h323ProductId == "NetMeeting")
    return PFalse;
  
  PStringArray packetizationStrings = mediaPacketization.Tokenise(",");
  
  // Only use the first packetization (= highest priority)
  return H323SetRTPPacketization(rtpPacketization, packetizationStrings[0], mediaFormat, payloadType);
}

bool H323SetRTPPacketization(H245_RTPPayloadType & rtpPacketization,
                             const PString & packetizationString,
                             const OpalMediaFormat & mediaFormat,
                             RTP_DataFrame::PayloadTypes payloadType)
{
  if (packetizationString.NumCompare("RFC") == PObject::EqualTo) {
    rtpPacketization.m_payloadDescriptor.SetTag(H245_RTPPayloadType_payloadDescriptor::e_rfc_number);
    ((PASN_Integer &)rtpPacketization.m_payloadDescriptor) = packetizationString.Mid(3).AsUnsigned();
  } 
  else if (packetizationString.FindSpan("0123456789.") == P_MAX_INDEX) {
    rtpPacketization.m_payloadDescriptor.SetTag(H245_RTPPayloadType_payloadDescriptor::e_oid);
    ((PASN_ObjectId &)rtpPacketization.m_payloadDescriptor) = packetizationString;
  }
  else {
    rtpPacketization.m_payloadDescriptor.SetTag(H245_RTPPayloadType_payloadDescriptor::e_nonStandardIdentifier);
    H245_NonStandardParameter & nonstd = rtpPacketization.m_payloadDescriptor;
    nonstd.m_nonStandardIdentifier.SetTag(H245_NonStandardIdentifier::e_h221NonStandard);
    H245_NonStandardIdentifier_h221NonStandard & h221 = nonstd.m_nonStandardIdentifier;
    h221.m_t35CountryCode = (unsigned)OpalProductInfo::Default().t35CountryCode;
    h221.m_t35Extension = (unsigned)OpalProductInfo::Default().t35Extension;
    h221.m_manufacturerCode = (unsigned)OpalProductInfo::Default().manufacturerCode;
    nonstd.m_data = packetizationString;
  }

  if (payloadType == RTP_DataFrame::IllegalPayloadType)
    payloadType = mediaFormat.GetPayloadType();

  rtpPacketization.IncludeOptionalField(H245_RTPPayloadType::e_payloadType);
  rtpPacketization.m_payloadType = payloadType;

  return PTrue;
}


PString H323GetRTPPacketization(const H245_RTPPayloadType & rtpPacketization)
{
  PString mediaPacketization;

  switch (rtpPacketization.m_payloadDescriptor.GetTag()) {
    case H245_RTPPayloadType_payloadDescriptor::e_rfc_number :
      mediaPacketization.sprintf("RFC%u", ((const PASN_Integer &)rtpPacketization.m_payloadDescriptor).GetValue());
      break;

    case H245_RTPPayloadType_payloadDescriptor::e_oid :
      mediaPacketization = ((const PASN_ObjectId &)rtpPacketization.m_payloadDescriptor).AsString();
      if (mediaPacketization.IsEmpty()) {
        PTRACE(1, "RTP_UDP\tInvalid OID in packetization type.");
      }
      break;
    case H245_RTPPayloadType_payloadDescriptor::e_nonStandardIdentifier :
      mediaPacketization = ((const H245_NonStandardParameter &)rtpPacketization.m_payloadDescriptor).m_data.AsString();
      if (mediaPacketization.IsEmpty()) {
        PTRACE(1, "RTP_UDP\tInvalid non-standard identifier in packetization type.");
      }
      break;

    default :
      PTRACE(1, "RTP_UDP\tUnknown packetization type.");
  }

  return mediaPacketization;
}


bool H323GetRTPPacketization(OpalMediaFormat & mediaFormat, const H245_RTPPayloadType & rtpPacketization)
{
  PString mediaPacketization = H323GetRTPPacketization(rtpPacketization);
  if (mediaPacketization.IsEmpty())
    return false;
  
  mediaFormat.SetOptionString(OpalMediaFormat::MediaPacketizationsOption(), mediaPacketization);
  mediaFormat.SetOptionString(OpalMediaFormat::MediaPacketizationOption(), mediaPacketization.Left(mediaPacketization.Find(',')));
  return true;
}


PString H323GetCapabilityIdentifier(const H245_CapabilityIdentifier & capId)
{
  switch (capId.GetTag()) {
    case H245_CapabilityIdentifier::e_standard :
      return ((const PASN_ObjectId &)capId).AsString();

    case H245_CapabilityIdentifier::e_h221NonStandard :
    {
      PString str;
      const H245_NonStandardParameter & nonStd = capId;
      if (nonStd.m_nonStandardIdentifier.GetTag() == H245_NonStandardIdentifier::e_object)
        str = ((const PASN_ObjectId &)nonStd.m_nonStandardIdentifier).AsString();
      else {
        const H245_NonStandardIdentifier_h221NonStandard & h221 = nonStd.m_nonStandardIdentifier;
        str.sprintf("c=%u,cx=%u,o=%u",
                    h221.m_t35CountryCode.GetValue(),
                    h221.m_t35Extension.GetValue(),
                    h221.m_manufacturerCode.GetValue());
      }
      if (nonStd.m_data.GetSize() > 0)
        str += ':' + nonStd.m_data.AsString();
      return str;
    }
  }

  return PString::Empty();
}


static int ExtractVar(const PString & str, const PString & var)
{
  PRegularExpression regex("(^|[ \t\n,]+)" + var + "[ \t\n]*=[ \t\n]*[0-9]",
                           PRegularExpression::IgnoreCase|PRegularExpression::Extended);

  PINDEX pos, len;
  if (!str.FindRegEx(regex, pos, len))
    return -1;

  return str.Mid(pos+len-1).AsUnsigned();
}


bool H323SetCapabilityIdentifier(const PString & str, H245_CapabilityIdentifier & capId)
{
  PASN_ObjectId oid;
  oid.SetValue(str);
  if (oid.AsString() == str) {
    capId.SetTag(H245_CapabilityIdentifier::e_standard);
    ((PASN_ObjectId &)capId) = oid;
    return true;
  }

  PINDEX colon = str.Find(':');
  if (colon == 0)
    return false;

  if (colon != P_MAX_INDEX && oid.AsString() == str.Left(colon)) {
    capId.SetTag(H245_CapabilityIdentifier::e_h221NonStandard);
    H245_NonStandardParameter & nonStd = capId;
    nonStd.m_nonStandardIdentifier.SetTag(H245_NonStandardIdentifier::e_object);
    ((PASN_ObjectId &)nonStd.m_nonStandardIdentifier) = oid;
    nonStd.m_data = str.Mid(colon+1);
    return true;
  }

  int country = ExtractVar(str, 'c');
  int manufacturer = ExtractVar(str, 'o');
  if (country >= 0 && manufacturer >= 0) {
    capId.SetTag(H245_CapabilityIdentifier::e_h221NonStandard);
    H245_NonStandardParameter & nonStd = capId;
    nonStd.m_nonStandardIdentifier.SetTag(H245_NonStandardIdentifier::e_h221NonStandard);
    H245_NonStandardIdentifier_h221NonStandard & h221 = nonStd.m_nonStandardIdentifier;

    h221.m_t35CountryCode = country;
    h221.m_manufacturerCode = manufacturer;

    int extension = ExtractVar(str, "cx");
    if (extension >= 0)
      h221.m_t35Extension = extension;

    if (colon != P_MAX_INDEX)
      nonStd.m_data = str.Mid(colon+1);
    return true;
  }

  return false;
}


const H245_ParameterValue * H323GetGenericParameter(const H245_ArrayOf_GenericParameter & params, unsigned ordinal)
{
  for (PINDEX i = 0; i < params.GetSize(); i++) {
    const H245_GenericParameter & param = params[i];
    if (param.m_parameterIdentifier.GetTag() == H245_ParameterIdentifier::e_standard &&
                         ((const PASN_Integer &)param.m_parameterIdentifier) == ordinal)
      return &param.m_parameterValue;
  }

  return NULL;
}


bool H323GetGenericParameterBoolean(const H245_ArrayOf_GenericParameter & params, unsigned ordinal)
{
  const H245_ParameterValue * param = H323GetGenericParameter(params, ordinal);
  return param != NULL && param->GetTag() == H245_ParameterValue::e_logical;
}


unsigned H323GetGenericParameterInteger(const H245_ArrayOf_GenericParameter & params,
                                        unsigned ordinal,
                                        unsigned defValue,
                                        H245_ParameterValue::Choices subType)
{
  const H245_ParameterValue * param = H323GetGenericParameter(params, ordinal);
  if (param == NULL || param->GetTag() != (unsigned)subType)
    return defValue;
  return (const PASN_Integer &)*param;
}


H245_ParameterValue * H323AddGenericParameter(H245_ArrayOf_GenericParameter & params, unsigned ordinal, bool reorder)
{
  PINDEX size = params.GetSize();
  params.SetSize(size+1);

  PINDEX pos = size;
  if (reorder) {
    for (pos = size; pos > 0; pos--) {
      const H245_GenericParameter & param = params[pos-1];
      if (param.m_parameterIdentifier.GetTag() == H245_ParameterIdentifier::e_standard &&
                            ((const PASN_Integer &)param.m_parameterIdentifier) < ordinal)
        break;
      params[pos] = param;
    }
  }

  H245_GenericParameter & param = params[pos];
  param.m_parameterIdentifier.SetTag(H245_ParameterIdentifier::e_standard);
  (PASN_Integer &)param.m_parameterIdentifier = ordinal;
  return &param.m_parameterValue;
}


void H323AddGenericParameterBoolean(H245_ArrayOf_GenericParameter & params, unsigned ordinal, bool value, bool reorder)
{
  // Do not include a logical at all if it is false
  if (value)
    H323AddGenericParameter(params, ordinal, reorder)->SetTag(H245_ParameterValue::e_logical);
}


void H323AddGenericParameterInteger(H245_ArrayOf_GenericParameter & params,
                                    unsigned ordinal,
                                    unsigned value,
                                    H245_ParameterValue::Choices subType,
                                    bool reorder)
{
  H245_ParameterValue * param = H323AddGenericParameter(params, ordinal, reorder);
  param->SetTag(subType);
  (PASN_Integer &)*param = value;
}


void H323AddGenericParameterString(H245_ArrayOf_GenericParameter & params, unsigned ordinal, const PString & value, bool reorder)
{
  H245_ParameterValue * param = H323AddGenericParameter(params, ordinal, reorder);
  param->SetTag(H245_ParameterValue::e_octetString);
  (PASN_OctetString &)*param = value;
}


void H323AddGenericParameterOctets(H245_ArrayOf_GenericParameter & params, unsigned ordinal, const PBYTEArray & value, bool reorder)
{
  H245_ParameterValue * param = H323AddGenericParameter(params, ordinal, reorder);
  param->SetTag(H245_ParameterValue::e_octetString);
  (PASN_OctetString &)*param = value;
}


///////////////////////////////////////////////////////////////////////////////

#if OPAL_H460
static void SendSetupFeatureSet(const H323Connection * connection, H225_Setup_UUIE & pdu)
{
  H225_FeatureSet fs;
  
  if(!connection->OnSendFeatureSet(H460_MessageType::e_setup, fs)) {
    return;
  }
  
  if(fs.HasOptionalField(H225_FeatureSet::e_neededFeatures)) {
    pdu.IncludeOptionalField(H225_Setup_UUIE::e_neededFeatures);
    H225_ArrayOf_FeatureDescriptor & fsn = pdu.m_neededFeatures;
    fsn = fs.m_neededFeatures;
  }
  
  if(fs.HasOptionalField(H225_FeatureSet::e_desiredFeatures)) {
    pdu.IncludeOptionalField(H225_Setup_UUIE::e_desiredFeatures);
    H225_ArrayOf_FeatureDescriptor & fsn = pdu.m_desiredFeatures;
    fsn = fs.m_desiredFeatures;
  }
  
  if(fs.HasOptionalField(H225_FeatureSet::e_supportedFeatures)) {
    pdu.IncludeOptionalField(H225_Setup_UUIE::e_supportedFeatures);
    H225_ArrayOf_FeatureDescriptor & fsn = pdu.m_supportedFeatures;
    fsn = fs.m_supportedFeatures;
  }
}


template <typename PDUType>
static void SendFeatureSet(const H323Connection * connection, unsigned code, H225_H323_UU_PDU & msg, PDUType & pdu)
{
  H225_FeatureSet fs;
  if (!connection->OnSendFeatureSet(code,fs))
    return;

  if (code == H460_MessageType::e_callProceeding) {
    pdu.IncludeOptionalField(PDUType::e_featureSet);
    pdu.m_featureSet = fs;
    return;
  }

  if (!fs.HasOptionalField(H225_FeatureSet::e_supportedFeatures))
    return;

  msg.IncludeOptionalField(H225_H323_UU_PDU::e_genericData);

  H225_ArrayOf_FeatureDescriptor & fsn = fs.m_supportedFeatures;
  H225_ArrayOf_GenericData & data = msg.m_genericData;

  for (PINDEX i=0; i < fsn.GetSize(); i++) {
    PINDEX lastPos = data.GetSize();
    data.SetSize(lastPos+1);
    data[lastPos] = fsn[i];
  }
}
#endif


///////////////////////////////////////////////////////////////////////////////

H323SignalPDU::H323SignalPDU()
{
}


static unsigned SetH225Version(const H323Connection & connection,
                               H225_ProtocolIdentifier & protocolIdentifier)
{
  unsigned version = connection.GetSignallingVersion();
  protocolIdentifier.SetValue(psprintf("0.0.8.2250.0.%u", version));
  return version;
}


H225_Setup_UUIE & H323SignalPDU::BuildSetup(const H323Connection & connection,
                                            const H323TransportAddress & destAddr)
{
  H323EndPoint & endpoint = connection.GetEndPoint();

  q931pdu.BuildSetup(connection.GetCallReference());
  SetQ931Fields(connection, PTrue);

  m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_setup);
  H225_Setup_UUIE & setup = m_h323_uu_pdu.m_h323_message_body;

  if (SetH225Version(connection, setup.m_protocolIdentifier) < 3) {
    setup.RemoveOptionalField(H225_Setup_UUIE::e_multipleCalls);
    setup.RemoveOptionalField(H225_Setup_UUIE::e_maintainConnection);
  }

  setup.IncludeOptionalField(H225_Setup_UUIE::e_sourceAddress);
  {
    PString callingParty;
    callingParty = connection.GetStringOptions()(OPAL_OPT_CALLING_PARTY_NAME);
    if (callingParty.IsEmpty())
      H323SetAliasAddresses(endpoint.GetAliasNames(), setup.m_sourceAddress);
    else
      H323SetAliasAddresses(PStringArray(callingParty), setup.m_sourceAddress);
  }

  setup.m_conferenceID = connection.GetConferenceIdentifier();
  setup.m_conferenceGoal.SetTag(H225_Setup_UUIE_conferenceGoal::e_create);
  setup.m_callType.SetTag(H225_CallType::e_pointToPoint);

  setup.m_callIdentifier.m_guid = connection.GetCallIdentifier();
  setup.m_mediaWaitForConnect = PFalse;
  setup.m_canOverlapSend = PFalse;

  if (!destAddr) {
    setup.IncludeOptionalField(H225_Setup_UUIE::e_destCallSignalAddress);
    destAddr.SetPDU(setup.m_destCallSignalAddress);
  }

  PString destAlias = connection.GetRemotePartyName();
  if (!destAlias && destAlias != destAddr) {
    setup.IncludeOptionalField(H225_Setup_UUIE::e_destinationAddress);
    setup.m_destinationAddress.SetSize(1);

    // Try and encode it as a phone number
    H323SetAliasAddress(destAlias, setup.m_destinationAddress[0]);
    if (setup.m_destinationAddress[0].GetTag() == H225_AliasAddress::e_dialedDigits)
      q931pdu.SetCalledPartyNumber(destAlias);
  }

  endpoint.SetEndpointTypeInfo(setup.m_sourceInfo);
  
#if OPAL_H460
  SendSetupFeatureSet(&connection, setup);
#endif

  return setup;
}


#if OPAL_H460
void H323SignalPDU::InsertH460Setup(const H323Connection & connection, H225_Setup_UUIE & setup)
{
   SendSetupFeatureSet(&connection, setup);
}
#endif


H225_CallProceeding_UUIE &
        H323SignalPDU::BuildCallProceeding(const H323Connection & connection)
{
  q931pdu.BuildCallProceeding(connection.GetCallReference());
  SetQ931Fields(connection);

  m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_callProceeding);
  H225_CallProceeding_UUIE & proceeding = m_h323_uu_pdu.m_h323_message_body;

  if (SetH225Version(connection, proceeding.m_protocolIdentifier) < 3) {
    proceeding.RemoveOptionalField(H225_CallProceeding_UUIE::e_multipleCalls);
    proceeding.RemoveOptionalField(H225_CallProceeding_UUIE::e_maintainConnection);
  }

  proceeding.m_callIdentifier.m_guid = connection.GetCallIdentifier();
  connection.GetEndPoint().SetEndpointTypeInfo(proceeding.m_destinationInfo);
  
#if OPAL_H460
  SendFeatureSet<H225_CallProceeding_UUIE>(&connection, H460_MessageType::e_callProceeding, m_h323_uu_pdu, proceeding);
#endif

  return proceeding;
}


H225_Connect_UUIE & H323SignalPDU::BuildConnect(const H323Connection & connection)
{
  q931pdu.BuildConnect(connection.GetCallReference());
  SetQ931Fields(connection);

  m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_connect);
  H225_Connect_UUIE & connect = m_h323_uu_pdu.m_h323_message_body;

  if (SetH225Version(connection, connect.m_protocolIdentifier) < 3) {
    connect.RemoveOptionalField(H225_Connect_UUIE::e_multipleCalls);
    connect.RemoveOptionalField(H225_Connect_UUIE::e_maintainConnection);
  }
  connect.m_callIdentifier.m_guid = connection.GetCallIdentifier();
  connect.m_conferenceID = connection.GetConferenceIdentifier();

  connection.GetEndPoint().SetEndpointTypeInfo(connect.m_destinationInfo);
  
#if OPAL_H460
  SendFeatureSet<H225_Connect_UUIE>(&connection, H460_MessageType::e_connect, m_h323_uu_pdu, connect);
#endif

  return connect;
}


H225_Connect_UUIE & H323SignalPDU::BuildConnect(const H323Connection & connection,
                                                const PIPSocket::Address & h245Address,
                                                WORD port)
{
  H225_Connect_UUIE & connect = BuildConnect(connection);

  // indicate we are including the optional H245 address in the PDU
  connect.IncludeOptionalField(H225_Connect_UUIE::e_h245Address);

  // convert IP address into the correct H245 type
  H323TransportAddress transAddr(h245Address, port);
  transAddr.SetPDU(connect.m_h245Address);

  return connect;
}


H225_Alerting_UUIE & H323SignalPDU::BuildAlerting(const H323Connection & connection)
{
  q931pdu.BuildAlerting(connection.GetCallReference());
  SetQ931Fields(connection);

  m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_alerting);
  H225_Alerting_UUIE & alerting = m_h323_uu_pdu.m_h323_message_body;

  if (SetH225Version(connection, alerting.m_protocolIdentifier) < 3) {
    alerting.RemoveOptionalField(H225_Alerting_UUIE::e_multipleCalls);
    alerting.RemoveOptionalField(H225_Alerting_UUIE::e_maintainConnection);
  }

  alerting.m_callIdentifier.m_guid = connection.GetCallIdentifier();
  connection.GetEndPoint().SetEndpointTypeInfo(alerting.m_destinationInfo);
  
#if OPAL_H460
  SendFeatureSet<H225_Alerting_UUIE>(&connection, H460_MessageType::e_alerting, m_h323_uu_pdu, alerting);
#endif

  return alerting;
}


H225_Information_UUIE & H323SignalPDU::BuildInformation(const H323Connection & connection)
{
  q931pdu.BuildInformation(connection.GetCallReference(), connection.HadAnsweredCall());
  SetQ931Fields(connection);

  m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_information);
  H225_Information_UUIE & information = m_h323_uu_pdu.m_h323_message_body;

  SetH225Version(connection, information.m_protocolIdentifier);
  information.m_callIdentifier.m_guid = connection.GetCallIdentifier();

  return information;
}


static OpalConnection::CallEndReason H323TranslateToCallEndReasonCode(Q931::CauseValues cause, unsigned reasonTag)
{
  switch (cause) {
    case Q931::ErrorInCauseIE :
      switch (reasonTag) {
        case H225_ReleaseCompleteReason::e_noBandwidth :
          return H323Connection::EndedByNoBandwidth;

        case H225_ReleaseCompleteReason::e_gatekeeperResources :
        case H225_ReleaseCompleteReason::e_gatewayResources :
        case H225_ReleaseCompleteReason::e_adaptiveBusy :
          return H323Connection::EndedByRemoteCongestion;

        case H225_ReleaseCompleteReason::e_unreachableDestination :
          return H323Connection::EndedByUnreachable;

        case H225_ReleaseCompleteReason::e_calledPartyNotRegistered :
          return H323Connection::EndedByNoUser;

        case H225_ReleaseCompleteReason::e_callerNotRegistered:
          return H323Connection::EndedByGatekeeper;

        case H225_ReleaseCompleteReason::e_securityDenied:
          return H323Connection::EndedBySecurityDenial;

        case H225_ReleaseCompleteReason::e_newConnectionNeeded:
          return H323Connection::EndedByTemporaryFailure;
      }
      // Do next case

    case Q931::UnknownCauseIE :
      return H323Connection::EndedByRefusal;

    case Q931::NormalCallClearing :
      return H323Connection::EndedByRemoteUser;

    case Q931::UserBusy :
      return H323Connection::EndedByRemoteBusy;

    case Q931::Congestion :
    case Q931::NoCircuitChannelAvailable :
    case Q931::RequestedCircuitNotAvailable :
    case Q931::ResourceUnavailable :
      return H323Connection::EndedByRemoteCongestion;

    case Q931::CallRejected :
      return H323Connection::EndedByRefusal;

    case Q931::NoResponse :
    case Q931::NoAnswer :
      return H323Connection::EndedByNoAnswer;

    case Q931::NoRouteToNetwork :
    case Q931::ChannelUnacceptable :
      return H323Connection::EndedByUnreachable;

    case Q931::UnallocatedNumber :
    case Q931::NoRouteToDestination :
    case Q931::SubscriberAbsent :
      return H323Connection::EndedByNoUser;

    case Q931::Redirection :
      return H323Connection::EndedByCallForwarded;

    case Q931::DestinationOutOfOrder :
      return H323Connection::EndedByConnectFail;

    case Q931::TemporaryFailure :
      return H323Connection::EndedByTemporaryFailure;

    default:
      return H323Connection::EndedByQ931Cause;
  }
}

H323Connection::CallEndReason H323TranslateToCallEndReason(Q931::CauseValues cause, unsigned reasonTag)
{
  return OpalConnection::CallEndReason(H323TranslateToCallEndReasonCode(cause, reasonTag), cause);
}


Q931::CauseValues H323TranslateFromCallEndReason(H323Connection::CallEndReason callEndReason,
                                                 H225_ReleaseCompleteReason & releaseCompleteReason)
{
  static int const ReasonCodes[H323Connection::NumCallEndReasons] = {
    Q931::NormalCallClearing,                               /// EndedByLocalUser,         Local endpoint application cleared call
    Q931::UserBusy,                                         /// EndedByNoAccept,          Local endpoint did not accept call
    Q931::CallRejected,                                     /// EndedByAnswerDenied,      Local endpoint declined to answer call
    Q931::NormalCallClearing,                               /// EndedByRemoteUser,        Remote endpoint application cleared call
    -H225_ReleaseCompleteReason::e_destinationRejection,    /// EndedByRefusal,           Remote endpoint refused call
    Q931::NoAnswer,                                         /// EndedByNoAnswer,          Remote endpoint did not answer in required time
    Q931::NormalCallClearing,                               /// EndedByCallerAbort,       Remote endpoint stopped calling
    -H225_ReleaseCompleteReason::e_undefinedReason,         /// EndedByTransportFail,     Transport error cleared call
    -H225_ReleaseCompleteReason::e_unreachableDestination,  /// EndedByConnectFail,       Transport connection failed to establish call
    -H225_ReleaseCompleteReason::e_gatekeeperResources,     /// EndedByGatekeeper,        Gatekeeper has cleared call
    -H225_ReleaseCompleteReason::e_calledPartyNotRegistered,/// EndedByNoUser,            Call failed as could not find user (in GK)
    -H225_ReleaseCompleteReason::e_noBandwidth,             /// EndedByNoBandwidth,       Call failed as could not get enough bandwidth
    -H225_ReleaseCompleteReason::e_undefinedReason,         /// EndedByCapabilityExchange,Could not find common capabilities
    -H225_ReleaseCompleteReason::e_facilityCallDeflection,  /// EndedByCallForwarded,     Call was forwarded using FACILITY message
    -H225_ReleaseCompleteReason::e_securityDenied,          /// EndedBySecurityDenial,    Call failed a security check and was ended
    Q931::UserBusy,                                         /// EndedByLocalBusy,         Local endpoint busy
    Q931::Congestion,                                       /// EndedByLocalCongestion,   Local endpoint congested
    Q931::UserBusy,                                         /// EndedByRemoteBusy,        Remote endpoint busy
    Q931::Congestion,                                       /// EndedByRemoteCongestion,  Remote endpoint congested
    Q931::NoRouteToDestination,                             /// EndedByUnreachable,       Could not reach the remote party
    Q931::InvalidCallReference,                             /// EndedByNoEndPoint,        The remote party is not running an endpoint
    Q931::DestinationOutOfOrder,                            /// EndedByHostOffline,       The remote party host off line
    Q931::TemporaryFailure,                                 /// EndedByTemporaryFailure   The remote failed temporarily app may retry
    Q931::UnknownCauseIE,                                   /// EndedByQ931Cause,         The remote ended the call with unmapped Q.931 cause code
    Q931::NormalUnspecified,                                /// EndedByDurationLimit,     Call cleared due to an enforced duration limit
    Q931::InvalidCallReference,                             /// EndedByInvalidConferenceID,  Call cleared due to invalid conference ID
    Q931::NoResponse,                                       /// EndedByNoDialTone,           Call cleared due to missing dial tone
    Q931::NoResponse,                                       /// EndedByNoRingBackTone,       Call cleared due to missing ringback tone
    Q931::DestinationOutOfOrder,                            /// EndedByOutOfService,         Call cleared because the line is out of service, 
    Q931::Redirection,                                      /// EndedByAcceptingCallWaiting, Call cleared because another call is answered
    Q931::ExchangeRoutingError,                             /// EndedByGkAdmissionFailed,    Call cleared because gatekeeper admission request failed.
  };

  if (callEndReason.q931 != Q931::UnknownCauseIE)
    return (Q931::CauseValues)callEndReason.q931;

  int code = ReasonCodes[callEndReason];
  if (code >= 0)
    return (Q931::CauseValues)code;

  releaseCompleteReason.SetTag(-code);
  return Q931::ErrorInCauseIE;
}


H225_ReleaseComplete_UUIE &
        H323SignalPDU::BuildReleaseComplete(const H323Connection & connection)
{
  q931pdu.BuildReleaseComplete(connection.GetCallReference(), connection.HadAnsweredCall());

  m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_releaseComplete);

  H225_ReleaseComplete_UUIE & release = m_h323_uu_pdu.m_h323_message_body;

  SetH225Version(connection, release.m_protocolIdentifier);
  release.m_callIdentifier.m_guid = connection.GetCallIdentifier();

  Q931::CauseValues cause = H323TranslateFromCallEndReason(connection.GetCallEndReason(), release.m_reason);
  if (cause != Q931::ErrorInCauseIE)
    q931pdu.SetCause(cause);
  else
    release.IncludeOptionalField(H225_ReleaseComplete_UUIE::e_reason);

#if OPAL_H460
  SendFeatureSet<H225_ReleaseComplete_UUIE>(&connection, H460_MessageType::e_releaseComplete, m_h323_uu_pdu, release);
#endif

  return release;
}


H225_Facility_UUIE * H323SignalPDU::BuildFacility(const H323Connection & connection, bool empty, unsigned reason)
{
  q931pdu.BuildFacility(connection.GetCallReference(), connection.HadAnsweredCall());
  if (empty) {
    m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_empty);
    return NULL;
  }

  m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_facility);
  H225_Facility_UUIE & fac = m_h323_uu_pdu.m_h323_message_body;

  SetH225Version(connection, fac.m_protocolIdentifier);
  fac.IncludeOptionalField(H225_Facility_UUIE::e_callIdentifier);
  fac.m_callIdentifier.m_guid = connection.GetCallIdentifier();
  
#if OPAL_H460
  if (reason == H225_FacilityReason::e_featureSetUpdate)
    SendFeatureSet<H225_Facility_UUIE>(&connection, H460_MessageType::e_facility, m_h323_uu_pdu, fac);
#endif

  return &fac;
}


H225_Progress_UUIE & H323SignalPDU::BuildProgress(const H323Connection & connection)
{
  q931pdu.BuildProgress(connection.GetCallReference(), connection.HadAnsweredCall(), Q931::ProgressInbandInformationAvailable);
  SetQ931Fields(connection);

  m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_progress);
  H225_Progress_UUIE & progress = m_h323_uu_pdu.m_h323_message_body;

  SetH225Version(connection, progress.m_protocolIdentifier);
  progress.m_callIdentifier.m_guid = connection.GetCallIdentifier();
  connection.GetEndPoint().SetEndpointTypeInfo(progress.m_destinationInfo);

  return progress;
}


H225_Status_UUIE & H323SignalPDU::BuildStatus(const H323Connection & connection)
{
  q931pdu.BuildStatus(connection.GetCallReference(), connection.HadAnsweredCall());

  m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_status);
  H225_Status_UUIE & status = m_h323_uu_pdu.m_h323_message_body;

  SetH225Version(connection, status.m_protocolIdentifier);
  status.m_callIdentifier.m_guid = connection.GetCallIdentifier();

  return status;
}

H225_StatusInquiry_UUIE & H323SignalPDU::BuildStatusInquiry(const H323Connection & connection)
{
  q931pdu.BuildStatusEnquiry(connection.GetCallReference(), connection.HadAnsweredCall());

  m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_statusInquiry);
  H225_StatusInquiry_UUIE & inquiry = m_h323_uu_pdu.m_h323_message_body;

  SetH225Version(connection, inquiry.m_protocolIdentifier);
  inquiry.m_callIdentifier.m_guid = connection.GetCallIdentifier();

  return inquiry;
}


H225_SetupAcknowledge_UUIE & H323SignalPDU::BuildSetupAcknowledge(const H323Connection & connection)
{
  q931pdu.BuildSetupAcknowledge(connection.GetCallReference());

  m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_setupAcknowledge);
  H225_SetupAcknowledge_UUIE & setupAck = m_h323_uu_pdu.m_h323_message_body;

  SetH225Version(connection, setupAck.m_protocolIdentifier);
  setupAck.m_callIdentifier.m_guid = connection.GetCallIdentifier();

  return setupAck;
}


H225_Notify_UUIE & H323SignalPDU::BuildNotify(const H323Connection & connection)
{
  q931pdu.BuildNotify(connection.GetCallReference(), connection.HadAnsweredCall());

  m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_notify);
  H225_Notify_UUIE & notify = m_h323_uu_pdu.m_h323_message_body;

  SetH225Version(connection, notify.m_protocolIdentifier);
  notify.m_callIdentifier.m_guid = connection.GetCallIdentifier();

  return notify;
}


void H323SignalPDU::BuildQ931()
{
  // Encode the H225 PDu into the Q931 PDU as User-User data
  PPER_Stream strm;
  Encode(strm);
  strm.CompleteEncoding();
  q931pdu.SetIE(Q931::UserUserIE, strm);
}


void H323SignalPDU::PrintOn(ostream & strm) const
{
  int indent = strm.precision() + 2;
  strm << "{\n"
       << setw(indent+10) << "q931pdu = " << setprecision(indent) << q931pdu << '\n'
       << setw(indent+10) << "h225pdu = " << setprecision(indent);
  H225_H323_UserInformation::PrintOn(strm);
  strm << '\n'
       << setw(indent-1) << "}";
}


PBoolean H323SignalPDU::Read(H323Transport & transport)
{
  PBYTEArray rawData;
  if (!transport.ReadPDU(rawData)) {
    PTRACE_IF(1, transport.GetErrorCode(PChannel::LastReadError) != PChannel::Timeout,
              "H225\tRead error (" << transport.GetErrorNumber(PChannel::LastReadError)
              << "): " << transport.GetErrorText(PChannel::LastReadError));
    return PFalse;
  }

  if (!q931pdu.Decode(rawData)) {
    PTRACE(1, "H225\tParse error of Q931 PDU:\n" << hex << setfill('0')
                                                 << setprecision(2) << rawData
                                                 << dec << setfill(' '));
    return PFalse;
  }

  if (!q931pdu.HasIE(Q931::UserUserIE)) {
    m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_empty);
    PTRACE(1, "H225\tNo Q931 User-User Information Element,"
              "\nRaw PDU:\n" << hex << setfill('0')
                             << setprecision(2) << rawData
                             << dec << setfill(' ') <<
              "\nQ.931 PDU:\n  " << setprecision(2) << q931pdu);
    return PTrue;
  }

  PPER_Stream strm = q931pdu.GetIE(Q931::UserUserIE);
  if (!Decode(strm)) {
    PTRACE(1, "H225\tRead error: PER decode failure in Q.931 User-User Information Element,"
              "\nRaw PDU:\n" << hex << setfill('0')
                             << setprecision(2) << rawData
                             << dec << setfill(' ') <<
              "\nQ.931 PDU:\n  " << setprecision(2) << q931pdu <<
              "\nPartial PDU:\n  " << setprecision(2) << *this);
    m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_empty);
    return PTrue;
  }

  H323TraceDumpPDU("H225", PFalse, rawData, *this, m_h323_uu_pdu.m_h323_message_body, 0);
  return PTrue;
}


PBoolean H323SignalPDU::Write(H323Transport & transport)
{
  if (!q931pdu.HasIE(Q931::UserUserIE) && m_h323_uu_pdu.m_h323_message_body.IsValid())
    BuildQ931();

  PBYTEArray rawData;
  if (!q931pdu.Encode(rawData))
    return PFalse;

  H323TraceDumpPDU("H225", PTrue, rawData, *this, m_h323_uu_pdu.m_h323_message_body, 0);

  if (transport.WritePDU(rawData))
    return PTrue;

  PTRACE(1, "H225\tWrite PDU failed ("
         << transport.GetErrorNumber(PChannel::LastWriteError)
         << "): " << transport.GetErrorText(PChannel::LastWriteError));
  return PFalse;
}


PString H323SignalPDU::GetSourceAliases(const H323Transport * transport) const
{
  PString remoteHostName;
  
  if (transport != NULL)
    remoteHostName = transport->GetRemoteAddress().GetHostName();

  PString displayName = GetQ931().GetDisplayName();

  PStringStream aliases;
  if (displayName != remoteHostName)
    aliases << displayName;

  if (m_h323_uu_pdu.m_h323_message_body.GetTag() == H225_H323_UU_PDU_h323_message_body::e_setup) {
    const H225_Setup_UUIE & setup = m_h323_uu_pdu.m_h323_message_body;

    if (remoteHostName.IsEmpty() &&
        setup.HasOptionalField(H225_Setup_UUIE::e_sourceCallSignalAddress)) {
      H323TransportAddress remoteAddress(setup.m_sourceCallSignalAddress);
      remoteHostName = remoteAddress.GetHostName();
    }

    if (setup.m_sourceAddress.GetSize() > 0) {
      PBoolean needParen = !aliases.IsEmpty();
      PBoolean needComma = PFalse;
      for (PINDEX i = 0; i < setup.m_sourceAddress.GetSize(); i++) {
        PString alias = H323GetAliasAddressString(setup.m_sourceAddress[i]);
        if (alias != displayName && alias != remoteHostName) {
          if (needComma)
            aliases << ", ";
          else if (needParen)
            aliases << " (";
          aliases << alias;
          needComma = PTrue;
        }
      }
      if (needParen && needComma)
        aliases << ')';
    }
  }

  if (aliases.IsEmpty())
    return remoteHostName;

  aliases << " [" << remoteHostName << ']';
  aliases.MakeMinimumSize();
  return aliases;
}


PString H323SignalPDU::GetDestinationAlias(PBoolean firstAliasOnly) const
{
  PStringStream aliases;

  PString number;
  if (GetQ931().GetCalledPartyNumber(number)) {
    if (firstAliasOnly)
      return number;
    aliases << number;
  }

  if (m_h323_uu_pdu.m_h323_message_body.GetTag() == H225_H323_UU_PDU_h323_message_body::e_setup) {
    const H225_Setup_UUIE & setup = m_h323_uu_pdu.m_h323_message_body;
    if (setup.m_destinationAddress.GetSize() > 0) {
      if (firstAliasOnly)
        return H323GetAliasAddressString(setup.m_destinationAddress[0]);

      for (PINDEX i = 0; i < setup.m_destinationAddress.GetSize(); i++) {
        if (!aliases.IsEmpty())
          aliases << '\t';
        aliases << H323GetAliasAddressString(setup.m_destinationAddress[i]);
      }
    }

    if (setup.HasOptionalField(H225_Setup_UUIE::e_destCallSignalAddress)) {
      if (!aliases.IsEmpty())
        aliases << '\t';
      aliases << H323TransportAddress(setup.m_destCallSignalAddress);
    }
  }

  aliases.MakeMinimumSize();
  return aliases;
}


PBoolean H323SignalPDU::GetSourceE164(PString & number) const
{
  if (GetQ931().GetCallingPartyNumber(number))
    return PTrue;

  if (m_h323_uu_pdu.m_h323_message_body.GetTag() != H225_H323_UU_PDU_h323_message_body::e_setup)
    return PFalse;

  const H225_Setup_UUIE & setup = m_h323_uu_pdu.m_h323_message_body;
  if (!setup.HasOptionalField(H225_Setup_UUIE::e_sourceAddress))
    return PFalse;

  PINDEX i;
  for (i = 0; i < setup.m_sourceAddress.GetSize(); i++) {
    if (setup.m_sourceAddress[i].GetTag() == H225_AliasAddress::e_dialedDigits) {
      number = (PASN_IA5String &)setup.m_sourceAddress[i];
      return PTrue;
    }
  }

  for (i = 0; i < setup.m_sourceAddress.GetSize(); i++) {
    PString str = H323GetAliasAddressString(setup.m_sourceAddress[i]);
    if (IsE164(str)) {
      number = str;
      return PTrue;
    }
  }

  return PFalse;
}


PBoolean H323SignalPDU::GetDestinationE164(PString & number) const
{
  if (GetQ931().GetCalledPartyNumber(number))
    return PTrue;

  if (m_h323_uu_pdu.m_h323_message_body.GetTag() != H225_H323_UU_PDU_h323_message_body::e_setup)
    return PFalse;

  const H225_Setup_UUIE & setup = m_h323_uu_pdu.m_h323_message_body;
  if (!setup.HasOptionalField(H225_Setup_UUIE::e_destinationAddress))
    return PFalse;

  PINDEX i;
  for (i = 0; i < setup.m_destinationAddress.GetSize(); i++) {
    if (setup.m_destinationAddress[i].GetTag() == H225_AliasAddress::e_dialedDigits) {
      number = (PASN_IA5String &)setup.m_destinationAddress[i];
      return PTrue;
    }
  }

  for (i = 0; i < setup.m_destinationAddress.GetSize(); i++) {
    PString str = H323GetAliasAddressString(setup.m_destinationAddress[i]);
    if (IsE164(str)) {
      number = str;
      return PTrue;
    }
  }

  return PFalse;
}


unsigned H323SignalPDU::GetDistinctiveRing() const
{
  Q931::SignalInfo sig = GetQ931().GetSignalInfo();
  if (sig < Q931::SignalAlertingPattern0 || sig > Q931::SignalAlertingPattern7)
    return 0;

  return sig - Q931::SignalAlertingPattern0;
}


void H323SignalPDU::SetQ931Fields(const H323Connection & connection, bool insertPartyNumbers)
{
  const PStringList & aliases = connection.GetLocalAliasNames();

  PString localName;
  PString displayName;
  PString number;

  {
    const OpalConnection::StringOptions & stringOptions = connection.GetStringOptions();
    localName   = stringOptions(OPAL_OPT_CALLING_PARTY_NAME, connection.GetLocalPartyName());
    displayName = stringOptions(OPAL_OPT_CALLING_DISPLAY_NAME, connection.GetDisplayName());
  }

  if (IsE164(localName)) {
    number = localName;
    if (displayName.IsEmpty()) {
      for (PStringList::const_iterator alias = aliases.begin(); alias != aliases.end(); ++alias) {
        if (!IsE164(*alias)) {
          displayName = *alias;
          break;
        }
      }
    }
  }
  else {
    if (displayName.IsEmpty())
      displayName = localName;
    for (PStringList::const_iterator alias = aliases.begin(); alias != aliases.end(); ++alias) {
      if (IsE164(*alias)) {
        number = *alias;
        break;
      }
    }
  }

  q931pdu.SetDisplayName(displayName);

  if (insertPartyNumbers) {
    PString otherNumber = connection.GetRemotePartyNumber();
    if (otherNumber.IsEmpty()) {
      PString otherName = connection.GetRemotePartyName();
      if (IsE164(otherName))
        otherNumber = otherName;
    }

    int plan = 1; // ISDN/Telephony numbering system
    int type = 0; // Unknown number type
    int presentation = connection.IsPresentationBlocked() ? 1 : -1; // Presentation Blocked, or no octet3a
    int screening = presentation == -1 ? -1 : 0; // User-provided, not screened

    if (connection.HadAnsweredCall()) {
      if (!number)
        q931pdu.SetCalledPartyNumber(number, plan, type);
      if (!otherNumber)
        q931pdu.SetCallingPartyNumber(otherNumber, plan, type, presentation, screening);
    }
    else {
      if (!number)
        q931pdu.SetCallingPartyNumber(number, plan, type, presentation, screening);
      if (!otherNumber)
        q931pdu.SetCalledPartyNumber(otherNumber, plan, type);
    }
  }

  unsigned ring = connection.GetDistinctiveRing();
  if (ring != 0)
    q931pdu.SetSignalInfo((Q931::SignalInfo)(ring + Q931::SignalAlertingPattern0));
}


/////////////////////////////////////////////////////////////////////////////

H245_RequestMessage & H323ControlPDU::Build(H245_RequestMessage::Choices request)
{
  SetTag(e_request);
  H245_RequestMessage & msg = *this;
  msg.SetTag(request);
  return msg;
}


H245_ResponseMessage & H323ControlPDU::Build(H245_ResponseMessage::Choices response)
{
  SetTag(e_response);
  H245_ResponseMessage & resp = *this;
  resp.SetTag(response);
  return resp;
}


H245_CommandMessage & H323ControlPDU::Build(H245_CommandMessage::Choices command)
{
  SetTag(e_command);
  H245_CommandMessage & cmd = *this;
  cmd.SetTag(command);
  return cmd;
}


H245_IndicationMessage & H323ControlPDU::Build(H245_IndicationMessage::Choices indication)
{
  SetTag(e_indication);
  H245_IndicationMessage & ind = *this;
  ind.SetTag(indication);
  return ind;
}


H245_MasterSlaveDetermination & 
      H323ControlPDU::BuildMasterSlaveDetermination(unsigned terminalType,
                                                    unsigned statusDeterminationNumber)
{
  H245_MasterSlaveDetermination & msd = Build(H245_RequestMessage::e_masterSlaveDetermination);
  msd.m_terminalType = terminalType;
  msd.m_statusDeterminationNumber = statusDeterminationNumber;
  return msd;
}


H245_MasterSlaveDeterminationAck &
      H323ControlPDU::BuildMasterSlaveDeterminationAck(PBoolean isMaster)
{
  H245_MasterSlaveDeterminationAck & msda = Build(H245_ResponseMessage::e_masterSlaveDeterminationAck);
  msda.m_decision.SetTag(isMaster
                            ? H245_MasterSlaveDeterminationAck_decision::e_slave
                            : H245_MasterSlaveDeterminationAck_decision::e_master);
  return msda;
}


H245_MasterSlaveDeterminationReject &
      H323ControlPDU::BuildMasterSlaveDeterminationReject(unsigned cause)
{
  H245_MasterSlaveDeterminationReject & msdr = Build(H245_ResponseMessage::e_masterSlaveDeterminationReject);
  msdr.m_cause.SetTag(cause);
  return msdr;
}


H245_TerminalCapabilitySet &
      H323ControlPDU::BuildTerminalCapabilitySet(const H323Connection & connection,
                                                 unsigned sequenceNumber,
                                                 PBoolean empty)
{
  H245_TerminalCapabilitySet & cap = Build(H245_RequestMessage::e_terminalCapabilitySet);

  cap.m_sequenceNumber = sequenceNumber;
  cap.m_protocolIdentifier.SetValue(H245_ProtocolID, PARRAYSIZE(H245_ProtocolID));

  if (empty)
    return cap;

  cap.IncludeOptionalField(H245_TerminalCapabilitySet::e_multiplexCapability);
  cap.m_multiplexCapability.SetTag(H245_MultiplexCapability::e_h2250Capability);
  H245_H2250Capability & h225_0 = cap.m_multiplexCapability;
  h225_0.m_maximumAudioDelayJitter = connection.GetMaxAudioJitterDelay();
  h225_0.m_receiveMultipointCapability.m_mediaDistributionCapability.SetSize(1);
  h225_0.m_transmitMultipointCapability.m_mediaDistributionCapability.SetSize(1);
  h225_0.m_receiveAndTransmitMultipointCapability.m_mediaDistributionCapability.SetSize(1);
  h225_0.m_t120DynamicPortCapability = PTrue;

  // Set the table of capabilities
  connection.GetLocalCapabilities().BuildPDU(connection, cap);

  return cap;
}


H245_TerminalCapabilitySetAck &
      H323ControlPDU::BuildTerminalCapabilitySetAck(unsigned sequenceNumber)
{
  H245_TerminalCapabilitySetAck & cap = Build(H245_ResponseMessage::e_terminalCapabilitySetAck);
  cap.m_sequenceNumber = sequenceNumber;
  return cap;
}


H245_TerminalCapabilitySetReject &
      H323ControlPDU::BuildTerminalCapabilitySetReject(unsigned sequenceNumber,
                                                       unsigned cause)
{
  H245_TerminalCapabilitySetReject & cap = Build(H245_ResponseMessage::e_terminalCapabilitySetReject);
  cap.m_sequenceNumber = sequenceNumber;
  cap.m_cause.SetTag(cause);

  return cap;
}


H245_OpenLogicalChannel &
      H323ControlPDU::BuildOpenLogicalChannel(unsigned forwardLogicalChannelNumber)
{
  H245_OpenLogicalChannel & open = Build(H245_RequestMessage::e_openLogicalChannel);
  open.m_forwardLogicalChannelNumber = forwardLogicalChannelNumber;
  return open;
}


H245_RequestChannelClose &
      H323ControlPDU::BuildRequestChannelClose(unsigned channelNumber,
                                               unsigned reason)
{
  H245_RequestChannelClose & rcc = Build(H245_RequestMessage::e_requestChannelClose);
  rcc.m_forwardLogicalChannelNumber = channelNumber;
  rcc.IncludeOptionalField(H245_RequestChannelClose::e_reason);
  rcc.m_reason.SetTag(reason);
  return rcc;
}


H245_CloseLogicalChannel &
      H323ControlPDU::BuildCloseLogicalChannel(unsigned channelNumber)
{
  H245_CloseLogicalChannel & clc = Build(H245_RequestMessage::e_closeLogicalChannel);
  clc.m_forwardLogicalChannelNumber = channelNumber;
  clc.m_source.SetTag(H245_CloseLogicalChannel_source::e_lcse);
  return clc;
}


H245_OpenLogicalChannelAck &
      H323ControlPDU::BuildOpenLogicalChannelAck(unsigned channelNumber)
{
  H245_OpenLogicalChannelAck & ack = Build(H245_ResponseMessage::e_openLogicalChannelAck);
  ack.m_forwardLogicalChannelNumber = channelNumber;
  return ack;
}


H245_OpenLogicalChannelReject &
      H323ControlPDU::BuildOpenLogicalChannelReject(unsigned channelNumber,
                                                    unsigned cause)
{
  H245_OpenLogicalChannelReject & reject = Build(H245_ResponseMessage::e_openLogicalChannelReject);
  reject.m_forwardLogicalChannelNumber = channelNumber;
  reject.m_cause.SetTag(cause);
  return reject;
}


H245_OpenLogicalChannelConfirm &
      H323ControlPDU::BuildOpenLogicalChannelConfirm(unsigned channelNumber)
{
  H245_OpenLogicalChannelConfirm & chan = Build(H245_IndicationMessage::e_openLogicalChannelConfirm);
  chan.m_forwardLogicalChannelNumber = channelNumber;
  return chan;
}


H245_CloseLogicalChannelAck &
      H323ControlPDU::BuildCloseLogicalChannelAck(unsigned channelNumber)
{
  H245_CloseLogicalChannelAck & chan = Build(H245_ResponseMessage::e_closeLogicalChannelAck);
  chan.m_forwardLogicalChannelNumber = channelNumber;
  return chan;
}


H245_RequestChannelCloseAck &
      H323ControlPDU::BuildRequestChannelCloseAck(unsigned channelNumber)
{
  H245_RequestChannelCloseAck & rcca = Build(H245_ResponseMessage::e_requestChannelCloseAck);
  rcca.m_forwardLogicalChannelNumber = channelNumber;
  return rcca;
}


H245_RequestChannelCloseReject &
      H323ControlPDU::BuildRequestChannelCloseReject(unsigned channelNumber)
{
  H245_RequestChannelCloseReject & rccr = Build(H245_ResponseMessage::e_requestChannelCloseReject);
  rccr.m_forwardLogicalChannelNumber = channelNumber;
  return rccr;
}


H245_RequestChannelCloseRelease &
      H323ControlPDU::BuildRequestChannelCloseRelease(unsigned channelNumber)
{
  H245_RequestChannelCloseRelease & rccr = Build(H245_IndicationMessage::e_requestChannelCloseRelease);
  rccr.m_forwardLogicalChannelNumber = channelNumber;
  return rccr;
}


H245_RequestMode & H323ControlPDU::BuildRequestMode(unsigned sequenceNumber)
{
  H245_RequestMode & rm = Build(H245_RequestMessage::e_requestMode);
  rm.m_sequenceNumber = sequenceNumber;

  return rm;
}


H245_RequestModeAck & H323ControlPDU::BuildRequestModeAck(unsigned sequenceNumber,
                                                          unsigned response)
{
  H245_RequestModeAck & ack = Build(H245_ResponseMessage::e_requestModeAck);
  ack.m_sequenceNumber = sequenceNumber;
  ack.m_response.SetTag(response);
  return ack;
}


H245_RequestModeReject & H323ControlPDU::BuildRequestModeReject(unsigned sequenceNumber,
                                                                unsigned cause)
{
  H245_RequestModeReject & reject = Build(H245_ResponseMessage::e_requestModeReject);
  reject.m_sequenceNumber = sequenceNumber;
  reject.m_cause.SetTag(cause);
  return reject;
}


H245_RoundTripDelayRequest &
      H323ControlPDU::BuildRoundTripDelayRequest(unsigned sequenceNumber)
{
  H245_RoundTripDelayRequest & req = Build(H245_RequestMessage::e_roundTripDelayRequest);
  req.m_sequenceNumber = sequenceNumber;
  return req;
}


H245_RoundTripDelayResponse &
      H323ControlPDU::BuildRoundTripDelayResponse(unsigned sequenceNumber)
{
  H245_RoundTripDelayResponse & resp = Build(H245_ResponseMessage::e_roundTripDelayResponse);
  resp.m_sequenceNumber = sequenceNumber;
  return resp;
}


H245_UserInputIndication &
      H323ControlPDU::BuildUserInputIndication(const PString & value)
{
  H245_UserInputIndication & ind = Build(H245_IndicationMessage::e_userInput);
  ind.SetTag(H245_UserInputIndication::e_alphanumeric);
  (PASN_GeneralString &)ind = value;
  return ind;
}


H245_UserInputIndication & H323ControlPDU::BuildUserInputIndication(char tone,
                                                                    unsigned duration,
                                                                    unsigned logicalChannel,
                                                                    unsigned rtpTimestamp)
{
  H245_UserInputIndication & ind = Build(H245_IndicationMessage::e_userInput);

  if (tone != ' ') {
    ind.SetTag(H245_UserInputIndication::e_signal);
    H245_UserInputIndication_signal & sig = ind;

    sig.m_signalType.SetValue(tone);

    if (duration > 0) {
      sig.IncludeOptionalField(H245_UserInputIndication_signal::e_duration);
      sig.m_duration = duration;
    }

    if (logicalChannel > 0) {
      sig.IncludeOptionalField(H245_UserInputIndication_signal::e_rtp);
      sig.m_rtp.m_logicalChannelNumber = logicalChannel;
      sig.m_rtp.m_timestamp = rtpTimestamp;
    }
  }
  else {
    ind.SetTag(H245_UserInputIndication::e_signalUpdate);
    H245_UserInputIndication_signalUpdate & sig = ind;

    sig.m_duration = duration;
    if (logicalChannel > 0) {
      sig.IncludeOptionalField(H245_UserInputIndication_signalUpdate::e_rtp);
      sig.m_rtp.m_logicalChannelNumber = logicalChannel;
    }
  }

  return ind;
}


H245_MiscellaneousCommand & H323ControlPDU::BuildMiscellaneousCommand(unsigned channelNumber, unsigned type)
{
  H245_MiscellaneousCommand & miscCommand = Build(H245_CommandMessage::e_miscellaneousCommand);
  miscCommand.m_logicalChannelNumber = channelNumber;
  miscCommand.m_type.SetTag(type);
  return miscCommand;
}


H245_FlowControlCommand & H323ControlPDU::BuildFlowControlCommand(unsigned channelNumber, unsigned maxBitRate)
{
  H245_FlowControlCommand & flowControlCommand = Build(H245_CommandMessage::e_flowControlCommand);
  flowControlCommand.m_scope.SetTag(H245_FlowControlCommand_scope::e_logicalChannelNumber);
  PASN_Integer & logChanNumPDU = flowControlCommand.m_scope;
  logChanNumPDU = channelNumber;

  flowControlCommand.m_restriction.SetTag(H245_FlowControlCommand_restriction::e_maximumBitRate);
  PASN_Integer & maxBitRatePDU = flowControlCommand.m_restriction;
  maxBitRatePDU = maxBitRate;
 
  return flowControlCommand;
}


H245_MiscellaneousIndication & H323ControlPDU::BuildMiscellaneousIndication(unsigned channelNumber, unsigned type)
{
  H245_MiscellaneousIndication & miscIndication = Build(H245_IndicationMessage::e_miscellaneousIndication);
  miscIndication.m_logicalChannelNumber = channelNumber;
  miscIndication.m_type.SetTag(type);
  return miscIndication;
}


H245_GenericMessage & H323ControlPDU::BuildGenericRequest(const PString & identifier, unsigned subMsgId)
{
  H245_GenericMessage & msg = Build(H245_RequestMessage::e_genericRequest);
  H323SetCapabilityIdentifier(identifier, msg.m_messageIdentifier);
  msg.m_subMessageIdentifier = subMsgId;
  return msg;
}


H245_GenericMessage & H323ControlPDU::BuildGenericResponse(const PString & identifier, unsigned subMsgId)
{
  H245_GenericMessage & msg = Build(H245_ResponseMessage::e_genericResponse);
  H323SetCapabilityIdentifier(identifier, msg.m_messageIdentifier);

  msg.IncludeOptionalField(H245_GenericMessage::e_subMessageIdentifier);
  msg.m_subMessageIdentifier = subMsgId;

  // Assume always have a content
  msg.IncludeOptionalField(H245_GenericMessage::e_messageContent);
  return msg;
}


H245_GenericMessage & H323ControlPDU::BuildGenericCommand(const PString & identifier, unsigned subMsgId)
{
  H245_GenericMessage & msg = Build(H245_CommandMessage::e_genericCommand);
  H323SetCapabilityIdentifier(identifier, msg.m_messageIdentifier);

  msg.IncludeOptionalField(H245_GenericMessage::e_subMessageIdentifier);
  msg.m_subMessageIdentifier = subMsgId;

  return msg;
}


H245_GenericMessage & H323ControlPDU::BuildGenericIndication(const PString & identifier, unsigned subMsgId)
{
  H245_GenericMessage & msg = Build(H245_IndicationMessage::e_genericIndication);
  H323SetCapabilityIdentifier(identifier, msg.m_messageIdentifier);

  msg.IncludeOptionalField(H245_GenericMessage::e_subMessageIdentifier);
  msg.m_subMessageIdentifier = subMsgId;

  return msg;
}


H245_FunctionNotUnderstood &
      H323ControlPDU::BuildFunctionNotUnderstood(const H323ControlPDU & pdu)
{
  H245_FunctionNotUnderstood & fnu = Build(H245_IndicationMessage::e_functionNotUnderstood);

  switch (pdu.GetTag()) {
    case H245_MultimediaSystemControlMessage::e_request :
      fnu.SetTag(H245_FunctionNotUnderstood::e_request);
      (H245_RequestMessage &)fnu = (const H245_RequestMessage &)pdu;
      break;

    case H245_MultimediaSystemControlMessage::e_response :
      fnu.SetTag(H245_FunctionNotUnderstood::e_response);
      (H245_ResponseMessage &)fnu = (const H245_ResponseMessage &)pdu;
      break;

    case H245_MultimediaSystemControlMessage::e_command :
      fnu.SetTag(H245_FunctionNotUnderstood::e_command);
      (H245_CommandMessage &)fnu = (const H245_CommandMessage &)pdu;
      break;
  }

  return fnu;
}


H245_EndSessionCommand & H323ControlPDU::BuildEndSessionCommand(unsigned reason)
{
  H245_EndSessionCommand & end = Build(H245_CommandMessage::e_endSessionCommand);
  end.SetTag(reason);
  return end;
}


/////////////////////////////////////////////////////////////////////////////

H323RasPDU::H323RasPDU()
{
}


H323RasPDU::H323RasPDU(const H235Authenticators & auth)
  : H323TransactionPDU(auth)
{
}


PObject * H323RasPDU::Clone() const
{
  return new H323RasPDU(*this);
}


PASN_Object & H323RasPDU::GetPDU()
{
  return *this;
}


PASN_Choice & H323RasPDU::GetChoice()
{
  return *this;
}


const PASN_Object & H323RasPDU::GetPDU() const
{
  return *this;
}


const PASN_Choice & H323RasPDU::GetChoice() const
{
  return *this;
}


unsigned H323RasPDU::GetSequenceNumber() const
{
  switch (GetTag()) {
    case H225_RasMessage::e_gatekeeperRequest :
      return ((const H225_GatekeeperRequest &)*this).m_requestSeqNum;

    case H225_RasMessage::e_gatekeeperConfirm :
      return ((const H225_GatekeeperConfirm &)*this).m_requestSeqNum;

    case H225_RasMessage::e_gatekeeperReject :
      return ((const H225_GatekeeperReject &)*this).m_requestSeqNum;

    case H225_RasMessage::e_registrationRequest :
      return ((const H225_RegistrationRequest &)*this).m_requestSeqNum;

    case H225_RasMessage::e_registrationConfirm :
      return ((const H225_RegistrationConfirm &)*this).m_requestSeqNum;

    case H225_RasMessage::e_registrationReject :
      return ((const H225_RegistrationReject &)*this).m_requestSeqNum;

    case H225_RasMessage::e_unregistrationRequest :
      return ((const H225_UnregistrationRequest &)*this).m_requestSeqNum;

    case H225_RasMessage::e_unregistrationConfirm :
      return ((const H225_UnregistrationConfirm &)*this).m_requestSeqNum;

    case H225_RasMessage::e_unregistrationReject :
      return ((const H225_UnregistrationReject &)*this).m_requestSeqNum;

    case H225_RasMessage::e_admissionRequest :
      return ((const H225_AdmissionRequest &)*this).m_requestSeqNum;

    case H225_RasMessage::e_admissionConfirm :
      return ((const H225_AdmissionConfirm &)*this).m_requestSeqNum;

    case H225_RasMessage::e_admissionReject :
      return ((const H225_AdmissionReject &)*this).m_requestSeqNum;

    case H225_RasMessage::e_bandwidthRequest :
      return ((const H225_BandwidthRequest &)*this).m_requestSeqNum;

    case H225_RasMessage::e_bandwidthConfirm :
      return ((const H225_BandwidthConfirm &)*this).m_requestSeqNum;

    case H225_RasMessage::e_bandwidthReject :
      return ((const H225_BandwidthReject &)*this).m_requestSeqNum;

    case H225_RasMessage::e_disengageRequest :
      return ((const H225_DisengageRequest &)*this).m_requestSeqNum;

    case H225_RasMessage::e_disengageConfirm :
      return ((const H225_DisengageConfirm &)*this).m_requestSeqNum;

    case H225_RasMessage::e_disengageReject :
      return ((const H225_DisengageReject &)*this).m_requestSeqNum;

    case H225_RasMessage::e_locationRequest :
      return ((const H225_LocationRequest &)*this).m_requestSeqNum;

    case H225_RasMessage::e_locationConfirm :
      return ((const H225_LocationConfirm &)*this).m_requestSeqNum;

    case H225_RasMessage::e_locationReject :
      return ((const H225_LocationReject &)*this).m_requestSeqNum;

    case H225_RasMessage::e_infoRequest :
      return ((const H225_InfoRequest &)*this).m_requestSeqNum;

    case H225_RasMessage::e_infoRequestResponse :
      return ((const H225_InfoRequestResponse &)*this).m_requestSeqNum;

    case H225_RasMessage::e_nonStandardMessage :
      return ((const H225_NonStandardMessage &)*this).m_requestSeqNum;

    case H225_RasMessage::e_unknownMessageResponse :
      return ((const H225_UnknownMessageResponse &)*this).m_requestSeqNum;

    case H225_RasMessage::e_requestInProgress :
      return ((const H225_RequestInProgress &)*this).m_requestSeqNum;

    case H225_RasMessage::e_resourcesAvailableIndicate :
      return ((const H225_ResourcesAvailableIndicate &)*this).m_requestSeqNum;

    case H225_RasMessage::e_resourcesAvailableConfirm :
      return ((const H225_ResourcesAvailableConfirm &)*this).m_requestSeqNum;

    case H225_RasMessage::e_infoRequestAck :
      return ((const H225_InfoRequestAck &)*this).m_requestSeqNum;

    case H225_RasMessage::e_infoRequestNak :
      return ((const H225_InfoRequestNak &)*this).m_requestSeqNum;

    case H225_RasMessage::e_serviceControlIndication :
      return ((const H225_ServiceControlIndication &)*this).m_requestSeqNum;

    case H225_RasMessage::e_serviceControlResponse :
      return ((const H225_ServiceControlResponse &)*this).m_requestSeqNum;

    default :
      return 0;
  }
}


unsigned H323RasPDU::GetRequestInProgressDelay() const
{
  if (GetTag() != H225_RasMessage::e_requestInProgress)
    return 0;

  return ((const H225_RequestInProgress &)*this).m_delay;
}


#if PTRACING
const char * H323RasPDU::GetProtocolName() const
{
  return "H225RAS";
}
#endif


H323TransactionPDU * H323RasPDU::ClonePDU() const
{
  return new H323RasPDU(*this);
}


void H323RasPDU::DeletePDU()
{
  delete this;
}


H225_GatekeeperRequest & H323RasPDU::BuildGatekeeperRequest(unsigned seqNum)
{
  SetTag(e_gatekeeperRequest);
  H225_GatekeeperRequest & grq = *this;
  grq.m_requestSeqNum = seqNum;
  grq.m_protocolIdentifier.SetValue(H225_ProtocolID, PARRAYSIZE(H225_ProtocolID));
  return grq;
}


H225_GatekeeperConfirm & H323RasPDU::BuildGatekeeperConfirm(unsigned seqNum)
{
  SetTag(e_gatekeeperConfirm);
  H225_GatekeeperConfirm & gcf = *this;
  gcf.m_requestSeqNum = seqNum;
  gcf.m_protocolIdentifier.SetValue(H225_ProtocolID, PARRAYSIZE(H225_ProtocolID));
  return gcf;
}


H225_GatekeeperReject & H323RasPDU::BuildGatekeeperReject(unsigned seqNum, unsigned reason)
{
  SetTag(e_gatekeeperReject);
  H225_GatekeeperReject & grj = *this;
  grj.m_requestSeqNum = seqNum;
  grj.m_protocolIdentifier.SetValue(H225_ProtocolID, PARRAYSIZE(H225_ProtocolID));
  grj.m_rejectReason.SetTag(reason);
  return grj;
}


H225_RegistrationRequest & H323RasPDU::BuildRegistrationRequest(unsigned seqNum)
{
  SetTag(e_registrationRequest);
  H225_RegistrationRequest & rrq = *this;
  rrq.m_requestSeqNum = seqNum;
  rrq.m_protocolIdentifier.SetValue(H225_ProtocolID, PARRAYSIZE(H225_ProtocolID));
  return rrq;
}


H225_RegistrationConfirm & H323RasPDU::BuildRegistrationConfirm(unsigned seqNum)
{
  SetTag(e_registrationConfirm);
  H225_RegistrationConfirm & rcf = *this;
  rcf.m_requestSeqNum = seqNum;
  rcf.m_protocolIdentifier.SetValue(H225_ProtocolID, PARRAYSIZE(H225_ProtocolID));
  return rcf;
}


H225_RegistrationReject & H323RasPDU::BuildRegistrationReject(unsigned seqNum, unsigned reason)
{
  SetTag(e_registrationReject);
  H225_RegistrationReject & rrj = *this;
  rrj.m_requestSeqNum = seqNum;
  rrj.m_protocolIdentifier.SetValue(H225_ProtocolID, PARRAYSIZE(H225_ProtocolID));
  rrj.m_rejectReason.SetTag(reason);
  return rrj;
}


H225_UnregistrationRequest & H323RasPDU::BuildUnregistrationRequest(unsigned seqNum)
{
  SetTag(e_unregistrationRequest);
  H225_UnregistrationRequest & urq = *this;
  urq.m_requestSeqNum = seqNum;
  return urq;
}


H225_UnregistrationConfirm & H323RasPDU::BuildUnregistrationConfirm(unsigned seqNum)
{
  SetTag(e_unregistrationConfirm);
  H225_UnregistrationConfirm & ucf = *this;
  ucf.m_requestSeqNum = seqNum;
  return ucf;
}


H225_UnregistrationReject & H323RasPDU::BuildUnregistrationReject(unsigned seqNum, unsigned reason)
{
  SetTag(e_unregistrationReject);
  H225_UnregistrationReject & urj = *this;
  urj.m_requestSeqNum = seqNum;
  urj.m_rejectReason.SetTag(reason);
  return urj;
}


H225_LocationRequest & H323RasPDU::BuildLocationRequest(unsigned seqNum)
{
  SetTag(e_locationRequest);
  H225_LocationRequest & lrq = *this;
  lrq.m_requestSeqNum = seqNum;
  return lrq;
}


H225_LocationConfirm & H323RasPDU::BuildLocationConfirm(unsigned seqNum)
{
  SetTag(e_locationConfirm);
  H225_LocationConfirm & lcf = *this;
  lcf.m_requestSeqNum = seqNum;
  return lcf;
}


H225_LocationReject & H323RasPDU::BuildLocationReject(unsigned seqNum, unsigned reason)
{
  SetTag(e_locationReject);
  H225_LocationReject & lrj = *this;
  lrj.m_requestSeqNum = seqNum;
  lrj.m_rejectReason.SetTag(reason);
  return lrj;
}


H225_AdmissionRequest & H323RasPDU::BuildAdmissionRequest(unsigned seqNum)
{
  SetTag(e_admissionRequest);
  H225_AdmissionRequest & arq = *this;
  arq.m_requestSeqNum = seqNum;
  return arq;
}


H225_AdmissionConfirm & H323RasPDU::BuildAdmissionConfirm(unsigned seqNum)
{
  SetTag(e_admissionConfirm);
  H225_AdmissionConfirm & acf = *this;
  acf.m_requestSeqNum = seqNum;
  return acf;
}


H225_AdmissionReject & H323RasPDU::BuildAdmissionReject(unsigned seqNum, unsigned reason)
{
  SetTag(e_admissionReject);
  H225_AdmissionReject & arj = *this;
  arj.m_requestSeqNum = seqNum;
  arj.m_rejectReason.SetTag(reason);
  return arj;
}


H225_DisengageRequest & H323RasPDU::BuildDisengageRequest(unsigned seqNum)
{
  SetTag(e_disengageRequest);
  H225_DisengageRequest & drq = *this;
  drq.m_requestSeqNum = seqNum;
  return drq;
}


H225_DisengageConfirm & H323RasPDU::BuildDisengageConfirm(unsigned seqNum)
{
  SetTag(e_disengageConfirm);
  H225_DisengageConfirm & dcf = *this;
  dcf.m_requestSeqNum = seqNum;
  return dcf;
}


H225_DisengageReject & H323RasPDU::BuildDisengageReject(unsigned seqNum, unsigned reason)
{
  SetTag(e_disengageReject);
  H225_DisengageReject & drj = *this;
  drj.m_requestSeqNum = seqNum;
  drj.m_rejectReason.SetTag(reason);
  return drj;
}


H225_BandwidthRequest & H323RasPDU::BuildBandwidthRequest(unsigned seqNum)
{
  SetTag(e_bandwidthRequest);
  H225_BandwidthRequest & brq = *this;
  brq.m_requestSeqNum = seqNum;
  return brq;
}


H225_BandwidthConfirm & H323RasPDU::BuildBandwidthConfirm(unsigned seqNum, unsigned bandWidth)
{
  SetTag(e_bandwidthConfirm);
  H225_BandwidthConfirm & bcf = *this;
  bcf.m_requestSeqNum = seqNum;
  bcf.m_bandWidth = bandWidth;
  return bcf;
}


H225_BandwidthReject & H323RasPDU::BuildBandwidthReject(unsigned seqNum, unsigned reason)
{
  SetTag(e_bandwidthReject);
  H225_BandwidthReject & brj = *this;
  brj.m_requestSeqNum = seqNum;
  brj.m_rejectReason.SetTag(reason);
  return brj;
}


H225_InfoRequest & H323RasPDU::BuildInfoRequest(unsigned seqNum,
                                                unsigned callRef,
                                                const OpalGloballyUniqueID * id)
{
  SetTag(e_infoRequest);
  H225_InfoRequest & irq = *this;
  irq.m_requestSeqNum = seqNum;
  irq.m_callReferenceValue = callRef;
  if (callRef != 0 && id != NULL)
    irq.m_callIdentifier.m_guid = *id;
  return irq;
}


H225_InfoRequestResponse & H323RasPDU::BuildInfoRequestResponse(unsigned seqNum)
{
  SetTag(e_infoRequestResponse);
  H225_InfoRequestResponse & irr = *this;
  irr.m_requestSeqNum = seqNum;
  return irr;
}


H225_InfoRequestAck & H323RasPDU::BuildInfoRequestAck(unsigned seqNum)
{
  SetTag(e_infoRequestAck);
  H225_InfoRequestAck & iack = *this;
  iack.m_requestSeqNum = seqNum;
  return iack;
}


H225_InfoRequestNak & H323RasPDU::BuildInfoRequestNak(unsigned seqNum, unsigned reason)
{
  SetTag(e_infoRequestNak);
  H225_InfoRequestNak & inak = *this;
  inak.m_requestSeqNum = seqNum;
  inak.m_nakReason.SetTag(reason);
  return inak;
}


H225_UnknownMessageResponse & H323RasPDU::BuildUnknownMessageResponse(unsigned seqNum)
{
  SetTag(e_unknownMessageResponse);
  H225_UnknownMessageResponse & umr = *this;
  umr.m_requestSeqNum = seqNum;
  return umr;
}


H225_RequestInProgress & H323RasPDU::BuildRequestInProgress(unsigned seqNum, unsigned delay)
{
  SetTag(e_requestInProgress);
  H225_RequestInProgress & rip = *this;
  rip.m_requestSeqNum = seqNum;
  rip.m_delay = delay;
  return rip;
}


H225_ServiceControlIndication & H323RasPDU::BuildServiceControlIndication(unsigned seqNum, const OpalGloballyUniqueID * id)
{
  SetTag(e_serviceControlIndication);
  H225_ServiceControlIndication & sci = *this;
  sci.m_requestSeqNum = seqNum;

  if (id != NULL && !id->IsNULL()) {
    sci.IncludeOptionalField(H225_ServiceControlIndication::e_callSpecific);
    sci.m_callSpecific.m_callIdentifier.m_guid = *id;
  }

  return sci;
}


H225_ServiceControlResponse & H323RasPDU::BuildServiceControlResponse(unsigned seqNum)
{
  SetTag(e_serviceControlResponse);
  H225_ServiceControlResponse & scr = *this;
  scr.m_requestSeqNum = seqNum;
  return scr;
}


#endif // OPAL_H323

/////////////////////////////////////////////////////////////////////////////
