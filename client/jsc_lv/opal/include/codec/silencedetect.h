/*
 * silencedetect.h
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2004 Post Increment
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
 * $Revision: 23735 $
 * $Author: rjongbloed $
 * $Date: 2009-10-31 02:20:26 +0000 (Sat, 31 Oct 2009) $
 */

#ifndef OPAL_CODEC_SILENCEDETECT_H
#define OPAL_CODEC_SILENCEDETECT_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>
#include <rtp/rtp.h>


///////////////////////////////////////////////////////////////////////////////

class OpalSilenceDetector : public PObject
{
    PCLASSINFO(OpalSilenceDetector, PObject);
  public:
    enum Mode {
      NoSilenceDetection,
      FixedSilenceDetection,
      AdaptiveSilenceDetection,
      NumModes
    };

    struct Params {
      Params(
        Mode mode = AdaptiveSilenceDetection, ///<  New silence detection mode
        unsigned threshold = 0,               ///<  Threshold value if FixedSilenceDetection
        unsigned signalDeadband = 10,         ///<  10 milliseconds of signal needed
        unsigned silenceDeadband = 400,       ///<  400 milliseconds of silence needed
        unsigned adaptivePeriod = 600         ///<  600 millisecond window for adaptive threshold
      )
        : m_mode(mode),
          m_threshold(threshold),
          m_signalDeadband(signalDeadband),
          m_silenceDeadband(silenceDeadband),
          m_adaptivePeriod(adaptivePeriod)
        { }

      Mode     m_mode;             /// Silence detection mode
      unsigned m_threshold;        /// Threshold value if FixedSilenceDetection
      unsigned m_signalDeadband;   /// milliseconds of signal needed
      unsigned m_silenceDeadband;  /// milliseconds of silence needed
      unsigned m_adaptivePeriod;   /// millisecond window for adaptive threshold
    };

  /**@name Construction */
  //@{
    /**Create a new detector. Default clock rate is 8000.
     */
    OpalSilenceDetector(
      const Params & newParam ///<  New parameters for silence detector
    );
  //@}

  /**@name Basic operations */
  //@{
    const PNotifier & GetReceiveHandler() const { return receiveHandler; }

    /**Set the silence detector parameters.
       This adjusts the silence detector "agression". The deadband and 
       adaptive periods are in ms units to work for any clock rate.
	   The clock rate value is optional: 0 leaves value unchanged.
       This may be called while audio is being transferred, but if in
       adaptive mode calling this will reset the filter.
      */
    void SetParameters(
      const Params & params,  ///< New parameters for silence detector
      const int clockRate = 0 ///< Sampling clock rate for the preprocessor
    );

    /**Set the sampling clock rate for the preprocessor.
       Adusts the interpretation of time values.
       This may be called while audio is being transferred, but if in
       adaptive mode calling this will reset the filter.
     */
    void SetClockRate(
      const int clockRate     ///< Sampling clock rate for the preprocessor
    );

    /**Get the cyrrent sampling clock rate.
      */
    int GetClockRate() const { return clockRate; }

    /**Get silence detection status

       The inTalkBurst value is PTrue if packet transmission is enabled and
       PFalse if it is being suppressed due to silence.

       The currentThreshold value is the value from 0 to 32767 which is used
       as the threshold value for 16 bit PCM data.
      */
    Mode GetStatus(
      PBoolean * isInTalkBurst,
      unsigned * currentThreshold
    ) const;

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
    ) = 0;

  private:
    /**Reset the adaptive filter
     */
    void AdaptiveReset();

  protected:
    PDECLARE_NOTIFIER(RTP_DataFrame, OpalSilenceDetector, ReceivedPacket);

    PNotifier receiveHandler;

    Mode mode;
    unsigned signalDeadband;        // #samples of signal needed
    unsigned silenceDeadband;       // #samples of silence needed
    unsigned adaptivePeriod;        // #samples window for adaptive threshold
    int clockRate;                  // audio sampling rate

    unsigned lastTimestamp;         // Last timestamp received
    unsigned receivedTime;          // Signal/Silence duration received so far.
    unsigned levelThreshold;        // Threshold level for silence/signal
    unsigned signalMinimum;         // Minimum of frames above threshold
    unsigned silenceMaximum;        // Maximum of frames below threshold
    unsigned signalReceivedTime;    // Duration of signal received
    unsigned silenceReceivedTime;   // Duration of silence received
    bool     inTalkBurst;           // Currently sending RTP data
    PMutex   inUse;                 // Protects values to allow change while running
};


class OpalPCM16SilenceDetector : public OpalSilenceDetector
{
    PCLASSINFO(OpalPCM16SilenceDetector, OpalSilenceDetector);
  public:
    /** Construct new silence detector for PCM-16/8000
      */
    OpalPCM16SilenceDetector(
      const Params & newParam ///<  New parameters for silence detector
    ) : OpalSilenceDetector(newParam) { }

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
};


extern ostream & operator<<(ostream & strm, OpalSilenceDetector::Mode mode);


#endif // OPAL_CODEC_SILENCEDETECT_H


/////////////////////////////////////////////////////////////////////////////
