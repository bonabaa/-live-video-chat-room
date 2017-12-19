/*
 * lidep.h
 *
 * Line Interface Device EndPoint
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
 * Portions of this code were written with the assisance of funding from 
 * Quicknet Technologies, Inc. http://www.quicknet.net.
 * 
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23275 $
 * $Author: rjongbloed $
 * $Date: 2009-08-27 07:17:54 +0000 (Thu, 27 Aug 2009) $
 */

#ifndef OPAL_LIDS_LIDEP_H
#define OPAL_LIDS_LIDEP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <opal/endpoint.h>
#include <lids/lid.h>
#include <codec/silencedetect.h>


class OpalLineConnection;


/**This class describes and endpoint that handles LID lines.
   This is the ancestor class to the LID endpoints that handle PSTN lines
   and POTS lines respectively.
 */
class OpalLineEndPoint : public OpalEndPoint
{
  PCLASSINFO(OpalLineEndPoint, OpalEndPoint);

  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalLineEndPoint(
      OpalManager & manager   ///<  Manager of all endpoints.
    );

    /// Make sure thread has stopped before exiting.
    ~OpalLineEndPoint();
  //@}

  /**@name Overrides from OpalEndPoint */
  //@{
    /**Set up a connection to a remote party.
       This is called from the OpalManager::MakeConnection() function once
       it has determined that this is the endpoint for the protocol.

       The general form for this party parameter is:

            [proto:][alias@][transport$]address[:port]

       where the various fields will have meanings specific to the endpoint
       type. For example, with H.323 it could be "h323:Fred@site.com" which
       indicates a user Fred at gatekeeper size.com. Whereas for the PSTN
       endpoint it could be "pstn:5551234" which is to call 5551234 on the
       first available PSTN line.

       The proto field is optional when passed to a specific endpoint. If it
       is present, however, it must agree with the endpoints protocol name or
       PFalse is returned.

       This function usually returns almost immediately with the connection
       continuing to occur in a new background thread.

       If PFalse is returned then the connection could not be established. For
       example if a PSTN endpoint is used and the assiciated line is engaged
       then it may return immediately. Returning a non-NULL value does not
       mean that the connection will succeed, only that an attempt is being
       made.

       The default behaviour is pure.
     */
    virtual PSafePtr<OpalConnection> MakeConnection(
      OpalCall & call,          ///< Owner of connection
      const PString & party,    ///< Remote party to call
      void * userData = NULL,   ///< Arbitrary data to pass to connection
      unsigned int options = 0,  ///<  options to pass to conneciton
      OpalConnection::StringOptions * stringOptions  = NULL
    );

    /**Get the data formats this endpoint is capable of operating.
       This provides a list of media data format names that may be used by an
       OpalMediaStream may be created by a connection from this endpoint.

       Note that a specific connection may not actually support all of the
       media formats returned here, but should return no more.

       The default behaviour is pure.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;
  //@}

  /**@name Connection management */
  //@{
    virtual OpalLineConnection * CreateConnection(
      OpalCall & call,        ///<  Call that owns the connection
      OpalLine & line,        ///<  Line connection is to use
      void * userData,        ///<  Arbitrary user data from MakeConnection
      const PString & number  ///<  Number to dial in outgoing call
    );
  //@}

  /**@name LID management */
  //@{
    /**Find a connection that uses the specified token.
       This searches the endpoint for the connection that contains the token
       as provided by functions such as MakeConnection().
      */
    PSafePtr<OpalLineConnection> GetLIDConnectionWithLock(
      const PString & token,     ///<  Token to identify connection
      PSafetyMode mode = PSafeReadWrite
    ) { return PSafePtrCast<OpalConnection, OpalLineConnection>(GetConnectionWithLock(token, mode)); }

    /**Add a line to the endpoint.
       Note that once the line is added it is "owned" by the endpoint and
       should not be deleted directly. Use the RemoveLine() function to
       delete the line.

       Returns PTrue if the lines device was open and the line was added.
      */
    PBoolean AddLine(
      OpalLine * line
    );

    /**Remove a line from the endpoint.
       The line is removed from the endpoints processing and deleted.
      */
    void RemoveLine(
      OpalLine * line
    );

    
    /**
     * get all lines of the endpopint 
     * @return the constant list of all lines belonged to the endpoint 
     */
    const PList<OpalLine> & GetLines() const { return lines;};
    
    /**Remove a line from the endpoint.
       The line is removed from the endpoints processing and deleted.
      */
    void RemoveLine(
      const PString & token
    );

    /**Remove all lines from the endpoint.
       The line is removed from the endpoints processing and deleted. All
       devices are also deleted from the endpoint
      */
    void RemoveAllLines();

    /**Add all lines on a device to the endpoint.
       Note that once the line is added it is "owned" by the endpoint and
       should not be deleted directly. Use the RemoveLine() function to
       delete the line.

       Note the device should already be open or no lines are added.

       Returns PTrue if at least one line was added.
      */
    virtual PBoolean AddLinesFromDevice(
      OpalLineInterfaceDevice & device  ///<  Device to add lines
    );

    /**Remove all lines on a device from the endpoint.
       The lines are removed from the endpoints processing and deleted.
      */
    void RemoveLinesFromDevice(
      OpalLineInterfaceDevice & device  ///<  Device to remove lines
    );

    /**Add a line interface devices to the endpoint by name.
       This calls AddDeviceName() for each entry in the array.

       Returns PTrue if at least one line from one device was added.
      */
    PBoolean AddDeviceNames(
      const PStringArray & descriptors  ///<  Device descritptions to add
    );

    /**Add a line interface device to the endpoint by name.
       This will add the registered OpalLineInterfaceDevice descendent and
       all of the lines that it has to the endpoint. The parameter is a string
       as would be returned from the OpalLineInterfaceDevice::GetAllDevices()
       function.

       Returns PTrue if at least one line was added or the descriptor was
       already present.
      */
    PBoolean AddDeviceName(
      const PString & descriptor  ///<  Device descritption to add
    );

    /**Indicate if the line interface device is in use by the endpoint.
      */
    const OpalLineInterfaceDevice * GetDeviceByName(
      const PString & descriptor  ///<  Device descritption to add
    );

    /**Add a line interface device to the endpoint.
       This will add the OpalLineInterfaceDevice descendent and all of the
       lines that it has to the endpoint.

       The lid is then "owned" by the endpoint and will be deleted
       automatically when the endpoint is destroyed.

       Note the device should already be open or no lines are added.

       Returns PTrue if at least one line was added.
      */
    virtual PBoolean AddDevice(
      OpalLineInterfaceDevice * device    ///<  Device to add
    );

    /**Remove the device and all its lines from the endpoint.
       The device will be automatically deleted.
      */
    void RemoveDevice(
      OpalLineInterfaceDevice * device  ///<  Device to remove
    );

    /**Remove all devices from the endpoint.
      */
    void RemoveDevices() { RemoveAllLines(); }

    /**Get the line by name.
       The lineName parameter may be "*" to matche the first line.

       If the enableAudio flag is PTrue then the EnableAudio() function is
       called on the line and it is returns only if successful. This
       effectively locks the line for exclusive use of the caller.
      */
    OpalLine * GetLine(
      const PString & lineName,  ///< Name of line to get.
      bool enableAudio = false,  ///< Flag to enable audio on the line.
      bool terminating = true    ///< Flag to indicate should be a terminating line
    );

    /**Set the default line to be used on call.
       If the lineName is "*" then the first available line is used.
      */
    void SetDefaultLine(
      const PString & lineName  ///<  Name of line to set to default.
    );
  //@}


  protected:
    PDECLARE_NOTIFIER(PThread, OpalLineEndPoint, MonitorLines);
    virtual void MonitorLine(OpalLine & line);

    OpalLIDList  devices;
    OpalLineList lines;
    PString      defaultLine;
    PMutex       linesMutex;
    PThread    * monitorThread;
    PSyncPoint   exitFlag;
};


/**This class describes the LID based codec capability.
 */
class OpalLineConnection : public OpalConnection
{
  PCLASSINFO(OpalLineConnection, OpalConnection);

  public:
  /**@name Construction */
  //@{
    /**Create a new connection.
     */
    OpalLineConnection(
      OpalCall & call,              ///<  Owner calll for connection
      OpalLineEndPoint & endpoint,   ///<  Endpoint for LID connection
      OpalLine & line,              ///<  Line to make connection on
      const PString & number        ///<  Number to call on line
    );
  //@}

  /**@name Overrides from OpalConnection */
  //@{
    /**Get this connections protocol prefix for URLs.
      */
    virtual PString GetPrefixName() const;

    /**Get indication of connection being to a "network".
       This indicates the if the connection may be regarded as a "network"
       connection. The distinction is about if there is a concept of a "remote"
       party being connected to and is best described by example: sip, h323,
       iax and pstn are all "network" connections as they connect to something
       "remote". While pc, pots and ivr are not as the entity being connected
       to is intrinsically local.
      */
    virtual bool IsNetworkConnection() const { return !line.IsTerminal(); }

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
       has received an OnAlerting() indicating that its remoteendpoint is
       "ringing".

       The default behaviour starts the ring back tone.
      */
    virtual PBoolean SetAlerting(
      const PString & calleeName,   ///<  Name of endpoint being alerted.
      PBoolean withMedia                ///<  Open media with alerting
    );

    /**Indicate to remote endpoint we are connected.

       The default behaviour stops the ring back tone.
      */
    virtual PBoolean SetConnected();

    /**Clean up the termination of the connection.
       This function can do any internal cleaning up and waiting on background
       threads that may be using the connection object.

       Note that there is not a one to one relationship with the
       OnEstablishedConnection() function. This function may be called without
       that function being called. For example if SetUpConnection() was used
       but the call never completed.

       Classes that override this function should make sure they call the
       ancestor version for correct operation.

       An application will not typically call this function as it is used by
       the OpalManager during a release of the connection.

       The default behaviour calls starts playing the busy tone and calls the
       ancestor function.
      */
    virtual void OnReleased();

    /**Get the destination address of an incoming connection.
       The default behaviour collects a DTMF number terminated with a '#' or
       if no digits were entered for a time (default 3 seconds). If no digits
       are entered within a longer time time (default 30 seconds), then an
       empty string is returned.
      */
    virtual PString GetDestinationAddress();

    /**Get the data formats this connection is capable of operating.
       This provides a list of media data format names that a
       OpalMediaStream may be created in within this connection.

       The default behaviour returns the capabilities of the LID line.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;

    /**Open a new media stream.
       This will create a media stream of an appropriate subclass as required
       by the underlying connection protocol. For instance H.323 would create
       an OpalRTPStream.

       The sessionID parameter may not be needed by a particular media stream
       and may be ignored. In the case of an OpalRTPStream it us used.

       Note that media streams may be created internally to the underlying
       protocol. This function is not the only way a stream can come into
       existance.

       The default behaviour is pure.
     */
    virtual OpalMediaStream * CreateMediaStream(
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      PBoolean isSource                        ///<  Is a source stream
    );

    /**Call back when opening a media stream.
       This function is called when a connection has created a new media
       stream according to the logic of its underlying protocol.

       The usual requirement is that media streams are created on all other
       connections participating in the call and all of the media streams are
       attached to an instance of an OpalMediaPatch object that will read from
       one of the media streams passing data to the other media streams.

       The default behaviour calls the ancestor and adds a LID silence
       detector filter.
      */
    virtual PBoolean OnOpenMediaStream(
      OpalMediaStream & stream    ///<  New media stream being opened
    );

    /**Set  the volume (gain) for the audio media channel to the specified percentage.
      */
    virtual PBoolean SetAudioVolume(
      PBoolean source,                  ///< true for source (microphone), false for sink (speaker)
      unsigned percentage           ///< Gain, 0=silent, 100=maximun
    );

    /**Get the average signal level (0..32767) for the audio media channel.
       A return value of UINT_MAX indicates no valid signal, eg no audio channel opened.
      */
    virtual unsigned GetAudioSignalLevel(
      PBoolean source                   ///< true for source (microphone), false for sink (speaker)
    );

    /**Send a user input indication to the remote endpoint.
       This sends an arbitrary string as a user indication. If DTMF tones in
       particular are required to be sent then the SendIndicationTone()
       function should be used.

       The default behaviour plays the DTMF tones on the line.
      */
    virtual PBoolean SendUserInputString(
      const PString & value                   ///<  String value of indication
    );

    /**Send a user input indication to the remote endpoint.
       This sends DTMF emulation user input. If something other than the
       standard tones need be sent use the SendUserInputString() function.

       The default behaviour plays the DTMF tone on the line.
      */
    virtual PBoolean SendUserInputTone(
      char tone,    ///<  DTMF tone code
      int duration  ///<  Duration of tone in milliseconds
    );

    /**Play a prompt to the connection before rading user indication string.

       For example the LID connection would play a dial tone.

       The default behaviour does nothing.
      */
    virtual PBoolean PromptUserInput(
      PBoolean play   ///<  Flag to start or stop playing the prompt
    );
  //@}

  /**@name Call handling functions */
  //@{
    /**Handle a new connection from the LID line.
      */
    void StartIncoming();

    /**Check for line hook state, DTMF tone for user indication etc.
      */
    virtual void Monitor();
  //@}


  /**@name Member variable access */
  //@{
    /**Get the line being used by this media stream.
      */
    OpalLine & GetLine() { return line; }

    /** Get the prompt tone used on POTS lines.
        Defaults to OpalLineInterfaceDevice::DialTone.
      */
    OpalLineInterfaceDevice::CallProgressTones GetPromptTone() const { return m_promptTone; }

    /** Set the prompt tone used on POTS lines.
      */
    void SetPromptTone(OpalLineInterfaceDevice::CallProgressTones tone) { m_promptTone = tone; }

    /** delay in msec to wait between the dial tone detection and dialing the dtmf 
      * @param uiDial the dial delay to set
     */
    void setDialDelay(unsigned int uiDialDelay) { m_dialParams.m_dialStartDelay = uiDialDelay;}
    
    /** delay in msec to wait between the dial tone detection and dialing the dtmf 
     * @return uiDialDelay the dial delay to get
     */
    unsigned int getDialDelay() const { return m_dialParams.m_dialStartDelay; }
  //@}
        
  protected:
    OpalLineEndPoint & endpoint;
    OpalLine        & line;
    bool              wasOffHook;
    unsigned          minimumRingCount;
    OpalLineInterfaceDevice::DialParams m_dialParams;
    OpalLineInterfaceDevice::CallProgressTones m_promptTone;

    PDECLARE_NOTIFIER(PThread, OpalLineConnection, HandleIncoming);
    PThread         * handlerThread;
};


/**This class describes a media stream that transfers data to/from a Line
   Interface Device.
  */
class OpalLineMediaStream : public OpalMediaStream
{
    PCLASSINFO(OpalLineMediaStream, OpalMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream for Line Interface Devices.
      */
    OpalLineMediaStream(
      OpalLineConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      PBoolean isSource,                       ///<  Is a source stream
      OpalLine & line                      ///<  LID line to stream to/from
    );
  //@}

    ~OpalLineMediaStream();


  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Open the media stream.

       The default behaviour sets the OpalLineInterfaceDevice format and
       calls Resume() on the associated OpalMediaPatch thread.
      */
    virtual PBoolean Open();

    /**Close the media stream.

       The default does nothing.
      */
    virtual PBoolean Close();

    /**Read an RTP frame of data from the source media stream.
       The default behaviour simply calls ReadData() on the data portion of the
       RTP_DataFrame and sets the frames timestamp and marker from the internal
       member variables of the media stream class.
      */
    virtual PBoolean ReadPacket(
      RTP_DataFrame & packet
    );

    /**Write an RTP frame of data to the sink media stream.
       The default behaviour simply calls WriteData() on the data portion of the
       RTP_DataFrame and and sets the internal timestamp and marker from the
       member variables of the media stream class.
      */
    virtual PBoolean WritePacket(
      RTP_DataFrame & packet
    );

    /**Read raw media data from the source media stream.
       The default behaviour reads from the OpalLine object.
      */
    virtual PBoolean ReadData(
      BYTE * data,      ///<  Data buffer to read to
      PINDEX size,      ///<  Size of buffer
      PINDEX & length   ///<  Length of data actually read
    );

    /**Write raw media data to the sink media stream.
       The default behaviour writes to the OpalLine object.
      */
    virtual PBoolean WriteData(
      const BYTE * data,   ///<  Data to write
      PINDEX length,       ///<  Length of data to read.
      PINDEX & written     ///<  Length of data actually written
    );

    /**Set the data size in bytes that is expected to be used. Some media
       streams can make use of this information to perform optimisations.

       The default behaviour does nothing.
      */
    virtual PBoolean SetDataSize(
      PINDEX dataSize,  ///< New data size (in total)
      PINDEX frameTime  ///< Individual frame time (if applicable)
    );

    /**Indicate if the media stream is synchronous.
       Returns PTrue for LID streams.
      */
    virtual PBoolean IsSynchronous() const;

    /**Indicate if the media stream requires a OpalMediaPatch thread (active patch).
       This is called on the source/sink stream and is passed the sink/source
       stream that the patch will initially be using. The function could
       conditionally require the patch thread to execute a thread reading and
       writing data, or prevent  it from doing so as it can do so in hardware
       in some way.

       The default behaviour here determines if both streams are on the same
       OpalLineInterfaceDevice and returns false if so.
      */
    virtual PBoolean RequiresPatchThread(
      OpalMediaStream * stream  ///< Other stream in patch
    ) const;
  //@}

  /**@name Member variable access */
  //@{
    /**Get the line being used by this media stream.
      */
    OpalLine & GetLine() { return line; }
  //@}

  protected:
    OpalLine & line;
    bool       notUsingRTP;
    bool       useDeblocking;
    unsigned   missedCount;
    BYTE       lastSID[4];
    bool       lastFrameWasSignal;
    unsigned   directLineNumber;
};


class OpalLineSilenceDetector : public OpalSilenceDetector
{
    PCLASSINFO(OpalLineSilenceDetector, OpalSilenceDetector);
  public:
  /**@name Construction */
  //@{
    /**Create a new silence detector for a LID.
     */
    OpalLineSilenceDetector(
      OpalLine & line,        ///<  Line to detect silence on
      const Params & newParam ///<  New parameters for silence detector
    );
  //@}

  /**@name Overrides from OpalSilenceDetector */
  //@{
    /**Get the average signal level in the stream.
       This is called from within the silence detection algorithm to
       calculate the average signal level of the last data frame read from
       the stream.

       The default behaviour returns UINT_MAX which indicates that the
       average signal has no meaning for the stream.
      */
    virtual unsigned GetAverageSignalLevel(
      const BYTE * buffer,  ///<  RTP payload being detected
      PINDEX size           ///<  Size of payload buffer
    );
  //@}

  protected:
    OpalLine & line;
};


#endif // OPAL_LIDS_LIDEP_H


// End of File ///////////////////////////////////////////////////////////////
