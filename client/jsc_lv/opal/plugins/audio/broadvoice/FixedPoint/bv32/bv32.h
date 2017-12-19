/*****************************************************************************/
/* BroadVoice(R)32 (BV32) Fixed-Point ANSI-C Source Code                     */
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
  bv32.h : 

  $Log$
******************************************************************************/

extern void Reset_BV32_Encoder(
struct BV32_Encoder_State *cs);

extern void BV32_Encode(
struct BV32_Bit_Stream *bs,
struct BV32_Encoder_State *cs,
Word16 *inx);

extern void Reset_BV32_Decoder(
struct BV32_Decoder_State *ds);

extern void BV32_Decode(
struct BV32_Bit_Stream     *bs,
struct BV32_Decoder_State  *ds,
Word16 *out);

extern void BV32_PLC(
struct BV32_Decoder_State   *ds,
Word16 *out);

