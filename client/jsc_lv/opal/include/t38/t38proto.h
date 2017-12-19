/*
 * t38proto.h
 *
 * T.38 protocol handler
 *
 * Open Phone Abstraction Library
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23926 $
 * $Author: rjongbloed $
 * $Date: 2010-01-13 00:44:33 +0000 (Wed, 13 Jan 2010) $
 */

#ifndef OPAL_T38_T38PROTO_H
#define OPAL_T38_T38PROTO_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>


#if OPAL_FAX

#include <ptlib/pipechan.h>

#include <opal/mediafmt.h>
#include <opal/mediastrm.h>
#include <opal/endpoint.h>


class OpalTransport;
class T38_IFPPacket;
class PASN_OctetString;
class OpalFaxConnection;


///////////////////////////////////////////////////////////////////////////////

class OpalFaxConnection;

/** Fax Endpoint.
    This class represents connection that can take a standard group 3 fax
    TIFF file and produce either T.38 packets or actual tones represented
    by a stream of PCM. For T.38 it is expected the second connection in the
    call supports T.38 e.g. SIP or H.323. If PCM is being used then the second
    connection may be anything that supports PCM, such as SIP or H.323 using
    G.711 codec or OpalLineEndpoint which could the send the TIFF file to a
    physical fax machine.

    Relies on the presence of the spandsp plug in to do the hard work.
 */
class OpalFaxEndPoint : public OpalEndPoint
{
  PCLASSINFO(OpalFaxEndPoint, OpalEndPoint);
  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalFaxEndPoint(
      OpalManager & manager,        ///<  Manager of all endpoints.
      const char * g711Prefix = "fax", ///<  Prefix for URL style address strings
      const char * t38Prefix = "t38"  ///<  Prefix for URL style address strings
    );

    /**Destroy endpoint.
     */
    ~OpalFaxEndPoint();
  //@}

    virtual PSafePtr<OpalConnection> MakeConnection(
      OpalCall & call,          ///<  Owner of connection
      const PString & party,    ///<  Remote party to call
      void * userData = NULL,          ///<  Arbitrary data to pass to connection
      unsigned int options = 0,     ///<  options to pass to conneciton
      OpalConnection::StringOptions * stringOptions = NULL
    );

    /**Create a connection for the fax endpoint.
      */
    virtual OpalFaxConnection * CreateConnection(
      OpalCall & call,          ///< Owner of connection
      void * userData,
      OpalConnection::StringOptions * stringOptions,
      const PString & filename, ///< filename to send/receive
      bool receiving,
      bool disableT38
    );

    /**Get the data formats this endpoint is capable of operating.
       This provides a list of media data format names that may be used by an
       OpalMediaStream may be created by a connection from this endpoint.

       Note that a specific connection may not actually support all of the
       media formats returned here, but should return no more.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;

  /**@name User Interface operations */
    /**Accept the incoming connection.
      */
    virtual void AcceptIncomingConnection(
      const PString & connectionToken ///<  Token of connection to accept call
    );

    /**Fax transmission/receipt completed.
       Default behaviour releases the connection.
      */
    virtual void OnFaxCompleted(
      OpalFaxConnection & connection, ///< Connection that completed.
      bool failed   ///< Fax ended with failure
    );
  //@}

  /**@name Member variable access */
    /**Get the default directory for received faxes.
      */
    const PString & GetDefaultDirectory() const { return m_defaultDirectory; }

    /**Set the default directory for received faxes.
      */
    void SetDefaultDirectory(
      const PString & dir    /// New directory for fax reception
    ) { m_defaultDirectory = dir; }

    const PString & GetT38Prefix() const { return m_t38Prefix; }
  //@}

  protected:
    PString    m_t38Prefix;
    PDirectory m_defaultDirectory;
};


///////////////////////////////////////////////////////////////////////////////

/** Fax Connection.
    There are six modes of operation:
        Mode            receiving     disableT38    filename
        TIFF -> T.38      false         false       "something.tif"
        T.38 -> TIFF      true          false       "something.tif"
        TIFF -> G.711     false         true        "something.tif"
        G.711 ->TIFF      true          true        "something.tif"
        T.38  -> G.711    false       don't care    PString::Empty()
        G.711 -> T.38     true        don't care    PString::Empty()

    If T.38 is involved then there is generally two stages to the setup, as
    indicated by the m_switchedToT38 flag. When false then we are in audio
    mode looking for CNG/CED tones. When true, then we are switching, or have
    switched, to T.38 operation. If the switch fails, then the m_disableT38
    is set and we proceed in fall back mode.
 */
class OpalFaxConnection : public OpalConnection
{
  PCLASSINFO(OpalFaxConnection, OpalConnection);
  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalFaxConnection(
      OpalCall & call,                 ///< Owner calll for connection
      OpalFaxEndPoint & endpoint,      ///< Owner endpoint for connection
      const PString & filename,        ///< TIFF file name to send/receive
      bool receiving,                  ///< True if receiving a fax
      bool disableT38,                 ///< True if want to force G.711
      OpalConnection::StringOptions * stringOptions = NULL
    );

    /**Destroy endpoint.
     */
    ~OpalFaxConnection();
  //@}

  /**@name Overrides from OpalConnection */
  //@{
    virtual PString GetPrefixName() const;

    /**Get indication of connection being to a "network".
       This indicates the if the connection may be regarded as a "network"
       connection. The distinction is about if there is a concept of a "remote"
       party being connected to and is best described by example: sip, h323,
       iax and pstn are all "network" connections as they connect to something
       "remote". While pc, pots and ivr are not as the entity being connected
       to is intrinsically local.
      */
    virtual bool IsNetworkConnection() const { return false; }

    virtual void ApplyStringOptions(OpalConnection::StringOptions & stringOptions);
    virtual OpalMediaFormatList GetMediaFormats() const;
    virtual void AdjustMediaFormats(bool local, OpalMediaFormatList & mediaFormats, OpalConnection * otherConnection) const;

    /**Start an outgoing connection.
       This function will initiate the connection to the remote entity, for
       example in H.323 it sends a SETUP, in SIP it sends an INVITE etc.

       The default behaviour does.
      */
    virtual PBoolean SetUpConnection();

    /**Indicate to remote endpoint an alert is in progress.
       If this is an incoming connection and the AnswerCallResponse is in a
       AnswerCallDeferred or AnswerCallPending state, then this function is
       used to indicate to that endpoint that an alert is in progress. This is
       usually due to another connection which is in the call (the B party)
       has received an OnAlerting() indicating that its remote endpoint is
       "ringing".

       The default behaviour does nothing.
      */
    virtual PBoolean SetAlerting(
      const PString & calleeName,   ///<  Name of endpoint being alerted.
      PBoolean withMedia                ///<  Open media with alerting
    );

    virtual PBoolean SetConnected();
    virtual void OnEstablished();
    virtual void OnReleased();
    virtual OpalMediaStream * CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, PBoolean isSource);
    virtual void OnStartMediaPatch(OpalMediaPatch & patch);
    virtual void OnStopMediaPatch(OpalMediaPatch & patch);
    virtual PBoolean SendUserInputTone(char tone, unsigned duration);
    virtual void OnUserInputTone(char tone, unsigned duration);
    virtual bool SwitchFaxMediaStreams(bool enableFax);
    virtual void OnSwitchedFaxMediaStreams(bool enabledFax);

  /**@name New operations */
  //@{
    /**Accept the incoming connection.
      */
    virtual void AcceptIncoming();

    /**Fax transmission/receipt completed.
       Default behaviour calls equivalent function on OpalFaxEndPoint.
      */
    virtual void OnFaxCompleted(
      bool failed   ///< Fax ended with failure
    );

#if OPAL_STATISTICS
    /**Get fax transmission/receipt statistics.
      */
    virtual void GetStatistics(
      OpalMediaStatistics & statistics  ///< Statistics for call
    ) const;
#endif

    /**Get the file to send/receive
      */
    const PString & GetFileName() const { return m_filename; }

    /**Get receive fax flag.
      */
    bool IsReceive() const { return m_receiving; }
  //@}

  protected:
    PDECLARE_NOTIFIER(PTimer,  OpalFaxConnection, OnSendCNGCED);
    PDECLARE_NOTIFIER(PThread, OpalFaxConnection, OpenFaxStreams);


    OpalFaxEndPoint & m_endpoint;
    PString           m_filename;
    bool              m_receiving;
    PString           m_stationId;
    bool              m_disableT38;
    PTimeInterval     m_releaseTimeout;
    PTimeInterval     m_switchTimeout;
    OpalMediaFormat   m_tiffFileFormat;

    bool     m_awaitingSwitchToT38;
    PTimer   m_faxTimer;

  friend class OpalFaxMediaStream;
};


typedef OpalFaxConnection OpalT38Connection; // For backward compatibility


#endif // OPAL_FAX

#endif // OPAL_T38_T38PROTO_H
