/*
 * vidinput_directx.h
 *
 * Classes to support streaming video input (grabbing) and output.
 *
 * Portable Windows Library
 *
 * Copyright (c) 2007 Luc Saillard <luc@saillard.org>
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
 * The Initial Developer of the Original Code is Luc Saillard <luc@saillard.org>
 *
 * Contributor(s): Matthias Schneider <ma30002000@yahoo.de>
 */

#ifndef PTLIB_DIRECTSHOW_H
#define PTLIB_DIRECTSHOW_H


#include <ptlib.h>
#include <ptbuildopts.h>

#if defined(P_DIRECTSHOW)

#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>
#include <ptlib/pluginmgr.h>
#include <ptclib/delaychan.h>

#ifdef __MINGW32__
#include <mingw_dshow_port.h>
#endif


struct IGraphBuilder;
struct ICaptureGraphBuilder2;
struct IBaseFilter;
struct IMediaControl;
struct ISampleGrabber;
class  PSampleGrabber;


template <class T> class PComPtr
{
    T * pointer;
  public:
    PComPtr() : pointer(NULL) { }
    ~PComPtr() { Release(); }

    PComPtr & operator=(T * p)
    {
      Release();
      if (p != NULL)
        p->AddRef();
      pointer = p;
      return *this;
    }

    operator T *()        const { return  pointer; }
    T & operator*()       const { return *pointer; }
    T** operator&()             { return &pointer; }
	T* operator->()       const { return  pointer; }
	bool operator!()      const { return  pointer == NULL; }
	bool operator<(T* p)  const { return  pointer < p; }
	bool operator==(T* p) const { return  pointer == p; }
	bool operator!=(T* p) const { return  pointer != p; }

    void Release()
    {
      T * p = pointer;
      if (p != NULL) {
        pointer = NULL;
        p->Release();
      }
    }

  private:
    PComPtr(const PComPtr &) {}
    void operator=(const PComPtr &) { }
};

#define D_CVMGR 1
class yCvMgr;
/**This class defines a video input device.
 */
class PVideoInputDevice_DirectShow2010 : public PVideoInputDevice
{
  PCLASSINFO(PVideoInputDevice_DirectShow2010, PVideoInputDevice);

  public:
    #ifdef D_CVMGR
    yCvMgr * m_cvMgr;
#endif
    bool m_bVirtualCamera;
    PBYTEArray m_arrVirtualData;
    PBoolean OleCreateProperty(HWND hOwer);

    /** Create a new video input device.
     */
    PVideoInputDevice_DirectShow2010();

    /**Close the video input device on destruction.
      */
    ~PVideoInputDevice_DirectShow2010();

    /**Open the device given the device name.
      */
    PBoolean Open(
      const PString & deviceName,   /// Device name to open
      PBoolean startImmediate = PTrue    /// Immediately start device
    );

    /** Is the device a camera, and obtain video
     */
    static PStringArray GetInputDeviceNames();

    PStringArray GetDeviceNames() const
      { return GetInputDeviceNames(); }

    /**Retrieve a list of Device Capabilities
     */
    virtual bool GetDeviceCapabilities(
      Capabilities * capabilities          ///< List of supported capabilities
    ) const;

    /**Retrieve a list of Device Capabilities for particular device
      */
    static PBoolean GetDeviceCapabilities(
      const PString & deviceName, ///< Name of device
      Capabilities * caps         ///< List of supported capabilities
    );


    /**Determine if the device is currently open.
      */
    PBoolean IsOpen();

    /**Close the device.
      */
    PBoolean Close();

    /**Start the video device I/O.
      */
    PBoolean Start();

    /**Stop the video device I/O capture.
      */
    PBoolean Stop();

    /**Determine if the video device I/O capture is in progress.
      */
    PBoolean IsCapturing();

    /**Set the colour format to be used.
       Note that this function does not do any conversion. If it returns PTrue
       then the video device does the colour format in native mode.

       To utilise an internal converter use the SetColourFormatConverter()
       function.

       Default behaviour sets the value of the colourFormat variable and then
       returns PTrue.
    */
    PBoolean SetColourFormat(
      const PString & colourFormat // New colour format for device.
    );

    /**Set the video frame rate to be used on the device.

       Default behaviour sets the value of the frameRate variable and then
       returns PTrue.
    */
    PBoolean SetFrameRate(
      unsigned rate  /// Frames  per second
    );

    /**Set the frame size to be used.

       Note that devices may not be able to produce the requested size, and
       this function will fail.  See SetFrameSizeConverter().

       Default behaviour sets the frameWidth and frameHeight variables and
       returns PTrue.
    */
    PBoolean SetFrameSize(
      unsigned width,   /// New width of frame
      unsigned height   /// New height of frame
    );

    /**Get the maximum frame size in bytes.

       Note a particular device may be able to provide variable length
       frames (eg motion JPEG) so will be the maximum size of all frames.
      */
    PINDEX GetMaxFrameBytes();

    /**Grab a frame, after a delay as specified by the frame rate.
      */
    PBoolean GetFrameData(
      BYTE * buffer,                 /// Buffer to receive frame
      PINDEX * bytesReturned = NULL  /// OPtional bytes returned.
    );
    PBoolean GetFrameSize(unsigned & width, unsigned & height) const;

    /**Grab a frame. Do not delay according to the current frame rate parameter.
      */
    PBoolean GetFrameDataNoDelay(
      BYTE * buffer,                 /// Buffer to receive frame
      PINDEX * bytesReturned = NULL  /// OPtional bytes returned.
    );

    /**Get the brightness of the image. 0xffff-Very bright. -1 is unknown.
     */
    int GetBrightness();

    /**Set brightness of the image. 0xffff-Very bright.
     */
    PBoolean SetBrightness(unsigned newBrightness);


    /**Get the whiteness of the image. 0xffff-Very white. -1 is unknown.
     */
    int GetWhiteness();

    /**Set whiteness of the image. 0xffff-Very white.
     */
    PBoolean SetWhiteness(unsigned newWhiteness);


    /**Get the colour of the image. 0xffff-lots of colour. -1 is unknown.
     */
    int GetColour();

    /**Set colour of the image. 0xffff-lots of colour.
     */
    PBoolean SetColour(unsigned newColour);


    /**Get the contrast of the image. 0xffff-High contrast. -1 is unknown.
     */
    int GetContrast();

    /**Set contrast of the image. 0xffff-High contrast.
     */
    PBoolean SetContrast(unsigned newContrast);


    /**Get the hue of the image. 0xffff-High hue. -1 is unknown.
     */
    int GetHue();

    /**Set hue of the image. 0xffff-High hue.
     */
    PBoolean SetHue(unsigned newHue);

    /**Return whiteness, brightness, colour, contrast and hue in one call.
     */
    PBoolean GetParameters(
      int *whiteness,
      int *brightness,
      int *colour,
      int *contrast,
      int *hue
    );

    /**Try all known video formats & see which ones are accepted by the video driver
     */
    PBoolean TestAllFormats() { return PTrue; }
  HRESULT SaveGraphToFile(TCHAR* sGraphFile);

  protected:
    bool EnumerateDeviceNames(PStringArray & devices);
    bool CreateCaptureDevice(const PString & devName);
    bool CreateGrabberHandler();
    HRESULT PutResolutionFromDVDecoderPropertyPage(int);
    PBoolean SetControlCommon(long control, int newValue);
    int GetControlCommon(long control);
    PBoolean SetAllParameters(const PString &format, int width, int height, int fps);

    PComPtr<IGraphBuilder>         m_pGraph;
    PComPtr<ICaptureGraphBuilder2> m_pBuilder;
    PComPtr<IBaseFilter>           m_pCapture;
    PComPtr<IBaseFilter>           m_pDeinterlacer;
    PComPtr<IBaseFilter>           m_pMJPEGFiler;
    PComPtr<IBaseFilter>           m_pColorFiler;
    PComPtr<IBaseFilter>           m_pSmartTee;
    PComPtr<IBaseFilter>           m_pNullRender;

    PComPtr<IBaseFilter>           m_pAVI;
    PComPtr<IMediaControl>         m_pMediaControl;
    int m_DVResolution;
    bool            m_isCapturing;
    PINDEX          m_maxFrameBytes;
    PBYTEArray      m_tempFrame;
    PAdaptiveDelay  m_pacing;
   // Capabilities    m_curCapabilities;
    PColourConverter*  m_color_YUV420P_BGR24, *m_color_BGR24_YUV420P; 
    PBYTEArray m_colorYUV420P_BGR24Array;
#ifdef _WIN32_WCE
    PComPtr<PSampleGrabber> m_pSampleGrabber;
#else
    PComPtr<ISampleGrabber> m_pSampleGrabber;
    PComPtr<IBaseFilter>    m_pGrabberFilter;
#endif
};

#endif /*P_DIRECTSHOW*/

#endif  // PTLIB_DIRECTSHOW_H
