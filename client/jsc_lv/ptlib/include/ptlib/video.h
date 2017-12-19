/*
 * video.h
 *
 * Video interface class.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): Derek Smithies (derek@indranet.co.nz)
 *
 * $Revision: 21788 $
 * $Author: rjongbloed $
 * $Date: 2008-12-12 05:42:13 +0000 (Fri, 12 Dec 2008) $
 */

#ifndef PTLIB_VIDEO_H
#define PTLIB_VIDEO_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptbuildopts.h>

#if P_VIDEO

#include <ptlib/videoio.h>

/**A class representing a video channel. This class is provided mainly for
   the playback or recording of video on the system.

   Note that this video channel is implicitly a series of frames in YUV411P format.
   No conversion is performed on data to/from the channel.
 */
class PVideoChannel : public PChannel
{
  PCLASSINFO(PVideoChannel, PChannel);

  public:
  /**@name Construction */
  //@{
    enum Directions {
      Recorder,
      Player
    };

    /// Create a video channel.
    PVideoChannel();

    /** Create a video channel.
        Create a reference to the video drivers for the platform.
      */
    PVideoChannel(
      const PString & device,       /// Name of video driver/device
      Directions dir               /// Video I/O direction
    );
    // 

    ~PVideoChannel();
    // Destroy and close the video driver
  //@}

  /**@name Open functions */
  //@{
    /**Open the specified device for playing or recording. The device name is
       platform specific and is as returned in the GetDevices() function.

       @return
       PTrue if the video device is valid for playing/recording.
     */
    PBoolean Open(
      const PString & device,       /// Name of video driver/device
      Directions dir               /// Video I/O direction
    );

    /** return True if one (or both) of the video device class pointers
       is non NULL. If either pointer is non NULL, then a device is ready
       to be written to, which indicates this channel is open.
    */
     PBoolean IsOpen() const;
    
    /**Get all of the names for video devices/drivers that are available on
       this platform. Note that a named device may not necessarily do both
       playing and recording so the arrays returned with the #dir#
       parameter in each value is not necessarily the same.

       @return
       An array of platform dependent strings for each video player/recorder.
     */
    static PStringArray GetDeviceNames(
      Directions dir    // Video I/O direction
    )  ;

    /**Get the name for the default video devices/driver that is on this
       platform. Note that a named device may not necessarily do both
       playing and recording so the arrays returned with the #dir#
       parameter in each value is not necessarily the same.

       @return
       A platform dependent string for the video player/recorder.
     */
    static PString GetDefaultDevice(
      Directions dir    // Video I/O direction
    );
    //@}

    
    /**Return the width of the currently selected grabbing device.
     */
    virtual PINDEX  GetGrabWidth(); 

    /**Return the height of the currently selected grabbing device.
     */
    virtual PINDEX  GetGrabHeight();

    virtual PBoolean Read(void * buf, PINDEX  len);
      // Low level read from the video channel. This function will block until the
      // requested number of characters were read.
  
  
    /**Low level write to the channel, which is data to be rendered to the 
       local video display device.
       */
    PBoolean Write(const void * buf,  //Pointer to the image data to be rendered
               PINDEX      len);
    
    /**Cause the referenced data to be drawn to the 
       previously defined media 
     */
    virtual PBoolean Redraw(const void * frame); 

    /**Return the previously specified width.
     */
    PINDEX  GetRenderWidth();

    /**Return the previously specified height.
     */
    PINDEX  GetRenderHeight();

    /**Specifiy the width and height of the video stream, which is to be
       rendered onto the previously specified device.
     */
    virtual void SetRenderFrameSize(int width, int height); 

    /**Specifiy the width and height of the video stream, which is to be
       extracted from the previously specified device.
     */
    virtual void SetGrabberFrameSize(int width, int height); 

    /**Attach a user specific class for rendering video 

       If keepCurrent is true, an abort is caused when the program attempts to attach
       a new player when there is already a video player attached.

       If keepCurrent is false, the existing video player is deleted before attaching
       the new player.
     */
    virtual void AttachVideoPlayer(PVideoOutputDevice * device, PBoolean keepCurrent = PTrue);

    /**Attach a user specific class for acquiring video 

      If keepCurrent is true, an abort is caused when the program attempts to attach
       a new reader when there is already a video reader attached.

       If keepCurrent is false, the existing video reader is deleted before attaching
       the new reader.
     */
    virtual void AttachVideoReader(PVideoInputDevice * device, PBoolean keepCurrent = PTrue);

    /**Return a pointer to the class for acquiring video 
     */
    virtual PVideoInputDevice *GetVideoReader();

    /**Return a pointer to the class for displaying video
     */
    virtual PVideoOutputDevice *GetVideoPlayer();

    /**See if the grabber is open 
     */
    virtual PBoolean IsGrabberOpen();
    
    /**See if the rendering device is open
     */
    virtual PBoolean IsRenderOpen();

    /**Get data from the attached inputDevice, and display on the
       attached ouptutDevice.
    */
    PBoolean DisplayRawData(void *videoBuffer);

    /**Destroy the attached grabber class.
     */
    virtual void CloseVideoReader();

    /**Destroy the attached video display class.
     */
    virtual void CloseVideoPlayer();

    /**Restrict others from using this video channel.
     */
    void RestrictAccess();
    
    /**Allow free access to this video channel.
     */
    void EnableAccess();

    /**Toggle the vertical flip state of the video grabber.
    */
    PBoolean ToggleVFlipInput();

 protected:

    Directions       direction;

    PString          deviceName;     ///Specified video device name, eg /dev/video0.
    PVideoInputDevice  *mpInput;    /// For grabbing video from the camera.
    PVideoOutputDevice *mpOutput;   /// For displaying video on the screen.

    PMutex           accessMutex;   // Ensure that only task is accesing 
                                    // members in this video channel.
  private:
    void Construct();


// Include platform dependent part of class
#ifdef _WIN32
#include "msos/ptlib/video.h"
#else
#include "unix/ptlib/video.h"
#endif
};

#endif // P_VIDEO

#endif // PTLIB_VIDEO_H


// End Of File ///////////////////////////////////////////////////////////////
