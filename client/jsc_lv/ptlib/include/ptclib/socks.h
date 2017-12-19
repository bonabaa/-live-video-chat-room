/*
 * socks.h
 *
 * SOCKS protocol
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-2001 Equivalence Pty. Ltd.
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
 * $Revision: 21788 $
 * $Author: rjongbloed $
 * $Date: 2008-12-12 05:42:13 +0000 (Fri, 12 Dec 2008) $
 */

#ifndef PTLIB_SOCKS_H
#define PTLIB_SOCKS_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <ptlib/sockets.h>


/**
Base class for implementing the SOCKS protocol for use with PSocksSocket and
PSocksUDPSocket.
*/

class PSocksProtocol
{
  public:
    PSocksProtocol(WORD port);
    virtual ~PSocksProtocol() { }

  // New functions for class
    enum {
      DefaultServerPort = 1080
    };
    PBoolean SetServer(
      const PString & hostname,
      const char * service = "socks 1080"
    );
    PBoolean SetServer(
      const PString & hostname,
      WORD port
    );

    /** Set the username and password for the SOCKS server authentication. This
       is for the cleartext authentication only, GSSAPI, Kerberos etc is not
       yet supported.
     */
    void SetAuthentication(
      const PString & username,
      const PString & password
    );

  protected:
    PBoolean ConnectSocksServer(PTCPSocket & thisSocket);

    virtual void SetErrorCodes(PChannel::Errors errCode, int osErr) = 0;

    virtual PBoolean SendSocksCommand(PTCPSocket & socket,
                                  BYTE command,
                                  const char * hostname,
                                  PIPSocket::Address addr);
    virtual PBoolean ReceiveSocksResponse(PTCPSocket & socket,
                                      PIPSocket::Address & addr,
                                      WORD & port);


    PString            serverHost;
    WORD               serverPort;
    PString            authenticationUsername;
    PString            authenticationPassword;
    PIPSocket::Address remoteAddress;
    WORD               remotePort;
    PIPSocket::Address localAddress;
    WORD               localPort;
};


/** This is an ancestor class allowing access to a SOCKS servers (version 4 and 5).
 */
class PSocksSocket : public PTCPSocket, public PSocksProtocol
{
  PCLASSINFO(PSocksSocket, PTCPSocket)

  public:
    PSocksSocket(
      WORD port = 0
    );

  // Overrides from class PSocket.
    /** Connect a socket to a remote host on the specified port number. This is
       typically used by the client or initiator of a communications channel.
       This connects to a "listening" socket at the other end of the
       communications channel.

       The port number as defined by the object instance construction or the
       <A>PIPSocket::SetPort()</A> function.

       @return
       PTrue if the channel was successfully connected to the remote host.
     */
    virtual PBoolean Connect(
      const PString & address   ///< Address of remote machine to connect to.
    );
    virtual PBoolean Connect(
      const Address & addr      ///< Address of remote machine to connect to.
    );

    /** Listen on a socket for a remote host on the specified port number. This
       may be used for server based applications. A "connecting" socket begins
       a connection by initiating a connection to this socket. An active socket
       of this type is then used to generate other "accepting" sockets which
       establish a two way communications channel with the "connecting" socket.

       If the <CODE>port</CODE> parameter is zero then the port number as
       defined by the object instance construction or the
       <A>PIPSocket::SetPort()</A> function.

       For the UDP protocol, the <CODE>queueSize</CODE> parameter is ignored.

       @return
       PTrue if the channel was successfully opened.
     */
    virtual PBoolean Listen(
      unsigned queueSize = 5,  ///< Number of pending accepts that may be queued.
      WORD port = 0,           ///< Port number to use for the connection.
      Reusability reuse = AddressIsExclusive ///< Can/Cant listen more than once.
    );

    /** Open a socket to a remote host on the specified port number. This is an
       "accepting" socket. When a "listening" socket has a pending connection
       to make, this will accept a connection made by the "connecting" socket
       created to establish a link.

       The port that the socket uses is the one used in the <A>Listen()</A>
       command of the <CODE>socket</CODE> parameter.

       Note that this function will block until a remote system connects to the
       port number specified in the "listening" socket.

       A further note is that when the version that uses a parameter is used,
       the <CODE>socket</CODE> parameter is automatically closed and its
       operating system handle transferred to the current object. This occurs
       regardless of the return value of the function.

       @return
       PTrue if the channel was successfully opened.
     */
    PBoolean Accept();
    virtual PBoolean Accept(
      PSocket & socket          ///< Listening socket making the connection.
    );


  // Overrides from class PIPSocket.
    /** Get the Internet Protocol address for the local host.

       @return
       PTrue if the IP number was returned.
     */
    virtual PBoolean GetLocalAddress(
      Address & addr    ///< Variable to receive hosts IP address
    );
    virtual PBoolean GetLocalAddress(
      Address & addr,    ///< Variable to receive peer hosts IP address
      WORD & port        ///< Variable to receive peer hosts port number
    );

    /** Get the Internet Protocol address for the peer host the socket is
       connected to.

       @return
       PTrue if the IP number was returned.
     */
    virtual PBoolean GetPeerAddress(
      Address & addr    ///< Variable to receive hosts IP address
    );
    virtual PBoolean GetPeerAddress(
      Address & addr,    ///< Variable to receive peer hosts IP address
      WORD & port        ///< Variable to receive peer hosts port number
    );


  protected:
    virtual void SetErrorCodes(PChannel::Errors errCode, int osErr);
    int TransferHandle(PSocksSocket & destination);

  private:
    virtual PBoolean Connect(WORD localPort, const Address & addr);
};


/** This class allows access to RFC1928 compliant SOCKS server.
 */
class PSocks4Socket : public PSocksSocket
{
  PCLASSINFO(PSocks4Socket, PSocksSocket)

  public:
    PSocks4Socket(
      WORD port = 0
    );
    PSocks4Socket(
      const PString & host,
      WORD port = 0
    );

  // Overrides from class PObject
    /** Create a copy of the class on the heap. The exact semantics of the
       descendent class determine what is required to make a duplicate of the
       instance. Not all classes can even <EM>do</EM> a clone operation.
       
       The main user of the clone function is the <A>PDictionary</A> class as
       it requires copies of the dictionary keys.

       The default behaviour is for this function to assert.

       @return
       pointer to new copy of the class instance.
     */
    virtual PObject * Clone() const;


  protected:
    virtual PBoolean SendSocksCommand(PTCPSocket & socket,
                                  BYTE command,
                                  const char * hostname,
                                  PIPSocket::Address addr);
    virtual PBoolean ReceiveSocksResponse(PTCPSocket & socket,
                                      PIPSocket::Address & addr,
                                      WORD & port);
};


/** This class allows access to RFC1928 compliant SOCKS server.
 */
class PSocks5Socket : public PSocksSocket
{
  PCLASSINFO(PSocks5Socket, PSocksSocket)

  public:
    PSocks5Socket(
      WORD port = 0
    );
    PSocks5Socket(
      const PString & host,
      WORD port = 0
    );

  // Overrides from class PObject
    /** Create a copy of the class on the heap. The exact semantics of the
       descendent class determine what is required to make a duplicate of the
       instance. Not all classes can even <EM>do</EM> a clone operation.
       
       The main user of the clone function is the <A>PDictionary</A> class as
       it requires copies of the dictionary keys.

       The default behaviour is for this function to assert.

       @return
       pointer to new copy of the class instance.
     */
    virtual PObject * Clone() const;
};


/** This class allows access to RFC1928 compliant SOCKS server.
 */
class PSocksUDPSocket : public PUDPSocket, public PSocksProtocol
{
  PCLASSINFO(PSocksUDPSocket, PUDPSocket)

  public:
    PSocksUDPSocket(
      WORD port = 0
    );
    PSocksUDPSocket(
      const PString & host,
      WORD port = 0
    );


  // Overrides from class PObject
    /** Create a copy of the class on the heap. The exact semantics of the
       descendent class determine what is required to make a duplicate of the
       instance. Not all classes can even <EM>do</EM> a clone operation.
       
       The main user of the clone function is the <A>PDictionary</A> class as
       it requires copies of the dictionary keys.

       The default behaviour is for this function to assert.

       @return
       pointer to new copy of the class instance.
     */
    virtual PObject * Clone() const;


  // Overrides from class PSocket.
    /** Connect a socket to a remote host on the specified port number. This is
       typically used by the client or initiator of a communications channel.
       This connects to a "listening" socket at the other end of the
       communications channel.

       The port number as defined by the object instance construction or the
       <A>PIPSocket::SetPort()</A> function.

       @return
       PTrue if the channel was successfully connected to the remote host.
     */
    virtual PBoolean Connect(
      const PString & address   // Address of remote machine to connect to.
    );
    virtual PBoolean Connect(
      const Address & addr      // Address of remote machine to connect to.
    );

    /** Listen on a socket for a remote host on the specified port number. This
       may be used for server based applications. A "connecting" socket begins
       a connection by initiating a connection to this socket. An active socket
       of this type is then used to generate other "accepting" sockets which
       establish a two way communications channel with the "connecting" socket.

       If the <CODE>port</CODE> parameter is zero then the port number as
       defined by the object instance construction or the
       <A>PIPSocket::SetPort()</A> function.

       For the UDP protocol, the <CODE>queueSize</CODE> parameter is ignored.

       @return
       PTrue if the channel was successfully opened.
     */
    virtual PBoolean Listen(
      unsigned queueSize = 5,  // Number of pending accepts that may be queued.
      WORD port = 0,           // Port number to use for the connection.
      Reusability reuse = AddressIsExclusive // Can/Cant listen more than once.
    );

  // Overrides from class PIPSocket.
    /** Get the Internet Protocol address for the local host.

       @return
       PTrue if the IP number was returned.
     */
    virtual PBoolean GetLocalAddress(
      Address & addr    // Variable to receive hosts IP address
    );
    virtual PBoolean GetLocalAddress(
      Address & addr,    // Variable to receive peer hosts IP address
      WORD & port        // Variable to receive peer hosts port number
    );

    /** Get the Internet Protocol address for the peer host the socket is
       connected to.

       @return
       PTrue if the IP number was returned.
     */
    virtual PBoolean GetPeerAddress(
      Address & addr    // Variable to receive hosts IP address
    );
    virtual PBoolean GetPeerAddress(
      Address & addr,    // Variable to receive peer hosts IP address
      WORD & port        // Variable to receive peer hosts port number
    );


  // Overrides from class PIPDatagramSocket.
    /** Read a datagram from a remote computer.
       
       @return
       PTrue if any bytes were sucessfully read.
     */
    virtual PBoolean ReadFrom(
      void * buf,     // Data to be written as URGENT TCP data.
      PINDEX len,     // Number of bytes pointed to by <CODE>buf</CODE>.
      Address & addr, // Address from which the datagram was received.
      WORD & port     // Port from which the datagram was received.
    );

    /** Write a datagram to a remote computer.

       @return
       PTrue if all the bytes were sucessfully written.
     */
    virtual PBoolean WriteTo(
      const void * buf,   // Data to be written as URGENT TCP data.
      PINDEX len,         // Number of bytes pointed to by <CODE>buf</CODE>.
      const Address & addr, // Address to which the datagram is sent.
      WORD port           // Port to which the datagram is sent.
    );


  protected:
    virtual void SetErrorCodes(PChannel::Errors errCode, int osErr);

    PTCPSocket socksControl;
    Address    serverAddress;

  private:
    virtual PBoolean Connect(WORD localPort, const Address & addr);
};


#endif  // PTLIB_SOCKS_H


// End of File ///////////////////////////////////////////////////////////////
