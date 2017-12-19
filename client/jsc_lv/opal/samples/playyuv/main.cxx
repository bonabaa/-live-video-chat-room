/*
 * main.cxx
 *
 * OPAL application source file for playing video from a YUV file
 *
 * Main program entry point.
 *
 * Copyright (c) 20087 Post Increment
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
 * $Revision: 20134 $
 * $Author: csoutheren $
 * $Date: 2008-04-28 13:45:00 +1000 (Mon, 28 Apr 2008) $
 */

#include "precompile.h"
#include "main.h"

#include <ptclib/pvidfile.h>


PCREATE_PROCESS(PlayYUV);



PlayYUV::PlayYUV()
  : PProcess("OPAL Video Player", "PlayYUV", 1, 0, ReleaseCode, 0)
  , m_display(NULL)
{
}


PlayYUV::~PlayYUV()
{
}


void PlayYUV::Main()
{
  PArgList & args = GetArguments();

  args.Parse("h-help."
             "V-video-driver:"
             "v-video-device:"
             "p-singlestep."
             "i-info."
#if PTRACING
             "o-output:"             "-no-output."
             "t-trace."              "-no-trace."
#endif
             , FALSE);

#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
         PTrace::Blocks | PTrace::Timestamp | PTrace::Thread | PTrace::FileAndLine);
#endif

  if (args.HasOption('h') || args.GetCount() == 0) {
    PError << "usage: PlayYUV [ options ] filename [ filename ... ]\n"
              "\n"
              "Available options are:\n"
              "  --help                   : print this help message.\n"
              "  -V or --video-driver drv : Video display driver to use.\n"
              "  -v or --video-device dev : Video display device to use.\n"
              "  -p or --singlestep       : Single step through input data.\n"
              "  -i or --info             : Display per-frame information.\n"
#if PTRACING
              "  -o or --output file     : file name for output of log messages\n"       
              "  -t or --trace           : degree of verbosity in error log (more times for more detail)\n"     
#endif
              "\n"
              "e.g. ./PlayYUV file.yuv\n\n";
    return;
  }

  m_singleStep = args.HasOption('p');
  m_info       = args.HasOption('i');

  // Video display
  PString driverName = args.GetOptionString('V');
  PString deviceName = args.GetOptionString('v');
  m_display = PVideoOutputDevice::CreateOpenedDevice(driverName, deviceName, FALSE);
  if (m_display == NULL) {
    cerr << "Cannot use ";
    if (driverName.IsEmpty() && deviceName.IsEmpty())
      cerr << "default ";
    cerr << "video display";
    if (!driverName)
      cerr << ", driver \"" << driverName << '"';
    if (!deviceName)
      cerr << ", device \"" << deviceName << '"';
    cerr << ", must be one of:\n";
    PStringList devices = PVideoOutputDevice::GetDriversDeviceNames("*");
    for (PINDEX i = 0; i < devices.GetSize(); i++)
      cerr << "   " << devices[i] << '\n';
    cerr << endl;
    return;
  }

  m_display->SetColourFormatConverter(OpalYUV420P);

  cout << "Display ";
  if (!driverName.IsEmpty())
    cout << "driver \"" << driverName << "\" and ";
  cout << "device \"" << m_display->GetDeviceName() << "\" opened." << endl;

  for (PINDEX i = 0; i < args.GetCount(); i++)
    Play(args[i]);
}


void PlayYUV::Play(const PFilePath & filename)
{
  PYUVFile vidFile;
  if (!vidFile.Open(filename, PFile::ReadOnly)) {
    cout << "Could not open file \"" << filename << '"' << endl;
    return;
  }

  PINDEX frameSize = vidFile.GetFrameBytes();
  unsigned width, height;
  vidFile.GetFrameSize(width, height);

  cout << "Playing file " << filename << " at " << width << "x" << height << endl;

  BYTE * frameBuffer = new BYTE[frameSize];

  m_display->Start();

  for (;;) {

    if (!vidFile.ReadFrame(frameBuffer)) {
      cout << "Truncated file \"" << filename << '"' << endl;
      return;
    }

    m_display->SetFrameSize(width, height);
    m_display->SetFrameData(0, 0, width, height, frameBuffer, true);

    Sleep(100);
  }
}

// End of File ///////////////////////////////////////////////////////////////
