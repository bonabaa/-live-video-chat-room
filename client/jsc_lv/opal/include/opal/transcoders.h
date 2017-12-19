/*
 * transcoders.h
 *
 * Abstractions for converting media from one format to another.
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 * Contributor(s): ______________________________________.
 *
 * $Revision: 22444 $
 * $Author: rjongbloed $
 * $Date: 2009-04-20 23:49:06 +0000 (Mon, 20 Apr 2009) $
 */

#ifndef OPAL_OPAL_TRANSCODERS_H
#define OPAL_OPAL_TRANSCODERS_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <opal/mediafmt.h>
#include <opal/mediacmd.h>

#include <rtp/rtp.h>

class RTP_DataFrame;
class OpalTranscoder;


///////////////////////////////////////////////////////////////////////////////

/** This class described an OpalTranscoder in terms of a pair of
    OpalMediaFormat instances, for input and output.
  */
class OpalMediaFormatPair : public PObject
{
    PCLASSINFO(OpalMediaFormatPair, PObject);
  public:
  /**@name Construction */
  //@{
    /** Create a new transcoder implementation.
      */
    OpalMediaFormatPair(
      const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
      const OpalMediaFormat & outputMediaFormat  ///<  Output media format
    );
  //@}

  /**@name Overrides from PObject */
  //@{
    /**Standard stream print function.
       The PObject class has a << operator defined that calls this function
       polymorphically.
      */
    void PrintOn(
      ostream & strm    ///<  Stream to output text representation
    ) const;

    /** Compare the two objects and return their relative rank. This function is
       usually overridden by descendent classes to yield the ranking according
       to the semantics of the object.
       
       The default function is to use the #CompareObjectMemoryDirect()#
       function to do a byte wise memory comparison of the two objects.

       @return
       #LessThan#, #EqualTo# or #GreaterThan#
       according to the relative rank of the objects.
     */
    virtual Comparison Compare(
      const PObject & obj   ///<  Object to compare against.
    ) const;
  //@}

  /**@name Operations */
  //@{
    /**Get the names of the input or output formats.
      */
    const OpalMediaFormat & GetInputFormat() const { return inputMediaFormat; }

    /**Get the names of the input or output formats.
      */
    const OpalMediaFormat & GetOutputFormat() const { return outputMediaFormat; }
  //@}

  protected:
    OpalMediaFormat inputMediaFormat;
    OpalMediaFormat outputMediaFormat;
};


typedef std::pair<PString, PString>                                      OpalTranscoderKey;
typedef PFactory<OpalTranscoder, OpalTranscoderKey>                      OpalTranscoderFactory;
typedef PFactory<OpalTranscoder, OpalTranscoderKey>::KeyList_T           OpalTranscoderList;
typedef PFactory<OpalTranscoder, OpalTranscoderKey>::KeyList_T::iterator OpalTranscoderIterator;

__inline OpalTranscoderKey MakeOpalTranscoderKey(const OpalMediaFormat & from, const OpalMediaFormat & to)
{
  return OpalTranscoderKey(from.GetName(), to.GetName());
}

__inline OpalTranscoderKey MakeOpalTranscoderKey(const char * from, const char * to)
{
  return OpalTranscoderKey(from, to);
}

#define OPAL_REGISTER_TRANSCODER(cls, input, output) \
  PFACTORY_CREATE(OpalTranscoderFactory, cls, MakeOpalTranscoderKey(input, output), false)


/**This class embodies the implementation of a specific transcoder instance
   used to convert data from one format to another.

   An application may create a descendent off this class and override
   functions as required for implementing a transcoder.
 */
class OpalTranscoder : public OpalMediaFormatPair
{
    PCLASSINFO(OpalTranscoder, OpalMediaFormatPair);
  public:
  /**@name Construction */
  //@{
    /** Create a new transcoder implementation.
      */
    OpalTranscoder(
      const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
      const OpalMediaFormat & outputMediaFormat  ///<  Output media format
    );
  //@}

  /**@name Operations */
  //@{
    /**Update the input and output media formats. This can be used to adjust
       the parameters of a codec at run time. Note you cannot change the basic
       media format, eg change GSM0610 to G.711, only options for that
       format, eg 6k3 mode to 5k3 mode in G.723.1. If the formats are
       different then a OpalMediaFormat::Merge() is performed.

       If a format is empty (invalid) it is ignored and does not update the
       internal variable. In this way only the input or output side can be
       updated.

       The default behaviour updates the inputMediaFormat and outputMediaFormat
       member variables.
      */
    virtual bool UpdateMediaFormats(
      const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
      const OpalMediaFormat & outputMediaFormat  ///<  Output media format
    );

    /**Execute the command specified to the transcoder. The commands are
       highly context sensitive, for example VideoFastUpdate would only apply
       to a video transcoder.

       The default behaviour simply returns PFalse.
      */
    virtual PBoolean ExecuteCommand(
      const OpalMediaCommand & command    ///<  Command to execute.
    );

    /**Get the optimal size for data frames to be converted.
       This function returns the size of frames that will be most efficient
       in conversion. A RTP_DataFrame will attempt to provide or use data in
       multiples of this size. Note that it may not do so, so the transcoder
       must be able to handle any sized packets.
      */
    virtual PINDEX GetOptimalDataFrameSize(
      PBoolean input      ///<  Flag for input or output data size
    ) const = 0;

    /**Convert the data from one format to another.
       This function takes the input data as a RTP_DataFrame and converts it
       to its output format, placing it (possibly) into multiple RTP_DataFrame
       objects.

       The default behaviour makes sure the output list has only one element
       in it and calls the Convert() function.

       Returns PFalse if the conversion fails.
      */
    virtual PBoolean ConvertFrames(
      const RTP_DataFrame & input,  ///<  Input data
      RTP_DataFrameList & output    ///<  Output data
    );

    /**Convert the data from one format to another.
       This function takes the input data as a RTP_DataFrame and converts it
       to its output format, placing it into the RTP_DataFrame provided.

       Returns PFalse if the conversion fails.
      */
    virtual PBoolean Convert(
      const RTP_DataFrame & input,  ///<  Input data
      RTP_DataFrame & output        ///<  Output data
    ) = 0;

    /**Create an instance of a media conversion function.
       Returns NULL if there is no registered media transcoder between the two
       named formats.
      */
    static OpalTranscoder * Create(
      const OpalMediaFormat & srcFormat,  ///<  Name of source format
      const OpalMediaFormat & dstFormat,  ///<  Name of destination format
      const BYTE * instance = NULL,       ///<  Unique instance identifier for transcoder
      unsigned instanceLen = 0            ///<  Length of instance identifier
    );

    /**Find media format(s) for transcoders.
       This function attempts to find and intermediate media format that will
       allow two transcoders to be used to get data from the source format to
       the destination format.

       There could be many possible matches between the two lists, so
       preference is given to the order of the destination formats.

       At least one of the two selected medai formats (source or destination)
       must also be of the specified media type ("audio", "video" etc).

       Returns false if there is no registered media transcoder that can be
               used to get from src to dst for the media type.
      */
    static bool SelectFormats(
      const OpalMediaType & mediaType,        ///< Media type for selection.
      const OpalMediaFormatList & srcFormats, ///<  Names of possible source formats
      const OpalMediaFormatList & dstFormats, ///<  Names of possible destination formats
      const OpalMediaFormatList & allFormats, ///<  Master list of formats for merging options
      OpalMediaFormat & srcFormat,            ///<  Selected source format to be used
      OpalMediaFormat & dstFormat             ///<  Selected destination format to be used
    );

    /**Find media intermediate format for transcoders.
       This function attempts to find the intermediate media format that will
       allow two transcoders to be used to get data from the source format to
       the destination format.

       If there is a transcoder that can go directly from the source format to
       the destination format then the function returns true but the
       intermediateFormat parmaeter will be an invlid format.

       Returns false if there is no registered media transcoder that can be used
       between the two named formats.
      */
    static bool FindIntermediateFormat(
      const OpalMediaFormat & srcFormat,    ///<  Selected destination format to be used
      const OpalMediaFormat & dstFormat,    ///<  Selected destination format to be used
      OpalMediaFormat & intermediateFormat  ///<  Intermediate format that can be used
    );

    /**Get a list of possible destination media formats for the destination.
      */
    static OpalMediaFormatList GetDestinationFormats(
      const OpalMediaFormat & srcFormat    ///<  Selected source format
    );

    /**Get a list of possible source media formats for the destination.
      */
    static OpalMediaFormatList GetSourceFormats(
      const OpalMediaFormat & dstFormat    ///<  Selected destination format
    );

    /**Get a list of possible media formats that can do bi-directional media.
      */
    static OpalMediaFormatList GetPossibleFormats(
      const OpalMediaFormatList & formats    ///<  Destination format list
    );
  //@}

  /**@name Operations */
  //@{
    /**Get maximum output size.
      */
    PINDEX GetMaxOutputSize() const { return maxOutputSize; }

    /**Set the maximum output size.
      */
    void SetMaxOutputSize(
      PINDEX size
    ) { maxOutputSize = size; }

    /**Set a notifier to receive commands generated by the transcoder. The
       commands are highly context sensitive, for example VideoFastUpdate
       would only apply to a video transcoder.
      */
    void SetCommandNotifier(
      const PNotifier & notifier    ///<  Command to execute.
    ) { commandNotifier = notifier; }

    /**Get the notifier to receive commands generated by the transcoder. The
       commands are highly context sensitive, for example VideoFastUpdate
       would only apply to a video transcoder.
      */
    const PNotifier & GetCommandNotifier() const { return commandNotifier; }

    /** Set the unique instance identifier for transcoder
      */
    virtual void SetInstanceID(
      const BYTE * instance,              ///<  Unique instance identifier for transcoder
      unsigned instanceLen                ///<  Length of instance identifier
    );

    RTP_DataFrame::PayloadTypes GetPayloadType(
      PBoolean input      ///<  Flag for input or output data size
    ) const;

    virtual bool AcceptComfortNoise() const  { return false; }
    virtual bool AcceptEmptyPayload() const  { return acceptEmptyPayload; }
    virtual bool AcceptOtherPayloads() const { return acceptOtherPayloads; }

#if OPAL_STATISTICS
    virtual void GetStatistics(OpalMediaStatistics & statistics) const;
#endif
  //@}

  protected:
    PINDEX    maxOutputSize;
    PNotifier commandNotifier;
    PMutex    updateMutex;

    PBoolean outputIsRTP, inputIsRTP;
    bool acceptEmptyPayload;
    bool acceptOtherPayloads;
};


/**This class defines a transcoder implementation class that will
   encode/decode fixed sized blocks. That is each input block of n bytes
   is encoded to exactly m bytes of data, eg GSM etc.

   An application may create a descendent off this class and override
   functions as required for descibing a specific transcoder.
 */
class OpalFramedTranscoder : public OpalTranscoder
{
    PCLASSINFO(OpalFramedTranscoder, OpalTranscoder);
  public:
  /**@name Construction */
  //@{
    /** Create a new framed transcoder implementation.
      */
    OpalFramedTranscoder(
      const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
      const OpalMediaFormat & outputMediaFormat, ///<  Output media format
      PINDEX inputBytesPerFrame,  ///<  Number of bytes in an input frame
      PINDEX outputBytesPerFrame  ///<  Number of bytes in an output frame
    );
  //@}

  /**@name Operations */
  //@{
    /**Update the input and output media formats. This can be used to adjust
       the parameters of a codec at run time. Note you cannot change the basic
       media format, eg change GSM0610 to G.711, only options for that
       format, eg 6k3 mode to 5k3 mode in G.723.1. If the formats are
       different then a OpalMediaFormat::Merge() is performed.

       If a format is empty (invalid) it is ignored and does not update the
       internal variable. In this way only the input or output side can be
       updated.

       The default behaviour updates the inputMediaFormat and outputMediaFormat
       member variables.
      */
    virtual bool UpdateMediaFormats(
      const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
      const OpalMediaFormat & outputMediaFormat  ///<  Output media format
    );

    /**Get the optimal size for data frames to be converted.
       This function returns the size of frames that will be most efficient
       in conversion. A RTP_DataFrame will attempt to provide or use data in
       multiples of this size. Note that it may not do so, so the transcoder
       must be able to handle any sized packets.
      */
    virtual PINDEX GetOptimalDataFrameSize(
      PBoolean input      ///<  Flag for input or output data size
    ) const;

    /**Convert the data from one format to another.
       This function takes the input data as a RTP_DataFrame and converts it
       to its output format, placing it into the RTP_DataFrame provided.

       Returns FALSE if the conversion fails.
      */
    virtual PBoolean Convert(
      const RTP_DataFrame & input,  ///<  Input data
      RTP_DataFrame & output        ///<  Output data
    );

    /**Convert a frame of data from one format to another.
       This function implicitly knows the input and output frame sizes.
      */
    virtual PBoolean ConvertFrame(
      const BYTE * input,   ///<  Input data
      BYTE * output         ///<  Output data
    );
    virtual PBoolean ConvertFrame(
      const BYTE * input,   ///<  Input data
      PINDEX & consumed,    ///<  number of input bytes consumed
      BYTE * output,        ///<  Output data
      PINDEX & created      ///<  number of output bytes created
    );
    virtual PBoolean ConvertSilentFrame(
      BYTE * output         ///<  Output data
    );
  //@}

  protected:
    PINDEX     inputBytesPerFrame;
    PINDEX     outputBytesPerFrame;
    PINDEX     maxOutputDataSize;
};


/**This class defines a transcoder implementation class where the encode/decode
   is streamed. That is each input n bit PCM sample is encoded to m bits of
   encoded data, eg G.711 etc.

   An application may create a descendent off this class and override
   functions as required for descibing a specific transcoder.
 */
class OpalStreamedTranscoder : public OpalTranscoder
{
    PCLASSINFO(OpalStreamedTranscoder, OpalTranscoder);
  public:
  /**@name Construction */
  //@{
    /** Create a new streamed transcoder implementation.
      */
    OpalStreamedTranscoder(
      const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
      const OpalMediaFormat & outputMediaFormat, ///<  Output media format
      unsigned inputBits,           ///<  Bits per sample in input data
      unsigned outputBits           ///<  Bits per sample in output data
    );
  //@}

  /**@name Operations */
  //@{
    /**Get the optimal size for data frames to be converted.
       This function returns the size of frames that will be most efficient
       in conversion. A RTP_DataFrame will attempt to provide or use data in
       multiples of this size. Note that it may not do so, so the transcoder
       must be able to handle any sized packets.
      */
    virtual PINDEX GetOptimalDataFrameSize(
      PBoolean input      ///<  Flag for input or output data size
    ) const;

    /**Convert the data from one format to another.
       This function takes the input data as a RTP_DataFrame and converts it
       to its output format, placing it into the RTP_DataFrame provided.

       Returns PFalse if the conversion fails.
      */
    virtual PBoolean Convert(
      const RTP_DataFrame & input,  ///<  Input data
      RTP_DataFrame & output        ///<  Output data
    );

    /**Convert one sample from one format to another.
       This function takes the input data as a single sample value and
       converts it to its output format.

       Returns converted value.
      */
    virtual int ConvertOne(int sample) const = 0;
  //@}

  protected:
    unsigned inputBitsPerSample;
    unsigned outputBitsPerSample;
};


///////////////////////////////////////////////////////////////////////////////

class Opal_Linear16Mono_PCM : public OpalStreamedTranscoder {
  public:
    Opal_Linear16Mono_PCM();
    virtual int ConvertOne(int sample) const;
};


///////////////////////////////////////////////////////////////////////////////

class Opal_PCM_Linear16Mono : public OpalStreamedTranscoder {
  public:
    Opal_PCM_Linear16Mono();
    virtual int ConvertOne(int sample) const;
};


///////////////////////////////////////////////////////////////////////////////

#define OPAL_REGISTER_L16_MONO() \
  OPAL_REGISTER_TRANSCODER(Opal_Linear16Mono_PCM, OpalL16_MONO_8KHZ, OpalPCM16); \
  OPAL_REGISTER_TRANSCODER(Opal_PCM_Linear16Mono, OpalPCM16,         OpalL16_MONO_8KHZ)


class OpalEmptyFramedAudioTranscoder : public OpalFramedTranscoder
{
  PCLASSINFO(OpalEmptyFramedAudioTranscoder, OpalFramedTranscoder);
  public:
    OpalEmptyFramedAudioTranscoder(const char * inFormat, const char * outFormat)
      : OpalFramedTranscoder(inFormat, outFormat, 100, 100)
    {  }

    PBoolean ConvertFrame(const BYTE *, PINDEX &, BYTE *, PINDEX &)
    { return PFalse; }
};

#define OPAL_DECLARE_EMPTY_TRANSCODER(fmt) \
class Opal_Empty_##fmt##_Encoder : public OpalEmptyFramedAudioTranscoder \
{ \
  public: \
    Opal_Empty_##fmt##_Encoder() \
      : OpalEmptyFramedAudioTranscoder(OpalPCM16, fmt) \
    { } \
}; \
class Opal_Empty_##fmt##_Decoder : public OpalEmptyFramedAudioTranscoder \
{ \
  public: \
    Opal_Empty_##fmt##_Decoder() \
      : OpalEmptyFramedAudioTranscoder(fmt, OpalPCM16) \
    { } \
}; \

#define OPAL_DEFINE_EMPTY_TRANSCODER(fmt) \
OPAL_REGISTER_TRANSCODER(Opal_Empty_##fmt##_Encoder, OpalPCM16, fmt); \
OPAL_REGISTER_TRANSCODER(Opal_Empty_##fmt##_Decoder, fmt,       OpalPCM16); \

#endif // OPAL_OPAL_TRANSCODERS_H


// End of File ///////////////////////////////////////////////////////////////
