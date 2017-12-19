/*
 * VoIPcodecs - a series of DSP components for telephony
 *
 * dc_restore.h - General telephony routines to restore the zero D.C.
 *                level to audio which has a D.C. bias.
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2001 Steve Underwood
 *
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: dc_restore.h,v 1.18 2007/04/08 08:16:17 steveu Exp $
 */

/*! \file */

#if !defined(_SPANDSP_DC_RESTORE_H_)
#define _SPANDSP_DC_RESTORE_H_

/*! \page dc_restore_page Removing DC bias from a signal

\section dc_restore_page_sec_1 What does it do?

Telecoms signals often contain considerable DC, but DC upsets a lot of signal
processing functions. Placing a zero DC restorer at the front of the processing
chain can often simplify the downstream processing. 

\section dc_restore_page_sec_2 How does it work?

The DC restorer uses a leaky integrator to provide a long-ish term estimate of
the DC bias in the signal. A 32 bit estimate is used for the 16 bit audio, so
the noise introduced by the estimation can be keep in the lower bits, and the 16
bit DC value, which is subtracted from the signal, is fairly clean. The
following code fragment shows the algorithm used. dc_bias is a 32 bit integer,
while the sample and the resulting clean_sample are 16 bit integers. 

    dc_bias += ((((int32_t) sample << 15) - dc_bias) >> 14);
    clean_sample = sample - (dc_bias >> 15); 
*/

/*!
    Zero DC restoration descriptor. This defines the working state for a single
    instance of DC content filter.
*/
typedef struct
{
    int32_t state;
} dc_restore_state_t;

#if defined(__cplusplus)
extern "C"
{
#endif

static __inline__ void dc_restore_init(dc_restore_state_t *dc)
{
    dc->state = 0;
}
/*- End of function --------------------------------------------------------*/

static __inline__ int16_t dc_restore(dc_restore_state_t *dc, int16_t sample)
{
    dc->state += ((((int32_t) sample << 15) - dc->state) >> 14);
    return (int16_t) (sample - (dc->state >> 15));
}
/*- End of function --------------------------------------------------------*/

static __inline__ int16_t dc_restore_estimate(dc_restore_state_t *dc)
{
    return (int16_t) (dc->state >> 15);
}
/*- End of function --------------------------------------------------------*/

static __inline__ int16_t saturate(int32_t amp)
{
    int16_t amp16;

    /* Hopefully this is optimised for the common case - not clipping */
    amp16 = (int16_t) amp;
    if (amp == amp16)
        return amp16;
    if (amp > INT16_MAX)
        return  INT16_MAX;
    return  INT16_MIN;
}
/*- End of function --------------------------------------------------------*/

#ifdef _MSC_VER
__inline float rintf (float flt)
{
	_asm
	{	fld flt
		frndint
	}
}

__inline double rint(double dbl)
{
    __asm 
	{
        fld dbl
        frndint
    }
}

__inline long lrintf (float flt)
{
	long retval;
	_asm
	{	fld flt
		fistp retval
	}
	return retval;
}
#endif

static __inline__ int16_t fsaturatef(float famp)
{
    if (famp > 32767.0)
        return  INT16_MAX;
    if (famp < -32768.0)
        return  INT16_MIN;
    return (int16_t) rintf(famp);
}
/*- End of function --------------------------------------------------------*/

static __inline__ int16_t fsaturate(double damp)
{
    if (damp > 32767.0)
        return  INT16_MAX;
    if (damp < -32768.0)
        return  INT16_MIN;
    return (int16_t) rint(damp);
}
/*- End of function --------------------------------------------------------*/

#if defined(__cplusplus)
}
#endif

#endif
/*- End of file ------------------------------------------------------------*/
