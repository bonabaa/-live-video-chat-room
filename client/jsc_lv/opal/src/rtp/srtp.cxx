/*
 * srtp.cxx
 *
 * SRTP protocol handler
 *
 * OPAL Library
 *
 * Copyright (C) 2006 Post Increment
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
 * The Original Code is OPAL Library.
 *
 * The Initial Developer of the Original Code is Post Increment
 *     Portions of this code were written with the assistance of funding from
 *     US Joint Forces Command Joint Concept Development & Experimentation (J9)
 *     http://www.jfcom.mil/about/abt_j9.htm
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 22444 $
 * $Author: rjongbloed $
 * $Date: 2009-04-20 23:49:06 +0000 (Mon, 20 Apr 2009) $
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "srtp.h"
#endif

#include <opal/buildopts.h>

#if OPAL_SRTP

#include <rtp/srtp.h>
#include <opal/connection.h>
#include <h323/h323caps.h>
#include <h323/h235auth.h>


class PNatMethod;


// default key = 2687012454


////////////////////////////////////////////////////////////////////
//
//  this class implements SRTP over UDP
//

OpalSRTP_UDP::OpalSRTP_UDP(const Params & params)
  : SecureRTP_UDP(params)
{
}


/////////////////////////////////////////////////////////////////////////////////////
//
//  SRTP implementation using Cisco libSRTP
//  See http://srtp.sourceforge.net/srtp.html
//

////////////////////////////////////////////////////////////////////
//
//  implement SRTP via libSRTP
//


#ifdef LIBSRTP_LIBRARY
#pragma comment(lib, LIBSRTP_LIBRARY)
#endif

#ifdef _WIN32
#pragma warning(disable:4244)
#pragma warning(disable:4505)
#endif

extern "C" {
#include "srtp/srtp.h"
};

///////////////////////////////////////////////////////

class LibSRTPSecurityMode_Base : public OpalSRTPSecurityMode
{
  PCLASSINFO(LibSRTPSecurityMode_Base, OpalSRTPSecurityMode);
  public:
    RTP_UDP * CreateRTPSession(
      OpalRTPConnection & connection,     ///< Connection creating session (may be needed by secure connections)
      const RTP_Session::Params & options ///< Parameters to construct with session.
    );

    PBoolean SetOutgoingKey(const KeySalt & key)  { outgoingKey = key; return PTrue; }
    PBoolean GetOutgoingKey(KeySalt & key) const  { key = outgoingKey; return PTrue; }
    PBoolean SetOutgoingSSRC(DWORD ssrc);
    PBoolean GetOutgoingSSRC(DWORD & ssrc) const;

    PBoolean SetIncomingKey(const KeySalt & key)  { incomingKey = key; return PTrue; }
    PBoolean GetIncomingKey(KeySalt & key) const  { key = incomingKey; return PTrue; } ;
    PBoolean SetIncomingSSRC(DWORD ssrc);
    PBoolean GetIncomingSSRC(DWORD & ssrc) const;

    PBoolean Open();

    srtp_t inboundSession;
    srtp_t outboundSession;

  protected:
    void Init();
    KeySalt incomingKey;
    KeySalt outgoingKey;
    srtp_policy_t inboundPolicy;
    srtp_policy_t outboundPolicy;

  private:
    static PBoolean inited;
    static PMutex initMutex;
};

PBoolean LibSRTPSecurityMode_Base::inited = PFalse;
PMutex LibSRTPSecurityMode_Base::initMutex;

void LibSRTPSecurityMode_Base::Init()
{
  {
    PWaitAndSignal m(initMutex);
    if (!inited) {
      srtp_init();
      inited = PTrue;
    }
  }
  inboundPolicy.ssrc.type  = ssrc_any_inbound;
  inboundPolicy.next       = NULL;
  outboundPolicy.ssrc.type = ssrc_any_outbound;
  outboundPolicy.next      = NULL;

  crypto_get_random(outgoingKey.key.GetPointer(SRTP_MASTER_KEY_LEN), SRTP_MASTER_KEY_LEN);
}


RTP_UDP * LibSRTPSecurityMode_Base::CreateRTPSession(OpalRTPConnection & /*connection*/,
                                                     const RTP_Session::Params & options)
{
  LibSRTP_UDP * session = new LibSRTP_UDP(options);
  session->SetSecurityMode(this);
  return session;
}

PBoolean LibSRTPSecurityMode_Base::SetIncomingSSRC(DWORD ssrc)
{
  inboundPolicy.ssrc.type  = ssrc_specific;
  inboundPolicy.ssrc.value = ssrc;
  return PTrue;
}

PBoolean LibSRTPSecurityMode_Base::SetOutgoingSSRC(DWORD ssrc)
{
  outboundPolicy.ssrc.type = ssrc_specific;
  outboundPolicy.ssrc.value = ssrc;

  return PTrue;
}

PBoolean LibSRTPSecurityMode_Base::GetOutgoingSSRC(DWORD & ssrc) const
{
  if (outboundPolicy.ssrc.type != ssrc_specific)
    return PFalse;
  ssrc = outboundPolicy.ssrc.value;
  return PTrue;
}

PBoolean LibSRTPSecurityMode_Base::GetIncomingSSRC(DWORD & ssrc) const
{
  if (inboundPolicy.ssrc.type != ssrc_specific)
    return PFalse;

  ssrc = inboundPolicy.ssrc.value;
  return PTrue;
}

PBoolean LibSRTPSecurityMode_Base::Open()
{
  outboundPolicy.key = outgoingKey.key.GetPointer();
  err_status_t err = srtp_create(&outboundSession, &outboundPolicy);
  if (err != ::err_status_ok)
    return PFalse;

  inboundPolicy.key = incomingKey.key.GetPointer();
  err = srtp_create(&inboundSession, &inboundPolicy);
  if (err != ::err_status_ok)
    return PFalse;

  return PTrue;
}


#define DECLARE_LIBSRTP_CRYPTO_ALG(name, policy_fn) \
class LibSRTPSecurityMode_##name : public LibSRTPSecurityMode_Base \
{ \
  public: \
  LibSRTPSecurityMode_##name() \
    { \
      policy_fn(&inboundPolicy.rtp); \
      policy_fn(&inboundPolicy.rtcp); \
      policy_fn(&outboundPolicy.rtp); \
      policy_fn(&outboundPolicy.rtcp); \
      Init(); \
    } \
}; \
PFACTORY_CREATE(PFactory<OpalSecurityMode>, LibSRTPSecurityMode_##name, "SRTP|" #name, false)

DECLARE_LIBSRTP_CRYPTO_ALG(AES_CM_128_HMAC_SHA1_80,  crypto_policy_set_aes_cm_128_hmac_sha1_80);
DECLARE_LIBSRTP_CRYPTO_ALG(AES_CM_128_HMAC_SHA1_32,  crypto_policy_set_aes_cm_128_hmac_sha1_32);
DECLARE_LIBSRTP_CRYPTO_ALG(AES_CM_128_NULL_AUTH,     crypto_policy_set_aes_cm_128_null_auth);
DECLARE_LIBSRTP_CRYPTO_ALG(NULL_CIPHER_HMAC_SHA1_80, crypto_policy_set_null_cipher_hmac_sha1_80);

DECLARE_LIBSRTP_CRYPTO_ALG(STRONGHOLD,               crypto_policy_set_aes_cm_128_hmac_sha1_80);

///////////////////////////////////////////////////////

LibSRTP_UDP::LibSRTP_UDP(const Params & params)
  : OpalSRTP_UDP(params)
{
}

LibSRTP_UDP::~LibSRTP_UDP()
{
}

PBoolean LibSRTP_UDP::Open(
      PIPSocket::Address localAddress,  ///<  Local interface to bind to
      WORD portBase,                    ///<  Base of ports to search
      WORD portMax,                     ///<  end of ports to search (inclusive)
      BYTE ipTypeOfService,             ///<  Type of Service byte
      PNatMethod * nat,                 ///<  NAT method to use createing sockets (or NULL if no STUN)
      RTP_QOS * rtpqos                  ///<  QOS spec (or NULL if no QoS)
)
{
  LibSRTPSecurityMode_Base * srtp = (LibSRTPSecurityMode_Base *)securityParms;
  if (srtp == NULL)
    return PFalse;

  // get the inbound and outbound SSRC from the SRTP parms and into the RTP session
  srtp->GetOutgoingSSRC(syncSourceOut);
  srtp->GetIncomingSSRC(syncSourceIn);

  return OpalSRTP_UDP::Open(localAddress, portBase, portMax, ipTypeOfService, nat, rtpqos);
}


RTP_UDP::SendReceiveStatus LibSRTP_UDP::OnSendData(RTP_DataFrame & frame)
{
  SendReceiveStatus stat = RTP_UDP::OnSendData(frame);
  if (stat != e_ProcessPacket)
    return stat;

  LibSRTPSecurityMode_Base * srtp = (LibSRTPSecurityMode_Base *)securityParms;

  int len = frame.GetHeaderSize() + frame.GetPayloadSize();
  frame.SetPayloadSize(len + SRTP_MAX_TRAILER_LEN);
  err_status_t err = ::srtp_protect(srtp->outboundSession, frame.GetPointer(), &len);
  if (err != err_status_ok)
    return RTP_Session::e_IgnorePacket;
  frame.SetPayloadSize(len - frame.GetHeaderSize());
  return e_ProcessPacket;
}

RTP_UDP::SendReceiveStatus LibSRTP_UDP::OnReceiveData(RTP_DataFrame & frame)
{
  LibSRTPSecurityMode_Base * srtp = (LibSRTPSecurityMode_Base *)securityParms;

  int len = frame.GetHeaderSize() + frame.GetPayloadSize();
  err_status_t err = ::srtp_unprotect(srtp->inboundSession, frame.GetPointer(), &len);
  if (err != err_status_ok)
    return RTP_Session::e_IgnorePacket;
  frame.SetPayloadSize(len - frame.GetHeaderSize());

  return RTP_UDP::OnReceiveData(frame);
}

RTP_UDP::SendReceiveStatus LibSRTP_UDP::OnSendControl(RTP_ControlFrame & frame, PINDEX & transmittedLen)
{
  SendReceiveStatus stat = RTP_UDP::OnSendControl(frame, transmittedLen);
  if (stat != e_ProcessPacket)
    return stat;

  frame.SetMinSize(transmittedLen + SRTP_MAX_TRAILER_LEN);
  int len = transmittedLen;

  LibSRTPSecurityMode_Base * srtp = (LibSRTPSecurityMode_Base *)securityParms;

  err_status_t err = ::srtp_protect_rtcp(srtp->outboundSession, frame.GetPointer(), &len);
  if (err != err_status_ok)
    return RTP_Session::e_IgnorePacket;
  transmittedLen = len;

  return e_ProcessPacket;
}

RTP_UDP::SendReceiveStatus LibSRTP_UDP::OnReceiveControl(RTP_ControlFrame & frame)
{
  LibSRTPSecurityMode_Base * srtp = (LibSRTPSecurityMode_Base *)securityParms;

  int len = frame.GetSize();
  err_status_t err = ::srtp_unprotect_rtcp(srtp->inboundSession, frame.GetPointer(), &len);
  if (err != err_status_ok)
    return RTP_Session::e_IgnorePacket;
  frame.SetSize(len);

  return RTP_UDP::OnReceiveControl(frame);
}

///////////////////////////////////////////////////////

#endif // OPAL_SRTP
