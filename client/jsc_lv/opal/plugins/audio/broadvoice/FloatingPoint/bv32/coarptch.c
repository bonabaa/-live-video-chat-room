/*****************************************************************************/
/* BroadVoice(R)32 (BV32) Floating-Point ANSI-C Source Code                  */
/* Revision Date: November 13, 2009                                          */
/* Version 1.1                                                               */
/*****************************************************************************/

/*****************************************************************************/
/* Copyright 2000-2009 Broadcom Corporation                                  */
/*                                                                           */
/* This software is provided under the GNU Lesser General Public License,    */
/* version 2.1, as published by the Free Software Foundation ("LGPL").       */
/* This program is distributed in the hope that it will be useful, but       */
/* WITHOUT ANY SUPPORT OR WARRANTY; without even the implied warranty of     */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the LGPL for     */
/* more details.  A copy of the LGPL is available at                         */
/* http://www.broadcom.com/licenses/LGPLv2.1.php,                            */
/* or by writing to the Free Software Foundation, Inc.,                      */
/* 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.                 */
/*****************************************************************************/


/*****************************************************************************
  coarptch.c: Coarse pitch search

  $Log$
******************************************************************************/

#include <stdlib.h>			/* abs() */
#include "typedef.h"
#include "bv32cnst.h"
#include "utility.h"
#include "bvcommon.h"
#include "bv32externs.h"

int coarsepitch(	/* in the undecimated domain Q3 */
                Float	*xw,
                Float	*xwdm,
                Float	*dfm,		/* (i/o) ellipse low pass filter memory */
                int	cpplast)		/* in the undecimated domain Q3 */
{
   
   Float	xwd[LXD];
   Float _cor[MAXPPD1+1], _cor2[MAXPPD1+1], _energy[MAXPPD1+1];
   Float	*cor, *cor2, *energy;
   Float cor2i[MAX_NPEAKS], energyi[MAX_NPEAKS];
   Float tmp[DFO+FRSZ], threshold;
   Float *fp0, *fp1, *fp2, *fp3, s, t, a, b, c, deltae;
   Float cor2max, energymax, cor2m, energym, ci, eni;
   int	cpp, maxdev, plag[MAX_NPEAKS], mplth;	/* in the undecimated domain Q3 */
   int   i, j, k, n, npeaks, imax, im, idx[HMAXPPD];
   int   flag, mpflag;
   
   cor = _cor+1;
   cor2 = _cor2+1;
   energy = _energy+1;
   
   /* LOWPASS FILTER xw() TO 800 Hz; SHIFT & OUTPUT INTO xwd() */
   
   /* load xwd[] buffer memory */
   Fcopy(xwd, xwdm, XDOFF);
   
   /* copy memory to temp buffer */
   fp1 = tmp; fp2 = dfm;
   for (i=0;i<DFO;i++) *fp1++ = *fp2++; 
   
   /* AP and AZ filtering and decimation */
   fp0 = xwd+XDOFF;
   fp3 = xw;
   for (i=0;i<FRSZD;i++) {
      for (k=0;k<DECF;k++) {
         t = *fp3++; fp2 = fp1-1;
         for (j=0;j<DFO;j++) t -= adf[j+1]*(*fp2--);
         *fp1++ = t;
      }
      fp2 = fp1-1;
      t = bdf[0] * (*fp2--);
      for (j=0;j<DFO;j++) t += bdf[j+1] * (*fp2--); 
      *fp0++ = t;
   }
   
   /* copy temp buffer to memory */
   fp1 -= DFO;
   for (i=0;i<DFO;i++) dfm[i] = *fp1++;
   Fcopy(xwdm, xwd+FRSZD, XDOFF);
   
   /* COMPUTE CORRELATION & ENERGY OF PREDICTION BASIS VECTOR */
   
   fp0 = xwd+MAXPPD1;
   fp1 = xwd+MAXPPD1-M1;
   s = t = 0.; 
   for (i=0;i<(LXD-MAXPPD1);i++) {
      s += (*fp1) * (*fp1);
      t += (*fp0++)*(*fp1++);
   }
   if (s<1.e-10) { s = t = 0.; }
   energy[M1-1] = s;
   cor[M1-1] = t;
   if (t > 0.0F)
      cor2[M1-1] = t*t;
   else
      cor2[M1-1] = -t*t;
   
   
   fp2 = xwd+LXD-M1-1;
   fp3 = xwd+MAXPPD1-M1-1;
   
   for (i=M1;i<M2;i++) {
      fp0 = xwd+MAXPPD1;
      fp1 = xwd+MAXPPD1-i-1;
      t = 0.;
      for (j=0;j<(LXD-MAXPPD1);j++) t += (*fp0++) * (*fp1++); 
      s = s - (*fp2)*(*fp2) + (*fp3)*(*fp3);
      if (s<1.e-10) { s = t = 0.; }
      cor[i] = t;
      if (t > 0.0F)
         cor2[i] = t*t;
      else
         cor2[i] = -t*t;
      fp2--; fp3--;
      energy[i] = s;
   }
   
   /* FIND POSITIVE COR*COR/ENERGY PEAKS */
   npeaks = 0;
   n = MINPPD-1;
   while ((npeaks<MAX_NPEAKS) && (n<MAXPPD)) {
      if ((cor2[n]*energy[n-1]>cor2[n-1]*energy[n]) && 
         (cor2[n]*energy[n+1]>cor2[n+1]*energy[n]) && (cor2[n]>0)) {
         idx[npeaks] = n;
         npeaks++;
      }
      n++;
   }
   
   /* RETURN EARLY IF THERE IS NO PEAK OR ONLY ONE PEAK */
   if (npeaks == 0)   /* if there are no positive peak, */
      return MINPPD*cpp_scale; /* return minimum pitch period */
   if (npeaks == 1)   /* if there is exactly one peak, */
      return (idx[0]+1)*cpp_scale; /* return the time lag for this peak */
   
   /* IF PROGRAM PROCEEDS TO HERE, THERE ARE 2 OR MORE PEAKS */
   cor2max=-1e30;
   energymax=1.0F;
   imax = 0;
   for (i=0; i < npeaks; i++) {
      
   /* USE QUADRATIC INTERPOLATION TO FIND THE INTERPOLATED cor[] AND
      energy[] CORRESPONDING TO INTERPOLATED PEAK OF cor2[]/energy[] */
      /* first calculate coefficients of y(x)=ax^2+bx+c; */
      n=idx[i];
      a=0.5F*(cor[n+1] + cor[n-1]) - cor[n];
      b=0.5F*(cor[n+1] - cor[n-1]);
      c=cor[n];
      
      /* INITIALIZE VARIABLES BEFORE SEARCHING FOR INTERPOLATED PEAK */
      im=0;
      cor2m=cor2[n];
      energym=energy[n];
      eni=energy[n];
      
      /* DERTERMINE WHICH SIDE THE INTERPOLATED PEAK FALLS IN, THEN
      DO THE SEARCH IN THE APPROPRIATE RANGE */
      if (cor2[n+1]*energy[n-1] > cor2[n-1]*energy[n+1]) { /* if right side */
         deltae=(energy[n+1] - eni) * INVDECF; /*increment for linear interp.*/
         for (k = 0; k < HDECF; k++) {
            ci = a*x2[k] + b*x[k] + c; /* quadratically interpolated cor[] */
            eni += deltae;             /* linearly interpolated energy[] */
            if (ci*ci*energym > cor2m*eni) {
               im = k+1;
               cor2m=ci*ci;
               energym=eni;
            }
         }        
      } else {    /* if interpolated peak is on the left side */
         deltae=(energy[n-1] - eni) * INVDECF; /*increment for linear interp.*/
         for (k = 0; k < HDECF; k++) {
            ci = a*x2[k] - b*x[k] + c;
            eni += deltae;
            if (ci*ci*energym > cor2m*eni) {
               im = -k-1;
               cor2m=ci*ci;
               energym=eni;
            }
         }        
      }
      
      /* SEARCH DONE; ASSIGN cor2[] AND energy[] CORRESPONDING TO 
      INTERPOLATED PEAK */ 
      plag[i]=(idx[i]+1)*cpp_scale+im; 		/* lag of interp. peak */
      cor2i[i]=cor2m; /* interpolated cor2[] of i-th interpolated peak */
      energyi[i]=energym; /* interpolated energy[] of i-th interpolated peak */
      
      /* SEARCH FOR GLOBAL MAXIMUM OF INTERPOLATED cor2[]/energy[] peak */
      if (cor2m*energymax > cor2max*energym) {
         imax=i;
         cor2max=cor2m;
         energymax=energym;
      }
   }
   cpp=plag[imax];	/* first candidate for coarse pitch period */
   mplth=plag[npeaks-1]; /* set mplth to the lag of last peak */
   
   /* FIND THE LARGEST PEAK (IF THERE IS ANY) AROUND THE LAST PITCH */
   maxdev=(int) (DEVTH*cpplast); /* maximum deviation from last pitch */
   im = -1;
   cor2m = -1e30;
   energym = 1.0F;
   for (i=0;i<npeaks;i++) {  /* loop thru the peaks before the largest peak */
      if (abs(plag[i]-cpplast) <= maxdev) {
         if (cor2i[i]*energym > cor2m*energyi[i]) {
            im=i;
            cor2m=cor2i[i];
            energym=energyi[i];
         }	
      }
   } /* if there is no peaks around last pitch, then im is still -1 */
   
   /* NOW SEE IF WE SHOULD PICK ANY ALTERNATICE PEAK */
   /* FIRST, SEARCH FIRST HALF OF PITCH RANGE, SEE IF ANY QUALIFIED PEAK
   HAS LARGE ENOUGH PEAKS AT EVERY MULTIPLE OF ITS LAG */
   i=0;
   while (plag[i] < 0.5*mplth) {
      
      /* DETERMINE THE APPROPRIATE THRESHOLD FOR THIS PEAK */
      if (i != im) {  /* if not around last pitch, */
         threshold = TH1;    /* use a higher threshold */
      } else {        /* if around last pitch */
         threshold = TH2;    /* use a lower threshold */
      }
      
      /* IF THRESHOLD EXCEEDED, TEST PEAKS AT MULTIPLES OF THIS LAG */
      if (cor2i[i]*energymax > threshold*cor2max*energyi[i]) { 
         flag=1;  
         j=i+1;
         k=0;
         s=2.0F*plag[i]; /* initialize t to twice the current lag */
         while (s <= mplth) { /* loop thru all multiple lag <= mplth */
            mpflag=0;   /* initialize multiple pitch flag to 0 */
            a=MPR1*s;   /* multiple pitch range lower bound */
            b=MPR2*s;   /* multiple pitch range upper bound */
            while (j < npeaks) { /* loop thru peaks with larger lags */
               if (plag[j] > b) { /* if range exceeded, */
                  break;          /* break the innermost while loop */
               }       /* if didn't break, then plag[j] <= b */
               if (plag[j] > a) { /* if current peak lag within range, */
                  /* then check if peak value large enough */
                  if (k < 4) {
                     c = MPTH[k];
                  } else {
                     c = MPTH4;
                  }
                  if (cor2i[j]*energymax > c*cor2max*energyi[j]) {
                     mpflag=1; /* if peak large enough, set mpflag, */
                     break; /* and break the innermost while loop */
                  } 
               }
               j++;
            }
            /* if no qualified peak found at this multiple lag */
            if (mpflag == 0) { 
               flag=0;     /* disqualify the lag plag[i] */
               break;      /* and break the while (s<=mplth) loop */
            }
            k++;
            s += plag[i]; /* update s to the next multiple pitch lag */
         }
         /* if there is a qualified peak at every multiple of plag[i], */
         if (flag == 1) { 
            return plag[i];         /* and return to calling function */
         }
      }       
      i++;
      if (i == npeaks)
         break;      /* to avoid out of array bound error */
   }
   
   /* IF PROGRAM PROCEEDS TO HERE, NONE OF THE PEAKS WITH LAGS < 0.5*mplth
   QUALIFIES AS THE FINAL PITCH. IN THIS CASE, CHECK IF
   THERE IS ANY PEAK LARGE ENOUGH AROUND LAST PITCH.  IF SO, USE ITS
   LAG AS THE FINAL PITCH. */
   if (im != -1) {   /* if there is at least one peak around last pitch */
      if (im == imax) { /* if this peak is also the global maximum, */
         return cpp;   /* return first pitch candidate at global max */
      }
      if (im < imax) { /* if lag of this peak < lag of global max, */
         if (cor2m*energymax > LPTH2*cor2max*energym) { 
            if (plag[im] > HMAXPPD*cpp_scale) {
               return plag[im];
            }
            for (k=2; k<=5;k++) { /* check if current candidate pitch */
               s=plag[imax]/(float)(k); /* is a sub-multiple of */
               a=SMDTH1*s;             /* the time lag of */
               b=SMDTH2*s;             /* the global maximum peak */
               if (plag[im]>a && plag[im]<b) {     /* if so, */
                  return plag[im];         /* and return as pitch */
               }
            }
         }
      } else {           /* if lag of this peak > lag of global max, */
         if (cor2m*energymax > LPTH1*cor2max*energym) { 
            return plag[im];         /* accept its lag */ 
         }
      }
   }
   
   /* IF PROGRAM PROCEEDS TO HERE, WE HAVE NO CHOICE BUT TO ACCEPT THE
   LAG OF THE GLOBAL MAXIMUM */
   return cpp;
   
}
