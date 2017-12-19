/*
 * main.cxx
 *
 * PWLib application source file for dtmftest
 *
 * Main program entry point.
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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

#include "precompile.h"
#include "main.h"
#include "version.h"


PCREATE_PROCESS(DtmfTest);

#include  <ptclib/dtmf.h>
#include  <ptclib/random.h>
#include  <ptlib/sound.h>


static const PINDEX samplesPerMillisecond = 8;


DtmfTest::DtmfTest()
  : PProcess("Equivalence", "dtmftest", MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER)
{
}


void DtmfTest::Main()
{
  PINDEX i;

  PArgList & args = GetArguments();

  args.Parse(
             "h-help."               "-no-help."
             "d-duration:"
             "n-noise:"              "-no-noise."
             "s-sound:"              "-no-sound."
             "T-tone."               "-no-tone."
#if PTRACING
             "o-output:"             "-no-output."
             "t-trace."              "-no-trace."
#endif
             "v-version."
  );

#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
         PTrace::Blocks | PTrace::Timestamp | PTrace::Thread | PTrace::FileAndLine);
#endif

  if (args.HasOption('v')) {
    cout << "Product Name: " << GetName() << endl
         << "Manufacturer: " << GetManufacturer() << endl
         << "Version     : " << GetVersion(PTrue) << endl
         << "System      : " << GetOSName() << '-'
         << GetOSHardware() << ' '
         << GetOSVersion() << endl;
    return;
  }

  if (args.HasOption('h')) {
    PError << "Generates tones with optional noise level and then decodes them.\n"
              "Simulation is done at 8000Hz, or 8khz, 16 bit integers.\n"
              "\n"
              "dtmftest [options] arg1 [ arg2 ... ]\n"
              "Available options are:\n"
              "\n"
              "  -h or --help          : print this help message.\n"
              "  -d or --duration #    : duration milliseconds.\n"
              "  -n or --noise #       : Peak noise level (0..10000)\n"
              "  -s or --sound #       : Output to sound device (use * for default)\n"
              "  -T or --tone          : Parameters are tone descriptors rather than DTMF\n"
#if PTRACING
              "  -o or --output file   : file name for output of log messages\n"       
              "  -t or --trace         : degree of verbosity in error log (more times for more detail)\n"     
#endif
              "  -v or --version       : report version information\n"
              "\n"
           << " e.g. ./dtmftest -d 60 -n 100 1234\n"
           << "                to generate 60ms long DTMF tones for 1234, with a signal noise factor of 100\n"
           << endl << endl;
    return;
  }


  unsigned milliseconds;
  if (args.HasOption('d')) {
    milliseconds = args.GetOptionString('d').AsUnsigned();
    if (milliseconds < 10) {
      cerr << "Invalid duration specified!\n";
      return;
    }
  }
  else
    milliseconds = 100;
  cout << "Sample section  is " << milliseconds << " ms long.\n";


  PRandom random;
  PShortArray noiseSignal(milliseconds * samplesPerMillisecond);
  if (args.HasOption('n')) {
    unsigned noise = args.GetOptionString('n').AsUnsigned();
    if (noise < 10 || noise > 10000) {
      cerr << "Invalid noise level specified!\n";
      return;
    }
    cout << "Peak noise magnitude is " << noise << '\n';
    for (i = 0; i < noiseSignal.GetSize(); i++)
      noiseSignal[i] = (short)((int)random.Generate(noise/2) - noise/2); 
  }


  PString tonesToPlay;
  for (i = 0; i < args.GetCount(); i++) {
    if (args.HasOption('T')) {
      if (!tonesToPlay.IsEmpty())
        tonesToPlay += '/';
      tonesToPlay += args[i];
    }
    else
      tonesToPlay += args[i];
  }
  if (tonesToPlay.IsEmpty())
    tonesToPlay = "0123456789ABCD*#XY";


  if (args.HasOption('s')) {
    PSoundChannel speaker;
    if (!speaker.Open(args.GetOptionString('s'), PSoundChannel::Player)) {
      cerr << "Could not open sound card!\n";
      return;
    }

    PDTMFEncoder toneData;
    if (args.HasOption('T')) {
      if (!toneData.Generate(tonesToPlay)) {
        cerr << "Error parsing tone descriptor \"" << tonesToPlay << "\"\n";
        return;
      }
    }
    else
    {
      toneData.AddTone(tonesToPlay, 1000);
    }

    if (!speaker.Write(toneData.GetPointer(), toneData.GetSize()*2))
      cerr << "Could not write tone data to sound card!\n";
    else {
      speaker.WaitForPlayCompletion();
      speaker.Abort();
    }
    return;
  }

  PShortArray result(milliseconds * samplesPerMillisecond);
  PDTMFDecoder decoder;

  int nCorrect = 0;
  for (const char * pDTMF = tonesToPlay; *pDTMF != '\0'; pDTMF++) {
    PDTMFEncoder encoder(*pDTMF, milliseconds);

    for (i = 0; i < result.GetSize(); i++)
      result[i] = encoder[i] + noiseSignal[i];

    PString detectedTones;

    PINDEX sample = 0;
    while (sample < result.GetSize() && (detectedTones = decoder.Decode(&result[sample], samplesPerMillisecond)).IsEmpty())
      sample += samplesPerMillisecond;

    if (detectedTones.IsEmpty())
      detectedTones = " ";
    cout << "Test : " << *pDTMF << " ---> " << detectedTones << "    ";

    if (detectedTones[0] == *pDTMF) {
      cout << "Good";
      nCorrect++;
    } else {
      cout << "Fail";
    }

    cout << "       decode time : " << sample/samplesPerMillisecond << " msecs, " << sample << " samples" << endl;
  }

  cout << endl << "Test run complete. Correctly interpreted " << (100 * nCorrect / tonesToPlay.GetLength()) << "%" << endl;
}

// End of File ///////////////////////////////////////////////////////////////
