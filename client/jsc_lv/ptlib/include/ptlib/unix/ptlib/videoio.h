/*
 * videoio.h
 *
 * Classes to support streaming video input (grabbing) and output.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-2000 Equivalence Pty. Ltd.
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
 * Contributor(s): ______________________________________.
 *
 * $Revision: 20385 $
 * $Author: rjongbloed $
 * $Date: 2008-06-04 10:40:38 +0000 (Wed, 04 Jun 2008) $
 */

#if defined(P_LINUX) && !defined(NO_VIDEO_CAPTURE)
#include <linux/videodev.h>     /* change this to "videodev2.h" for v4l2 */
#endif

#if defined(P_FREEBSD)
#include <machine/ioctl_meteor.h>
#endif

#if defined(P_OPENBSD) || defined(P_NETBSD)
#if P_OPENBSD >= 200105
#include <dev/ic/bt8xx.h> 
#elif P_NETBSD >= 105000000
#include <dev/ic/bt8xx.h> 
#else 
#include <i386/ioctl_meteor.h>
#endif
#endif

  public:
    virtual PBoolean SetVideoFormat(VideoFormat videoFormat);
    virtual int  GetNumChannels();
    virtual PBoolean SetChannel(int channelNumber);
    virtual PBoolean SetColourFormat(const PString & colourFormat);
    virtual PBoolean SetFrameRate(unsigned rate);
    virtual PBoolean GetFrameSizeLimits(unsigned & minWidth, unsigned & minHeight, unsigned & maxWidth, unsigned & maxHeight) ;
    virtual PBoolean SetFrameSize(unsigned width, unsigned height);
    virtual int GetBrightness();
    virtual PBoolean SetBrightness(unsigned newBrightness) ;
    virtual int GetContrast();
    virtual PBoolean SetContrast(unsigned newContrast); 
    virtual int GetHue();
    virtual PBoolean SetHue(unsigned newHue); 


#if defined(P_LINUX) && !defined(NO_VIDEO_CAPTURE)
    // only override these methods in Linux. Other platforms will use the
    // default methods in PVideoDevice
    virtual int GetWhiteness();
    virtual PBoolean SetWhiteness(unsigned newWhiteness); 
    virtual int GetColour();
    virtual PBoolean SetColour(unsigned newColour); 
    virtual PBoolean SetVideoChannelFormat(int channelNumber,
				       VideoFormat videoFormat);
#endif

    /** from one ioctl call, get whiteness, brightness, colour, contrast and hue.
     */
    virtual PBoolean GetParameters (int *whiteness, int *brightness, 
				int *colour, int *contrast, int *hue);

  protected:
    void ClearMapping();

    /** Do not use memory mapping, access the data with a call to ::read();
     */
    PBoolean NormalReadProcess(BYTE *resultBuffer, PINDEX *bytesReturned);


#if defined(P_LINUX) && !defined(NO_VIDEO_CAPTURE)
    int    videoFd;
    struct video_capability videoCapability;
    int    canMap;  // -1 = don't know, 0 = no, 1 = yes
    int    colourFormatCode;
    PINDEX hint_index;
    BYTE * videoBuffer;
    PINDEX frameBytes;

   /** Ensure each ::ioctl(VIDIOMCAPTURE) is matched by a ::ioctl(VIDIOCSYNC).
    */
    PBoolean   pendingSync[2];

    int    currentFrame;
    struct video_mbuf frame;
    struct video_mmap frameBuffer[2];
#endif

#if defined(P_FREEBSD) || defined(P_OPENBSD) || defined(P_NETBSD)
    struct video_capability
    {
        int channels;   /* Num channels */
        int maxwidth;   /* Supported width */
        int maxheight;  /* And height */
        int minwidth;   /* Supported width */
        int minheight;  /* And height */
    };

    int    videoFd;
    struct video_capability videoCapability;
    int    canMap;  // -1 = don't know, 0 = no, 1 = yes
    BYTE * videoBuffer;
    PINDEX frameBytes;
    int    mmap_size;
#endif

// End Of File ////////////////////////////////////////////////////////////////
