/*
 * g711codec.h
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 21988 $
 * $Author: rjongbloed $
 * $Date: 2009-02-04 06:28:03 +0000 (Wed, 04 Feb 2009) $
 */

#ifndef OPAL_CODEC_G711CODEC_H
#define OPAL_CODEC_G711CODEC_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <opal/transcoders.h>
#include <codec/g711a1_plc.h>


///////////////////////////////////////////////////////////////////////////////

class Opal_G711_PCM : public OpalStreamedTranscoder {
  public:
    Opal_G711_PCM(const OpalMediaFormat & inputMediaFormat);

#if OPAL_G711PLC 
   virtual PBoolean Convert(
      const RTP_DataFrame & input,  ///<  Input data
      RTP_DataFrame & output        ///<  Output data
    );

  protected:
    OpalG711_PLC plc;
    PINDEX       lastPayloadSize;
#endif
};


///////////////////////////////////////////////////////////////////////////////

class Opal_G711_uLaw_PCM : public Opal_G711_PCM {
  public:
    Opal_G711_uLaw_PCM();
    virtual int ConvertOne(int sample) const;
    static int ConvertSample(int sample);
};


///////////////////////////////////////////////////////////////////////////////

class Opal_PCM_G711_uLaw : public OpalStreamedTranscoder {
  public:
    Opal_PCM_G711_uLaw();
    virtual int ConvertOne(int sample) const;
    static int ConvertSample(int sample);
};


///////////////////////////////////////////////////////////////////////////////

class Opal_G711_ALaw_PCM : public Opal_G711_PCM {
  public:
    Opal_G711_ALaw_PCM();
    virtual int ConvertOne(int sample) const;
    static int ConvertSample(int sample);
};


///////////////////////////////////////////////////////////////////////////////

class Opal_PCM_G711_ALaw : public OpalStreamedTranscoder {
  public:
    Opal_PCM_G711_ALaw();
    virtual int ConvertOne(int sample) const;
    static int ConvertSample(int sample);
};


///////////////////////////////////////////////////////////////////////////////

#define OPAL_REGISTER_G711() \
OPAL_REGISTER_TRANSCODER(Opal_G711_uLaw_PCM, OpalG711_ULAW_64K, OpalPCM16); \
OPAL_REGISTER_TRANSCODER(Opal_PCM_G711_uLaw, OpalPCM16,         OpalG711_ULAW_64K); \
OPAL_REGISTER_TRANSCODER(Opal_G711_ALaw_PCM, OpalG711_ALAW_64K, OpalPCM16); \
OPAL_REGISTER_TRANSCODER(Opal_PCM_G711_ALaw, OpalPCM16,         OpalG711_ALAW_64K)

#endif // OPAL_CODEC_G711CODEC_H


/////////////////////////////////////////////////////////////////////////////
