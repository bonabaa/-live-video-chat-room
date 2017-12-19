/*
 * transaddr.cxx
 *
 * H.323 transports handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2000 Equivalence Pty. Ltd.
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

#include <ptlib.h>

#include <opal/buildopts.h>
#if OPAL_H323

#ifdef __GNUC__
#pragma implementation "transaddr.h"
#endif

#include <h323/transaddr.h>

#include <h323/h323ep.h>
#include <h323/h323pdu.h>



static PString BuildIP(const PIPSocket::Address & ip, unsigned port, const char * proto)
{
  PStringStream str;

  if (proto == NULL)
    str << "ip$"; // Compatible with both tcp$ and udp$
  else {
    str << proto;
    if (str.Find('$') == P_MAX_INDEX)
      str << '$';
  }

  if (!ip.IsValid())
    str << '*';
  else
#if OPAL_PTLIB_IPV6
  if (ip.GetVersion() == 6)
    str << '[' << ip << ']';
  else
#endif
    str << ip;

  if (port != 0)
    str << ':' << port;

  return str;
}


/////////////////////////////////////////////////////////////////////////////

H323TransportAddress::H323TransportAddress(const H225_TransportAddress & transport,
                                           const char * proto)
{
  switch (transport.GetTag()) {
    case H225_TransportAddress::e_ipAddress :
    {
      const H225_TransportAddress_ipAddress & ip = transport;
      *this = BuildIP(PIPSocket::Address(ip.m_ip.GetSize(), ip.m_ip.GetValue()), ip.m_port, proto);
      break;
    }
#if OPAL_PTLIB_IPV6
    case H225_TransportAddress::e_ip6Address :
    {
      const H225_TransportAddress_ip6Address & ip = transport;
      *this = BuildIP(PIPSocket::Address(ip.m_ip.GetSize(), ip.m_ip.GetValue()), ip.m_port, proto);
      break;
    }
#endif
  }

  SetInternalTransport(0, NULL);
}


H323TransportAddress::H323TransportAddress(const H245_TransportAddress & transport,
                                           const char * proto)
{
  switch (transport.GetTag()) {
    case H245_TransportAddress::e_unicastAddress :
    {
      const H245_UnicastAddress & unicast = transport;
      switch (unicast.GetTag()) {
        case H245_UnicastAddress::e_iPAddress :
        {
          const H245_UnicastAddress_iPAddress & ip = unicast;
          *this = BuildIP(PIPSocket::Address(ip.m_network.GetSize(), ip.m_network.GetValue()), ip.m_tsapIdentifier, proto);
          break;
        }
#if OPAL_PTLIB_IPV6
        case H245_UnicastAddress::e_iP6Address :
        {
          const H245_UnicastAddress_iP6Address & ip = unicast;
          *this = BuildIP(PIPSocket::Address(ip.m_network.GetSize(), ip.m_network.GetValue()), ip.m_tsapIdentifier, proto);
          break;
        }
#endif
      }
      break;
    }
  }

  SetInternalTransport(0, NULL);
}


PBoolean H323TransportAddress::SetPDU(H225_TransportAddress & pdu, WORD defPort) const
{
  PIPSocket::Address ip;
  WORD port = defPort;
  if (GetIpAndPort(ip, port)) {
#if OPAL_PTLIB_IPV6
    if (ip.GetVersion() == 6) {
      pdu.SetTag(H225_TransportAddress::e_ip6Address);
      H225_TransportAddress_ip6Address & addr = pdu;
      for (PINDEX i = 0; i < ip.GetSize(); i++)
        addr.m_ip[i] = ip[i];
      addr.m_port = port;
      return PTrue;
    }
#endif

    PAssert(port != 0, "Attempt to set transport address with empty port");

    pdu.SetTag(H225_TransportAddress::e_ipAddress);
    H225_TransportAddress_ipAddress & addr = pdu;
    for (PINDEX i = 0; i < 4; i++)
      addr.m_ip[i] = ip[i];
    addr.m_port = port;
    return PTrue;
  }

  return PFalse;
}


PBoolean H323TransportAddress::SetPDU(H245_TransportAddress & pdu, WORD defPort) const
{
  WORD port = defPort;
  if (defPort == 0)
    defPort = H323EndPoint::DefaultTcpSignalPort;

  PIPSocket::Address ip;
  if (GetIpAndPort(ip, port)) {
    pdu.SetTag(H245_TransportAddress::e_unicastAddress);

    H245_UnicastAddress & unicast = pdu;

#if OPAL_PTLIB_IPV6
    if (ip.GetVersion() == 6) {
      unicast.SetTag(H245_UnicastAddress::e_iP6Address);
      H245_UnicastAddress_iP6Address & addr = unicast;
      for (PINDEX i = 0; i < ip.GetSize(); i++)
        addr.m_network[i] = ip[i];
      addr.m_tsapIdentifier = port;
      return PTrue;
    }
#endif

    unicast.SetTag(H245_UnicastAddress::e_iPAddress);
    H245_UnicastAddress_iPAddress & addr = unicast;
    for (PINDEX i = 0; i < 4; i++)
      addr.m_network[i] = ip[i];
    addr.m_tsapIdentifier = port;
    return PTrue;
  }

  return PFalse;
}


/////////////////////////////////////////////////////////////////////////////

H323TransportAddressArray::H323TransportAddressArray(const H225_ArrayOf_TransportAddress & addresses)
{
  for (PINDEX i = 0; i < addresses.GetSize(); i++)
    AppendAddress(H323TransportAddress(addresses[i]));
}


void H323TransportAddressArray::AppendString(const char * str)
{
  AppendAddress(H323TransportAddress(str));
}


void H323TransportAddressArray::AppendString(const PString & str)
{
  AppendAddress(H323TransportAddress(str));
}


void H323TransportAddressArray::AppendAddress(const H323TransportAddress & addr)
{
  if (!addr)
    Append(new H323TransportAddress(addr));
}


void H323TransportAddressArray::AppendStringCollection(const PCollection & coll)
{
  for (PINDEX i = 0; i < coll.GetSize(); i++) {
    PObject * obj = coll.GetAt(i);
    if (obj != NULL && PIsDescendant(obj, PString))
      AppendAddress(H323TransportAddress(*(PString *)obj));
  }
}


void H323SetTransportAddresses(const H323Transport & associatedTransport,
                               const H323TransportAddressArray & addresses,
                               H225_ArrayOf_TransportAddress & pdu)
{
  for (PINDEX i = 0; i < addresses.GetSize(); i++) {
    H323TransportAddress addr = addresses[i];

    PTRACE(4, "TCP\tAppending H.225 transport " << addr
           << " using associated transport " << associatedTransport);

    PIPSocket::Address ip;
    WORD port;
    if (addr.GetIpAndPort(ip, port)) {
      PIPSocket::Address remoteIP;
      if (associatedTransport.GetRemoteAddress().GetIpAddress(remoteIP)) {
        if (associatedTransport.GetEndPoint().GetManager().TranslateIPAddress(ip, remoteIP))
          addr = H323TransportAddress(ip, port);
      }
    }

    H225_TransportAddress pduAddr;
    addr.SetPDU(pduAddr, associatedTransport.GetEndPoint().GetDefaultSignalPort());

    PINDEX lastPos = pdu.GetSize();

    // Check for already have had that address.
    PINDEX j;
    for (j = 0; j < lastPos; j++) {
      if (pdu[j] == pduAddr)
        break;
    }

    if (j >= lastPos) {
      // Put new listener into array
      pdu.SetSize(lastPos+1);
      pdu[lastPos] = pduAddr;
    }
  }
}


#endif // OPAL_H323

/////////////////////////////////////////////////////////////////////////////
