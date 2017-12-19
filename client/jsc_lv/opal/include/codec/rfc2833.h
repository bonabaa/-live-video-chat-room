/*
 * rfc2833.h
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
 * $Revision: 23862 $
 * $Author: rjongbloed $
 * $Date: 2009-12-10 01:35:47 +0000 (Thu, 10 Dec 2009) $
 */

#ifndef OPAL_CODEC_RFC2833_H
#define OPAL_CODEC_RFC2833_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <rtp/rtp.h>


class OpalMediaFormat;


///////////////////////////////////////////////////////////////////////////////

class OpalRFC2833Info : public PObject {
    PCLASSINFO(OpalRFC2833Info, PObject);
  public:
    // the following values are mandated by RFC 2833
    enum NTEEvent {
      Digit0 = 0,
      Digit1 = 1,
      Digit2 = 2,
      Digit3 = 3,
      Digit4 = 4,
      Digit5 = 5,
      Digit6 = 6,
      Digit7 = 7,
      Digit8 = 8,
      Digit9 = 9,
      Star   = 10,
      Hash   = 11,
      A      = 12,
      B      = 13,
      C      = 14,
      D      = 15,
      Flash  = 16,
      CED    = 32,
      CNG    = 36
    };

    OpalRFC2833Info(
      char tone,
      unsigned duration = 0,
      unsigned timestamp = 0
    );

    char GetTone() const { return tone; }
    unsigned GetDuration() const { return duration; }
    unsigned GetTimestamp() const { return timestamp; }
    bool IsToneStart() const { return duration == 0; }

  protected:
    char     tone;
    unsigned duration;
    unsigned timestamp;
};

class OpalRTPConnection;

class OpalRFC2833Proto : public PObject {
    PCLASSINFO(OpalRFC2833Proto, PObject);
  public:
    OpalRFC2833Proto(
      OpalRTPConnection & conn,
      const PNotifier & receiveNotifier,
      const OpalMediaFormat & mediaFormat
    );
    ~OpalRFC2833Proto();

    virtual bool SendToneAsync(
      char tone, 
      unsigned duration
    );

    virtual void OnStartReceive(
      char tone,
      unsigned timestamp
    );
    virtual void OnStartReceive(
      char tone
    );
    virtual void OnEndReceive(
      char tone,
      unsigned duration,
      unsigned timestamp
    );

    RTP_DataFrame::PayloadTypes GetPayloadType() const { return m_payloadType; }

    void SetPayloadType(
      RTP_DataFrame::PayloadTypes type ///<  new payload type
    ) { m_payloadType = type; }

    const PNotifier & GetReceiveHandler() const { return m_receiveHandler; }

    PString GetTxCapability() const;
    PString GetRxCapability() const;
    void SetTxCapability(const PString & codes, bool merge);
    void SetRxCapability(const PString & codes);

    static PINDEX ASCIIToRFC2833(char tone, bool hasNSE);
    static char RFC2833ToASCII(PINDEX rfc2833, bool hasNSE);

  protected:
    void SendAsyncFrame();

    PDECLARE_NOTIFIER(RTP_DataFrame, OpalRFC2833Proto, ReceivedPacket);
    PDECLARE_NOTIFIER(PTimer, OpalRFC2833Proto, ReceiveTimeout);
    PDECLARE_NOTIFIER(PTimer, OpalRFC2833Proto, AsyncTimeout);

    OpalRTPConnection         & m_connection;
    RTP_DataFrame::PayloadTypes m_payloadType;
    std::vector<bool>           m_txCapabilitySet;
    std::vector<bool>           m_rxCapabilitySet;
    PNotifier                   m_receiveNotifier;
    PNotifier                   m_receiveHandler;

    enum {
      ReceiveIdle,
      ReceiveActive,
      ReceiveEnding
    } m_receiveState;

    BYTE     m_receivedTone;
    unsigned m_tonesReceived;
    PTimer   m_receiveTimer;
    DWORD    m_previousReceivedTimestamp;

    enum {
      TransmitIdle,
      TransmitActive,
      TransmitEnding1,
      TransmitEnding2,
      TransmitEnding3,
    } m_transmitState;

    RTP_Session * m_rtpSession;
    PTimer        m_asyncTransmitTimer;
    PTimer        m_asyncDurationTimer;
    DWORD         m_transmitTimestamp;
    bool          m_rewriteTransmitTimestamp;
    PTimeInterval m_asyncStart;
    BYTE          m_transmitCode;
    unsigned      m_transmitDuration;

    PMutex m_mutex;
};


#endif // OPAL_CODEC_RFC2833_H


/////////////////////////////////////////////////////////////////////////////
