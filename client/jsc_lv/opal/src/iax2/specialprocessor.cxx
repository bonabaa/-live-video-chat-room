/*
 * Inter Asterisk Exchange 2
 * 
 * The entity which receives all manages weirdo iax2 packets that are 
 * sent outside of a regular call.
 * 
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2006 Stephen Cook 
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
 * The Initial Developer of the Original Code is Indranet Technologies Ltd
 *
 * The author of this code is Stephen Cook
 *
 * $Revision: 21293 $
 * $Author: rjongbloed $
 * $Date: 2008-10-12 23:24:41 +0000 (Sun, 12 Oct 2008) $
 */

#include <ptlib.h>
#include <opal/buildopts.h>

#if OPAL_IAX2

#include <typeinfo>

#ifdef P_USE_PRAGMA
#pragma implementation "specialprocessor.h"
#endif

#include <iax2/causecode.h>
#include <iax2/frame.h>
#include <iax2/iax2con.h>
#include <iax2/iax2ep.h>
#include <iax2/iax2medstrm.h>
#include <iax2/ies.h>
#include <iax2/processor.h>
#include <iax2/callprocessor.h>
#include <iax2/sound.h>
#include <iax2/transmit.h>

#define new PNEW
 
IAX2SpecialProcessor::IAX2SpecialProcessor(IAX2EndPoint & ep)
 : IAX2Processor(ep)
{
  Resume();
}

IAX2SpecialProcessor::~IAX2SpecialProcessor()
{
}
  
void IAX2SpecialProcessor::ProcessLists()
{
  while(ProcessOneIncomingEthernetFrame()) {
    ;
  }
}

void IAX2SpecialProcessor::PrintOn(ostream & strm) const
{
  strm << "Special call processor";
}

void IAX2SpecialProcessor::OnNoResponseTimeout()
{
}
 
void IAX2SpecialProcessor::ProcessFullFrame(IAX2FullFrame & fullFrame)
{
  switch(fullFrame.GetFrameType()) {
  case IAX2FullFrame::iax2ProtocolType:        
    PTRACE(3, "Build matching full frame    fullFrameProtocol");
    ProcessNetworkFrame(new IAX2FullFrameProtocol(fullFrame));
    break;
    
  default:
    PTRACE(3, "Build matching full frame, Type not expected");
  }
}

void IAX2SpecialProcessor::ProcessNetworkFrame(IAX2MiniFrame * src)
{
  PTRACE(1, "unexpected Mini Frame");
  delete src;
}

PBoolean IAX2SpecialProcessor::ProcessNetworkFrame(IAX2FullFrameProtocol * src)
{
  PTRACE(3, "ProcessNetworkFrame(IAX2FullFrameProtocol * src)");
  src->CopyDataFromIeListTo(ieData);
  
  //check if the common method can process it?
  if (IAX2Processor::ProcessNetworkFrame(src))
      return PTrue;
  
  switch (src->GetSubClass()) {
    case IAX2FullFrameProtocol::cmdPoke:
      ProcessIaxCmdPoke(src);
      break;
    default:
      PTRACE(1, "Process Full Frame Protocol, Type not expected");
      SendUnsupportedFrame(src);
      return PFalse;
  }
  
  return PTrue;
}

void IAX2SpecialProcessor::ProcessIaxCmdPoke(IAX2FullFrameProtocol *src)
{
  PTRACE(3, "ProcessIaxCmdPoke(IAX2FullFrameProtocol * src)");
  
  IAX2FullFrameProtocol *f = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdPong,
    IAX2FullFrameProtocol::callIrrelevant);    
  TransmitFrameToRemoteEndpoint(f);
  delete src;
}


#endif // OPAL_IAX2
