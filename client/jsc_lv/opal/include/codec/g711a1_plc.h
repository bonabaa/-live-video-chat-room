/*
 * g711a1_plc.h
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

#ifndef OPAL_CODEC_G711A1_PLC_H
#define OPAL_CODEC_G711A1_PLC_H

#ifndef SBC_DISABLE_PTLIB
#include <opal/buildopts.h>
#endif

#if OPAL_G711PLC

#include <string.h>

/** concealment state variables.
 * This class contains states and code of my concealment algorithm.
 */

class OpalG711_PLC
{
  private:
    enum {
      LOSS_PERIOD1=10,            /**< the first period of loss lasts 10 ms, in which the signal is not attenuated. (ITU G.711 I.2.6) */
      LOSS_PERIOD2start=20,       /**< first samples of loss period 2. */
      LOSS_PERIOD2overlap=21,     /**< period, in which concealment from loss period 1 and loss period 2 still overlap. */
      LOSS_PERIOD2=22,            /**< the second period of loss lasts 50 ms, in which the signal is attenuated. (ITU G.711 I.2.6) */
      LOSS_PERIOD3=30,            /**< after 60 ms of loss, the signal is zero (ITU G.711 I.2.6) */
      TRANSITION=40,              /**< the first parts of good samples are mixed with the concealed samples */
      NOLOSS=0                    /**< no loss of samples, only good samples */
    } mode;                       /**< current mode of operation */

    int      conceal_count;       /**< consecutive erased samples [samples] */
    
    short  * transition_buf;      /**< buffer containing the concealed frame */
    int      transition_len;      /**< length of the transition period [samples] */
    int      transition_count;    /**< counting the transition samples [samples] */
    
    int      hist_len;            /**< history buffer length [samples]*/
    short  * hist_buf;            /**< history buffer (ring buffer)*/
    short  * tmp_buf;             /**< history buffer (ring buffer)*/

    short *  conceal_overlapbuf;  /**< store overlapping speech segments */

    double * pitch_buf;           /**< buffer for cycles of speech */
    double * pitch_lastq;         /**< saved last quarter wavelengh */

    int      pitch_min;           /**< minimum allowed pitch. default 200 Hz [samples] */
    int      pitch_max;           /**< maximum allowed pitch. default 66 Hz [samples] */
    int      pitch_overlap;       /**< overlap based on pitch [samples] */
    int      pitch_offset;        /**< offset into pitch period [samples]*/
    int      pitch;               /**< pitch estimate [samples] */
    int      pitch_blen;          /**< current pitch buffer length [samples] */
    int      pitch_overlapmax;    /**< maximum pitch OLA window [samples] */
       
    int      rate;                /**< sampling rate [samples/second] */

    int ms2samples(int ms) const  /**< converts ms unit into sample unit */
    {
      return ms*rate/1000;
    }


   public:
    OpalG711_PLC(int rate=8000, double pitch_low=66.6, double pitch_high=200);    /**< constructor */
    ~OpalG711_PLC();

    void dofe(short *s, int size);         /**< synthesize speech for erasure */
    void addtohistory(short *s, int size);       /**< add a good frame to history buffer */
    int getAlgDelay() const { return pitch_overlapmax; };  /**< return the algorithmic delay of the algorithm [samples] */
    void drop(short *s, int size);               /**< the previous frame has to be dropped and the playout is increased */

   private:
    void scalespeech(short *inout, int sz, bool decay=true) const;
    void getfespeech(short *out, int sz);
    void hist_savespeech(short *s, int size);
    int findpitch();
    void overlapadd(double *l, double *r, double *o, int cnt) const;
    void overlapadds(short *l, short *r, short *o, int cnt) const;
    void overlapaddatend(short *s, short *f, int start, int end, int count) const;
    void convertsf(short *f, double *t, int cnt) const;
    void convertfs(double *f, short *t, int cnt) const;

    inline void copy(double *f, double *t, int cnt) const { memmove(t,f,cnt*sizeof(double)); };
    inline void copy(short *f, short *t, int cnt) const { memmove(t,f,cnt*sizeof(short)); };
    inline void zero(short *s, int cnt) const { memset(s, 0, cnt*sizeof(short)); };

    int dofe_partly(short *out, int size);
};


#endif // OPAL_G711PLC

#endif // OPAL_CODEC_G711A1_PLC_H
