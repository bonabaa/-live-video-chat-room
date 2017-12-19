/*
 * ===================================================================
 *  TS 26.104
 *  REL-5 V5.4.0 2004-03
 *  REL-6 V6.1.0 2004-03
 *  3GPP AMR Floating-point Speech Codec
 * ===================================================================
 *
 */

/*
 * interf_dec.h
 *
 *
 * Project:
 *    AMR Floating-Point Codec
 *
 * Contains:
 *    Defines interface to AMR decoder
 *
 */

#ifndef _interf_dec_h_
#define _interf_dec_h_

/*
 * Function prototypes
 */
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Conversion from packed bitstream to endoded parameters
 * Decoding parameters to speech
 */
extern void Decoder_Interface_Decode( void *st,

#ifndef ETSI
      unsigned char *bits,

#else
      short *bits,
#endif

      short *synth, int bfi );

/*
 * Reserve and init. memory
 */
extern void *Decoder_Interface_init( void );

/*
 * Exit and free memory
 */
extern void Decoder_Interface_exit( void *state );
#ifdef __cplusplus
}
#endif

#endif // !_interf_dec_h_

