/*
 * transport.h
 *
 * Transport declarations
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
 * Portions Copyright (C) 2006 by Post Increment
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): Post Increment
 *     Portions of this code were written with the assistance of funding from
 *     US Joint Forces Command Joint Concept Development & Experimentation (J9)
 *     http://www.jfcom.mil/about/abt_j9.htm
 *
 * $Revision: 22198 $
 * $Author: rjongbloed $
 * $Date: 2009-03-12 01:21:09 +0000 (Thu, 12 Mar 2009) $
 */

#ifndef OPAL_OPAL_TRANSPORT_H
#define OPAL_OPAL_TRANSPORT_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <ptlib/sockets.h>
#include <ptclib/psockbun.h>


class OpalManager;
class OpalEndPoint;
class OpalListener;
class OpalTransport;
class OpalInternalTransport;


////////////////////////////////////////////////////////////////

class OpalTransportAddress : public PCaselessString
{
  PCLASSINFO(OpalTransportAddress, PCaselessString);
  public:
  /**@name Construction */
  //@{
    OpalTransportAddress();
    OpalTransportAddress(
      const char * address,      ///<  Address string to parse
      WORD port = 0,             ///<  Default port number
      const char * proto = NULL  ///<  Default is "tcp"
    );
    OpalTransportAddress(
      const PString & address,   ///<  Address string to parse
      WORD port = 0,             ///<  Default port number
      const char * proto = NULL  ///<  Default is "tcp"
    );
    OpalTransportAddress(
      const PIPSocket::Address & ip,
      WORD port,
      const char * proto = NULL ///<  Default is "tcp"
    );
  //@}

  /**@name Operations */
  //@{
    /**Determine if the two transport addresses are equivalent.
       The second parameter indicates if wildcards are allowed. A wildcard
       IP address is one for which isAny() returns true, a wildcard port
       is the value 65535.
      */
    PBoolean IsEquivalent(
      const OpalTransportAddress & address,
      bool wildcards = false   ///< Allow wildcards
    ) const;

    /**Determine if the two transport addresses are compatible.
      */
    PBoolean IsCompatible(
      const OpalTransportAddress & address
    ) const;

    /**Return the underlying protocol for the transport address.
      */
    PCaselessString GetProto() const { return Left(Find('$')); }

    /**Extract the ip address from transport address.
       Returns PFalse, if the address is not an IP transport address.
      */
    PBoolean GetIpAddress(PIPSocket::Address & ip) const;

    /**Extract the ip address and port number from transport address.
       Returns PFalse, if the address is not an IP transport address.
      */
    PBoolean GetIpAndPort(PIPSocket::Address & ip, WORD & port) const;
    PBoolean GetIpAndPort(PIPSocketAddressAndPort & ipPort) const;

    /**Translate the transport address to a more human readable form.
       Returns the hostname if using IP.
      */
    virtual PString GetHostName() const;

    enum BindOptions {
      NoBinding,
      HostOnly,
      FullTSAP,
      Streamed,
      Datagram,
      RouteInterface,
      NumBindOptions
    };

    /**Create a listener based on this transport address.
       The BindOptions parameter indicates how the listener is to be created.
       Note that some transport types may not use this parameter.

       With FullTSAP the the full address is used for any local binding, for
       example, an address of "tcp$10.0.0.1:1720" would create a TCP listening
       socket that would be bound to the specific interface 10.0.0.1 and
       listens on port 1720. Note that the address "tcp$*:1720" can be used
       to bind to INADDR_ANY, and a port number of zero indicates allocate a
       new random port number.

       With HostOnly it would be equivalent to translating the above example
       to "tcp$10.0.0.1:0" before using it.

       Using Streamed or Datagram is similar to HostOnly as only the host part
       of the address is used, but instead of using the protocol type specifed
       by the address it guarentees the specifeid type. In the above example
       Streamed would be identical to HostOnly and Datagram would translate
       the address to udp$10.0.0.1:0 before using it.

       With NoBinding then a compatible listener is created and no local
       binding is made. This is equivalent to translating the address to
       "tcp$*:0" so that only the overall protocol type is used.

       With RouteInterface, the address is considered a remote address and the
       created transport is bound only to the address associated with the
       interface that would be used to get to that address.

       Also note that if the address has a trailing '+' character then the
       socket will be bound using the REUSEADDR option, where relevant.
      */
    OpalListener * CreateListener(
      OpalEndPoint & endpoint,   ///<  Endpoint object for transport creation.
      BindOptions option         ///<  Options for how to create listener
    ) const;

    /**Create a transport suitable for this address type.
       The BindOptions parameter indicates how the transport is to be created.
       Note that some transport types may not use this parameter.

       With FullTSAP the the full address is used for any local binding, for
       example, an address of "tcp$10.0.0.1:1720" would create a TCP transport
       socket that would be bound to the specific interface 10.0.0.1 and
       port 1720. Note that the address "tcp$*:1720" can be used to bind to
       INADDR_ANY, and a port number of zero indicates allocate a new random
       port number.

       With HostOnly it would be equivalent to translating the above example
       to "tcp$10.0.0.1:0" before using it.

       Using Streamed or Datagram is similar to HostOnly as only the host part
       of the address is used, but instead of using the protocol type specifed
       by the address it guarentees the specifeid type. In the above example
       Streamed would be identical to HostOnly and Datagram would translate
       the address to udp$10.0.0.1:0 before using it.

       With NoBinding then a compatible transport is created and no local
       binding is made. This is equivalent to translating the address to
       "tcp$*:0" so that only the overall protocol type is used.

       Also note that if the address has a trailing '+' character then the
       socket will be bound using the REUSEADDR option.
      */
    virtual OpalTransport * CreateTransport(
      OpalEndPoint & endpoint,   ///<  Endpoint object for transport creation.
      BindOptions option = HostOnly ///<  Options for how to create transport
    ) const;
  //@}


  protected:
    void SetInternalTransport(
      WORD port,             ///<  Default port number
      const char * proto     ///<  Default is "tcp"
    );

    OpalInternalTransport * transport;
};


PDECLARE_ARRAY(OpalTransportAddressArray, OpalTransportAddress)
  public:
    OpalTransportAddressArray(
      const OpalTransportAddress & address
    ) { AppendAddress(address); }
    OpalTransportAddressArray(
      const PStringArray & array
    ) { AppendStringCollection(array); }
    OpalTransportAddressArray(
      const PStringList & list
    ) { AppendStringCollection(list); }
    OpalTransportAddressArray(
      const PSortedStringList & list
    ) { AppendStringCollection(list); }

    void AppendString(
      const char * address
    );
    void AppendString(
      const PString & address
    );
    void AppendAddress(
      const OpalTransportAddress & address
    );

  protected:
    void AppendStringCollection(
      const PCollection & coll
    );
};




///////////////////////////////////////////////////////

/**This class describes a "listener" on a transport protocol.
   A "listener" is an object that listens for incoming connections on the
   particular transport. It is executed as a separate thread.

   The Main() function is used to handle incoming connections and dispatch
   them in new threads based on the actual OpalTransport class. This is
   defined in the descendent class that knows what the low level transport
   is, eg OpalListenerIP for the TCP/IP protocol.

   An application may create a descendent off this class and override
   functions as required for operating the channel protocol.
 */
class OpalListener : public PObject
{
    PCLASSINFO(OpalListener, PObject);
  public:
  /**@name Construction */
  //@{
    /**Create a new listener.
     */
    OpalListener(
      OpalEndPoint & endpoint   ///<  Endpoint listener is used for
    );
  //@}

  /**@name Overrides from PObject */
  //@{
    /**Print the description of the listener to the stream.
      */
    void PrintOn(
      ostream & strm
    ) const;
  //@}

  /**@name Operations */
  //@{
    enum ThreadMode {
      SpawnNewThreadMode,
      HandOffThreadMode,
      SingleThreadMode
    };

    /** Open the listener.
        A thread is spawned to listen for incoming connections. The notifier
        function acceptHandler is called when a new connection is created. The
        INT parameter to the acceptHandler is a pointer to the new
        OpalTransport instance created by the listener. If this is NULL then
        it indicates an error occurred during the accept.

        If singleThread is PFalse the acceptHandler function is called in the
        context of a new thread and the continues to listen for more
        connections. If PTrue, then the acceptHandler function is called from
        within the listener threads context and no more connections are
        created. That is only a single connection is ever created by this
        listener.
      */
    virtual PBoolean Open(
      const PNotifier & acceptHandler,  ///<  Handler function for new connections
      ThreadMode mode = SpawnNewThreadMode ///<  How handler function is called thread wise
    ) = 0;

    /** Indicate if the listener is open.
      */
    virtual PBoolean IsOpen() = 0;

    /**Stop the listener thread and no longer accept incoming connections.
     */
    virtual void Close() = 0;

    /**Accept a new incoming transport.
      */
    virtual OpalTransport * Accept(
      const PTimeInterval & timeout  ///<  Time to wait for incoming connection
    ) = 0;

    /**Create a transport compatible with this listener.
     */
    virtual OpalTransport * CreateTransport(
      const OpalTransportAddress & localAddress,
      const OpalTransportAddress & remoteAddress
    ) const = 0;

    /**Get the local transport address on which this listener may be accessed.
       If remoteAddress is present and is an address that requires NAT for
       connectivity, then the returned address is adjusted to return the
       external address and port.
      */
    virtual OpalTransportAddress GetLocalAddress(
      const OpalTransportAddress & remoteAddress = OpalTransportAddress()
    ) const = 0;

    /**Close channel and wait for associated thread to terminate.
      */
    void CloseWait();

    /**Close channel and wait for associated thread to terminate.
       For backward compatibility with OpenH323, now deprecated.
      */
    void CleanUpOnTermination() { CloseWait(); }
  //@}


  protected:
    /**Handle incoming connections and dispatch them in new threads based on
       the OpalTransport class. This is defined in the descendent class that
       knows what the low level transport is, eg OpalListenerTCP for the
       TCP/IP protocol.

       Note this function does not return until the Close() function is called
       or there is some other error.
     */
    PDECLARE_NOTIFIER(PThread, OpalListener, ListenForConnections);
    PBoolean StartThread(
      const PNotifier & acceptHandler,  ///<  Handler function for new connections
      ThreadMode mode                   ///<  How handler function is called thread wise
    );

    OpalEndPoint & endpoint;
    PThread      * thread;
    PNotifier      acceptHandler;
    ThreadMode     threadMode;
};


PLIST(OpalListenerList, OpalListener);


class OpalListenerIP : public OpalListener
{
  PCLASSINFO(OpalListenerIP, OpalListener);
  public:
  /**@name Construction */
  //@{
    /**Create a new IP listener.
     */
    OpalListenerIP(
      OpalEndPoint & endpoint,                 ///<  Endpoint listener is used for
      PIPSocket::Address binding = PIPSocket::GetDefaultIpAny(), ///<  Local interface to listen on
      WORD port = 0,                           ///<  TCP port to listen for connections
      PBoolean exclusive = PTrue
    );
    OpalListenerIP(
      OpalEndPoint & endpoint,                  ///<  Endpoint listener is used for
      const OpalTransportAddress & binding,     ///<  Local interface to listen on
      OpalTransportAddress::BindOptions option  ///< OPtions for binding
    );
  //@}

  /**@name Overrides from OpalListener */
  //@{
    /**Get the local transport address on which this listener may be accessed.
       If remoteAddress is present and is an address that requires NAT for
       connectivity, then the returned address is adjusted to return the
       external address and port.
      */
    virtual OpalTransportAddress GetLocalAddress(
      const OpalTransportAddress & remoteAddress = OpalTransportAddress()
    ) const;
  //@}

  /**@name Operations */
  //@{
    WORD GetListenerPort() const { return listenerPort; }

    virtual const char * GetProtoPrefix() const = 0;
  //@}


  protected:
    PIPSocket::Address localAddress;
    WORD               listenerPort;
    PBoolean               exclusiveListener;
};


class OpalListenerTCP : public OpalListenerIP
{
  PCLASSINFO(OpalListenerTCP, OpalListenerIP);
  public:
  /**@name Construction */
  //@{
    /**Create a new listener.
     */
    OpalListenerTCP(
      OpalEndPoint & endpoint,                 ///<  Endpoint listener is used for
      PIPSocket::Address binding = PIPSocket::GetDefaultIpAny(), ///<  Local interface to listen on
      WORD port = 0,                           ///<  TCP port to listen for connections
      PBoolean exclusive = PTrue
    );
    OpalListenerTCP(
      OpalEndPoint & endpoint,                  ///<  Endpoint listener is used for
      const OpalTransportAddress & binding,     ///<  Local interface to listen on
      OpalTransportAddress::BindOptions option  ///< OPtions for binding
    );

    /** Destroy the listener thread.
      */
    ~OpalListenerTCP();
  //@}

  /**@name Overrides from OpalListener */
  //@{
    /** Open the listener.
        Listen for an incoming connection and create a OpalTransport of the
        appropriate subclass.

        If notifier function acceptHandler is non-NULL a thread is spawned to
        listen for incoming connections. The acceptHandler is called when a
        new connection is created. The INT parameter to the acceptHandler is
        a pointer to the new OpalTransport instance created by the listener.

        If singleThread is PFalse the acceptHandler function is called in the
        context of a new thread and the continues to listen for more
        connections. If PTrue, then the acceptHandler function is called from
        within the listener threads context and no more connections are
        created. That is only a single connection is ever created by this
        listener.

        If acceptHandler is NULL, then no thread is started and it is assumed
        that the caller is responsible for calling Accept() and waiting for
        the new connection.
      */
    virtual PBoolean Open(
      const PNotifier & acceptHandler,  ///<  Handler function for new connections
      ThreadMode mode = SpawnNewThreadMode ///<  How handler function is called thread wise
    );

    /** Indicate if the listener is open.
      */
    virtual PBoolean IsOpen();

    /**Stop the listener thread and no longer accept incoming connections.
     */
    virtual void Close();

    /**Accept a new incoming transport.
      */
    virtual OpalTransport * Accept(
      const PTimeInterval & timeout  ///<  Time to wait for incoming connection
    );

    /**Create a transport compatible with this listener.
     */
    virtual OpalTransport * CreateTransport(
      const OpalTransportAddress & localAddress,
      const OpalTransportAddress & remoteAddress
    ) const;
  //@}


  protected:
    virtual const char * GetProtoPrefix() const;

    PTCPSocket listener;
};


class OpalListenerUDP : public OpalListenerIP
{
  PCLASSINFO(OpalListenerUDP, OpalListenerIP);
  public:
  /**@name Construction */
  //@{
    /**Create a new listener.
     */
    OpalListenerUDP(
      OpalEndPoint & endpoint,  ///<  Endpoint listener is used for
      PIPSocket::Address binding = PIPSocket::GetDefaultIpAny(), ///<  Local interface to listen on
      WORD port = 0,            ///<  TCP port to listen for connections
      PBoolean exclusive = PTrue
    );
    OpalListenerUDP(
      OpalEndPoint & endpoint,                  ///<  Endpoint listener is used for
      const OpalTransportAddress & binding,     ///<  Local interface to listen on
      OpalTransportAddress::BindOptions option  ///< OPtions for binding
    );

    /** Destroy the listener thread.
      */
    ~OpalListenerUDP();
  //@}

  /**@name Overrides from OpalListener */
  //@{
    /** Open the listener.
        Listen for an incoming connection and create a OpalTransport of the
        appropriate subclass.

        If notifier function acceptHandler is non-NULL a thread is spawned to
        listen for incoming connections. The acceptHandler is called when a
        new connection is created. The INT parameter to the acceptHandler is
        a pointer to the new OpalTransport instance created by the listener.

        If singleThread is PFalse the acceptHandler function is called in the
        context of a new thread and the continues to listen for more
        connections. If PTrue, then the acceptHandler function is called from
        within the listener threads context and no more connections are
        created. That is only a single connection is ever created by this
        listener.

        If acceptHandler is NULL, then no thread is started and it is assumed
        that the caller is responsible for calling Accept() and waiting for
        the new connection.
      */
    virtual PBoolean Open(
      const PNotifier & acceptHandler,  ///<  Handler function for new connections
      ThreadMode mode = SpawnNewThreadMode ///<  How handler function is called thread wise
    );

    /** Indicate if the listener is open.
      */
    virtual PBoolean IsOpen();

    /**Stop the listener thread and no longer accept incoming connections.
     */
    virtual void Close();

    /**Accept a new incoming transport.
      */
    virtual OpalTransport * Accept(
      const PTimeInterval & timeout  ///<  Time to wait for incoming connection
    );

    /**Create a transport compatible with this listener.
     */
    virtual OpalTransport * CreateTransport(
      const OpalTransportAddress & localAddress,
      const OpalTransportAddress & remoteAddress
    ) const;

    /**Get the local transport address on which this listener may be accessed.
       If remoteAddress is present and is an address that requires NAT for
       connectivity, then the returned address is adjusted to return the
       external address and port.
      */
    virtual OpalTransportAddress GetLocalAddress(
      const OpalTransportAddress & remoteAddress = OpalTransportAddress()
    ) const;
  //@}


  protected:
    virtual const char * GetProtoPrefix() const;

    PMonitoredSocketsPtr listenerBundle;
};


////////////////////////////////////////////////////////////////

/**This class describes a I/O transport for a protocol.
   A "transport" is an object that allows the transfer and processing of data
   from one entity to another.
 */
class OpalTransport : public PIndirectChannel
{
    PCLASSINFO(OpalTransport, PIndirectChannel);
  public:
  /**@name Construction */
  //@{
    /**Create a new transport channel.
     */
    OpalTransport(OpalEndPoint & endpoint);

    /** Destroy the transport channel.
      */
    ~OpalTransport();
  //@}

  /**@name Overrides from PObject */
  //@{
    /**Print the description of the listener to the stream.
      */
    void PrintOn(
      ostream & strm
    ) const;
  //@}

  /**@name Operations */
  //@{
    /**Get indication of the type of underlying transport.
      */
    virtual PBoolean IsReliable() const = 0;

    /** Get the interface this transport is bound to.
        This is generally only relevant for datagram based transports such as
        UDP and TCP is always bound to a local interface once open.

        The default behaviour returns the local address via GetLocalAddress()
      */
    virtual PString GetInterface() const;

    /**Bind this transport to an interface.
        This is generally only relevant for datagram based transports such as
        UDP and TCP is always bound to a local interface once open.

       The default behaviour does nothing.
      */
    virtual bool SetInterface(
      const PString & iface  ///< Interface to use
    );

    /**Get the transport dependent name of the local endpoint.
      */
    virtual OpalTransportAddress GetLocalAddress(
      bool allowNAT = true   ///< Allow translation if remote needs NAT
    ) const = 0;

    /**Set local address to connect from.
       Note that this may not work for all transport types or may work only
       before Connect() has been called.
      */
    virtual PBoolean SetLocalAddress(
      const OpalTransportAddress & address
    ) = 0;

    /**Get the transport address of the remote endpoint.
      */
    virtual OpalTransportAddress GetRemoteAddress() const = 0;

    /**Set remote address to connect to.
       Note that this does not necessarily initiate a transport level
       connection, but only indicates where to connect to. The actual
       connection is made by the Connect() function.
      */
    virtual PBoolean SetRemoteAddress(
      const OpalTransportAddress & address
    ) = 0;

    /**Connect to the remote address.
      */
    virtual PBoolean Connect() = 0;

    /**Connect to the specified address.
      */
    PBoolean ConnectTo(
      const OpalTransportAddress & address
    ) { return SetRemoteAddress(address) && Connect(); }

    /**Close the channel.
      */
    virtual PBoolean Close();

    /**Close channel and wait for associated thread to terminate.
      */
    void CloseWait();

    /**Close channel and wait for associated thread to terminate.
       For backward compatibility with OpenH323, now deprecated.
      */
    void CleanUpOnTermination() { CloseWait(); }

    /**Check that the transport address is compatible with transport.
      */
    virtual PBoolean IsCompatibleTransport(
      const OpalTransportAddress & address
    ) const = 0;

    ///<  Promiscious modes for transport
    enum PromisciousModes {
      AcceptFromRemoteOnly,
      AcceptFromAnyAutoSet,
      AcceptFromAny,
      NumPromisciousModes
    };

    /**Set read to promiscuous mode.
       Normally only reads from the specifed remote address are accepted. This
       flag allows packets to be accepted from any remote, provided the
       underlying protocol can do so. For example TCP will do nothing.

       The Read() call may optionally set the remote address automatically to
       whatever the sender host of the last received message was.

       Default behaviour does nothing.
      */
    virtual void SetPromiscuous(
      PromisciousModes promiscuous
    );

    /**Get the transport address of the last received PDU.

       Default behaviour returns GetRemoteAddress().
      */
    virtual OpalTransportAddress GetLastReceivedAddress() const;

    /**Get the interface of the last received PDU arrived on.

       Default behaviour returns GetLocalAddress().
      */
    virtual PString GetLastReceivedInterface() const;

    /**Read a protocol data unit from the transport.
       This will read using the transports mechanism for PDU boundaries, for
       example UDP is a single Read() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual PBoolean ReadPDU(
      PBYTEArray & packet   ///<  Packet read from transport
    ) = 0;

    /**Write a packet to the transport.
       This will write using the transports mechanism for PDU boundaries, for
       example UDP is a single Write() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual PBoolean WritePDU(
      const PBYTEArray & pdu     ///<  Packet to write
    ) = 0;

    typedef PBoolean (*WriteConnectCallback)(OpalTransport & transport, void * userData);

    /**Write the first packet to the transport, after a connect.
       This will adjust the transport object and call the callback function,
       possibly multiple times for some transport types.

       It is expected that this is used just after a Connect() call where some
       transports (eg UDP) cannot determine its local address which is
       required in the PDU to be sent. This must be done fer each interface so
       WriteConnect() calls WriteConnectCallback for each interface. The
       subsequent ReadPDU() returns the answer from the first interface.

       The default behaviour simply calls the WriteConnectCallback function.
      */
    virtual PBoolean WriteConnect(
      WriteConnectCallback function,  ///<  Function for writing data
      void * userData                 ///<  user data to pass to write function
    );

    /**Attach a thread to the transport.
      */
    virtual void AttachThread(
      PThread * thread
    );

    /**Determine of the transport is running with a background thread.
      */
    virtual PBoolean IsRunning() const;
  //@}

    OpalEndPoint & GetEndPoint() const { return endpoint; }

    /**Get the prefix for this transports protocol type.
      */
    virtual const char * GetProtoPrefix() const = 0;

    PMutex & GetWriteMutex() { return m_writeMutex; }

  protected:
    OpalEndPoint & endpoint;
    PThread      * thread;      ///<  Thread handling the transport
    PMutex         m_writeMutex;
};


class OpalTransportIP : public OpalTransport
{
  PCLASSINFO(OpalTransportIP, OpalTransport);
  public:
  /**@name Construction */
  //@{
    /**Create a new transport channel.
     */
    OpalTransportIP(
      OpalEndPoint & endpoint,    ///<  Endpoint object
      PIPSocket::Address binding, ///<  Local interface to use
      WORD port                   ///<  Local port to bind to
    );
  //@}

  /**@name Operations */
  //@{
    /**Get the transport dependent name of the local endpoint.
      */
    virtual OpalTransportAddress GetLocalAddress(
      bool allowNAT = true   ///< Allow translation if remote needs NAT
    ) const;

    /**Set local address to connect from.
       Note that this may not work for all transport types or may work only
       before Connect() has been called.
      */
    virtual PBoolean SetLocalAddress(
      const OpalTransportAddress & address
    );

    /**Get the transport dependent name of the remote endpoint.
      */
    virtual OpalTransportAddress GetRemoteAddress() const;

    /**Set remote address to connect to.
       Note that this does not necessarily initiate a transport level
       connection, but only indicates where to connect to. The actual
       connection is made by the Connect() function.
      */
    virtual PBoolean SetRemoteAddress(
      const OpalTransportAddress & address
    );

  //@}

  protected:
    /**Get the prefix for this transports protocol type.
      */
    virtual const char * GetProtoPrefix() const = 0;

    PIPSocket::Address localAddress;  // Address of the local interface
    WORD               localPort;
    PIPSocket::Address remoteAddress; // Address of the remote host
    WORD               remotePort;
};


class OpalTransportTCP : public OpalTransportIP
{
    PCLASSINFO(OpalTransportTCP, OpalTransportIP);
  public:
  /**@name Construction */
  //@{
    /**Create a new transport channel.
     */
    OpalTransportTCP(
      OpalEndPoint & endpoint,    ///<  Endpoint object
      PIPSocket::Address binding = PIPSocket::GetDefaultIpAny(), ///<  Local interface to use
      WORD port = 0,              ///<  Local port to bind to
      PBoolean reuseAddr = PFalse      ///<  Flag for binding to already bound interface
    );
    OpalTransportTCP(
      OpalEndPoint & endpoint,    ///<  Endpoint object
      PTCPSocket * socket         ///<  Socket to use
    );

    /// Destroy the TCP channel
    ~OpalTransportTCP();
  //@}

  /**@name Overides from class OpalTransport */
  //@{
    /**Get indication of the type of underlying transport.
      */
    virtual PBoolean IsReliable() const;

    /**Check that the transport address is compatible with transport.
      */
    virtual PBoolean IsCompatibleTransport(
      const OpalTransportAddress & address
    ) const;

    /**Connect to the remote address.
      */
    virtual PBoolean Connect();

    /**Read a packet from the transport.
       This will read using the transports mechanism for PDU boundaries, for
       example UDP is a single Read() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual PBoolean ReadPDU(
      PBYTEArray & pdu  ///<  PDU read from transport
    );

    /**Write a packet to the transport.
       This will write using the transports mechanism for PDU boundaries, for
       example UDP is a single Write() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual PBoolean WritePDU(
      const PBYTEArray & pdu     ///<  Packet to write
    );
  //@}


  protected:
    /**Get the prefix for this transports protocol type.
      */
    virtual const char * GetProtoPrefix() const;

    /**This callback is executed when the Open() function is called with
       open channels. It may be used by descendent channels to do any
       handshaking required by the protocol that channel embodies.

       The default behaviour is to simply return PTrue.

       @return
       Returns PTrue if the protocol handshaking is successful.
     */
    virtual PBoolean OnOpen();


    PBoolean reuseAddressFlag;
};


class OpalTransportUDP : public OpalTransportIP
{
  PCLASSINFO(OpalTransportUDP, OpalTransportIP);
  public:
  /**@name Construction */
  //@{
    /**Create a new transport channel.
     */
    OpalTransportUDP(
      OpalEndPoint & endpoint,    ///<  Endpoint object
      PIPSocket::Address binding = PIPSocket::GetDefaultIpAny(), ///<  Local interface to use
      WORD port = 0,              ///<  Local port to bind to
      bool reuseAddr = false,     ///<  Flag for binding to already bound interface
      bool preOpen = false        ///<  Flag to pre-open socket
    );

    /**Create a new transport channel.
     */
    OpalTransportUDP(
      OpalEndPoint & endpoint,              ///<  Endpoint object
      const PBYTEArray & preReadPacket,     ///<  Packet already read by OpalListenerUDP
      const PMonitoredSocketsPtr & sockets, ///<  Bundle of sockets from OpalListenerUDP
      const PString & iface,                ///<  Local interface to use
      PIPSocket::Address remoteAddress,     ///<  Remote address received PDU on
      WORD remotePort                       ///<  Remote port received PDU on
    );

    /// Destroy the UDP channel
    ~OpalTransportUDP();
  //@}

  /**@name Overides from class PChannel */
  //@{
    virtual PBoolean Read(
      void * buffer,
      PINDEX length
    );
  //@}

  /**@name Overides from class OpalTransport */
  //@{
    /**Get indication of the type of underlying transport.
      */
    virtual PBoolean IsReliable() const;

    /**Check that the transport address is compatible with transport.
      */
    virtual PBoolean IsCompatibleTransport(
      const OpalTransportAddress & address
    ) const;

    /**Connect to the remote party.
       This will createa a socket for each interface on the system, then the
       use of WriteConnect() will send out on every interface. ReadPDU() will
       return the first interface that has data, then the user can select
       which interface it wants by further calls to ReadPDU(). Once it has
       selected one it calls SetInterface() to finalise the selection process.
      */
    virtual PBoolean Connect();

    /** Get the interface this transport is bound to.
      */
    virtual PString GetInterface() const;

    /**Bind this transport to an interface.
        This is generally only relevant for datagram based transports such as
        UDP and TCP is always bound to a local interface once open.

       The default behaviour does nothing.
      */
    virtual bool SetInterface(
      const PString & iface  ///< Interface to use
    );

    /**Get the transport dependent name of the local endpoint.
      */
    virtual OpalTransportAddress GetLocalAddress(
      bool allowNAT = true   ///< Allow translation if remote needs NAT
    ) const;

    /**Set local address to connect from.
       Note that this may not work for all transport types or may work only
       before Connect() has been called.
      */
    virtual PBoolean SetLocalAddress(
      const OpalTransportAddress & address
    );

    /**Set remote address to connect to.
       Note that this does not necessarily initiate a transport level
       connection, but only indicates where to connect to. The actual
       connection is made by the Connect() function.
      */
    virtual PBoolean SetRemoteAddress(
      const OpalTransportAddress & address
    );

    /**Set read to promiscuous mode.
       Normally only reads from the specifed remote address are accepted. This
       flag allows packets to be accepted from any remote, provided the
       underlying protocol can do so.

       The Read() call may optionally set the remote address automatically to
       whatever the sender host of the last received message was.

       Default behaviour sets the internal flag, so that Read() operates as
       described.
      */
    virtual void SetPromiscuous(
      PromisciousModes promiscuous
    );

    /**Get the transport address of the last received PDU.

       Default behaviour returns the lastReceivedAddress member variable.
      */
    virtual OpalTransportAddress GetLastReceivedAddress() const;

    /**Get the interface of the last received PDU arrived on.

       Default behaviour returns GetLocalAddress().
      */
    virtual PString GetLastReceivedInterface() const;

    /**Read a protocol data unit from the transport.
       This will read using the transports mechanism for PDU boundaries, for
       example UDP is a single Read() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual PBoolean ReadPDU(
      PBYTEArray & packet   ///<  Packet read from transport
    );

    /**Write a packet to the transport.
       This will write using the transports mechanism for PDU boundaries, for
       example UDP is a single Write() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual PBoolean WritePDU(
      const PBYTEArray & pdu     ///<  Packet to write
    );

    /**Write the first packet to the transport, after a connect.
       This will adjust the transport object and call the callback function,
       possibly multiple times for some transport types.

       It is expected that this is used just after a Connect() call where some
       transports (eg UDP) cannot determine its local address which is
       required in the PDU to be sent. This must be done fer each interface so
       WriteConnect() calls WriteConnectCallback for each interface. The
       subsequent ReadPDU() returns the answer from the first interface.
      */
    virtual PBoolean WriteConnect(
      WriteConnectCallback function,  ///<  Function for writing data
      void * userData                 ///<  User data to pass to write function
    );
  //@}

  protected:
    /**Get the prefix for this transports protocol type.
      */
    virtual const char * GetProtoPrefix() const;

    OpalManager & manager;
    PBYTEArray    preReadPacket;
};


////////////////////////////////////////////////////////////////

class OpalInternalTransport : public PObject
{
    PCLASSINFO(OpalInternalTransport, PObject);
  public:
    virtual PString GetHostName(
      const OpalTransportAddress & address
    ) const;

    virtual PBoolean GetIpAndPort(
      const OpalTransportAddress & address,
      PIPSocket::Address & ip,
      WORD & port
    ) const;

    virtual OpalListener * CreateListener(
      const OpalTransportAddress & address,
      OpalEndPoint & endpoint,
      OpalTransportAddress::BindOptions options
    ) const  = 0;

    virtual OpalTransport * CreateTransport(
      const OpalTransportAddress & address,
      OpalEndPoint & endpoint,
      OpalTransportAddress::BindOptions options
    ) const = 0;
};


////////////////////////////////////////////////////////////////

class OpalInternalIPTransport : public OpalInternalTransport
{
    PCLASSINFO(OpalInternalIPTransport, OpalInternalTransport);
  public:
    virtual PString GetHostName(
      const OpalTransportAddress & address
    ) const;
    virtual PBoolean GetIpAndPort(
      const OpalTransportAddress & address,
      PIPSocket::Address & ip,
      WORD & port
    ) const;

    static PBoolean GetAdjustedIpAndPort(const OpalTransportAddress & address,
                                     OpalEndPoint & endpoint,
                                     OpalTransportAddress::BindOptions option,
                                     PIPSocket::Address & ip,
                                     WORD & port,
                                     PBoolean & reuseAddr);
};

template <class ListenerType, class TransportType, unsigned AltTypeOption, class AltTypeClass>
class OpalInternalIPTransportTemplate : public OpalInternalIPTransport
{
  public:
    OpalListener * CreateListener(
      const OpalTransportAddress & address,
      OpalEndPoint & endpoint,
      OpalTransportAddress::BindOptions options
    ) const
    {
      return new ListenerType(endpoint, address, options);
    }

    OpalTransport * CreateTransport(
      const OpalTransportAddress & address,
      OpalEndPoint & endpoint,
      OpalTransportAddress::BindOptions options
    ) const
    {
      PIPSocket::Address ip;
      WORD port;
      PBoolean reuseAddr;
      if (GetAdjustedIpAndPort(address, endpoint, options, ip, port, reuseAddr)) {
        if (options == AltTypeOption)
          return new AltTypeClass(endpoint, ip, 0, reuseAddr);
        else
          return new TransportType(endpoint, ip, 0, reuseAddr);
      }
      return NULL;
    }
};

typedef OpalInternalIPTransportTemplate<OpalListenerTCP, OpalTransportTCP, OpalTransportAddress::Datagram, OpalTransportUDP> OpalInternalTCPTransport;
typedef OpalInternalIPTransportTemplate<OpalListenerUDP, OpalTransportUDP, OpalTransportAddress::Streamed, OpalTransportTCP> OpalInternalUDPTransport;

#if OPAL_PTLIB_SSL

class PSSLContext;

class OpalListenerTCPS : public OpalListenerTCP
{
  PCLASSINFO(OpalListenerTCPS, OpalListenerTCP);
  public:
    OpalListenerTCPS(
      OpalEndPoint & endpoint,                 ///<  Endpoint listener is used for
      PIPSocket::Address binding = PIPSocket::GetDefaultIpAny(), ///<  Local interface to listen on
      WORD port = 0,                           ///<  TCP port to listen for connections
      PBoolean exclusive = PTrue
    );
    OpalListenerTCPS(
      OpalEndPoint & endpoint,                  ///<  Endpoint listener is used for
      const OpalTransportAddress & binding,     ///<  Local interface to listen on
      OpalTransportAddress::BindOptions option  ///< OPtions for binding
    );

    /** Destroy the listener thread.
      */
    ~OpalListenerTCPS();

    OpalTransport * Accept(const PTimeInterval & timeout);
    const char * GetProtoPrefix() const;

  protected:
    void Construct();

    PSSLContext * sslContext;
};

class OpalTransportTCPS : public OpalTransportTCP
{
  PCLASSINFO(OpalTransportTCPS, OpalTransportTCP);
    public:
      OpalTransportTCPS(
        OpalEndPoint & endpoint,    ///<  Endpoint object
        PIPSocket::Address binding = PIPSocket::GetDefaultIpAny(), ///<  Local interface to use
        WORD port = 0,              ///<  Local port to bind to
        PBoolean reuseAddr = PFalse      ///<  Flag for binding to already bound interface
      );
      OpalTransportTCPS(
        OpalEndPoint & endpoint,    ///<  Endpoint object
        PTCPSocket * socket         ///<  Socket to use
      );

      /// Destroy the TCPS channel
      ~OpalTransportTCPS();

      PBoolean IsCompatibleTransport(const OpalTransportAddress & address) const;
      PBoolean Connect();
      PBoolean OnOpen();
      const char * GetProtoPrefix() const;

    protected:
      PSSLContext * sslContext;
};

typedef OpalInternalIPTransportTemplate<OpalListenerTCPS, OpalTransportTCPS, OpalTransportAddress::Datagram, OpalTransportUDP> OpalInternalTCPSTransport;


#endif // OPAL_PTLIB_SSL


#endif  // OPAL_OPAL_TRANSPORT_H


// End of File ///////////////////////////////////////////////////////////////
