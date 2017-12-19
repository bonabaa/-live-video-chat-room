/*
 * sdp.h
 *
 * Session Description Protocol
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23966 $
 * $Author: rjongbloed $
 * $Date: 2010-01-22 08:22:57 +0000 (Fri, 22 Jan 2010) $
 */

#ifndef OPAL_SIP_SDP_H
#define OPAL_SIP_SDP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#if OPAL_SIP

#include <opal/transports.h>
#include <opal/mediatype.h>
#include <opal/mediafmt.h>
#include <rtp/rtp.h>

/////////////////////////////////////////////////////////

class SDPBandwidth : public std::map<PString, unsigned>
{
  public:
    unsigned & operator[](const PString & type);
    unsigned operator[](const PString & type) const;
    friend ostream & operator<<(ostream & out, const SDPBandwidth & bw);
    bool Parse(const PString & param);
    void SetMin(const PString & type, unsigned value);
};

/////////////////////////////////////////////////////////

class SDPMediaDescription;

class SDPMediaFormat : public PObject
{
  PCLASSINFO(SDPMediaFormat, PObject);
  public:
    SDPMediaFormat(
      SDPMediaDescription & parent,
      RTP_DataFrame::PayloadTypes payloadType,
      const char * name = NULL
    );

    SDPMediaFormat(
      SDPMediaDescription & parent,
      const OpalMediaFormat & mediaFormat
    );

    virtual void PrintOn(ostream & str) const;

    RTP_DataFrame::PayloadTypes GetPayloadType() const { return payloadType; }

    PString GetEncodingName() const         { return encodingName; }
    void SetEncodingName(const PString & v) { encodingName = v; }

    void SetFMTP(const PString & _fmtp); 
    PString GetFMTP() const;

    unsigned GetClockRate(void)                        { return clockRate ; }
    void SetClockRate(unsigned  v)                     { clockRate = v; }

    void SetParameters(const PString & v) { parameters = v; }

    void SetPacketTime(const PString & optionName, unsigned ptime);

    const OpalMediaFormat & GetMediaFormat() const;
    OpalMediaFormat & GetWritableMediaFormat();

	/**Return all registered media formats that match information 
	 */
    OpalMediaFormatList GetMediaFormats() const;

    bool PreEncode();
    bool PostDecode(unsigned bandwidth);

  protected:
    void InitialiseMediaFormat(OpalMediaFormat & mediaFormat) const;

    OpalMediaFormat mediaFormat;

    SDPMediaDescription & m_parent;
    RTP_DataFrame::PayloadTypes payloadType;
    unsigned clockRate;
    PString encodingName;
    PString parameters;
    PString m_fmtp;
};

PLIST(SDPMediaFormatList, SDPMediaFormat);

/////////////////////////////////////////////////////////

class SDPMediaDescription : public PObject
{
  PCLASSINFO(SDPMediaDescription, PObject);
  public:
    // The following enum is arranged so it can be used as a bit mask,
    // e.g. GetDirection()&SendOnly indicates send is available
    enum Direction {
      Undefined = -1,
      Inactive,
      RecvOnly,
      SendOnly,
      SendRecv
    };

    SDPMediaDescription(
      const OpalTransportAddress & address,
      const OpalMediaType & mediaType
    );
    PBoolean SetTransportAddressCtrl(const OpalTransportAddress &t);
    virtual bool PreEncode();
    virtual void Encode(const OpalTransportAddress & commonAddr, ostream & str) const;
    virtual bool PrintOn(ostream & strm, const PString & str) const;

    virtual bool Decode(const PStringArray & tokens);
    virtual bool Decode(char key, const PString & value);
    virtual bool PostDecode();

    // return the string used within SDP to identify this media type
    virtual PString GetSDPMediaType() const = 0;

    // return the string used within SDP to identify the transport used by this media
    virtual PCaselessString GetSDPTransportType() const = 0;

    virtual const SDPMediaFormatList & GetSDPMediaFormats() const
      { return formats; }

    virtual OpalMediaFormatList GetMediaFormats() const;

    virtual void AddSDPMediaFormat(SDPMediaFormat * sdpMediaFormat);

    virtual void AddMediaFormat(const OpalMediaFormat & mediaFormat);
    virtual void AddMediaFormats(const OpalMediaFormatList & mediaFormats, const OpalMediaType & mediaType);

    virtual void SetAttribute(const PString & attr, const PString & value);

    virtual void SetDirection(const Direction & d) { direction = d; }
    virtual Direction GetDirection() const { return transportAddress.IsEmpty() ? Inactive : direction; }

    virtual const OpalTransportAddress & GetTransportAddress() const { return transportAddress; }
    virtual const OpalTransportAddress & GetTransportAddressCtrl() const { return m_localControltransport; }
    virtual PBoolean SetTransportAddress(const OpalTransportAddress &t);

    virtual WORD GetPort() const { return port; }

    virtual OpalMediaType GetMediaType() const { return mediaType; }

    virtual unsigned GetBandwidth(const PString & type) const { return bandwidth[type]; }
    virtual void SetBandwidth(const PString & type, unsigned value) { bandwidth[type] = value; }

    virtual const SDPBandwidth & GetBandwidth() const { return bandwidth; }

    virtual void RemoveSDPMediaFormat(const SDPMediaFormat & sdpMediaFormat);

    virtual void CreateSDPMediaFormats(const PStringArray & tokens);
    virtual SDPMediaFormat * CreateSDPMediaFormat(const PString & portString) = 0;

    virtual PString GetSDPPortList() const = 0;

    virtual void ProcessMediaOptions(SDPMediaFormat & sdpFormat, const OpalMediaFormat & mediaFormat);

  protected:
    virtual SDPMediaFormat * FindFormat(PString & str) const;
    virtual void SetPacketTime(const PString & optionName, const PString & value);

    OpalTransportAddress transportAddress;
    Direction direction;
    WORD port;
    WORD portCount;
    OpalMediaType mediaType;

    SDPMediaFormatList formats;
    SDPBandwidth       bandwidth;

    OpalTransportAddress m_localControltransport;
};

PARRAY(SDPMediaDescriptionArray, SDPMediaDescription);

/////////////////////////////////////////////////////////
//
//  SDP media description for media classes using RTP/AVP transport (audio and video)
//

class SDPRTPAVPMediaDescription : public SDPMediaDescription
{
  PCLASSINFO(SDPRTPAVPMediaDescription, SDPMediaDescription);
  public:
    SDPRTPAVPMediaDescription(const OpalTransportAddress & address, const OpalMediaType & mediaType);
    virtual PCaselessString GetSDPTransportType() const;
    virtual SDPMediaFormat * CreateSDPMediaFormat(const PString & portString);
    virtual PString GetSDPPortList() const;
    virtual bool PrintOn(ostream & str, const PString & connectString) const;
    void SetAttribute(const PString & attr, const PString & value);
};

/////////////////////////////////////////////////////////
//
//  SDP media description for audio media
//

class SDPAudioMediaDescription : public SDPRTPAVPMediaDescription
{
  PCLASSINFO(SDPAudioMediaDescription, SDPRTPAVPMediaDescription);
  public:
    SDPAudioMediaDescription(const OpalTransportAddress & address);
    virtual bool PreEncode();
    virtual PString GetSDPMediaType() const;
    virtual bool PrintOn(ostream & str, const PString & connectString) const;
    void SetAttribute(const PString & attr, const PString & value);
};

/////////////////////////////////////////////////////////
//
//  SDP media description for video media
//

class SDPVideoMediaDescription : public SDPRTPAVPMediaDescription
{
  PCLASSINFO(SDPVideoMediaDescription, SDPRTPAVPMediaDescription);
  public:
    SDPVideoMediaDescription(const OpalTransportAddress & address);
    virtual PString GetSDPMediaType() const;
    virtual bool PreEncode();
    virtual bool PrintOn(ostream & str, const PString & connectString) const;
    void SetAttribute(const PString & attr, const PString & value);
};

/////////////////////////////////////////////////////////
//
//  SDP media description for application media
//

class SDPApplicationMediaDescription : public SDPMediaDescription
{
  PCLASSINFO(SDPApplicationMediaDescription, SDPMediaDescription);
  public:
    SDPApplicationMediaDescription(const OpalTransportAddress & address);
    virtual PCaselessString GetSDPTransportType() const;
    virtual SDPMediaFormat * CreateSDPMediaFormat(const PString & portString);
    virtual PString GetSDPMediaType() const;
    virtual PString GetSDPPortList() const;
};

/////////////////////////////////////////////////////////

class SDPSessionDescription : public PObject
{
  PCLASSINFO(SDPSessionDescription, PObject);
  public:
    SDPSessionDescription(
      time_t sessionId,
      unsigned version,
      const OpalTransportAddress & address, const PString& strUser
    );

    void PrintOn(ostream & strm) const;
    PString Encode() const;
    PBoolean Decode(const PString & str);

    void SetSessionName(const PString & v) { sessionName = v; }
    PString GetSessionName() const         { return sessionName; }

    void SetICEInfo(const PString & v) ;
    PString GetICEInfo() const        ;
    void SetUserName(const PString & v)    { ownerUsername = v; }
    PString GetUserName() const            { return ownerUsername; }

    const SDPMediaDescriptionArray & GetMediaDescriptions() const { return mediaDescriptions; }

    SDPMediaDescription * GetMediaDescriptionByType(const OpalMediaType & rtpMediaType) const;
    SDPMediaDescription * GetMediaDescriptionByIndex(PINDEX i) const;
    void AddMediaDescription(SDPMediaDescription * md) { mediaDescriptions.Append(md); }
    
    void SetDirection(const SDPMediaDescription::Direction & d) { direction = d; }
    SDPMediaDescription::Direction GetDirection(unsigned) const;
    bool IsHold() const;

    const OpalTransportAddress & GetDefaultConnectAddress() const { return defaultConnectAddress; }
    void SetDefaultConnectAddress(
      const OpalTransportAddress & address
    );
	
    time_t GetOwnerSessionId() const { return ownerSessionId; }
    void SetOwnerSessionId(time_t value) { ownerSessionId = value; }

    PINDEX GetOwnerVersion() const { return ownerVersion; }
    void SetOwnerVersion(PINDEX value) { ownerVersion = value; }

    OpalTransportAddress GetOwnerAddress() const { return ownerAddress; }
    void SetOwnerAddress(OpalTransportAddress addr) { ownerAddress = addr; }

    unsigned GetBandwidth(const PString & type) const { return bandwidth[type]; }
    void SetBandwidth(const PString & type, unsigned value) { bandwidth[type] = value; }

    OpalMediaFormatList GetMediaFormats() const;

    static const PString & ConferenceTotalBandwidthType();
    static const PString & ApplicationSpecificBandwidthType();
    static const PString & TransportIndependentBandwidthType(); // RFC3890

  protected:
    void ParseOwner(const PString & str);

    SDPMediaDescriptionArray mediaDescriptions;
    SDPMediaDescription::Direction direction;

    PINDEX protocolVersion;
    PString sessionName;
    PString ICEInfo;

    PString ownerUsername;
    time_t ownerSessionId;
    unsigned ownerVersion;
    OpalTransportAddress ownerAddress;
    OpalTransportAddress defaultConnectAddress;

    SDPBandwidth bandwidth;	
};

/////////////////////////////////////////////////////////


#endif // OPAL_SIP

#endif // OPAL_SIP_SDP_H


// End of File ///////////////////////////////////////////////////////////////
