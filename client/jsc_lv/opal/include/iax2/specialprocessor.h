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

#ifndef OPAL_IAX2_SPECIALPROCESSOR_H
#define OPAL_IAX2_SPECIALPROCESSOR_H

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal/buildopts.h>

#if OPAL_IAX2

#include <opal/connection.h>

#include <iax2/processor.h>
#include <iax2/frame.h>
#include <iax2/iedata.h>
#include <iax2/remote.h>
#include <iax2/safestrings.h>
#include <iax2/sound.h>

/**This is the special processor which is created to handle the weirdo iax2 packets
   that are sent outside of a particular call. Examples of weirdo packets are the
   ping/pong/lagrq/lagrp.
  */
class IAX2SpecialProcessor : public IAX2Processor
{
  PCLASSINFO(IAX2SpecialProcessor, IAX2Processor);
  
 public:
  /**Construct this class */
  IAX2SpecialProcessor(IAX2EndPoint & ep);

  /**Destructor */
  virtual ~IAX2SpecialProcessor();
  
 protected:
  /**Go through the three lists for incoming data (ethernet/sound/UI commands.  */
  virtual void ProcessLists();
  
  /**Processes a full frame*/
  virtual void ProcessFullFrame(IAX2FullFrame & fullFrame);
  
  /**Processes are mini frame*/
  virtual void ProcessNetworkFrame(IAX2MiniFrame * src);
  
  /**Print information about the class on to a stream*/
  virtual void PrintOn(ostream & strm) const;
  
  /**Called when there is no response to a request*/
  virtual void OnNoResponseTimeout();
  
  /**Process an IAX2FullFrameProtocol. This special processor handles
   things relative to special needs of full frame protocols.*/
  virtual PBoolean ProcessNetworkFrame(IAX2FullFrameProtocol * src);
  
  /**Process a poke command*/
  void ProcessIaxCmdPoke(IAX2FullFrameProtocol * src);

  /**Test the sequence number of the incoming frame. This is only
     valid for handling a call. If the message is outof order, the
     supplied fullframe is deleted.

     @return false always, as the special packet handler never gets
     frame ordered correctly - there will always be skipped frames. */
  virtual PBoolean IncomingMessageOutOfOrder(IAX2FullFrame *) 
  { return PFalse; }


};


#endif // OPAL_IAX2

#endif // OPAL_IAX2_SPECIALPROCESSOR_H
