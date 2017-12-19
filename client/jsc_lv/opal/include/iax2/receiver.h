/*
 *
 *
 * Inter Asterisk Exchange 2
 * 
 * The entity which receives all packets for all calls.
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
 * $Revision: 21293 $
 * $Author: rjongbloed $
 * $Date: 2008-10-12 23:24:41 +0000 (Sun, 12 Oct 2008) $
 */

#ifndef OPAL_IAX2_RECEIVER_H
#define OPAL_IAX2_RECEIVER_H

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal/buildopts.h>

#if OPAL_IAX2

#include <ptlib/sockets.h>

#ifdef P_USE_PRAGMA
#pragma interface
#endif

class IAX2EndPoint;
class IAX2Frame;
class IAX2FrameList;
class IAX2PacketIdList;

#include <iax2/frame.h>

/**Manage the reception of etherenet packets on the specified port.
   All received packets are handed to the appropriate connection.
   A separate thread is used to wait on the ethernet port*/
class IAX2Receiver : public PThread
{ 
  PCLASSINFO(IAX2Receiver, PThread);
 public:
  /**@name Constructors/destructors*/
  //@{
  /**Construct a receiver, given references to the endpoint and socket*/
  IAX2Receiver(IAX2EndPoint & _newEndpoint, PUDPSocket & _newSocket);
  
  /**Destroy the receiver*/
  ~IAX2Receiver();
  //@}
  
  /**@name general worker methods*/
  //@{
  /*The method which the receiver thread invokes*/
  virtual void Main();

  /**Close down this thread in a civilised fashion, by sending an empty packet
     to the PUDPSocket of this protocol. The Receiver will receive this
     packet, and check the close down flag, and so realise it is time to close
     down. */
  virtual void Terminate();
  
  /**Sit in here, waiting for data on the socket*/
  PBoolean ReadNetworkSocket();
  
  /**We have just read a frame from the network. This is a good
     IAX2Frame. Put it on the queue of frames to be processed by the
     IAX2EndPoint.  The IAX2EndPoint will give this frame to the
     appropriate IAXConnection. Since this frame could be encrypted,
     and we do not have access to the keys (only the IAX2Connection
     has the keys, we cannot do anymore with the frame). Indeed, we
     are the receiving thread, and must put all our time into reading
     from the socket, not processing the packets. */
  void AddNewReceivedFrame(IAX2Frame *newFrame);

  /**Report how many frames are in the receive queue, waiting for extraction*/
  PINDEX GetSize() { return fromNetworkFrames.GetSize(); }

  //@}
 protected:
  /**Global variable which holds the application specific data */
  IAX2EndPoint &endpoint;
  
  /**Socket that is used to receive all network data from */
  PUDPSocket & sock;
  
  /**The act of processing a header will (inevitably) create  additional
     frames as trunked frames are split up */
  IAX2FrameList      fromNetworkFrames;
  
  /**Flag to indicate if this receiver thread should keep listening for network data */
  PBoolean           keepGoing;
};


#endif // OPAL_IAX2

#endif // OPAL_IAX2_RECEIVER_H

/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 2 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-basic-offset:2
 * End:
 */
