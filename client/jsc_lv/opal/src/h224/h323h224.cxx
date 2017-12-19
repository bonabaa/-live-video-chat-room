/*
 * h323h224.h
 *
 * H.323 H.224 logical channel establishment implementation for the 
 * OpenH323 Project.
 *
 * Copyright (c) 2006 Network for Educational Technology, ETH Zurich.
 * Written by Hannes Friederich.
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
 * Contributor(s): ______________________________________.
 *
 * $Revision: 22709 $
 * $Author: rjongbloed $
 * $Date: 2009-05-25 00:39:16 +0000 (Mon, 25 May 2009) $
 */

#include <ptlib.h>

#include <opal/buildopts.h>

#ifdef __GNUC__
#pragma implementation "h323h224.h"
#endif

#include <h224/h323h224.h>


#if OPAL_HAS_H224

#if OPAL_H323

#include <h323/h323ep.h>
#include <h323/h323con.h>
#include <h323/channels.h>
#include <h323/h323rtp.h>

#include <asn/h245.h>

H323_H224_AnnexQCapability::H323_H224_AnnexQCapability()
: H323DataCapability(640)
{
  SetPayloadType((RTP_DataFrame::PayloadTypes)100);
}

H323_H224_AnnexQCapability::~H323_H224_AnnexQCapability()
{
}

PObject::Comparison H323_H224_AnnexQCapability::Compare(const PObject & obj) const
{
  Comparison result = H323DataCapability::Compare(obj);
  
  if(result != EqualTo)	{
    return result;
  }
	
  PAssert(PIsDescendant(&obj, H323_H224_AnnexQCapability), PInvalidCast);
	
  return EqualTo;
}

PObject * H323_H224_AnnexQCapability::Clone() const
{
  return new H323_H224_AnnexQCapability(*this);
}

unsigned H323_H224_AnnexQCapability::GetSubType() const
{
  return H245_DataApplicationCapability_application::e_genericDataCapability;
}

PString H323_H224_AnnexQCapability::GetFormatName() const
{
  return GetOpalH224_H323AnnexQ().GetName();
}

H323Channel * H323_H224_AnnexQCapability::CreateChannel(H323Connection & connection,
                                                        H323Channel::Directions direction,
                                                        unsigned int sessionID,
                                                        const H245_H2250LogicalChannelParameters * params) const
{
  return connection.CreateRealTimeLogicalChannel(*this, direction, sessionID, params);
}

PBoolean H323_H224_AnnexQCapability::OnSendingPDU(H245_DataApplicationCapability & pdu) const
{
  pdu.m_maxBitRate = maxBitRate;
  pdu.m_application.SetTag(H245_DataApplicationCapability_application::e_genericDataCapability);
	
  H245_GenericCapability & capability = pdu.m_application;
  
  H245_CapabilityIdentifier & capabilityIdentifier = capability.m_capabilityIdentifier;
  capabilityIdentifier.SetTag(H245_CapabilityIdentifier::e_standard);
  PASN_ObjectId & objectId = capabilityIdentifier;
  objectId.SetValue("0.0.8.224.1.0");
	
  return PTrue;
}

PBoolean H323_H224_AnnexQCapability::OnSendingPDU(H245_DataMode & pdu) const
{
  pdu.m_bitRate = maxBitRate;
  pdu.m_application.SetTag(H245_DataMode_application::e_genericDataMode);
  
  H245_GenericCapability & capability = pdu.m_application;
  
  H245_CapabilityIdentifier & capabilityIdentifier = capability.m_capabilityIdentifier;
  capabilityIdentifier.SetTag(H245_CapabilityIdentifier::e_standard);
  PASN_ObjectId & objectId = capabilityIdentifier;
  objectId.SetValue("0.0.8.224.1.0");
	
  return PTrue;
}

PBoolean H323_H224_AnnexQCapability::OnReceivedPDU(const H245_DataApplicationCapability & /*pdu*/)
{
  return PTrue;
}

//////////////////////////////////////////////////////////////////////

H323_H224_HDLCTunnelingCapability::H323_H224_HDLCTunnelingCapability()
: H323DataCapability(640)
{
  SetPayloadType((RTP_DataFrame::PayloadTypes)100);
}

H323_H224_HDLCTunnelingCapability::~H323_H224_HDLCTunnelingCapability()
{
}

PObject::Comparison H323_H224_HDLCTunnelingCapability::Compare(const PObject & obj) const
{
  Comparison result = H323DataCapability::Compare(obj);

  if(result != EqualTo)	{
    return result;
  }
	
  PAssert(PIsDescendant(&obj, H323_H224_HDLCTunnelingCapability), PInvalidCast);
	
  return EqualTo;
}

PObject * H323_H224_HDLCTunnelingCapability::Clone() const
{
  return new H323_H224_HDLCTunnelingCapability(*this);
}

unsigned H323_H224_HDLCTunnelingCapability::GetSubType() const
{
  return H245_DataApplicationCapability_application::e_h224;
}

PString H323_H224_HDLCTunnelingCapability::GetFormatName() const
{
  return GetOpalH224_HDLCTunneling().GetName();
}

H323Channel * H323_H224_HDLCTunnelingCapability::CreateChannel(H323Connection & connection,
                                                               H323Channel::Directions direction,
                                                               unsigned int sessionID,
                                                               const H245_H2250LogicalChannelParameters * params) const
{
  return connection.CreateRealTimeLogicalChannel(*this, direction, sessionID, params);
}

PBoolean H323_H224_HDLCTunnelingCapability::OnSendingPDU(H245_DataApplicationCapability & pdu) const
{
  pdu.m_maxBitRate = maxBitRate;
  pdu.m_application.SetTag(H245_DataApplicationCapability_application::e_h224);
	
  H245_DataProtocolCapability & dataProtocolCapability = pdu.m_application;
  dataProtocolCapability.SetTag(H245_DataProtocolCapability::e_hdlcFrameTunnelling);
	
  return PTrue;
}

PBoolean H323_H224_HDLCTunnelingCapability::OnSendingPDU(H245_DataMode & pdu) const
{
  pdu.m_bitRate = maxBitRate;
  pdu.m_application.SetTag(H245_DataMode_application::e_h224);
  
  H245_DataProtocolCapability & dataProtocolCapability = pdu.m_application;
  dataProtocolCapability.SetTag(H245_DataProtocolCapability::e_hdlcFrameTunnelling);
	
  return PTrue;
}

PBoolean H323_H224_HDLCTunnelingCapability::OnReceivedPDU(const H245_DataApplicationCapability & /*pdu*/)
{
  return PTrue;
}


#endif // OPAL_H323

#endif // OPAL_HAS_H224
