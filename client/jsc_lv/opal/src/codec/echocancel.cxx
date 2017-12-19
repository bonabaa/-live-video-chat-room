/*
 * echocancel.cxx
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
 * The author of this code is Damien Sandras
 *
 * Contributor(s): Miguel Rodriguez Perez.
 *
 * $Revision: 21004 $
 * $Author: rjongbloed $
 * $Date: 2008-09-16 07:08:56 +0000 (Tue, 16 Sep 2008) $
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "echocancel.h"
#endif

#include <opal/buildopts.h>

extern "C" {
#ifdef OPAL_SYSTEM_SPEEX
	#if OPAL_HAVE_SPEEX_SPEEX_H
		#include <speex/speex_echo.h>
		#include <speex/speex_preprocess.h>
	#else
		#include <speex_echo.h>
		#include <speex_preprocess.h>
#endif
#else
	//#include "../src/codec/speex/libspeex/speex_echo.h"
	//#include "../src/codec/speex/libspeex/speex_preprocess.h"
	#include <speex/speex_echo.h>
	#include "speex/speex_preprocess.h"
#endif
};

#include <codec/echocancel.h>
#define D_MAX_TIME_ECHO (200)
#define D_FRAME_SIZE 480//it is short
#define D_CLOCKRATE 32000//sample
#define D_HEADER_SIZE_IN_QUEUE 8

///////////////////////////////////////////////////////////////////////////////

OpalEchoCanceler::OpalEchoCanceler()
#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif
  : receiveHandler(PCREATE_NOTIFIER(ReceivedPacket)),
    sendHandler(PCREATE_NOTIFIER(SentPacket))
#ifdef _MSC_VER
#pragma warning(default:4355)
#endif
{
  echoState = NULL;
  preprocessState = NULL;

  e_buf = NULL;
  echo_buf = NULL;
  ref_buf = NULL;
  noise = NULL;

  echo_chan = new PQueueChannel();
	  echo_chan->Open((960+D_HEADER_SIZE_IN_QUEUE)*2);
	//m_btReUseData.SetSize(200*1000);
  //echo_chan->Open(10000);
  echo_chan->SetReadTimeout(1);
  echo_chan->SetWriteTimeout(1);

  mean = 0;
  clockRate = D_CLOCKRATE;

  PTRACE(4, "Echo Canceler\tHandler created");
}


OpalEchoCanceler::~OpalEchoCanceler()
{
  PWaitAndSignal m(stateMutex);
  if (echoState) {
    speex_echo_state_destroy(echoState);
    echoState = NULL;
  }
  
  if (preprocessState) {
    speex_preprocess_state_destroy(preprocessState);
    preprocessState = NULL;
  }

  if (ref_buf)
    free(ref_buf);
  if (e_buf)
    free(e_buf);
  if (echo_buf)
    free(echo_buf);
  if (noise)
    free(noise);
  
  echo_chan->Close();
  delete(echo_chan);
}


void OpalEchoCanceler::SetParameters(const Params& newParam)
{
  PWaitAndSignal m(stateMutex);
  param = newParam;

  if (echoState) {
    speex_echo_state_destroy(echoState);
    echoState = NULL;
  }
  
  if (preprocessState) {
    speex_preprocess_state_destroy(preprocessState);
    preprocessState = NULL;
  }
}


void OpalEchoCanceler::SetClockRate(const int rate)
{
  clockRate = rate;
}


void OpalEchoCanceler::SentPacket(RTP_DataFrame& echo_frame, INT)
{
  if (echo_frame.GetPayloadSize() == 0)
    return;

  if (param.m_mode == NoCancelation)
    return;

  PWaitAndSignal m(stateMutex);
	PTime now;
	//echo_frame.SetTimestamp(now.GetTimestamp() );
	PInt64 timestamp= now.GetTimestamp();
  echo_chan->Write( & timestamp , D_HEADER_SIZE_IN_QUEUE);

  /* Write to the soundcard, and write the frame to the PQueueChannel */
  echo_chan->Write(echo_frame.GetPayloadPtr(), echo_frame.GetPayloadSize());
}


void OpalEchoCanceler::ReceivedPacket(RTP_DataFrame& input_frame, INT)
{
	PTime now ;
	PInt64 tnow = now.GetTimestamp();

  int inputSize = 0, inputSizeShort=0;
//  int i = 1;
  
  if (input_frame.GetPayloadSize() == 0)
    return;
  
  if (param.m_mode == NoCancelation)
    return;

  inputSize = input_frame.GetPayloadSize(); // Size is in bytes
  inputSizeShort = inputSize/2;
  PWaitAndSignal m(stateMutex);

#if 0//old
	if (echoState == NULL) 
    echoState = speex_echo_state_init(inputSize/sizeof(short),inputSize);
  
  if (preprocessState == NULL) { 
    preprocessState = speex_preprocess_state_init(inputSize/sizeof(short) , clockRate);
    speex_preprocess_ctl(preprocessState, SPEEX_PREPROCESS_SET_DENOISE, &i);
  }
#else
	if (echoState == NULL) 
	{
    echoState = speex_echo_state_init(D_FRAME_SIZE/*inputSize/sizeof(short)*/,D_FRAME_SIZE);
    preprocessState = speex_preprocess_state_init(D_FRAME_SIZE/*inputSize/sizeof(short)*/, clockRate);
  
		speex_echo_ctl(echoState, SPEEX_ECHO_SET_SAMPLING_RATE, &clockRate);
		speex_preprocess_ctl(preprocessState, SPEEX_PREPROCESS_SET_ECHO_STATE, echoState);
	}

	 //if (preprocessState == NULL) { 
  //  preprocessState = speex_preprocess_state_init(inputSize/sizeof(short), clockRate);
  //  speex_preprocess_ctl(preprocessState, SPEEX_PREPROCESS_SET_DENOISE, &i);
  //}

#endif
  if (echo_buf == NULL)
    echo_buf = (spx_int16_t *) malloc(inputSize+D_HEADER_SIZE_IN_QUEUE);
  if (e_buf == NULL)
    e_buf = (spx_int16_t *) malloc(inputSize);
  if (ref_buf == NULL)
    ref_buf = (spx_int16_t *) malloc(inputSize);

  /* Remove the DC offset */
#if 0
  short *j = (short *) input_frame.GetPayloadPtr();
  for (i = 0 ; i < (int) (inputSize/sizeof(short)) ; i++) {
    mean = 0.999*mean + 0.001*j[i];
    ((spx_int16_t *)ref_buf)[i] = j[i] - (short) mean;
  }
#else
	memcpy(ref_buf, input_frame.GetPayloadPtr(), inputSize);
#endif
  
#if 0  //已经加入了HEADER？？？必须要移出D_HEADER_SIZE_IN_QUEUE
  /* Read from the PQueueChannel a reference echo frame of the size
   * of the captured frame. */
  if (!echo_chan->Read((short *) echo_buf,inputSize+D_HEADER_SIZE_IN_QUEUE)) {
    
    /* Nothing to read from the speaker signal, only suppress the noise
     * and return.
     */
    speex_preprocess(preprocessState, (spx_int16_t *)ref_buf, NULL);
    memcpy(input_frame.GetPayloadPtr(), (spx_int16_t *)ref_buf, input_frame.GetPayloadSize());

    return;
  }
  char *echo_buf_body= (char*)echo_buf+ D_HEADER_SIZE_IN_QUEUE; 
  /* Cancel the echo in this frame */

	speex_echo_cancellation(echoState, (short *)ref_buf, (short *)echo_buf, (short *)e_buf/*, (spx_int32_t *)noise*/);
  
  /* Suppress the noise */
//  speex_preprocess(preprocessState, (spx_int16_t *)e_buf, (spx_int32_t *)noise);
  //speex_preprocess(preprocessState, (spx_int16_t *)e_buf, (float *)noise);
	speex_preprocess_run(preprocessState, (spx_int16_t *)e_buf/*, (spx_int32_t *)noise*/);
  memcpy(input_frame.GetPayloadPtr(),  e_buf, inputSize);

#else
	int iReadCount =0;
	PQueueChannel reRead;
	reRead.Open(100*1000);
	reRead.SetReadTimeout(10);
	reRead.SetWriteTimeout(10);
	void* echo_buf_body=NULL;
	PInt64 Oldtimestamp =0;
  int nIndexEchoBuf=0, nIndexRefbuf=0;
	while(echo_chan->Read((short *) echo_buf, inputSize+D_HEADER_SIZE_IN_QUEUE  )) 
	{
		memcpy(&Oldtimestamp, echo_buf, D_HEADER_SIZE_IN_QUEUE);
		//echo_buf_body = echo_buf;
		echo_buf_body= (char*)echo_buf+ D_HEADER_SIZE_IN_QUEUE;

		iReadCount++;

    for(int i=0;i< inputSizeShort/D_FRAME_SIZE;i++ ){
      nIndexEchoBuf =  i*D_FRAME_SIZE;
      for(int j=0;j< inputSizeShort/D_FRAME_SIZE;j++ ){//ref buff
        nIndexRefbuf = j* D_FRAME_SIZE;
			  speex_echo_cancellation(echoState, ((short *)ref_buf) + nIndexRefbuf , ((short *)echo_buf_body) +nIndexEchoBuf, ((short *)e_buf) + nIndexRefbuf );
        //memcpy(e_buf, ref_buf, inputSize);
		    speex_preprocess_run(preprocessState,( (spx_int16_t *)e_buf) + nIndexRefbuf);
        //memcpy(ref_buf, e_buf, inputSize);
      }
      //speex_echo_cancellation(echoState, ((short *)ref_buf)   , ((short *)echo_buf_body) , ((short *)e_buf) /*, (spx_int32_t *)noise*/);
    }

		//memcpy(ref_buf, e_buf, inputSize);
		PInt64 noff=0;
		if ( (noff = (tnow - Oldtimestamp ))< D_MAX_TIME_ECHO && noff >= 0 ){
			reRead.Write(echo_buf, inputSize+D_HEADER_SIZE_IN_QUEUE);
			//PTRACE(3, "echocancel\t the frame reused. size is " << reRead.GetSize() << "noff is " << noff );
		}
	}//while end
	if (iReadCount ==0)
	{
    /* Nothing to read from the speaker signal, only suppress the noise
     * and return.
     */
    int nIndexBuf=0;
    for(int i=0;i< inputSizeShort/D_FRAME_SIZE;i++ ){
      nIndexBuf =  i*D_FRAME_SIZE;
		  speex_preprocess_run(preprocessState,( (spx_int16_t *)e_buf) + nIndexBuf);
    }
		//speex_preprocess_run(preprocessState, (spx_int16_t *)e_buf/*, (spx_int32_t *)noise*/);
    //speex_preprocess(preprocessState, (spx_int16_t *)ref_buf, NULL);
    memcpy(input_frame.GetPayloadPtr(),  e_buf, inputSize);
    reRead.Close();
    return;
	}else
	{
    memcpy(input_frame.GetPayloadPtr(),  e_buf, inputSize);
	  while(reRead.Read((short *) echo_buf, inputSize+ D_HEADER_SIZE_IN_QUEUE)) 
	  {
		  echo_chan->Write(echo_buf, inputSize+D_HEADER_SIZE_IN_QUEUE);
	  }
    reRead.Close();
	}
#endif
}
OpalEchoCancelerYY::OpalEchoCancelerYY()
{
}
OpalEchoCancelerYY::~OpalEchoCancelerYY()
{
    
}