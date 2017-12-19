/*
 * h281handler.h
 *
 * H.281 protocol handler implementation for the OpenH323 Project.
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
 * $Revision: 21324 $
 * $Author: hfriederich $
 * $Date: 2008-10-14 07:12:32 +0000 (Tue, 14 Oct 2008) $
 */

#ifndef OPAL_H224_H281HANDLER_H
#define OPAL_H224_H281HANDLER_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal/buildopts.h>

#include <h224/h224handler.h>
#include <h224/h281.h>

class OpalH224Handler;

/** This class implements a storage for which cameras are
    available at both the local or remote side
 */
class H281VideoSource : public PObject
{
  PCLASSINFO(H281VideoSource, PObject);

public:
	
  H281VideoSource();
  ~H281VideoSource();
	
  PBoolean IsEnabled() const { return isEnabled; }
  void SetEnabled(PBoolean flag) { isEnabled = flag; }
	
  BYTE GetVideoSourceNumber() const { return (firstOctet >> 4) & 0x0f; }
  void SetVideoSourceNumber(BYTE number);
	
  PBoolean CanMotionVideo() const { return (firstOctet >> 2) & 0x01; }
  void SetCanMotionVideo(PBoolean flag);

  PBoolean CanNormalResolutionStillImage() const { return (firstOctet >> 1) & 0x01; }
  void SetCanNormalResolutionStillImage(PBoolean flag);

  PBoolean CanDoubleResolutionStillImage() const { return (firstOctet & 0x01); }
  void SetCanDoubleResolutionStillImage(PBoolean flag);

  PBoolean CanPan() const { return (secondOctet >> 7) & 0x01; }
  void SetCanPan(PBoolean flag);
	
  PBoolean CanTilt() const { return (secondOctet >> 6) & 0x01; }
  void SetCanTilt(PBoolean flag);
	
  PBoolean CanZoom() const { return (secondOctet >> 5) & 0x01; }
  void SetCanZoom(PBoolean flag);
	
  PBoolean CanFocus() const { return (secondOctet >> 4) & 0x01; }
  void SetCanFocus(PBoolean flag);
	
  void Encode(BYTE *data) const;
  PBoolean Decode(const BYTE *data);
	
protected:
	
  PBoolean isEnabled;
  BYTE firstOctet;
  BYTE secondOctet;
	
};

/** This class implements a default H.281 handler
 */
class OpalH281Handler : public OpalH224Client
{
  PCLASSINFO(OpalH281Handler, PObject);
	
public:
	
  OpalH281Handler();
  ~OpalH281Handler();
	
  enum VideoSource {
    CurrentVideoSource      = 0x00,
    MainCamera              = 0x01,
    AuxiliaryCamera         = 0x02,
    DocumentCamera          = 0x03,
    AuxiliaryDocumentCamera = 0x04,
    VideoPlaybackSource     = 0x05
  };
  
  /**Overriding default OpalH224Client methods */
  virtual BYTE GetClientID() const { return OpalH224Client::H281ClientID; }
  virtual PBoolean HasExtraCapabilities() const { return PTrue; }
  
  /**Process incoming frames. Overrides from OpalH224Client */
  virtual void OnReceivedExtraCapabilities(const BYTE *capabilities, PINDEX size);
  virtual void OnReceivedMessage(const H224_Frame & message);
	
  BYTE GetLocalNumberOfPresets() const { return localNumberOfPresets; }
  void SetLocalNumberOfPresets(BYTE presets) { localNumberOfPresets = presets; }
	
  BYTE GetRemoteNumberOfPresets() const { return remoteNumberOfPresets; }
	
  H281VideoSource & GetLocalVideoSource(VideoSource source);
  H281VideoSource & GetRemoteVideoSource(VideoSource source);
	
  /** Causes the H.281 handler to start the desired action
      The action will continue until StopAction() is called.
   */
  void StartAction(H281_Frame::PanDirection panDirection,
                   H281_Frame::TiltDirection tiltDirection,
                   H281_Frame::ZoomDirection zoomDireciton,
                   H281_Frame::FocusDirection focusDirection);
  
  /** Stops any action currently ongoing
   */
  void StopAction();

  /** Tells the remote side to select the desired video source using the
	  mode specified. Does nothing if either video source or mode aren't
	  available
	*/
  void SelectVideoSource(BYTE videoSourceNumber, H281_Frame::VideoMode videoMode);
  
  /** Tells the remote side to store the current camera settings as a preset
	  with the preset number given
   */
  void StoreAsPreset(BYTE presetNumber);
  
  /** Tells the remote side to activate the given preset
   */
  void ActivatePreset(BYTE presetNumber);
	
  /** Causes the H.281 handler to send its capabilities.
	  Capabilities include the number of available cameras, (default one)
	  the camera abilities (default none) and the number of presets that
      can be stored (default zero)
   */
  void SendExtraCapabilities() const;
	
  /*
   * methods that subclasses can override.
   * The default handler does not implement FECC on the local side.
   * Thus, the default behaviour is to do nothing.
   */
	
  /** Called each time a remote endpoint sends its capability list
   */
  virtual void OnRemoteCapabilitiesUpdated();
	
  /** Indicates to start the action specified
   */
  virtual void OnStartAction(H281_Frame::PanDirection panDirection,
                             H281_Frame::TiltDirection tiltDirection,
                             H281_Frame::ZoomDirection zoomDirection,
                             H281_Frame::FocusDirection focusDirection);
	
  /** Indicates to stop the action stared with OnStartAction()
   */
  virtual void OnStopAction();
	
  /** Indicates to select the desired video source
   */
  virtual void OnSelectVideoSource(BYTE videoSourceNumber, H281_Frame::VideoMode videoMode);
	
  /** Indicates to store the current camera settings as a preset
   */
  virtual void OnStoreAsPreset(BYTE presetNumber);

  /** Indicates to activate the given preset number
   */
  virtual void OnActivatePreset(BYTE presetNumber);
	
protected:
		
  PDECLARE_NOTIFIER(PTimer, OpalH281Handler, ContinueAction);
  PDECLARE_NOTIFIER(PTimer, OpalH281Handler, StopActionLocally);
	
  BYTE localNumberOfPresets;
  BYTE remoteNumberOfPresets;
  H281VideoSource localVideoSources[6];
  H281VideoSource remoteVideoSources[6];
	
  H281_Frame transmitFrame;
  PTimer transmitTimer;

  H281_Frame::PanDirection requestedPanDirection;
  H281_Frame::TiltDirection requestedTiltDirection;
  H281_Frame::ZoomDirection requestedZoomDirection;
  H281_Frame::FocusDirection requestedFocusDirection;
  PTimer receiveTimer;
};

#endif // OPAL_H224_H281HANDLER_H

