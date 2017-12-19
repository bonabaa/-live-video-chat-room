/*
 * paec.h
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
 * The author of this code is Damien Sandras
 *
 * rewritten amd made generic ptlib by Simon Horne
 *
 * Contributor(s): Miguel Rodriguez Perez
 *
 * $Revision: 21788 $
 * $Author: rjongbloed $
 * $Date: 2008-12-12 05:42:13 +0000 (Fri, 12 Dec 2008) $
 */

#ifndef PTLIB_PAEC_H
#define PTLIB_PAEC_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptclib/qchannel.h>

/** This class implements Acoustic Echo Cancellation
  * The principal is to copy to a buffer incoming audio.
  * after it has been decoded and when recording the audio 
  * to remove the echo pattern from the incoming audio 
  * prior to sending to the enooder..
  */

PQUEUE(ReceiveTimeQueue, PTimeInterval);

struct SpeexEchoState;
struct SpeexPreprocessState;
class PAec : public PObject
{
  PCLASSINFO(PAec, PObject);
public:

  /**@name Construction */
  //@{
  /**Create a new canceler.
   */
     PAec(int _clock = 8000, int _sampletime = 30);
     ~PAec();
  //@}

  /**@@name Basic operations */
  //@{
  /**Recording Channel. Should be called prior to encoding audio
   */
    void Send(BYTE * buffer, unsigned & length);

  /**Playing Channel  Should be called after decoding and prior to playing.
   */
    void Receive(BYTE * buffer, unsigned & length);
  //@}

protected:
  PMutex readwritemute;
  PQueueChannel *echo_chan;
  SpeexEchoState *echoState;
  SpeexPreprocessState *preprocessState;
  int clockrate;                      // Frame Rate default 8000hz for narrowband audio
  int bufferTime;                     // Time between receiving and Transmitting   
  PInt64 minbuffer;                   // minbuffer (in milliseconds)
  PInt64 maxbuffer;                   // maxbuffer (in milliseconds)
  int sampleTime;                     // Length of each sample
  ReceiveTimeQueue rectime;           // Queue of timestamps for ensure read/write in sync
  PTimeInterval lastTimeStamp;        // LastTimeStamp of recieved data
  PBoolean receiveReady;
  void *ref_buf;
  void *echo_buf;
  void *e_buf;
  void *noise;

};

#endif // PTLIB_PAEC_H

// End Of File ///////////////////////////////////////////////////////////////
