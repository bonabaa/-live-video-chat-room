/*
 * spandsp_if.cpp
 *
 * A C++ interface to the SpanDSP library
 *
 * Written by Craig Southeren <craigs@postincrement.com>
 *
 * Copyright (C) 2007 Craig Southeren
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: spandsp_if.cpp,v 1.7 2007/07/24 04:39:44 csoutheren Exp $
 */

#include <iostream>

#if defined (_WIN32) || defined (_WIN32_WCE)
  #pragma warning(disable: 4996)
  #include <winsock2.h>
  #include <Ws2tcpip.h>
  #include <io.h>
  #include <sys/types.h>
  #include <sys/stat.h>
#else
  #include <unistd.h>
  #include <sys/time.h>
  #include <sys/socket.h>
  #include <stdint.h>
  #include <netinet/in.h>
  #include <sys/ioctl.h>
#endif

#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <tiffio.h>
#include <fcntl.h>

#include <string>
#include <queue>
#include <string>
using namespace std;

#include "spandsp_if.h"

namespace SpanDSP {
char * progname = "(no mode)";
char * progmode = "(no mode)";
}

static bool firstT38Read = true;
static bool firstT38Write = true;

static bool firstAudioRead = true;
static bool firstAudioWrite = true;

#define SAMPLES_PER_CHUNK 160

struct RTPHeader {
  unsigned int flags:8;
  unsigned int payloadMarker:8;
  unsigned int sequence:16;
  unsigned int timestamp:32;
  unsigned int ssrc:32;
};

//#define USE_PACING

//////////////////////////////////////////////////////////////////////////////////
// 
//  Windows specific network functions
//

#ifdef _WIN32

typedef int socklen_t;

typedef SOCKET socket_t;

struct iovec {
   void *iov_base;   /* Starting address */
   size_t iov_len;   /* Number of bytes */
};

struct msghdr {
    void         * msg_name;     /* optional address */
    socklen_t    msg_namelen;    /* size of address */
    struct iovec * msg_iov;      /* scatter/gather array */
    size_t       msg_iovlen;     /* # elements in msg_iov */
    int          msg_flags;      /* flags on received message */
};

inline int __socket_recvfrom(socket_t fd, void * buf, int len, int flags, sockaddr * from, int * fromLen)  { return ::recvfrom(fd, (char *)buf, len, flags, from, fromLen); }
inline int __socket_read    (socket_t fd, void * buf, int len)                                             { return ::recvfrom(fd, (char *)buf, len, 0, NULL, NULL); }
inline int __socket_sendto  (socket_t fd, const void * data, int len, int flags, sockaddr *to, int toLen)  { return ::sendto(fd, (const char *)data, len, flags, to, toLen); }
inline int __socket_ioctl   (socket_t fd, int request, int * data)                                         { return ::ioctlsocket(fd, request, (u_long *)data); }

inline int __socket_getlasterror() { return WSAGetLastError(); }

ostream & __socket_error(ostream & strm) { int err = __socket_getlasterror(); strm << " code " << err; return strm; }

inline bool __socket_iseagain(int code) { return code == WSAEWOULDBLOCK || code == WSAECONNRESET; }

int __socket_sendmsg(socket_t fd, const struct msghdr *msg, int flags)
{
  if (msg == NULL)
    return -1;

  // buffer bigger than we should ever send
  char buffer[1024];
  size_t bufferLen = 0;

  // copy all of the vectors into the buffer
  size_t i;
  for (i = 0; i < msg->msg_iovlen && (bufferLen < sizeof(buffer)); ++i) {
    if (msg->msg_iov[i].iov_base != NULL && msg->msg_iov[i].iov_len != 0) {
      memcpy(buffer+bufferLen, msg->msg_iov[i].iov_base, msg->msg_iov[i].iov_len);
      bufferLen += msg->msg_iov[i].iov_len;
    }
  }

  // now write it
  return __socket_sendto(fd, buffer, (int)bufferLen, msg->msg_flags, (sockaddr *)msg->msg_name, (int)msg->msg_namelen);
}

int __socket_recvmsg(socket_t fd, struct msghdr *msg, int flags)
{
  if (msg == NULL)
    return -1;

  // buffer bigger than we should ever receive
  char buffer[2000];
  int bufferLen = 0;

  bufferLen = __socket_recvfrom(fd, buffer, sizeof(buffer), flags, (sockaddr *)msg->msg_name, (msg->msg_name == NULL) ? NULL : (int *)&msg->msg_namelen);
  if (bufferLen <= 0)
    return -1;

  size_t i = 0;
  size_t lenSoFar = 0;
  while (lenSoFar < (size_t)bufferLen && i < msg->msg_iovlen) {
    if (msg->msg_iov[i].iov_base != NULL && msg->msg_iov[i].iov_len != 0) {
      size_t len = msg->msg_iov[i].iov_len;
      if (len > bufferLen - lenSoFar)
        len = bufferLen - lenSoFar;
      memcpy(msg->msg_iov[i].iov_base, buffer+lenSoFar, len);
      lenSoFar += len;
      ++i;
    }
  }

  return (int)lenSoFar;
}

inline void __sleep(int len)  { if (len > 0) ::Sleep(len); }

#else

//////////////////////////////////////////////////////////////////////////////////
// 
//  Unix specific network functions
//

typedef int socket_t;

inline int __socket_recvfrom(socket_t fd, void * buf, int len, int flags, sockaddr * from, socklen_t * fromLen)  { return ::recvfrom(fd, (char *)buf, len, flags, from, fromLen); }
inline int __socket_read(socket_t fd, void * buf, int len)                                                 { return ::read(fd, buf, len); }
inline int __socket_sendto  (socket_t fd, const void * data, int len, int flags, sockaddr *to, socklen_t toLen)  { return ::sendto(fd, data, len, flags, to, toLen); }
inline int __socket_write   (socket_t fd, const void * data, int len)                                            { return ::write(fd, data, len); }
inline int __socket_ioctl   (socket_t fd, int request, int * data)                                        { return ::ioctl(fd, request, data); }

inline int __socket_sendmsg(socket_t fd, const struct msghdr *msg, int flags)  { return ::sendmsg(fd, msg, flags); }
inline int __socket_recvmsg(socket_t fd, struct msghdr *msg, int flags)        { return ::recvmsg(fd, msg, flags); }

inline int __socket_getlasterror() { return errno; }
inline bool __socket_iseagain(int code) { return code == EAGAIN; }
inline ostream & __socket_error(ostream & strm) { strm << "(" << __socket_getlasterror() << ") " << strerror(__socket_getlasterror()); return strm; }

inline void __sleep(int len)  { if (len > 0) usleep(len * 1000); }

#endif

//////////////////////////////////////////////////////////////////////////////////
// 
//  Implement an adpative delay that allows approximation of a real timer
//

SpanDSP::AdaptiveDelay::__time_t SpanDSP::AdaptiveDelay::GetTime()
{
#ifdef _WIN32
  SYSTEMTIME sysTime;
  GetSystemTime(&sysTime);

  FILETIME fileTime;
  SystemTimeToFileTime(&sysTime, &fileTime);

  ULARGE_INTEGER uTime;
  uTime.HighPart = fileTime.dwHighDateTime;
  uTime.LowPart  = fileTime.dwLowDateTime;

  return (uTime.QuadPart + 5000)/ 10000;
#else
  timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec * 1000) + ((tv.tv_usec + 500) / 1000);
#endif
}

SpanDSP::AdaptiveDelay::AdaptiveDelay()
{
  Start();
}

void SpanDSP::AdaptiveDelay::Start()
{
  first = true;
  accumulator = 0;
}

void SpanDSP::AdaptiveDelay::Delay(int delay)
{
  __sleep(Calculate(delay));
}

int SpanDSP::AdaptiveDelay::Calculate(int delay)
{
  if (first) {
    lastTime    = GetTime();
    accumulator = delay;
    first = false;
    return delay;
  } else {
    __time_t now = GetTime();
    __time_t actual = now - lastTime;

    //accumulator = (accumulator * 3) / 4;
    accumulator -= actual;

    accumulator += delay;

    if (accumulator < -4*delay)
      accumulator = -delay*4;
    else if (accumulator > 4*delay)
      accumulator = delay*4;

//cerr << "accum  = " << accumulator << "(actual " << actual << ")" << endl;

    lastTime = now;

    if (accumulator > 0)
      return (int)accumulator;

    return 0;
  }
}

//////////////////////////////////////////////////////////////////////////////////
// 
//  print a IP address/port address 
//

static void PrintSocketAddr(const sockaddr_in & addr, ostream & strm)
{
  strm << inet_ntoa(addr.sin_addr) << ":" << htons(addr.sin_port);
}

//////////////////////////////////////////////////////////////////////////////////
// 
//  Read a packet of audio from a UDP socket
//

static bool ReadAudioPacket(socket_t fd, short * data, int & len, sockaddr_in & address, bool & listen, bool verbose)
{
  len = SAMPLES_PER_CHUNK*2;
  if (!listen) 
    len = __socket_read(fd, data, len);
  else {
    socklen_t sockLen = sizeof(address);
    len = __socket_recvfrom(fd, data, len, 0, (sockaddr *)&address, &sockLen);
  }  

  if (len > 0) {
    if (verbose && firstAudioRead) {
      cout << SpanDSP::progmode << " first read from audio socket" << endl;
      firstAudioRead = false;
    }

    if (listen) {
      if (verbose)
        cout << "info: remote address set to ";
      PrintSocketAddr(address, cout);
      cout << endl;
      listen = false;
      int cmd = 0;
      if (__socket_ioctl(fd, FIONBIO, &cmd) != 0) {
        cerr << SpanDSP::progmode << ": cannot set socket into blocking mode" << endl;
         return false;
      }
      while (len < SAMPLES_PER_CHUNK*2) {
        data[len/2] = 0;
        len += 2;
      }
    }
  }
  else {
    int err = __socket_getlasterror();
    if ((len < 0) && (!__socket_iseagain(err))) {
      cerr << SpanDSP::progmode << ": read from socket failed " ; __socket_error(cerr) << endl;
      return false;
    }
    if (listen)
      len = 0;
    else {
//      cerr << progmode << " missed audio packet" << endl;
      memset(data, 0, SAMPLES_PER_CHUNK*2);
      len = SAMPLES_PER_CHUNK*2;
    }
  }
  return true;
}


//////////////////////////////////////////////////////////////////////////////////
// 
//  Implement an entity that sends or receives faxes
//

SpanDSP::FaxElement::FaxElement(bool _transmitter, bool _verbose)
  : transmitter(_transmitter), verbose(_verbose)
{
  finished = false;
  useECM   = false;
}

void SpanDSP::FaxElement::phase_b_handler(t30_state_t *s, void *user_data, int result)
{
  FaxElement * element = (FaxElement *)user_data;
  if (element != NULL)
    element->PhaseBHandler(result);
}

void SpanDSP::FaxElement::phase_d_handler(t30_state_t *s, void *user_data, int result)
{
  FaxElement * element = (FaxElement *)user_data;
  if (element != NULL)
    element->PhaseDHandler(result);
}

void SpanDSP::FaxElement::phase_e_handler(t30_state_t *s, void *user_data, int result)
{
  FaxElement * element = (FaxElement *)user_data;
  if (element != NULL)
    element->PhaseEHandler(result);
}

void SpanDSP::FaxElement::PhaseBHandler(int)
{ }

void SpanDSP::FaxElement::PhaseDHandler(int)
{ }

void SpanDSP::FaxElement::PhaseEHandler(int)
{ } 

void SpanDSP::FaxElement::SetLocalStationID(const std::string & str)
{ localStationID = str; }

void SpanDSP::FaxElement::SetECM(bool v)
{ useECM = v; }

bool SpanDSP::FaxElement::GetECM() const
{ return useECM; }

//////////////////////////////////////////////////////////////////////////////////
// 
//  Implement an entity that sends or receives faxes via audio tones to or from TIFF files
//

SpanDSP::FaxTerminal::FaxTerminal(bool _transmitter, bool _verbose)
  : FaxElement(_transmitter, _verbose)
{ }

SpanDSP::FaxTerminal::~FaxTerminal()
{
  ::fax_release(&faxState);
}

void SpanDSP::FaxTerminal::Start()
{
  ::fax_init(&faxState, transmitter ? 1 : 0);
                    faxState.logging.level = verbose ? SPAN_LOG_SHOW_SEVERITY | SPAN_LOG_SHOW_PROTOCOL | SPAN_LOG_DEBUG : 0;
  //::t30_set_ecm_capability(&faxState.t30_state, useECM ? 1 : 0);
  if (localStationID.length() != 0)
    ::t30_set_local_ident(&faxState.t30_state, localStationID.c_str());
  else
    ::t30_set_local_ident(&faxState.t30_state, " ");
}

bool SpanDSP::FaxTerminal::PutPCMData(const short * pcm, unsigned sampleCount)
{
  return (::fax_rx(&faxState, (int16_t *)pcm, sampleCount) != 0) ? false : true;
}

unsigned SpanDSP::FaxTerminal::GetPCMData(short * pcm, unsigned sampleCount)
{
  unsigned len = ::fax_tx(&faxState, pcm, sampleCount);
  if (len < sampleCount) {
    memset(pcm + len, 0, (sampleCount - len)*2);
    len = sampleCount;
  }
  return len;
}

void SpanDSP::FaxTerminal::tx_data_handler(t30_state_t *s, void *user_data, unsigned code, unsigned len)
{
  FaxTerminal * terminal = (FaxTerminal *)user_data;
  if (terminal != NULL)
    terminal->TXDataHandler(code, len);
}

void SpanDSP::FaxTerminal::TXDataHandler(unsigned code, unsigned len)
{
//#if 0
  switch (code) {
    case T30_MODEM_NONE:
      printf("SILENCE %i\n", len);
      break;
    case T30_MODEM_PAUSE:
      printf("T30_MODEM_PAUSE %i\n", len);
      break;
    case T30_MODEM_CED:
      printf("T30_MODEM_CED %i\n", len);
      break;
    case T30_MODEM_CNG:
      printf("T30_MODEM_CNG %i\n", len);
      break;
    case T30_MODEM_V21:
      printf("T30_MODEM_V21 %i\n", len);
      break;
    case T30_MODEM_V27TER_2400:
      printf("T30_MODEM_V27TER_2400 %i\n", len);
      break;
    case T30_MODEM_V27TER_4800:
      printf("T30_MODEM_V27TER_4800 %i\n", len);
      break;
    case T30_MODEM_V29_7200:
      printf("T30_MODEM_V29_7200 %i\n", len);
      break;
    case T30_MODEM_V29_9600:
      printf("T30_MODEM_V29_9600 %i\n", len);
      break;
    case T30_MODEM_V17_7200:
      printf("T30_MODEM_V17_7200 %i\n", len);
      break;
    case T30_MODEM_V17_9600:
      printf("T30_MODEM_V17_9600 %i\n", len);
      break;
    case T30_MODEM_V17_12000:
      printf("T30_MODEM_V17_12000 %i\n", len);
      break;
    case T30_MODEM_V17_14400:
      printf("T30_MODEM_V17_14400 %i\n", len);
      break;
    default:
      printf("unknown %i\n", code, len);
      break;
  }
//#endif
}

void SpanDSP::FaxTerminal::SetLocalStationID(const std::string & str)
{ localStationID = str; }

bool SpanDSP::FaxTerminal::SendFiller() const
{ return false; }

bool SpanDSP::FaxTerminal::Serve(socket_t fd)
{
  sockaddr_in peer;
  socklen_t len = sizeof(peer);
  if (getpeername(fd, (sockaddr *)&peer, &len) != 0) 
    return false;
  return Serve(fd, peer, false);
}

bool SpanDSP::FaxTerminal::Serve(socket_t fd, sockaddr_in & address, bool listen)
{
  int port;
  {
    sockaddr_in local;
    socklen_t len = sizeof(local);
    if (getsockname(fd, (sockaddr *)&local, &len) != 0) {
      cerr << progmode << ": cannot get local port number" << endl;
      return false;
    }
    port = ntohs(local.sin_port);
    if (verbose)
      cout << progmode << " local fax port = " << port << endl;
  }

  // set socket into non-blocking mode
  int cmd = 1;
  if (__socket_ioctl(fd, FIONBIO, &cmd) != 0) {
     cerr << progmode << ": cannot set socket into non-blocking mode" << endl;
     return false;
  }

  AdaptiveDelay delay;

#if WRITE_PCM_FILES
  int outFile;
  int inFile;
  {
    char fn[1024];

    strcpy(fn, "fax_audio_out_");
    strcat(fn, progmode);
    strcat(fn, ".pcm");
    outFile = _open(fn, _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE);
    if (outFile < 0) {
      cerr << progmode << ": cannot open " << fn << endl;
    }
    else
    {
      cerr << progmode << ": opened " << fn << endl;
    }

    strcpy(fn, "fax_audio_in_");
    strcat(fn, progmode);
    strcat(fn, ".pcm");
    inFile = _open(fn, _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE);
    if (outFile < 0) {
      cerr << progmode << ": cannot open " << fn << endl;
    }
    else
    {
      cerr << progmode << ": opened " << fn << endl;
    }
  }
#endif

  while (!finished) {

    delay.Delay(20);

    // get audio data from terminal and send to socket
    {
      short data[SAMPLES_PER_CHUNK];
      unsigned len = GetPCMData(data, sizeof(data)/2) * 2;
      if (!listen) {
#if WRITE_PCM_FILES      	
        if (outFile >= 0) {
          if (write(outFile, data, len) < len) {
            cerr << progmode << ": cannot write output PCM data to file" << endl;
            outFile = -1;
          }
        }
#endif        

        if (__socket_sendto(fd, data, len, 0, (sockaddr *)&address, sizeof(address)) != len) {
          if ( __socket_getlasterror() == ENOENT)
            cerr << progmode << ": audio write socket not ready" << endl;
          else {
            cerr << progmode << ": write to audio socket failed\n";  __socket_error(cerr) << endl;
            break;
          }
        } else if (verbose && firstAudioWrite) {
          cout << progmode << " first send from audio socket" << endl;
          firstAudioWrite = false;
        }
      }
    }

    // get audio data from the fax socket and send to the terminal
    int len;
    short data[750];
    if (!ReadAudioPacket(fd, data, len, address, listen, verbose))
      break;
    if (len > 0) {
#if WRITE_PCM_FILES    	
      if (inFile >= 0) {
        if (write(inFile, data, len) < len) {
          cerr << progmode << ": cannot write input PCM data to file" << endl;
          outFile = -1;
        }
      }
#endif

      if (!PutPCMData(data, len/2)) {
        cerr << progmode << ": write to terminal failed" << endl;
        break;
      }
    }
  }

  if (verbose)
    cout << progmode << ": fax terminal serve finished" << endl;

  // keep sending silence until the UDP socket closed
  if (SendFiller()) {
    short data[SAMPLES_PER_CHUNK];
    memset(&data, 0, sizeof(data));
    int i = 100;
    while (i-- > 0) {
      if (__socket_sendto(fd, data, sizeof(data), 0, (sockaddr *)&address, sizeof(address)) <= 0) 
        break;
      delay.Delay(20);
    }
  }

  return true;
}

//////////////////////////////////////////////////////////////////////////////////
// 
//  Implement an entity that sends faxes via audio tones from a TIFF file
//

SpanDSP::FaxTerminalSender::FaxTerminalSender(bool verbose)
  : FaxTerminal(true, verbose)
{
}

bool SpanDSP::FaxTerminalSender::Start(const std::string & filename)
{
  FaxTerminal::Start();
  ::t30_set_tx_file(&faxState.t30_state, filename.c_str(), -1, -1);
  ::t30_set_phase_e_handler(&faxState.t30_state, &FaxElement::phase_e_handler, this);
  //::t30_set_tx_data_handler(&faxState.t30_state, &FaxSoftModemSender::tx_data_handler, this);
  return true;
}

void SpanDSP::FaxTerminalSender::PhaseEHandler(int result)
{
  //struct ast_channel *chan;
  char far_ident[21];
  
  //chan = (struct ast_channel *) user_data;
  if (result == T30_ERR_OK) {
    ::t30_get_far_ident(&faxState.t30_state, far_ident);
    if (verbose)
      cout << progmode << ": fax transmission successful" << endl;
  }
  else {
    if (verbose)
      cout << progmode << ": fax transmission was not successful " << t30_completion_code_to_str(result) << endl;
    // fax was not successful
  }
  finished = true;
}

//////////////////////////////////////////////////////////////////////////////////
// 
//  Implement an entity that receives faxes via audio tones into a TIFF file
//

SpanDSP::FaxTerminalReceiver::FaxTerminalReceiver(bool verbose)
  : FaxTerminal(false, verbose)
{
}

bool SpanDSP::FaxTerminalReceiver::Start(const std::string & filename)
{
  FaxTerminal::Start();
  ::t30_set_rx_file(&faxState.t30_state, filename.c_str(), -1);
  ::t30_set_phase_e_handler(&faxState.t30_state, FaxElement::phase_e_handler, this);
  //::t30_set_tx_data_handler(&faxState.t30_state, FaxSoftModemReceiver::tx_data_handler, this);
  return true;
}

void SpanDSP::FaxTerminalReceiver::PhaseEHandler(int result)
{
  if (result == T30_ERR_OK)
  {
    char local_ident[21];
    char far_ident[21];
    t30_stats_t t;
    t30_get_transfer_statistics(&faxState.t30_state, &t);
    t30_get_far_ident(&faxState.t30_state, far_ident);
    t30_get_local_ident(&faxState.t30_state, local_ident);
    if (verbose)
      cout << progmode << ": fax receive completed" << endl;
  }
  else if (verbose)
    cout << progmode << ": fax receive not successful " <<  t30_completion_code_to_str(result) << endl;

  finished = true;
}

bool SpanDSP::FaxTerminalReceiver::SendFiller() const
{
  return true;
}

//////////////////////////////////////////////////////////////////////////////////
// 
//  Implement an entity that sends or receives faxes via T.38
//

SpanDSP::T38Element::T38Element(bool _transmitter, bool _verbose)
  : FaxElement(_transmitter, _verbose)
{ 
  txTimestamp = 0;
  txFd = -1;
  version = T38_VERSION;
}

void SpanDSP::T38Element::SetVersion(unsigned v)
{
  version = v;
}

unsigned SpanDSP::T38Element::GetVersion()
{
  return version;
}

int SpanDSP::T38Element::tx_packet_handler(t38_core_state_t *s, void *user_data, const uint8_t *buf, int len, int count)
{
  T38Element * terminal = (T38Element *)user_data;
  if (terminal == NULL)
    return 0;
  return terminal->TXPacketHandler(buf, len, s->tx_seq_no);
}

int SpanDSP::T38Element::TXPacketHandler(const uint8_t * buf, int len, int sequence)
{ 
  if (txFd >= 0) 
    SendT38Packet(txFd, T38Packet(buf, len, sequence), (const sockaddr *)&txAddr);

  return 0;
}

//
//  Send a T.38 packet on a UDP port
//

bool SpanDSP::T38Element::SendT38Packet(socket_t fd, const T38Packet & pkt, const sockaddr * address)
{
  RTPHeader rtpHeader;
  rtpHeader.flags         = 0x80;
  rtpHeader.payloadMarker = 0x00 | 96;
  rtpHeader.sequence      = htons(pkt.sequence);
  rtpHeader.timestamp     = htonl(txTimestamp);
  rtpHeader.ssrc          = 0;

  txTimestamp += 160;

  struct iovec vectors[2];
  vectors[0].iov_base = &rtpHeader;
  vectors[0].iov_len  = sizeof(rtpHeader);
  vectors[1].iov_base = (void *)&pkt[0];
  vectors[1].iov_len  = pkt.size();

  msghdr msg;
  memset(&msg, 0, sizeof(msg));

  msg.msg_iov     = vectors;
  msg.msg_iovlen  = 2;

  msg.msg_name    = (void *)address;
  msg.msg_namelen = sizeof(sockaddr);

  static int counter = 0;
  if (verbose && (++counter % 25 == 0)) {
    cout << progmode << " " << counter << "t38 writes" << endl;
  }

  if (__socket_sendmsg(fd, &msg, 0) <= 0) {
    cerr << progmode << ": sendmsg failed - " ; __socket_error(cerr) << endl;
    return true;
  }

  if (verbose && firstT38Write) {
    cout << progmode << " first write from t38 socket to port " << htons(((sockaddr_in *)address)->sin_port) << endl;
    firstT38Write = false;
  }

  return true;
}

//
//  Receive a T.38 packet on a UDP port
//

bool SpanDSP::T38Element::ReceiveT38Packet(socket_t fd, SpanDSP::T38Terminal::T38Packet & pkt, sockaddr_in & address, bool & listen)
{
  pkt.resize(1500);

  RTPHeader rtpHeader;

  struct iovec vectors[2];
  vectors[0].iov_base = &rtpHeader;
  vectors[0].iov_len  = sizeof(rtpHeader);
  vectors[1].iov_base = &pkt[0];
  vectors[1].iov_len  = pkt.size();

  msghdr msg;
  memset(&msg, 0, sizeof(msg));

  msg.msg_iov     = vectors;
  msg.msg_iovlen  = 2;
  if (listen) {
    msg.msg_name    = &address;
    msg.msg_namelen = sizeof(address);
  }

  int len = __socket_recvmsg(fd, &msg, 0);

  if (len < 0) {
    int err = __socket_getlasterror();
    if (__socket_iseagain(err)) {
      pkt.resize(0);
      return true;
    }
    cerr << progmode << ": read failed - (" << errno << ") " ; __socket_error(cerr) << endl;
    return false;
  }
  if (len < sizeof(rtpHeader)) {
    if (len > 0)
      cerr << progmode << ": malformed T.38 packet received via UDP" << endl;
    pkt.resize(0);
    return true;
  }

  static int counter = 0;
  if (verbose && (++counter % 25 == 0)) {
    cout << progmode << " " << counter << "t38 reads" << endl;
  }

  pkt.sequence = ntohs(rtpHeader.sequence);
  pkt.resize(len -  sizeof(rtpHeader));

  if (listen) {
    listen = false;
    txFd = fd;
    memcpy(&txAddr, &address, sizeof(txAddr));
    if (verbose) {
      cout << progmode << ": remote address set to ";
      PrintSocketAddr(address, cout);
      cout << endl;
    }
  }

  if (verbose && firstT38Read) {
    cout << progmode << " first read from t38 socket" << endl;
    firstT38Read = false;
  }

  return true;
}

//////////////////////////////////////////////////////////////////////////////////
// 
//  Implement an entity that sends or receives faxes via T.38 to or from TIFF files
//

SpanDSP::T38Terminal::T38Terminal(bool _transmitter, bool _verbose)
  : T38Element(_transmitter, _verbose)
{
}

SpanDSP::T38Terminal::~T38Terminal()
{
}

bool SpanDSP::T38Terminal::PutPCMData(const short *, unsigned)
{ return false; }

unsigned SpanDSP::T38Terminal::GetPCMData(short *, unsigned)
{ return 0; }

bool SpanDSP::T38Terminal::Start(const std::string & filename)
{
  if (::t38_terminal_init(&t38TerminalState, transmitter ? TRUE : FALSE, T38Element::tx_packet_handler, this) == NULL)
    return false;

  if (verbose)
    cout << "starting T.38 terminal with version " << version << endl;

  ::t38_set_t38_version(&t38TerminalState.t38, version);
#ifdef USE_PACING
  ::t38_terminal_set_config(&t38TerminalState, 0);      // enable "pacing"
#endif

  ::span_log_set_level(&t38TerminalState.logging, SPAN_LOG_DEBUG);
#if 0
  ::span_log_set_tag  (&t38TerminalState.logging, "T.38-A");
  ::span_log_set_level(&t38TerminalState.t38.logging, SPAN_LOG_DEBUG | SPAN_LOG_SHOW_TAG);
  ::span_log_set_tag  (&t38TerminalState.t38.logging, "T.38-A");
  ::span_log_set_level(&t38TerminalState.t30_state.logging, SPAN_LOG_DEBUG | SPAN_LOG_SHOW_TAG);
  ::span_log_set_tag  (&t38TerminalState.t30_state.logging, "T.38-A");
#endif

  if (localStationID.length() != 0)
    ::t30_set_local_ident(&t38TerminalState.t30_state, localStationID.c_str());
  else
    ::t30_set_local_ident(&t38TerminalState.t30_state, " ");

  ::t30_set_ecm_capability(&t38TerminalState.t30_state, useECM ? 1 : 0);

  ::t30_set_phase_b_handler(&t38TerminalState.t30_state, FaxElement::phase_b_handler, this);
  ::t30_set_phase_d_handler(&t38TerminalState.t30_state, FaxElement::phase_d_handler, this);
  ::t30_set_phase_e_handler(&t38TerminalState.t30_state, FaxElement::phase_e_handler, this);

  return true;
}

void SpanDSP::T38Terminal::QueuePacket(const T38Packet & pkt)
{
#if SPANDSP_VER3
  ::t38_core_rx_ifp_packet(&t38TerminalState.t38, pkt.sequence, &pkt[0], pkt.size());
#else
  ::t38_core_rx_ifp_packet(&t38TerminalState.t38, &pkt[0], pkt.size(), pkt.sequence);
#endif
}


bool SpanDSP::T38Terminal::Serve(socket_t fd)
{
  sockaddr_in peer;
  socklen_t len = sizeof(peer);
  if (getpeername(fd, (sockaddr *)&peer, &len) != 0) 
    return false;
  return Serve(fd, peer, false);
}

bool SpanDSP::T38Terminal::Serve(socket_t fd, sockaddr_in & address, bool listen)
{
  {
    sockaddr_in local;
    socklen_t len = sizeof(local);
    if (getsockname(fd, (sockaddr *)&local, &len) != 0) {
      cerr << progmode << ": cannot get local port number" << endl;
      return false;
    }
  }

  if (!listen) {
    txFd = fd;
    memcpy(&txAddr, &address, sizeof(txAddr));
  }

  // set socket into non-blocking mode
  int cmd = 1;
  if (__socket_ioctl(fd, FIONBIO, &cmd) != 0) {
    cerr << progmode << ": cannot set socket into non-blocking mode" << endl;
    return false;
  }

  int done = 0;
  int started = 0;

#ifndef USE_PACING
  AdaptiveDelay delay;
#endif

  while (!finished) {

    //if (started)
      done = ::t38_terminal_send_timeout(&t38TerminalState, SAMPLES_PER_CHUNK);

    SpanDSP::T38Terminal::T38Packet pkt;

#ifndef USE_PACING
    delay.Delay(20);

    if (!ReceiveT38Packet(fd, pkt, address, listen)) {
      finished = true;
      break;
    } else if (pkt.size() != 0) 
      QueuePacket(pkt);

    if (finished || done)
      break;
#else
    int ret;
    {
      fd_set rfds;
      FD_ZERO(&rfds);
      FD_SET(fd, &rfds);
      timeval t;

      if (!started) {
        t.tv_sec  = 60;
        t.tv_usec = 0;
      }
      else
      {
        t.tv_sec  = 0;
        t.tv_usec = 30000;
      }

      ret = select(fd+1, &rfds, NULL, NULL, &t);
    }

    // read incoming packets
    if (!started) {
      if (ret <= 0 || !ReceiveT38Packet(fd, pkt, address, listen))
        finished = true;
      started = 1;
    }
    else if (ret != 0) {
      if (ret < 0 || !ReceiveT38Packet(fd, pkt, address, listen)) {
        finished = true;
        break;
      } else if (pkt.size() != 0) 
        QueuePacket(pkt);
    }
#endif
  }

  cout << "finished" << endl;

  return true;
}

//////////////////////////////////////////////////////////////////////////////////
// 
//  Implement an entity that sends via faxes T.38 from TIFF files
//

SpanDSP::T38TerminalSender::T38TerminalSender(bool verbose)
  : T38Terminal(true, verbose)
{
}

bool SpanDSP::T38TerminalSender::Start(const std::string & filename)
{
  if (!T38Terminal::Start(filename))
    return false;

  ::t30_set_tx_file(&t38TerminalState.t30_state, filename.c_str(), -1, -1);

  return true;
}

void SpanDSP::T38TerminalSender::PhaseEHandler(int result)
{
  //struct ast_channel *chan;
  char far_ident[21];
  
  //chan = (struct ast_channel *) user_data;
  if (result == T30_ERR_OK) {
    ::t30_get_far_ident(&t38TerminalState.t30_state, far_ident);
    cout << "fax transmission successful" << endl;
  }
  else {
    cout << "fax transmission was not successful " << t30_completion_code_to_str(result) << endl;
    // fax was not successfuE
  }
  //finished = true;
}

//////////////////////////////////////////////////////////////////////////////////
// 
//  Implement an entity that receives faxes via T.38 to TIFF files
//

SpanDSP::T38TerminalReceiver::T38TerminalReceiver(bool verbose)
  : T38Terminal(false, verbose)
{
}

bool SpanDSP::T38TerminalReceiver::Start(const std::string & filename)
{
  if (!T38Terminal::Start(filename))
    return false;

  ::t30_set_rx_file(&t38TerminalState.t30_state, filename.c_str(), -1);
  return true;
}

void SpanDSP::T38TerminalReceiver::PhaseEHandler(int result)
{
  if (result == T30_ERR_OK)
  {
    char local_ident[21];
    char far_ident[21];
    t30_stats_t t;
    t30_get_transfer_statistics(&t38TerminalState.t30_state, &t);
    t30_get_far_ident(&t38TerminalState.t30_state, far_ident);
    t30_get_local_ident(&t38TerminalState.t30_state, local_ident);
    cout << "fax receive completed" << endl;
  }
  else
  {
    cout << "fax receive not successful " <<  t30_completion_code_to_str(result) << endl;
  }
  //finished = true;
}

//////////////////////////////////////////////////////////////////////////////////
// 
//  Implement an entity that gatewayes beteen T.38 and audio tones
//

SpanDSP::T38Gateway::T38Gateway(bool _verbose)
  : T38Element(false, _verbose)
{
}

SpanDSP::T38Gateway::~T38Gateway()
{
}

bool SpanDSP::T38Gateway::PutPCMData(const short * pcm, unsigned sampleCount)
{
 return (::t38_gateway_rx(&t38GatewayState, (int16_t *)pcm, sampleCount) != 0) ? false : true;
}

unsigned SpanDSP::T38Gateway::GetPCMData(short * pcm, unsigned sampleCount)
{
  unsigned len = ::t38_gateway_tx(&t38GatewayState, pcm, sampleCount);
  if (len < sampleCount) {
    memset(pcm + len, 0, (sampleCount - len)*2);
    len = sampleCount;
  }
  return len;
}

bool SpanDSP::T38Gateway::Start()
{
  if (::t38_gateway_init(&t38GatewayState, tx_packet_handler, this) == NULL)
    return false;

  ::t38_set_t38_version(&t38GatewayState.t38, version);

#if SPANDSP_VER3
  ::t38_gateway_ecm_control(&t38GatewayState, useECM ? 1 : 0);
#endif

  return true;
}

void SpanDSP::T38Gateway::QueuePacket(const T38Packet & pkt)
{
#if SPANDSP_VER3
  ::t38_core_rx_ifp_packet(&t38GatewayState.t38, pkt.sequence, &pkt[0], pkt.size());
#else
  ::t38_core_rx_ifp_packet(&t38GatewayState.t38, &pkt[0], pkt.size(), pkt.sequence);
#endif
}


bool SpanDSP::T38Gateway::Serve(socket_t fax, socket_t t38)
{
  sockaddr_in t38peer;
  socklen_t len = sizeof(t38peer);
  memset(&t38peer, 0, len);
  if (getpeername(t38, (sockaddr *)&t38peer, &len) != 0) 
    return false;

  sockaddr_in faxpeer;
  len = sizeof(faxpeer);
  memset(&faxpeer, 0, len);
  if (getpeername(fax, (sockaddr *)&faxpeer, &len) != 0) 
    return false;

  return Serve(fax, faxpeer, t38, t38peer, false);
}

bool SpanDSP::T38Gateway::Serve(socket_t fax, sockaddr_in & faxAddress, socket_t t38, sockaddr_in & t38Address, bool listenFlag)
{
  bool faxListen, t38Listen;
  faxListen = t38Listen = listenFlag;

  {
    sockaddr_in local;
    socklen_t len = sizeof(local);
    if (getsockname(fax, (sockaddr *)&local, &len) != 0) {
      cerr << progmode << ": cannot get local fax port number" << endl;
      return false;
    }
    if (verbose)
      cout << progmode << " local fax port = " << ntohs(local.sin_port) << endl;
  }

  {
    sockaddr_in local;
    socklen_t len = sizeof(local);
    if (getsockname(t38, (sockaddr *)&local, &len) != 0) {
      cerr << progmode << ": cannot get local t38 port number" << endl;
      return false;
    }
    if (verbose)
      cout << progmode << " local t38 port = " << ntohs(local.sin_port) << endl;
  }

  // set sockets into non-blocking mode
  int cmd = 1;
  if (__socket_ioctl(fax, FIONBIO, &cmd) != 0) {
     cerr << progmode << ": cannot set fax socket into non-blocking mode" << endl;
     return false;
  }

  // set socket into non-blocking mode
  cmd = 1;
  if (__socket_ioctl(t38, FIONBIO, &cmd) != 0) {
     cerr << progmode << ": cannot set t38 socket into non-blocking mode" << endl;
     return false;
  }

  if (!listenFlag) {
    txFd = t38;
    memcpy(&txAddr, &t38Address, sizeof(txAddr));
  }

#if WRITE_PCM_FILES
  int outFile;
  int inFile;
  {
    char fn[1024];

    strcpy(fn, "gw_audio_out_");
    strcat(fn, progmode);
    strcat(fn, ".pcm");
    outFile = _open(fn, _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE);
    if (outFile < 0) {
      cerr << progmode << ": cannot open " << fn << endl;
    }
    else
    {
      cerr << progmode << ": opened " << fn << endl;
    }

    strcpy(fn, "gw_audio_in_");
    strcat(fn, progmode);
    strcat(fn, ".pcm");
    inFile = _open(fn, _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE);
    if (outFile < 0) {
      cerr << progmode << ": cannot open " << fn << endl;
    }
    else
    {
      cerr << progmode << ": opened " << fn << endl;
    }
  }
#endif

  AdaptiveDelay delay;

  while (!finished) {

    delay.Delay(20);

    // get audio data from the fax socket and give to the gateway
    {
      int len;
      short data[750];
      if (ReadAudioPacket(fax, data, len, faxAddress, faxListen, verbose)) {
#if WRITE_PCM_FILES
        if (inFile >= 0) {
          if (write(inFile, data, len) < len) {
            cerr << progmode << ": cannot write input PCM data to file" << endl;
            outFile = -1;
          }
        }
#endif        
        if (len > 0) {
          if (!PutPCMData(data, len/2)) {
            cerr << progmode << ": write to terminal failed" << endl;
            break;
          }
        }
      }
    }

    // get audio data from the gateway and send to the fax socket
    {
      short data[SAMPLES_PER_CHUNK];
      unsigned len = GetPCMData(data, sizeof(data)/2) * 2;
#if _WIN32
      if (!faxListen && (__socket_sendto(fax, data, len, 0, (sockaddr *)&faxAddress, sizeof(faxAddress)) <= 0)) {
#else
      if (__socket_write(fax, data, len) <= 0) {
#endif
        if ( __socket_getlasterror() == ENOENT)
          cerr << progmode << ": fax write socket not ready" << endl;
        else {
          cerr << progmode << ": write to fax socket failed\n";  __socket_error(cerr) << endl;
          break;
        }
      }
      if (verbose && firstAudioWrite) {
        cout << progmode << " first send from audio socket " << len << endl;
        firstAudioWrite = false;
      }
#if WRITE_PCM_FILES
      if (outFile >= 0) {
        if (write(outFile, data, len) < len) {
          cerr << progmode << ": cannot write output PCM data to file" << endl;
          outFile = -1;
        }
      }
#endif      
    }

    if (finished) {
      if (verbose)
        cout << progmode << " finished" << endl;
      break;
    }

    // read any T38 packets received and send to the gateway
    T38Packet pkt;
    do { //for (;;) {
      if (!ReceiveT38Packet(t38, pkt, t38Address, t38Listen)) {
        cerr << progmode << " receive failed" << endl;
        finished = true;
        break;
      }
      if (pkt.size() == 0) 
        break;
      QueuePacket(pkt);
    } while (0);

  }

  // keep sending silence until the UDP socket closed
  {
    short data[SAMPLES_PER_CHUNK];
    memset(&data, 0, sizeof(data));
    int i = 100;
    while (i-- > 0) {
      if (__socket_sendto(fax, data, sizeof(data), 0, (sockaddr *)&faxAddress, sizeof(faxAddress)) <= 0) 
        break;
      delay.Delay(20);
    }
  }

  if (verbose)
    cout << progmode << " t38gw thread ended" << endl;

  return true;
}

//////////////////////////////////////////////////////////////////////////////////
