/*  
 * Fax plugin codec for OPAL using SpanDSP
 *
 * Copyright (C) 2007 Post Increment, All Rights Reserved
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
 * The Original Code is Fax plugin codec for OPAL using SpanDSP
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: spandsp_fax.cpp,v $
 * Revision 1.5  2007/07/24 04:37:35  csoutheren
 * MIgrated fax code from PrePresenceBranch
 *
 * Revision 1.3.2.2  2007/07/19 08:25:22  csoutheren
 * Fix length check problem
 *
 * Revision 1.3.2.1  2007/05/18 04:24:29  csoutheren
 * Fixed Linux compile
 *
 * Revision 1.3  2007/05/10 05:32:43  csoutheren
 * Fix release build
 * Normalise log output
 * Add blocking and deblocking
 *
 * Revision 1.2  2007/03/29 08:28:58  csoutheren
 * Fix shutdown issues
 *
 * Revision 1.1  2007/03/29 04:49:20  csoutheren
 * Initial checkin
 *
 */

#include <codec/opalplugin.h>

extern "C" {
  
PLUGIN_CODEC_IMPLEMENT(SpanDSP_Fax)

};

#define USE_EMBEDDED_SPANDSP  1

#include <stdlib.h>
#include <iostream>
using namespace std;

#if defined(_WIN32) || defined(_WIN32_WCE)
  #define _CRT_SECURE_NO_DEPRECATE
  #include <windows.h>
  #include <process.h>
  #include <malloc.h>
  #include <sys/stat.h>
  #define STRCMPI  _strcmpi
#else
  #define STRCMPI  strcasecmp
  #include <semaphore.h>
  #include <time.h>
  #include <signal.h>
  #include <unistd.h>
#endif

#include <string.h>
#include <vector>
#include <map>

#if USE_EMBEDDED_SPANDSP
#include "spandsp_util/spandsp_if.h"
#else
#error "Non-embedded SpanDSP not yet supported"
#endif

const char sdpT38[]  = "t38";

#define   BITS_PER_SECOND           14400
#define   SAMPLES_PER_FRAME         160
#define   BYTES_PER_FRAME           100
#define   PREF_FRAMES_PER_PACKET    1
#define   MAX_FRAMES_PER_PACKET     1
  
/////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4100)
#endif

/////////////////////////////////////////////////////////////////
//
// define a class to implement a critical section mutex
// based on PCriticalSection from PWLib

class CriticalSection
{
  public:
    CriticalSection()
    { 
#ifdef _WIN32
      ::InitializeCriticalSection(&criticalSection); 
#else
      ::sem_init(&sem, 0, 1);
#endif
    }

    ~CriticalSection()
    { 
#ifdef _WIN32
      ::DeleteCriticalSection(&criticalSection); 
#else
      ::sem_destroy(&sem);
#endif
    }

    void Wait()
    { 
#ifdef _WIN32
      ::EnterCriticalSection(&criticalSection); 
#else
      ::sem_wait(&sem);
#endif
    }

    void Signal()
    { 
#ifdef _WIN32
      ::LeaveCriticalSection(&criticalSection); 
#else
      ::sem_post(&sem); 
#endif
    }

  private:
    CriticalSection & operator=(const CriticalSection &) { return *this; }
#ifdef _WIN32
    mutable CRITICAL_SECTION criticalSection; 
#else
    mutable sem_t sem;
#endif
};
    
class WaitAndSignal {
  public:
    inline WaitAndSignal(const CriticalSection & cs)
      : sync((CriticalSection &)cs)
    { sync.Wait(); }

    ~WaitAndSignal()
    { sync.Signal(); }

    WaitAndSignal & operator=(const WaitAndSignal &) 
    { return *this; }

  protected:
    CriticalSection & sync;
};

#if USE_EMBEDDED_SPANDSP
#if _WIN32
int socketpair(int d, int type, int protocol, socket_t fds[2])
{
  fds[0] = fds[1] = INVALID_SOCKET;

  if (d != AF_UNIX || type != SOCK_DGRAM)
    return -1;

  // get local IP address
  in_addr hostAddress;
  {
    char hostname[80];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR)
      return -1;
    struct hostent * he = gethostbyname(hostname);
    if (he == 0)
      return -1;
    int i;
    for (i = 0; he->h_addr_list[i] != 0; ++i) {
      if (*(u_long *)(he->h_addr_list[i]) != INADDR_LOOPBACK) {
        memcpy(&hostAddress, he->h_addr_list[i], sizeof(hostAddress));
        break;
      }
    }
    if (he->h_addr_list[i] == 0)
      return -1;
  }

  // windows does not support AF_UNIX, so use localhost instead
  sockaddr_in sockAddrs[2];
  int sockLens[2];
  
  int i;

  for (i = 0; i < 2; ++i) {
    if ((fds[i] = socket(AF_INET, type, 0)) == INVALID_SOCKET)
      break;

    sockLens[i] = sizeof(sockAddrs[0]);
    memset(&sockAddrs[i], 0, sizeof(sockLens[i]));

    sockAddrs[i].sin_family  = AF_INET;
    sockAddrs[i].sin_addr    = hostAddress;
    sockAddrs[i].sin_port    = 0;

    if (bind(fds[i], (sockaddr *)&sockAddrs[i], sockLens[i]) != 0) 
      break;

    sockLens[i] = sizeof(sockAddrs[0]);
    memset(&sockAddrs[i], 0, sizeof(sockLens[i]));

    if (getsockname(fds[i], (sockaddr *)&sockAddrs[i], &sockLens[i]) != 0) 
      break;
  }

  if (i == 2) {
    if (connect(fds[0], (sockaddr *)&sockAddrs[1], sockLens[1]) == 0) {
      if (connect(fds[1], (sockaddr *)&sockAddrs[0], sockLens[0]) == 0)
        return 0;
    }
  }

  cerr << "last error is " << WSAGetLastError() << endl;

  closesocket(fds[0]);
  closesocket(fds[1]);

  fds[0] = fds[1] = INVALID_SOCKET;

  return -1;
}
#endif
#endif

/////////////////////////////////////////////////////////////////

typedef std::vector<unsigned char> InstanceKey;

class FaxInstance
{
  public:
    CriticalSection mutex;

    FaxInstance();
    ~FaxInstance();

    bool WritePCM(const void * from, unsigned * fromLen);
    bool ReadPCM(void * to, unsigned * toLen, bool & moreToRead);

    bool WriteT38(const void * from, unsigned * fromLen);
    bool ReadT38(void * to, unsigned  * toLen);

    unsigned refCount;

#if USE_EMBEDDED_SPANDSP
    SpanDSP::T38Gateway t38Gateway;
    void GatewayMain();
    SpanDSP::AdaptiveDelay writeDelay;

#if _WIN32
    SOCKET t38Sockets[2];
    SOCKET faxSockets[2];
    HANDLE threadHandle;
#else
    int t38Sockets[2];
    int faxSockets[2];
    pthread_t threadHandle;
#endif
#endif // USE_EMBEDDED_SPANDSP
    bool Open();
    bool first;
};

#if USE_EMBEDDED_SPANDSP

extern "C" {
#if _WIN32
static unsigned __stdcall GatewayMain_Static(void * userData)
#else
static void * GatewayMain_Static(void * userData)
#endif
{
  FaxInstance * fax = (FaxInstance*)userData;
  if (fax != NULL)
    fax->GatewayMain();
  return 0;
}
};

FaxInstance::FaxInstance()
{ 
  refCount = 0; 
  first    = true;
#if _WIN32
  t38Sockets[0] = t38Sockets[1] = INVALID_SOCKET;
  faxSockets[0] = faxSockets[1] = INVALID_SOCKET;
  threadHandle = NULL;
#else
  t38Sockets[0] = t38Sockets[1] = -1;
  faxSockets[0] = faxSockets[1] = -1;
  threadHandle = 0;
#endif
}

FaxInstance::~FaxInstance()
{
#if _WIN32
  if (t38Sockets[0] != INVALID_SOCKET)
    closesocket(t38Sockets[0]);
  if (t38Sockets[1] != INVALID_SOCKET)
    closesocket(t38Sockets[1]);
  if (faxSockets[0] != INVALID_SOCKET)
    closesocket(faxSockets[0]);
  if (faxSockets[1] != INVALID_SOCKET)
    closesocket(faxSockets[1]);
  if (threadHandle != NULL) {
    DWORD result;
    int retries = 10;
    while ((result = WaitForSingleObject(threadHandle, 1000)) != WAIT_TIMEOUT) {
      if (result == WAIT_OBJECT_0)
        break;
      if (::GetLastError() != ERROR_INVALID_HANDLE) 
        break;
      if (retries == 0)
        break;
      retries--;
    }
    CloseHandle(threadHandle);
  }
#else
  if (t38Sockets[0] != -1)
    close(t38Sockets[0]);
  if (t38Sockets[1] != -1)
    close(t38Sockets[1]);
  if (faxSockets[0] != -1)
    close(faxSockets[0]);
  if (faxSockets[1] != -1)
    close(faxSockets[1]);
  if (threadHandle != 0) {
    int i = 20;
    while (i-- > 0) {
      if (pthread_kill(threadHandle, 0) == 0)
        break;
      usleep(100000);
    }
  }
#endif
}

void FaxInstance::GatewayMain()
{
  t38Gateway.Serve(faxSockets[1], t38Sockets[1]);
}

bool FaxInstance::Open()
{
  SpanDSP::progmode = "SpanDSP_Fax";

  // open the sockets
  if ((socketpair(AF_UNIX, SOCK_DGRAM, 0, faxSockets) != 0) || 
      (socketpair(AF_UNIX, SOCK_DGRAM, 0, t38Sockets) != 0))
    return false;

  t38Gateway.SetVersion(0);

  // start the gateway object
  t38Gateway.Start();

  // put gateway into a seperate thread

#if _WIN32

  threadHandle = (HANDLE)_beginthreadex(NULL, 10000, &GatewayMain_Static, this, 0, NULL);
  return TRUE;

#else

  pthread_attr_t threadAttr;
  pthread_attr_init(&threadAttr);
  pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
  return pthread_create(&threadHandle, &threadAttr, &GatewayMain_Static, this) == 0;

#endif

}

bool FaxInstance::WritePCM(const void * from, unsigned * fromLen)
{
  return sendto(faxSockets[0], 12+(const char *)from, *fromLen-12, 0, NULL, 0) == (int)(*fromLen-12);
}

bool FaxInstance::ReadPCM(void * to, unsigned * toLen, bool & moreToRead)
{
  moreToRead = false;
  static short seq = 0;

  if (*toLen < 320+12)
    return false;
  int stat = recvfrom(faxSockets[0], 12+(char *)to, 320, 0, NULL, 0);
  if (stat < 0) {
    cerr << "fax read failed" << endl;
    return false;
  }

#if WRITE_PCM_FILES
  static int file = _open("codec_read.pcm", _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE);
  if (file >= 0) {
    if (_write(file, 12+(char *)to, stat) < stat) {
      cerr << "cannot write codec read PCM data to file" << endl;
      file = -1;
    }
  }
#endif

  if (stat == 320)
    *toLen = 320+12;
  else {
    *toLen = 0;
    cerr << "fax read returned error" << endl;
  }

  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(faxSockets[0], &fds);

  timeval timeout;
  timeout.tv_sec  = 0;
  timeout.tv_usec = 0;
  moreToRead = select(faxSockets[0]+1, &fds, NULL, NULL, &timeout) > 0;

  return TRUE;
}

bool FaxInstance::WriteT38(const void * from, unsigned * fromLen)
{
  return sendto(t38Sockets[0], (const char *)from, *fromLen, 0, NULL, 0) == (int)*fromLen;
}

bool FaxInstance::ReadT38(void * to, unsigned  * toLen)
{
  // T.38 packets do not arrive regularly, so don't block waiting for them
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(t38Sockets[0], &fds);

  int delay = writeDelay.Calculate(20);
  if (delay == 0)
    delay = 1;
  timeval timeout;
  timeout.tv_sec  = 0;
  timeout.tv_usec = delay * 1000;
  int stat = select(t38Sockets[0]+1, &fds, NULL, NULL, &timeout);

  if (stat == 0) {
    *toLen = 0;
    return true;
  }
  stat = recvfrom(t38Sockets[0], (char *)to, *toLen, 0, NULL, 0);
  if (stat < 0)
    return false;
  *toLen = stat;

  return true;
}

#endif // USE_EMBEDDED_SPANDSP


typedef std::map<InstanceKey, FaxInstance *> InstanceMapType_T;
static InstanceMapType_T instanceMap;
CriticalSection instanceMapMutex;

class FaxCodecContext
{
  public:
    FaxCodecContext()
    { 
      key.resize(0); 
      instance = NULL;
    }

    ~FaxCodecContext()
    {
      if ((instance == NULL) || (key.size() == 0))
        return;

      WaitAndSignal m(instanceMapMutex);

      InstanceMapType_T::iterator r = instanceMap.find(key);
      if (r != instanceMap.end()) {
        instance = r->second;
        instance->mutex.Wait();
        if (instance->refCount > 0)
          --instance->refCount;
        else {
          instance->mutex.Signal();
          delete instance;
          instance = NULL;
        }
      }
    }

    bool StartCodec()
    {
      if (instance != NULL)
        return true;

      if (key.size() == 0)
        return false;

      WaitAndSignal m(instanceMapMutex);

      InstanceMapType_T::iterator r = instanceMap.find(key);
      if (r != instanceMap.end()) {
        instance = r->second;
        instance->mutex.Wait();
        ++instance->refCount;
        instance->mutex.Signal();
      }
      else {
        instance = new FaxInstance();
        if (!instance->Open())
          return false;

        instance->mutex.Wait();
        instanceMap.insert(InstanceMapType_T::value_type(key, instance));
        instance->mutex.Signal();
      }
      return true;
    }

    InstanceKey key;
    FaxInstance * instance;
};

static void * create_encoder(const struct PluginCodec_Definition * codec)
{
  return new FaxCodecContext();
}

static void destroy_coder(const struct PluginCodec_Definition * codec, void * _context)
{
  if (_context == NULL)
    return;
  
  FaxCodecContext * context = (FaxCodecContext *)_context;
  delete context;
}

static int codec_pcm_to_t38(const struct PluginCodec_Definition * codec, 
                                           void * _context,
                                     const void * from, 
                                       unsigned * fromLen,
                                           void * to,         
                                       unsigned * toLen,
                                   unsigned int * flag)
{
  if (_context == NULL)
    return 0;

  FaxCodecContext & context = *(FaxCodecContext *)_context;

  if ((context.instance == NULL) && !context.StartCodec())
    return 0;

  context.instance->WritePCM(from, fromLen);
  context.instance->ReadT38(to, toLen);

  *flag = PluginCodec_ReturnCoderLastFrame;
  
  return 1;
}

static void * create_decoder(const struct PluginCodec_Definition * codec)
{
  return new FaxCodecContext();
}

static int codec_t38_to_pcm(const struct PluginCodec_Definition * codec, 
                                           void * _context,
                                     const void * from, 
                                       unsigned * fromLen,
                                           void * to,         
                                       unsigned * toLen,
                                   unsigned int * flag)
{
  if (_context == NULL)
    return 0;
  
  FaxCodecContext & context = *(FaxCodecContext *)_context;
  
  if ((context.instance == NULL) && !context.StartCodec())
    return 0;

  // ignore padding frames
  if ((*fromLen == 13) && (((const unsigned char *)from)[12] == 0xff))
    ;
  // only write T.38 data if there is a payload
  else if (*fromLen > 12)
    context.instance->WriteT38(from, fromLen);

  // always read PCM
  bool moreToRead;
  context.instance->ReadPCM(to, toLen, moreToRead);

  *flag = moreToRead ? 0 : PluginCodec_ReturnCoderLastFrame;

  return 1;
}

static int set_instance_id(
      const struct PluginCodec_Definition * codec, 
      void * _context, 
      const char * , 
      void * parm , 
      unsigned * parmLen)
{
  if (_context == NULL || parm == NULL || parmLen == NULL)
    return 0;

  FaxCodecContext & context = *(FaxCodecContext *)_context;

  context.key.resize(*parmLen);
  memcpy(&context.key[0], parm, *parmLen);

  return 1;
}

static int valid_for_sip(
      const struct PluginCodec_Definition * codec, 
      void * context, 
      const char * key, 
      void * parm , 
      unsigned * parmLen)
{
  if (parmLen == NULL || parm == NULL || *parmLen != sizeof(char *))
    return 0;

  return (STRCMPI((const char *)parm, "sip") == 0) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_information licenseInfo = {
  1081086550,                              // timestamp = Sun 04 Apr 2004 01:49:10 PM UTC = 

  "Craig Southeren, Post Increment",                           // source code author
  "1.0",                                                       // source code version
  "craigs@postincrement.com",                                  // source code email
  "http://www.postincrement.com",                              // source code URL
  "Copyright (C) 2007 by Post Increment, All Rights Reserved", // source code copyright
  "MPL 1.0",                                                   // source code license
  PluginCodec_License_MPL,                                     // source code license
  
  "T.38 Fax Codec",                                            // codec description
  "Craig Southeren",                                           // codec author
  "Version 1",                                                 // codec version
  "craigs@postincrement.com",                                  // codec email
  "",                                                          // codec URL
  "",                                                          // codec copyright information
  NULL,                                                        // codec license
  PluginCodec_License_MPL                                      // codec license code
};

/////////////////////////////////////////////////////////////////////////////

static const char L16Desc[]  = { "L16" };

static const char t38CodecName[]  = { "T.38" };

static char * default_t38_parms[] = {
//  { "fmtp",                       "sr=16000,mode=any" ,     "s" },
   "T38FaxVersion",                      "<0",                "i",
   "T38MaxBitRate",                      "<14400",            "i",
   "T38FaxRateManagement",               "transferredTCF",    "e:localTCF:transferredTCF",
   "T38FaxMaxBuffer",                    "<72",               "i",
   "T38FaxMaxDatagram",                  "<316",              "i", 
   "T38FaxUdpEC",                        "t38UDPFEC",         "e:t38UDPRedundancy:t38UDPFEC",

   //"T38FaxFillBitRemoval",               "false",             "b"
   //"T38FaxTranscodingMMR",               "false",             "b"
   //"T38FaxTranscodingJBIG",              "false",             "b"

   NULL, NULL, NULL
};

static int coder_get_sip_options (
      const struct PluginCodec_Definition * codec, 
      void * context, 
      const char * key, 
      void * parm , 
      unsigned * parmLen)
{
  if (parmLen == NULL || parm == NULL || *parmLen != sizeof(char *))
    return 0;

  char ***options = (char ***)parm;

  *options = default_t38_parms;

  return 1; 
}


static struct PluginCodec_ControlDefn sipCoderControls[] = {
  { "set_instance_id",          set_instance_id       },
  { "valid_for_protocol",       valid_for_sip         },
  { "get_codec_options",        coder_get_sip_options },
//  { "set_codec_options",        coder_set_sip_options },
  { NULL }
};

/////////////////////////////////////////////////////////////////////////////


static struct PluginCodec_Definition faxCodecDefn[2] = {

{ 
  // encoder
  PLUGIN_CODEC_VERSION_FAX,           // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeFax |          // audio codec
  PluginCodec_InputTypeRaw |          // raw input data
  PluginCodec_OutputTypeRTP |         // RTP output data
  PluginCodec_RTPTypeDynamic,         // dynamic RTP type

  t38CodecName,                       // text decription
  L16Desc,                            // source format
  t38CodecName,                       // destination format

  0,                                  // user data (no WAV49)

  8000,                               // samples per second
  BITS_PER_SECOND,                    // raw bits per second
  20000,                              // nanoseconds per frame
  SAMPLES_PER_FRAME,                  // samples per frame
  BYTES_PER_FRAME,                    // bytes per frame
  PREF_FRAMES_PER_PACKET,             // recommended number of frames per packet
  MAX_FRAMES_PER_PACKET,              // maximum number of frames per packe
  0,                                  // dynamic payload
  sdpT38,                             // RTP payload name

  create_encoder,                     // create codec function
  destroy_coder,                      // destroy codec
  codec_pcm_to_t38,                   // encode/decode
  sipCoderControls,                   // codec controls

  PluginCodec_H323T38Codec,           // h323CapabilityType 
  NULL                                // h323CapabilityData
},

{ 
  // decoder
  PLUGIN_CODEC_VERSION_FAX,           // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeFax |          // audio codec
  PluginCodec_InputTypeRTP |          // raw input RTP
  PluginCodec_OutputTypeRaw |         // raw output data
  PluginCodec_RTPTypeDynamic,         // dynamic RTP type

  t38CodecName,                       // text decription
  t38CodecName,                       // source format
  L16Desc,                            // destination format

  0,                                  // user data

  8000,                               // samples per second
  BITS_PER_SECOND,                    // raw bits per second
  20000,                              // nanoseconds per frame
  SAMPLES_PER_FRAME,                  // samples per frame
  BYTES_PER_FRAME,                    // bytes per frame
  PREF_FRAMES_PER_PACKET,             // recommended number of frames per packet
  MAX_FRAMES_PER_PACKET,              // maximum number of frames per packe
  0,                                  // dynamic payload
  sdpT38,                             // RTP payload name

  create_decoder,                     // create codec function
  destroy_coder,                      // destroy codec
  codec_t38_to_pcm,                   // encode/decode
  sipCoderControls,                   // codec controls

  PluginCodec_H323T38Codec,            // h323CapabilityType 
  NULL                                 // h323CapabilityData
},


};

#define NUM_DEFNS   (sizeof(faxCodecDefn) / sizeof(struct PluginCodec_Definition))

/////////////////////////////////////////////////////////////////////////////

extern "C" {

PLUGIN_CODEC_DLL_API struct PluginCodec_Definition * PLUGIN_CODEC_GET_CODEC_FN(unsigned * count, unsigned version)
{
  *count = NUM_DEFNS;
  return faxCodecDefn;
}

};
