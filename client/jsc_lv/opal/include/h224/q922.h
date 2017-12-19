/*
 * q922.h
 *
 * Q.922 PDU implementation for the OpenH323 Project.
 *
 * Copyright (c) 2006 Network for Educational Technology, ETH Zurich.
 * Written by Hannes Friederich.
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
 * Contributor(s): ______________________________________.
 *
 * $Revision: 21346 $
 * $Author: hfriederich $
 * $Date: 2008-10-15 22:21:15 +0000 (Wed, 15 Oct 2008) $
 */

#ifndef OPAL_H224_Q922_H
#define OPAL_H224_Q922_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal/buildopts.h>

#define Q922_HEADER_SIZE 3

class Q922_Frame : public PBYTEArray
{
  PCLASSINFO(Q922_Frame, PBYTEArray);
	
public:
	
  Q922_Frame(PINDEX informationFieldSize = 260);
  ~Q922_Frame();
	
  BYTE GetHighOrderAddressOctet() const { return theArray[0]; }
  BYTE GetLowOrderAddressOctet() const { return theArray[1]; }
  void SetHighOrderAddressOctet(BYTE octet) { theArray[0] = octet; }
  void SetLowOrderAddressOctet(BYTE octet) { theArray[1] = octet; }
	
  BYTE GetControlFieldOctet() const { return theArray[2]; }
  void SetControlFieldOctet(BYTE octet) { theArray[2] = octet; }
	
  BYTE *GetInformationFieldPtr() const { return (BYTE *)(theArray + Q922_HEADER_SIZE); }
	
  PINDEX GetInformationFieldSize() const { return informationFieldSize; }
  void SetInformationFieldSize(PINDEX size);
  
  /**Decodes a Q.922 frame from a given buffer, encoded as descibed in
     H.323 Annex Q. Returns the success of this operation
    **/
  PBoolean DecodeAnnexQ(const BYTE *buffer, PINDEX size);
  
  /**Returns the size if encoded as described in H.323 Annex Q
    */
  PINDEX GetAnnexQEncodedSize() const;
  
  /** Encodes this Q.922 frame into the given buffer.
    * On return, size contains the number of octets occupied in the buffer
    */
  PBoolean EncodeAnnexQ(BYTE *buffer, PINDEX & size) const;
  
  /**Decodes a Q.922 frame from a given buffer.
     Returns the success of this operation
    */
  PBoolean DecodeHDLC(const BYTE *data, PINDEX size);
	
  /** Returns an estimate of the encoded size.
	 The receiver will use at most the size when encoding. Returns zero if encoding will fail.
   */
  PINDEX GetHDLCEncodedSize() const;
  
  /** Encodes this Q.922 frame into the given buffer.
	  On return, size contains the number of octets occupied in the buffer.
	*/
  PBoolean EncodeHDLC(BYTE *buffer, PINDEX & size) const;

  /** Encodes this Q.922 frame into the given buffer.
	  On return, size contains the number of octets occupied in the buffer.
	  Use bitPosition to determine at which bit the Q.922 FLAG sequence should begin.
	  On return, bitPosition contains the bit at which the encoded stream ends.
	  bitPosition shall be in the range 0-7, whereas 7 means that the FLAG sequence
	  is encoded at byte boundaries
	*/
  PBoolean EncodeHDLC(BYTE *buffer, PINDEX & size, BYTE & bitPosition) const;
	
protected:
	
  PINDEX informationFieldSize;
	
private:

  // for HDLC mode
  inline PBoolean FindFlagEnd(const BYTE *buffer, PINDEX bufferSize, PINDEX & octetIndex, BYTE & bitIndex);
  inline BYTE DecodeOctet(const BYTE *buffer, BYTE *destination, PINDEX & octetIndex, BYTE & bitIndex, BYTE & onesCounter);
  inline BYTE DecodeBit(const BYTE *buffer, PINDEX & octetIndex, BYTE & bitIndex);
	
  inline void EncodeOctet(BYTE octet, BYTE *buffer, PINDEX & octetIndex, BYTE & bitIndex, BYTE & onesCounter) const;
  inline void EncodeOctetNoEscape(BYTE octet, BYTE *buffer, PINDEX & octetIndex, BYTE & bitIndex) const;
  inline void EncodeBit(BYTE bit, BYTE *buffer, PINDEX & octetIndex, BYTE & bitIndex) const;
	
  inline WORD CalculateFCS(const BYTE*data, PINDEX length) const;
};

#endif // OPAL_H224_Q922_H

