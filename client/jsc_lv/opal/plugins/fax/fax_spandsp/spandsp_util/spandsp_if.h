/*
 * spandsp_if.h
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
 * $Id: spandsp_if.h,v 1.1 2007/03/29 04:46:38 csoutheren Exp $
 */

//#define SPANDSP_VER3  1

#include <string>
#include <vector>
#include <queue>
#include <ostream>
#include <math.h>

#if defined (_WIN32) || defined (_WIN32_WCE)

  #pragma warning(disable: 4996)

  #ifndef FD_SETSIZE
  #include <winsock2.h>
  #include <Ws2tcpip.h>
  #endif

  typedef SOCKET socket_t;

#else

  typedef int socket_t;
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <netdb.h>

#endif


extern "C" {
#if defined (_WIN32) || defined (_WIN32_WCE)
  #include <tgmath.h>
#else
  #define  INT16_MAX  SHRT_MAX
  #define  INT16_MIN  SHRT_MIN
#endif

#include "spandsp.h"
};

std::ostream & __socket_error(std::ostream & strm);

#define T38_VERSION   1

namespace SpanDSP {

extern char * progmode;
extern char * progname;

//////////////////////////////////////////////////////////////////////////////////
// 
//  Implement an adpative delay that allows approximation of a real timer
//

class AdaptiveDelay
{
  public:
    typedef long long __time_t;

  protected:
    bool first;
    __time_t lastTime;
    __time_t accumulator;

  public:
    static __time_t GetTime();
    AdaptiveDelay();
    void Start();
    void Delay(int delay);
    int Calculate(int delay);
};


//////////////////////////////////////////////////////////////////////////////////
// 
//  Implement an entity that sends or receives faxes
//

class FaxElement
{
  public:
    FaxElement(bool _transmitter, bool _verbose = false);

    virtual bool PutPCMData(const short * pcm, unsigned sampleCount) = 0;
    virtual unsigned GetPCMData(short * pcm, unsigned sampleCount) = 0;

    static void phase_b_handler(t30_state_t *s, void *user_data, int result);
    static void phase_d_handler(t30_state_t *s, void *user_data, int result);
    static void phase_e_handler(t30_state_t *s, void *user_data, int result);

    virtual void PhaseBHandler(int);
    virtual void PhaseDHandler(int);
    virtual void PhaseEHandler(int);

    void SetLocalStationID(const std::string & str);

    void SetECM(bool v = TRUE);
    bool GetECM() const;

  protected:
    bool transmitter;
    bool verbose;
    std::string localStationID;
    bool finished;
    bool useECM;
};

//////////////////////////////////////////////////////////////////////////////////
// 
//  Implement an entity that sends or receives faxes via audio tones to or from TIFF files
//

class FaxTerminal : public FaxElement
{
  public:
    FaxTerminal(bool _transmitter, bool _verbose = false);
    ~FaxTerminal();

    void Start();

    bool PutPCMData(const short * pcm, unsigned sampleCount);
    unsigned GetPCMData(short * pcm, unsigned sampleCount);

    bool Serve(socket_t fd);
    bool Serve(socket_t fd, sockaddr_in & address, bool listen);

    static void tx_data_handler(t30_state_t *s, void *user_data, unsigned code, unsigned len);

    virtual void TXDataHandler(unsigned code, unsigned len);
    void SetLocalStationID(const std::string & str);
    virtual bool SendFiller() const;

  protected:
    ::fax_state_t faxState;
};


//////////////////////////////////////////////////////////////////////////////////
// 
//  Implement an entity that sends faxes via audio tones from a TIFF file
//

class FaxTerminalSender : public FaxTerminal
{
  public:
    FaxTerminalSender(bool verbose = false);

    bool Start(const std::string & filename);
    void PhaseEHandler(int result);
};

//////////////////////////////////////////////////////////////////////////////////
// 
//  Implement an entity that receives faxes via audio tones into a TIFF file
//

class FaxTerminalReceiver : public FaxTerminal
{
  public:
    FaxTerminalReceiver(bool verbose = false);

    bool Start(const std::string & filename);
    void PhaseEHandler(int result);
    virtual bool SendFiller() const;
};

//////////////////////////////////////////////////////////////////////////////////
// 
//  Implement an entity that sends or receives faxes via T.38
//

class T38Element : public FaxElement
{
  public:
    class T38Packet : public std::vector<unsigned char>
    {
      public:
        T38Packet()
        { }

        T38Packet(const uint8_t * buf, int len, int _sequence)
        {
          resize(len);
          memcpy(&(operator[](0)), buf, len);
          sequence = _sequence;
        }

      int sequence;
    };

    typedef std::queue<T38Packet> T38PacketQueue;

    T38Element(bool _transmitter, bool _verbose = false);

    bool DequeueT38Packet(T38Packet & pkt);

    static int tx_packet_handler(t38_core_state_t *s, void *user_data, const uint8_t *buf, int len, int count);

    virtual int TXPacketHandler(const uint8_t * buf, int len, int sequence);

    bool SendT38Packet(socket_t fd, const T38Packet & pkt, const sockaddr * address);
    bool ReceiveT38Packet(socket_t fd, T38Packet & pkt, sockaddr_in & address, bool & listen);

    virtual void SetVersion(unsigned v);
    virtual unsigned GetVersion();

  protected:
    unsigned version;
    unsigned int txTimestamp;
    socket_t txFd;
    sockaddr_in txAddr;
};

//////////////////////////////////////////////////////////////////////////////////
// 
//  Implement an entity that sends or receives faxes via T.38 to or from TIFF files
//

class T38Terminal : public T38Element
{
  public:
    T38Terminal(bool _transmitter, bool _verbose = false);

    virtual ~T38Terminal();

    bool PutPCMData(const short *, unsigned);

    unsigned GetPCMData(short *, unsigned);

    virtual bool Start(const std::string & filename);

    void QueuePacket(const T38Packet & pkt);

    bool Serve(socket_t fax);
    bool Serve(socket_t fd, sockaddr_in & address, bool listen);

    t38_terminal_state_t t38TerminalState;
};



//////////////////////////////////////////////////////////////////////////////////
// 
//  Implement an entity that sends via faxes T.38 from TIFF files
//

class T38TerminalSender : public T38Terminal
{
  public:
    T38TerminalSender(bool verbose = false);
    bool Start(const std::string & filename);
    void PhaseEHandler(int result);
};

//////////////////////////////////////////////////////////////////////////////////
// 
//  Implement an entity that receives faxes via T.38 to TIFF files
//

class T38TerminalReceiver : public T38Terminal
{
  public:
    T38TerminalReceiver(bool verbose = false);
    bool Start(const std::string & filename);
    void PhaseEHandler(int result);
};

//////////////////////////////////////////////////////////////////////////////////
// 
//  Implement an entity that gatewayes beteen T.38 and audio tones
//

class T38Gateway : public T38Element
{
  public:
    T38Gateway(bool _verbose = false);
    ~T38Gateway();
    bool PutPCMData(const short * pcm, unsigned sampleCount);
    unsigned GetPCMData(short * pcm, unsigned sampleCount);
    virtual bool Start();
    void QueuePacket(const T38Packet & pkt);
    bool Serve(socket_t fax, sockaddr_in & faxAddress, socket_t t38, sockaddr_in & t38Address, bool listen);
    bool Serve(socket_t fax, socket_t t38);

  protected:
    t38_gateway_state_t t38GatewayState;
};

};  // namespace SpanDSP

