/**
* g711a1_plc.cxx
*
* packet loss concealment algorithm is based on the ITU recommendation
* G.711 Appendix I.
*
* Copyright (c) 2008 Christian Hoene, University of Tuebingen, Germany
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
* The Initial Developer of the Original Code is Christian Hoene
* based on the code given in ITU-T Recommendation G.711 Appendix I (09/99) 
* "A high quality low-complexity algorithm for packet loss concealment with G.711"
* Approved in 1999-09, www.itu.int.
*
* Contributor(s): ______________________________________.
*
* $Revision: 21408 $
* $Author: csoutheren $
* $Date: 2008-10-23 07:08:46 +0000 (Thu, 23 Oct 2008) $
*/

#ifndef SBC_DISABLE_PTLIB
#include <ptlib.h>
#include <opal/buildopts.h>
#endif

#include <codec/g711a1_plc.h>

#if OPAL_G711PLC

#include <math.h>


static const int CORR_DECIMATION_RATE=4000;     /**< decimation to a rate of [Hz] */
static const int corr_len_ms=20;       /**< Correlation length [ms] */

static const int OVERLAPADD=4;             /**< length of the overlapadd windows [ms] */
static const double CORR_MINPOWER=250./80.; /**< minimum power */

static const int CONCEAL_ATTENUATION_PERIOD1=10;      /**< first 10 ms are not faded out */
static const int CONCEAL_ATTENUATION_PERIOD2=50;      /**< Then, attenuation for [ms] */

static const int CONCEAL_PERIOD1=10;     /**< the first 10 ms uses just the first pitch period */
static const int CONCEAL_PERIOD2=50;     /**< length of the second period uses just multiple pitch periods */

static const int TRANSITION_START=10;    /**< after 10ms of loss, the transition period will be prolonged (ITU G.711 I.2.7) [ms] */
static const double TRANSITION_RATIO=0.4;  /**< for each 10ms of loss, the transition period will be prolonged by 4ms (ITU G.711 I.2.7) */
static const int TRANSITION_MAX=10;      /**< the transition period will have a maximal length of 10ms (ITU G.711 I.2.7 [ms] */

static const double PITCH_LOW=66.6;/**< minimum allowed pitch. default 66 Hz [Hz] */
static const double PITCH_HIGH=200;/**< maximum allowed pitch. default 200 Hz [Hz] */


#ifdef _MSC_VER
__inline double round(const double & value)
{
  return value < 0 ? floor(value-0.5) : ceil(value+0.5);
}
#endif


/** concealment constructor.
* This function inits all state variables and print them out for verification purposes. 
* In addition it allocated memory.
* @see g711plc_destroy
* @param lc the concealment state.  
* @param rate the sampling rate. Default is 8000.
* @param pitch_max_hz the highest frequency, which is considered for determing the pitch [Hz]. Default is 200.
* @param pitch_min_hz the lowest pitch frequency [Hz}. Default is 66.
*/

OpalG711_PLC::OpalG711_PLC(int rate, double pitch_low, double pitch_high)
{
#ifndef SBC_DISABLE_PTLIB
  PAssert(rate >= 8000 && rate <= 48000, PInvalidParameter);
#endif
  this->rate = rate;

#ifndef SBC_DISABLE_PTLIB
  PAssert(pitch_high <= 1000 && pitch_high > pitch_low, PInvalidParameter);
#endif
  pitch_min = int(rate/pitch_high);                /* minimum allowed pitch, default 200 Hz */

#ifndef SBC_DISABLE_PTLIB
  PAssert(1/pitch_low < corr_len_ms, PInvalidParameter);
#endif
  pitch_max = int(rate/pitch_low);                /* maximum allowed pitch, default 66 Hz */


  this->pitch_overlapmax = (this->pitch_max >> 2);             /* maximum pitch OLA window */
  this->hist_len = (this->pitch_max * 3 + this->pitch_overlapmax);/* history buffer length*/

  pitch_buf = new double[hist_len];  /* buffer for cycles of speech */
  hist_buf = new short[hist_len];  /* history buffer */
  tmp_buf = new short[pitch_overlapmax];


  /* correlation related variables */

  /*
  printf("sampling rate         %dHz\n",rate);
  printf("highest allowed pitch %lfHz\t%d\n",pitch_high,pitch_min);
  printf("lowest  allowed pitch %lfHz\t%d\n",pitch_low,pitch_max);
  printf("correlation length    %dHz\t%d\n",corr_len_ms,ms2samples(corr_len_ms));
  */

  pitch_lastq = new double[this->pitch_overlapmax];  /* saved last quarter wavelengh */
  conceal_overlapbuf = new short[this->hist_len];     /* store overlapping speech segments */

  transition_buf = new short[ms2samples(TRANSITION_MAX)];

  this->conceal_count = 0;

  zero(hist_buf, hist_len);

  mode = NOLOSS;
}

/** concealment destructor.
* This function frees the memory.
* @see OpalG711_PLC::construct
* @param lc the concealment state.  
*/

OpalG711_PLC::~OpalG711_PLC()
{
  //  printf("Destroy OpalG711_PLC()\n");
  delete[] transition_buf;
  delete[] conceal_overlapbuf;
  delete[] pitch_buf;
  delete[] pitch_lastq;
  delete[] hist_buf;
  delete[] tmp_buf;
}

/** Get samples from the circular pitch buffer. Update pitch_offset so
* when subsequent frames are erased the signal continues.
* @param out buffer to which the pitch period will be copied.
* @param sz size of the buffer.
*/
void OpalG711_PLC::getfespeech(short *out, int sz)
{
  while (sz) {
    int cnt = pitch_blen - pitch_offset;
    if (cnt > sz)
      cnt = sz;
    convertfs(pitch_buf+hist_len-pitch_blen+pitch_offset, out, cnt);
    //    for(int i=0;i<cnt;i++)
    //      printf("$ %d %d %04X\n",sz,pitch_offset+i,out[i]);
    pitch_offset += cnt;
    if (pitch_offset == pitch_blen)
      pitch_offset = 0;
    out += cnt;
    sz -= cnt;
  }
}

/** Decay concealed signal.
* The concealed signal is slowly faded out. For a CONEAL_OFFSET period the signal is
* keep high. Then, it is slowly decayed until it become pure silence.
* @param lc concealment state.
* @param inout the buffer to decay.
* @param size the size of the buffer.
* 
*/
void OpalG711_PLC::scalespeech(short *inout, int size, bool decay) const
{
  int i;
  /** attenuation per sample */
  double attenincr = 1./ms2samples(CONCEAL_ATTENUATION_PERIOD2);  
  double g = (double)1. - (conceal_count-ms2samples(CONCEAL_ATTENUATION_PERIOD1)) * attenincr;

  for (i = 0; i < size; i++) {
    if(g<0)
      inout[i]=0;
    else {
      if(g<1) 
        inout[i] = short(round(inout[i] * g));
      if(decay)
        g -= attenincr;
    }
  }
}

/** Generate the synthetic signal.
* @param out buffer for synthetic signal
* @param size number of samples requested
*/
void OpalG711_PLC::dofe(short *out, int size)
{
  do {
    int res = dofe_partly(out, size);
#ifndef SBC_DISABLE_PTLIB
    PAssert(res > 0 && res <=size, PInvalidParameter);
#endif
    size -= res;
    out += res;
  }while(size>0);
}

/** Generate the synthetic signal until a new mode is reached.
* At the beginning of an erasure determine the pitch, and extract
* one pitch period from the tail of the signal. Do an OLA for 1/4
* of the pitch to smooth the signal. Then repeat the extracted signal
* for the length of the erasure. If the erasure continues for more than
* 10 msec, increase the number of periods in the pitchbuffer. At the end
* of an erasure, do an OLA with the start of the first good frame.
* The gain decays as the erasure gets longer.
* @param out buffer for synthetic signal
* @param size number of samples requested
* @return number of samples generated
*/

int OpalG711_PLC::dofe_partly(short *out, int size)
{
  //  printf("dofe_partly: mode %d\tcnt %d\tsz %d\t->\t",mode, conceal_count,size);

  switch(mode) {
  case NOLOSS:
  case TRANSITION:
    // first erased bytes


    convertsf(hist_buf, pitch_buf, hist_len);          /* get history */
    pitch = findpitch();                         /* find pitch */
    pitch_overlap = this->pitch >> 2;           /* OLA 1/4 wavelength */
    if(pitch_overlap > pitch_overlapmax)               /* limited algorithmic delay */
      pitch_overlap = pitch_overlapmax;

    copy(pitch_buf + hist_len-pitch_overlap, pitch_lastq, pitch_overlap);    /* save original last pitch_overlap samples */

    pitch_offset = 0;                                         /* create pitch buffer with 1 period */
    pitch_blen = pitch;
    overlapadd(pitch_lastq, 
      pitch_buf + (hist_len-pitch_blen-pitch_overlap), 
      pitch_buf + (hist_len-pitch_overlap), 
      pitch_overlap);

    /* update last 1/4 wavelength in hist_buf buffer */
    convertfs(pitch_buf + (hist_len-pitch_overlap), hist_buf  + (hist_len-pitch_overlap), pitch_overlap);

    conceal_count = 0;
    mode = LOSS_PERIOD1;
    // fall thru

  case LOSS_PERIOD1:
    /* get synthesized speech */
    if(size+conceal_count >= ms2samples(CONCEAL_PERIOD1)) {
      size = ms2samples(CONCEAL_PERIOD1)-conceal_count;
      mode = LOSS_PERIOD2start;
    }
    getfespeech(out, size);    
    break;

    // start of second period
  case LOSS_PERIOD2start: {    
    /* tail of previous pitch estimate */
    int saveoffset = pitch_offset;                    /* save offset for OLA */
    getfespeech(tmp_buf, pitch_overlap);                  /* continue with old pitch_buf */

    /* add periods to the pitch buffer */
    pitch_offset = saveoffset % pitch; // why??
    pitch_blen += pitch;                      /* add a period */
    overlapadd(pitch_lastq, 
      pitch_buf + (hist_len-pitch_blen-pitch_overlap),
      pitch_buf + (hist_len-pitch_overlap), 
      pitch_overlap);

    /* overlap add old pitchbuffer with new */
    getfespeech(conceal_overlapbuf, pitch_overlap);
    overlapadds(tmp_buf, conceal_overlapbuf, conceal_overlapbuf, pitch_overlap);
    scalespeech(conceal_overlapbuf, pitch_overlap);
    mode = LOSS_PERIOD2overlap;

    // fall thru
                          }
                          // still overlapping period at the beginning?
  case LOSS_PERIOD2overlap:
    if(conceal_count + size >= ms2samples(CONCEAL_PERIOD1)+pitch_overlap) {
      size = ms2samples(CONCEAL_PERIOD1)+pitch_overlap-conceal_count;
      mode=LOSS_PERIOD2;
    }
    copy(conceal_overlapbuf+conceal_count-ms2samples(CONCEAL_PERIOD1), out, size);
    break;

    // no overlapping period
  case LOSS_PERIOD2:
    if(size + conceal_count >= ms2samples(CONCEAL_PERIOD1+CONCEAL_PERIOD2)) {
      size = ms2samples(CONCEAL_PERIOD1+CONCEAL_PERIOD2)-conceal_count;
      mode = LOSS_PERIOD3;
    }
    getfespeech(out, size);
    scalespeech(out, size);
    break;

    // erased bytes after the second period
  case LOSS_PERIOD3:
    zero(out, size);
    break;

  default:
#ifndef SBC_DISABLE_PTLIB
    PAssertAlways(PLogicError)
#endif
    ;
  }

  conceal_count+=size;
  hist_savespeech(out, size);
  //  printf("mode %d\tsz %d\n",mode,size);
  return size;
}

/** Saves samples in the history buffer.
* Save a frames worth of new speech in the history buffer.
* Return the output speech delayed by this->pitch_overlapmax
* @param inout buffer which contains the input speech and to which the delay speech is written
* @param size size of this buffer
*/
void OpalG711_PLC::hist_savespeech(short *inout, int size)
{
  if(size < hist_len-pitch_overlapmax) {
    //    printf(".");
    /* make room for new signal */
    copy(hist_buf + size, hist_buf, hist_len - size);
    /* copy in the new frame */
    copy(inout, hist_buf + hist_len - size, size);
    /* copy history signal to inout buffer*/
    copy(hist_buf + hist_len - size - pitch_overlapmax, inout, size);
  }
  else if(size <= hist_len) {
    //    printf(";");
    /* store old signal tail */
    copy(hist_buf + hist_len - pitch_overlapmax, tmp_buf, pitch_overlapmax);
    /* make room for new signal */
    copy(hist_buf + size, hist_buf, hist_len - size);
    /* copy in the new frame */
    copy(inout, hist_buf + hist_len - size, size);
    /* move data to delay frame */
    copy(inout, inout + pitch_overlapmax, size - pitch_overlapmax);
    /* copy history signal */
    copy(tmp_buf, inout, pitch_overlapmax);
  }
  else {
    //    printf(": %d %d %d\n",size,hist_len,pitch_overlapmax);
    /* store old signal tail */
    copy(hist_buf + hist_len - pitch_overlapmax, tmp_buf, pitch_overlapmax);
    /* copy in the new frame */
    copy(inout + size - hist_len, hist_buf, hist_len);
    /* move data to delay frame */
    copy(inout, inout + pitch_overlapmax, size - pitch_overlapmax);
    /* copy history signal */
    copy(tmp_buf, inout, pitch_overlapmax);
  }
}

/** receiving good samples.
* A good frame was received and decoded.
* If right after an erasure, do an overlap add with the synthetic signal.
* Add the frame to history buffer.
* @param s pointer to samples
* @param size number of samples
*/
void OpalG711_PLC::addtohistory(short *s, int size)
{
  int end;

  switch(mode) {
  case NOLOSS:
    break;
  case LOSS_PERIOD1:
  case LOSS_PERIOD2:
  case LOSS_PERIOD2start:
  case LOSS_PERIOD2overlap:
  case LOSS_PERIOD3:
    mode = TRANSITION;
    /** I.2.7   First good frame after an erasure.
    At the first good frame after an erasure, a smooth transition is needed between the synthesized
    erasure speech and the real signal. To do this, the synthesized speech from the pitch buffer is
    continued beyond the end of the erasure, and then mixed with the real signal using an OLA. The
    length of the OLA depends on both the pitch period and the length of the erasure. For short, 10 ms
    erasures, a 1/4 wavelength window is used. For longer erasures the window is increased by 4 ms per
    10 ms of erasure, up to a maximum of the frame size, 10 ms.
    */
    transition_len = pitch_overlap;
    if(conceal_count > ms2samples(TRANSITION_START))
      transition_len += int(round((conceal_count - ms2samples(TRANSITION_START))*TRANSITION_RATIO));
    if(transition_len > ms2samples(TRANSITION_MAX))
      transition_len = ms2samples(TRANSITION_MAX);
    getfespeech(transition_buf, transition_len);
    scalespeech(transition_buf, transition_len,false);
    transition_count=0;

  case TRANSITION:
    end = transition_count+size;
    if(end >= transition_len) {
      end = transition_len;
      mode = NOLOSS;
    }
    //    printf("addtohistory: s %d\tlen %d\te %d\tcnt %d\tadel %d\n",transition_count,transition_len, end, conceal_count, getAlgDelay());
    overlapaddatend(s, transition_buf+transition_count, transition_count, end, transition_len);
    transition_count = end;
  }
  hist_savespeech(s, size);
}

/** the previous frame is dropped intentionally to speed up the play out
* @param s pointer to samples
* @param size number of samples
*/
void OpalG711_PLC::drop(short *s, int size)
{
  dofe(transition_buf,ms2samples(TRANSITION_MAX));
  transition_len = pitch_overlap;
  transition_count = 0;
  mode = TRANSITION;
  addtohistory(s, size);
}

/** 
* Overlapp add the end of the erasure with the start of the first good frame.
* @param s pointer to good signal
* @param f pointer to concealed signal
* @param start first sample to consider
* @param end last sample to consider (not including)
* @param count length of decay period [samples]
*/
void OpalG711_PLC::overlapaddatend(short *s, short *f, int start, int end, int count) const
{
#ifndef SBC_DISABLE_PTLIB
  PAssert(start<=end, PInvalidParameter);
  PAssert(end<=count, PInvalidParameter);
  PAssert(start >= 0 && count < 32767, PInvalidParameter);
#endif

  int size=end-start;
  start++;
  for (int i = 0; i < size; i++) {
    int t = ((count-start) * f[i] + start * s[i]) / count;
    //    printf("%d %d %d %d -> %d\n",count-start,f[i],start,s[i], t);
    //    printf("%04X %04X -> %08X (%d %d)\n",f[i]&0xffff,s[i]&0xffff,t,start,count);
    if (t > 32767)
      t = 32767;
    else if (t < -32768)
      t = -32768;
    s[i] = (short)t;
#ifndef SBC_DISABLE_PTLIB
    PAssert(end >=0 && end<=count && start >=0 && start <= count, PInvalidParameter);
#endif
    start++;
  }
}

/** Overlapp add left and right sides.
* @param l   input buffer (gets silent)
* @param r   input buffer (gets lough)
* @param o   output buffer
* @param cnt size of buffers
*/
void OpalG711_PLC::overlapadd(double *l, double *r, double *o, int cnt) const
{
  int  i;
  double  incr, lw, rw, t;

  if (cnt == 0)
    return;
  incr = (double)1. / cnt;
  lw = (double)1. - incr;
  rw = incr;
  for (i = 0; i < cnt; i++) {
    t = lw * l[i] + rw * r[i];
    if (t > (double)32767.)
      t = (double)32767.;
    else if (t < (double)-32768.)
      t = (double)-32768.;
    o[i] = t;
    lw -= incr;
    rw += incr;
  }
}

/** Overlapp add left and right sides.
* @param l  input buffer (gets silent).
* @param r  input buffer (gets lough).
* @param o output buffer.
* @param cnt size of buffers.
*/
void OpalG711_PLC::overlapadds(short *l, short *r, short *o, int cnt) const
{
  int  i;
  double  incr, lw, rw, t;

  if (cnt == 0)
    return;
  incr = (double)1. / cnt;
  lw = (double)1. - incr;
  rw = incr;
  for (i = 0; i < cnt; i++) {
    t = lw * l[i] + rw * r[i];
    if (t > (double)32767.)
      t = (double)32767.;
    else if (t < (double)-32768.)
      t = (double)-32768.;
    o[i] = (short)t;
    lw -= incr;
    rw += incr;
  }
}

/** Estimate the pitch.
*/
int OpalG711_PLC::findpitch()
{
  /** decimation for correlation. default is 2:1 at 8000 Hz */
  int corr_ndec = rate/CORR_DECIMATION_RATE;   
  /** minimum power */
  double corr_minpower = CORR_MINPOWER * ms2samples(corr_len_ms) / corr_ndec;

  int  i, j, k;
  int  bestmatch;
  double  bestcorr;
  double  corr;    /**< correlation */
  double  energy;    /**< running energy */
  double  scale;    /**< scale correlation by average power */
  double  *rp;    /**< segment to match */
  /** l - pointer to first sample in last 20 msec of speech. */
  double  *l = pitch_buf + (hist_len - ms2samples(corr_len_ms));
  /** r - points to the sample pitch_max before l. */
  double  *r = pitch_buf + (hist_len - ms2samples(corr_len_ms) - pitch_max);

  /* coarse search */
  rp = r;
  energy = (double)0.;
  corr = (double)0.;
  for (i = 0; i < ms2samples(corr_len_ms); i += corr_ndec) {
    energy += rp[i] * rp[i];
    corr += rp[i] * l[i];
  }
  scale = energy;
  if (scale < corr_minpower)
    scale = corr_minpower;
  corr = corr / (double)sqrt(scale);
  bestcorr = corr;
  bestmatch = 0;
  for (j = corr_ndec; j <= (pitch_max-pitch_min); j += corr_ndec) {
    energy -= rp[0] * rp[0];
    energy += rp[ms2samples(corr_len_ms)] * rp[ms2samples(corr_len_ms)];
    rp += corr_ndec;
    corr = 0.f;
    for (i = 0; i < ms2samples(corr_len_ms); i += corr_ndec)
      corr += rp[i] * l[i];
    scale = energy;
    if (scale < corr_minpower)
      scale = corr_minpower;
    corr /= (double)sqrt(scale);
    if (corr >= bestcorr) {
      bestcorr = corr;
      bestmatch = j;
    }
  }

  /* fine search */
  j = bestmatch - (corr_ndec - 1);
  if (j < 0)
    j = 0;
  k = bestmatch + (corr_ndec - 1);
  if (k > (pitch_max-pitch_min))
    k = (pitch_max-pitch_min);
  rp = &r[j];
  energy = 0.f;
  corr = 0.f;
  for (i = 0; i < ms2samples(corr_len_ms); i++) {
    energy += rp[i] * rp[i];
    corr += rp[i] * l[i];
  }
  scale = energy;
  if (scale < corr_minpower)
    scale = corr_minpower;
  corr = corr / (double)sqrt(scale);
  bestcorr = corr;
  bestmatch = j;
  for (j++; j <= k; j++) {
    energy -= rp[0] * rp[0];
    energy += rp[ms2samples(corr_len_ms)] * rp[ms2samples(corr_len_ms)];
    rp++;
    corr = 0.f;
    for (i = 0; i < ms2samples(corr_len_ms); i++)
      corr += rp[i] * l[i];
    scale = energy;
    if (scale < corr_minpower)
      scale = corr_minpower;
    corr = corr / (double)sqrt(scale);
    if (corr > bestcorr) {
      bestcorr = corr;
      bestmatch = j;
    }
  }
  return pitch_max - bestmatch;
}

/** converts short into double values.
* @param f input buffer of type short.
* @param t output buffer of type double.
* @param cnt size of buffers.
*/
void OpalG711_PLC::convertsf(short *f, double *t, int cnt) const
{
  int  i;
  for (i = 0; i < cnt; i++)
    t[i] = (double)f[i];
}

/** converts double into short values.
* @param f input buffer of type double.
* @param t output buffer of type short.
* @param cnt size of buffers.
*/
void OpalG711_PLC::convertfs(double *f, short *t, int cnt) const
{
  int  i;
  for (i = 0; i < cnt; i++)
    t[i] = (short)f[i];
}

#endif // OPAL_G711PLC
