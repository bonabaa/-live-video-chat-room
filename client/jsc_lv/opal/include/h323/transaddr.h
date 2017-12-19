/*
 * transport.h
 *
 * H.323 protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2001 Equivalence Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions of this code were written with the assisance of funding from
 * Vovida Networks, Inc. http://www.vovida.com.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 22023 $
 * $Author: rjongbloed $
 * $Date: 2009-02-08 05:56:02 +0000 (Sun, 08 Feb 2009) $
 */

#ifndef OPAL_H323_TRANSADDR_H
#define OPAL_H323_TRANSADDR_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#if OPAL_H323

#include <ptlib/sockets.h>
#include <opal/transports.h>


class H225_TransportAddress;
class H245_TransportAddress;
class H225_ArrayOf_TransportAddress;


typedef OpalListener  H323Listener;
typedef PList<H323Listener> H323ListenerList;
typedef OpalTransport H323Transport;
typedef OpalTransportUDP H323TransportUDP;


///////////////////////////////////////////////////////////////////////////////

/**Transport address for H.323.
   This adds functions to the basic OpalTransportAddress for conversions to
   and from H.225 and H.245 PDU structures.
 */
class H323TransportAddress : public OpalTransportAddress
{
    PCLASSINFO(H323TransportAddress, OpalTransportAddress);
  public:
    H323TransportAddress()
      { }
    H323TransportAddress(const char * addr, WORD port = 0, const char * proto = NULL)
      : OpalTransportAddress(addr, port, proto) { }
    H323TransportAddress(const PString & addr, WORD port = 0, const char * proto = NULL)
      : OpalTransportAddress(addr, port, proto) { }
    H323TransportAddress(const OpalTransportAddress & addr)
      : OpalTransportAddress(addr) { }
    H323TransportAddress(PIPSocket::Address ip, WORD port, const char * proto = NULL)
      : OpalTransportAddress(ip, port, proto) { }

    H323TransportAddress(
      const H225_TransportAddress & pdu,
      const char * proto = NULL ///<  Default to tcp
    );
    H323TransportAddress(
      const H245_TransportAddress & pdu,
      const char * proto = NULL ///<  default to udp
    );

    PBoolean SetPDU(H225_TransportAddress & pdu, WORD defPort = 0) const;
    PBoolean SetPDU(H245_TransportAddress & pdu, WORD defPort = 0) const;
};


PDECLARE_ARRAY(H323TransportAddressArray, H323TransportAddress)
  public:
    H323TransportAddressArray(
      const OpalTransportAddress & address
    ) { AppendAddress(address); }
    H323TransportAddressArray(
      const H323TransportAddress & address
    ) { AppendAddress(address); }
    H323TransportAddressArray(
      const H225_ArrayOf_TransportAddress & addresses
    );
    H323TransportAddressArray(
      const OpalTransportAddressArray & array
    ) { AppendStringCollection(array); }
    H323TransportAddressArray(
      const PStringArray & array
    ) { AppendStringCollection(array); }
    H323TransportAddressArray(
      const PStringList & list
    ) { AppendStringCollection(list); }
    H323TransportAddressArray(
      const PSortedStringList & list
    ) { AppendStringCollection(list); }

    void AppendString(
      const char * address
    );
    void AppendString(
      const PString & address
    );
    void AppendAddress(
      const H323TransportAddress & address
    );

  protected:
    void AppendStringCollection(
      const PCollection & coll
    );
};


/**Set the PDU field for the list of transport addresses
  */
void H323SetTransportAddresses(
  const H323Transport & associatedTransport,   ///<  Transport for NAT address translation
  const H323TransportAddressArray & addresses, ///<  Addresses to set
  H225_ArrayOf_TransportAddress & pdu          ///<  List of PDU transport addresses
);


#endif // OPAL_H323

#endif // OPAL_H323_TRANSADDR_H


/////////////////////////////////////////////////////////////////////////////
