/*
 * t140.h
 *
 * Declarations for implementation of T.140 Protocol for Multimedia Application Text Conversation
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2008 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23400 $
 * $Author: csoutheren $
 * $Date: 2009-09-09 16:42:23 +0000 (Wed, 09 Sep 2009) $
 */

#ifndef OPAL_IM_T140_H
#define OPAL_IM_T140_H

#include <ptlib.h>
#include <opal/buildopts.h>
#include <opal/mediatype.h>

class OpalT140MediaType : public OpalRTPAVPMediaType 
{
  public:
    OpalT140MediaType();
  
    static const OpalMediaType & MediaType();

#if OPAL_SIP
    SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress);
#endif
};


class T140String : public PBYTEArray
{
  public:
    enum {
      ZERO_WIDTH_NO_BREAK = 0xfeff,
      UTF_NEWLINE         = 0x2028,
    };

    T140String();
    T140String(const BYTE * data, PINDEX len);
    T140String(const PBYTEArray & bytes);
    T140String(const char * chars);
    T140String(const PString & str);

    PINDEX GetLength() const { return length; }

    PINDEX GetUTFLen(WORD c);
    PINDEX GetUTF(const BYTE * ptr, PINDEX len, WORD & ch);
    PINDEX GetUTF(PINDEX pos, WORD & ch);

    PINDEX AppendUnicode16(WORD c);
    PINDEX AppendUTF(const BYTE * utf, PINDEX utfLen);

    bool AsString(PString & str);

  protected:
    PINDEX SetUTF(BYTE * ptr, WORD c);
    PINDEX length;
};

#endif // OPAL_IM_T140_H
