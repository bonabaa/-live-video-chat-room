/*
 * pglobalstatic.cxx
 *
 * Various global statics that need to be instantiated upon startup
 *
 * Portable Windows Library
 *
 * Copyright (C) 2004 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 20705 $
 * $Author: ms30002000 $
 * $Date: 2008-08-20 17:38:32 +0000 (Wed, 20 Aug 2008) $
 */

#ifndef _PGLOBALSTATIC_CXX
#define _PGLOBALSTATIC_CXX

#include <ptbuildopts.h>
#include <ptlib/plugin.h>
#include <iostream>


//
// Load static sound modules as required 
//
#if defined(P_AUDIO)

  #if defined(_WIN32) 
    PWLIB_STATIC_LOAD_PLUGIN(WindowsMultimedia, PSoundChannel);
  #elif defined(__BEOS__)
    PWLIB_STATIC_LOAD_PLUGIN(BeOS, PSoundChannel);
  #endif

  #if defined(P_WAVFILE)
    PWLIB_STATIC_LOAD_PLUGIN(WAVFile, PSoundChannel)
  #endif

#endif

//
// Load static video modules as required 
//
#if defined(P_VIDEO)

  #include <ptlib/videoio.h>

  #if defined(_WIN32) 
    #if ! defined(NO_VIDEO_CAPTURE)
      #if defined(P_VFW_CAPTURE) 
        PWLIB_STATIC_LOAD_PLUGIN(Window, PVideoOutputDevice);
        PWLIB_STATIC_LOAD_PLUGIN(VideoForWindows, PVideoInputDevice);
      #endif /*P_VFW_CAPTURE*/
      #if defined(P_DIRECTSHOW) && defined(P_DIRECTX)
        PWLIB_STATIC_LOAD_PLUGIN(DirectShow, PVideoInputDevice)
      #endif /*P_DIRECTSHOW*/
    #endif
  #endif

  //PWLIB_STATIC_LOAD_PLUGIN(FakeVideo, PVideoInputDevice);
  //PWLIB_STATIC_LOAD_PLUGIN(NULLOutput, PVideoOutputDevice);

  #if P_VIDFILE
  PWLIB_STATIC_LOAD_PLUGIN(YUVFile, PVideoInputDevice)
  PWLIB_STATIC_LOAD_PLUGIN(YUVFile, PVideoOutputDevice)
  PLOAD_FACTORY(PVideoFile, PDefaultPFactoryKey)
  #endif

#endif

//
// instantiate text to speech factory
//
#if defined(P_TTS)
  PLOAD_FACTORY(PTextToSpeech, PString)
#endif

//
// instantiate WAV file factory
//
#if defined(P_WAVFILE)
  PLOAD_FACTORY(PWAVFileConverter, unsigned)
  PLOAD_FACTORY(PWAVFileFormat,    unsigned)
#endif

//
// instantiate URL factory
//
#if defined(P_HTTP)
  //PLOAD_FACTORY(PURLScheme, PString)
#endif


//
//  instantiate startup factory
//
#if defined(P_PLUGINS)
  PLOAD_FACTORY(PluginLoaderStartup, PString)
#endif


#endif // _PGLOBALSTATIC_CXX
