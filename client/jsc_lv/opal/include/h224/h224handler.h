/*
 * h224handler.h
 *
 * H.224 protocol handler implementation for the OpenH323 Project.
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
 * $Revision: 23268 $
 * $Author: rjongbloed $
 * $Date: 2009-08-25 08:37:19 +0000 (Tue, 25 Aug 2009) $
 */

#ifndef OPAL_H224_H224HANDLER_H
#define OPAL_H224_H224HANDLER_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal/buildopts.h>

#include <opal/connection.h>
#include <opal/transports.h>
#include <opal/mediastrm.h>
#include <rtp/rtp.h>
#include <h224/h224.h>

class OpalH224Handler;

class OpalH224Client : public PObject
{
  PCLASSINFO(OpalH224Client, PObject);
  
public:

  OpalH224Client();
  ~OpalH224Client();
  
  enum {
    CMEClientID         = 0x00,
    H281ClientID        = 0x01,
    ExtendedClientID    = 0x7e,
    NonStandardClientID = 0x7f,
  };

  /**Return the client ID if this is a standard client.
     Else, return either ExtendedClientId or NonStandardClientID
    */
  virtual BYTE GetClientID() const = 0;

  /**Return the extended client ID if given. The default returns 0x00
    */
  virtual BYTE GetExtendedClientID() const { return 0x00; }

  /**Return the T.35 country code octet for the non-standard client.
     Default returns CountryCodeEscape
    */
  virtual BYTE GetCountryCode() const { return 0xff; /* CountryCodeEscape */ }

  /**Return the T.35 extension code octet for the non-standard client.
     Default returns 0x00
    */
  virtual BYTE GetCountryCodeExtension() const { return 0x00; }

  /**Return the manufacturer code word for the non-standard client.
     Default returns 0x0000
    */
  virtual WORD GetManufacturerCode() const { return 0x0000; }

  /**Return the Manufacturer Client ID for the non-standard client.
     Default returns 0x00;
    */
  virtual BYTE GetManufacturerClientID() const { return 0x00; }

  /**Return whether this client has extra capabilities.
     Default returns FALSE.
    */
  virtual PBoolean HasExtraCapabilities() const { return PFalse; }

  /**Called if the CME client received an Extra Capabilities PDU for this client.
     Default does nothing.
    */
  virtual void OnReceivedExtraCapabilities(const BYTE * /*capabilities*/, PINDEX /*size*/) { }

  /**Called if a PDU for this client was received.
     Default does nothing.
    */
  virtual void OnReceivedMessage(const H224_Frame & /*message*/) { }

  /**Called to indicate that the extra capabilities pdu should be sent.
     Default does nothing
    */
  virtual void SendExtraCapabilities() const { }

  virtual Comparison Compare(const PObject & obj);

  /**Connection to the H.224 protocol handler */
  void SetH224Handler(OpalH224Handler * handler) { h224Handler = handler; }

  /**Called by the H.224 handler to indicate if the remote party has such a client or not */
  void SetRemoteClientAvailable(PBoolean remoteClientAvailable, PBoolean remoteClientHasExtraCapabilities);

  PBoolean GetRemoteClientAvailable() const { return remoteClientAvailable; }
  PBoolean GetRemoteClientHasExtraCapabilities() const { return remoteClientHasExtraCapabilities; }

protected:

  PBoolean remoteClientAvailable;
  PBoolean remoteClientHasExtraCapabilities;
  OpalH224Handler * h224Handler;
};

PSORTED_LIST(OpalH224ClientList, OpalH224Client);

///////////////////////////////////////////////////////////////////////////////

class OpalH224MediaStream;

class OpalH224Handler : public PObject
{
  PCLASSINFO(OpalH224Handler, PObject);
	
public:
	
  OpalH224Handler();
  ~OpalH224Handler();
  
  enum {
    Broadcast = 0x0000,
    
    CMEClientListCode        = 0x01,
    CMEExtraCapabilitiesCode = 0x02,
    CMEMessage               = 0x00,
    CMECommand               = 0xff,
    
    CountryCodeEscape   = 0xff,
  };
  
  /**Adds / removes the client from the client list */
  PBoolean AddClient(OpalH224Client & client);
  PBoolean RemoveClient(OpalH224Client & client);
  
  /**Sets the transmit / receive media format*/
  void SetTransmitMediaFormat(const OpalMediaFormat & mediaFormat);
  void SetReceiveMediaFormat(const OpalMediaFormat & mediaFormat);
  
  /**Sets / unsets the transmit H224 media stream*/
  void SetTransmitMediaStream(OpalH224MediaStream * transmitMediaStream);
	
  virtual void StartTransmit();
  virtual void StopTransmit();
  
  /**Sends the complete client list with all clients registered */
  PBoolean SendClientList();
  
  /**Sends the extra capabilities for all clients that indicate to have extra capabilities. */
  PBoolean SendExtraCapabilities();
  
  /**Requests the remote side to send it's client list */
  PBoolean SendClientListCommand();
  
  /**Request the remote side to send the extra capabilities for the given client */
  PBoolean SendExtraCapabilitiesCommand(const OpalH224Client & client);

  /**Callback for H.224 clients to send their extra capabilities */
  PBoolean SendExtraCapabilitiesMessage(const OpalH224Client & client, BYTE *data, PINDEX length);

  /**Callback for H.224 clients to send a client frame */
  PBoolean TransmitClientFrame(const OpalH224Client & client, H224_Frame & frame);
	
  PBoolean HandleFrame(const RTP_DataFrame & rtpFrame);
  virtual PBoolean OnReceivedFrame(H224_Frame & frame);
  virtual PBoolean OnReceivedCMEMessage(H224_Frame & frame);
  virtual PBoolean OnReceivedClientList(H224_Frame & frame);
  virtual PBoolean OnReceivedClientListCommand();
  virtual PBoolean OnReceivedExtraCapabilities(H224_Frame & frame);
  virtual PBoolean OnReceivedExtraCapabilitiesCommand();
  
  PMutex & GetTransmitMutex() { return transmitMutex; }
	
protected:

  PMutex transmitMutex;
  PBoolean canTransmit;
  RTP_DataFrame transmitFrame;
  BYTE transmitBitIndex;
  PTime *transmitStartTime;
  OpalH224MediaStream * transmitMediaStream;
  
  H224_Frame receiveFrame;
  
  OpalH224ClientList clients;
	
private:
  void TransmitFrame(H224_Frame & frame);
	
  PBoolean transmitHDLCTunneling;
  PBoolean receiveHDLCTunneling;
};

///////////////////////////////////////////////////////////////////////////////

class OpalH224MediaStream : public OpalMediaStream
{
  PCLASSINFO(OpalH224MediaStream, OpalMediaStream);
  
public:
  OpalH224MediaStream(OpalConnection & connection,
                      OpalH224Handler & h224Handler,
                      const OpalMediaFormat & mediaFormat,
                      unsigned sessionID,
                      PBoolean isSource);
  ~OpalH224MediaStream();
  
  virtual void OnStartMediaPatch();
  virtual PBoolean Close();
  virtual PBoolean ReadPacket(RTP_DataFrame & packet);
  virtual PBoolean WritePacket(RTP_DataFrame & packet);
  virtual PBoolean IsSynchronous() const { return PFalse; }
  virtual PBoolean RequiresPatchThread() const { return isSource ? PFalse : PTrue; }
  
private:
    OpalH224Handler & h224Handler;
};

#endif // OPAL_H224_H224HANDLER_H

