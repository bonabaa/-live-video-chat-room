/*
 * lidpluginmgr.h
 *
 * Line Interface Device plugins manager
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (C) 2005 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23898 $
 * $Author: rjongbloed $
 * $Date: 2009-12-24 01:57:27 +0000 (Thu, 24 Dec 2009) $
 */

#ifndef OPAL_LIDS_LIDPLUGINMGR_H
#define OPAL_LIDS_LIDPLUGINMGR_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <ptlib/pluginmgr.h>
#include <ptlib/sound.h>
#include <lids/lidplugin.h>
#include <lids/lid.h>



class OpalPluginLIDRegistration : public OpalLIDRegistration
{
  public:
    OpalPluginLIDRegistration(const PluginLID_Definition & definition);

    OpalLineInterfaceDevice * Create(void * param) const;

  private:
    PluginLID_Definition m_definition;
};


class OpalPluginLIDManager : public PPluginModuleManager
{
  PCLASSINFO(OpalPluginLIDManager, PPluginModuleManager);
  public:
    OpalPluginLIDManager(PPluginManager * pluginMgr = NULL);
    ~OpalPluginLIDManager();

    virtual void OnLoadPlugin(PDynaLink & dll, INT code);
    virtual void OnShutdown();

  protected:
    PList<OpalPluginLIDRegistration> m_registrations;
};



class OpalPluginLID : public OpalLineInterfaceDevice
{
  PCLASSINFO(OpalPluginLID, OpalLineInterfaceDevice);
  public:
    OpalPluginLID(const PluginLID_Definition & definition);
    ~OpalPluginLID();

    /**Open the line interface device.
      */
    virtual PBoolean Open(
      const PString & device      ///<  Device identifier name.
    );

    /**Close the line interface device.
      */
    virtual PBoolean Close();

    /**Get the device type identifier.
       This is as is used in the factory registration.
      */
    virtual PString GetDeviceType() const;

    /**Get the device name, as used to open the device.
       Note the format of this name should be as is returned from GetAllName()
       and must be able to be used in a subsequent Open() call.
      */
    virtual PString GetDeviceName() const;

    /**Get all the possible devices that can be opened.
      */
    virtual PStringArray GetAllNames() const;

    /**Get the description of the line interface device.
       This is a string indication of the card type for user interface
       display purposes or device specific control. The device should be
       as detailed as possible eg "Quicknet LineJACK".
      */
    virtual PString GetDescription() const;

    /**Get the total number of lines supported by this device.
      */
    virtual unsigned GetLineCount() const;

    /**Get the type of the line.
       A "terminal" line is one where a call may terminate. For example a POTS
       line with a standard telephone handset on it would be a terminal line.
       The alternative is a "network" line, that is one connected to switched
       network eg the standard PSTN.
      */
    virtual PBoolean IsLineTerminal(
      unsigned line   ///<  Number of line
    );


    /**Determine if a physical line is present on the logical line.
      */
    virtual PBoolean IsLinePresent(
      unsigned line,      ///<  Number of line
      PBoolean force = PFalse  ///<  Force test, do not optimise
    );


    /**Determine if line is currently off hook.
       This function implies that the state is debounced and that a return
       value of PTrue indicates that the phone is really off hook. That is
       hook flashes and winks are masked out.
      */
    virtual PBoolean IsLineOffHook(
      unsigned line   ///<  Number of line
    );

    /**Set the hook state of the line.
       Note that not be possible on a given line, for example a POTS line with
       a standard telephone handset. The hook state is determined by external
       hardware and cannot be changed by the software.
      */
    virtual PBoolean SetLineOffHook(
      unsigned line,        ///<  Number of line
      PBoolean newState = PTrue  ///<  New state to set
    );

    /**Set the hook state off then straight back on again.
       This will only operate if the line is currently off hook.
      */
    virtual PBoolean HookFlash(
      unsigned line,              ///<  Number of line
      unsigned flashTime = 200    ///<  Time for hook flash in milliseconds
    );

    /**Return PTrue if a hook flash has been detected
      */
    virtual PBoolean HasHookFlash(unsigned line);


    /**Determine if line is ringing.
       This function implies that the state is "debounced" and that a return
       value of PTrue indicates that the phone is still ringing and it is not
       simply a pause in the ring cadence.

       If cadence is not NULL then it is set with the bit pattern for the
       incoming ringing. Note that in this case the funtion may take a full
       sequence to return. If it is NULL it can be assumed that the function
       will return quickly.
      */
    virtual PBoolean IsLineRinging(
      unsigned line,          ///<  Number of line
      DWORD * cadence = NULL  ///<  Cadence of incoming ring
    );

    /**Begin ringing local phone set with specified cadence.
       If nCadence is zero then stops ringing.

       Note that not be possible on a given line, for example on a PSTN line
       the ring state is determined by external hardware and cannot be
       changed by the software.

       Also note that the cadence may be ignored by particular hardware driver
       so that only the zero or non-zero values are significant.

       The ring pattern is an array of millisecond times for on and off parts
       of the cadence. Thus the Australian ring cadence would be represented
       by the array   unsigned AusRing[] = { 400, 200, 400, 2000 }
      */
    virtual PBoolean RingLine(
      unsigned line,                   ///< Number of line
      PINDEX nCadence,                 ///< Number of entries in cadence array
      const unsigned * pattern = NULL, ///< Ring pattern times
      unsigned frequency = 400         ///< Frequency of ring (if relevant)
    );


    /**Indicate to the POTS handset that the call is connected.
       This uses the hardware (and country) dependent means to indicate to
       the remote end of a POTS connection that we have answerd. Typically
       this is a "polarity reversal" but other techniques may be used.

       The "connected" state remains in force till the remote disconnects
       the call, though hanging up.

       Returns true if successful, always returns false for PSTN lines.
      */
    virtual PBoolean SetLineConnected(
      unsigned line   ///<  Number of line
    );

    /**Determine if remote has answered call on line.
       This uses the hardware (and country) dependent means for determining
       if the remote end of a PSTN connection has answered. Typically this
       is a "polarity reversal" but other techniques may be used.

       For a POTS port this is equivalent to IsLineOffHook().
      */
    virtual PBoolean IsLineConnected(
      unsigned line   ///<  Number of line
    );


    /**Determine if line has been disconnected from a call.
       This uses the hardware (and country) dependent means for determining
       if the remote end of a PSTN connection has hung up.

       For a POTS port this is equivalent to !IsLineOffHook().
      */
    virtual PBoolean IsLineDisconnected(
      unsigned line,   ///<  Number of line
      PBoolean checkForWink = PTrue
    );


    /**Directly connect the two lines.
      */
    virtual PBoolean SetLineToLineDirect(
      unsigned line1,   ///<  Number of first line
      unsigned line2,   ///<  Number of second line
      PBoolean connect      ///<  Flag for connect/disconnect
    );

    /**Determine if the two lines are directly connected.
      */
    virtual PBoolean IsLineToLineDirect(
      unsigned line1,   ///<  Number of first line
      unsigned line2    ///<  Number of second line
    );


    /**Get the media formats this device is capable of using.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;

    /**Set the media format (codec) for reading on the specified line.
      */
    virtual PBoolean SetReadFormat(
      unsigned line,    ///<  Number of line
      const OpalMediaFormat & mediaFormat   ///<  Codec type
    );

    /**Set the media format (codec) for writing on the specified line.
      */
    virtual PBoolean SetWriteFormat(
      unsigned line,    ///<  Number of line
      const OpalMediaFormat & mediaFormat   ///<  Codec type
    );

    /**Get the media format (codec) for reading on the specified line.
      */
    virtual OpalMediaFormat GetReadFormat(
      unsigned line    ///<  Number of line
    );

    /**Get the media format (codec) for writing on the specified line.
      */
    virtual OpalMediaFormat GetWriteFormat(
      unsigned line    ///<  Number of line
    );

    /**Stop the read codec.
      */
    virtual PBoolean StopReading(
      unsigned line   ///<  Number of line
    );

    /**Stop the write codec.
      */
    virtual PBoolean StopWriting(
      unsigned line   ///<  Number of line
    );

    /**Set the read frame size in bytes.
       Note that a LID may ignore this value so always use GetReadFrameSize()
       for I/O.
      */
    virtual PBoolean SetReadFrameSize(
      unsigned line,    ///<  Number of line
      PINDEX frameSize  ///<  New frame size
    );

    /**Set the write frame size in bytes.
       Note that a LID may ignore this value so always use GetReadFrameSize()
       for I/O.
      */
    virtual PBoolean SetWriteFrameSize(
      unsigned line,    ///<  Number of line
      PINDEX frameSize  ///<  New frame size
    );

    /**Get the read frame size in bytes.
       All calls to ReadFrame() will return this number of bytes.
      */
    virtual PINDEX GetReadFrameSize(
      unsigned line   ///<  Number of line
    );

    /**Get the write frame size in bytes.
       All calls to WriteFrame() must be this number of bytes.
      */
    virtual PINDEX GetWriteFrameSize(
      unsigned line   ///<  Number of line
    );

    /**Low level read of a frame from the device.
     */
    virtual PBoolean ReadFrame(
      unsigned line,    ///<  Number of line
      void * buf,       ///<  Pointer to a block of memory to receive data.
      PINDEX & count    ///<  Number of bytes read, <= GetReadFrameSize()
    );

    /**Low level write frame to the device.
     */
    virtual PBoolean WriteFrame(
      unsigned line,    ///<  Number of line
      const void * buf, ///<  Pointer to a block of memory to write.
      PINDEX count,     ///<  Number of bytes to write, <= GetWriteFrameSize()
      PINDEX & written  ///<  Number of bytes written, <= GetWriteFrameSize()
    );


    /**Get average signal level in last frame.
      */
    virtual unsigned GetAverageSignalLevel(
      unsigned line,  ///<  Number of line
      PBoolean playback   ///<  Get average playback or record level.
    );


    /**Enable audio for the line.
      */
    virtual PBoolean EnableAudio(
      unsigned line,      ///<  Number of line
      PBoolean enable = PTrue
    );

    /**Determine if audio for the line is enabled.
      */
    virtual PBoolean IsAudioEnabled(
      unsigned line      ///<  Number of line
    ) const;


    /**Set volume level for recording.
       A value of 100 is the maximum volume possible for the hardware.
       A value of 0 is the minimum volume possible for the hardware.
      */
    virtual PBoolean SetRecordVolume(
      unsigned line,    ///<  Number of line
      unsigned volume   ///<  Volume level from 0 to 100%
    );

    /**Set volume level for playing.
       A value of 100 is the maximum volume possible for the hardware.
       A value of 0 is the minimum volume possible for the hardware.
      */
    virtual PBoolean SetPlayVolume(
      unsigned line,    ///<  Number of line
      unsigned volume   ///<  Volume level from 0 to 100%
    );

    /**Get volume level for recording.
       A value of 100 is the maximum volume possible for the hardware.
       A value of 0 is the minimum volume possible for the hardware.
      */
    virtual PBoolean GetRecordVolume(
      unsigned line,      ///<  Number of line
      unsigned & volume   ///<  Volume level from 0 to 100%
    );

    /**Set volume level for playing.
       A value of 100 is the maximum volume possible for the hardware.
       A value of 0 is the minimum volume possible for the hardware.
      */
    virtual PBoolean GetPlayVolume(
      unsigned line,      ///<  Number of line
      unsigned & volume   ///<  Volume level from 0 to 100%
    );


    /**Get acoustic echo cancellation.
       Note, not all devices may support this function.
      */
    virtual AECLevels GetAEC(
      unsigned line    ///<  Number of line
    ) const;

    /**Set acoustic echo cancellation.
       Note, not all devices may support this function.
      */
    virtual PBoolean SetAEC(
      unsigned line,    ///<  Number of line
      AECLevels level   ///<  AEC level
    );

    /**Get voice activity detection.
       Note, not all devices, or selected codecs, may support this function.
      */
    virtual PBoolean GetVAD(
      unsigned line    ///<  Number of line
    ) const;

    /**Set voice activity detection.
       Note, not all devices, or selected codecs, may support this function.
      */
    virtual PBoolean SetVAD(
      unsigned line,    ///<  Number of line
      PBoolean enable       ///<  Flag for enabling VAD
    );


    /**Get Caller ID from the last incoming ring.
       The idString parameter is either simply the "number" field of the caller
       ID data, or if full is true, all of the fields in the caller ID data.

       The full data of the caller ID string consists fields separated by tab
       characters ('\\t'), the first three are always the Calling Line Identity
       (CLI or calling number), the date and the Calling Line Name field. Other
       fields may follow and are the of the form name=value. The values are
       LID dependent.

       A false is returned if there is no Caller ID information available, e.g.
       if no ring has occurred.
      */
    virtual PBoolean GetCallerID(
      unsigned line,      ///<  Number of line
      PString & idString, ///<  ID string returned
      PBoolean full = PFalse   ///<  Get full information in idString
    );

    /**Set Caller ID information.
       The idString must be as a minimum a number fields for the Calling Line
       Identity.

       The full data of the caller ID string consists fields separated by tab
       characters ('\\t'), the first three are always the Calling Line Identity
       (CLI or calling number), the date and the Calling Line Name field. Other
       fields may follow and are the of the form name=value. The values are
       LID dependent.

       If the date field is missing (e.g. two consecutive tabs) then the current
       time and date is used. Using an empty string will clear the caller ID
       so that no caller ID is sent on the next RingLine() call.

       if the line is on hook then this information is sent when the next
       RingLine() function is called to start a ring cycle. Note that if the
       Ring cycle had already been started then this function may return false.

       If the line is off hook, then a Caller ID on Message Waiting is sent, if
       supported by the LID, otherwise false is returned.
      */
    virtual PBoolean SetCallerID(
      unsigned line,            ///<  Number of line
      const PString & idString  ///<  ID string to use
    );

    /**Send a Visual Message Waiting Indicator
      */
    virtual PBoolean SendVisualMessageWaitingIndicator(
      unsigned line,            ///<  Number of line
      PBoolean on
    );


    /**Play a DTMF digit.
       Any characters that are not in the set 0-9, A-D, * or # will be ignored.
      */
    virtual PBoolean PlayDTMF(
      unsigned line,            ///<  Number of line
      const char * digits,      ///<  DTMF digits to be played
      DWORD onTime = DefaultDTMFOnTime,  ///<  Number of milliseconds to play each DTMF digit
      DWORD offTime = DefaultDTMFOffTime ///<  Number of milliseconds between digits
    );

    /**Read a DTMF digit detected.
       This may be characters from the set 0-9, A-D, * or #. A null ('\\0')
       character indicates that there are no tones in the queue.
       Characters E through P indicate the following tones:

         E = 800   F = 1000  G = 1250  H = 950   I = 1100  J = 1400
         K = 1500  L = 1600  M = 1800  N = 2100  O = 1300  P = 2450

      */
    virtual char ReadDTMF(
      unsigned line   ///<  Number of line
    );

    /**Get DTMF removal mode.
       When set in this mode the DTMF tones detected are removed from the
       encoded data stream as returned by ReadFrame().
      */
    virtual PBoolean GetRemoveDTMF(
      unsigned line   ///<  Number of line
    );

    /**Set DTMF removal mode.
       When set in this mode the DTMF tones detected are removed from the
       encoded data stream as returned by ReadFrame().
      */
    virtual PBoolean SetRemoveDTMF(
      unsigned line,     ///<  Number of line
      PBoolean removeTones   ///<  Flag for removing DTMF tones.
    );


    /**See if any tone is detected.
      */
    virtual CallProgressTones IsToneDetected(
      unsigned line   ///< Number of line
    );

    /**See if any tone is detected.
      */
    virtual CallProgressTones WaitForToneDetect(
      unsigned line,          ///< Number of line
      unsigned timeout = 3000 ///< Milliseconds to wait for
    );

    /**See if a specific tone is detected.
      */
    virtual PBoolean WaitForTone(
      unsigned line,          ///<  Number of line
      CallProgressTones tone, ///<  Tone to wait for
      unsigned timeout = 3000 ///<  Milliseconds to wait for
    );

    /**Set a tones filter information.
      */
    virtual bool SetToneParameters(
      unsigned line,            ///<  Number of line
      CallProgressTones tone,   ///<  Tone filter to change
      unsigned frequency1,      ///<  Usually low frequency
      unsigned frequency2,      ///<  Usually high frequency
      ToneMixingModes mode,     ///<  Mode for how freqencies are mixed, -1 is 
      PINDEX numCadences,       ///<  Number of cadence times
      const unsigned * onTimes, ///<  Cadence ON times
      const unsigned * offTimes ///<  Cadence OFF times
    );

    /**Play a tone.
      */
    virtual PBoolean PlayTone(
      unsigned line,          ///<  Number of line
      CallProgressTones tone  ///<  Tone to be played
    );

    /**Determine if a tone is still playing
      */
    virtual PBoolean IsTonePlaying(
      unsigned line   ///<  Number of line
    );

    /**Stop playing a tone.
      */
    virtual PBoolean StopTone(
      unsigned line   ///<  Number of line
    );


    /**Dial a number on network line.
       The takes the line off hook, waits for dial tone, and transmits the
       specified number as DTMF tones.

       If the requireTones flag is PTrue the call is aborted of the call
       progress tones are not detected. Otherwise the call proceeds with short
       delays while it tries to detect the call progress tones.

       The return code indicates the following:
          DialTone  No dial tone detected
          RingTone  Dial was successful
          BusyTone  The remote phone was busy
          ClearTone Dial failed (usually means rang out)
          NoTone    There was an internal error making the call
      */
    virtual CallProgressTones DialOut(
      unsigned line,                ///< Number of line
      const PString & number,       ///< Number to dial
      const DialParams & params = DialParams() ///< Optional parameters for dial out.
    );


    /**Get wink detect minimum duration.
       This is the signal used by telcos to end PSTN call.
      */
    virtual unsigned GetWinkDuration(
      unsigned line    ///<  Number of line
    );

    /**Set wink detect minimum duration.
       This is the signal used by telcos to end PSTN call.
      */
    virtual PBoolean SetWinkDuration(
      unsigned line,        ///<  Number of line
      unsigned winkDuration ///<  New minimum duration
    );

    /**Set the country code set for the device.
       This may change the line analogue coefficients, ring detect, call
       disconnect detect and call progress tones to fit the countries
       telephone network.
      */
    virtual PBoolean SetCountryCode(
      T35CountryCodes country   ///<  COuntry code for device
    );

    /**Get the list of countries actually supported by the device
     */
    virtual PStringList GetCountryCodeNameList() const;



  protected:
    PDECLARE_NOTIFIER(PThread, OpalPluginLID, TonePlayer);
    bool StartTonePlayerThread(int tone);
    void StopTonePlayerThread();

#if PTRACING
    bool BadContext() const;
    bool BadFunction(void * fnPtr, const char * fnName) const;
    PluginLID_Errors CheckError(PluginLID_Errors error, const char * fnName) const;
#endif

    const PluginLID_Definition & m_definition;
    void                       * m_context;
    PString                      m_deviceName;

    PSoundChannel                m_recorder;
    PSoundChannel                m_player;
    PThread                    * m_tonePlayer;
    PSyncPoint                   m_stopTone;
    bool                         m_lockOutTones;
};


PFACTORY_LOAD(OpalPluginLIDManager);


#endif // OPAL_LIDS_LIDPLUGINMGR_H
