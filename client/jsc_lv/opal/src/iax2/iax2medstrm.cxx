/*
 *
 * Inter Asterisk Exchange 2
 * 
 * Implementation of the class that connects media to Opal and IAX2
 * 
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2005 Indranet Technologies Ltd.
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
 * The Initial Developer of the Original Code is Indranet Technologies Ltd.
 *
 * The author of this code is Derek J Smithies
 *
 * $Revision: 22710 $
 * $Author: dereksmithies $
 * $Date: 2009-05-25 03:17:57 +0000 (Mon, 25 May 2009) $
 */

#include <ptlib.h>
#include <opal/buildopts.h>

#if OPAL_IAX2

#ifdef __GNUC__
#pragma implementation "iax2medstrm.h"
#endif

#include <iax2/iax2medstrm.h>

#include <iax2/frame.h>
#include <iax2/iax2con.h>
#include <lids/lid.h>
#include <opal/mediastrm.h>
#include <opal/patch.h>
#include <ptlib/videoio.h>
#include <rtp/rtp.h>
#include <codec/vidcodec.h>


#define MAX_PAYLOAD_TYPE_MISMATCHES 10


#define new PNEW


///////////////////////////////////////////////////////////////////////////////

OpalIAX2MediaStream::OpalIAX2MediaStream(IAX2Connection & conn, 
                                  const OpalMediaFormat & mediaFormat,
					 unsigned sessionID,   
					 PBoolean isSource)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource),
    connection(conn)
{
  PTRACE(6, "Media\tCreate OpalIAX2MediaStream-" 
	 << mediaFormat 
	 << (IsSource() ? "source" : "sink"));
}
 
OpalIAX2MediaStream::~OpalIAX2MediaStream()
{
  Close();
  PTRACE(6, "Media\tDestroy OpalIAX2MediaStream");
}
 
PBoolean OpalIAX2MediaStream::Open()
{
  if (IsOpen())
    return PTrue;

  PBoolean res = OpalMediaStream::Open();
  PTRACE(3, "Media\t" << *this << " is now open");
 
  return res;
}
 
PBoolean OpalIAX2MediaStream::Start()
{
  PTRACE(2, "Media\t" << *this << " Run ::Start");

  return OpalMediaStream::Start();
}
  
PBoolean OpalIAX2MediaStream::Close()
{
  PBoolean res = OpalMediaStream::Close();

  PTRACE(3, "Media\t" << *this << " is now closed"); 
  return res;
}
 
 
PBoolean OpalIAX2MediaStream::ReadPacket(RTP_DataFrame & packet)
{
  PTRACE(5, "Media\tRead media compressed audio packet from the iax2 connection");

  if (IsSink()) {
    PTRACE(1, "Media\tTried to read from sink media stream");
    return PFalse;
  }

  if (!IsOpen()) {
    PTRACE(3, "Media\tStream has been closed, so exit now");
    return PFalse;
  }
    
  PBoolean success = connection.ReadSoundPacket(packet); 

  return success;
}

PBoolean OpalIAX2MediaStream::IsSynchronous() const
{
  return IsSource();
}
/////////////////////////////////////////////////////////////////////////////

// This routine takes data from the source (eg mic) and sends the data to the remote host.
PBoolean OpalIAX2MediaStream::WriteData(const BYTE * buffer, PINDEX length, PINDEX & written)
{
  written = 0;
  if (IsSource()) {
    PTRACE(1, "Media\tTried to write to source media stream");
    return PFalse;
  }
  PTRACE(5, "Media\tSend data to the network : have " 
	 << length << " bytes to send to remote host");
  PBYTEArray *sound = new PBYTEArray(buffer, length);
  written = length;
  connection.PutSoundPacketToNetwork(sound);

  return PTrue;
}


#endif // OPAL_IAX2

/////////////////////////////////////////////////////////////////////////////


/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 2 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-basic-offset:2
 * End:
 */
