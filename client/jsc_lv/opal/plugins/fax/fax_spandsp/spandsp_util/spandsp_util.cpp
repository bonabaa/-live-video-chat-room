/*
 * spandsp_util.cpp
 *
 * A program wrapper for the SpanDSP library
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
 * $Id: spandsp_util.cpp,v 1.2 2007/05/10 05:28:49 csoutheren Exp $
 */

#include <iostream>
using namespace std;

#include "spandsp_if.h"
#include <tiff.h>
#include <fcntl.h>

//////////////////////////////////////////////////////////////////////////////////
// 
//  create a UDP socket on the local host
//

static socket_t CreateLocalUDPPort(unsigned short port)
{
  socket_t fd = socket(PF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    cerr << SpanDSP::progname << ": cannot create socket - " ; __socket_error(cerr) << endl;
    exit (-1);
  }  

  sockaddr_in sockAddr;
  memset(&sockAddr, 0, sizeof(sockAddr));
  sockAddr.sin_family        = AF_INET;
  sockAddr.sin_addr.s_addr   = INADDR_ANY;
  sockAddr.sin_port = htons(port);

  if (bind(fd, (sockaddr *)&sockAddr, sizeof(sockAddr)) != 0) {
    cerr << SpanDSP::progname << ": cannot bind socket on " << port << " - " ; __socket_error(cerr) << endl;
    exit(-1);
  }

  return fd;
}

/////////////////////////////////////////////////////////////////

int Usage()
{
  return -1;
}

//////////////////////////////////////////////////////////////////////////////////
// 
//  parse a string containing a hostname/IP address and port number
//

static bool ParseAddressAndPort(const char * _str, sockaddr_in & addr)
{
  char * str = strdup(_str);
  char * pos = strchr(str, ':');

  bool stat = false;
  if (pos != NULL) {
    *pos = '\0';
    hostent * host = gethostbyname(str);
    if (host != NULL) {
      memcpy(&addr.sin_addr, host->h_addr, host->h_length);
      stat = true;
    } else {
#if _WIN32
      addr.sin_addr.S_un.S_addr = inet_addr(str);
      if (addr.sin_addr.S_un.S_addr != INADDR_NONE)
#else
      addr.sin_addr.s_addr = inet_addr(str);
      if (addr.sin_addr.s_addr != INADDR_NONE)

#endif
        stat = true;
      else
        cerr << SpanDSP::progname << ": unknown hostname '" << str << "'" << " " ; __socket_error(cerr) << endl;
    }
  }

  if (stat)
    addr.sin_port = htons(atoi(pos+1));

  free(str);
  return stat;
}

void tiffWarningHandler(const char* module, const char* fmt, va_list ap)
{
}

//////////////////////////////////////////////////////////////////////////////////
// 
//  main function argument parsing etc
//

int main(int argc, char* argv[])
{
  SpanDSP::progname = strrchr(argv[0], '/');
  if (SpanDSP::progname != NULL)
    ++SpanDSP::progname;
  else
    SpanDSP::progname = argv[0];

#if _WIN32
  _set_fmode(_O_BINARY);
  {
    WORD wVersionRequested = MAKEWORD( 2, 2 );
    WSADATA wsaData;
    if (WSAStartup(wVersionRequested, &wsaData) != 0) {
      cerr << SpanDSP::progname << ": cannot start winsock" << endl;
      return -1;
    }
  }
#endif

  TIFFSetWarningHandler(&tiffWarningHandler);

  int argIndex = 1;
  unsigned short faxPort = 0;
  unsigned short t38Port = 0;

  sockaddr_in faxAddress, t38Address;

  memset(&faxAddress, 0, sizeof(faxAddress));
  faxAddress.sin_family = AF_INET;

  memset(&t38Address, 0, sizeof(t38Address));
  t38Address.sin_family = AF_INET;

  bool listen = false;
  string mode("none");

  char * stationID = NULL;
  char * filename = NULL;

  bool verbose = false;

  unsigned version = T38_VERSION;

  // parse options
  while (argIndex < argc) {

    if (argv[argIndex][0] != '-')
      break;

    if (strlen(argv[argIndex]) == 1) {
      argIndex++;
      continue;
    }

    switch (argv[argIndex][1]) {
      case 'v':
        verbose = true;
        ++argIndex;
        break;

      case 'l':
        listen = true;
        ++argIndex;
        break;

      case 'V':
        if (++argIndex >= argc) {
          cerr << SpanDSP::progname << ": -V option requires argument" << endl;
          return Usage();
        }
        version = atoi(argv[argIndex]);
        argIndex++;
        break;

      case 'f':
        if (++argIndex >= argc) {
          cerr << SpanDSP::progname << ": -f option requires argument" << endl;
          return Usage();
        }
        if (!ParseAddressAndPort(argv[argIndex], faxAddress)) {
          cerr << SpanDSP::progname << ": cannot parse fax address '" << argv[argIndex] << "'" << endl;
          return -1;
        }
        argIndex++;
        break;

      case 'F':
        if (++argIndex >= argc) {
          cerr << SpanDSP::progname << ": -F option requires argument" << endl;
          return Usage();
        }
        faxPort = atoi(argv[argIndex++]);
        break;

      case 't':
        if (++argIndex >= argc) {
          cerr << SpanDSP::progname << ": -t option requires argument" << endl;
          return Usage();
        }
        if (!ParseAddressAndPort(argv[argIndex], t38Address)) {
          cerr << SpanDSP::progname << ": cannot parse t38 address '" << argv[argIndex] << "'" << endl;
          return -1;
        }
        argIndex++;
        break;

      case 'T':
        if (++argIndex >= argc) {
          cerr << SpanDSP::progname << ": -T option requires argument" << endl;
          return Usage();
        }
        t38Port = atoi(argv[argIndex++]);
        break;

      case 'm':
        if (++argIndex >= argc) {
          cerr << SpanDSP::progname << ": -m option requires argument" << endl;
          return Usage();
        }
        mode = SpanDSP::progmode = argv[argIndex++];
        if ( 
            (mode != "fax_to_tiff") && 
            (mode != "tiff_to_fax") &&

            (mode != "t38_to_tiff") && 
            (mode != "tiff_to_t38") &&

            (mode != "fax_to_t38") &&
            (mode != "t38_to_fax") 

          ) {
          cerr << SpanDSP::progname << ": unknown mode '" << mode << "'" << endl;
          return -1;
        }
        break;

      case 'n':
        if (++argIndex >= argc) {
          cerr << SpanDSP::progname << ": -n option requires argument" << endl;
          return Usage();
        }
        filename = argv[argIndex++];
        break;

      case 's':
        if (++argIndex >= argc) {
          cerr << SpanDSP::progname << ": -s option requires argument" << endl;
          return Usage();
        }
        stationID = argv[argIndex++];
        break;

      default:
        cerr << SpanDSP::progname << ": unknown option " << argv[argIndex] << endl;
        return Usage();
    }
  }

  // receive a fax via T.38 stream into a TIFF file
  if (mode == "t38_to_tiff") {
    if (!listen && ((t38Address.sin_port == 0) || (t38Address.sin_addr.s_addr == 0))) {
      cerr << SpanDSP::progname << ": must set remote address using -r option or specify -l" << endl;
      return -1;
    }
    if (filename == NULL) {
      cerr << SpanDSP::progname << ": must set filename using -n option" << endl;
      return -1;
    }
    socket_t fd = CreateLocalUDPPort(t38Port);
    SpanDSP::T38TerminalReceiver terminal(verbose);
    terminal.SetVersion(version);
    if (stationID != NULL)
      terminal.SetLocalStationID(stationID);
    terminal.Start(filename);
    terminal.Serve(fd, t38Address, listen);
  }

  // send a fax via T.38 from a TIFF file
  else if (mode == "tiff_to_t38") {
    if (!listen && ((t38Address.sin_port == 0) || (t38Address.sin_addr.s_addr == 0))) {
      cerr << SpanDSP::progname << ": must set remote address using -t option or specify -l" << endl;
      return -1;
    }
    if (filename == NULL) {
      cerr << SpanDSP::progname << ": must set filename using -n option" << endl;
      return -1;
    }
    socket_t fd = CreateLocalUDPPort(t38Port);
    SpanDSP::T38TerminalSender terminal(verbose);
    terminal.SetVersion(version);
    if (stationID != NULL)
      terminal.SetLocalStationID(stationID);
    terminal.Start(filename);
    terminal.Serve(fd, t38Address, listen);
  }

  // receive a fax via audio tones into a TIFF file
  else if (mode == "fax_to_tiff") {
    if (!listen && ((faxAddress.sin_port == 0) || (faxAddress.sin_addr.s_addr == 0))) {
      cerr << SpanDSP::progname << ": must set remote address using -f option or specify -l" << endl;
      return -1;
    }
    if (filename == NULL) {
      cerr << SpanDSP::progname << ": must set filename using -n option" << endl;
      return -1;
    }
    socket_t fd = CreateLocalUDPPort(faxPort);
    SpanDSP::FaxTerminalReceiver terminal(verbose);
    if (stationID != NULL)
      terminal.SetLocalStationID(stationID);
    terminal.Start(filename);
    terminal.Serve(fd, faxAddress, listen);
  }

  // send a fax via audio tones from a TIFF file
  else if (mode == "tiff_to_fax") {
    if (!listen && ((faxAddress.sin_port == 0) || (faxAddress.sin_addr.s_addr == 0))) {
      cerr << SpanDSP::progname << ": must set remote address using -f option or specify -l" << endl;
      return -1;
    }
    if (filename == NULL) {
      cerr << SpanDSP::progname << ": must set filename using -n option" << endl;
      return -1;
    }
    socket_t fd = CreateLocalUDPPort(faxPort);
    SpanDSP::FaxTerminalSender terminal(verbose);
    if (stationID != NULL)
      terminal.SetLocalStationID(stationID);
    terminal.Start(filename);
    terminal.Serve(fd, faxAddress, listen);
  }

  else if ((mode == "fax_to_t38") || ("t38_to_fax")) {
    if (!listen && ((faxAddress.sin_port == 0) || (faxAddress.sin_addr.s_addr == 0) ||
                    (t38Address.sin_port == 0) || (t38Address.sin_addr.s_addr == 0))) {
      cerr << SpanDSP::progname << ": must set remote address using -f and -t options or specify -l" << endl;
      return -1;
    }
    socket_t fax = CreateLocalUDPPort(faxPort);
    socket_t t38 = CreateLocalUDPPort(t38Port);

    SpanDSP::T38Gateway gateway(verbose);
    gateway.SetVersion(version);
    gateway.Start();
    gateway.Serve(fax, faxAddress, t38, t38Address, listen);
  }

  else {
    cerr << SpanDSP::progname << ": mode '" << mode << "' not supported - must be one of:\n"
         << "fax_to_t38, fax_to_file, file_to_fax, t38_to_file, file_to_t38\n";
    return -1;
  }

  return 0;
}

// End of File ///////////////////////////////////////////////////////////////
