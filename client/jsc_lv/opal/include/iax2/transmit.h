/*
 *
 *
 * Inter Asterisk Exchange 2
 * 
 * Class definition for describing the entity that sends all packets
 * for all calls
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

#ifndef OPAL_IAX2_TRANSMIT_H
#define OPAL_IAX2_TRANSMIT_H

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal/buildopts.h>

#if OPAL_IAX2

#include <ptlib/sockets.h>

#include <iax2/frame.h>
#include <iax2/iax2ep.h>

#ifdef P_USE_PRAGMA
#pragma interface
#endif

/**Manage the transmission of ethernet packets on the specified
   port.  All transmitted packets are received from any of the current
   connections.  A separate thread is used to wait on the request to
   send packets.  Full frame packets, which have been resent the
   requisite number of times are deleted.  This class will
   (eventually) delete all the frames it is given.
   
   Note that this class is a thread, and runs when activated by outside
   events.*/
class IAX2Transmit : public PThread
{ 
  PCLASSINFO(IAX2Transmit, PThread);
 public:
  /**@name Construction/destruction*/
  //@{
  
  /**
     Constructor, which creates a thread to send all packets on the
     designated socket.  */
  IAX2Transmit(IAX2EndPoint & _newEndpoint, PUDPSocket & _newSocket);
  
  /** 
      Destructor. Deletes all pending packets.
  */
  ~IAX2Transmit();
  //@}
  
  /**@name Worker methods*/
  //@{
  
  /**Cause this thread to end now */
  virtual void Terminate();

  /**Queue a frame for delivery. This is called by a connection, and then
     the transmit thread is woken up.
  */
  void SendFrame(IAX2Frame *newFrame);
  
  /**Activate the transmit thread to process all frames in the lists 
   */
  void ProcessLists() { activate.Signal(); }
  
  /**An Ack has been received..
     Delete the matching frame from the  queue waiting for an ack
  */
  void AckReceived();
  
  /**Do the work of the thread here. Read all lists and check for
     frames to send/delete.*/
  virtual void Main();
  
  /** A full frame was transmitted a while ago, and the receiver has replied
      with a suitable acknowledgement. The acknowledgment (the newFrame) means
      that matching frames in the ack list should be removed.
  */
  void PurgeMatchingFullFrames(IAX2Frame *frame);

  /** A Vnak frame has been received (voice not acknowledged) which actually
      means, retransmit all those frames you have on this particular call
      number from the oseqno specified in the supplied frame */
  void SendVnakRequestedFrames(IAX2FullFrameProtocol &src);

  /** Report on the contents of the lists waiting for transmission */
  void ReportLists(PString & answer, bool getFullReport=false);
  //@}
  
 protected:
  
  /**Go through the acking list:: delete those who have too many
     retries, and transmit those who need retransmitting */
  void ProcessAckingList();  
  
  /**Go through the send list:: send all frames on this list */
  void ProcessSendList();
  
  /**Global variable specifying application specific variables */
  IAX2EndPoint &ep;
  
  /**Network socket used to transmit  packets*/
  PUDPSocket & sock;
  
  /**Flag to activate this thread*/
  PSyncPoint activate;
  
  /**Frames in the acking list - These frames are waiting on an
     ack. Full frames in this list will be resent an additional 3
     times if not replied to. There are no mini frames in this list -
     mini frames are not acked.*/
  IAX2ActiveFrameList  ackingFrames;   
  
  /**Send Now list of frames - These frames are to be sent now */
  IAX2ActiveFrameList  sendNowFrames;  
  
  /**Flag to indicate that this thread should keep working */
  PBoolean       keepGoing;
};


#endif // OPAL_IAX2

#endif // OPAL_IAX2_TRANSMIT_H

/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 4 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-basic-offset:2
 * End:
 */
