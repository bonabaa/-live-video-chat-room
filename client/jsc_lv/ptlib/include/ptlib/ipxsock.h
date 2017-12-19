/*
 * ipxsock.h
 *
 * IPX protocol socket I/O channel class.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
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
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 20385 $
 * $Author: rjongbloed $
 * $Date: 2008-06-04 10:40:38 +0000 (Wed, 04 Jun 2008) $
 */

#ifndef _PIPXSOCKET
#define _PIPXSOCKET

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib/socket.h>


/**This class describes a type of socket that will communicate using the
   IPX/SPX protocols.
 */
class PIPXSocket : public PSocket
{
  PCLASSINFO(PIPXSocket, PSocket);

  public:
    /**Create a new IPX datagram socket.
     */
    PIPXSocket(
      WORD port = 0       ///< Port number to use for the connection.
    );


  public:
    /** IPX protocol address specification.
     */
    class Address {
      public:
        union {
          struct {
            BYTE b1,b2,b3,b4;
          } b;
          struct {
            WORD w1,s_w2;
          } w;
          DWORD dw;
        } network;
        BYTE node[6];

        /** Create new, invalid, address. */
        Address();
        /** Create copy of existing address */
        Address(const Address & addr /** Address to copy */);
        /** Create address from string representation. */
        Address(const PString & str /** String representation of address */);
        /** Create address from node and net numbers. */
        Address(
          DWORD netNum, ///< IPX network number.
          const char * nodeNum  ///< IPX node number (MAC address)
        );
        /** Create copy of existing address */
        Address & operator=(const Address & addr /** Address to copy */);
        /** Get string representation of IPX address */
        operator PString() const;
        /** Determine if address is valid. Note that this does not mean that
            the host is online.
            @return PTrue is address is valid.
          */
        PBoolean IsValid() const;
      /** Output string representation of IPX address to stream. */
      friend ostream & operator<<(
        ostream & strm, ///< Stream to output to
        Address & addr  ///< Address to output
      ) { return strm << (PString)addr; }
    };

  /**@name Overrides from class PChannel */
  //@{
    /**Get the platform and I/O channel type name of the channel. For an
       IPX/SPX socket this returns the network number, node number of the
       peer the socket is connected to, followed by the socket number it
       is connected to.

       @return
       the name of the channel.
     */
    virtual PString GetName() const;
  //@}


  /**@name Overrides from class PSocket */
  //@{
    /**Connect a socket to a remote host on the port number of the socket.
       This is
       typically used by the client or initiator of a communications channel.
       This connects to a "listening" socket at the other end of the
       communications channel.

       The port number as defined by the object instance construction or the
       #PIPSocket::SetPort()# function.

       @return
       PTrue if the channel was successfully connected to the remote host.
     */
    virtual PBoolean Connect(
      const PString & address   ///< Address of remote machine to connect to.
    );
    /**Connect a socket to a remote host on the port number of the socket.
       This is
       typically used by the client or initiator of a communications channel.
       This connects to a "listening" socket at the other end of the
       communications channel.

       The port number as defined by the object instance construction or the
       #PIPSocket::SetPort()# function.

       @return
       PTrue if the channel was successfully connected to the remote host.
     */
    virtual PBoolean Connect(
      const Address & address   ///< Address of remote machine to connect to.
    );

    /**Listen on a socket for a remote host on the specified port number. This
       may be used for server based applications. A "connecting" socket begins
       a connection by initiating a connection to this socket. An active socket
       of this type is then used to generate other "accepting" sockets which
       establish a two way communications channel with the "connecting" socket.

       If the #port# parameter is zero then the port number as
       defined by the object instance construction or the
       #PIPSocket::SetPort()# function.

       For the UDP protocol, the #queueSize# parameter is ignored.

       @return
       PTrue if the channel was successfully opened.
     */
    virtual PBoolean Listen(
      unsigned queueSize = 5,  ///< Number of pending accepts that may be queued.
      WORD port = 0,           ///< Port number to use for the connection.
      Reusability reuse = AddressIsExclusive ///< Can/Cant listen more than once.
    );
  //@}

  /**@name Address and name space look up functions */
  //@{
    /**Get the host name for the host specified server.

       @return
       Name of the host or IPX number of host.
     */
    static PString GetHostName(
      const Address & addr    ///< Hosts IP address to get name for
    );

    /**Get the IPX address for the specified host.

       @return
       PTrue if the IPX number was returned.
     */
    static PBoolean GetHostAddress(
      Address & addr    ///< Variable to receive this hosts IP address
    );

    /**Get the IPX address for the specified host.

       @return
       PTrue if the IPX number was returned.
     */
    static PBoolean GetHostAddress(
      const PString & hostname,
      /** Name of host to get address for. This may be either a server name or
         an IPX number in "colon" format.
       */
      Address & addr    ///< Variable to receive hosts IPX address
    );

    /**Get the IPX/SPX address for the local host.

       @return
       PTrue if the IPX number was returned.
     */
    PBoolean GetLocalAddress(
      Address & addr    ///< Variable to receive hosts IPX address
    );

    /**Get the IPX/SPX address for the local host.

       @return
       PTrue if the IPX number was returned.
     */
    PBoolean GetLocalAddress(
      Address & addr,    ///< Variable to receive peer hosts IPX address
      WORD & port        ///< Variable to receive peer hosts port number
    );

    /**Get the IPX/SPX address for the peer host the socket is
       connected to.

       @return
       PTrue if the IPX number was returned.
     */
    PBoolean GetPeerAddress(
      Address & addr    ///< Variable to receive hosts IPX address
    );

    /**Get the IPX/SPX address for the peer host the socket is
       connected to.

       @return
       PTrue if the IPX number was returned.
     */
    PBoolean GetPeerAddress(
      Address & addr,    ///< Variable to receive peer hosts IPX address
      WORD & port        ///< Variable to receive peer hosts port number
    );
  //@}

  /**@name I/O functions */
  //@{
    /**Sets the packet type for datagrams sent by this socket.

       @return
       PTrue if the type was successfully set.
     */
    PBoolean SetPacketType(
      int type    ///< IPX packet type for this socket.
    );

    /**Gets the packet type for datagrams sent by this socket.

       @return
       type of packets or -1 if error.
     */
    int GetPacketType();


    /**Read a datagram from a remote computer.
       
       @return
       PTrue if all the bytes were sucessfully written.
     */
    virtual PBoolean ReadFrom(
      void * buf,     ///< Data to be written as URGENT TCP data.
      PINDEX len,     ///< Number of bytes pointed to by #buf#.
      Address & addr, ///< Address from which the datagram was received.
      WORD & port     ///< Port from which the datagram was received.
    );

    /**Write a datagram to a remote computer.

       @return
       PTrue if all the bytes were sucessfully written.
     */
    virtual PBoolean WriteTo(
      const void * buf,   ///< Data to be written as URGENT TCP data.
      PINDEX len,         ///< Number of bytes pointed to by #buf#.
      const Address & addr, ///< Address to which the datagram is sent.
      WORD port           ///< Port to which the datagram is sent.
    );
  //@}


  protected:
    virtual PBoolean OpenSocket();
    virtual const char * GetProtocolName() const;


// Include platform dependent part of class
#ifdef _WIN32
#include "msos/ptlib/ipxsock.h"
#else
#include "unix/ptlib/ipxsock.h"
#endif
};

#endif

// End Of File ///////////////////////////////////////////////////////////////
