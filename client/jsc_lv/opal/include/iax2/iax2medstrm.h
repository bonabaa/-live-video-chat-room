/*
 *
 * Inter Asterisk Exchange 2
 * 
 * Open Phone Abstraction Library (OPAL)
 *
 * Extension of the Opal Media stream, where the media from the IAX2 side is
 * linked to the OPAL 
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
 * $Revision: 22799 $
 * $Author: rjongbloed $
 * $Date: 2009-06-03 12:13:44 +0000 (Wed, 03 Jun 2009) $
 */

#ifndef OPAL_IAX2_MEDIASTRM_H
#define OPAL_IAX2_MEDIASTRM_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#if OPAL_IAX2

#include <opal/mediafmt.h>
#include <iax2/iax2con.h>

class RTP_Session;
class OpalMediaPatch;
class OpalLine;


/**This class describes a media stream, which is an interface to the opal classes for 
   generating encoded media data 
*/
class OpalIAX2MediaStream : public OpalMediaStream
{
  PCLASSINFO(OpalIAX2MediaStream, OpalMediaStream);
  /**@name Construction and Destruction*/
  //@{
    /**Construct a new media stream for connecting to the media.
       This method grabs a SafeReference on the connection, so the connection won't go away on us.
      */
    OpalIAX2MediaStream(
		   IAX2Connection &con,                 /*!< IAX connection to read/send incoming packets */
		   const OpalMediaFormat & mediaFormat, /*!< Media format for stream */
		   unsigned sessionID,                  /*!< Session number for stream */
		   PBoolean isSource                        /*!< Is a source stream */
		   );
    /**Destroy a new media stream for connecting to the media.
       This method releases the SafeReference on the connection, so the connection can be destroyed */
    ~OpalIAX2MediaStream();
  //@}


 
 public:
  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Open the media stream.
 
 
      */
    virtual PBoolean Open();
 
    /**Start the media stream.
       
    The default behaviour calls Resume() on the associated
    OpalMediaPatch thread if it was suspended.
    */
    virtual PBoolean Start();

    /**Close the media stream.
 
       The default does nothing.
      */
    virtual PBoolean Close();
 
    /**
       Goes to the IAX2Connection class, and removes a packet from the connection. The connection class turned the media 
       packet into a RTP_DataFrame class, and jitter buffered it.

    @return PTrue on successful read of a packet, PFalse on faulty read.*/
    virtual PBoolean ReadPacket(
      RTP_DataFrame & packet ///< Data buffer to read to
    );

   /**Write raw media data to the sink media stream.
       The default behaviour writes to the OpalLine object.
      */
    virtual PBoolean WriteData(
      const BYTE * data,   ///< Data to write
      PINDEX length,       ///< Length of data to write.
      PINDEX & written     ///<Length of data actually written
    );

 /**Indicate if the media stream is synchronous.
       @Return false if this stream is from the network.
       @return true if this stream is from a sound card.
      */
    virtual PBoolean IsSynchronous() const;

  //@}

  protected:
    /**The connection is the source/sink of our data packets */
    IAX2Connection & connection;

    /**There was unused data from an incoming ethernet frame. The
       unused data is stored here. 
    */
    PBYTEArray pendingData;
};


#endif // OPAL_IAX2

#endif  // OPAL_IAX2_MEDIASTRM_H

/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 4 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-basic-offset:4
 * End:
 */
