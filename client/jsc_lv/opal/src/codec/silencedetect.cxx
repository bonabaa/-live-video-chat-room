/*
 * silencedetect.cxx
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2001 Post Increment
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

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "silencedetect.h"
#endif
#include <opal/buildopts.h>

#include <codec/silencedetect.h>
#include <opal/patch.h>

#define new PNEW


extern "C" {
  unsigned char linear2ulaw(int pcm_val);
  int ulaw2linear(unsigned char u_val);
};


ostream & operator<<(ostream & strm, OpalSilenceDetector::Mode mode)
{
  static const char * const names[OpalSilenceDetector::NumModes] = {
      "NoSilenceDetection",
      "FixedSilenceDetection",
      "AdaptiveSilenceDetection"
  };

  if (mode >= 0 && mode < OpalSilenceDetector::NumModes && names[mode] != NULL)
    strm << names[mode];
  else
    strm << "OpalSilenceDetector::Modes<" << mode << '>';
  return strm;
}


///////////////////////////////////////////////////////////////////////////////

OpalSilenceDetector::OpalSilenceDetector(const Params & theParam)
#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif
  : receiveHandler(PCREATE_NOTIFIER(ReceivedPacket)),
#ifdef _MSC_VER
#pragma warning(default:4355)
#endif
  clockRate (8000)
{
  // Initialise the adaptive threshold variables.
  SetParameters(theParam);

  PTRACE(4, "Silence\tHandler created");
}


void OpalSilenceDetector::AdaptiveReset()
{
  // Initialise threshold level
  levelThreshold = 0;

  // Initialise the adaptive threshold variables.
  signalMinimum = UINT_MAX;
  silenceMaximum = 0;
  signalReceivedTime = 0;
  silenceReceivedTime = 0;

  // Restart in silent mode
  inTalkBurst = false;
  lastTimestamp = 0;
  receivedTime = 0;
}


void OpalSilenceDetector::SetParameters(const Params & newParam, const int rate /*= 0*/)
{
  PWaitAndSignal mutex(inUse);
  if (rate)
    clockRate = rate;
  mode = newParam.m_mode;
  signalDeadband = newParam.m_signalDeadband * clockRate / 1000;
  silenceDeadband = newParam.m_silenceDeadband * clockRate / 1000;
  adaptivePeriod = newParam.m_adaptivePeriod * clockRate / 1000;
  if (mode == FixedSilenceDetection)
    levelThreshold = newParam.m_threshold;// note: this value compared to uLaw encoded signal level
  else
    AdaptiveReset();

  PTRACE(4, "Silence\tParameters set: "
            "mode=" << mode << ", "
            "threshold=" << levelThreshold << ", "
            "silencedb=" << silenceDeadband << " samples, "
            "signaldb=" << signalDeadband << " samples, "
            "period=" << adaptivePeriod << " samples");
}


void OpalSilenceDetector::SetClockRate(const int rate)
{
  PWaitAndSignal mutex(inUse);
  signalDeadband = signalDeadband * 1000 / clockRate * rate / 1000;
  silenceDeadband = silenceDeadband * 1000 / clockRate * rate / 1000;
  adaptivePeriod = adaptivePeriod * 1000 / clockRate * rate / 1000;
  clockRate = rate;
  if (mode == AdaptiveSilenceDetection)
    AdaptiveReset();
}


OpalSilenceDetector::Mode OpalSilenceDetector::GetStatus(PBoolean * isInTalkBurst,
                                                       unsigned * currentThreshold) const
{
  if (isInTalkBurst != NULL)
    *isInTalkBurst = inTalkBurst;

  if (currentThreshold != NULL)
    *currentThreshold = ulaw2linear((BYTE)(levelThreshold ^ 0xff));

  return mode;
}


void OpalSilenceDetector::ReceivedPacket(RTP_DataFrame & frame, INT)
{
  // Already silent
  if (frame.GetPayloadSize() == 0)
    return;

  PWaitAndSignal mutex(inUse);

  // Can never have silence if NoSilenceDetection
  if (mode == NoSilenceDetection)
    return;

  unsigned thisTimestamp = frame.GetTimestamp();
  if (lastTimestamp == 0) {
    lastTimestamp = thisTimestamp;
    return;
  }

  unsigned timeSinceLastFrame = thisTimestamp - lastTimestamp;
  lastTimestamp = thisTimestamp;

  // Average is absolute value up to 32767
  unsigned level = GetAverageSignalLevel(frame.GetPayloadPtr(), frame.GetPayloadSize());

  // Can never have average signal level that high, this indicates that the
  // hardware cannot do silence detection.
  if (level == UINT_MAX)
    return;

  // Convert to a logarithmic scale - use uLaw which is complemented
  level = linear2ulaw(level) ^ 0xff;

  // Now if signal level above threshold we are "talking"
  PBoolean haveSignal = level > levelThreshold;

  // If no change ie still talking or still silent, reset frame counter
  if (inTalkBurst == haveSignal)
    receivedTime = 0;
  else {
    receivedTime += timeSinceLastFrame;
    // If have had enough consecutive frames talking/silent, swap modes.
    if (receivedTime >= (inTalkBurst ? silenceDeadband : signalDeadband)) {
      inTalkBurst = !inTalkBurst;
      PTRACE(4, "Silence\tDetector transition: "
             << (inTalkBurst ? "Talk" : "Silent")
             << " level=" << level << " threshold=" << levelThreshold);

      // If we had talk/silence transition restart adaptive threshold measurements
      signalMinimum = UINT_MAX;
      silenceMaximum = 0;
      signalReceivedTime = 0;
      silenceReceivedTime = 0;

      // If we just have moved to sending a talk burst, set the RTP marker
      if (inTalkBurst)
        frame.SetMarker(true);
    }
  }

  if (mode == FixedSilenceDetection) {
    if (!inTalkBurst)
      frame.SetPayloadSize(0); // Not in talk burst so silence the frame
    return;
  }

  // Adaptive silence detection
  if (levelThreshold == 0) {
    if (level > 1) {
      // Bootstrap condition, use first frame level as silence level
      levelThreshold = level/2;
      PTRACE(4, "Silence\tThreshold initialised to: " << levelThreshold);
    }
    // inTalkBurst always PFalse here, so return silent
    frame.SetPayloadSize(0);
    return;
  }

  // Count the number of silent and signal frames and calculate min/max
  if (haveSignal) {
    if (level < signalMinimum)
      signalMinimum = level;
    signalReceivedTime=signalReceivedTime+timeSinceLastFrame;
  }
  else {
    if (level > silenceMaximum)
      silenceMaximum = level;
    silenceReceivedTime=silenceReceivedTime+timeSinceLastFrame;
  }

  // See if we have had enough frames to look at proportions of silence/signal
  if ((signalReceivedTime + silenceReceivedTime) > adaptivePeriod) {

    /* Now we have had a period of time to look at some average values we can
       make some adjustments to the threshold. There are four cases:
     */
    if (signalReceivedTime >= adaptivePeriod) {
      /* If every frame was noisy, move threshold up. Don't want to move too
         fast so only go a quarter of the way to minimum signal value over the
         period. This avoids oscillations, and time will continue to make the
         level go up if there really is a lot of background noise.
       */
      int delta = (signalMinimum - levelThreshold)/4;
      if (delta != 0) {
        levelThreshold += delta;
        PTRACE(4, "Silence\tThreshold increased to: " << levelThreshold);
      }
    }
    else if (silenceReceivedTime >= adaptivePeriod) {
      /* If every frame was silent, move threshold down. Again do not want to
         move too quickly, but we do want it to move faster down than up, so
         move to halfway to maximum value of the quiet period. As a rule the
         lower the threshold the better as it would improve response time to
         the start of a talk burst.
       */
      unsigned newThreshold = (levelThreshold + silenceMaximum)/2 + 1;
      if (levelThreshold != newThreshold) {
        levelThreshold = newThreshold;
        PTRACE(4, "Silence\tThreshold decreased to: " << levelThreshold);
      }
    }
    else if (signalReceivedTime > silenceReceivedTime) {
      /* We haven't got a definitive silent or signal period, but if we are
         constantly hovering at the threshold and have more signal than
         silence we should creep up a bit.
       */
      levelThreshold++;
      PTRACE(4, "Silence\tThreshold incremented to: " << levelThreshold
             << " signal=" << signalReceivedTime << ' ' << signalMinimum
             << " silence=" << silenceReceivedTime << ' ' << silenceMaximum);
    }

    signalMinimum = UINT_MAX;
    silenceMaximum = 0;
    signalReceivedTime = 0;
    silenceReceivedTime = 0;
  }

  if (!inTalkBurst)
    frame.SetPayloadSize(0); // Not in talk burst so silence the frame
}


/////////////////////////////////////////////////////////////////////////////

unsigned OpalPCM16SilenceDetector::GetAverageSignalLevel(const BYTE * buffer, PINDEX size)
{
  // Calculate the average signal level of this frame
  int sum = 0;
  PINDEX samples = size/2;
  const short * pcm = (const short *)buffer;
  const short * end = pcm + samples;
  while (pcm != end) {
    if (*pcm < 0)
      sum -= *pcm++;
    else
      sum += *pcm++;
  }

  return sum/samples;
}


/////////////////////////////////////////////////////////////////////////////
