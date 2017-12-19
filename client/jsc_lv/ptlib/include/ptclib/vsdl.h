/*
 * vsdl.h
 *
 * Classes to support video output via SDL
 *
 * Copyright (c) 1999-2000 Equivalence Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): Derek J Smithies (derek@indranet.co.nz)
 *
 * $Revision: 23337 $
 * $Author: rjongbloed $
 * $Date: 2009-09-05 01:21:09 +0000 (Sat, 05 Sep 2009) $
 */

#ifndef PTLIB_VSDL_H
#define PTLIB_VSDL_H

#if P_SDL

#include <ptlib.h>
#include <SDL/SDL.h>

#include <ptlib/videoio.h>

#undef main


/**Display data to the SDL screen.
  */
class PVideoOutputDevice_SDL : public PVideoOutputDevice
{
    PCLASSINFO(PVideoOutputDevice_SDL, PVideoOutputDevice);
  
  public:
    /**Constructor. Does not make a window.
      */
    PVideoOutputDevice_SDL();
  
      /**Destructor.  Closes window if necessary, (which initializes all variables)
    */
    ~PVideoOutputDevice_SDL();
  
    /**Get a list of all of the devices available.
    */
    virtual PStringArray GetDeviceNames() const;
  
    /**Open the device given the device name.
    */
    virtual PBoolean Open(
      const PString & /*deviceName*/,   ///< Device name to open
      PBoolean /*startImmediate*/ = PTrue    ///< Immediately start device
    );
  
    /**Synonymous with the destructor.
    */
    virtual PBoolean Close();
  
    /**Global test function to determine if this video rendering
    class is open.*/
    virtual PBoolean IsOpen();
  
    /**Set the colour format to be used.
       Note that this function does not do any conversion. If it returns PTrue
       then the video device does the colour format in native mode.

       To utilise an internal converter use the SetColourFormatConverter()
       function.

       Default behaviour sets the value of the colourFormat variable and then
       returns PTrue.
    */
    virtual PBoolean SetColourFormat(
      const PString & colourFormat ///< New colour format for device.
    );

    /**Set the frame size to be used.

       Note that devices may not be able to produce the requested size, and
       this function will fail.  See SetFrameSizeConverter().

       Default behaviour sets the frameWidth and frameHeight variables and
       returns PTrue.
    */
    virtual PBoolean SetFrameSize(
      unsigned width,   ///< New width of frame
      unsigned height   ///< New height of frame
    );

    /**Get the maximum frame size in bytes.

       Note a particular device may be able to provide variable length
       frames (eg motion JPEG) so will be the maximum size of all frames.
      */
    virtual PINDEX GetMaxFrameBytes();

    /**Set a section of the output frame buffer.
      */
    virtual PBoolean SetFrameData(
      unsigned x,
      unsigned y,
      unsigned width,
      unsigned height,
      const BYTE * data,
      PBoolean endFrame = PTrue
    );

  protected:
    PDECLARE_NOTIFIER(PThread, PVideoOutputDevice_SDL, SDLThreadMain);
    bool InitialiseSDL();
    bool ProcessSDLEvents();

    PThread     * sdlThread;
    PSyncPoint    sdlStarted;
    PSyncPointAck sdlStop;
    PSyncPointAck adjustSize;
    bool          updateOverlay;
    PMutex        mutex;

    SDL_Surface * screen;
    SDL_Overlay * overlay;
  };


typedef PVideoOutputDevice_SDL PSDLVideoDevice; // Backward compatibility


#endif    // P_SDL

#endif // PTLIB_VSDL_H


// End Of File ///////////////////////////////////////////////////////////////
