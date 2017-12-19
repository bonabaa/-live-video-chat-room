/*
 * opal.h
 *
 * "C" language interface for OPAL
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2008 Vox Lucida
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
 * The Initial Developer of the Original Code is Vox Lucida (Robert Jongbloed)
 *
 * This code was initially written with the assisance of funding from
 * Stonevoice. http://www.stonevoice.com.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23980 $
 * $Author: rjongbloed $
 * $Date: 2010-01-25 01:30:23 +0000 (Mon, 25 Jan 2010) $
 */

#ifndef OPAL_OPAL_H
#define OPAL_OPAL_H

#include <stdlib.h>


#ifdef __cplusplus
extern "C" {
#endif

/**\file opal.h
   \brief This file contains a simplified API to the OPAL system. It provides a
    pure "C" language interface as will as a very simple C++ class and a
    contrained set of functions for "late binding".

    It should be noted that the simplified API, is sill not very simple. There are
    complexities invoved that cannot be avoided. However, this API does remove
    some issues with the full API such as multi-threading and advanced C++ concepts.

    The other major feature of this API is the ability to be  easily "late bound"
    using Windows LoadLibrary() or Unix dlopen() at run time. You may look at the
    sample code in opal/samples/c_api/main.c for an example of how to do late binding.

    Late binding also allows for easier integration of OPAL fucntionality to
    interpreted languages such as Java, Perl etc. Systems like "swig" may be used to
    produce interface files for such languages.

    To make the above easier, there are only four functions: OpalInitialise(),
    OpalShutDown(), OpalGetMessage() and OpalSendMessage(). All commands to OPAL and
    indications back from OPAL are done through the latter two functions.

    This API also provides a basic C++ class OpalContext, which may be used for C++
    programmers that do not wish to learn the large number of classes in the full
    API. At the cost of minimal extensibility and control.
  */

#ifdef _WIN32
  #define OPAL_EXPORT __stdcall
#else
  #define OPAL_EXPORT
#endif

/// Handle to initialised OPAL instance.
typedef struct OpalHandleStruct * OpalHandle;


typedef struct OpalMessage OpalMessage;

/// Current API version
#define OPAL_C_API_VERSION 21


///////////////////////////////////////

/** Initialise the OPAL system, returning a "handle" to the system that must
    be used in other calls to OPAL.

    The version parameter indicates the version of the API being used by the
    caller. It should always be set to the constant OPAL_C_API_VERSION. On
    return the library will indicate the API version it supports, if it is
    lower than that provided by the application.

    The C string options are space separated tokens indicating various options
    to be enabled, for example the protocols to be available. NULL or an empty
    string will load all available protocols. The current protocol tokens are:
        <code>
        sip sips h323 h323s iax2 pc local pots pstn ivr
        </code>
    The above protocols are in priority order, so if a protocol is not
    explicitly in the address, then the first one of the opposite "category"
    s used. There are two categories, network protocols (sip, h323, iax & pstn)
    and non-network protocols (pc, local, pots & ivr).

    Additional options are:
        <table border=0>
        <tr><td>TraceLevel=1     <td>Level for tracing.
        <tr><td>TraceAppend      <td>Append to the trace file.
        <tr><td>TraceFile="name" <td>Set the filename for trace output. Note quotes are
                                     required if spaces are in filename.
        </table>
    It should also be noted that there must not be spaces around the '=' sign
    in the above options.

    If NULL is returned then an initialisation error occurred. This can only
    really occur if the user specifies prefixes which are not supported by
    the library.

    Example:
      <code>
      OpalHandle hOPAL;
      unsigned   version;

      version = OPAL_C_API_VERSION;
      if ((hOPAL = OpalInitialise(&version,
                                  OPAL_PREFIX_H323  " "
                                  OPAL_PREFIX_SIP   " "
                                  OPAL_PREFIX_IAX2  " "
                                  OPAL_PREFIX_PCSS
                                  " TraceLevel=4")) == NULL) {
        fputs("Could not initialise OPAL\n", stderr);
        return false;
      }
      </code>
  */
OpalHandle OPAL_EXPORT OpalInitialise(unsigned * version, const char * options);

/** String representation of the OpalIntialise() which may be used for late
    binding to the library.
 */
#define OPAL_INITIALISE_FUNCTION   "OpalInitialise"

/** Typedef representation of the pointer to the OpalIntialise() function which
    may be used for late binding to the library.
 */
typedef OpalHandle (OPAL_EXPORT *OpalInitialiseFunction)(unsigned * version, const char * options);


///////////////////////////////////////

/** Shut down and clean up all resource used by the OPAL system. The parameter
    must be the handle returned by OpalInitialise().

    Example:
      <code>
      OpalShutDown(hOPAL);
      </code>
  */
void OPAL_EXPORT OpalShutDown(OpalHandle opal);

/** String representation of the OpalShutDown() which may be used for late
    binding to the library.
 */
#define OPAL_SHUTDOWN_FUNCTION     "OpalShutDown"

/** Typedef representation of the pointer to the OpalShutDown() function which
    may be used for late binding to the library.
 */
typedef void (OPAL_EXPORT *OpalShutDownFunction)(OpalHandle opal);


///////////////////////////////////////

/** Get a message from the OPAL system. The first parameter must be the handle
    returned by OpalInitialise(). The second parameter is a timeout in
    milliseconds. NULL is returned if a timeout occurs. A value of UINT_MAX
    will wait forever for a message.

    The returned message must be disposed of by a call to OpalFreeMessage().

    The OPAL system will serialise all messages returned from this function to
    avoid any multi-threading issues. If the application wishes to avoid even
    this small delay, there is a callback function that may be configured that
    is not thread safe but may be used to get the messages as soon as they are
    generated. See OpalCmdSetGeneralParameters.

    Note if OpalShutDown() is called from a different thread then this function
    will break from its block and return NULL.

    Example:
      <code>
      OpalMessage * message;
        
      while ((message = OpalGetMessage(hOPAL, timeout)) != NULL) {
        switch (message->m_type) {
          case OpalIndRegistration :
            HandleRegistration(message);
            break;
          case OpalIndIncomingCall :
            Ring(message);
            break;
          case OpalIndCallCleared :
            HandleHangUp(message);
            break;
        }
        FreeMessageFunction(message);
      }
      </code>
  */
OpalMessage * OPAL_EXPORT OpalGetMessage(OpalHandle opal, unsigned timeout);

/** String representation of the OpalGetMessage() which may be used for late
    binding to the library.
 */
#define OPAL_GET_MESSAGE_FUNCTION  "OpalGetMessage"

/** Typedef representation of the pointer to the OpalGetMessage() function which
    may be used for late binding to the library.
 */
typedef OpalMessage * (OPAL_EXPORT *OpalGetMessageFunction)(OpalHandle opal, unsigned timeout);


///////////////////////////////////////

/** Send a message to the OPAL system. The first parameter must be the handle
    returned by OpalInitialise(). The second parameter is a constructed message
    which is a command to the OPAL system.

    Within the command message, generally a NULL or empty string, or zero value
    for integral types, indicates the particular parameter is to be ignored.
    Documentation on individiual messages will indicate which are mandatory.
    
    The return value is another message which will have a type of
    OpalIndCommandError if an error occurs. The OpalMessage::m_commandError field
    will contain a string indicating the error that occurred.

    If successful, the the type of the message is the same as the command type.
    The message fields in the return will generally be set to the previous value
    for the field, where relevant. For example in the OpalCmdSetGeneralParameters
    command the OpalParamGeneral::m_stunServer would contain the STUN server name
    prior to the command.

    A NULL is only returned if the either OpalHandle or OpalMessage parameters is NULL.

    The returned message must be disposed of by a call to OpalFreeMessage().

    Example:
      <code>
      void SendCommand(OpalMessage * command)
      {
        OpalMessage * response;
        if ((response = OpalSendMessage(hOPAL, command)) == NULL) {
          puts("OPAL not initialised.");
        else if (response->m_type != OpalIndCommandError)
          HandleResponse(response);
        else if (response->m_param.m_commandError == NULL || *response->m_param.m_commandError == '\\0')
          puts("OPAL error.");
        else
          printf("OPAL error: %s\n", response->m_param.m_commandError);

        FreeMessageFunction(response);
      }
      </code>
  */
OpalMessage * OPAL_EXPORT OpalSendMessage(OpalHandle opal, const OpalMessage * message);

/** String representation of the OpalSendMessage() which may be used for late
    binding to the library.
 */
typedef OpalMessage * (OPAL_EXPORT *OpalSendMessageFunction)(OpalHandle opal, const OpalMessage * message);

/** Typedef representation of the pointer to the OpalSendMessage() function which
    may be used for late binding to the library.
 */
#define OPAL_SEND_MESSAGE_FUNCTION "OpalSendMessage"


///////////////////////////////////////

/** Free memeory in message the OPAL system has sent. The parameter must be
    the message returned by OpalGetMessage() or OpalSendMessage().
  */
void OPAL_EXPORT OpalFreeMessage(OpalMessage * message);

/** String representation of the OpalFreeMessage() which may be used for late
    binding to the library.
 */
#define OPAL_FREE_MESSAGE_FUNCTION "OpalFreeMessage"

/** Typedef representation of the pointer to the OpalFreeMessage() function which
    may be used for late binding to the library.
 */
typedef void (OPAL_EXPORT *OpalFreeMessageFunction)(OpalMessage * message);


///////////////////////////////////////

#define OPAL_PREFIX_H323  "h323"    ///< H.323 Protocol supported string for OpalInitialise()
#define OPAL_PREFIX_SIP   "sip"     ///< SIP Protocol supported string for OpalInitialise()
#define OPAL_PREFIX_IAX2  "iax2"    ///< IAX2 Protocol supported string for OpalInitialise()
#define OPAL_PREFIX_PCSS  "pc"      ///< PC sound system supported string for OpalInitialise()
#define OPAL_PREFIX_LOCAL "local"   ///< Local endpoint supported string for OpalInitialise()
#define OPAL_PREFIX_POTS  "pots"    ///< Plain Old Telephone System supported string for OpalInitialise()
#define OPAL_PREFIX_PSTN  "pstn"    ///< Public Switched Network supported string for OpalInitialise()
#define OPAL_PREFIX_IVR   "ivr"     ///< Interactive Voice Response supported string for OpalInitialise()

#define OPAL_PREFIX_ALL OPAL_PREFIX_H323  " " \
                        OPAL_PREFIX_SIP   " " \
                        OPAL_PREFIX_IAX2  " " \
                        OPAL_PREFIX_PCSS  " " \
                        OPAL_PREFIX_LOCAL " " \
                        OPAL_PREFIX_POTS  " " \
                        OPAL_PREFIX_PSTN  " " \
                        OPAL_PREFIX_IVR


/**Type code for messages defined by OpalMessage.
  */
typedef enum OpalMessageType {
  OpalIndCommandError,          /**<An error occurred during a command. This is only returned by
                                    OpalSendMessage(). The details of the error are shown in the
                                    OpalMessage::m_commandError field. */
  OpalCmdSetGeneralParameters,  /**<Set general parameters command. This configures global settings in OPAL.
                                    See the OpalParamGeneral structure for more information. */
  OpalCmdSetProtocolParameters, /**<Set protocol parameters command. This configures settings in OPAL that
                                    may be different for each protocol, e.g. SIP & H.323. See the 
                                    OpalParamProtocol structure for more information. */
  OpalCmdRegistration,          /**<Register/Unregister command. This initiates a registration or
                                    unregistration operation with a protocol dependent server. Currently
                                    only for H.323 and SIP. See the OpalParamRegistration structure for more
                                    information. */
  OpalIndRegistration,          /**<Status of registration indication. After the OpalCmdRegistration has
                                    initiated a registration, this indication will be returned by the
                                    OpalGetMessage() function when the status of the registration changes,
                                    e.g. successful registration or communications failure etc. See the
                                    OpalStatusRegistration structure for more information. */
  OpalCmdSetUpCall,             /**<Set up a call command. This starts the outgoing call process. The
                                    OpalIndAlerting, OpalIndEstablished and OpalIndCallCleared messages are
                                    returned by OpalGetMessage() to indicate the call progress. See the 
                                    OpalParamSetUpCall structure for more information. */
  OpalIndIncomingCall,          /**<Incoming call indication. This is returned by the OpalGetMessage() function
                                    at any time after listeners are set up via the OpalCmdSetProtocolParameters
                                    command. See the OpalStatusIncomingCall structure for more information. */
  OpalCmdAnswerCall,            /**<Answer call command. After a OpalIndIncomingCall is returned by the
                                    OpalGetMessage() function, an application maye indicate that the call is
                                    to be answered with this message. The OpalMessage m_callToken field is
                                    set to the token returned in OpalIndIncomingCall. */
  OpalCmdClearCall,             /**<Hang Up call command. After a OpalCmdSetUpCall command is executed or a
                                    OpalIndIncomingCall indication is received then this may be used to
                                    "hang up" the call. The OpalIndCallCleared is subsequently returned in
                                    the OpalGetMessage() when the call has completed its hang up operation.
                                    See OpalParamCallCleared structure for more information.*/
  OpalIndAlerting,              /**<Remote is alerting indication. This message is returned in the
                                    OpalGetMessage() function when the underlying protocol states the remote
                                    telephone is "ringing". See the OpalParamSetUpCall structure for more
                                    information. */
  OpalIndEstablished,           /**<Call is established indication. This message is returned in the
                                    OpalGetMessage() function when the remote or local endpont has "answered"
                                    the call and there is media flowing. See the  OpalParamSetUpCall
                                    structure for more information. */
  OpalIndUserInput,             /**<User input indication. This message is returned in the OpalGetMessage()
                                    function when, during a call, user indications (aka DTMF tones) are
                                    received. See the OpalStatusUserInput structure for more information. */
  OpalIndCallCleared,           /**<Call is cleared indication. This message is returned in the
                                    OpalGetMessage() function when the call has completed. The OpalMessage
                                    m_callToken field indicates which call cleared. */
  OpalCmdHoldCall,              /**<Place call in a hold state. The OpalMessage m_callToken field is set to
                                    the token returned in OpalIndIncomingCall. */
  OpalCmdRetrieveCall,          /**<Retrieve call from hold state. The OpalMessage m_callToken field is set
                                    to the token returned in OpalIndIncomingCall. */
  OpalCmdTransferCall,          /**<Transfer a call to another party. This starts the outgoing call process
                                    for the other party. See the  OpalParamSetUpCall structure for more
                                    information.*/
  OpalCmdUserInput,             /**<User input command. This sends specified user input to the remote
                                    connection. See the OpalStatusUserInput structure for more information. */
  OpalIndMessageWaiting,        /**<Message Waiting indication. This message is returned in the
                                    OpalGetMessage() function when an MWI is received on any of the supported
                                    protocols. */
  OpalIndMediaStream,           /**<A media stream has started/stopped. This message is returned in the
                                    OpalGetMessage() function when a media stream is started or stopped. See the
                                    OpalStatusMediaStream structure for more information. */
  OpalCmdMediaStream,           /**<Execute control on a media stream. See the OpalStatusMediaStream structure
                                    for more information. */
  OpalCmdSetUserData,           /**<Set the user data field associated with a call */
  OpalIndLineAppearance,        /**<Line Appearance indication. This message is returned in the
                                    OpalGetMessage() function when any of the supported protocols indicate that
                                    the state of a "line" has changed, e.g. free, busy, on hold etc. */
  OpalCmdStartRecording,        /**<Start recording an active call. See the OpalParamRecording structure
                                    for more information. */
  OpalCmdStopRecording,         /**<Stop recording an active call. Only the m_callToken field of the
                                    OpalMessage union is used. */
  OpalIndProceeding,            /**<Call has been accepted by remote. This message is returned in the
                                    OpalGetMessage() function when the underlying protocol states the remote
                                    endpoint acknowledged that it will route the call. This is distinct from
                                    OpalIndAlerting in that it is not known at this time if anything is
                                    ringing. This indication may be used to distinguish between "transport"
                                    level error, in which case another host may be tried, and that the
                                    responsibility for finalising the call has moved "upstream". See the
                                    OpalParamSetUpCall structure for more information. */
  OpalCmdAlerting,              /**<Send an indication to the remote that we are "ringing". The OpalMessage
                                    m_callToken field indicates which call is alerting.  */
  OpalMessageTypeCount
} OpalMessageType;


/**Type code the silence detect algorithm modes.
   This is used by the OpalCmdSetGeneralParameters command in the OpalParamGeneral structure.
  */
typedef enum OpalSilenceDetectMode {
  OpalSilenceDetectNoChange,  /**< No change to the silence detect mode. */
  OpalSilenceDetectDisabled,  /**< Indicate silence detect is disabled */
  OpalSilenceDetectFixed,     /**< Indicate silence detect uses a fixed threshold */
  OpalSilenceDetectAdaptive   /**< Indicate silence detect uses an adaptive threashold */
} OpalSilenceDetectMode;


/**Type code the echo cancellation algorithm modes.
   This is used by the OpalCmdSetGeneralParameters command in the OpalParamGeneral structure.
  */
typedef enum OpalEchoCancelMode {
  OpalEchoCancelNoChange,   /**< No change to the echo cancellation mode. */
  OpalEchoCancelDisabled,   /**< Indicate the echo cancellation is disabled */
  OpalEchoCancelEnabled     /**< Indicate the echo cancellation is enabled */
} OpalEchoCancelMode;


/** Function for reading/writing media data.
    Returns size of data actually read or written, or -1 if there is an error
    and the media stream should be shut down.

    The "write" function, which is taking data from a remote and providing it
    to the "C" application for writing, should not be assumed to have a one to
    one correspondence with RTP packets. The OPAL jiter buffer may insert
    "silence" data for missing or too late packets. In this case the function
    is called with the size parameter equal to zero. It is up to the
    application what it does in that circumstance.

    Note that this function will be called in the context of different threads
    so the user must take care of any mutex and synchonisation issues.
 */
typedef int (*OpalMediaDataFunction)(
  const char * token,   /**< Call token for media data as returned by OpalIndIncomingCall.
                             This may be used to discriminate between individiual calls. */
  const char * stream,  /**< Stream identifier for media data. This may be used to
                             discriminate between media streams within a call, applicable
                             if there can be more than one stream of a particular format,
                             e.g. two H.263 video channels. */
  const char * format,  /**< Format of media data, e.g. "PCM-16" */
  void * userData,      /**< user data associated with the call */
  void * data,          /**< Data to read/write */
  int size              /**< Maximum size of data to read, or size of actual data to write */
);


/** Function called when a message event becomes available.
    This function is called before the message is queued for the GetMessage()
    function.

    A return value of zero indicates that the message is not to be passed on
    to the GetMessage(). A non-zero value will pass the message on.

    Note that this function will be called in the context of different threads
    so the user must take care of any mutex and synchonisation issues. If the
    user subsequently uses the GetMessage() then the message will have been
    serialised so that there are no multi-threading issues.

    A simple use case would be for this function to send a signal or message
    to the applications main thread and then return a non-zero value. The
    main thread would then wake up and get the message using GetMessage.
 */
typedef int (*OpalMessageAvailableFunction)(
  const OpalMessage * message  /**< Message that has become available. */
);


/**Type code the media data call back functions data type.
   This is used by the OpalCmdSetGeneralParameters command in the
   OpalParamGeneral structure.

   This controls if the whole RTP data frame or just the paylaod part
   is passed to the read/write function.
  */
typedef enum OpalMediaDataType {
  OpalMediaDataNoChange,      /**< No change to the media data type. */
  OpalMediaDataPayloadOnly,   /**< Indicate only the RTP payload is passed to the
                                   read/write function */
  OpalMediaDataWithHeader     /**< Indicate the whole RTP frame including header is
                                   passed to the read/write function */
} OpalMediaDataType;


/**Timing mode for the media data call back functions data type.
   This is used by the OpalCmdSetGeneralParameters command in the
   OpalParamGeneral structure.

   This controls if the read/write function is in control of the real time
   aspects of the media flow. If synchronous then the read/write function
   is expected to handle the real time "pacing" of the read or written data.

   Note this is important both for reads and writes. For example in
   synchronous mode you cannot simply read from a file and send, or you will
   likely overrun the remotes buffers. Similarly for writing to a file, the
   correct operation of the OPAL jitter buffer is dependent on it not being
   drained too fast by the "write" function.

   If marked as asynchroous then the OPAL stack itself will take care of the
   timing and things like read/write to a disk file will work correctly.
  */
typedef enum OpalMediaTiming {
  OpalMediaTimingNoChange,      /**< No change to the media data type. */
  OpalMediaTimingSynchronous,   /**< Indicate the read/write function is going to handle
                                     all real time aspects of the media flow. */
  OpalMediaTimingAsynchronous   /**< Indicate the read/write function does not handle
                                     the real time aspects of the media flow. */
} OpalMediaTiming;


/**General parameters for the OpalCmdSetGeneralParameters command.
   This is only passed to and returned from the OpalSendMessage() function.

   Example:
      <code>
      OpalMessage   command;
      OpalMessage * response;

      memset(&command, 0, sizeof(command));
      command.m_type = OpalCmdSetGeneralParameters;
      command.m_param.m_general.m_stunServer = "stun.voxgratia.org";
      command.m_param.m_general.m_mediaMask = "RFC4175*";
      response = OpalSendMessage(hOPAL, &command);
      </code>
  */
typedef struct OpalParamGeneral {
  const char * m_audioRecordDevice;   /**< Audio recording device name */
  const char * m_audioPlayerDevice;   /**< Audio playback device name */
  const char * m_videoInputDevice;    /**< Video input (e.g. camera) device name */
  const char * m_videoOutputDevice;   /**< Video output (e.g. window) device name */
  const char * m_videoPreviewDevice;  /**< Video preview (e.g. window) device name */
  const char * m_mediaOrder;          /**< List of media format names to set the preference order for media.
                                           This list of names (e.g. "G.723.1") is separated by the '\n'
                                           character. */
  const char * m_mediaMask;           /**< List of media format names to set media to be excluded.
                                           This list of names (e.g. "G.723.1") is separated by the '\n'
                                           character. */
  const char * m_autoRxMedia;         /**< List of media types (e.g. audio, video) separated by spaces
                                           which may automatically be received automatically. */
  const char * m_autoTxMedia;         /**< List of media types (e.g. audio, video) separated by spaces
                                           which may automatically be transmitted automatically. */
  const char * m_natRouter;           /**< The host name or IP address of the Network Address Translation
                                           router which may be between the endpoint and the Internet. */
  const char * m_stunServer;          /**< The host name or IP address of the STUN server which may be
                                           used to determine the NAT router characteristics automatically. */
  unsigned     m_tcpPortBase;         /**< Base of range of ports to use for TCP communications. This may
                                           be required by some firewalls. */
  unsigned     m_tcpPortMax;          /**< Max of range of ports to use for TCP communications. This may
                                           be required by some firewalls. */
  unsigned     m_udpPortBase;         /**< Base of range of ports to use for UDP communications. This may
                                           be required by some firewalls. */
  unsigned     m_udpPortMax;          /**< Max of range of ports to use for UDP communications. This may
                                           be required by some firewalls. */
  unsigned     m_rtpPortBase;         /**< Base of range of ports to use for RTP/UDP communications. This may
                                           be required by some firewalls. */
  unsigned     m_rtpPortMax;          /**< Max of range of ports to use for RTP/UDP communications. This may
                                           be required by some firewalls. */
  unsigned     m_rtpTypeOfService;    /**< Value for the Type Of Service byte with UDP/IP packets which may
                                           be used by some routers for simple Quality of Service control. */
  unsigned     m_rtpMaxPayloadSize;   /**< Maximum payload size for RTP packets. This may sometimes need to
                                           be set according to the MTU or the underlying network. */
  unsigned     m_minAudioJitter;      /**< Minimum jitter time in milliseconds. For audio RTP data being
                                           received this sets the minimum time of the adaptive jitter buffer
                                           which smooths out irregularities in the transmission of audio
                                           data over the Internet. */
  unsigned     m_maxAudioJitter;      /**< Maximum jitter time in milliseconds. For audio RTP data being
                                           received this sets the maximum time of the adaptive jitter buffer
                                           which smooths out irregularities in the transmission of audio
                                           data over the Internet. */
  OpalSilenceDetectMode m_silenceDetectMode; /**< Silence detection mode. This controls the silence
                                           detection algorithm for audio transmission: 0=no change,
                                           1=disabled, 2=fixed, 3=adaptive. */
  unsigned     m_silenceThreshold;    /**< Silence detection threshold value. This applies if
                                           m_silenceDetectMode is fixed (2) and is a PCM-16 value. */
  unsigned     m_signalDeadband;      /**< Time signal is required before audio is transmitted. This is
                                           is RTP timestamp units (8000Hz). */
  unsigned     m_silenceDeadband;     /**< Time silence is required before audio is transmission is stopped.
                                           This is is RTP timestamp units (8000Hz). */
  unsigned     m_silenceAdaptPeriod;  /**< Window for adapting the silence threashold. This applies if
                                           m_silenceDetectMode is adaptive (3). This is is RTP timestamp
                                           units (8000Hz). */
  OpalEchoCancelMode m_echoCancellation; /**< Accoustic Echo Cancellation control. 0=no change, 1=disabled,
                                              2=enabled. */
  unsigned     m_audioBuffers;        /**< Set the number of hardware sound buffers to use.
                                           Note the largest of m_audioBuffers and m_audioBufferTime/frametime
                                           will be used. */
  OpalMediaDataFunction m_mediaReadData;   /**< Callback function for reading raw media data. See
                                                OpalMediaDataFunction for more information. */
  OpalMediaDataFunction m_mediaWriteData;  /**< Callback function for writing raw media data. See
                                                OpalMediaDataFunction for more information.  */
  OpalMediaDataType     m_mediaDataHeader; /**< Indicate that the media read/write callback function
                                           is passed the full RTP header or just the payload.
                                           0=no change, 1=payload only, 2=with RTP header. */
  OpalMessageAvailableFunction m_messageAvailable; /**< If non-null then this function is called before
                                                        the message is queued for return in the
                                                        GetMessage(). See the
                                                        OpalMessageAvailableFunction for more details. */
  const char * m_mediaOptions;        /**< List of media format options to be set. This is a '\n' separated
                                           list of entries of the form "codec:option=value". Codec is either
                                           a media type (e.g. "Audio" or "Video") or a specific media format,
                                           for example:
                                             <code>
                                             "G.723.1:Tx Frames Per Packet=2\nH.263:Annex T=0\n"
                                             "Video:Max Rx Frame Width=176\nVideo:Max Rx Frame Height=144"
                                             </code>
                                           */
  unsigned     m_audioBufferTime;     /**< Set the hardware sound buffers to use in milliseconds.
                                           Note the largest of m_audioBuffers and m_audioBufferTime/frametime
                                           will be used. */
  unsigned m_manualAlerting;          /**< Indicate that an "alerting" message is automatically (value=1)
                                           or manually (value=2) sent to the remote on receipt of an
                                           OpalIndIncomingCall message. If set to manual then it is up
                                           to the application to send a OpalCmdAlerting message to
                                           indicate to the remote system that we are "ringing".
                                           If zero then no change is made. */
  OpalMediaTiming m_mediaTiming;      /**< Indicate that the media read/write callback function
                                           handles the real time aspects of the media flow.
                                           0=no change, 1=synchronous, 2=asynchronous. */
} OpalParamGeneral;


/**Product description variables.
  */
typedef struct OpalProductDescription {
  const char * m_vendor;              /**< Name of the vendor or manufacturer of the application. This is
                                           used to identify the software which can be very useful for
                                           solving interoperability issues. e.g. "Vox Lucida". */
  const char * m_name;                /**< Name of the product within the vendor name space. This is
                                           used to identify the software which can be very useful for
                                           solving interoperability issues. e.g. "OpenPhone". */
  const char * m_version;             /**< Version of the product within the vendor/product name space. This
                                           is used to identify the software which can be very useful for
                                           solving interoperability issues. e.g. "2.1.4". */
  unsigned     m_t35CountryCode;      /**< T.35 country code for the name space in which the vendor or
                                           manufacturer is identified. This is the part of the H.221
                                           equivalent of the m_vendor string above and  used to identify the
                                           software which can be very useful for solving interoperability
                                           issues. e.g. 9 is for Australia. */
  unsigned     m_t35Extension;        /**< T.35 country extension code for the name space in which the vendor or
                                           manufacturer is identified. This is the part of the H.221
                                           equivalent of the m_vendor string above and  used to identify the
                                           software which can be very useful for solving interoperability
                                           issues. Very rarely used. */
  unsigned     m_manufacturerCode;    /**< Manuacturer code for the name space in which the vendor or
                                           manufacturer is identified. This is the part of the H.221
                                           equivalent of the m_vendor string above and  used to identify the
                                           software which can be very useful for solving interoperability
                                           issues. e.g. 61 is for Equivalance and was allocated by the
                                           Australian Communications Authority, Oct 2000. */
} OpalProductDescription;


/**Protocol parameters for the OpalCmdSetProtocolParameters command.
   This is only passed to and returned from the OpalSendMessage() function.

   Example:
      <code>
      OpalMessage   command;
      OpalMessage * response;

      memset(&command, 0, sizeof(command));
      command.m_type = OpalCmdSetProtocolParameters;
      command.m_param.m_protocol.m_userName = "robertj";
      command.m_param.m_protocol.m_displayName = "Robert Jongbloed";
      command.m_param.m_protocol.m_interfaceAddresses = "*";
      response = OpalSendMessage(hOPAL, &command);
      </code>
  */
typedef struct OpalParamProtocol {
  const char * m_prefix;              /**< Protocol prefix for parameters, e.g. "h323" or "sip". If this is
                                           NULL or empty string, then the parameters are set for all protocols
                                           where they maybe set. */
  const char * m_userName;            /**< User name to identify the endpoint. This is usually the protocol
                                           specific name and may interact with the OpalCmdRegistration
                                           command. e.g. "robertj or 61295552148 */
  const char * m_displayName;         /**< Display name to be used. This is the human readable form of the
                                           users name, e.g. "Robert Jongbloed". */
  OpalProductDescription m_product;   /**< Product description data */
  const char * m_interfaceAddresses;  /**< A list of interfaces to start listening for incoming calls.
                                           This list is separated by the '\n' character. If NULL no
                                           listeners are started or stopped. If and empty string ("")
                                           then all listeners are stopped. If a "*" then listeners
                                           are started for all interfaces in the system. */
} OpalParamProtocol;


/// Name of SIP event package for Message Waiting events.
#define OPAL_MWI_EVENT_PACKAGE             "message-summary"

/// Name of SIP even package fo rmonitoring call status
#define OPAL_LINE_APPEARANCE_EVENT_PACKAGE "dialog;sla;ma"

/**Registration parameters for the OpalCmdRegistration command.
   This is only passed to and returned from the OpalSendMessage() function.

   Example:
      <code>
      OpalMessage   command;
      OpalMessage * response;

      // H.323 register with gatekeeper
      memset(&command, 0, sizeof(command));
      command.m_type = OpalCmdRegistration;
      command.m_param.m_registrationInfo.m_protocol = "h323";
      command.m_param.m_registrationInfo.m_identifier = "31415";
      command.m_param.m_registrationInfo.m_hostName = gk.voxgratia.org;
      command.m_param.m_registrationInfo.m_password = "secret";
      command.m_param.m_registrationInfo.m_timeToLive = 300;
      response = OpalSendMessage(hOPAL, &command);
      if (response != NULL && response->m_type == OpalCmdRegistration)
        m_AddressOfRecord = response->m_param.m_registrationInfo.m_identifier

      // SIP register with regstrar/proxy
      memset(&command, 0, sizeof(command));
      command.m_type = OpalCmdRegistration;
      command.m_param.m_registrationInfo.m_protocol = "sip";
      command.m_param.m_registrationInfo.m_identifier = "rjongbloed@ekiga.net";
      command.m_param.m_registrationInfo.m_password = "secret";
      command.m_param.m_registrationInfo.m_timeToLive = 300;
      response = OpalSendMessage(hOPAL, &command);
      if (response != NULL && response->m_type == OpalCmdRegistration)
        m_AddressOfRecord = response->m_param.m_registrationInfo.m_identifier

      // unREGISTER
      memset(&command, 0, sizeof(command));
      command.m_type = OpalCmdRegistration;
      command.m_param.m_registrationInfo.m_protocol = "sip";
      command.m_param.m_registrationInfo.m_identifier = m_AddressOfRecord;
      command.m_param.m_registrationInfo.m_timeToLive = 0;
      response = OpalSendMessage(hOPAL, &command);

      // Set event package so do SUBSCRIBE
      memset(&command, 0, sizeof(command));
      command.m_type = OpalCmdRegistration;
      command.m_param.m_registrationInfo.m_protocol = "sip";
      command.m_param.m_registrationInfo.m_identifier = "2012@pbx.local";
      command.m_param.m_registrationInfo.m_hostName = "sa@pbx.local";
      command.m_param.m_registrationInfo.m_eventPackage = OPAL_LINE_APPEARANCE_EVENT_PACKAGE;
      command.m_param.m_registrationInfo.m_timeToLive = 300;
      response = OpalSendMessage(hOPAL, &command);
      if (response != NULL && response->m_type == OpalCmdRegistration)
        m_AddressOfRecord = response->m_param.m_registrationInfo.m_identifier

      // unSUBSCRIBE
      memset(&command, 0, sizeof(command));
      command.m_type = OpalCmdRegistration;
      command.m_param.m_registrationInfo.m_protocol = "sip";
      command.m_param.m_registrationInfo.m_identifier = m_AddressOfRecord;
      command.m_param.m_registrationInfo.m_eventPackage = OPAL_LINE_APPEARANCE_EVENT_PACKAGE;
      command.m_param.m_registrationInfo.m_timeToLive = 0;
      response = OpalSendMessage(hOPAL, &command);
      </code>
  */
typedef struct OpalParamRegistration {
  const char * m_protocol;      /**< Protocol prefix for registration. Currently must be "h323" or
                                     "sip", cannot be NULL. */
  const char * m_identifier;    /**< Identifier for name to be registered at server. If NULL
                                     or empty then the value provided in the OpalParamProtocol::m_userName
                                     field of the OpalCmdSetProtocolParameters command is used. Note
                                     that for SIP the default value will have "@" and the
                                     OpalParamRegistration::m_hostName field apepnded to it to create
                                     and Address-Of_Record. */
  const char * m_hostName;      /**< Host or domain name for server. For SIP this cannot be NULL.
                                     For H.323 a NULL value indicates that a broadcast discovery is
                                     be performed. If, for SIP, this contains an "@" and a user part
                                     then a "third party" registration is performed. */
  const char * m_authUserName;  /**< User name for authentication. */
  const char * m_password;      ///< Password for authentication with server.
  const char * m_adminEntity;   /**< Identification of the administrative entity. For H.323 this will
                                     by the gatekeeper identifier. For SIP this is the authentication
                                     realm. */
  unsigned     m_timeToLive;    /**< Time in seconds between registration updates. If this is zero then
                                     the identifier is unregistered from the server. */
  unsigned     m_restoreTime;   /**< Time in seconds between attempts to restore a registration after
                                     registrar/gatekeeper has gone offline. If zero then a default
                                     value is used. */
  const char * m_eventPackage;  /**< If non-NULL then this indicates that a subscription is made
                                     rather than a registration. The string represents the particular
                                     event package being subscribed too.
                                     A value of OPAL_MWI_EVENT_PACKAGE will cause an
                                     OpalIndMessageWaiting to be sent.
                                     A value of OPAL_LINE_APPEARANCE_EVENT_PACKAGE will cause the
                                     OpalIndLineAppearance to be sent.
                                     Other values are currently not supported. */
} OpalParamRegistration;


/**Type code for media stream status/control.
   This is used by the OpalIndRegistration indication in the OpalStatusRegistration structure.
  */
typedef enum OpalRegistrationStates {
  OpalRegisterSuccessful,   /**< Successfully registered. */
  OpalRegisterRemoved,      /**< Successfully unregistered. Note that the m_error field may be
                                 non-null if an error occurred during unregistration, however
                                 the unregistration will "complete" as far as the local endpoint
                                 is concerned and no more registration retries are made. */
  OpalRegisterFailed,       /**< Registration has failed. The m_error field of the
                                 OpalStatusRegistration structure will contain more details. */
  OpalRegisterRetrying,     /**< Registrar/Gatekeeper has gone offline and a failed retry
                                 has been executed. */
  OpalRegisterRestored,     /**< Registration has been restored after a succesfull retry. */
} OpalRegistrationStates;


/**Registration status for the OpalIndRegistration indication.
   This is only returned from the OpalGetMessage() function.
  */
typedef struct OpalStatusRegistration {
  const char * m_protocol;    /**< Protocol prefix for registration. Currently must be "h323" or
                                   "sip", is never NULL. */
  const char * m_serverName;  /**< Nmae of the registration server. The exact format is protocol
                                   specific but generally contains the host or domain name, e.g.
                                   "GkId@gatekeeper.voxgratia.org" or "sip.voxgratia.org". */
  const char * m_error;       /**< Error message for registration. If any error in the initial
                                   registration or any subsequent registration update occurs, then
                                   this contains a string indicating the type of error. If no
                                   error occured then this will be NULL. */
  OpalRegistrationStates m_status; /**< Status of registration, see enum for details. */
  OpalProductDescription m_product; /**< Product description data */
} OpalStatusRegistration;


/**Set up call parameters for several command and indication messages.

   When establishing a new call via the OpalCmdSetUpCall command, the m_partyA and
   m_partyB fields indicate the parties to connect.

   For OpalCmdTransferCall, m_partyA indicates the connection to be transferred and
   m_partyB is the party to be transferred to. If the call transfer is successful then
   a OpalIndCallCleared message will be received clearing the local call.

   For OpalIndAlerting and OpalIndEstablished indications the three fields are set
   to the data for the call in progress.

   Example:
      <code>
      OpalMessage   command;
      OpalMessage * response;

      memset(&command, 0, sizeof(command));
      command.m_type = OpalCmdSetUpCall;
      command.m_param.m_callSetUp.m_partyB = "h323:10.0.1.11";
      response = OpalSendMessage(hOPAL, &command);

      memset(&command, 0, sizeof(command));
      command.m_type = OpalCmdSetUpCall;
      command.m_param.m_callSetUp.m_partyA = "pots:LINE1";
      command.m_param.m_callSetUp.m_partyB = "sip:10.0.1.11";
      response = OpalSendMessage(hOPAL, &command);
      callToken = strdup(response->m_param.m_callSetUp.m_callToken);

      memset(&command, 0, sizeof(command));
      command.m_type = OpalCmdTransferCall;
      command.m_param.m_callSetUp.m_callToken = callToken;
      command.m_param.m_callSetUp.m_partyB = "sip:10.0.1.12";
      response = OpalSendMessage(hOPAL, &command);
      </code>
  */
typedef struct OpalParamSetUpCall {
  const char * m_partyA;      /**< A-Party for call.

                                   For OpalCmdSetUpCall, this indicates what subsystem will
                                   be starting the call, e.g. "pots:Handset One". If NULL
                                   or empty string then "pc:*" is used indication that the
                                   standard PC sound system ans screen is to be used.

                                   For OpalCmdTransferCall this indicates the party to be
                                   transferred, e.g. "sip:fred@nurk.com". If NULL then
                                   it is assumed that the party to be transferred is of the
                                   same "protocol" as the m_partyB field, e.g. "pc" or "sip".

                                   For OpalIndAlerting and OpalIndEstablished this indicates
                                   the A-party of the call in progress. */
  const char * m_partyB;      /**< B-Party for call. This is typically a remote host URL
                                   address with protocol, e.g. "h323:simple.com" or
                                   "sip:fred@nurk.com".

                                   This must be provided in the OpalCmdSetUpCall and
                                   OpalCmdTransferCall commands, and is set by the system
                                   in the OpalIndAlerting and OpalIndEstablished indications.

                                   If used in the OpalCmdTransferCall command, this may be
                                   a valid call token for another call on hold. The remote
                                   is transferred to the call on hold and both calls are
                                   then cleared. */
  const char * m_callToken;   /**< Value of call token for new call. The user would pass NULL
                                   for this string in OpalCmdSetUpCall, a new value is
                                   returned by the OpalSendMessage() function. The user would
                                   provide the call token for the call being transferred when
                                   OpalCmdTransferCall is being called. */
  const char * m_alertingType;/**< The type of "distinctive ringing" for the call. The string
                                   is protocol dependent, so the caller would need to be aware
                                   of the type of call being made. Some protocols may ignore
                                   the field completely.

                                   For SIP this corresponds to the string contained in the
                                   "Alert-Info" header field of the INVITE. This is typically
                                   a URI for the ring file.

                                   For H.323 this must be a string representation of an
                                   integer from 0 to 7 which will be contained in the
                                   Q.931 SIGNAL (0x34) Information Element.

                                   This is only used in OpalCmdSetUpCall to set the string to
                                   be sent to the remote to change the type of ring the remote
                                   may emit.

                                   For other indications this field is NULL. */

  const char * m_protocolCallId;  /**< ID assigned by the underlying protocol for the call. 
                                       Only available in version 18 and above */

} OpalParamSetUpCall;


/**Incoming call information for the OpalIndIncomingCall indication.
   This is only returned from the OpalGetMessage() function.
  */
typedef struct OpalStatusIncomingCall {
  const char * m_callToken;         ///< Call token for new call.
  const char * m_localAddress;      ///< URL of local interface. e.g. "sip:me@here.com"
  const char * m_remoteAddress;     ///< URL of calling party. e.g. "sip:them@there.com"
  const char * m_remotePartyNumber; ///< This is the E.164 number of the caller, if available.
  const char * m_remoteDisplayName; ///< Display name calling party. e.g. "Fred Nurk"
  const char * m_calledAddress;     ///< URL of called party the remote is trying to contact.
  const char * m_calledPartyNumber; ///< This is the E.164 number of the called party, if available.
  OpalProductDescription m_product; /**< Product description data */
  const char * m_alertingType;/**< The type of "distinctive ringing" for the call. The string
                                   is protocol dependent, so the caller would need to be aware
                                   of the type of call being made. Some protocols may ignore
                                   the field completely.

                                   For SIP this corresponds to the string contained in the
                                   "Alert-Info" header field of the INVITE. This is typically
                                   a URI for the ring file.

                                   For H.323 this must be a string representation of an
                                   integer from 0 to 7 which will be contained in the
                                   Q.931 SIGNAL (0x34) Information Element. */
  const char * m_protocolCallId;  /**< ID assigned by the underlying protocol for the call. 
                                       Only available in version 18 and above */
} OpalStatusIncomingCall;


/**Type code for media stream status/control.
   This is used by the OpalIndMediaStream indication and OpalCmdMediaStream command
   in the OpalStatusMediaStream structure.
  */
typedef enum OpalMediaStates {
  OpalMediaStateNoChange,   /**< No change to the media stream state. */
  OpalMediaStateOpen,       /**< Media stream has been opened when indication,
                                 or is to be opened when a command. */
  OpalMediaStateClose,      /**< Media stream has been closed when indication,
                                 or is to be closed when a command. */
  OpalMediaStatePause,      /**< Media stream has been paused when indication,
                                 or is to be paused when a command. */
  OpalMediaStateResume      /**< Media stream has been paused when indication,
                                 or is to be paused when a command. */
} OpalMediaStates;


/**Media stream information for the OpalIndMediaStream indication and
   OpalCmdMediaStream command.

   This is may be returned from the OpalGetMessage() function or
   provided to the OpalSendMessage() function.
  */
typedef struct OpalStatusMediaStream {
  const char    * m_callToken;   ///< Call token for the call the media stream is.
  const char    * m_identifier;  /**< Unique identifier for the media stream. For OpalCmdMediaStream
                                      this may be left empty and the first stream of the type
                                      indicated by m_mediaType is used. */
  const char    * m_type;        /**< Media type and direction for the stream. This is a keyword such
                                      as "audio" or "video" indicating the type of the stream, a space,
                                      then either "in" or "out" indicating the direction. For
                                      OpalCmdMediaStream this may be left empty if m_identifier is
                                      used. */
  const char    * m_format;      /**< Media format for the stream. For OpalIndMediaStream this
                                      shows the format being used. For OpalCmdMediaStream this
                                      is the format to use. In the latter case, if empty or
                                      NULL, then a default is used. */
  OpalMediaStates m_state;       /**< For OpalIndMediaStream this indicates the status of the stream.
                                      For OpalCmdMediaStream this indicates the state to move to, see
                                      OpalMediaStates for more information. */
} OpalStatusMediaStream;


/** Assign a user data field to a call
*/
typedef struct OpalParamSetUserData {
  const char    * m_callToken;   ///< Call token for the call the media stream is.
  void *        m_userData;      ///< user data value to associate with this call
} OpalParamSetUserData;


/**User input information for the OpalIndUserInput/OpalCmdUserInput indication.

   This is may be returned from the OpalGetMessage() function or
   provided to the OpalSendMessage() function.
  */
typedef struct OpalStatusUserInput {
  const char * m_callToken;   ///< Call token for the call the User Input was received on.
  const char * m_userInput;   ///< User input string, e.g. "#".
  unsigned     m_duration;    /**< Duration in milliseconds for tone. For DTMF style user input
                                   the time the tone was detected may be placed in this field.
                                   Generally zero is passed which means the m_userInput is a
                                   single "string" input. If non-zero then m_userInput must
                                   be a single character. */
} OpalStatusUserInput, OpalParamUserInput;


/**Message Waiting information for the OpalIndMessageWaiting indication.
   This is only returned from the OpalGetMessage() function.
  */
typedef struct OpalStatusMessageWaiting {
  const char * m_party;     ///< Party for which the MWI is directed
  const char * m_type;      ///< Type for MWI, "Voice", "Fax", "Pager", "Multimedia", "Text", "None"
  const char * m_extraInfo; /**< Extra information for the MWI, e.g. "SUBSCRIBED",
                                 "UNSUBSCRIBED", "2/8 (0/2)"
                             */
} OpalStatusMessageWaiting;


/**Type code for media stream status/control.
   This is used by the OpalIndMediaStream indication and OpalCmdMediaStream command
   in the OpalStatusMediaStream structure.
  */
typedef enum OpalLineAppearanceStates {
  OpalLineTerminated,  /**< Line has ended a call. */
  OpalLineTrying,      /**< Line has been siezed. */
  OpalLineProceeding,  /**< Line is trying to make a call. */
  OpalLineRinging,     /**< Line is ringing. */
  OpalLineConnected,   /**< Line is connected. */
  OpalLineSubcribed,   /**< Line appearance subscription successful. */
  OpalLineUnsubcribed, /**< Line appearance unsubscription successful. */

  OpalLineIdle = OpalLineTerminated // Kept for backward compatibility
} OpalLineAppearanceStates;


/**Line Appearance information for the OpalIndLineAppearance indication.
   This is only returned from the OpalGetMessage() function.
  */
typedef struct OpalStatusLineAppearance {
  const char *             m_line;       ///< URI for the line whose state is changing
  OpalLineAppearanceStates m_state;      ///< State the line has just moved to.
  int                      m_appearance; /**< Appearance code, this is an arbitrary integer
                                              and is defined by the remote servers. If negative
                                              then it is undefined. */
  const char *             m_callId;     /**< If line is "in use" then this gives information
                                              that identifies the call. Note that this will
                                              include the from/to "tags" that can identify
                                              the dialog for REFER/Replace. */
  const char *             m_partyA;     /**< A-Party for call. */
  const char *             m_partyB;     /**< B-Party for call. */
} OpalStatusLineAppearance;


/**Type of mixing for video when recording.
   This is used by the OpalCmdStartRecording command in the OpalParamRecording structure.
  */
typedef enum OpalVideoRecordMixMode {
  OpalSideBySideLetterbox, /**< Two images side by side with black bars top and bottom.
                                It is expected that the input frames and output are all
                                the same aspect ratio, e.g. 4:3. Works well if inputs
                                are QCIF and output is CIF for example. */
  OpalSideBySideScaled,    /**< Two images side by side, scaled to fit halves of output
                                frame. It is expected that the output frame be double
                                the width of the input data to maintain aspect ratio.
                                e.g. for CIF inputs, output would be 704x288. */
  OpalStackedPillarbox,    /**< Two images, one on top of the other with black bars down
                                the sides. It is expected that the input frames and output
                                are all the same aspect ratio, e.g. 4:3. Works well if
                                inputs are QCIF and output is CIF for example. */
  OpalStackedScaled,       /**< Two images, one on top of the other, scaled to fit halves
                                of output frame. It is expected that the output frame be
                                double the height of the input data to maintain aspect
                                ratio. e.g. for CIF inputs, output would be 352x576. */
} OpalVideoRecordMixMode;


/**Call recording information for the OpalCmdStartRecording command.
  */
typedef struct OpalParamRecording {
  const char * m_callToken;  ///< Call token for call being cleared.
  const char * m_file;       /**< File to record into. If NULL then a test is done
                                  for if recording is currently active. */
  unsigned     m_channels;   /**< Number of channels in WAV file, 1 for mono (default) or 2 for
                                  stereo where incoming & outgoing audio are in individual
                                  channels. */
  const char * m_audioFormat; /**< Audio recording format. This is generally an OpalMediaFormat
                                   name which will be used in the recording file. The exact
                                   values possible is dependent on many factors including the
                                   specific file type and what codecs are loaded as plug ins. */
  const char * m_videoFormat; /**< Video recording format. This is generally an OpalMediaFormat
                                   name which will be used in the recording file. The exact
                                   values possible is dependent on many factors including the
                                   specific file type and what codecs are loaded as plug ins. */
  unsigned     m_videoWidth;  /**< Width of image for recording video. */
  unsigned     m_videoHeight; /**< Height of image for recording video. */
  unsigned     m_videoRate;   /**< Frame rate for recording video. */
  OpalVideoRecordMixMode m_videoMixing; /**< How the two images are saved in video recording. */
} OpalParamRecording;


/**Call clearance information for the OpalIndCallCleared indication.
   This is only returned from the OpalGetMessage() function.
  */
typedef struct OpalStatusCallCleared {
  const char * m_callToken;   ///< Call token for call being cleared.
  const char * m_reason;      /**< String representing the reason for the call
                                   completing. This string begins with a numeric
                                   code corresponding to values in the
                                   OpalCallEndReason enum, followed by a colon and
                                   an English description. */
} OpalStatusCallCleared;


/**Type code for media stream status/control.
   This is used by the OpalIndMediaStream indication and OpalCmdMediaStream command
   in the OpalStatusMediaStream structure.
  */
typedef enum OpalCallEndReason {
  OpalCallEndedByLocalUser,         /// Local endpoint application cleared call
  OpalCallEndedByNoAccept,          /// Local endpoint did not accept call OnIncomingCall()=PFalse
  OpalCallEndedByAnswerDenied,      /// Local endpoint declined to answer call
  OpalCallEndedByRemoteUser,        /// Remote endpoint application cleared call
  OpalCallEndedByRefusal,           /// Remote endpoint refused call
  OpalCallEndedByNoAnswer,          /// Remote endpoint did not answer in required time
  OpalCallEndedByCallerAbort,       /// Remote endpoint stopped calling
  OpalCallEndedByTransportFail,     /// Transport error cleared call
  OpalCallEndedByConnectFail,       /// Transport connection failed to establish call
  OpalCallEndedByGatekeeper,        /// Gatekeeper has cleared call
  OpalCallEndedByNoUser,            /// Call failed as could not find user (in GK)
  OpalCallEndedByNoBandwidth,       /// Call failed as could not get enough bandwidth
  OpalCallEndedByCapabilityExchange,/// Could not find common capabilities
  OpalCallEndedByCallForwarded,     /// Call was forwarded using FACILITY message
  OpalCallEndedBySecurityDenial,    /// Call failed a security check and was ended
  OpalCallEndedByLocalBusy,         /// Local endpoint busy
  OpalCallEndedByLocalCongestion,   /// Local endpoint congested
  OpalCallEndedByRemoteBusy,        /// Remote endpoint busy
  OpalCallEndedByRemoteCongestion,  /// Remote endpoint congested
  OpalCallEndedByUnreachable,       /// Could not reach the remote party
  OpalCallEndedByNoEndPoint,        /// The remote party is not running an endpoint
  OpalCallEndedByHostOffline,       /// The remote party host off line
  OpalCallEndedByTemporaryFailure,  /// The remote failed temporarily app may retry
  OpalCallEndedByQ931Cause,         /// The remote ended the call with unmapped Q.931 cause code
  OpalCallEndedByDurationLimit,     /// Call cleared due to an enforced duration limit
  OpalCallEndedByInvalidConferenceID, /// Call cleared due to invalid conference ID
  OpalCallEndedByNoDialTone,        /// Call cleared due to missing dial tone
  OpalCallEndedByNoRingBackTone,    /// Call cleared due to missing ringback tone
  OpalCallEndedByOutOfService,      /// Call cleared because the line is out of service, 
  OpalCallEndedByAcceptingCallWaiting, /// Call cleared because another call is answered
  OpalCallEndedWithQ931Code = 0x100  /// Q931 code specified in MS byte
} OpalCallEndReason;


/**Call clearance information for the OpalCmdClearCall command.
  */
typedef struct OpalParamCallCleared {
  const char      * m_callToken;  ///< Call token for call being cleared.
  OpalCallEndReason m_reason;     /**< Code for the call termination to be provided to the
                                       remote system. */
} OpalParamCallCleared;


/** Message to/from OPAL system.
    This is passed via the OpalGetMessage() or OpalSendMessage() functions.
  */
struct OpalMessage {
  OpalMessageType m_type;   ///< Type of message
  union {
    const char *             m_commandError;       ///< Used by OpalIndCommandError
    OpalParamGeneral         m_general;            ///< Used by OpalCmdSetGeneralParameters
    OpalParamProtocol        m_protocol;           ///< Used by OpalCmdSetProtocolParameters
    OpalParamRegistration    m_registrationInfo;   ///< Used by OpalCmdRegistration
    OpalStatusRegistration   m_registrationStatus; ///< Used by OpalIndRegistration
    OpalParamSetUpCall       m_callSetUp;          ///< Used by OpalCmdSetUpCall/OpalIndProceeding/OpalIndAlerting/OpalIndEstablished
    const char *             m_callToken;          ///< Used by OpalCmdAnswerCall/OpalCmdHoldcall/OpalCmdRetrieveCall/OpalCmdStopRecording/OpalCmdAlerting
    OpalStatusIncomingCall   m_incomingCall;       ///< Used by OpalIndIncomingCall
    OpalStatusUserInput      m_userInput;          ///< Used by OpalIndUserInput/OpalCmdUserInput
    OpalStatusMessageWaiting m_messageWaiting;     ///< Used by OpalIndMessageWaiting
    OpalStatusLineAppearance m_lineAppearance;     ///< Used by OpalIndLineAppearance
    OpalStatusCallCleared    m_callCleared;        ///< Used by OpalIndCallCleared
    OpalParamCallCleared     m_clearCall;          ///< Used by OpalCmdClearCall
    OpalStatusMediaStream    m_mediaStream;        ///< Used by OpalIndMediaStream/OpalCmdMediaStream
    OpalParamSetUserData     m_setUserData;        ///< Used by OpalCmdSetUserData
    OpalParamRecording       m_recording;          ///< Used by OpalCmdStartRecording
  } m_param;
};


#ifdef __cplusplus
};
#endif

#if defined(__cplusplus) || defined(DOC_PLUS_PLUS)

/// Wrapper around the OpalMessage structure
class OpalMessagePtr
{
  public:
    OpalMessagePtr(OpalMessageType type = OpalIndCommandError);
    ~OpalMessagePtr();

    OpalMessageType GetType() const;
    void SetType(OpalMessageType type);

    const char               * GetCallToken() const;          ///< Used by OpalCmdAnswerCall/OpalCmdHoldCall/OpalCmdRetrieveCall/OpalCmdStopRecording/OpalCmdAlerting
    void                       SetCallToken(const char * token);

    const char               * GetCommandError() const;       ///< Used by OpalIndCommandError

    OpalParamGeneral         * GetGeneralParams() const;      ///< Used by OpalCmdSetGeneralParameters
    OpalParamProtocol        * GetProtocolParams() const;     ///< Used by OpalCmdSetProtocolParameters
    OpalParamRegistration    * GetRegistrationInfo() const;   ///< Used by OpalCmdRegistration
    OpalStatusRegistration   * GetRegistrationStatus() const; ///< Used by OpalIndRegistration
    OpalParamSetUpCall       * GetCallSetUp() const;          ///< Used by OpalCmdSetUpCall/OpalIndProceeding/OpalIndAlerting/OpalIndEstablished
    OpalStatusIncomingCall   * GetIncomingCall() const;       ///< Used by OpalIndIncomingCall
    OpalStatusUserInput      * GetUserInput() const;          ///< Used by OpalIndUserInput/OpalCmdUserInput
    OpalStatusMessageWaiting * GetMessageWaiting() const;     ///< Used by OpalIndMessageWaiting
    OpalStatusLineAppearance * GetLineAppearance() const;     ///< Used by OpalIndLineAppearance
    OpalStatusCallCleared    * GetCallCleared() const;        ///< Used by OpalIndCallCleared
    OpalParamCallCleared     * GetClearCall() const;          ///< Used by OpalCmdClearCall
    OpalStatusMediaStream    * GetMediaStream() const;        ///< Used by OpalIndMediaStream/OpalCmdMediaStream
    OpalParamSetUserData     * GetSetUserData() const;        ///< Used by OpalCmdSetUserData
    OpalParamRecording       * GetRecording() const;          ///< Used by OpalCmdStartRecording

  protected:
    OpalMessage * m_message;

  private:
    OpalMessagePtr(const OpalMessagePtr &) { }
    void operator=(const OpalMessagePtr &) { }

  friend class OpalContext;
};


#ifdef GetMessage
#undef GetMessage
#endif
#ifdef SendMessage
#undef SendMessage
#endif


/** This class is a wrapper around the "C" API.

    It may seem odd to have a C++ wrapper around a "C" API which is itself a
    wrapper around a C++ API, but sometimes a C++ programmer may wish to
    access the OPAL system via this simplified API instead of the quite
    complex one in the base OPAL library.
  */
class OpalContext
{
  public:
    /// Construct an unintialised OPAL context.
    OpalContext();

    /// Destroy the OPAL context, calls ShutDown().
    virtual ~OpalContext();

    /// Calls OpalIntialise() to initialise the OPAL context.
    /// Returns version of API supported by library, zero if error.
    unsigned Initialise(
      const char * options,  ///< List of options to pass to OpalIntialise()
      unsigned version = OPAL_C_API_VERSION ///< Version expected by application
    );

    /// Indicate if the OPAL context has been initialised.
    bool IsInitialised() const { return m_handle != NULL; }

    /// Calls OpalShutDown() to dispose of the OPAL context.
    void ShutDown();

    /// Calls OpalGetMessage() to get next message from the OPAL context.
    bool GetMessage(
      OpalMessagePtr & message,
      unsigned timeout = 0
    );

    /// Calls OpalSendMessage() to send a message to the OPAL context.
    bool SendMessage(
      const OpalMessagePtr & message,  ///< Message to send to OPAL.
      OpalMessagePtr & response        ///< Response from OPAL.
    );


    /// Execute OpalSendMessage() using OpalCmdSetUpCall
    bool SetUpCall(
      OpalMessagePtr & response,       ///< Response from OPAL context on initiating call.
      const char * partyB,             ///< Destination address, see OpalCmdSetUpCall.
      const char * partyA = NULL,      ///< Calling sub-system, see OpalCmdSetUpCall.
      const char * alertingType = NULL ///< Alerting type code, see OpalCmdSetUpCall.
    );

    /// Answer a call using OpalCmdAnswerCall via OpalSendMessage()
    bool AnswerCall(
      const char * callToken           ///< Call token for call being answered.
    );

    /// Clear a call using OpalCmdClearCall via OpalSendMessage()
    bool ClearCall(
      const char * callToken,          ///< Call token for call being cleared.
      OpalCallEndReason reason = OpalCallEndedByLocalUser  ///< Code for the call termination, see OpalCmdClearCall.
    );

    /// Send user input using OpalCmdUserInput via OpalSendMessage()
    bool SendUserInput(
      const char * callToken,     ///< Call token for the call, see OpalCmdUserInput.
      const char * userInput,     ///< User input string, e.g. "#", see OpalCmdUserInput.
      unsigned     duration = 0   ///< Duration in milliseconds for tone, see OpalCmdUserInput.
    );

  protected:
    OpalHandle m_handle;
};

#endif

#endif // OPAL_OPAL_H


/////////////////////////////////////////////////////////////////////////////
