/*
 * h224.cxx
 *
 * H.224 implementation for the OpenH323 Project.
 *
 * Copyright (c) 2006 Network for Educational Technology, ETH Zurich.
 * Written by Hannes Friederich.
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
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23831 $
 * $Author: rjongbloed $
 * $Date: 2009-12-03 10:21:07 +0000 (Thu, 03 Dec 2009) $
 */

/*
  This file implements H.224 as part of H.323, as well as RFC 4573 for H.224 over SIP 
 */

#include <ptlib.h>

#include <opal/buildopts.h>

#ifdef __GNUC__
#pragma implementation "h224.h"
#pragma implementation "h224handler.h"
#endif

#if OPAL_HAS_H224

#include <h224/h224.h>
#include <h224/h224handler.h>

#if OPAL_SIP
#include <sip/sdp.h>
#endif

#define H224_MAX_HEADER_SIZE 6+5

/////////////////////////////////////////////////////////////////////////

OPAL_INSTANTIATE_MEDIATYPE(h224, OpalH224MediaType);

OpalH224MediaType::OpalH224MediaType()
  : OpalRTPAVPMediaType("h224", "application")
{
}

const OpalMediaType & OpalH224MediaType::MediaType()
{
  static const OpalMediaType type = "h224";
  return type;
}

#if OPAL_SIP

class SDPH224MediaDescription : public SDPRTPAVPMediaDescription
{
  PCLASSINFO(SDPH224MediaDescription, SDPRTPAVPMediaDescription);
  public:
    SDPH224MediaDescription(const OpalTransportAddress & address);
    virtual PString GetSDPMediaType() const;
};

SDPMediaDescription * OpalH224MediaType::CreateSDPMediaDescription(const OpalTransportAddress & localAddress)
{
  return new SDPH224MediaDescription(localAddress);
}

SDPH224MediaDescription::SDPH224MediaDescription(const OpalTransportAddress & address)
  : SDPRTPAVPMediaDescription(address, OpalH224MediaType::MediaType())
{
}

PString SDPH224MediaDescription::GetSDPMediaType() const
{
  return "application";
}

#endif

/////////////////////////////////////////////////////////////////////////

const OpalMediaFormat & GetOpalH224_H323AnnexQ()
{
  static class H224_AnnexQ_MediaFormat : public OpalH224MediaFormat { 
    public: 
      H224_AnnexQ_MediaFormat() 
        : OpalH224MediaFormat("H.224/H323AnnexQ", RTP_DataFrame::DynamicBase)
      { 
        OpalMediaOption * option = new OpalMediaOptionBoolean("HDLC Tunneling", true, OpalMediaOption::MinMerge, false);
        AddOption(option);
      } 
  } const h224q; 
  return h224q; 
};

const OpalMediaFormat & GetOpalH224_HDLCTunneling()
{
  static class H224_HDLCTunneling_MediaFormat : public OpalH224MediaFormat { 
    public: 
      H224_HDLCTunneling_MediaFormat() 
        : OpalH224MediaFormat("H.224/HDLCTunneling", RTP_DataFrame::MaxPayloadType)    // HDLC tunnelled is not sent over RTP
      { 
        OpalMediaOption * option = new OpalMediaOptionBoolean("HDLC Tunneling", true, OpalMediaOption::MinMerge, true);
        AddOption(option);
      } 
  } const h224h; 
  return h224h; 
}

OpalH224MediaFormat::OpalH224MediaFormat(
      const char * fullName,                      ///<  Full name of media format
      RTP_DataFrame::PayloadTypes rtpPayloadType  ///<  RTP payload type code
  ) 
  : OpalMediaFormat(fullName, 
                    "h224", 
                    rtpPayloadType,
                    "h224",
                    PFalse,
                    6400,  // 6.4kbit/s as defined in RFC 4573
                    0,
                    0,
                    4800,  // As defined in RFC 4573
                    0)
{
}

PObject * OpalH224MediaFormat::Clone() const
{
  return new OpalH224MediaFormat(*this);
}

PBoolean OpalH224MediaFormat::IsValidForProtocol(const PString & protocol) const
{
  // HDLC tunnelling only makes sense for H.323. Everything else uses RTP;
  return !GetOptionBoolean("HDLC Tunneling") || (protocol == "h323");
}

/////////////////////////////////////////////////////////////////////////

H224_Frame::H224_Frame(PINDEX size)
: Q922_Frame(H224_MAX_HEADER_SIZE + size)
{
  SetHighPriority(PFalse);	
  SetControlFieldOctet(0x03); // UI-Mode
  SetDestinationTerminalAddress(OpalH224Handler::Broadcast);
  SetSourceTerminalAddress(OpalH224Handler::Broadcast);
  
  // setting Client ID to CME
  SetClientID(OpalH224Client::CMEClientID);
  
  // Setting ES / BS / C1 / C0 / Segment number to zero
  SetBS(PFalse);
  SetES(PFalse);
  SetC1(PFalse);
  SetC0(PFalse);
  SetSegmentNumber(0x00);
  
  SetClientDataSize(size);
}

H224_Frame::H224_Frame(const OpalH224Client & h224Client, PINDEX size)
: Q922_Frame(H224_MAX_HEADER_SIZE + size)
{
  SetHighPriority(PFalse);	
  SetControlFieldOctet(0x03); // UI-Mode
  SetDestinationTerminalAddress(OpalH224Handler::Broadcast);
  SetSourceTerminalAddress(OpalH224Handler::Broadcast);
  
  SetClient(h224Client);
  
  // Setting ES / BS / C1 / C0 / Segment number to zero
  SetBS(PFalse);
  SetES(PFalse);
  SetC1(PFalse);
  SetC0(PFalse);
  SetSegmentNumber(0x00);
  
  SetClientDataSize(size);
}

H224_Frame::~H224_Frame()
{
}

void H224_Frame::SetHighPriority(PBoolean flag)
{
  SetHighOrderAddressOctet(0x00);
	
  if (flag) {
    SetLowOrderAddressOctet(0x71);
  } else {
    SetLowOrderAddressOctet(0x061);
  }
}

WORD H224_Frame::GetDestinationTerminalAddress() const
{
  BYTE *data = GetInformationFieldPtr();
  return (WORD)((data[0] << 8) | data[1]);
}

void H224_Frame::SetDestinationTerminalAddress(WORD address)
{
  BYTE *data = GetInformationFieldPtr();
  data[0] = (BYTE)(address >> 8);
  data[1] = (BYTE) address;
}

WORD H224_Frame::GetSourceTerminalAddress() const
{
  BYTE *data = GetInformationFieldPtr();
  return (WORD)((data[2] << 8) | data[3]);
}

void H224_Frame::SetSourceTerminalAddress(WORD address)
{
  BYTE *data = GetInformationFieldPtr();
  data[2] = (BYTE)(address >> 8);
  data[3] = (BYTE) address;
}

void H224_Frame::SetClient(const OpalH224Client & h224Client)
{
  BYTE clientID = h224Client.GetClientID();
  
  SetClientID(clientID);
  
  if (clientID == OpalH224Client::ExtendedClientID) {
    SetExtendedClientID(h224Client.GetExtendedClientID());
    
  } else if (clientID == OpalH224Client::NonStandardClientID) {
    SetNonStandardClientInformation(h224Client.GetCountryCode(),
                                    h224Client.GetCountryCodeExtension(),
                                    h224Client.GetManufacturerCode(),
                                    h224Client.GetManufacturerClientID());
  }
}

BYTE H224_Frame::GetClientID() const
{
  BYTE *data = GetInformationFieldPtr();
  return data[4] & 0x7f;
}

void H224_Frame::SetClientID(BYTE clientID)
{
	BYTE *data = GetInformationFieldPtr();
  data[4] = (clientID & 0x7f);
}

BYTE H224_Frame::GetExtendedClientID() const
{
  if (GetClientID() != OpalH224Client::ExtendedClientID) {
    return 0x00;
  }
	
  BYTE *data = GetInformationFieldPtr();
  return data[5];
}

void H224_Frame::SetExtendedClientID(BYTE extendedClientID)
{
  if (GetClientID() != OpalH224Client::ExtendedClientID) {
    return;
  }
	
  BYTE *data = GetInformationFieldPtr();
  data[5] = extendedClientID;
}

BYTE H224_Frame::GetCountryCode() const
{
  if (GetClientID() != OpalH224Client::NonStandardClientID) {
    return 0x00;
  }
	
  BYTE *data = GetInformationFieldPtr();
  return data[5];
}

BYTE H224_Frame::GetCountryCodeExtension() const
{
  if (GetClientID() != OpalH224Client::NonStandardClientID) {
    return 0x00;
  }
	
  BYTE *data = GetInformationFieldPtr();
  return data[6];
}

WORD H224_Frame::GetManufacturerCode() const
{
  if (GetClientID() != OpalH224Client::NonStandardClientID) {
    return 0x0000;
  }
	
  BYTE *data = GetInformationFieldPtr();
  return (((WORD)data[7] << 8) | (WORD)data[8]);
}

BYTE H224_Frame::GetManufacturerClientID() const
{
  if (GetClientID() != OpalH224Client::NonStandardClientID) {
    return 0x00;
  }
	
  BYTE *data = GetInformationFieldPtr();
  return data[9];
}

void H224_Frame::SetNonStandardClientInformation(BYTE countryCode,
                                                 BYTE countryCodeExtension,
                                                 WORD manufacturerCode,
                                                 BYTE manufacturerClientID)
{
  if (GetClientID() != OpalH224Client::NonStandardClientID) {	
    return;
  }
	
  BYTE *data = GetInformationFieldPtr();
	
  data[5] = countryCode;
  data[6] = countryCodeExtension;
  data[7] = (BYTE)(manufacturerCode << 8);
  data[8] = (BYTE) manufacturerCode;
  data[9] = manufacturerClientID;
}

PBoolean H224_Frame::GetBS() const
{
  BYTE *data = GetInformationFieldPtr();
	
  return (data[5] & 0x80) != 0;
}

void H224_Frame::SetBS(PBoolean flag)
{
  BYTE *data = GetInformationFieldPtr();
	
  if (flag) {
    data[5] |= 0x80;
  }	else {
    data[5] &= 0x7f;
  }
}

PBoolean H224_Frame::GetES() const
{
  BYTE *data = GetInformationFieldPtr();
	
  return (data[5] & 0x40) != 0;
}

void H224_Frame::SetES(PBoolean flag)
{
  BYTE *data = GetInformationFieldPtr();
	
  if (flag) {
    data[5] |= 0x40;
  } else {
    data[5] &= 0xbf;
  }
}

PBoolean H224_Frame::GetC1() const
{
  BYTE *data = GetInformationFieldPtr();
	
  return (data[5] & 0x20) != 0;
}

void H224_Frame::SetC1(PBoolean flag)
{
  BYTE *data = GetInformationFieldPtr();
	
  if (flag) {
    data[5] |= 0x20;
  } else {
    data[5] &= 0xdf;
  }
}

PBoolean H224_Frame::GetC0() const
{
  BYTE *data = GetInformationFieldPtr();
	
  return (data[5] & 0x10) != 0;
}

void H224_Frame::SetC0(PBoolean flag)
{
  BYTE *data = GetInformationFieldPtr();
	
  if (flag) {
    data[5] |= 0x10;
  }	else {
    data[5] &= 0xef;
  }
}

BYTE H224_Frame::GetSegmentNumber() const
{
  BYTE *data = GetInformationFieldPtr();
	
  return (data[5] & 0x0f);
}

void H224_Frame::SetSegmentNumber(BYTE segmentNumber)
{
  BYTE *data = GetInformationFieldPtr();
	
  data[5] &= 0xf0;
  data[5] |= (segmentNumber & 0x0f);
}

BYTE * H224_Frame::GetClientDataPtr() const
{
  BYTE * data = GetInformationFieldPtr();
  return (data + GetHeaderSize());
}

PINDEX H224_Frame::GetClientDataSize() const
{
  PINDEX size = GetInformationFieldSize();
  return (size - GetHeaderSize());
}

void H224_Frame::SetClientDataSize(PINDEX size)
{
  SetInformationFieldSize(size + GetHeaderSize());
}

PBoolean H224_Frame::DecodeAnnexQ(const BYTE *data, PINDEX size)
{
  PBoolean result = Q922_Frame::DecodeAnnexQ(data, size);
	
  if (result == PFalse) {
    return PFalse;
  }
	
  // doing some validity checks for H.224 frames
  BYTE highOrderAddressOctet = GetHighOrderAddressOctet();
  BYTE lowOrderAddressOctet = GetLowOrderAddressOctet();
  BYTE controlFieldOctet = GetControlFieldOctet();
	
  if ((highOrderAddressOctet != 0x00) ||
      (!(lowOrderAddressOctet == 0x61 || lowOrderAddressOctet == 0x71)) ||
      (controlFieldOctet != 0x03)) {		
	  return PFalse;
  }
	
  return PTrue;
  
}

PBoolean H224_Frame::DecodeHDLC(const BYTE *data, PINDEX size)
{
  PBoolean result = Q922_Frame::DecodeHDLC(data, size);
	
  if (result == PFalse) {
    return PFalse;
  }
	
  // doing some validity checks for H.224 frames
  BYTE highOrderAddressOctet = GetHighOrderAddressOctet();
  BYTE lowOrderAddressOctet = GetLowOrderAddressOctet();
  BYTE controlFieldOctet = GetControlFieldOctet();
	
  if ((highOrderAddressOctet != 0x00) ||
     (!(lowOrderAddressOctet == 0x61 || lowOrderAddressOctet == 0x71)) ||
     (controlFieldOctet != 0x03)) {		
	  return PFalse;
  }
	
  return PTrue;
}

PINDEX H224_Frame::GetHeaderSize() const
{
  BYTE clientID = GetClientID();
  
  if (clientID < OpalH224Client::ExtendedClientID) {
    return 6;
  } else if (clientID == OpalH224Client::ExtendedClientID) {
    return 7; // one extra octet
  } else {
    return 11; // 5 extra octets
  }
}

////////////////////////////////////

OpalH224Handler::OpalH224Handler()
: transmitMutex(),
  transmitFrame(300),
  receiveFrame()
{
  canTransmit = PFalse;
  transmitBitIndex = 7;
  transmitStartTime = NULL;
  transmitMediaStream = NULL;
  
  transmitHDLCTunneling = PFalse;
  receiveHDLCTunneling = PFalse;
  
  clients.DisallowDeleteObjects();
}

OpalH224Handler::~OpalH224Handler()
{
}

PBoolean OpalH224Handler::AddClient(OpalH224Client & client)
{
  if (client.GetClientID() == OpalH224Client::CMEClientID) {
    return PFalse; // No client may have CMEClientID
  }
	
  if (clients.GetObjectsIndex(&client) != P_MAX_INDEX) {
    return PFalse; // Only allow one instance of a client
  }
	
  clients.Append(&client);
  client.SetH224Handler(this);
  return PTrue;
}

PBoolean OpalH224Handler::RemoveClient(OpalH224Client & client)
{
  PBoolean result = clients.Remove(&client);
  if (result == PTrue) {
    client.SetH224Handler(NULL);
  }
  return result;
}

void OpalH224Handler::SetTransmitMediaFormat(const OpalMediaFormat & mediaFormat)
{
  PAssert(mediaFormat.GetMediaType() == "h224", "H.224 handler passed incorrect media format");
  transmitHDLCTunneling = mediaFormat.GetOptionBoolean("HDLC Tunneling");
}

void OpalH224Handler::SetReceiveMediaFormat(const OpalMediaFormat & mediaFormat)
{
  PAssert(mediaFormat.GetMediaType() == "h224", "H.224 handler passed incorrect media format");
  receiveHDLCTunneling = mediaFormat.GetOptionBoolean("HDLC Tunneling");
}

void OpalH224Handler::SetTransmitMediaStream(OpalH224MediaStream * mediaStream)
{
  PWaitAndSignal m(transmitMutex);
	
  transmitMediaStream = mediaStream;
	
  if (transmitMediaStream != NULL) {
    transmitFrame.SetPayloadType(transmitMediaStream->GetMediaFormat().GetPayloadType());
  }
}

void OpalH224Handler::StartTransmit()
{
  PWaitAndSignal m(transmitMutex);
	
  if (canTransmit == PTrue) {
    return;
  }
	
  canTransmit = PTrue;
  
  transmitBitIndex = 7;
  transmitStartTime = new PTime();
  
  SendClientList();
  SendExtraCapabilities();
}

void OpalH224Handler::StopTransmit()
{
  PWaitAndSignal m(transmitMutex);
  
  if (canTransmit == PFalse) {
    return;
  }
	
  delete transmitStartTime;
  transmitStartTime = NULL;
	
  canTransmit = PFalse;
}

PBoolean OpalH224Handler::SendClientList()
{
  PWaitAndSignal m(transmitMutex);
	
  if (canTransmit == PFalse) {
    return PFalse;
  }
  
  // If all clients are non-standard, 5 octets per clients + 3 octets header information
  H224_Frame h224Frame = H224_Frame(5*clients.GetSize() + 3);
	
  h224Frame.SetHighPriority(PTrue);
  h224Frame.SetDestinationTerminalAddress(Broadcast);
  h224Frame.SetSourceTerminalAddress(Broadcast);
	
  // CME frame
  h224Frame.SetClientID(OpalH224Client::CMEClientID);
	
  // Begin and end of sequence
  h224Frame.SetBS(PTrue);
  h224Frame.SetES(PTrue);
  h224Frame.SetC1(PFalse);
  h224Frame.SetC0(PFalse);
  h224Frame.SetSegmentNumber(0);
	
  BYTE *ptr = h224Frame.GetClientDataPtr();
	
  ptr[0] = OpalH224Handler::CMEClientListCode;
  ptr[1] = OpalH224Handler::CMEMessage;
  ptr[2] = (BYTE)clients.GetSize();
  
  PINDEX dataIndex = 3;
  for (PINDEX i = 0; i < clients.GetSize(); i++) {
    OpalH224Client & client = clients[i];
    
    BYTE clientID = client.GetClientID();
    
    if (client.HasExtraCapabilities()) {
      ptr[dataIndex] = (0x80 | clientID);
    } else {
      ptr[dataIndex] = (0x7f & clientID);
    }
    dataIndex++;
    
    if (clientID == OpalH224Client::ExtendedClientID) {
      ptr[dataIndex] = client.GetExtendedClientID();
      dataIndex++;
    
    } else if (clientID == OpalH224Client::NonStandardClientID) {
      
      ptr[dataIndex] = client.GetCountryCode();
      dataIndex++;
      ptr[dataIndex] = client.GetCountryCodeExtension();
      dataIndex++;
      
      WORD manufacturerCode = client.GetManufacturerCode();
      ptr[dataIndex] = (BYTE)(manufacturerCode >> 8);
      dataIndex++;
      ptr[dataIndex] = (BYTE) manufacturerCode;
      dataIndex++;
      
      ptr[dataIndex] = client.GetManufacturerClientID();
      dataIndex++;
    }
  }
  
  h224Frame.SetClientDataSize(dataIndex);
	
  TransmitFrame(h224Frame);
	
  return PTrue;
}

PBoolean OpalH224Handler::SendExtraCapabilities()
{
  for (PINDEX i = 0; i < clients.GetSize(); i++) {
    OpalH224Client & client = clients[i];
    client.SendExtraCapabilities();
  }
	
  return PTrue;
}

PBoolean OpalH224Handler::SendClientListCommand()
{
  PWaitAndSignal m(transmitMutex);
	
  if (canTransmit == PFalse) {
    return PFalse;
  }
	
  H224_Frame h224Frame = H224_Frame(2);
  h224Frame.SetHighPriority(PTrue);
  h224Frame.SetDestinationTerminalAddress(OpalH224Handler::Broadcast);
  h224Frame.SetSourceTerminalAddress(OpalH224Handler::Broadcast);
	
  // CME frame
  h224Frame.SetClientID(OpalH224Client::CMEClientID);
	
  // Begin and end of sequence
  h224Frame.SetBS(PTrue);
  h224Frame.SetES(PTrue);
  h224Frame.SetC1(PFalse);
  h224Frame.SetC0(PFalse);
  h224Frame.SetSegmentNumber(0);
	
  BYTE *ptr = h224Frame.GetClientDataPtr();
	
  ptr[0] = OpalH224Handler::CMEClientListCode;
  ptr[1] = OpalH224Handler::CMECommand;
	
  TransmitFrame(h224Frame);
	
  return PTrue;
}

PBoolean OpalH224Handler::SendExtraCapabilitiesCommand(const OpalH224Client & client)
{
  PWaitAndSignal m(transmitMutex);
	
  if (canTransmit == PFalse) {
    return PFalse;
  }
	
  if (clients.GetObjectsIndex(&client) == P_MAX_INDEX) {
    return PFalse; // only allow if the client is really registered
  }
	
  H224_Frame h224Frame = H224_Frame(8);
  h224Frame.SetHighPriority(PTrue);
  h224Frame.SetDestinationTerminalAddress(OpalH224Handler::Broadcast);
  h224Frame.SetSourceTerminalAddress(OpalH224Handler::Broadcast);
	
  // CME frame
  h224Frame.SetClientID(OpalH224Client::CMEClientID);
	
  // Begin and end of sequence
  h224Frame.SetBS(PTrue);
  h224Frame.SetES(PTrue);
  h224Frame.SetC1(PFalse);
  h224Frame.SetC0(PFalse);
  h224Frame.SetSegmentNumber(0);
	
  BYTE *ptr = h224Frame.GetClientDataPtr();
	
  ptr[0] = OpalH224Handler::CMEExtraCapabilitiesCode;
  ptr[1] = OpalH224Handler::CMECommand;

  PINDEX dataSize;
  
  BYTE extendedCapabilitiesFlag = client.HasExtraCapabilities() ? 0x80 : 0x00;
  BYTE clientID = client.GetClientID();
  ptr[2] = (extendedCapabilitiesFlag | (clientID & 0x7f));
  
  if (clientID < OpalH224Client::ExtendedClientID) {
    dataSize = 3;
  } else if (clientID == OpalH224Client::ExtendedClientID) {
    ptr[3] = client.GetExtendedClientID();
    dataSize = 4;
  } else {
    ptr[3] = client.GetCountryCode();
    ptr[4] = client.GetCountryCodeExtension();
	  
    WORD manufacturerCode = client.GetManufacturerCode();
    ptr[5] = (BYTE)(manufacturerCode >> 8);
    ptr[6] = (BYTE) manufacturerCode;
	  
    ptr[7] = client.GetManufacturerClientID();
    dataSize = 8;
  }
  h224Frame.SetClientDataSize(dataSize);
	
  TransmitFrame(h224Frame);
	
  return PTrue;
}

PBoolean OpalH224Handler::SendExtraCapabilitiesMessage(const OpalH224Client & client, 
                                                       BYTE *data, PINDEX length)
{	
  PWaitAndSignal m(transmitMutex);
	
  if (clients.GetObjectsIndex(&client) == P_MAX_INDEX) {
    return PFalse; // Only allow if the client is really registered
  }
	
  H224_Frame h224Frame = H224_Frame(length+3);
  h224Frame.SetHighPriority(PTrue);
  h224Frame.SetDestinationTerminalAddress(OpalH224Handler::Broadcast);
  h224Frame.SetSourceTerminalAddress(OpalH224Handler::Broadcast);
	
  // use clientID zero to indicate a CME frame
  h224Frame.SetClientID(OpalH224Client::CMEClientID);
	
  // Begin and end of sequence, rest is zero
  h224Frame.SetBS(PTrue);
  h224Frame.SetES(PTrue);
  h224Frame.SetC1(PFalse);
  h224Frame.SetC0(PFalse);
  h224Frame.SetSegmentNumber(0);
	
  BYTE *ptr = h224Frame.GetClientDataPtr();
	
  ptr[0] = CMEExtraCapabilitiesCode;
  ptr[1] = CMEMessage;
  
  PINDEX headerSize;
  BYTE clientID = client.GetClientID();
  BYTE extendedCapabilitiesFlag = client.HasExtraCapabilities() ? 0x80 : 0x00;
  
  ptr[2] = (extendedCapabilitiesFlag | (clientID & 0x7f));
  
  if (clientID < OpalH224Client::ExtendedClientID) {
    headerSize = 3;
  } else if (clientID == OpalH224Client::ExtendedClientID) {
    ptr[3] = client.GetExtendedClientID();
    headerSize = 4;
  } else {
    ptr[3] = client.GetCountryCode();
    ptr[4] = client.GetCountryCodeExtension();
	  
    WORD manufacturerCode = client.GetManufacturerCode();
    ptr[5] = (BYTE) (manufacturerCode >> 8);
    ptr[6] = (BYTE) manufacturerCode;
	  
    ptr[7] = client.GetManufacturerClientID();
    headerSize = 8;
  }
	
  h224Frame.SetClientDataSize(length+headerSize);
  memcpy(ptr+headerSize, data, length);
	
  TransmitFrame(h224Frame);
	
  return PTrue;	
}

PBoolean OpalH224Handler::TransmitClientFrame(const OpalH224Client & client, H224_Frame & frame)
{
  PWaitAndSignal m(transmitMutex);
  
  if (canTransmit == PFalse) {
    return PFalse;
  }
	
  if (clients.GetObjectsIndex(&client) == P_MAX_INDEX) {
    return PFalse; // Only allow if the client is really registered
  }
  
  TransmitFrame(frame);
	
  return PTrue;
}

PBoolean OpalH224Handler::OnReceivedFrame(H224_Frame & frame)
{
  if (frame.GetDestinationTerminalAddress() != OpalH224Handler::Broadcast) {
    // only broadcast frames are handled at the moment
    PTRACE(3, "H.224\tReceived frame with non-broadcast address");
    return PTrue;
  }
  BYTE clientID = frame.GetClientID();
	
  if (clientID == OpalH224Client::CMEClientID) {
    return OnReceivedCMEMessage(frame);
  }
	
  for (PINDEX i = 0; i < clients.GetSize(); i++) {
    OpalH224Client & client = clients[i];
    if (client.GetClientID() == clientID) {
      PBoolean found = PFalse;
      if (clientID < OpalH224Client::ExtendedClientID) {
        found = PTrue;
      } else if (clientID == OpalH224Client::ExtendedClientID) {
        if (client.GetExtendedClientID() == frame.GetExtendedClientID()) {
          found = PTrue;
        }
      } else {
        if (client.GetCountryCode() == frame.GetCountryCode() &&
           client.GetCountryCodeExtension() == frame.GetCountryCodeExtension() &&
           client.GetManufacturerCode() == frame.GetManufacturerCode() &&
           client.GetManufacturerClientID() == frame.GetManufacturerClientID()) {
          found = PTrue;
        }
      }
      if (found == PTrue) {
        client.OnReceivedMessage(frame);
        return PTrue;
      }
    }
  }
  
  // ignore if no client found
	
  return PTrue;
}

PBoolean OpalH224Handler::OnReceivedCMEMessage(H224_Frame & frame)
{
  BYTE *data = frame.GetClientDataPtr();
	
  if (data[0] == CMEClientListCode) {
	
    if (data[1] == CMEMessage) {
      return OnReceivedClientList(frame);
		
    } else if (data[1] == CMECommand) {
      return OnReceivedClientListCommand();
    }
	  
  } else if (data[0] == CMEExtraCapabilitiesCode) {
	  
    if (data[1] == CMEMessage) {
      return OnReceivedExtraCapabilities(frame);
		
    } else if (data[1] == CMECommand) {
      return OnReceivedExtraCapabilitiesCommand();
    }
  }
	
  // incorrect frames are simply ignored
  return PTrue;
}

PBoolean OpalH224Handler::OnReceivedClientList(H224_Frame & frame)
{
  // First, reset all clients
  for (PINDEX i = 0; i < clients.GetSize(); i++)
  {
    OpalH224Client & client = clients[i];
    client.SetRemoteClientAvailable(PFalse, PFalse);
  }
  
  BYTE *data = frame.GetClientDataPtr();
	
  BYTE numberOfClients = data[2];
	
  PINDEX dataIndex = 3;
	
  while (numberOfClients > 0) {
	  
    BYTE clientID = (data[dataIndex] & 0x7f);
    PBoolean hasExtraCapabilities = (data[dataIndex] & 0x80) != 0 ? PTrue: PFalse;
    dataIndex++;
    BYTE extendedClientID = 0x00;
    BYTE countryCode = CountryCodeEscape;
    BYTE countryCodeExtension = 0x00;
    WORD manufacturerCode = 0x0000;
    BYTE manufacturerClientID = 0x00;
    
    if (clientID == OpalH224Client::ExtendedClientID) {
      extendedClientID = data[dataIndex];
      dataIndex++;
    } else if (clientID == OpalH224Client::NonStandardClientID) {
      countryCode = data[dataIndex];
      dataIndex++;
      countryCodeExtension = data[dataIndex];
      dataIndex++;
      manufacturerCode = (((WORD)data[dataIndex] << 8) | (WORD)data[dataIndex+1]);
      dataIndex += 2;
      manufacturerClientID = data[dataIndex];
      dataIndex++;
    }
    
    for (PINDEX i = 0; i < clients.GetSize(); i++) {
      OpalH224Client & client = clients[i];
      PBoolean found = PFalse;
      if (client.GetClientID() == clientID) {
        if (clientID < OpalH224Client::ExtendedClientID) {
          found = PTrue;
        } else if (clientID == OpalH224Client::ExtendedClientID) {
          if (client.GetExtendedClientID() == extendedClientID) {
            found = PTrue;
          }
        } else {
          if (client.GetCountryCode() == countryCode &&
             client.GetCountryCodeExtension() == countryCodeExtension &&
             client.GetManufacturerCode() == manufacturerCode &&
             client.GetManufacturerClientID() == manufacturerClientID) {
            found = PTrue;
          }
        }
      }
      if (found == PTrue) {
        client.SetRemoteClientAvailable(PTrue, hasExtraCapabilities);
        break;
      }
    }
    numberOfClients--;
  }
  
	
  return PTrue;
}

PBoolean OpalH224Handler::OnReceivedClientListCommand()
{
  SendClientList();
  return PTrue;
}

PBoolean OpalH224Handler::OnReceivedExtraCapabilities(H224_Frame & frame)
{
  BYTE *data = frame.GetClientDataPtr();
	
  BYTE clientID = (data[2] & 0x7f);
  PINDEX dataIndex = 0;
  BYTE extendedClientID = 0x00;
  BYTE countryCode = CountryCodeEscape;
  BYTE countryCodeExtension = 0x00;
  WORD manufacturerCode = 0x0000;
  BYTE manufacturerClientID = 0x00;
  
  if (clientID < OpalH224Client::ExtendedClientID) {
    dataIndex = 3;
  } else if (clientID == OpalH224Client::ExtendedClientID) {
    extendedClientID = data[3];
    dataIndex = 4;
  } else if (clientID == OpalH224Client::NonStandardClientID) {
    countryCode = data[3];
    countryCodeExtension = data[4];
    manufacturerCode = (((WORD)data[5] << 8) | (WORD)data[6]);
    manufacturerClientID = data[7];
    dataIndex = 8;
  }
  
  for (PINDEX i = 0; i < clients.GetSize(); i++) {
    OpalH224Client & client = clients[i];
    PBoolean found = PFalse;
    if (client.GetClientID() == clientID) {
      if (clientID < OpalH224Client::ExtendedClientID) {
        found = PTrue;
      } else if (clientID == OpalH224Client::ExtendedClientID) {
        if (client.GetExtendedClientID() == extendedClientID) {
          found = PTrue;
        }
      } else {
        if (client.GetCountryCode() == countryCode &&
           client.GetCountryCodeExtension() == countryCodeExtension &&
           client.GetManufacturerCode() == manufacturerCode &&
           client.GetManufacturerClientID() == manufacturerClientID) {
          found = PTrue;
        }
      }
    }
    if (found) {
      PINDEX size = frame.GetClientDataSize() - dataIndex;
      client.SetRemoteClientAvailable(PTrue, PTrue);
      client.OnReceivedExtraCapabilities((data + dataIndex), size);
      return PTrue;
    }
  }
  
  // Simply ignore if no client is available for this clientID
	
  return PTrue;
}

PBoolean OpalH224Handler::OnReceivedExtraCapabilitiesCommand()
{
  SendExtraCapabilities();
  return PTrue;
}

PBoolean OpalH224Handler::HandleFrame(const RTP_DataFrame & dataFrame)
{
  if (receiveHDLCTunneling) {
    if (receiveFrame.DecodeHDLC(dataFrame.GetPayloadPtr(), dataFrame.GetPayloadSize())) {
      PBoolean result = OnReceivedFrame(receiveFrame);
      return result;
    } else {
      PTRACE(1, "H224\tDecoding of the frame failed");
      return PFalse;
    }
  } else {
    if (receiveFrame.DecodeAnnexQ(dataFrame.GetPayloadPtr(), dataFrame.GetPayloadSize())) {
      PBoolean result = OnReceivedFrame(receiveFrame);
      return result;
    } else {
      PTRACE(1, "H224\tDecoding of the frame failed");
      return PFalse;
    }
  }
}

void OpalH224Handler::TransmitFrame(H224_Frame & frame)
{
  PINDEX size;
  if (transmitHDLCTunneling) {
    size = frame.GetHDLCEncodedSize();
    transmitFrame.SetMinSize(size);
    if (!frame.EncodeHDLC(transmitFrame.GetPayloadPtr(), size, transmitBitIndex)) {
      PTRACE(1, "H224\tFailed to encode the frame");
      return;
    }
  } else {
    size = frame.GetAnnexQEncodedSize();
    transmitFrame.SetMinSize(size);
    if (!frame.EncodeAnnexQ(transmitFrame.GetPayloadPtr(), size)) {
      PTRACE(1, "H224\tFailed to encode the frame");
      return;
    }
  }
  
  // determining correct timestamp
  PTime currentTime = PTime();
  PTimeInterval timePassed = currentTime - *transmitStartTime;
  transmitFrame.SetTimestamp((DWORD)timePassed.GetMilliSeconds() * 8);
  
  transmitFrame.SetPayloadSize(size);
  transmitFrame.SetMarker(PTrue);
  
  if (transmitMediaStream != NULL) {
    transmitMediaStream->PushPacket(transmitFrame);
  }
}

////////////////////////////////////

OpalH224MediaStream::OpalH224MediaStream(OpalConnection & connection, 
                                         OpalH224Handler & handler,
                                         const OpalMediaFormat & mediaFormat,
                                         unsigned sessionID,
                                         PBoolean isSource)
: OpalMediaStream(connection, mediaFormat, sessionID, isSource),
  h224Handler(handler)
{
  if (isSource == PTrue) {
    h224Handler.SetTransmitMediaFormat(mediaFormat);
    h224Handler.SetTransmitMediaStream(this);
  } else {
    h224Handler.SetReceiveMediaFormat(mediaFormat);
  }
}

OpalH224MediaStream::~OpalH224MediaStream()
{
  Close();
}

void OpalH224MediaStream::OnStartMediaPatch()
{	
  h224Handler.StartTransmit();
  OpalMediaStream::OnStartMediaPatch();
}

PBoolean OpalH224MediaStream::Close()
{
  if (OpalMediaStream::Close() == PFalse) {
    return PFalse;
  }
	
  if (IsSource()) {
    h224Handler.StopTransmit();
    h224Handler.SetTransmitMediaStream(NULL);
  }
	
  return PTrue;
}

PBoolean OpalH224MediaStream::ReadPacket(RTP_DataFrame & /*packet*/)
{
  return PFalse;
}

PBoolean OpalH224MediaStream::WritePacket(RTP_DataFrame & packet)
{
  return h224Handler.HandleFrame(packet);
}

////////////////////////////////////

OpalH224Client::OpalH224Client()
{
  remoteClientAvailable = PFalse;
  remoteClientHasExtraCapabilities = PFalse;
  h224Handler = NULL;
}

OpalH224Client::~OpalH224Client()
{
}

PObject::Comparison OpalH224Client::Compare(const PObject & obj)
{
  if (!PIsDescendant(&obj, OpalH224Client)) {
    return LessThan;
  }
	
  const OpalH224Client & otherClient = (const OpalH224Client &) obj;
	
  BYTE clientID = GetClientID();
  BYTE otherClientID = otherClient.GetClientID();
	
  if (clientID < otherClientID) {
    return LessThan;
  } else if (clientID > otherClientID) {
    return GreaterThan;
  }
	
  if (clientID < ExtendedClientID) {
    return EqualTo;
  }
	
  if (clientID == ExtendedClientID) {
    BYTE extendedClientID = GetExtendedClientID();
    BYTE otherExtendedClientID = otherClient.GetExtendedClientID();
		
    if (extendedClientID < otherExtendedClientID) {
      return LessThan;
    } else if (extendedClientID > otherExtendedClientID) {
      return GreaterThan;
    } else {
      return EqualTo;
    }
  }
	
  // Non-standard client.
  // Compare country code, extended country code, manufacturer code, manufacturer client ID
  BYTE countryCode = GetCountryCode();
  BYTE otherCountryCode = otherClient.GetCountryCode();
  if (countryCode < otherCountryCode) {
    return LessThan;
  } else if (countryCode > otherCountryCode) {
    return GreaterThan;
  }
	
  BYTE countryCodeExtension = GetCountryCodeExtension();
  BYTE otherCountryCodeExtension = otherClient.GetCountryCodeExtension();
  if (countryCodeExtension < otherCountryCodeExtension) {
    return LessThan;
  } else if (countryCodeExtension > otherCountryCodeExtension) {
    return GreaterThan;
  }
	
  WORD manufacturerCode = GetManufacturerCode();
  WORD otherManufacturerCode = otherClient.GetManufacturerCode();
  if (manufacturerCode < otherManufacturerCode) {
    return LessThan;
  } else if (manufacturerCode > otherManufacturerCode) {
    return GreaterThan;
  }
	
  BYTE manufacturerClientID = GetManufacturerClientID();
  BYTE otherManufacturerClientID = otherClient.GetManufacturerClientID();
  
  if (manufacturerClientID < otherManufacturerClientID) {
    return LessThan;
  } else if (manufacturerClientID > otherManufacturerClientID) {
    return GreaterThan;
  }
	
  return EqualTo;
}

void OpalH224Client::SetRemoteClientAvailable(PBoolean available, PBoolean hasExtraCapabilities)
{
  remoteClientAvailable = available;
  remoteClientHasExtraCapabilities = hasExtraCapabilities;
}

#endif // OPAL_HAS_H224
