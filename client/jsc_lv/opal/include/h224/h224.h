/*
 * h224.h
 *
 * H.224 PDU implementation for the OpenH323 Project.
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
 * $Revision: 21862 $
 * $Author: csoutheren $
 * $Date: 2008-12-23 03:24:53 +0000 (Tue, 23 Dec 2008) $
 */

#ifndef OPAL_H224_H224_H
#define OPAL_H224_H224_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal/buildopts.h>

#include <opal/mediatype.h>
#include <opal/mediafmt.h>
#include <h224/q922.h>

#define H224_HEADER_SIZE 6

///////////////////////////////////////////////////////////////////////////////
//
//  declare a media type for H.224
//

class OpalH224MediaType : public OpalRTPAVPMediaType 
{
  public:
    OpalH224MediaType();
  
    static const OpalMediaType & MediaType();

#if OPAL_SIP
    SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress);
#endif
};

///////////////////////////////////////////////////////////////////////////////
//
// H.224 Media Format
//

class OpalH224MediaFormat : public OpalMediaFormat
{
  PCLASSINFO(OpalH224MediaFormat, OpalMediaFormat);
  
  public:
    OpalH224MediaFormat(
      const char * fullName,                      ///<  Full name of media format
      RTP_DataFrame::PayloadTypes rtpPayloadType  ///<  RTP payload type code
    );
    virtual PObject * Clone() const;
    virtual PBoolean IsValidForProtocol(const PString & protocol) const;
};

extern const OpalMediaFormat & GetOpalH224_H323AnnexQ();
extern const OpalMediaFormat & GetOpalH224_HDLCTunneling();


#define OpalH224AnnexQ      GetOpalH224_H323AnnexQ()
#define OpalH224Tunnelled   GetOpalH224_HDLCTunneling()

///////////////////////////////////////////////////////////////////////////////

class OpalH224Client; 

class H224_Frame : public Q922_Frame
{
  PCLASSINFO(H224_Frame, Q922_Frame);
	
public:
	
  H224_Frame(PINDEX clientDataSize = 254);
  H224_Frame(const OpalH224Client & h224Client, PINDEX clientDataSize = 254);
  ~H224_Frame();
	
  PBoolean IsHighPriority() const { return (GetLowOrderAddressOctet() == 0x71); }
  void SetHighPriority(PBoolean flag);
	
  WORD GetDestinationTerminalAddress() const;
  void SetDestinationTerminalAddress(WORD destination);
	
  WORD GetSourceTerminalAddress() const;
  void SetSourceTerminalAddress(WORD source);
	
  /**Convenience function to set the H.224 header values */
  void SetClient(const OpalH224Client & h224Client);
  
  BYTE GetClientID() const;
  void SetClientID(BYTE clientID);
  
  /**Returns 0 in case clientID isn't set to ExtendedClientID */
  BYTE GetExtendedClientID() const;
  /**Does nothing in case clientID isn't set to ExtendedClientID */
  void SetExtendedClientID(BYTE extendedClientID);
  
  /**Returns 0 in case clientID isn't set to NonStandardClientID */
  BYTE GetCountryCode() const;
  BYTE GetCountryCodeExtension() const;
  WORD GetManufacturerCode() const;
  BYTE GetManufacturerClientID() const;
  
  /**Does nothing in case clientID isn't set to NonStandardClientID */
  void SetNonStandardClientInformation(BYTE countryCode,
                                       BYTE countryCodeExtension,
                                       WORD manufacturerCode,
                                       BYTE manufacturerClientID);
	
  /**Note: The following methods depend on the value of clientID as to where put the value.
    Always set clientID first before altering these values */
  PBoolean GetBS() const;
  void SetBS(PBoolean bs);
	
  PBoolean GetES() const;
  void SetES(PBoolean es);
	
  PBoolean GetC1() const;
  void SetC1(PBoolean c1);
	
  PBoolean GetC0() const;
  void SetC0(PBoolean c0);
	
  BYTE GetSegmentNumber() const;
  void SetSegmentNumber(BYTE segmentNumber);
	
  BYTE *GetClientDataPtr() const;
	
  PINDEX GetClientDataSize() const;
  void SetClientDataSize(PINDEX size);
	
  PBoolean DecodeAnnexQ(const BYTE *data, PINDEX size);
  PBoolean DecodeHDLC(const BYTE *data, PINDEX size);
  
private:
  PINDEX GetHeaderSize() const;
};

#endif // OPAL_H224_H224_H

