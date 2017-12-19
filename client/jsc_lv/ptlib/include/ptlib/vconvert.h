/*
 * vconvert.h
 *
 * Classes to support streaming video input (grabbing) and output.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-2000 Equivalence Pty. Ltd.
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
 * Contributor(s): Derek Smithies (derek@indranet.co.nz)
 *                 Thorsten Westheider (thorsten.westheider@teleos-web.de)
 *                 Mark Cooke (mpc@star.sr.bham.ac.uk)
 *
 * $Revision: 22477 $
 * $Author: rjongbloed $
 * $Date: 2009-04-29 06:03:51 +0000 (Wed, 29 Apr 2009) $
 */

#ifndef PTLIB_CONVERT_H
#define PTLIB_CONVERT_H

#ifdef P_USE_PRAGMA
#ifndef P_MACOSX
#pragma interface
#endif
#endif

#include <ptbuildopts.h>

#if P_VIDEO

#include <ptlib/videoio.h>

struct jdec_private;


/**This class registers a colour conversion class.
   There should be one and one only instance of this class for each pair of
   srcColurFormat and dstColourFormat strings. Use the
   PCOLOUR_CONVERTER_REGISTRATION macro to do this.
 */
class PColourConverterRegistration : public PCaselessString
{
    PCLASSINFO(PColourConverterRegistration, PCaselessString);
  public:
    PColourConverterRegistration(
      const PString & srcColourFormat,  ///< Name of source colour format
      const PString & destColourFormat  ///< Name of destination colour format
    );

  protected:
    virtual PColourConverter * Create(
      const PVideoFrameInfo & src, ///< Source frame info (colour formet, size etc)
      const PVideoFrameInfo & dst  ///< Destination frame info
    ) const = 0;

    PColourConverterRegistration * link;

  friend class PColourConverter;
};


/**This class defines a means to convert an image from one colour format to another.
   It is an ancestor class for the individual formatting functions.
 */
class PColourConverter : public PObject
{
    PCLASSINFO(PColourConverter, PObject);
  public:
    /**Create a new converter.
      */
    PColourConverter(
      const PString & srcColourFormat,  ///< Name of source colour format
      const PString & dstColourFormat,  ///< Name of destination colour format
      unsigned width,   ///< Width of frame
      unsigned height   ///< Height of frame
    );
    PColourConverter(
      const PVideoFrameInfo & src, ///< Source frame info (colour formet, size etc)
      const PVideoFrameInfo & dst  ///< Destination frame info
    );

    /**Get the video conversion vertical flip state
     */
    PBoolean GetVFlipState() 
      { return verticalFlip; }
    
    /**Set the video conversion vertical flip state
     */
    void SetVFlipState(PBoolean vFlipState) 
      { verticalFlip = vFlipState; }
    
    /**Set the frame size to be used.

       Default behaviour calls SetSrcFrameSize() and SetDstFrameSize().
    */
    virtual PBoolean SetFrameSize(
      unsigned width,   ///< New width of frame
      unsigned height   ///< New height of frame
    );

    /**Set the source frame info to be used.

       Default behaviour sets the srcFrameWidth and srcFrameHeight variables and
       recalculates the frame buffer size in bytes then returns PTrue if the size
       was calculated correctly.

       Returns PFalse if the colour formats do not agree.
    */
    virtual PBoolean SetSrcFrameInfo(
      const PVideoFrameInfo & info   ///< New info for frame
    );

    /**Set the destination frame info to be used.

       Default behaviour sets the dstFrameWidth and dstFrameHeight variables,
       and the scale / crop preference. It then recalculates the frame buffer
       size in bytes then returns PTrue if the size was calculated correctly.

       Returns PFalse if the colour formats do not agree.
    */
    virtual PBoolean SetDstFrameInfo(
      const PVideoFrameInfo & info  ///< New info for frame
    );

    /**Get the source frame info to be used.
    */
    virtual void GetSrcFrameInfo(
      PVideoFrameInfo & info   ///< New info for frame
    );

    /**Get the destination frame info to be used.
    */
    virtual void GetDstFrameInfo(
      PVideoFrameInfo & info  ///< New info for frame
    );

    /**Set the source frame size to be used.

       Default behaviour sets the srcFrameWidth and srcFrameHeight variables and
       recalculates the frame buffer size in bytes then returns PTrue if the size
       was calculated correctly.
    */
    virtual PBoolean SetSrcFrameSize(
      unsigned width,   ///< New width of frame
      unsigned height   ///< New height of frame
    );

    /**Set the destination frame size to be used.

       Default behaviour sets the dstFrameWidth and dstFrameHeight variables,
       and the scale / crop preference. It then recalculates the frame buffer
       size in bytes then returns PTrue if the size was calculated correctly.
    */
    virtual PBoolean SetDstFrameSize(
      unsigned width,  ///< New width of target frame
      unsigned height ///< New height of target frame
    );
    virtual PBoolean SetDstFrameSize(
      unsigned width,  ///< New width of target frame
      unsigned height, ///< New height of target frame
      PBoolean bScale
    );

    /**Get the source colour format.
      */
    const PString & GetSrcColourFormat() { return srcColourFormat; }

    /**Get the destination colour format.
      */
    const PString & GetDstColourFormat() { return dstColourFormat; }

    /**Get the maximum frame size in bytes for source frames.

       Note a particular device may be able to provide variable length
       frames (eg motion JPEG) so will be the maximum size of all frames.
      */
    PINDEX GetMaxSrcFrameBytes() { return srcFrameBytes; }

    /**Get the maximum frame size in bytes for destination frames.

       Note a particular device may be able to provide variable length
       frames (eg motion JPEG) so will be the maximum size of all frames.
      */
    PINDEX GetMaxDstFrameBytes() { return dstFrameBytes; }


    /**Convert from one colour format to another.
       This version will copy the data from one frame buffer to another.
       An implementation of this function should allow for the case of
       where srcFrameBuffer and dstFrameBuffer are the same, if the conversion
       algorithm allows for that to occur without an intermediate frame store.

       The function should return PFalse if srcFrameBuffer and dstFrameBuffer
       are the same and that form pf conversion is not allowed
    */
    virtual PBoolean Convert(
      const BYTE * srcFrameBuffer,  ///< Frame store for source pixels
      BYTE * dstFrameBuffer,        ///< Frame store for destination pixels
      PINDEX * bytesReturned = NULL ///< Bytes written to dstFrameBuffer
    ) = 0;

    virtual PBoolean Convert(
      const BYTE * srcFrameBuffer,  ///< Frame store for source pixels
      BYTE * dstFrameBuffer,        ///< Frame store for destination pixels
      unsigned int srcFrameBytes,
      PINDEX * bytesReturned = NULL ///< Bytes written to dstFrameBuffer
    ) = 0;

    /**Convert from one colour format to another.
       This version will copy the data from one frame buffer to the same frame
       buffer. Not all conversions can do this so an intermediate store and
       copy may be required. If the noIntermediateFrame parameter is PTrue
       and the conversion cannot be done in place then the function returns
       PFalse. If the in place conversion can be done then that parameter is
       ignored.

       Note that the frame should be large enough to take the destination
       pixels.

       Default behaviour calls Convert() from the frameBuffer to itself, and
       if that returns PFalse then calls it again (provided noIntermediateFrame
       is PFalse) using an intermediate store, copying the intermediate store
       back to the original frame store.
    */
    virtual PBoolean ConvertInPlace(
      BYTE * frameBuffer,               ///< Frame buffer to translate data
      PINDEX * bytesReturned = NULL,    ///< Bytes written to frameBuffer
      PBoolean noIntermediateFrame = PFalse  ///< Flag to use intermediate store
    );


    /**Create an instance of a colour conversion function.
       Returns NULL if there is no registered colour converter between the two
       named formats.
      */
    static PColourConverter * Create(
      const PVideoFrameInfo & src, ///< Source frame info (colour formet, size etc)
      const PVideoFrameInfo & dst  ///< Destination frame info
    );
    static PColourConverter * Create(
      const PString & srcColourFormat,
      const PString & destColourFormat,
      unsigned width,
      unsigned height
    );

    /**Get the output frame size.
      */
    PBoolean GetDstFrameSize(
      unsigned & width, ///< Width of destination frame
      unsigned & height ///< Height of destination frame
    ) const;

    /**Get the input frame size.
      */
    PBoolean GetSrcFrameSize(
      unsigned & width, ///< Width of source frame
      unsigned & height ///< Height of source frame
    ) const;

    unsigned GetSrcFrameWidth()  const { return srcFrameWidth;  }
    unsigned GetSrcFrameHeight() const { return srcFrameHeight; }
    unsigned GetDstFrameWidth()  const { return dstFrameWidth;  }
    unsigned GetDstFrameHeight() const { return dstFrameHeight; }

    /**Set the resize mode to be used.
    */
    void SetResizeMode(
      PVideoFrameInfo::ResizeMode mode
    ) { if (mode < PVideoFrameInfo::eMaxResizeMode) resizeMode = mode; }

    /**Get the resize mode to be used.
    */
    PVideoFrameInfo::ResizeMode GetResizeMode() const { return resizeMode; }

    /**Convert RGB to YUV.
      */
    static void RGBtoYUV(
      unsigned   r, unsigned   g, unsigned   b,
      unsigned & y, unsigned & u, unsigned & v
    );
    static void RGBtoYUV(
      unsigned r, unsigned g, unsigned b,
      BYTE   & y, BYTE   & u, BYTE   & v
    );

    /**Copy a section of the source frame to a section of the destination
       frame with scaling/cropping as required.
      */
    static bool CopyYUV420P(
      unsigned srcX, unsigned srcY, unsigned srcWidth, unsigned srcHeight,
      unsigned srcFrameWidth, unsigned srcFrameHeight, const BYTE * srcYUV,
      unsigned dstX, unsigned dstY, unsigned dstWidth, unsigned dstHeight,
      unsigned dstFrameWidth, unsigned dstFrameHeight, BYTE * dstYUV,
      PVideoFrameInfo::ResizeMode resizeMode
    );

    static bool FillYUV420P(
      unsigned x, unsigned y, int width, int height,
      unsigned frameWidth, unsigned frameHeight, BYTE * yuv,
      unsigned r, unsigned g, unsigned b
    );

  protected:
    PString  srcColourFormat;
    PString  dstColourFormat;
    unsigned srcFrameWidth;
    unsigned srcFrameHeight;
    unsigned srcFrameBytes;

    // Needed for resizing
    unsigned dstFrameWidth;
    unsigned dstFrameHeight;
    unsigned dstFrameBytes;

    PVideoFrameInfo::ResizeMode resizeMode;
     
    PBoolean     verticalFlip;

    PBYTEArray intermediateFrameStore;

#ifndef P_MACOSX
      /* Use by the jpeg decompressor */
    struct jdec_private *jdec;
#endif

  friend class PColourConverterRegistration;
};


/**Declare a colour converter class with Convert() function.
   This should only be used once and at the global scope level for each
   converter. It declares everything needs so only the body of the Convert()
   function need be added.
  */
#define PCOLOUR_CONVERTER2(cls,ancestor,srcFmt,dstFmt) \
class cls : public ancestor { \
  public: \
  cls(const PVideoFrameInfo & src, const PVideoFrameInfo & dst) \
    : ancestor(src, dst) { } \
  virtual PBoolean Convert(const BYTE *, BYTE *, PINDEX * = NULL); \
  virtual PBoolean Convert(const BYTE *, BYTE *, unsigned int , PINDEX * = NULL); \
}; \
static class cls##_Registration : public PColourConverterRegistration { \
  public: cls##_Registration() \
    : PColourConverterRegistration(srcFmt,dstFmt) { } \
  protected: virtual PColourConverter * Create(const PVideoFrameInfo & src, const PVideoFrameInfo & dst) const; \
} p_##cls##_registration_instance; \
PColourConverter * cls##_Registration::Create(const PVideoFrameInfo & src, const PVideoFrameInfo & dst) const \
  { return new cls(src, dst); } \
PBoolean cls::Convert(const BYTE *srcFrameBuffer, BYTE *dstFrameBuffer, unsigned int p_srcFrameBytes, PINDEX * bytesReturned) \
  { srcFrameBytes = p_srcFrameBytes;return Convert(srcFrameBuffer, dstFrameBuffer, bytesReturned); } \
PBoolean cls::Convert(const BYTE *srcFrameBuffer, BYTE *dstFrameBuffer, PINDEX * bytesReturned)


/**Declare a colour converter class with Convert() function.
   This should only be used once and at the global scope level for each
   converter. It declares everything needs so only the body of the Convert()
   function need be added.
  */
#define PCOLOUR_CONVERTER(cls,src,dst) \
        PCOLOUR_CONVERTER2(cls,PColourConverter,src,dst)



/**Define synonym colour format converter.
   This is a class that defines for which no conversion is required between
   the specified colour format names.
  */
class PSynonymColour : public PColourConverter {
  public:
    PSynonymColour(
      const PVideoFrameInfo & src,
      const PVideoFrameInfo & dst
    ) : PColourConverter(src, dst) { }
    virtual PBoolean Convert(const BYTE *, BYTE *, PINDEX * = NULL);
    virtual PBoolean Convert(const BYTE *, BYTE *, unsigned int , PINDEX * = NULL);
};


/**Define synonym colour format registration.
   This is a class that defines for which no conversion is required between
   the specified colour format names.
  */
class PSynonymColourRegistration : public PColourConverterRegistration {
  public:
    PSynonymColourRegistration(
      const char * srcFmt,
      const char * dstFmt
    );

  protected:
    virtual PColourConverter * Create(const PVideoFrameInfo & src, const PVideoFrameInfo & dst) const;
};


/**Define synonym colour format.
   This is a class that defines for which no conversion is required between
   the specified colour format names.
  */
#define PSYNONYM_COLOUR_CONVERTER(from,to) \
  static PSynonymColourRegistration p_##from##_##to##_registration_instance(#from,#to)


#endif // P_VIDEO

#endif // PTLIB_CONVERT_H


// End of file ///////////////////////////////////////////////////////////////
