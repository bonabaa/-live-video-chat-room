/*
 * main.cxx
 *
 * A simple H.323 "net telephone" application.
 *
 * Copyright (c) 2000 Equivalence Pty. Ltd.
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
 * $Revision: 21158 $
 * $Author: rjongbloed $
 * $Date: 2008-09-25 03:03:10 +0000 (Thu, 25 Sep 2008) $
 */


/////-l -a -u Zhang3 --grabber "Microsoft WDM Image Capture (Win32)" --display "MSWIN STYLE=0x80C80000 TITLE='Video Output'" --register-sip 192.168.1.100 --sip-domain 192.168.1.100 --sip-listen udp$:*:5070
#include "precompile.h"
#include "main.h"
#include "../../version.h"

PSafePtr<OpalCall> g_GetToken(){return NULL;}
#define new PNEW


PCREATE_PROCESS(SimpleOpalProcess);

///////////////////////////////////////////////////////////////

SimpleOpalProcess::SimpleOpalProcess()
  : PProcess("qe Limited ", "demo",
             MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER)
{
}

void SimpleOpalProcess::Main()
{
  cout << GetName()
       << " Version " << GetVersion(PTrue)
       << " by " << GetManufacturer()
       << " on " << GetOSClass() << ' ' << GetOSName()
       << " (" << GetOSVersion() << '-' << GetOSHardware() << ")\n\n";

  // Get and parse all of the command line arguments.
  PArgList & args = GetArguments();
  args.Parse(
             "a-auto-answer."
             "b-bandwidth:"
             "D-disable:"
             "d-dial-peer:"
             "-disableui."
             "e-silence."
             "f-fast-disable."
             "g-gatekeeper:"
             "G-gk-id:"
             "-gk-token:"
             "-disable-grq."
             "h-help."
             "H-no-h323."
#if OPAL_PTLIB_SSL
             "-no-h323s."
             "-h323s-listen:"
             "-h323s-gk:"
#endif
             "-h323-listen:"
             "I-no-sip."
             "j-jitter:"
             "l-listen."
#if OPAL_LID
             "L-no-lid."
             "-lid:"
             "-country:"
#endif
             "-no-std-dial-peer."
#if PTRACING
             "o-output:"
#endif
             "O-option:"
             "P-prefer:"
             "p-password:"
             "-portbase:"
             "-portmax:"
             "R-require-gatekeeper."
             "r-register-sip:"
             "-rtp-base:"
             "-rtp-max:"
             "-rtp-tos:"
             "s-sound:"
             "S-no-sound."
             "-sound-in:"
             "-sound-out:"
             "-srcep:"
             "-sip-listen:"
             "-sip-proxy:"
             "-sip-domain:"
             "-sip-user-agent:"
             "-sip-ui:"
             "-stun:"
             "T-h245tunneldisable."
             "-translate:"
             "-tts:"

#if PTRACING
             "t-trace."
#endif
             "-tcp-base:"
             "-tcp-max:"
             "u-user:"
             "-udp-base:"
             "-udp-max:"
             "-use-long-mime."
#if OPAL_VIDEO
             "-rx-video." "-no-rx-video."
             "-tx-video." "-no-tx-video."
             "-grabber:"
             "-grabdriver:"
             "-grabchannel:"
             "-display:"
             "-displaydriver:"
             "-video-size:"
             "-video-rate:"
#endif
#if OPAL_IVR
             "V-no-ivr."
             "x-vxml:"
#endif
#if OPAL_IAX2
	     "X-no-iax2."
#endif
          , PFalse);


  if (args.HasOption('h') || (!args.HasOption('l') && args.GetCount() == 0)) {
    cout << "Usage : " << GetName() << " [options] -l\n"
            "      : " << GetName() << " [options] [alias@]hostname   (no gatekeeper)\n"
            "      : " << GetName() << " [options] alias[@hostname]   (with gatekeeper)\n"
            "General options:\n"
            "  -l --listen             : Listen for incoming calls.\n"
            "  -d --dial-peer spec     : Set dial peer for routing calls (see below)\n"
            "     --no-std-dial-peer   : Do not include the standard dial peers\n"
            "  -a --auto-answer        : Automatically answer incoming calls.\n"
            "  -u --user name          : Set local alias name(s) (defaults to login name).\n"
            "  -p --password pwd       : Set password for user (gk or SIP authorisation).\n"
            "  -D --disable media      : Disable the specified codec (may be used multiple times)\n"
            "  -P --prefer media       : Prefer the specified codec (may be used multiple times)\n"
            "  -O --option fmt:opt=val : Set codec option (may be used multiple times)\n"
            "                          :  fmt is name of codec, eg \"H.261\"\n"
            "                          :  opt is name of option, eg \"Target Bit Rate\"\n"
            "                          :  val is value of option, eg \"48000\"\n"
            "  --srcep ep              : Set the source endpoint to use for making calls\n"
            "  --disableui             : disable the user interface\n"
            "\n"
            "Audio options:\n"
            "  -j --jitter [min-]max   : Set minimum (optional) and maximum jitter buffer (in milliseconds).\n"
            "  -e --silence            : Disable transmitter silence detection.\n"
            "\n"
#if OPAL_VIDEO
            "Video options:\n"
            "     --rx-video           : Start receiving video immediately.\n"
            "     --tx-video           : Start transmitting video immediately.\n"
            "     --no-rx-video        : Don't start receiving video immediately.\n"
            "     --no-tx-video        : Don't start transmitting video immediately.\n"
            "     --grabber dev        : Set the video grabber device.\n"
            "     --grabdriver dev     : Set the video grabber driver (if device name is ambiguous).\n"
            "     --grabchannel num    : Set the video grabber device channel.\n"
            "     --display dev        : Set the video display device.\n"
            "     --displaydriver dev  : Set the video display driver (if device name is ambiguous).\n"
            "     --video-size size    : Set the size of the video for all video formats, use\n"
            "                          : \"qcif\", \"cif\", WxH etc\n"
            "     --video-rate rate    : Set the frame rate of video for all video formats\n"
            "\n"
#endif

#if OPAL_SIP
            "SIP options:\n"
            "  -I --no-sip             : Disable SIP protocol.\n"
            "  -r --register-sip host  : Register with SIP server.\n"
            "     --sip-proxy url      : SIP proxy information, may be just a host name\n"
            "                          : or full URL eg sip:user:pwd@host\n"
            "     --sip-listen iface   : Interface/port(s) to listen for SIP requests\n"
            "                          : '*' is all interfaces, (default udp$:*:5060)\n"
            "     --sip-user-agent name: SIP UserAgent name to use.\n"
            "     --sip-ui type        : Set type of user indications to use for SIP. Can be one of 'rfc2833', 'info-tone', 'info-string'.\n"
            "     --use-long-mime      : Use long MIME headers on outgoing SIP messages\n"
            "     --sip-domain str     : set authentication domain/realm\n"
            "\n"
#endif

#if OPAL_H323
            "H.323 options:\n"
            "  -H --no-h323            : Disable H.323 protocol.\n"
#if OPAL_PTLIB_SSL
            "     --no-h323s           : Do not create secure H.323 endpoint\n"
#endif
            "  -g --gatekeeper host    : Specify gatekeeper host, '*' indicates broadcast discovery.\n"
            "  -G --gk-id name         : Specify gatekeeper identifier.\n"
#if OPAL_PTLIB_SSL
            "     --h323s-gk host      : Specify gatekeeper host for secure H.323 endpoint\n"
#endif
            "  -R --require-gatekeeper : Exit if gatekeeper discovery fails.\n"
            "     --gk-token str       : Set gatekeeper security token OID.\n"
            "     --disable-grq        : Do not send GRQ when registering with GK\n"
            "  -b --bandwidth bps      : Limit bandwidth usage to bps bits/second.\n"
            "  -f --fast-disable       : Disable fast start.\n"
            "  -T --h245tunneldisable  : Disable H245 tunnelling.\n"
            "     --h323-listen iface  : Interface/port(s) to listen for H.323 requests\n"
#if OPAL_PTLIB_SSL
            "     --h323s-listen iface : Interface/port(s) to listen for secure H.323 requests\n"
#endif
            "                          : '*' is all interfaces, (default tcp$:*:1720)\n"
#endif

            "\n"
#if OPAL_LID
            "Line Interface options:\n"
            "  -L --no-lid             : Do not use line interface device.\n"
            "     --lid device         : Select line interface device (eg Quicknet:013A17C2, default *:*).\n"
            "     --country code       : Select country to use for LID (eg \"US\", \"au\" or \"+61\").\n"
            "\n"
#endif
            "Sound card options:\n"
            "  -S --no-sound           : Do not use sound input/output device.\n"
            "  -s --sound device       : Select sound input/output device.\n"
            "     --sound-in device    : Select sound input device.\n"
            "     --sound-out device   : Select sound output device.\n"
            "\n"
#if OPAL_IVR
            "IVR options:\n"
            "  -V --no-ivr             : Disable IVR.\n"
            "  -x --vxml file          : Set vxml file to use for IVR.\n"
            "  --tts engine            : Set the text to speech engine\n"
            "\n"
#endif
            "IP options:\n"
            "     --translate ip       : Set external IP address if masqueraded\n"
            "     --portbase n         : Set TCP/UDP/RTP port base\n"
            "     --portmax n          : Set TCP/UDP/RTP port max\n"
            "     --tcp-base n         : Set TCP port base (default 0)\n"
            "     --tcp-max n          : Set TCP port max (default base+99)\n"
            "     --udp-base n         : Set UDP port base (default 6000)\n"
            "     --udp-max n          : Set UDP port max (default base+199)\n"
            "     --rtp-base n         : Set RTP port base (default 5000)\n"
            "     --rtp-max n          : Set RTP port max (default base+199)\n"
            "     --rtp-tos n          : Set RTP packet IP TOS bits to n\n"
	          "     --stun server        : Set STUN server\n"
            "\n"
            "Debug options:\n"
#if PTRACING
            "  -t --trace              : Enable trace, use multiple times for more detail.\n"
            "  -o --output             : File for trace output, default is stderr.\n"
#endif
#if OPAL_IAX2
            "  -X --no-iax2            : Remove support for iax2\n"
#endif
            "  -h --help               : This help message.\n"
            "\n"
            "\n"
            "Dial peer specification:\n"
"  General form is pattern=destination where pattern is a regular expression\n"
"  matching the incoming calls destination address and will translate it to\n"
"  the outgoing destination address for making an outgoing call. For example,\n"
"  picking up a PhoneJACK handset and dialling 2, 6 would result in an address\n"
"  of \"pots:26\" which would then be matched against, say, a spec of\n"
"  pots:26=h323:10.0.1.1, resulting in a call from the pots handset to\n"
"  10.0.1.1 using the H.323 protocol.\n"
"\n"
"  As the pattern field is a regular expression, you could have used in the\n"
"  above .*:26=h323:10.0.1.1 to achieve the same result with the addition that\n"
"  an incoming call from a SIP client would also be routed to the H.323 client.\n"
"\n"
"  Note that the pattern has an implicit ^ and $ at the beginning and end of\n"
"  the regular expression. So it must match the entire address.\n"
"\n"
"  If the specification is of the form @filename, then the file is read with\n"
"  each line consisting of a pattern=destination dial peer specification. Lines\n"
"  without and equal sign or beginning with '#' are ignored.\n"
"\n"
"  The standard dial peers that will be included are:\n"
"    If SIP is enabled but H.323 & IAX2 are disabled:\n"
"      pots:.*\\*.*\\*.* = sip:<dn2ip>\n"
"      pots:.*         = sip:<da>\n"
"      pc:.*           = sip:<da>\n"
"\n"
"    If SIP & IAX2 are not enabled and H.323 is enabled:\n"
"      pots:.*\\*.*\\*.* = h323:<dn2ip>\n"
"      pots:.*         = h323:<da>\n"
"      pc:.*           = h323:<da>\n"
"\n"
"    If POTS is enabled:\n"
"      h323:.* = pots:<dn>\n"
"      sip:.*  = pots:<dn>\n"
"      iax2:.* = pots:<dn>\n"
"\n"
"    If POTS is not enabled and the PC sound system is enabled:\n"
"      iax2:.* = pc:<du>\n"
"      h323:.* = pc:<du>\n"
"      sip:. * = pc:<du>\n"
"\n"
#if OPAL_IVR
"    If IVR is enabled then a # from any protocol will route it it, ie:\n"
"      .*:#  = ivr:\n"
"\n"
#endif
#if OPAL_IAX2
"    If IAX2 is enabled then you can make a iax2 call with a command like:\n"
"       simpleopal -IHn  iax2:guest@misery.digium.com/s\n"
#endif
            << endl;
    return;
  }

//#if PTRACING
   PTrace::Initialise(5,"DEBUGSTREAM",
                      
                     PTrace::Timestamp|PTrace::Thread|PTrace::FileAndLine);
//#endif

  // Create the Opal Manager and initialise it
  opal = new MyManager;

  if (opal->Initialise(args))
    opal->Main(args);

  cout << "Exiting " << GetName() << endl;

  delete opal;
}


///////////////////////////////////////////////////////////////

MyManager::MyManager()
{
#if OPAL_LID
  potsEP = NULL;
#endif
  pcssEP = NULL;

#if OPAL_H323
  h323EP = NULL;
#endif
#if OPAL_SIP
  sipEP  = NULL;
#endif
#if OPAL_IAX2
  iax2EP = NULL;
#endif
#if OPAL_IVR
  ivrEP  = NULL;
#endif
#if OPAL_FAX
  faxEP = NULL;
  t38EP = NULL;
#endif

  pauseBeforeDialing = PFalse;
}


MyManager::~MyManager()
{
#if OPAL_LID
  // Must do this before we destroy the manager or a crash will result
  if (potsEP != NULL)
    potsEP->RemoveAllLines();
#endif
}


PBoolean MyManager::Initialise(PArgList & args)
{
#if OPAL_VIDEO
  // Set the various global options
  if (args.HasOption("rx-video"))
    autoStartReceiveVideo = PTrue;
  if (args.HasOption("no-rx-video"))
    autoStartReceiveVideo = PFalse;
  if (args.HasOption("tx-video"))
    autoStartTransmitVideo = PTrue;
  if (args.HasOption("no-tx-video"))
    autoStartTransmitVideo = PFalse;

    autoStartTransmitVideo = PTrue;
   autoStartReceiveVideo = PTrue;
  if (args.HasOption("grabber")) {
    PVideoDevice::OpenArgs video = GetVideoInputDevice();
    video.deviceName = args.GetOptionString("grabber");
    video.driverName = args.GetOptionString("grabdriver");
    video.channelNumber = args.GetOptionString("grabchannel").AsInteger();
    if (!SetVideoInputDevice(video)) {
      cerr << "Unknown grabber device " << video.deviceName << "\n"
              "Available devices are:" << setfill(',') << PVideoInputDevice::GetDriversDeviceNames("") << endl;
      return PFalse;
    }
  }

  if (args.HasOption("display")) {
    PVideoDevice::OpenArgs video = GetVideoOutputDevice();
    video.deviceName = args.GetOptionString("display");
    video.driverName = args.GetOptionString("displaydriver");
    if (!SetVideoOutputDevice(video)) {
      cerr << "Unknown display device " << video.deviceName << "\n"
              "Available devices are:" << setfill(',') << PVideoOutputDevice::GetDriversDeviceNames("") << endl;
      return PFalse;
    }
  if (args.HasOption("display")) {
    PVideoDevice::OpenArgs video = GetVideoPreviewDevice();
    video.deviceName = "SDL";
    //video.driverName = args.GetOptionString("displaydriver");
    if (!SetVideoPreviewDevice(video)) {
      cerr << "Unknown display device " << video.deviceName << "\n"
              "Available devices are:" << setfill(',') << PVideoOutputDevice::GetDriversDeviceNames("") << endl;
      return PFalse;
    }
  }
  }
#endif

  if (args.HasOption('j')) {
    unsigned minJitter;
    unsigned maxJitter;
    PStringArray delays = args.GetOptionString('j').Tokenise(",-");
    if (delays.GetSize() < 2) {
      maxJitter = delays[0].AsUnsigned();
      minJitter = PMIN(GetMinAudioJitterDelay(), maxJitter);
    }
    else {
      minJitter = delays[0].AsUnsigned();
      maxJitter = delays[1].AsUnsigned();
    }
    if (minJitter >= 20 && minJitter <= maxJitter && maxJitter <= 1000)
      SetAudioJitterDelay(minJitter, maxJitter);
    else {
      cerr << "Jitter should be between 20 and 1000 milliseconds.\n";
      return PFalse;
    }
  }

  silenceDetectParams.m_mode = args.HasOption('e') ? OpalSilenceDetector::NoSilenceDetection
                                                   : OpalSilenceDetector::AdaptiveSilenceDetection;

  if (args.HasOption('D'))
    SetMediaFormatMask(args.GetOptionString('D').Lines());
  if (args.HasOption('P'))
    SetMediaFormatOrder(args.GetOptionString('P').Lines());

  cout << "Jitter buffer: "  << GetMinAudioJitterDelay() << '-' << GetMaxAudioJitterDelay() << " ms\n";

  if (args.HasOption("translate")) {
    SetTranslationAddress(args.GetOptionString("translate"));
    cout << "External address set to " << GetTranslationAddress() << '\n';
  }

  if (args.HasOption("portbase")) {
    unsigned portbase = args.GetOptionString("portbase").AsUnsigned();
    unsigned portmax  = args.GetOptionString("portmax").AsUnsigned();
    SetTCPPorts  (portbase, portmax);
    SetUDPPorts  (portbase, portmax);
    SetRtpIpPorts(portbase, portmax);
  } else {
    if (args.HasOption("tcp-base"))
      SetTCPPorts(args.GetOptionString("tcp-base").AsUnsigned(),
                  args.GetOptionString("tcp-max").AsUnsigned());

    if (args.HasOption("udp-base"))
      SetUDPPorts(args.GetOptionString("udp-base").AsUnsigned(),
                  args.GetOptionString("udp-max").AsUnsigned());

    if (args.HasOption("rtp-base"))
      SetRtpIpPorts(args.GetOptionString("rtp-base").AsUnsigned(),
                    args.GetOptionString("rtp-max").AsUnsigned());
  }

  if (args.HasOption("rtp-tos")) {
    unsigned tos = args.GetOptionString("rtp-tos").AsUnsigned();
    if (tos > 255) {
      cerr << "IP Type Of Service bits must be 0 to 255.\n";
      return PFalse;
    }
    SetRtpIpTypeofService(tos);
  }

  cout << "TCP ports: " << GetTCPPortBase() << '-' << GetTCPPortMax() << "\n"
          "UDP ports: " << GetUDPPortBase() << '-' << GetUDPPortMax() << "\n"
          "RTP ports: " << GetRtpIpPortBase() << '-' << GetRtpIpPortMax() << "\n"
          "RTP IP TOS: 0x" << hex << (unsigned)GetRtpIpTypeofService() << dec << "\n"
          "STUN server: " << flush;

  if (args.HasOption("stun"))
    SetSTUNServer(args.GetOptionString("stun"));

  if (stun != NULL)
    cout << stun->GetServer() << " replies " << stun->GetNatTypeName();
  else
    cout << "None";
  cout << '\n';

  OpalMediaFormatList allMediaFormats;

  ///////////////////////////////////////
  // Open the LID if parameter provided, create LID based endpoint
#if OPAL_LID
  if (!args.HasOption('L')) {
    PStringArray devices = args.GetOptionString("lid").Lines();
    if (devices.IsEmpty() || devices[0] == "*" || devices[0] == "*:*")
      devices = OpalLineInterfaceDevice::GetAllDevices();
    for (PINDEX d = 0; d < devices.GetSize(); d++) {
      PINDEX colon = devices[d].Find(':');
      OpalLineInterfaceDevice * lid = OpalLineInterfaceDevice::Create(devices[d].Left(colon));
      if (lid->Open(devices[d].Mid(colon+1).Trim())) {
        if (args.HasOption("country")) {
          PString country = args.GetOptionString("country");
          if (!lid->SetCountryCodeName(country))
            cerr << "Could not set LID to country name \"" << country << '"' << endl;
        }

        // Create LID protocol handler, automatically adds to manager
        if (potsEP == NULL)
          potsEP = new OpalLineEndPoint(*this);
        if (potsEP->AddDevice(lid)) {
          cout << "Line interface device \"" << devices[d] << "\" added." << endl;
          allMediaFormats += potsEP->GetMediaFormats();
        }
      }
      else {
        cerr << "Could not open device \"" << devices[d] << '"' << endl;
        delete lid;
      }
    }
  }
#endif

  ///////////////////////////////////////
  // Create PC Sound System handler

  if (!args.HasOption('S')) {
    pcssEP = new MyPCSSEndPoint(*this);

    pcssEP->SetSoundChannelBufferDepth(10);

    pcssEP->autoAnswer = args.HasOption('a');
    cout << "Auto answer is " << (pcssEP->autoAnswer ? "on" : "off") << "\n";
          
    if (!pcssEP->SetSoundDevice(args, "sound", PSoundChannel::Recorder))
      return PFalse;
    if (!pcssEP->SetSoundDevice(args, "sound", PSoundChannel::Player))
      return PFalse;
    if (!pcssEP->SetSoundDevice(args, "sound-in", PSoundChannel::Recorder))
      return PFalse;
    if (!pcssEP->SetSoundDevice(args, "sound-out", PSoundChannel::Player))
      return PFalse;

    allMediaFormats += pcssEP->GetMediaFormats();

    cout << "Sound output device: \"" << pcssEP->GetSoundChannelPlayDevice() << "\"\n"
            "Sound  input device: \"" << pcssEP->GetSoundChannelRecordDevice() << "\"\n"
#if OPAL_VIDEO
            "Video output device: \"" << GetVideoOutputDevice().deviceName << "\"\n"
            "Video  input device: \"" << GetVideoInputDevice().deviceName << '"'
#endif
         << endl;
  }

#if OPAL_H323

  ///////////////////////////////////////
  // Create H.323 protocol handler
  if (!args.HasOption("no-h323")) {
    h323EP = new H323EndPoint(*this);
    if (!InitialiseH323EP(args, false, h323EP))
      return PFalse;
  }

#endif

#if OPAL_IAX2
  ///////////////////////////////////////
  // Create IAX2 protocol handler

  if (!args.HasOption("no-iax2")) {
    iax2EP = new IAX2EndPoint(*this);
    
    if (args.HasOption('p'))
      iax2EP->SetPassword(args.GetOptionString('p'));
    
    if (args.HasOption('u')) {
      PStringArray aliases = args.GetOptionString('u').Lines();
      iax2EP->SetLocalUserName(aliases[0]);
    }
  }
#endif

#if OPAL_SIP

  ///////////////////////////////////////
  // Create SIP protocol handler

  if (!args.HasOption("no-sip")) {
    sipEP = new SIPEndPoint(*this);

    if (args.HasOption("sip-user-agent"))
      sipEP->SetUserAgent(args.GetOptionString("sip-user-agent"));

    PString str = args.GetOptionString("sip-ui");
    if (str *= "rfc2833")
      sipEP->SetSendUserInputMode(OpalConnection::SendUserInputAsSeparateRFC2833);
    else if (str *= "info-tone")
      sipEP->SetSendUserInputMode(OpalConnection::SendUserInputAsTone);
    else if (str *= "info-string")
      sipEP->SetSendUserInputMode(OpalConnection::SendUserInputAsString);

    if (args.HasOption("sip-proxy"))
      sipEP->SetProxy(args.GetOptionString("sip-proxy"));

    // set MIME format
    sipEP->SetMIMEForm(args.HasOption("use-long-mime"));

    // Get local username, multiple uses of -u indicates additional aliases
    if (args.HasOption('u')) {
      PStringArray aliases = args.GetOptionString('u').Lines();
      sipEP->SetDefaultLocalPartyName(aliases[0]);
    }

    sipEP->SetRetryTimeouts(10000, 30000);

    // Start the listener thread for incoming calls.
    PStringArray listeners = args.GetOptionString("sip-listen").Lines();
    if (!sipEP->StartListeners(listeners)) {
      cerr <<  "Could not open any SIP listener from "
            << setfill(',') << listeners << endl;
      return PFalse;
    }
    cout <<  "SIP started on " << setfill(',') << sipEP->GetListeners() << setfill(' ') << endl;

    if (args.HasOption('r')) {
      PString registrar = args.GetOptionString('r');
      cout << "Using SIP registrar " << registrar << " ... " << flush;
      if (sipEP->Register(registrar, args.GetOptionString('u'), args.GetOptionString('u'), args.GetOptionString('p'), args.GetOptionString("sip-domain"), 300 ))
        cout << "done.";
      else
        cout << "failed!";
      cout << endl;
      pauseBeforeDialing = PTrue;
    }
  }

#endif


#if OPAL_IVR
  ///////////////////////////////////////
  // Create IVR protocol handler

  if (!args.HasOption('V')) {
    ivrEP = new OpalIVREndPoint(*this);
    if (args.HasOption('x'))
      ivrEP->SetDefaultVXML(args.GetOptionString('x'));

    allMediaFormats += ivrEP->GetMediaFormats();

    PString ttsEngine = args.GetOptionString("tts");
    if (ttsEngine.IsEmpty() && PFactory<PTextToSpeech>::GetKeyList().size() > 0) 
      ttsEngine = PFactory<PTextToSpeech>::GetKeyList()[0];
    if (!ttsEngine.IsEmpty()) 
      ivrEP->SetDefaultTextToSpeech(ttsEngine);
  }
#endif

#if OPAL_FAX
  ///////////////////////////////////////
  // Create T38 protocol handler
  {
    OpalMediaFormat fmt(OpalT38); // Force instantiation of T.38 media format
    faxEP = new OpalFaxEndPoint(*this);
    t38EP = new OpalT38EndPoint(*this);

    allMediaFormats += t38EP->GetMediaFormats();
    allMediaFormats += faxEP->GetMediaFormats();
  }
#endif

  ///////////////////////////////////////
  // Set the dial peers

  if (args.HasOption('d')) {
    if (!SetRouteTable(args.GetOptionString('d').Lines())) {
      cerr <<  "No legal entries in dial peer!" << endl;
      return PFalse;
    }
  }

  if (!args.HasOption("no-std-dial-peer")) {
#if OPAL_IVR
    // Need to make sure wildcard on source ep type is first or it won't be
    // selected in preference to the specific entries below
    if (ivrEP != NULL)
      AddRouteEntry(".*:#  = ivr:"); // A hash from anywhere goes to IVR
#endif

#if OPAL_SIP
    if (sipEP != NULL) {
#if OPAL_FAX
      AddRouteEntry("t38:.*             = sip:<da>");
      AddRouteEntry("sip:.*\tfax@.*     = t38:received_fax_%s.tif;receive");
      AddRouteEntry("sip:.*\tsip:329@.* = t38:received_fax_%s.tif;receive");
#endif
      AddRouteEntry("pots:.*\\*.*\\*.*  = sip:<dn2ip>");
      AddRouteEntry("pots:.*            = sip:<da>");
      AddRouteEntry("pc:.*              = sip:<da>");
    }
#endif

#if OPAL_H323
    if (h323EP != NULL) {
      AddRouteEntry("pots:.*\\*.*\\*.* = h323:<dn2ip>");
      AddRouteEntry("pots:.*           = h323:<da>");
      AddRouteEntry("pc:.*             = h323:<da>");
#if OPAL_PTLIB_SSL
      {
        AddRouteEntry("pots:.*\\*.*\\*.* = h323s:<dn2ip>");
        AddRouteEntry("pots:.*           = h323s:<da>");
        AddRouteEntry("pc:.*             = h323s:<da>");
      }
#endif
    }
#endif

#if OPAL_LID
    if (potsEP != NULL) {
#if OPAL_H323
      AddRouteEntry("h323:.* = pots:<du>");
#if OPAL_PTLIB_SSL
      //if (h323sEP != NULL) 
        AddRouteEntry("h323s:.* = pots:<du>");
#endif
#endif
#if OPAL_SIP
      AddRouteEntry("sip:.*  = pots:<du>");
#endif
    }
    else
#endif // OPAL_LID
    if (pcssEP != NULL) {
#if OPAL_H323
      AddRouteEntry("h323:.* = pc:<du>");
#if OPAL_PTLIB_SSL
      //if (h323sEP != NULL) 
        AddRouteEntry("h323s:.* = pc:<du>");
#endif
#endif
#if OPAL_SIP
      AddRouteEntry("sip:.*  = pc:<du>");
#endif
    }
  }
                                                                                                                                            
#if OPAL_IAX2
  if (pcssEP != NULL) {
    AddRouteEntry("iax2:.*  = pc:<du>");
    AddRouteEntry("pc:.*   = iax2:<da>");
  }
#endif

#if OPAL_FAX
  if (t38EP != NULL) {
      AddRouteEntry("sip:.*  = t38:<da>");
  }
#endif

  PString defaultSrcEP = pcssEP != NULL ? "pc:*"
                                      #if OPAL_LID
                                        : potsEP != NULL ? "pots:*"
                                      #endif
                                      #if OPAL_IVR
                                        : ivrEP != NULL ? "ivr:#"
                                      #endif
                                      #if OPAL_SIP
                                        : sipEP != NULL ? "sip:localhost"
                                      #endif
                                      #if OPAL_H323
                                        : h323EP != NULL ? "sip:localhost"
                                      #endif
                                        : "";
  srcEP = args.GetOptionString("srcep", defaultSrcEP);

  if (FindEndPoint(srcEP.Left(srcEP.Find(':'))) == NULL)
    srcEP = defaultSrcEP;

  allMediaFormats = OpalTranscoder::GetPossibleFormats(allMediaFormats); // Add transcoders
  for (PINDEX i = 0; i < allMediaFormats.GetSize(); i++) {
     if (allMediaFormats[i].GetName().Left(6) == "Linear")
        mediaFormatMask.AppendString(allMediaFormats[i].GetName());
     if (allMediaFormats[i].GetName().Left(3) == "RFC")
        mediaFormatMask.AppendString(allMediaFormats[i].GetName());
     //allMediaFormats.RemoveAt(i--);

    if (!allMediaFormats[i].IsTransportable())
      allMediaFormats.RemoveAt(i--); // Don't show media formats that are not used over the wire
  }
  allMediaFormats.Remove(GetMediaFormatMask());
  allMediaFormats.Reorder(GetMediaFormatOrder());
 
  cout << "Local endpoint type: " << srcEP << "\n"
          "Codecs removed: " << setfill(',') << GetMediaFormatMask() << "\n"
          "Codec order: " << GetMediaFormatOrder() << "\n"
          "Available codecs: " << allMediaFormats << setfill(' ') << "AAA"<< endl;

#if OPAL_VIDEO
  OpalMediaFormat::GetAllRegisteredMediaFormats(allMediaFormats);
  for (PINDEX i = 0; i < allMediaFormats.GetSize(); i++) {
    OpalMediaFormat mediaFormat = allMediaFormats[i];
    if (mediaFormat.GetMediaType() == OpalMediaType::Video()) {
      if (args.HasOption("video-size")) {
        PString sizeStr = args.GetOptionString("video-size");
        unsigned width, height;
        if (PVideoFrameInfo::ParseSize(sizeStr, width, height)) {
            mediaFormat.SetOptionInteger(OpalVideoFormat::FrameWidthOption(), width);
            mediaFormat.SetOptionInteger(OpalVideoFormat::FrameHeightOption(), height);
        }
        else
          cerr << "Unknown video size \"" << sizeStr << '"' << endl;
      }

      if (args.HasOption("video-rate")) {
        unsigned rate = args.GetOptionString("video-rate").AsUnsigned();
        unsigned frameTime = 90000 / rate;
        mediaFormat.SetOptionInteger(OpalMediaFormat::FrameTimeOption(), frameTime);
      }
      OpalMediaFormat::SetRegisteredMediaFormat(mediaFormat);
    }
  }
#endif

  PStringArray options = args.GetOptionString('O').Lines();
  for (PINDEX i = 0; i < options.GetSize(); i++) {
    const PString & optionDescription = options[i];
    PINDEX colon = optionDescription.Find(':');
    PINDEX equal = optionDescription.Find('=', colon+2);
    if (colon == P_MAX_INDEX || equal == P_MAX_INDEX) {
      cerr << "Invalid option description \"" << optionDescription << '"' << endl;
      continue;
    }
    OpalMediaFormat mediaFormat = optionDescription.Left(colon);
    if (mediaFormat.IsEmpty()) {
      cerr << "Invalid media format in option description \"" << optionDescription << '"' << endl;
      continue;
    }
    PString optionName = optionDescription(colon+1, equal-1);
    if (!mediaFormat.HasOption(optionName)) {
      cerr << "Invalid option name in description \"" << optionDescription << '"' << endl;
      continue;
    }
    PString valueStr = optionDescription.Mid(equal+1);
    if (!mediaFormat.SetOptionValue(optionName, valueStr)) {
      cerr << "Invalid option value in description \"" << optionDescription << '"' << endl;
      continue;
    }
    OpalMediaFormat::SetRegisteredMediaFormat(mediaFormat);
    cout << "Set option \"" << optionName << "\" to \"" << valueStr << "\" in \"" << mediaFormat << '"' << endl;
  }

//#if PTRACING
//  allMediaFormats = OpalMediaFormat::GetAllRegisteredMediaFormats();
//  ostream & traceStream = PTrace::Begin(3, __FILE__, __LINE__);
//  traceStream << "Simple\tRegistered media formats:\n";
//  for (PINDEX i = 0; i < allMediaFormats.GetSize(); i++)
//    allMediaFormats[i].PrintOptions(traceStream);
//  traceStream << PTrace::End;
//#endif
  return PTrue;
}

#if OPAL_H323

PBoolean MyManager::InitialiseH323EP(PArgList & args, PBoolean secure, H323EndPoint * h323EP)
{
  h323EP->DisableFastStart(args.HasOption('f'));
  h323EP->DisableH245Tunneling(args.HasOption('T'));
  h323EP->SetSendGRQ(!args.HasOption("disable-grq"));


  // Get local username, multiple uses of -u indicates additional aliases
  if (args.HasOption('u')) {
    PStringArray aliases = args.GetOptionString('u').Lines();
    h323EP->SetLocalUserName(aliases[0]);
    for (PINDEX i = 1; i < aliases.GetSize(); i++)
      h323EP->AddAliasName(aliases[i]);
  }

  if (args.HasOption('b')) {
    unsigned initialBandwidth = args.GetOptionString('b').AsUnsigned()*100;
    if (initialBandwidth == 0) {
      cerr << "Illegal bandwidth specified." << endl;
      return PFalse;
    }
    h323EP->SetInitialBandwidth(initialBandwidth);
  }

  h323EP->SetGkAccessTokenOID(args.GetOptionString("gk-token"));

  PString prefix = h323EP->GetPrefixName();

  cout << prefix << " Local username: " << h323EP->GetLocalUserName() << "\n"
       << prefix << " FastConnect is " << (h323EP->IsFastStartDisabled() ? "off" : "on") << "\n"
       << prefix << " H245Tunnelling is " << (h323EP->IsH245TunnelingDisabled() ? "off" : "on") << "\n"
       << prefix << " gk Token OID is " << h323EP->GetGkAccessTokenOID() << endl;


  // Start the listener thread for incoming calls.
  PStringArray listeners = args.GetOptionString(secure ? "h323s-listen" : "h323-listen").Lines();
  if (!h323EP->StartListeners(listeners)) {
    PTRACE(3,  "Could not open any " << prefix << " listener from "
         << setfill(',') << listeners );
    return PFalse;
  }
  cout << prefix << " listeners: " << setfill(',') << h323EP->GetListeners() << setfill(' ') << endl;


  if (args.HasOption('p'))
    h323EP->SetGatekeeperPassword(args.GetOptionString('p'));

  // Establish link with gatekeeper if required.
  if (args.HasOption(secure ? "h323s-gk" : "gatekeeper")) {
    PString gkHost      = args.GetOptionString(secure ? "h323s-gk" : "gatekeeper");
    if (gkHost == "*")
      gkHost = PString::Empty();
    PString gkIdentifer = args.GetOptionString('G');
    PString gkInterface = args.GetOptionString(secure ? "h323s-listen" : "h323-listen");
    cout << "Gatekeeper: " << flush;
    if (h323EP->UseGatekeeper(gkHost, gkIdentifer, gkInterface))
      cout << *h323EP->GetGatekeeper() << endl;
    else {
      cout << "none." << endl;
      cerr << "Could not register with gatekeeper";
      if (!gkIdentifer)
        cerr << " id \"" << gkIdentifer << '"';
      if (!gkHost)
        cerr << " at \"" << gkHost << '"';
      if (!gkInterface)
        cerr << " on interface \"" << gkInterface << '"';
      if (h323EP->GetGatekeeper() != NULL) {
        switch (h323EP->GetGatekeeper()->GetRegistrationFailReason()) {
          case H323Gatekeeper::InvalidListener :
            cerr << " - Invalid listener";
            break;
          case H323Gatekeeper::DuplicateAlias :
            cerr << " - Duplicate alias";
            break;
          case H323Gatekeeper::SecurityDenied :
            cerr << " - Security denied";
            break;
          case H323Gatekeeper::TransportError :
            cerr << " - Transport error";
            break;
          default :
            cerr << " - Error code " << h323EP->GetGatekeeper()->GetRegistrationFailReason();
        }
      }
      cerr << '.' << endl;
      if (args.HasOption("require-gatekeeper")) 
        return PFalse;
    }
  }
  return PTrue;
}

#endif  //OPAL_H323


#if OPAL_PTLIB_CONFIG_FILE
void MyManager::NewSpeedDial(const PString & ostr)
{
  PString str = ostr;
  PINDEX idx = str.Find(' ');
  if (str.IsEmpty() || (idx == P_MAX_INDEX)) {
    cout << "Must specify speedial number and string" << endl;
    return;
  }
 
  PString key  = str.Left(idx).Trim();
  PString data = str.Mid(idx).Trim();
 
  PConfig config("Speeddial");
  config.SetString(key, data);
 
  cout << "Speedial " << key << " set to " << data << endl;
}
#endif // OPAL_PTLIB_CONFIG_FILE
 

void MyManager::Main(PArgList & args)
{
  OpalConnection::StringOptions stringOptions;

  // See if making a call or just listening.
  switch (args.GetCount()) {
    case 0 :
      cout << "Waiting for incoming calls\n";
      break;

    case 1 :
      if (pauseBeforeDialing) {
        cout << "Pausing to allow registration to occur..." << flush;
        PThread::Sleep(2000);
        cout << "done" << endl;
      }

      cout << "Initiating call to \"" << args[0] << "\"\n";
      SetUpCall(srcEP, args[0], currentCallToken, 0, 0, &stringOptions);
      break;

    default :
      if (pauseBeforeDialing) {
        cout << "Pausing to allow registration to occur..." << flush;
        PThread::Sleep(2000);
        cout << "done" << endl;
      }
      cout << "Initiating call from \"" << args[0] << "\"to \"" << args[1] << "\"\n";
      SetUpCall(args[0], args[1], currentCallToken, 0, 0, &stringOptions);
      break;
  }

  if (args.HasOption("disableui")) {
    while (FindCallWithLock(currentCallToken) != NULL) 
      PThread::Sleep(1000);
  }
  else {
    cout << "Press ? for help." << endl;

    PStringStream help;

    help << "Select:\n"
            "  0-9 : send user indication message\n"
            "  *,# : send user indication message\n"
            "  M   : send text message to remote user\n"
            "  C   : Connect to remote host\n"
            "  S   : Display statistics\n"
            "  O   : Put call on hold\n"
            "  T   : Transfer remote to new destination or held call\n"
            "  H   : Hang up phone\n"
            "  L   : List speed dials\n"
            "  D   : Create new speed dial\n"
            "  {}  : Increase/reduce record volume\n"
            "  []  : Increase/reduce playback volume\n"
      "  V   : Display current volumes\n";
     
    PConsoleChannel console(PConsoleChannel::StandardInput);
    for (;;) {
      // display the prompt
      cout << "Command ? " << flush;
       
       
      // terminate the menu loop if console finished
      char ch = (char)console.peek();
      if (console.eof()) {
        cout << "\nConsole gone - menu disabled" << endl;
        goto endSimpleOPAL;
      }
       
      PString line;
      console >> line;
      line = line.LeftTrim();
      ch = line[0];
      line = line.Mid(1).Trim();

      PTRACE(3, "console in audio test is " << ch);
      switch (tolower(ch)) {
      case 'x' :
      case 'q' :
        goto endSimpleOPAL;

      case '?' :       
        cout << help ;
        break;

      case 'z':
        if (currentCallToken.IsEmpty())
         cout << "Cannot stop or start record whilst no call in progress.\n";
        else if (ch == 'z') {
          StartRecording(currentCallToken, "record.wav");
          cout << "Recording started.\n";
        }
        else {
          StopRecording(currentCallToken);
          cout << "Recording stopped.\n";
        }
        break;
        
      case 'y' :
        if ( pcssEP != NULL &&
            !pcssEP->incomingConnectionToken &&
            !pcssEP->AcceptIncomingConnection(pcssEP->incomingConnectionToken))
          cout << "Could not answer connection " << pcssEP->incomingConnectionToken << endl;
        break;

      case 'n' :
        if ( pcssEP != NULL &&
            !pcssEP->incomingConnectionToken &&
            !pcssEP->RejectIncomingConnection(pcssEP->incomingConnectionToken))
          cout << "Could not reject connection " << pcssEP->incomingConnectionToken << endl;
        break;

#if OPAL_PTLIB_CONFIG_FILE
      case 'l' :
        ListSpeedDials();
        break;
 
      case 'd' :
        NewSpeedDial(line);
        break;
#endif // OPAL_PTLIB_CONFIG_FILE
        
      case 'h' :
        HangupCurrentCall();
        break;

      case 'c' :
        StartCall(line);
        break;
      case 'e' :
        SetOptions(line);
        break;

      case 'o' :
        HoldRetrieveCall();
        break;

      case 't' :
        TransferCall(line);
        break;

      case 'r':
        cout << "Current call token is \"" << currentCallToken << "\"\n";
        if (!heldCallToken.IsEmpty())
          cout << "Held call token is \"" << heldCallToken << "\"\n";
        break;

      case 'm' :
        SendMessageToRemoteNode(line);
        break;

      case 'f' :
        SendTone('x');
        break;

      default:
        if (isdigit(ch) || ch == '*' || ch == '#')
          SendTone(ch);
        break;
      }
    }
  endSimpleOPAL:
    if (!currentCallToken.IsEmpty())
      HangupCurrentCall();
  }

  cout << "Console finished " << endl;
}

void MyManager::HangupCurrentCall()
{
  PString & token = currentCallToken.IsEmpty() ? heldCallToken : currentCallToken;

  PSafePtr<OpalCall> call = FindCallWithLock(token);
  if (call == NULL)
    cout << "No call to hang up!\n";
  else {
    cout << "Clearing call " << *call << endl;
    call->Clear();
    token.MakeEmpty();
  }
}


void MyManager::HoldRetrieveCall()
{
  if (currentCallToken.IsEmpty() && heldCallToken.IsEmpty()) {
    cout << "Cannot do hold while no call in progress\n";
    return;
  }

  if (heldCallToken.IsEmpty()) {
    PSafePtr<OpalCall> call = FindCallWithLock(currentCallToken);
    if (call == NULL)
      cout << "Current call disappeared!\n";
    else if (call->Hold()) {
      cout << "Call held.\n";
      heldCallToken = currentCallToken;
      currentCallToken.MakeEmpty();
    }
  }
  else {
    PSafePtr<OpalCall> call = FindCallWithLock(heldCallToken);
    if (call == NULL)
      cout << "Held call disappeared!\n";
    else if (call->Retrieve()) {
      cout << "Call retrieved.\n";
      currentCallToken = heldCallToken;
      heldCallToken.MakeEmpty();
    }
  }
}


void MyManager::TransferCall(const PString & dest)
{
  if (currentCallToken.IsEmpty()) {
    cout << "Cannot do transfer while no call in progress\n";
    return;
  }

  if (dest.IsEmpty() && heldCallToken.IsEmpty()) {
    cout << "Must supply a destination for transfer, or have a call on hold!\n";
    return;
  }

  PSafePtr<OpalCall> call = FindCallWithLock(currentCallToken);
  if (call == NULL) {
    cout << "Current call disappeared!\n";
    return;
  }

  for (PSafePtr<OpalConnection> connection = call->GetConnection(0); connection != NULL; ++connection) {
    if (!PIsDescendant(&(*connection), OpalPCSSConnection) && !PIsDescendant(&(*connection), OpalLineConnection)) {
      connection->TransferConnection(dest.IsEmpty() ? heldCallToken : dest);
      break;
    }
  }
}


void MyManager::SendMessageToRemoteNode(const PString & str)
{
  if (str.IsEmpty()) {
    cout << "Must supply a message to send!\n";
    return;
  }

  PSafePtr<OpalCall> call = FindCallWithLock(currentCallToken);
  if (call == NULL) {
    cout << "Cannot send a message while no call in progress\n";
    return;
  }

  PSafePtr<OpalConnection> conn = call->GetConnection(0);
  while (conn != NULL) {
    conn->SendUserInputString(str);
    cout << "Sent \"" << str << "\" to " << conn->GetRemotePartyName() << endl;
    ++conn;
  }
}

void MyManager::SendTone(const char tone)
{
  if (currentCallToken.IsEmpty()) {
    cout << "Cannot send a digit while no call in progress\n";
    return;
  }

  PSafePtr<OpalCall> call = FindCallWithLock(currentCallToken);
  if (call == NULL) {
    cout << "Cannot send a message while no call in progress\n";
    return;
  }

  PSafePtr<OpalConnection> conn = call->GetConnection(0);
  while (conn != NULL) {
    conn->SendUserInputTone(tone, 180);
    cout << "Sent \"" << tone << "\" to " << conn->GetRemotePartyName() << endl;
    ++conn;
  }
}
PBoolean MyManager::CreateVideoInputDevice(const OpalConnection & /*connection*/,
                                         const OpalMediaFormat & mediaFormat,
                                         PVideoInputDevice * & device,
                                         PBoolean & autoDelete)
{
  // Make copy so we can adjust the size
  PVideoDevice::OpenArgs args = videoInputDevice;
  args.width = mediaFormat.GetOptionInteger(OpalVideoFormat::FrameWidthOption(), PVideoFrameInfo::QCIFWidth);
  args.height = mediaFormat.GetOptionInteger(OpalVideoFormat::FrameHeightOption(), PVideoFrameInfo::QCIFHeight);
  args.rate = mediaFormat.GetClockRate()/mediaFormat.GetFrameTime();

  autoDelete = PTrue;
  device = PVideoInputDevice::CreateOpenedDevice(args, false);
  PTRACE_IF(2, device == NULL, "OpalCon\tCould not open video device \"" << args.deviceName << '"');
  m_pInputDevice = device;
  return device != NULL;
}

void MyManager::SetOptions(PString line)
{
   PSafePtr<OpalCall> call = FindCallWithLock(currentCallToken);
   if (!call)
     return ;
   unsigned int w =176 ,h=144;
   PVideoFrameInfo::ParseSize(line , w, h);
   PSafePtr<OpalConnection> conn = call->GetConnection(1);
   if (conn != NULL)
   {
     OpalMediaStreamPtr  pMedia = conn->GetMediaStream(OpalMediaType::Video(), false);
      if (pMedia)
      {
        m_pInputDevice->SetFrameSize(w, h);
        OpalMediaFormat  fmt = pMedia->GetMediaFormat();
        fmt.SetOptionInteger(PLUGINCODEC_OPTION_FRAME_WIDTH,w);
        fmt.SetOptionInteger(PLUGINCODEC_OPTION_FRAME_HEIGHT,h);
        pMedia->UpdateMediaFormat(fmt);
         
      } 

   }

}
void MyManager::StartCall2(const PString & dest)
{
  //if (!currentCallToken.IsEmpty()) {
  //  cout << "Cannot make call whilst call in progress\n";
  //  return;
  //}
  if (dest.IsEmpty()) {
    cout << "Must supply hostname to connect to!\n";
    return ;
  }

  PString str = dest;

#if OPAL_PTLIB_CONFIG_FILE
  // check for speed dials, and match wild cards as we go
  PString key, prefix;
  if ((str.GetLength() > 1) && (str[str.GetLength()-1] == '#')) {
 
    key = str.Left(str.GetLength()-1).Trim(); 
    str = PString();
    PConfig config("Speeddial");
    PINDEX p;
    for (p = key.GetLength(); p > 0; p--) {
 
      PString newKey = key.Left(p);
      prefix = newKey;
      PINDEX q;
 
      // look for wild cards
      str = config.GetString(newKey + '*').Trim();
      if (!str.IsEmpty())
        break;
 
      // look for digit matches
      for (q = p; q < key.GetLength(); q++)
        newKey += '?';
      str = config.GetString(newKey).Trim();
      if (!str.IsEmpty())
        break;
    }
    if (str.IsEmpty())
      cout << "Speed dial \"" << key << "\" not defined\n";
  }
#endif // OPAL_PTLIB_CONFIG_FILE

  //if (!str.IsEmpty())
  //  SetUpCall(srcEP, str, currentCallToken);
 // OpalCall *pCall=m_pCall;
 // autoStartTransmitVideo = PTrue;
 // autoStartReceiveVideo = PTrue;

 // MakeConnection(*pCall, str, NULL,0,NULL);
 // //this->AdjustMediaFormats(*pCall->GetConnection(1), pcssEP->GetMediaFormats());
 // OpalMediaFormatList allMediaFormats;
 // OpalMediaFormat::GetAllRegisteredMediaFormats(allMediaFormats);
 // pCall->GetConnection(1)->AdjustMediaFormats(allMediaFormats);
 //if (pCall->GetConnection(1)->SetUpConnection()) {
 //   PTRACE(3, "OpalMan\tSetUpCall succeeded, call=" << *pCall);
 //   return ;
 // }

/*  PSafePtr<OpalConnection> connection = pCall->GetConnection(0);
  OpalConnection::CallEndReason endReason = connection != NULL ? connection->GetCallEndReason() : OpalConnection::NumCallEndReasons;
  pCall->Clear(endReason != OpalConnection::NumCallEndReasons ? endReason : OpalConnection::EndedByTemporaryFailure);
 */ 
  return;
}
OpalCall * MyManager::CreateCall()
{
  OpalCall* pCall = new OpalCall(*this);
  m_mapCalls.insert(make_pair(pCall->GetToken(), pCall));
  return pCall;
}
void MyManager::StartCall(const PString & dest)
{
  //if (!currentCallToken.IsEmpty()) {
  //  cout << "Cannot make call whilst call in progress\n";
  //  return;
  //}

  if (dest.IsEmpty()) {
    cout << "Must supply hostname to connect to!\n";
    return ;
  }

  PString str = dest;

#if OPAL_PTLIB_CONFIG_FILE
  // check for speed dials, and match wild cards as we go
  PString key, prefix;
  if ((str.GetLength() > 1) && (str[str.GetLength()-1] == '#')) {
 
    key = str.Left(str.GetLength()-1).Trim(); 
    str = PString();
    PConfig config("Speeddial");
    PINDEX p;
    for (p = key.GetLength(); p > 0; p--) {
 
      PString newKey = key.Left(p);
      prefix = newKey;
      PINDEX q;
 
      // look for wild cards
      str = config.GetString(newKey + '*').Trim();
      if (!str.IsEmpty())
        break;
 
      // look for digit matches
      for (q = p; q < key.GetLength(); q++)
        newKey += '?';
      str = config.GetString(newKey).Trim();
      if (!str.IsEmpty())
        break;
    }
    if (str.IsEmpty())
      cout << "Speed dial \"" << key << "\" not defined\n";
  }
#endif // OPAL_PTLIB_CONFIG_FILE

  if (!str.IsEmpty())
    SetUpCall(srcEP, str, currentCallToken);

  return;
}

#if OPAL_PTLIB_CONFIG_FILE
void MyManager::ListSpeedDials()
{
  PConfig config("Speeddial");
  PStringList keys = config.GetKeys();
  if (keys.GetSize() == 0) {
    cout << "No speed dials defined\n";
    return;
  }
 
  PINDEX i;
  for (i = 0; i < keys.GetSize(); i++)
    cout << keys[i] << ":   " << config.GetString(keys[i]) << endl;
}
#endif // OPAL_PTLIB_CONFIG_FILE

void MyManager::OnEstablishedCall(OpalCall & call)
{
  currentCallToken = call.GetToken();
  cout << "In call with " << call.GetPartyB() << " using " << call.GetPartyA() << endl;  
}

void MyManager::OnClearedCall(OpalCall & call)
{
  if (currentCallToken == call.GetToken())
    currentCallToken.MakeEmpty();
  else if (heldCallToken == call.GetToken())
    heldCallToken.MakeEmpty();

  PString remoteName = '"' + call.GetPartyB() + '"';
  switch (call.GetCallEndReason()) {
    case OpalConnection::EndedByRemoteUser :
      cout << remoteName << " has cleared the call";
      break;
    case OpalConnection::EndedByCallerAbort :
      cout << remoteName << " has stopped calling";
      break;
    case OpalConnection::EndedByRefusal :
      cout << remoteName << " did not accept your call";
      break;
    case OpalConnection::EndedByNoAnswer :
      cout << remoteName << " did not answer your call";
      break;
    case OpalConnection::EndedByTransportFail :
      cout << "Call with " << remoteName << " ended abnormally";
      break;
    case OpalConnection::EndedByCapabilityExchange :
      cout << "Could not find common codec with " << remoteName;
      break;
    case OpalConnection::EndedByNoAccept :
      cout << "Did not accept incoming call from " << remoteName;
      break;
    case OpalConnection::EndedByAnswerDenied :
      cout << "Refused incoming call from " << remoteName;
      break;
    case OpalConnection::EndedByNoUser :
      cout << "Gatekeeper or registrar could not find user " << remoteName;
      break;
    case OpalConnection::EndedByNoBandwidth :
      cout << "Call to " << remoteName << " aborted, insufficient bandwidth.";
      break;
    case OpalConnection::EndedByUnreachable :
      cout << remoteName << " could not be reached.";
      break;
    case OpalConnection::EndedByNoEndPoint :
      cout << "No phone running for " << remoteName;
      break;
    case OpalConnection::EndedByHostOffline :
      cout << remoteName << " is not online.";
      break;
    case OpalConnection::EndedByConnectFail :
      cout << "Transport error calling " << remoteName;
      break;
    default :
      cout << "Call with " << remoteName << " completed";
  }
  PTime now;
  cout << ", on " << now.AsString("w h:mma") << ". Duration "
       << setprecision(0) << setw(5) << (now - call.GetStartTime())
       << "s." << endl;

  OpalManager::OnClearedCall(call);
}


PBoolean MyManager::OnOpenMediaStream(OpalConnection & connection,
                                  OpalMediaStream & stream)
{
  if (!OpalManager::OnOpenMediaStream(connection, stream))
    return PFalse;

  cout << "Started ";

  PCaselessString prefix = connection.GetEndPoint().GetPrefixName();
  if (prefix == "pc" || prefix == "pots")
    cout << (stream.IsSink() ? "playing " : "grabbing ") << stream.GetMediaFormat();
  else if (prefix == "ivr")
    cout << (stream.IsSink() ? "streaming " : "recording ") << stream.GetMediaFormat();
  else
    cout << (stream.IsSink() ? "sending " : "receiving ") << stream.GetMediaFormat()
          << (stream.IsSink() ? " to " : " from ")<< prefix;

  cout << endl;

  return PTrue;
}



void MyManager::OnUserInputString(OpalConnection & connection, const PString & value)
{
  cout << "User input received: \"" << value << '"' << endl;
  OpalManager::OnUserInputString(connection, value);
}


///////////////////////////////////////////////////////////////

MyPCSSEndPoint::MyPCSSEndPoint(MyManager & mgr)
  : OpalPCSSEndPoint(mgr)
{
}


PBoolean MyPCSSEndPoint::OnShowIncoming(const OpalPCSSConnection & connection)
{
  incomingConnectionToken = connection.GetToken();

  if (autoAnswer)
    AcceptIncomingConnection(incomingConnectionToken);
  else {
    PTime now;
    cout << "\nCall on " << now.AsString("w h:mma")
         << " from " << connection.GetRemotePartyName()
         << ", answer (Y/N)? " << flush;
  }

  return PTrue;
}


PBoolean MyPCSSEndPoint::OnShowOutgoing(const OpalPCSSConnection & connection)
{
  PTime now;
  cout << connection.GetRemotePartyName() << " is ringing on "
       << now.AsString("w h:mma") << " ..." << endl;
  return PTrue;
}


PBoolean MyPCSSEndPoint::SetSoundDevice(PArgList & args,
                                    const char * optionName,
                                    PSoundChannel::Directions dir)
{
  if (!args.HasOption(optionName))
    return PTrue;

  PString dev = args.GetOptionString(optionName);

  if (dir == PSoundChannel::Player) {
    if (SetSoundChannelPlayDevice(dev))
      return PTrue;
  }
  else {
    if (SetSoundChannelRecordDevice(dev))
      return PTrue;
  }

  cerr << "Device for " << optionName << " (\"" << dev << "\") must be one of:\n";

  PStringArray names = PSoundChannel::GetDeviceNames(dir);
  for (PINDEX i = 0; i < names.GetSize(); i++)
    cerr << "  \"" << names[i] << "\"\n";

  return PFalse;
}

// End of File ///////////////////////////////////////////////////////////////
