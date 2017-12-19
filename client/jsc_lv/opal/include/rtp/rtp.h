/*
 * rtp.h
 *
 * RTP protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2001 Equivalence Pty. Ltd.
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
 * $Revision: 23955 $
 * $Author: rjongbloed $
 * $Date: 2010-01-21 02:40:15 +0000 (Thu, 21 Jan 2010) $
 */

#ifndef OPAL_RTP_RTP_H
#define OPAL_RTP_RTP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <ptlib/sockets.h>
#include <ptlib/safecoll.h>
#include <udt.h>
#include <CCC.h>


class SIPConnection;
class RTP_JitterBuffer;
class PNatMethod;
class OpalSecurityMode;
#include <map>
class PByteBlock;
//#define _ORDER_RTP_PACKET
///////////////////////////////////////////////////////////////////////////////
// 
// class to hold the QoS definitions for an RTP channel

class RTP_QOS : public PObject
{
  PCLASSINFO(RTP_QOS,PObject);
  public:
    PQoS dataQoS;
    PQoS ctrlQoS;
};

///////////////////////////////////////////////////////////////////////////////
// Real Time Protocol - IETF RFC1889 and RFC1890

/**An RTP data frame encapsulation.
  */
class RTP_DataFrame : public PBYTEArray
{
  PCLASSINFO(RTP_DataFrame, PBYTEArray);

  public:
    RTP_DataFrame(PINDEX payloadSize = 0, PINDEX bufferSize = 0);
    RTP_DataFrame(const BYTE * data, PINDEX len, PBoolean dynamic = PTrue);

    enum {
      ProtocolVersion = 2,
      MinHeaderSize = 12,
      // Max safe MTU size (576 bytes as per RFC879) minus IP, UDP an RTP headers
      MaxMtuPayloadSize = (576-20-16-12)
    };

    enum PayloadTypes {
      PCMU,         // G.711 u-Law
      FS1016,       // Federal Standard 1016 CELP
      G721,         // ADPCM - Subsumed by G.726
      G726 = G721,
      GSM,          // GSM 06.10
      G7231,        // G.723.1 at 6.3kbps or 5.3 kbps
      DVI4_8k,      // DVI4 at 8kHz sample rate
      DVI4_16k,     // DVI4 at 16kHz sample rate
      LPC,          // LPC-10 Linear Predictive CELP
      PCMA,         // G.711 A-Law
      G722,         // G.722
      L16_Stereo,   // 16 bit linear PCM
      L16_Mono,     // 16 bit linear PCM
      G723,         // G.723
      CN,           // Confort Noise
      MPA,          // MPEG1 or MPEG2 audio
      G728,         // G.728 16kbps CELP
      DVI4_11k,     // DVI4 at 11kHz sample rate
      DVI4_22k,     // DVI4 at 22kHz sample rate
      G729,         // G.729 8kbps
      Cisco_CN,     // Cisco systems comfort noise (unofficial)

      CelB = 25,    // Sun Systems Cell-B video
      JPEG,         // Motion JPEG
      H261 = 31,    // H.261
      MPV,          // MPEG1 or MPEG2 video
      MP2T,         // MPEG2 transport system
      H263,         // H.263

      LastKnownPayloadType,

      DynamicBase = 96,
      MaxPayloadType = 127,
      IllegalPayloadType
    };

    unsigned GetVersion() const { return (theArray[0]>>6)&3; }

    PBoolean GetExtension() const   { return (theArray[0]&0x10) != 0; }
    void SetExtension(PBoolean ext);

    PBoolean GetMarker() const { return (theArray[1]&0x80) != 0; }
    void SetMarker(PBoolean m);

    bool GetPadding() const { return (theArray[0]&0x20) != 0; }
    void SetPadding(bool v)  { if (v) theArray[0] |= 0x20; else theArray[0] &= 0xdf; }

    unsigned GetPaddingSize() const;

    PayloadTypes GetPayloadType() const { return (PayloadTypes)(theArray[1]&0x7f); }
    void         SetPayloadType(PayloadTypes t);

    WORD GetSequenceNumber() const { return *(PUInt16b *)&theArray[2]; }
    void SetSequenceNumber(WORD n) { *(PUInt16b *)&theArray[2] = n; }

    DWORD GetTimestamp() const  { return *(PUInt32b *)&theArray[4]; }
    void  SetTimestamp(DWORD t) { *(PUInt32b *)&theArray[4] = t; }

    DWORD GetSyncSource() const  { return *(PUInt32b *)&theArray[8]; }
    void  SetSyncSource(DWORD s) { *(PUInt32b *)&theArray[8] = s; }

    PINDEX GetContribSrcCount() const { return theArray[0]&0xf; }
    DWORD  GetContribSource(PINDEX idx) const;
    void   SetContribSource(PINDEX idx, DWORD src);

    PINDEX GetHeaderSize() const;

    int GetExtensionType() const; // -1 is no extension
    void   SetExtensionType(int type);
    PINDEX GetExtensionSizeDWORDs() const;      // get the number of 32 bit words in the extension (excluding the header).
    bool   SetExtensionSizeDWORDs(PINDEX sz);   // set the number of 32 bit words in the extension (excluding the header)
    BYTE * GetExtensionPtr() const;

    PINDEX GetPayloadSize() const { return payloadSize - GetPaddingSize(); }
    PBoolean   SetPayloadSize(PINDEX sz);
    BYTE * GetPayloadPtr()     const { return (BYTE *)(theArray+GetHeaderSize()); }

    virtual void PrintOn(ostream & strm) const;

  protected:
    PINDEX payloadSize;

#if PTRACING
    friend ostream & operator<<(ostream & o, PayloadTypes t);
#endif
};

PLIST(RTP_DataFrameList, RTP_DataFrame);


/**An RTP control frame encapsulation.
  */
class RTP_ControlFrame : public PBYTEArray
{
  PCLASSINFO(RTP_ControlFrame, PBYTEArray);

  public:
    RTP_ControlFrame(PINDEX compoundSize = 2048);

    unsigned GetVersion() const { return (BYTE)theArray[compoundOffset]>>6; }

    unsigned GetCount() const { return (BYTE)theArray[compoundOffset]&0x1f; }
    void     SetCount(unsigned count);

    enum PayloadTypes {
      e_IntraFrameRequest = 192,
      e_SenderReport = 200,
      e_ReceiverReport,
      e_SourceDescription,
      e_Goodbye,
      e_ApplDefined
    };

    unsigned GetPayloadType() const { return (BYTE)theArray[compoundOffset+1]; }
    void     SetPayloadType(unsigned t);

    PINDEX GetPayloadSize() const { return 4*(*(PUInt16b *)&theArray[compoundOffset+2]); }
    void   SetPayloadSize(PINDEX sz);

    BYTE * GetPayloadPtr() const;

    PBoolean ReadNextPacket();
    PBoolean StartNewPacket();
    void EndPacket();

    PINDEX GetCompoundSize() const;

    void Reset(PINDEX size);

#pragma pack(1)
    struct ReceiverReport {
      PUInt32b ssrc;      /* data source being reported */
      BYTE fraction;      /* fraction lost since last SR/RR */
      BYTE lost[3];	  /* cumulative number of packets lost (signed!) */
      PUInt32b last_seq;  /* extended last sequence number received */
      PUInt32b jitter;    /* interarrival jitter */
      PUInt32b lsr;       /* last SR packet from this source */
      PUInt32b dlsr;      /* delay since last SR packet */

      unsigned GetLostPackets() const { return (lost[0]<<16U)+(lost[1]<<8U)+lost[2]; }
      void SetLostPackets(unsigned lost);
    };

    struct SenderReport {
      PUInt32b ntp_sec;   /* NTP timestamp */
      PUInt32b ntp_frac;
      PUInt32b rtp_ts;    /* RTP timestamp */
      PUInt32b psent;     /* packets sent */
      PUInt32b osent;     /* octets sent */ 
    };

    enum DescriptionTypes {
      e_END,
      e_CNAME,
      e_NAME,
      e_EMAIL,
      e_PHONE,
      e_LOC,
      e_TOOL,
      e_NOTE,
      e_PRIV,
      NumDescriptionTypes
    };

    struct SourceDescription {
      PUInt32b src;       /* first SSRC/CSRC */
      struct Item {
        BYTE type;        /* type of SDES item (enum DescriptionTypes) */
        BYTE length;      /* length of SDES item (in octets) */
        char data[1];     /* text, not zero-terminated */

        /* WARNING, SourceDescription may not be big enough to contain length and data, for 
           instance, when type == RTP_ControlFrame::e_END.
           Be careful whan calling the following function of it may read to over to 
           memory allocated*/
        unsigned int GetLengthTotal() const {return (unsigned int)(length + 2);} 
        const Item * GetNextItem() const { return (const Item *)((char *)this + length + 2); }
        Item * GetNextItem() { return (Item *)((char *)this + length + 2); }
      } item[1];          /* list of SDES items */
    };

    void StartSourceDescription(
      DWORD src   ///<  SSRC/CSRC identifier
    );

    void AddSourceDescriptionItem(
      unsigned type,            ///<  Description type
      const PString & data      ///<  Data for description
    );
#pragma pack()

  protected:
    PINDEX compoundOffset;
    PINDEX payloadSize;
};


class RTP_Session;

///////////////////////////////////////////////////////////////////////////////

#if OPAL_STATISTICS

/**This class carries statistics on the media stream.
  */
class OpalMediaStatistics : public PObject
{
    PCLASSINFO(OpalMediaStatistics, PObject);
  public:
    OpalMediaStatistics();

    // General info (typicallly from RTP)
    PUInt64  m_totalBytes;
    unsigned m_totalPackets;
    unsigned m_packetsLost;
    unsigned m_packetsOutOfOrder;
    unsigned m_packetsTooLate;
    unsigned m_packetOverruns;
    unsigned m_minimumPacketTime;
    unsigned m_averagePacketTime;
    unsigned m_maximumPacketTime;

    // Audio
    unsigned m_averageJitter;
    unsigned m_maximumJitter;

    // Video
    unsigned m_totalFrames;
    unsigned m_keyFrames;
    unsigned m_nFramesPerSec;
    unsigned m_nBytesPerSec;

    // Fax
#if OPAL_FAX
    struct Fax {
      Fax();

      int  m_result;      // -2=not started, -1=progress, 0=success, >0=ended with error
      int  m_bitRate;     // e.g. 14400, 9600
      int  m_compression; // 0=N/A, 1=T.4 1d, 2=T.4 2d, 3=T.6
      bool m_errorCorrection;
      int  m_txPages;
      int  m_rxPages;
      int  m_totalPages;
      int  m_imageSize;   // In bytes
      int  m_resolutionX; // Pixels per inch
      int  m_resolutionY; // Pixels per inch
      int  m_pageWidth;
      int  m_pageHeight;
      int  m_badRows;     // Total number of bad rows
      int  m_mostBadRows; // Longest run of bad rows
      int  m_errorCorrectionRetries;

      PString m_errorText;
    } m_fax;
#endif
};

#endif


/**This class is the base for user data that may be attached to the RTP_session
   allowing callbacks for statistics and progress monitoring to be passed to an
   arbitrary object that an RTP consumer may require.
  */
class RTP_UserData : public PObject
{
  PCLASSINFO(RTP_UserData, PObject);

  public:
    /**Callback from the RTP session for transmit statistics monitoring.
       This is called every RTP_Session::txStatisticsInterval packets on the
       transmitter indicating that the statistics have been updated.

       The default behaviour does nothing.
      */
    virtual void OnTxStatistics(
      const RTP_Session & session   ///<  Session with statistics
    ) const;

    /**Callback from the RTP session for receive statistics monitoring.
       This is called every RTP_Session::receiverReportInterval packets on the
       receiver indicating that the statistics have been updated.

       The default behaviour does nothing.
      */
    virtual void OnRxStatistics(
      const RTP_Session & session   ///<  Session with statistics
    ) const;

#if OPAL_VIDEO
    /**Callback from the RTP session when an intra frame request control
       packet is sent.

       The default behaviour does nothing.
      */
    virtual void OnTxIntraFrameRequest(
      const RTP_Session & session   ///<  Session with statistics
    ) const;

    /**Callback from the RTP session when an intra frame request control
       packet is received.

       The default behaviour does nothing.
      */
    virtual void OnRxIntraFrameRequest(
      const RTP_Session & session   ///<  Session with statistics
    ) const;
#endif

    /**Callback from the RTP session when RTP session is failing due to the remote being unavailable
       The default behaviour does nothing.
      */
    virtual void SessionFailing(
      RTP_Session & session   ///<  Session with statistics
    );
};

class RTP_Encoding;


/**This class is for encpsulating the IETF Real Time Protocol interface.
 */
class RTP_Session : public PObject
{
  PCLASSINFO(RTP_Session, PObject);

  public:
  /**@name Construction */
  //@{
    struct Params {
      Params()
        : id(0)
        , userData(NULL)
        , autoDelete(true)
        , isAudio(false)
        , remoteIsNAT(false)
      { }

      PString             encoding;    ///<  identifies initial RTP encoding (RTP/AVP, UDPTL etc)
      unsigned            id;          ///<  Session ID for RTP channel
      RTP_UserData      * userData;    ///<  Optional data for session.
      bool                autoDelete;  ///<  Delete optional data with session.
      bool                isAudio;     ///<  is audio RTP data
      bool                remoteIsNAT; ///<  Remote is behid NAT
    };

    /**Create a new RTP session.
     */
    RTP_Session(
      const Params & options ///< Parameters to construct with session.
    );

    /**Delete a session.
       This deletes the userData field if autoDeleteUserData is PTrue.
     */
    ~RTP_Session();
  //@}

  /**@name Operations */
  //@{
    /**Sets the size of the jitter buffer to be used by this RTP session.
       A session defaults to not having any jitter buffer enabled for reading
       and the ReadBufferedData() function simply calls ReadData().

       If either jitter delay parameter is zero, it destroys the jitter buffer
       attached to this RTP session.
      */
    void SetJitterBufferSize(
      unsigned minJitterDelay, ///<  Minimum jitter buffer delay in RTP timestamp units
      unsigned maxJitterDelay, ///<  Maximum jitter buffer delay in RTP timestamp units
      unsigned timeUnits = 0,  ///<  Time Units, zero uses default
      PINDEX packetSize = 2048 ///<  Receive RTP packet size
    );

    /**Get current size of the jitter buffer.
       This returns the currently used jitter buffer delay in RTP timestamp
       units. It will be some value between the minimum and maximum set in
       the SetJitterBufferSize() function.
      */
    unsigned GetJitterBufferSize() const;
    
    /**Get current time units of the jitter buffer.
     */
    unsigned GetJitterTimeUnits() const { return m_timeUnits; }

    /**Modifies the QOS specifications for this RTP session*/
    virtual PBoolean ModifyQOS(RTP_QOS * )
    { return PFalse; }

    /**Read a data frame from the RTP channel.
       This function will conditionally read data from the jitter buffer or
       directly if there is no jitter buffer enabled. An application should
       generally use this in preference to directly calling ReadData().
      */
    virtual PBoolean ReadBufferedData(
      RTP_DataFrame & frame   ///<  Frame read from the RTP session
    );

    /**Read a data frame from the RTP channel.
       Any control frames received are dispatched to callbacks and are not
       returned by this function. It will block until a data frame is
       available or an error occurs.
      */
    virtual PBoolean ReadData(
      RTP_DataFrame & frame,  ///<  Frame read from the RTP session
      PBoolean loop               ///<  If PTrue, loop as long as data is available, if PFalse, only process once
    ) = 0;

    /**Write a data frame from the RTP channel.
      */
    virtual PBoolean WriteData(
      RTP_DataFrame & frame   ///<  Frame to write to the RTP session
    ) = 0;

    /** Write data frame to the RTP channel outside the normal stream of media
      * Used for RFC2833 packets
      */
    virtual PBoolean WriteOOBData(
      RTP_DataFrame & frame,
      bool rewriteTimeStamp = true
    );

    /**Write a control frame from the RTP channel.
      */
    virtual PBoolean WriteControl(
      RTP_ControlFrame & frame    ///<  Frame to write to the RTP session
    ) = 0;

    /**Write the RTCP reports.
      */
    virtual PBoolean SendReport();

    /**Close down the RTP session.
      */
    virtual bool Close(
      PBoolean reading    ///<  Closing the read side of the session
    ) = 0;

   /**Reopens an existing session in the given direction.
      */
    virtual void Reopen(
      PBoolean isReading
    ) = 0;

    /**Get the local host name as used in SDES packes.
      */
    virtual PString GetLocalHostName() = 0;

#if OPAL_STATISTICS
    virtual void GetStatistics(OpalMediaStatistics & statistics, bool receiver) const;
#endif
  //@}

  /**@name Call back functions */
  //@{
    enum SendReceiveStatus {
      e_ProcessPacket,
      e_IgnorePacket,
      e_AbortTransport
    };
    virtual SendReceiveStatus OnSendData(RTP_DataFrame & frame);
    virtual SendReceiveStatus Internal_OnSendData(RTP_DataFrame & frame);

    virtual SendReceiveStatus OnSendControl(RTP_ControlFrame & frame, PINDEX & len);
    virtual SendReceiveStatus Internal_OnSendControl(RTP_ControlFrame & frame, PINDEX & len);

    virtual SendReceiveStatus OnReceiveData(RTP_DataFrame & frame);
    virtual SendReceiveStatus Internal_OnReceiveData(RTP_DataFrame & frame);

    virtual SendReceiveStatus OnReceiveControl(RTP_ControlFrame & frame);
    virtual bool yUDPSetRTPbitRate(  int nRate){nRate=-1;return false; };
    class ReceiverReport : public PObject  {
        PCLASSINFO(ReceiverReport, PObject);
      public:
        void PrintOn(ostream &) const;

        DWORD sourceIdentifier;
        DWORD fractionLost;         /* fraction lost since last SR/RR */
        DWORD totalLost;	    /* cumulative number of packets lost (signed!) */
        DWORD lastSequenceNumber;   /* extended last sequence number received */
        DWORD jitter;               /* interarrival jitter */
        PTimeInterval lastTimestamp;/* last SR packet from this source */
        PTimeInterval delay;        /* delay since last SR packet */
    };
    PARRAY(ReceiverReportArray, ReceiverReport);

    class SenderReport : public PObject  {
        PCLASSINFO(SenderReport, PObject);
      public:
        void PrintOn(ostream &) const;

        DWORD sourceIdentifier;
        PTime realTimestamp;
        DWORD rtpTimestamp;
        DWORD packetsSent;
        DWORD octetsSent;
    };
    virtual void OnRxSenderReport(const SenderReport & sender,
                                  const ReceiverReportArray & reports);
    virtual void OnRxReceiverReport(DWORD src,
                                    const ReceiverReportArray & reports);
    virtual void OnReceiverReports(const ReceiverReportArray & reports);

    class SourceDescription : public PObject  {
        PCLASSINFO(SourceDescription, PObject);
      public:
        SourceDescription(DWORD src) { sourceIdentifier = src; }
        void PrintOn(ostream &) const;

        DWORD            sourceIdentifier;
        POrdinalToString items;
    };
    PARRAY(SourceDescriptionArray, SourceDescription);
    virtual void OnRxSourceDescription(const SourceDescriptionArray & descriptions);

    virtual void OnRxGoodbye(const PDWORDArray & sources,
                             const PString & reason);

    virtual void OnRxApplDefined(const PString & type, unsigned subtype, DWORD src,
                                 const BYTE * data, PINDEX size);
  //@}

  /**@name Member variable access */
  //@{
    /**Get the ID for the RTP session.
      */
    unsigned GetSessionID() const { return sessionID; }

    /**Set the ID for the RTP session.
      */
    void SetSessionID(unsigned id) { sessionID = id; }

    /**Get flag for is audio RTP.
      */
    bool IsAudio() const { return isAudio; }

    /**Set flag for RTP session is audio.
     */
    void SetAudio(
      bool aud    /// New audio indication flag
    ) { isAudio = aud; }

    /**Get the canonical name for the RTP session.
      */
    PString GetCanonicalName() const;

    /**Set the canonical name for the RTP session.
      */
    void SetCanonicalName(const PString & name);

    /**Get the tool name for the RTP session.
      */
    PString GetToolName() const;

    /**Set the tool name for the RTP session.
      */
    void SetToolName(const PString & name);

    /**Get the user data for the session.
      */
    RTP_UserData * GetUserData() const { return userData; }

    /**Set the user data for the session.
      */
    void SetUserData(
      RTP_UserData * data,            ///<  New user data to be used
      PBoolean autoDeleteUserData = PTrue  ///<  Delete optional data with session.
    );

    /**Get the source output identifier.
      */
    DWORD GetSyncSourceOut() const { return syncSourceOut; }

    /**Indicate if will ignore all but first received SSRC value.
      */
    bool AllowAnySyncSource() const { return allowAnySyncSource; }

    /**Indicate if will ignore all but first received SSRC value.
      */
    void SetAnySyncSource(
      bool allow    ///<  Flag for allow any SSRC values
    ) { allowAnySyncSource = allow; }

    /**Indicate if will ignore out of order packets.
      */
    PBoolean WillIgnoreOutOfOrderPackets() const { return ignoreOutOfOrderPackets; }

    /**Indicate if will ignore out of order packets.
      */
    void SetIgnoreOutOfOrderPackets(
      PBoolean ignore   ///<  Flag for ignore out of order packets
    ) { ignoreOutOfOrderPackets = ignore; }

    /**Indicate if will ignore rtp payload type changes in received packets.
     */
    void SetIgnorePayloadTypeChanges(
      PBoolean ignore   ///<  Flag to ignore payload type changes
    ) { ignorePayloadTypeChanges = ignore; }

    /**Get the time interval for sending RTCP reports in the session.
      */
    const PTimeInterval & GetReportTimeInterval() { return reportTimeInterval; }

    /**Set the time interval for sending RTCP reports in the session.
      */
    void SetReportTimeInterval(
      const PTimeInterval & interval ///<  New time interval for reports.
    )  { reportTimeInterval = interval; }

    /**Get the current report timer
     */
    PTimeInterval GetReportTimer()
    { return reportTimer; }

    /**Get the interval for transmitter statistics in the session.
      */
    unsigned GetTxStatisticsInterval() { return txStatisticsInterval; }

    /**Set the interval for transmitter statistics in the session.
      */
    void SetTxStatisticsInterval(
      unsigned packets   ///<  Number of packets between callbacks
    );

    /**Get the interval for receiver statistics in the session.
      */
    unsigned GetRxStatisticsInterval() { return rxStatisticsInterval; }

    /**Set the interval for receiver statistics in the session.
      */
    void SetRxStatisticsInterval(
      unsigned packets   ///<  Number of packets between callbacks
    );

    /**Clear statistics
      */
    void ClearStatistics();

    /**Get total number of packets sent in session.
      */
    DWORD GetPacketsSent() const { return packetsSent; }

    /**Get total number of octets sent in session.
      */
    DWORD GetOctetsSent() const { return octetsSent; }

    /**Get total number of packets received in session.
      */
    DWORD GetPacketsReceived() const { return packetsReceived; }

    /**Get total number of octets received in session.
      */
    DWORD GetOctetsReceived() const { return octetsReceived; }

    /**Get total number received packets lost in session.
      */
    DWORD GetPacketsLost() const { return packetsLost; }

    /**Get total number transmitted packets lost by remote in session.
       Determined via RTCP.
      */
    DWORD GetPacketsLostByRemote() const { return packetsLostByRemote; }

    /**Get total number of packets received out of order in session.
      */
    DWORD GetPacketsOutOfOrder() const { return packetsOutOfOrder; }

    /**Get total number received packets too late to go into jitter buffer.
      */
    DWORD GetPacketsTooLate() const;

    /**Get total number received packets that could not fit into the jitter buffer.
      */
    DWORD GetPacketOverruns() const;

    /**Get average time between sent packets.
       This is averaged over the last txStatisticsInterval packets and is in
       milliseconds.
      */
    DWORD GetAverageSendTime() const { return averageSendTime; }

    /**Get the number of marker packets received this session.
       This can be used to find out the number of frames received in a video
       RTP stream.
      */
    DWORD GetMarkerRecvCount() const { return markerRecvCount; }

    /**Get the number of marker packets sent this session.
       This can be used to find out the number of frames sent in a video
       RTP stream.
      */
    DWORD GetMarkerSendCount() const { return markerSendCount; }

    /**Get maximum time between sent packets.
       This is over the last txStatisticsInterval packets and is in
       milliseconds.
      */
    DWORD GetMaximumSendTime() const { return maximumSendTime; }

    /**Get minimum time between sent packets.
       This is over the last txStatisticsInterval packets and is in
       milliseconds.
      */
    DWORD GetMinimumSendTime() const { return minimumSendTime; }

    /**Get average time between received packets.
       This is averaged over the last rxStatisticsInterval packets and is in
       milliseconds.
      */
    DWORD GetAverageReceiveTime() const { return averageReceiveTime; }

    /**Get maximum time between received packets.
       This is over the last rxStatisticsInterval packets and is in
       milliseconds.
      */
    DWORD GetMaximumReceiveTime() const { return maximumReceiveTime; }

    /**Get minimum time between received packets.
       This is over the last rxStatisticsInterval packets and is in
       milliseconds.
      */
    DWORD GetMinimumReceiveTime() const { return minimumReceiveTime; }

    enum { JitterRoundingGuardBits = 4 };
    /**Get averaged jitter time for received packets.
       This is the calculated statistical variance of the interarrival
       time of received packets in milliseconds.
      */
    DWORD GetAvgJitterTime() const { return (jitterLevel>>JitterRoundingGuardBits)/GetJitterTimeUnits(); }

    /**Get averaged jitter time for received packets.
       This is the maximum value of jitterLevel for the session.
      */
    DWORD GetMaxJitterTime() const { return (maximumJitterLevel>>JitterRoundingGuardBits)/GetJitterTimeUnits(); }

    /**Get jitter time for received packets on remote.
       This is the calculated statistical variance of the interarrival
       time of received packets in milliseconds.
      */
    DWORD GetJitterTimeOnRemote() const { return jitterLevelOnRemote/GetJitterTimeUnits(); }
  //@}

    virtual void SetCloseOnBYE(PBoolean v)  { closeOnBye = v; }

    /** Tell the rtp session to send out an intra frame request control packet.
        This is called when the media stream receives an OpalVideoUpdatePicture
        media command.
      */
    virtual void SendIntraFrameRequest();

    void SetNextSentSequenceNumber(WORD num) { lastSentSequenceNumber = (WORD)(num-1); }

    virtual PString GetEncoding() const { return m_encoding; }
    virtual void SetEncoding(const PString & newEncoding);

    DWORD GetSyncSourceIn() const { return syncSourceIn; }

    class EncodingLock
    {
      public:
        EncodingLock(RTP_Session & _session);
        ~EncodingLock();

        __inline RTP_Encoding * operator->() const { return m_encodingHandler; }

      protected:
        RTP_Session  & session;
        RTP_Encoding * m_encodingHandler;
    };

    friend class EncodingLock; 

    void SetFailed(bool v)
    { failed = v; }

    bool HasFailed() const
    { return failed; }

    void AddFilter(const PNotifier & filter);

  protected:
    virtual void SendBYE();
    void AddReceiverReport(RTP_ControlFrame::ReceiverReport & receiver);
    PBoolean InsertReportPacket(RTP_ControlFrame & report);

    PString             m_encoding;
    PMutex              m_encodingMutex;
    RTP_Encoding      * m_encodingHandler;

    unsigned           sessionID;
    bool               isAudio;
    unsigned           m_timeUnits;
    PString            canonicalName;
    PString            toolName;
    RTP_UserData     * userData;
    PBoolean           autoDeleteUserData;

    typedef PSafePtr<RTP_JitterBuffer, PSafePtrMultiThreaded> JitterBufferPtr;
    JitterBufferPtr m_jitterBuffer;

    PBoolean      ignoreOutOfOrderPackets;
    DWORD         syncSourceOut;
    DWORD         syncSourceIn;
    DWORD         lastSentTimestamp;
    bool          allowAnySyncSource;
    bool          allowOneSyncSourceChange;
    PBoolean      allowRemoteTransmitAddressChange;
    PBoolean      allowSequenceChange;
    PTimeInterval reportTimeInterval;
    unsigned      txStatisticsInterval;
    unsigned      rxStatisticsInterval;
    WORD          lastSentSequenceNumber;
    WORD          expectedSequenceNumber;
    PTimeInterval lastSentPacketTime;
    PTimeInterval lastReceivedPacketTime;
    WORD          lastRRSequenceNumber;
    PINDEX        consecutiveOutOfOrderPackets;

    PMutex        dataMutex;
    DWORD         timeStampOffs;               // offset between incoming media timestamp and timeStampOut
    PBoolean      oobTimeStampBaseEstablished; // PTrue if timeStampOffs has been established by media
    DWORD         oobTimeStampOutBase;         // base timestamp value for oob data
    PTimeInterval oobTimeStampBase;            // base time for oob timestamp

    // Statistics
    DWORD packetsSent;
    DWORD rtcpPacketsSent;
    DWORD octetsSent;
    DWORD packetsReceived;
    DWORD octetsReceived;
    DWORD packetsLost;
    DWORD packetsLostByRemote;
    DWORD packetsOutOfOrder;
    DWORD averageSendTime;
    DWORD maximumSendTime;
    DWORD minimumSendTime;
    DWORD averageReceiveTime;
    DWORD maximumReceiveTime;
    DWORD minimumReceiveTime;
    DWORD jitterLevel;
    DWORD jitterLevelOnRemote;
    DWORD maximumJitterLevel;

    DWORD markerSendCount;
    DWORD markerRecvCount;

    unsigned txStatisticsCount;
    unsigned rxStatisticsCount;

    DWORD    averageSendTimeAccum;
    DWORD    maximumSendTimeAccum;
    DWORD    minimumSendTimeAccum;
    DWORD    averageReceiveTimeAccum;
    DWORD    maximumReceiveTimeAccum;
    DWORD    minimumReceiveTimeAccum;
    DWORD    packetsLostSinceLastRR;
    DWORD    lastTransitTime;
    
    RTP_DataFrame::PayloadTypes lastReceivedPayloadType;
    PBoolean ignorePayloadTypeChanges;

    PMutex reportMutex;
    PTimer reportTimer;

    PBoolean closeOnBye;
    PBoolean byeSent;
    bool                failed;      ///<  set to true if session has received too many ICMP destination unreachable

    class Filter : public PObject {
        PCLASSINFO(Filter, PObject);
      public:
        Filter(const PNotifier & n) : notifier(n) { }
        PNotifier notifier;
    };
    PList<Filter> filters;
#ifdef _ORDER_RTP_PACKET
		//chenyuan
    struct mapvalue
    {
      DWORD len;
      char* data;
      mapvalue()
      {
        data =NULL;
      };
      ~mapvalue()
      {
        if (data !=NULL)
          delete[] data;
      };
    };
    PMutex m_mtOldPackets;
    std::map<DWORD, mapvalue> m_oldpackets;
    bool GetOldPacket(const DWORD seqExpect,RTP_DataFrame* buffer);
    bool AddOldPacket(const DWORD seqExpect,const char* buffer,const  int len);
   int m_nTrying;
   bool m_bUsequeues;
   
#endif
    void SetAudioTimestampForSend(DWORD timestamp);
    void SetAudioTimestampForRecv(DWORD timestamp);
    unsigned int GetAudioTimestampForSend( );
    unsigned int GetAudioTimestampForRecv( );
	 int m_nRtpType;// see call type
   SIPConnection *m_sipcon;

};
class OpalConnection;
class XMOpalManager;
class yCCC;
 /**This class is for the IETF Real Time Protocol interface on UDP/IP.
 */
class RTP_UDP : public RTP_Session
{
  PCLASSINFO(RTP_UDP, RTP_Session);

  public:
    UDTSOCKET MakeUDTConnection(PUDPSocket* pSocket/* create self and connect peer*/,bool isHostListener, CCCFactory<yCCC>* pCCCOut );

   bool ud_prepare(PUDPSocket * pSocket, bool listenOrConnect);
  /**@name Construction */
  //@{
    /**Create a new RTP channel.
     */
    RTP_UDP(
      const Params & options ///< Parameters to construct with session.
    );

    /// Destroy the RTP
    ~RTP_UDP();
  //@}

  /**@name Overrides from class RTP_Session */
  //@{
    /**Read a data frame from the RTP channel.
       Any control frames received are dispatched to callbacks and are not
       returned by this function. It will block until a data frame is
       available or an error occurs.
      */
    virtual PBoolean ReadData(RTP_DataFrame & frame, PBoolean loop);
    virtual PBoolean Internal_ReadData(RTP_DataFrame & frame, PBoolean loop);

    /** Write a data frame to the RTP channel.
      */
    virtual PBoolean WriteData(RTP_DataFrame & frame);
    virtual PBoolean Internal_WriteData(RTP_DataFrame & frame);

    /** Write data frame to the RTP channel outside the normal stream of media
      * Used for RFC2833 packets
      */
    virtual PBoolean WriteOOBData(RTP_DataFrame & frame, bool setTimeStamp = true);

    /**Write a control frame from the RTP channel.
      */
    virtual PBoolean WriteControl(RTP_ControlFrame & frame);

    /**Close down the RTP session.
      */
    virtual bool Close(
      PBoolean reading    ///<  Closing the read side of the session
    );

    /**Get the session description name.
      */
    virtual PString GetLocalHostName();
  //@}

    /**Change the QoS settings
      */
    virtual PBoolean ModifyQOS(RTP_QOS * rtpqos);

  /**@name New functions for class */
  //@{
    /**Open the UDP ports for the RTP session.
      */
    virtual PBoolean Open(
      OpalConnection* pCon,
      PIPSocket::Address localAddress,  ///<  Local interface to bind to
      WORD portBase,                    ///<  Base of ports to search
      WORD portMax,                     ///<  end of ports to search (inclusive)
      BYTE ipTypeOfService,             ///<  Type of Service byte
      PNatMethod * natMethod = NULL,    ///<  NAT traversal method to use createing sockets
      RTP_QOS * rtpqos = NULL           ///<  QOS spec (or NULL if no QoS)
    );
  //@}

   /**Reopens an existing session in the given direction.
      */
    virtual void Reopen(PBoolean isReading);
  //@}

  /**@name Member variable access */
  //@{
    /**Get local address of session.
      */
    virtual PIPSocket::Address GetLocalAddress() const { return localAddress; }

    /**Set local address of session.
      */
    virtual void SetLocalAddress(
      const PIPSocket::Address & addr
    ) { localAddress = addr; }

    /**Get remote address of session.
      */
    PIPSocket::Address GetRemoteAddress() const { return remoteAddress; }

    /**Get local data port of session.
      */
    virtual WORD GetLocalDataPort() const { return localDataPort; }

    /**Get local control port of session.
      */
    virtual WORD GetLocalControlPort() const { return localControlPort; }

    /**Get remote data port of session.
      */
    virtual WORD GetRemoteDataPort() const { return remoteDataPort; }

    /**Get remote control port of session.
      */
    virtual WORD GetRemoteControlPort() const { return remoteControlPort; }

    /**Get data UDP socket of session.
      */
    virtual PUDPSocket * GetDataSocket() { return dataSocket; }

    /**Get control UDP socket of session.
      */
    virtual PUDPSocket * GetControlSocket() { return controlSocket; }

    /**Set the remote address and port information for session.
      */
    virtual PBoolean SetRemoteSocketInfo(
      PIPSocket::Address address,   ///<  Addre ss of remote
      WORD port,                    ///<  Port on remote
      PBoolean isDataPort               ///<  Flag for data or control channel
    );

    /**Apply QOS - requires address to connect the socket on Windows platforms
     */
    virtual void ApplyQOS(
      const PIPSocket::Address & addr
    );
  //@}

    virtual int GetDataSocketHandle() const
    { return dataSocket != NULL ? dataSocket->GetHandle() : -1; }

    virtual int GetControlSocketHandle() const
    { return controlSocket != NULL ? controlSocket->GetHandle() : -1; }

    friend class RTP_Encoding;

    virtual int WaitForPDU(PUDPSocket * dataSocket, PUDPSocket * controlSocket, const PTimeInterval & timer);
    virtual int Internal_WaitForPDU(PUDPSocket * dataSocket, PUDPSocket * controlSocket, const PTimeInterval & timer);

    virtual SendReceiveStatus ReadDataPDU(RTP_DataFrame & frame);
    virtual SendReceiveStatus Internal_ReadDataPDU(RTP_DataFrame & frame);

    virtual SendReceiveStatus OnReadTimeout(RTP_DataFrame & frame);
    virtual SendReceiveStatus Internal_OnReadTimeout(RTP_DataFrame & frame);

    virtual SendReceiveStatus ReadControlPDU();
    virtual SendReceiveStatus ReadDataOrControlPDU(
 /*     BYTE * framePtr,
      PINDEX frameSize,*/
      PBYTEArray & frame,
      PBoolean fromDataChannel,
			PINDEX& pdulen
    );    
    virtual bool WriteDataPDU(RTP_DataFrame & frame);
    virtual bool WriteDataOrControlPDU(
      const BYTE * framePtr,
      PINDEX frameSize,
      bool toDataChannel
    );

 
  protected:
    PIPSocket::Address localAddress;
    WORD               localDataPort;
    WORD               localControlPort;

    PIPSocket::Address remoteAddress;
    WORD               remoteDataPort;
    WORD               remoteControlPort;

    PIPSocket::Address remoteTransmitAddress;

    PUDPSocket * dataSocket;
    PUDPSocket * controlSocket;

		PTCPSocket * dataSockettcp;//if support to transmit video VIA TCP channel, we use this 
		PByteBlock*				m_pReadBuf;			// buffer for read, used if we wanna read a whole message each time
		int 				m_nTimeOutForAudioCount;			//  if timeout we hungup the call

    bool shutdownRead;
    bool shutdownWrite;
    bool appliedQOS;
    bool remoteIsNAT;
    bool localHasNAT;
    bool first;
    time_t m_lasttimeRTPSend;
    int  badTransmitCounter;
    PTime badTransmitStart;
    unsigned int  m_nYYmeetingUID;
    int/*UDTSOCKET*/ m_fhandleForListen, m_fhandleClient;
    int m_timeOutForGetYYmeetingChan;
    void* m_yCCC;
    int m_emRepairSocket;//em_rtp_type
    
    bool yUDPWrite(PUDPSocket* socket, void * data, int len);
   virtual bool yUDPSetRTPbitRate(  int nRate);//set the udt rate
    PTimeInterval m_tick, m_preTick;
     
    bool yUDPRead(PUDPSocket* socket,       void * buf,     /// Data to be written as URGENT TCP data.
      PINDEX& len,     /// Number of bytes pointed to by #buf#.
      PIPSocket::Address & addr, /// Address from which the datagram was received.
      WORD & port ,    /// Port from which the datagram was received.
      bool fromDataChannel
    );

};

/////////////////////////////////////////////////////////////////////////////

class RTP_UDP;

class RTP_Encoding
{
  public:
    RTP_Encoding();
    virtual ~RTP_Encoding();
    virtual void OnStart(RTP_Session & _rtpSession);
    virtual void OnFinish();
    virtual RTP_Session::SendReceiveStatus OnSendData(RTP_DataFrame & frame);
    virtual PBoolean WriteData(RTP_DataFrame & frame, bool oob);
    virtual PBoolean WriteDataPDU(RTP_DataFrame & frame);
    virtual RTP_Session::SendReceiveStatus OnSendControl(RTP_ControlFrame & frame, PINDEX & len);
    virtual RTP_Session::SendReceiveStatus ReadDataPDU(RTP_DataFrame & frame);
    virtual RTP_Session::SendReceiveStatus OnReceiveData(RTP_DataFrame & frame);
    virtual RTP_Session::SendReceiveStatus OnReadTimeout(RTP_DataFrame & frame);
    virtual PBoolean ReadData(RTP_DataFrame & frame, PBoolean loop);
    virtual int WaitForPDU(PUDPSocket * dataSocket, PUDPSocket * controlSocket, const PTimeInterval &);

    PMutex      mutex;
    unsigned    refCount;

  protected:
    RTP_UDP     * rtpUDP;
};

PFACTORY_LOAD(RTP_Encoding);


/////////////////////////////////////////////////////////////////////////////

class SecureRTP_UDP : public RTP_UDP
{
  PCLASSINFO(SecureRTP_UDP, RTP_UDP);

  public:
  /**@name Construction */
  //@{
    /**Create a new RTP channel.
     */
    SecureRTP_UDP(
      const Params & options ///< Parameters to construct with session.
    );

    /// Destroy the RTP
    ~SecureRTP_UDP();

    virtual void SetSecurityMode(OpalSecurityMode * srtpParms);  
    virtual OpalSecurityMode * GetSecurityParms() const;

  protected:
    OpalSecurityMode * securityParms;
};

#endif // OPAL_RTP_RTP_H

/////////////////////////////////////////////////////////////////////////////
