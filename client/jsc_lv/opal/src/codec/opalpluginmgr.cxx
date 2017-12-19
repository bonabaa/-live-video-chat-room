/*
 * opalplugins.cxx
 *
 * OPAL codec plugins handler
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (C) 2005-2006 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23970 $
 * $Author: rjongbloed $
 * $Date: 2010-01-23 00:28:01 +0000 (Sat, 23 Jan 2010) $
 */

#ifdef __GNUC__
#pragma implementation "opalpluginmgr.h"
#endif

#include <ptlib.h>

#include <opal/buildopts.h>

#include <opal/transcoders.h>
#include <codec/opalpluginmgr.h>
#include <codec/opalplugin.h>
#include <codec/opalwavfile.h>

#if OPAL_H323
#include <h323/h323caps.h>
#include <asn/h245.h>
#include <t38/h323t38.h>
#endif

#if OPAL_VIDEO
#include <ptlib/videoio.h>
#include <codec/vidcodec.h>
#endif
#ifdef _WIN32
#else
extern "C" {
#include <libavcodec/avcodec.h>
	
};
#endif

PFACTORY_CREATE(PFactory<PPluginModuleManager>, OpalPluginCodecManager, "OpalPluginCodecManager", true);


#if OPAL_VIDEO

#if OPAL_H323

static const char * sqcifMPI_tag = PLUGINCODEC_SQCIF_MPI;
static const char * qcifMPI_tag  = PLUGINCODEC_QCIF_MPI;
static const char * cifMPI_tag   = PLUGINCODEC_CIF_MPI;
static const char * cif4MPI_tag  = PLUGINCODEC_CIF4_MPI;
static const char * cif16MPI_tag = PLUGINCODEC_CIF16_MPI;

#define H323CAP_TAG_PREFIX    "h323"

// H.261 only
static const char * h323_stillImageTransmission_tag            = H323CAP_TAG_PREFIX "_stillImageTransmission";

// H.261/H.263 tags
static const char * h323_qcifMPI_tag                           = H323CAP_TAG_PREFIX "_qcifMPI";
static const char * h323_cifMPI_tag                            = H323CAP_TAG_PREFIX "_cifMPI";

// H.263 only
static const char * h323_sqcifMPI_tag                          = H323CAP_TAG_PREFIX "_sqcifMPI";
static const char * h323_cif4MPI_tag                           = H323CAP_TAG_PREFIX "_cif4MPI";
static const char * h323_cif16MPI_tag                          = H323CAP_TAG_PREFIX "_cif16MPI";
static const char * h323_temporalSpatialTradeOffCapability_tag = H323CAP_TAG_PREFIX "_temporalSpatialTradeOffCapability";
static const char * h323_unrestrictedVector_tag                = H323CAP_TAG_PREFIX "_unrestrictedVector";
static const char * h323_arithmeticCoding_tag                  = H323CAP_TAG_PREFIX "_arithmeticCoding";      
static const char * h323_advancedPrediction_tag                = H323CAP_TAG_PREFIX "_advancedPrediction";
static const char * h323_pbFrames_tag                          = H323CAP_TAG_PREFIX "_pbFrames";
static const char * h323_hrdB_tag                              = H323CAP_TAG_PREFIX "_hrdB";
static const char * h323_bppMaxKb_tag                          = H323CAP_TAG_PREFIX "_bppMaxKb";
static const char * h323_errorCompensation_tag                 = H323CAP_TAG_PREFIX "_errorCompensation";

inline static bool IsValidMPI(int mpi)
{
  return mpi > 0 && mpi < PLUGINCODEC_MPI_DISABLED;
}

#endif // OPAL_H323
#endif // OPAL_VIDEO


/////////////////////////////////////////////////////////////////////////////

#ifdef OPAL_PLUGIN_DIR

class OpalPluginLoader : public PProcessStartup
{
  PCLASSINFO(OpalPluginLoader, PProcessStartup);
  public:
    void OnStartup()
    { 
      PPluginManager::AddPluginDirs(OPAL_PLUGIN_DIR);
    }
};

static PFactory<PProcessStartup>::Worker<OpalPluginLoader> opalpluginStartupFactory("OpalPluginLoader", true);

#endif // OPAL_PLUGIN_DIR


/////////////////////////////////////////////////////////////////////////////

template <typename CodecClass>
class OpalFixedCodecFactory : public PFactory<OpalFactoryCodec>
{
  public:
    class Worker : public PFactory<OpalFactoryCodec>::WorkerBase 
    {
      public:
        Worker(const PString & key)
          : PFactory<OpalFactoryCodec>::WorkerBase()
        { PFactory<OpalFactoryCodec>::Register(key, this); }

      protected:
        virtual OpalFactoryCodec * Create(const PString &) const
        { return new CodecClass(); }
    };
};


///////////////////////////////////////////////////////////////////////////////

template <class base>
class OpalPluginMediaOption : public base
{
  public:
    OpalPluginMediaOption(const PluginCodec_Option & descriptor)
      : base(descriptor.m_name, descriptor.m_readOnly != 0)
    {
      if (descriptor.m_merge == PluginCodec_CustomMerge) {
        m_mergeFunction = descriptor.m_mergeFunction;
        m_freeFunction = descriptor.m_freeFunction;
      }
      else {
        m_mergeFunction = NULL;
        m_freeFunction = NULL;
      }
    }

    virtual PObject * Clone() const
    {
      return new OpalPluginMediaOption(*this);
    }
    
    virtual bool Merge(const OpalMediaOption & option)
    {
      if (m_mergeFunction == NULL)
        return base::Merge(option);

      char * result = NULL;
      bool ok = m_mergeFunction(&result, base::AsString(), option.AsString());

      if (ok && result != NULL && base::FromString(result)) {
        PTRACE(4, "OpalPlugin\tChanged media option \"" << base::GetName() << "\" from \"" << *this << "\" to \"" << result << '"');
      }

      if (result != NULL && m_freeFunction != NULL)
        m_freeFunction(result);

      PTRACE_IF(2, !ok, "OpalPlugin\tMerge of media option \"" << base::GetName() << "\" failed.");
      return ok;
    }

  protected:
    PluginCodec_MergeFunction m_mergeFunction;
    PluginCodec_FreeFunction  m_freeFunction;
};


///////////////////////////////////////////////////////////////////////////////

OpalPluginControl::OpalPluginControl(const PluginCodec_Definition * def, const char * name)
  : codecDef(def)
  , fnName(name)
  , controlDef(NULL)
{
  if (codecDef == NULL || codecDef->codecControls == NULL || name == NULL)
    return;

  controlDef = codecDef->codecControls;

  while (controlDef->name != NULL) {
    if (strcasecmp(controlDef->name, name) == 0 && controlDef->control != NULL)
      return;
    controlDef++;
  }

  controlDef = NULL;
}


///////////////////////////////////////////////////////////////////////////////

OpalPluginMediaFormatInternal::OpalPluginMediaFormatInternal(const PluginCodec_Definition * defn)
  : codecDef(defn)
  , getOptionsControl(defn, PLUGINCODEC_CONTROL_GET_CODEC_OPTIONS)
  , freeOptionsControl(defn, PLUGINCODEC_CONTROL_FREE_CODEC_OPTIONS)
  , validForProtocolControl(defn, PLUGINCODEC_CONTROL_VALID_FOR_PROTOCOL)
  , toNormalisedControl    (defn, PLUGINCODEC_CONTROL_TO_NORMALISED_OPTIONS)
  , toCustomisedControl    (defn, PLUGINCODEC_CONTROL_TO_CUSTOMISED_OPTIONS)
{
}


void OpalPluginMediaFormatInternal::SetOldStyleOption(OpalMediaFormatInternal & format, const PString & _key, const PString & _val, const PString & type)
{
  PCaselessString key(_key);
  const char * val = _val;

#if OPAL_VIDEO
#if OPAL_H323
  // Backward compatibility tests
  if (key == h323_qcifMPI_tag)
    key = qcifMPI_tag;
  else if (key == h323_cifMPI_tag)
    key = cifMPI_tag;
  else if (key == h323_sqcifMPI_tag)
    key = sqcifMPI_tag;
  else if (key == h323_cif4MPI_tag)
    key = cif4MPI_tag;
  else if (key == h323_cif16MPI_tag)
    key = cif16MPI_tag;
#endif
#endif

  OpalMediaOption::MergeType op = OpalMediaOption::NoMerge;
  if (val[0] != '\0' && val[1] != '\0') {
    switch (val[0]) {
      case '<':
        op = OpalMediaOption::MinMerge;
        ++val;
        break;
      case '>':
        op = OpalMediaOption::MaxMerge;
        ++val;
        break;
      case '=':
        op = OpalMediaOption::EqualMerge;
        ++val;
        break;
      case '!':
        op = OpalMediaOption::NotEqualMerge;
        ++val;
        break;
      case '*':
        op = OpalMediaOption::AlwaysMerge;
        ++val;
        break;
      default:
        break;
    }
  }

  if (type[0] != '\0') {
    PStringArray tokens = PString(val+1).Tokenise(':', false);
    char ** array = tokens.ToCharArray();
    switch (toupper(type[0])) {
      case 'E':
        PTRACE(5, "OpalPlugin\tAdding enum option '" << key << "' " << tokens.GetSize() << " options");
        format.AddOption(new OpalMediaOptionEnum(key, false, array, tokens.GetSize(), op, tokens.GetStringsIndex(val)), true);
        break;
      case 'B':
        PTRACE(5, "OpalPlugin\tAdding boolean option '" << key << "'=" << val);
        format.AddOption(new OpalMediaOptionBoolean(key, false, op, (val[0] == '1') || (toupper(val[0]) == 'T')), true);
        break;
      case 'R':
        PTRACE(5, "OpalPlugin\tAdding real option '" << key << "'=" << val);
        if (tokens.GetSize() < 2)
          format.AddOption(new OpalMediaOptionReal(key, false, op, PString(val).AsReal()));
        else
          format.AddOption(new OpalMediaOptionReal(key, false, op, PString(val).AsReal(), tokens[0].AsReal(), tokens[1].AsReal()), true);
        break;
      case 'I':
        PTRACE(5, "OpalPlugin\tAdding integer option '" << key << "'=" << val);
        if (tokens.GetSize() < 2)
          format.AddOption(new OpalMediaOptionUnsigned(key, false, op, PString(val).AsUnsigned()), true);
        else
          format.AddOption(new OpalMediaOptionUnsigned(key, false, op, PString(val).AsUnsigned(), tokens[0].AsUnsigned(), tokens[1].AsUnsigned()), true);
        break;
      case 'S':
      default:
        PTRACE(5, "OpalPlugin\tAdding string option '" << key << "'=" << val);
        format.AddOption(new OpalMediaOptionString(key, false, val), true);
        break;
    }
    free(array);
  }
}


void OpalPluginMediaFormatInternal::PopulateOptions(OpalMediaFormatInternal & format)
{
  void ** rawOptions = NULL;
  unsigned int optionsLen = sizeof(rawOptions);
  getOptionsControl.Call(&rawOptions, &optionsLen, (void *)(const char *)format.GetName());
  if (rawOptions != NULL) {
    if (codecDef->version < PLUGIN_CODEC_VERSION_OPTIONS) {
      PTRACE(3, "OpalPlugin\tAdding options to OpalMediaFormat " << format << " using old style method");
      // Old scheme
      char const * const * options = (char const * const *)rawOptions;
      while (options[0] != NULL && options[1] != NULL && options[2] != NULL)  {
        SetOldStyleOption(format, options[0], options[1], options[2]);
        options += 3;
      }
    }
    else {
      // New scheme
      struct PluginCodec_Option const * const * options = (struct PluginCodec_Option const * const *)rawOptions;
      PTRACE(5, "OpalPlugin\tAdding options to OpalMediaFormat " << format << " using new style method");
      while (*options != NULL) {
        struct PluginCodec_Option const * option = *options++;
        OpalMediaOption * newOption;
        switch (option->m_type) {
          case PluginCodec_StringOption :
            newOption = new OpalPluginMediaOption<OpalMediaOptionString>(*option);
            break;
          case PluginCodec_BoolOption :
            newOption = new OpalPluginMediaOption<OpalMediaOptionBoolean>(*option);
            break;
          case PluginCodec_IntegerOption :
            newOption = new OpalPluginMediaOption<OpalMediaOptionUnsigned>(*option);
            ((OpalMediaOptionUnsigned*)newOption)->SetMinimum(PString(option->m_minimum).AsInteger());
            ((OpalMediaOptionUnsigned*)newOption)->SetMaximum(PString(option->m_maximum).AsInteger());
            break;
          case PluginCodec_RealOption :
            newOption = new OpalPluginMediaOption<OpalMediaOptionReal>(*option);
            ((OpalMediaOptionReal*)newOption)->SetMinimum(PString(option->m_minimum).AsReal());
            ((OpalMediaOptionReal*)newOption)->SetMaximum(PString(option->m_maximum).AsReal());
            break;
          case PluginCodec_EnumOption :
            newOption = new OpalPluginMediaOption<OpalMediaOptionEnum>(*option);
            ((OpalMediaOptionEnum*)newOption)->SetEnumerations(PString(option->m_minimum).Tokenise(':'));
            break;
          case PluginCodec_OctetsOption :
            newOption = new OpalPluginMediaOption<OpalMediaOptionOctets>(*option);
            ((OpalMediaOptionOctets*)newOption)->SetBase64(option->m_minimum != NULL); // Use minimum to indicate Base64
            break;
          default : // Huh?
            continue;
        }

        newOption->SetMerge((OpalMediaOption::MergeType)option->m_merge);
        if (option->m_value != NULL && *option->m_value != '\0' && !newOption->FromString(option->m_value)) {
          PTRACE(2, "OpalPlugin\tError converting default value \"" << option->m_value << "\""
                    " in option \"" << option->m_name << "\" in format \"" << format << '"');
        }

#if OPAL_SIP
        newOption->SetFMTPName(option->m_FMTPName);
        newOption->SetFMTPDefault(option->m_FMTPDefault);
#endif // OPAL_SIP

#if OPAL_H323
        OpalMediaOption::H245GenericInfo genericInfo;
        genericInfo.ordinal = option->m_H245Generic&PluginCodec_H245_OrdinalMask;
        if (option->m_H245Generic&PluginCodec_H245_Collapsing)
          genericInfo.mode = OpalMediaOption::H245GenericInfo::Collapsing;
        else if (option->m_H245Generic&PluginCodec_H245_NonCollapsing)
          genericInfo.mode = OpalMediaOption::H245GenericInfo::NonCollapsing;
        else
          genericInfo.mode = OpalMediaOption::H245GenericInfo::None;
        if (option->m_H245Generic&PluginCodec_H245_Unsigned32)
          genericInfo.integerType = OpalMediaOption::H245GenericInfo::Unsigned32;
        else if (option->m_H245Generic&PluginCodec_H245_BooleanArray)
          genericInfo.integerType = OpalMediaOption::H245GenericInfo::BooleanArray;
        else
          genericInfo.integerType = OpalMediaOption::H245GenericInfo::UnsignedInt;
        genericInfo.excludeTCS = (option->m_H245Generic&PluginCodec_H245_TCS) == 0;
        genericInfo.excludeOLC = (option->m_H245Generic&PluginCodec_H245_OLC) == 0;
        genericInfo.excludeReqMode = (option->m_H245Generic&PluginCodec_H245_ReqMode) == 0;
        newOption->SetH245Generic(genericInfo);
#endif // OPAL_H323

        format.AddOption(newOption, true);
      }
    }
    freeOptionsControl.Call(rawOptions, &optionsLen);
  }

#if OPAL_H323
  if (codecDef->h323CapabilityType == PluginCodec_H323Codec_generic && codecDef->h323CapabilityData != NULL) {
    const PluginCodec_H323GenericCodecData * genericData = (const PluginCodec_H323GenericCodecData *)codecDef->h323CapabilityData;
    const PluginCodec_H323GenericParameterDefinition *ptr = genericData->params;
    for (unsigned i = 0; i < genericData->nParameters; i++, ptr++) {
      OpalMediaOption::H245GenericInfo genericInfo;
      genericInfo.ordinal = ptr->id;
      genericInfo.mode = ptr->collapsing ? OpalMediaOption::H245GenericInfo::Collapsing : OpalMediaOption::H245GenericInfo::NonCollapsing;
      genericInfo.excludeTCS = ptr->excludeTCS;
      genericInfo.excludeOLC = ptr->excludeOLC;
      genericInfo.excludeReqMode = ptr->excludeReqMode;
      genericInfo.integerType = OpalMediaOption::H245GenericInfo::UnsignedInt;

      PString name(PString::Printf, "Generic Parameter %u", ptr->id);

      OpalMediaOption * mediaOption;
      switch (ptr->type) {
        case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_logical :
          mediaOption = new OpalMediaOptionBoolean(name, ptr->readOnly, OpalMediaOption::NoMerge, ptr->value.integer != 0);
          break;

        case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_booleanArray :
          genericInfo.integerType = OpalMediaOption::H245GenericInfo::BooleanArray;
          mediaOption = new OpalMediaOptionUnsigned(name, ptr->readOnly, OpalMediaOption::AndMerge, ptr->value.integer, 0, 255);
          break;

        case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_unsigned32Min :
          genericInfo.integerType = OpalMediaOption::H245GenericInfo::Unsigned32;
          // Do next case

        case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_unsignedMin :
          mediaOption = new OpalMediaOptionUnsigned(name, ptr->readOnly, OpalMediaOption::MinMerge, ptr->value.integer);
          break;

        case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_unsigned32Max :
          genericInfo.integerType = OpalMediaOption::H245GenericInfo::Unsigned32;
          // Do next case

        case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_unsignedMax :
          mediaOption = new OpalMediaOptionUnsigned(name, ptr->readOnly, OpalMediaOption::MaxMerge, ptr->value.integer);
          break;

        case PluginCodec_H323GenericParameterDefinition::PluginCodec_GenericParameter_octetString :
          mediaOption = new OpalMediaOptionString(name, ptr->readOnly, ptr->value.octetstring);
          break;

        default :
          mediaOption = NULL;
      }

      if (mediaOption != NULL) {
        mediaOption->SetH245Generic(genericInfo);
        format.AddOption(mediaOption);
      }
    }
  }
#endif // OPAL_H323

//  PStringStream str; format.PrintOptions(str);
//  PTRACE(5, "OpalPlugin\tOpalMediaFormat " << format << " has options\n" << str);
}


bool OpalPluginMediaFormatInternal::AdjustOptions(OpalMediaFormatInternal & fmt, OpalPluginControl & control) const
{
  if (!control.Exists())
    return true;

#if PTRACING
  if (PTrace::CanTrace(5))
    PTRACE(5, "OpalPlugin\t" << control.GetName() << ":\n" << setw(-1) << fmt);
  else
    PTRACE(4, "OpalPlugin\t" << control.GetName() << ": " << fmt);
#endif

  char ** input = fmt.GetOptions().ToCharArray(false);
  char ** output = input;

  bool ok = control.Call(&output, sizeof(output)) != 0;

  if (output != NULL && output != input) {
    for (char ** option = output; *option != NULL; option += 2) {
      PString oldValue;
      if (fmt.GetOptionValue(option[0], oldValue) && oldValue != option[1]) {
        PTRACE(3, "OpalPlugin\t" << control.GetName() << " changed option "
               "\"" << option[0] << "\" from \"" << oldValue << "\" to \"" << option[1] << '"');
        fmt.SetOptionValue(option[0], option[1]);
      }
    }
    freeOptionsControl.Call(output, sizeof(output));
  }

  free(input);

  return ok;
}


bool OpalPluginMediaFormatInternal::IsValidForProtocol(const PString & _protocol) const
{
  PString protocol(_protocol.ToLower());

  if (validForProtocolControl.Exists())
    return validForProtocolControl.Call((void *)(const char *)protocol, sizeof(const char *)) != 0;

  if (protocol == "h.323" || protocol == "h323")
    return (codecDef->h323CapabilityType != PluginCodec_H323Codec_undefined) &&
           (codecDef->h323CapabilityType != PluginCodec_H323Codec_NoH323);

  if (protocol == "sip") 
    return codecDef->sdpFormat != NULL;

  return false;
}


///////////////////////////////////////////////////////////////////////////////

static RTP_DataFrame::PayloadTypes GetPluginPayloadType(const PluginCodec_Definition * codecDefn)
{
  if ((codecDefn->flags & PluginCodec_RTPTypeMask) == PluginCodec_RTPTypeDynamic)
    return RTP_DataFrame::DynamicBase;

  return (RTP_DataFrame::PayloadTypes)codecDefn->rtpPayload;
}


///////////////////////////////////////////////////////////////////////////////

OpalPluginAudioFormatInternal::OpalPluginAudioFormatInternal(const PluginCodec_Definition * codecDefn,
                                                             const char * fmtName,
                                                             const char * rtpEncodingName, /// rtp encoding name
                                                             unsigned frameTime,           /// Time for frame in RTP units (if applicable)
                                                             unsigned /*timeUnits*/,       /// RTP units for frameTime (if applicable)
                                                             time_t timeStamp              /// timestamp (for versioning)
                                                            )
  : OpalAudioFormatInternal(fmtName,
                            GetPluginPayloadType(codecDefn),
                            rtpEncodingName,
                            codecDefn->parm.audio.bytesPerFrame,
                            frameTime,
                            codecDefn->parm.audio.maxFramesPerPacket,
                            codecDefn->parm.audio.recommendedFramesPerPacket,
                            codecDefn->parm.audio.maxFramesPerPacket,
                            codecDefn->sampleRate,
                            timeStamp)
  , OpalPluginMediaFormatInternal(codecDefn)
{
  PopulateOptions(*this);

  // Override calculated value if we have an explicit bit rate
  if (codecDefn->bitsPerSec > 0)
    SetOptionInteger(OpalMediaFormat::MaxBitRateOption(), codecDefn->bitsPerSec);
}


bool OpalPluginAudioFormatInternal::IsValidForProtocol(const PString & protocol) const
{
  return OpalPluginMediaFormatInternal::IsValidForProtocol(protocol);
}


PObject * OpalPluginAudioFormatInternal::Clone() const
{
  return new OpalPluginAudioFormatInternal(*this);
}


bool OpalPluginAudioFormatInternal::ToNormalisedOptions()
{
  return AdjustOptions(*this, toNormalisedControl);
}


bool OpalPluginAudioFormatInternal::ToCustomisedOptions()
{
  return AdjustOptions(*this, toCustomisedControl);
}


///////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO

OpalPluginVideoFormatInternal::OpalPluginVideoFormatInternal(const PluginCodec_Definition * codecDefn,
                                                             const char * fmtName,
                                                             const char * rtpEncodingName,
                                                             time_t timeStamp)
  : OpalVideoFormatInternal(fmtName,
                            GetPluginPayloadType(codecDefn),
                            rtpEncodingName,
                            codecDefn->parm.video.maxFrameWidth,
                            codecDefn->parm.video.maxFrameHeight,
                            codecDefn->parm.video.maxFrameRate,
                            codecDefn->bitsPerSec,
                            timeStamp)
  , OpalPluginMediaFormatInternal(codecDefn)
{
  PopulateOptions(*this);
}


PObject * OpalPluginVideoFormatInternal::Clone() const
{ 
  return new OpalPluginVideoFormatInternal(*this); 
}


bool OpalPluginVideoFormatInternal::IsValidForProtocol(const PString & protocol) const
{
  return OpalPluginMediaFormatInternal::IsValidForProtocol(protocol);
}


bool OpalPluginVideoFormatInternal::ToNormalisedOptions()
{
  return AdjustOptions(*this, toNormalisedControl);
}


bool OpalPluginVideoFormatInternal::ToCustomisedOptions()
{
  return AdjustOptions(*this, toCustomisedControl);
}


#endif // OPAL_VIDEO

//////////////////////////////////////////////////////////////////////////////

OpalPluginTranscoder::OpalPluginTranscoder(const PluginCodec_Definition * defn, bool isEnc)
  : codecDef(defn)
  , isEncoder(isEnc)
  , setCodecOptions(defn, PLUGINCODEC_CONTROL_SET_CODEC_OPTIONS)
  , getOutputDataSizeControl(defn, PLUGINCODEC_CONTROL_GET_OUTPUT_DATA_SIZE)
{
  if (codecDef->createCodec == NULL)
    context = NULL;
  else {
    context = (*codecDef->createCodec)(codecDef);
    PTRACE_IF(1, context == NULL, "OpalPlugin\tFailed to create context for \"" << codecDef->descr << '"');
  }
}


OpalPluginTranscoder::~OpalPluginTranscoder()
{
  if (codecDef != NULL && codecDef->destroyCodec != NULL)
    (*codecDef->destroyCodec)(codecDef, context);
}


bool OpalPluginTranscoder::UpdateOptions(const OpalMediaFormat & fmt)
{
  PTRACE(4, "OpalPlugin\t" << (isEncoder ? "Setting encoder options" : "Setting decoder options") << ":\n" << setw(-1) << fmt);

  char ** options = fmt.GetOptions().ToCharArray(false);
  bool ok = setCodecOptions.Call(options, sizeof(options), context) != 0;
  free(options);
  return ok;
}


bool OpalPluginTranscoder::ExecuteCommand(const OpalMediaCommand & command)
{
  OpalPluginControl cmd(codecDef, command.GetName());
  return cmd.Call(command.GetPlugInData(), command.GetPlugInSize(), context) > 0;
}


//////////////////////////////////////////////////////////////////////////////
//
// Plugin framed audio codec classes
//

static OpalMediaFormat GetRawPCM(const char * fmtName, unsigned sampleRate)
{
  if (strcmp(fmtName, "L16") != 0)
    return fmtName;

  switch (sampleRate) {
    default :
    case 8000 :
      return OpalPCM16;
    case 16000 :
      return OpalPCM16_16KHZ;
    case 32000 :
      return OpalPCM16_32KHZ;
    case 48000 :
      return OpalPCM16_48KHZ;
  }
}


OpalPluginFramedAudioTranscoder::OpalPluginFramedAudioTranscoder(const PluginCodec_Definition * codecDefn, bool isEncoder)
  : OpalFramedTranscoder(GetRawPCM(codecDefn->sourceFormat, codecDefn->sampleRate),
                         GetRawPCM(codecDefn->destFormat,   codecDefn->sampleRate),
                         isEncoder ? codecDefn->parm.audio.samplesPerFrame*2 : codecDefn->parm.audio.bytesPerFrame,
                         isEncoder ? codecDefn->parm.audio.bytesPerFrame     : codecDefn->parm.audio.samplesPerFrame*2)
  , OpalPluginTranscoder(codecDefn, isEncoder)
{ 
  inputIsRTP          = (codecDef->flags & PluginCodec_InputTypeMask)  == PluginCodec_InputTypeRTP;
  outputIsRTP         = (codecDef->flags & PluginCodec_OutputTypeMask) == PluginCodec_OutputTypeRTP;
  comfortNoise        = (codecDef->flags & PluginCodec_ComfortNoiseMask) == PluginCodec_ComfortNoise;
  acceptEmptyPayload  = (codecDef->flags & PluginCodec_EmptyPayloadMask) == PluginCodec_EmptyPayload;
  acceptOtherPayloads = (codecDef->flags & PluginCodec_OtherPayloadMask) == PluginCodec_OtherPayload;
}


PBoolean OpalPluginFramedAudioTranscoder::UpdateMediaFormats(const OpalMediaFormat & input, const OpalMediaFormat & output)
{
  return OpalFramedTranscoder::UpdateMediaFormats(input, output) && UpdateOptions(isEncoder ? output : input);
}


PBoolean OpalPluginFramedAudioTranscoder::ExecuteCommand(const OpalMediaCommand & command)
{
  return OpalPluginTranscoder::ExecuteCommand(command);
}


PBoolean OpalPluginFramedAudioTranscoder::ConvertFrame(const BYTE * input,
                                                   PINDEX & consumed,
                                                   BYTE * output,
                                                   PINDEX & created)
{
  unsigned int fromLen = consumed;
  unsigned int toLen   = created;
  unsigned flags = 0;

  bool stat = Transcode(input, &fromLen, output, &toLen, &flags);
  consumed = fromLen;
  created  = toLen;

  return stat;
}

PBoolean OpalPluginFramedAudioTranscoder::ConvertSilentFrame(BYTE * buffer)
{ 
  if (codecDef == NULL)
    return false;

  unsigned length;

  // for a decoder, this mean that we need to create a silence frame
  // which is easy - ask the decoder, or just create silence
  if (!isEncoder) {
    if ((codecDef->flags & PluginCodec_DecodeSilence) == 0) {
      memset(buffer, 0, outputBytesPerFrame); 
      return true;
    }
  }

  // for an encoder, we encode silence but set the flag so it can do something special if need be
  else {
    length = codecDef->parm.audio.bytesPerFrame;
    if ((codecDef->flags & PluginCodec_EncodeSilence) == 0) {
      void * silence = alloca(inputBytesPerFrame);
      memset(silence, 0, inputBytesPerFrame);
      unsigned silenceLen = inputBytesPerFrame;
      unsigned flags = 0;
      return Transcode(silence, &silenceLen, buffer, &length, &flags);
    }
  }

  unsigned flags = PluginCodec_CoderSilenceFrame;
  return Transcode(NULL, NULL, buffer, &length, &flags);
}


//////////////////////////////////////////////////////////////////////////////
//
// Plugin streamed audio codec classes
//

OpalPluginStreamedAudioTranscoder::OpalPluginStreamedAudioTranscoder(const PluginCodec_Definition * codecDefn,
                                                                     bool isEncoder)
  : OpalStreamedTranscoder(GetRawPCM(codecDefn->sourceFormat, codecDefn->sampleRate),
                           GetRawPCM(codecDefn->destFormat,   codecDefn->sampleRate),
                           16, 16)
  , OpalPluginTranscoder(codecDefn, isEncoder)
{
  (isEncoder ? outputBitsPerSample : inputBitsPerSample) =
                  (codecDefn->flags & PluginCodec_BitsPerSampleMask) >> PluginCodec_BitsPerSamplePos;
  comfortNoise       = (codecDef->flags & PluginCodec_ComfortNoiseMask) == PluginCodec_ComfortNoise;
  acceptEmptyPayload = (codecDef->flags & PluginCodec_EmptyPayloadMask) == PluginCodec_EmptyPayload;
  acceptOtherPayloads = (codecDef->flags & PluginCodec_OtherPayloadMask) == PluginCodec_OtherPayload;
}


PBoolean OpalPluginStreamedAudioTranscoder::UpdateMediaFormats(const OpalMediaFormat & input, const OpalMediaFormat & output)
{
  return OpalStreamedTranscoder::UpdateMediaFormats(input, output) && UpdateOptions(isEncoder ? output : input);
}


PBoolean OpalPluginStreamedAudioTranscoder::ExecuteCommand(const OpalMediaCommand & command)
{
  return OpalPluginTranscoder::ExecuteCommand(command);
}


int OpalPluginStreamedAudioTranscoder::ConvertOne(int from) const
{
  unsigned int fromLen = sizeof(from);
  int to;
  unsigned toLen = sizeof(to);
  unsigned flags = 0;
  return Transcode(&from, &fromLen, &to, &toLen, &flags) ? to : -1;
}


#if OPAL_VIDEO

/////////////////////////////////////////////////////////////////////////////

OpalPluginVideoTranscoder::OpalPluginVideoTranscoder(const PluginCodec_Definition * codecDefn, bool isEncoder)
  : OpalVideoTranscoder(codecDefn->sourceFormat, codecDefn->destFormat)
  , OpalPluginTranscoder(codecDefn, isEncoder)
  , m_bufferRTP(NULL)
  , m_lastVideoFastUpdate(PTimer::Tick())
#if PTRACING
  , m_consecutiveIntraFrames(0)
#endif
{ 
  acceptEmptyPayload = (codecDef->flags & PluginCodec_EmptyPayloadMask) == PluginCodec_EmptyPayload;
  acceptOtherPayloads = (codecDef->flags & PluginCodec_OtherPayloadMask) == PluginCodec_OtherPayload;
}

OpalPluginVideoTranscoder::~OpalPluginVideoTranscoder()
{ 
  delete m_bufferRTP;
}


PBoolean OpalPluginVideoTranscoder::UpdateMediaFormats(const OpalMediaFormat & input, const OpalMediaFormat & output)
{
  return OpalVideoTranscoder::UpdateMediaFormats(input, output) && UpdateOptions(isEncoder ? output : input);
}


PBoolean OpalPluginVideoTranscoder::ExecuteCommand(const OpalMediaCommand & command)
{
  return OpalPluginTranscoder::ExecuteCommand(command);
}


PBoolean OpalPluginVideoTranscoder::ConvertFrames(const RTP_DataFrame & src, RTP_DataFrameList & dstList)
{
  return isEncoder ? EncodeFrames(src, dstList) : DecodeFrames(src, dstList);
}


bool OpalPluginVideoTranscoder::EncodeFrames(const RTP_DataFrame & src, RTP_DataFrameList & dstList)
{
  dstList.RemoveAll();

  if (src.GetPayloadSize() == 0)
    return true;

  // get the size of the output buffer
  int outputDataSize = getOutputDataSizeControl.Call((void *)NULL, (unsigned *)NULL, context);
  int optimalDataSize = GetOptimalDataFrameSize(false);
  if (outputDataSize < optimalDataSize)
    outputDataSize = optimalDataSize;

  PINDEX maxEncoderSize = GetMaxEncoderOutputSize();
  if (outputDataSize > maxEncoderSize)
    outputDataSize = maxEncoderSize;

  unsigned flags;

  do {
    // Some plug ins a very rude and use more memory than we say they can, so add an extra 1k
    RTP_DataFrame * dst = new RTP_DataFrame(outputDataSize, outputDataSize+1024);
    dst->SetPayloadType(GetPayloadType(false));
    dst->SetTimestamp(src.GetTimestamp());

    // call the codec function
    unsigned int fromLen = src.GetHeaderSize() + src.GetPayloadSize();
    unsigned int toLen = outputDataSize+1024;
    flags = forceIFrame ? PluginCodec_CoderForceIFrame : 0;

    if (!Transcode((const BYTE *)src, &fromLen, dst->GetPointer(), &toLen, &flags)) {
      delete dst;
      return false;
    }

    lastFrameWasIFrame = (flags & PluginCodec_ReturnCoderIFrame) != 0;

    if (toLen < RTP_DataFrame::MinHeaderSize || (PINDEX)toLen < dst->GetHeaderSize())
      delete dst;
    else {
      dst->SetPayloadSize(toLen - dst->GetHeaderSize());
      dstList.Append(dst);
    }
		m_totalBytes+= toLen;//statistic the byte/sec

  } while ((flags & PluginCodec_ReturnCoderLastFrame) == 0);
 // PTRACE(5, "OpalPlugin\tEncoded video frame into " << dstList.GetSize() << " packets.");

		m_totalFrames++;
	
		PTime nowEnd;
		if ((nowEnd - m_nowLast).GetMilliSeconds() >1000)
		{
			m_nowLast = nowEnd;
#ifdef _DEBUG
			PTRACE (3, "Transcoder\t Encoded FPS =" << m_totalFrames - m_LasttotalFrames  <<"fps");
#endif
			m_totalFramesPerSec =  m_totalFrames - m_LasttotalFrames;
			m_totalBytesPerSec =  m_totalBytes - m_LasttotalBytes;
			m_LasttotalFrames = m_totalFrames;
			m_LasttotalBytes= m_totalBytes;
    }
  if (lastFrameWasIFrame)
    m_keyFrames++;


#if PTRACING
  if (!lastFrameWasIFrame)
    m_consecutiveIntraFrames = 0;
  else if (forceIFrame)
    PTRACE(3, "OpalPlugin\tEncoder sent forced I-Frame at " << m_totalFrames);
  else {
    if (m_consecutiveIntraFrames == 0) 
      PTRACE(4, "OpalPlugin\tEncoder sending I-Frame at " << m_totalFrames);
    else
      PTRACE(4, "OpalPlugin\tEncoder sending " << ((m_consecutiveIntraFrames == 0) ? "" : "consecutive ") << "I-Frame at " << m_totalFrames);
    if (++m_consecutiveIntraFrames >= 10) {
      PTRACE(3, "OpalPlugin\tEncoder has sent too many consecutive I-Frames - assume codec cannot do P-Frames");
    }
  }
#endif

  if (lastFrameWasIFrame)
    forceIFrame = false;

  return true;
}


bool OpalPluginVideoTranscoder::DecodeFrames(const RTP_DataFrame & src, RTP_DataFrameList & dstList)
{
  // We use the data size indicated by plug in as a payload size, we do not adjust the size
  // downward as many plug ins forget to add the RTP header size in its output data size and
  // it doesn't hurt to make this buffer an extra few bytes longer than needed.

  int outputDataSize = getOutputDataSizeControl.Call((void *)NULL, (unsigned *)NULL, context);
	outputDataSize = 1600*1600*3/2;
  if (outputDataSize <= 0)
    outputDataSize = GetOptimalDataFrameSize(false); // Fail safe for badly behaved plug in
  outputDataSize += sizeof(PluginCodec_Video_FrameHeader);

  if (m_bufferRTP == NULL) {
    if (dstList.IsEmpty())
      m_bufferRTP = new RTP_DataFrame(outputDataSize);
    else {
      // Re-use the previously allocated output frame. As video frames can be large
      // when the heap gets a bit fragmented it slows the system down substantially
      // searching for a large enough free memory block, so as we don't have to make
      // a new one every time, let's not.
      dstList.DisallowDeleteObjects();
      m_bufferRTP = (RTP_DataFrame *)dstList.RemoveAt(0);
      dstList.AllowDeleteObjects();
    }

    lastFrameWasIFrame = false;
  }

  dstList.RemoveAll();
	
  m_bufferRTP->SetPayloadSize(outputDataSize);

  // call the codec function
  unsigned fromLen = src.GetHeaderSize() + src.GetPayloadSize();
  unsigned toLen = m_bufferRTP->GetSize();
  unsigned flags = 0;
	m_totalBytes+= fromLen;//statistic the byte/sec
		PTime nowEnd;
		if ((nowEnd - m_nowLast).GetMilliSeconds() >1000)
		{
			m_nowLast = nowEnd;
#ifdef _DEBUG
			PTRACE (3, "Transcoder\t Decoded FPS =" << m_totalFrames - m_LasttotalFrames  <<"fps");
#endif
			m_totalFramesPerSec =  m_totalFrames - m_LasttotalFrames;
			m_totalBytesPerSec =  m_totalBytes - m_LasttotalBytes;
			m_LasttotalFrames = m_totalFrames;
			m_LasttotalBytes= m_totalBytes;
    }
  if (!Transcode((const BYTE *)src, &fromLen, m_bufferRTP->GetPointer(), &toLen, &flags))
	{
      dstList.RemoveAll();
      PTRACE (3, "Transcoder\tCould not Transcode frame, .");//return PFalse;
      //return PTrue;
	}
  if ((flags & PluginCodec_ReturnCoderBufferTooSmall) != 0) {
    m_bufferRTP->SetPayloadSize(getOutputDataSizeControl.Call((void *)NULL, (unsigned *)NULL, context));

    fromLen = src.GetHeaderSize() + src.GetPayloadSize();
    toLen = m_bufferRTP->GetSize();
    flags = 0;

    if (!Transcode((const BYTE *)src, &fromLen, m_bufferRTP->GetPointer(), &toLen, &flags))
		{
      dstList.RemoveAll();
      PTRACE (3, "Transcoder\tCould not Transcode frame, .");//return PFalse;
     // return PTrue;
		}
    if ((flags & PluginCodec_ReturnCoderBufferTooSmall) != 0) {
      PTRACE(1, "OpalPlugin\tNew output buffer size requested and allocated, still not big enough, error in plug in.");
      return false;
    }
  }

  if ((commandNotifier != PNotifier()) && (flags & PluginCodec_ReturnCoderRequestIFrame) != 0) {
    PTimeInterval tick = PTimer::Tick();
    // Don't send lots of consecutive VideoFastUpdate commands
    if (tick - m_lastVideoFastUpdate < 2000)
      PTRACE(4, "OpalPlugin\tCould not decode frame, but a recent VideoUpdatePicture was sent.");
    else {
      m_lastVideoFastUpdate = PTimer::Tick();
      OpalVideoUpdatePicture2 updatePictureCommand(src.GetSequenceNumber(), src.GetTimestamp());
      commandNotifier(updatePictureCommand, 0);
      PTRACE(3, "OpalPlugin\tCould not decode frame, sending VideoUpdatePicture in hope of an I-Frame.");
     
    }
     dstList.RemoveAll();//chenyuan if require IFrame, we do not display the pic
     return true;
  }

  if ((flags & PluginCodec_ReturnCoderIFrame) != 0)
    lastFrameWasIFrame = true;

  if (toLen > RTP_DataFrame::MinHeaderSize && (flags & PluginCodec_ReturnCoderLastFrame) != 0) {
    // Do sanity check on returned data.
    PINDEX headerSize = m_bufferRTP->GetHeaderSize();
    if (PAssert(toLen > (unsigned)(headerSize+sizeof(OpalVideoTranscoder::FrameHeader)),
                "Invalid return size from video plug in")) {
      toLen -= headerSize;
      OpalVideoTranscoder::FrameHeader * videoHeader = (OpalVideoTranscoder::FrameHeader *)m_bufferRTP->GetPayloadPtr();
      if (PAssert(videoHeader->x == 0 && videoHeader->y == 0 &&
                  videoHeader->width < 10000 && videoHeader->height < 10000 &&
                  toLen >= (videoHeader->width*videoHeader->height*3/2+sizeof(OpalVideoTranscoder::FrameHeader)),
                  "Invalid frame returned from video plug in")) {
        m_bufferRTP->SetPayloadSize(toLen);
        m_bufferRTP->SetTimestamp(src.GetTimestamp());
        m_bufferRTP->SetPayloadType(GetPayloadType(false));

        dstList.Append(m_bufferRTP);
        m_bufferRTP = NULL;

        m_totalFrames++;

        if ((flags & PluginCodec_ReturnCoderIFrame) != 0) {
          m_keyFrames++;
          PTRACE(5, "OpalPlugin\tVideo decoder returned I-frame");
        }
      }
    }
  }

  return true;
};


#endif // OPAL_VIDEO



//////////////////////////////////////////////////////////////////////////////
//
// Fax transcoder classes
//

#if OPAL_T38_CAPABILITY

OpalPluginFaxFormatInternal::OpalPluginFaxFormatInternal(const PluginCodec_Definition * codecDefn,
                                                         const char * fmtName,
                                                         const char * rtpEncodingName,
                                                         unsigned frameTime,
                                                         unsigned /*timeUnits*/,
                                                         time_t timeStamp)
  : OpalMediaFormatInternal(fmtName,
                            "fax",
                            GetPluginPayloadType(codecDefn),
                            rtpEncodingName,
                            false,                                // need jitter
                            8*codecDefn->parm.audio.bytesPerFrame*OpalMediaFormat::AudioClockRate/frameTime, // bandwidth
                            codecDefn->parm.audio.bytesPerFrame,         // size of frame in bytes
                            frameTime,                            // time for frame
                            codecDefn->sampleRate,            // clock rate
                            timeStamp)
  , OpalPluginMediaFormatInternal(codecDefn)
{
  PopulateOptions(*this);
}

PObject * OpalPluginFaxFormatInternal::Clone() const
{
  return new OpalPluginFaxFormatInternal(*this);
}

bool OpalPluginFaxFormatInternal::IsValidForProtocol(const PString & protocol) const
{
  return OpalPluginMediaFormatInternal::IsValidForProtocol(protocol);
}


static bool ExtractValue(const PString & msg, PINDEX & position, int & value, char sep = '=')
{
  position = msg.Find(sep, position);
  if (position == P_MAX_INDEX)
    return false;

  value = msg.Mid(++position).AsInteger();
  return true;
}


class OpalFaxTranscoder : public OpalTranscoder, public OpalPluginTranscoder
{
  PCLASSINFO(OpalFaxTranscoder, OpalTranscoder);
  protected:
    RTP_DataFrame * bufferRTP;
    OpalPluginControl getCodecStatistics;

  public:
    OpalFaxTranscoder(const PluginCodec_Definition * codecDefn, bool isEncoder)
      : OpalTranscoder(GetRawPCM(codecDefn->sourceFormat, codecDefn->sampleRate),
                       GetRawPCM(codecDefn->destFormat,   codecDefn->sampleRate))
      , OpalPluginTranscoder(codecDefn, isEncoder)
      , getCodecStatistics(codecDefn, PLUGINCODEC_CONTROL_GET_STATISTICS)
    { 
      bufferRTP = NULL;

      inputIsRTP          = (codecDef->flags & PluginCodec_InputTypeMask)  == PluginCodec_InputTypeRTP;
      outputIsRTP         = (codecDef->flags & PluginCodec_OutputTypeMask) == PluginCodec_OutputTypeRTP;
      acceptEmptyPayload  = (codecDef->flags & PluginCodec_EmptyPayloadMask) == PluginCodec_EmptyPayload;
      acceptOtherPayloads = (codecDef->flags & PluginCodec_OtherPayloadMask) == PluginCodec_OtherPayload;
    }

    ~OpalFaxTranscoder()
    { 
      delete bufferRTP;
    }

    virtual void SetInstanceID(const BYTE * instance, unsigned instanceLen)
    {
      if (instance != NULL && instanceLen > 0) {
        OpalPluginControl ctl(codecDef, PLUGINCODEC_CONTROL_SET_INSTANCE_ID);
        ctl.Call((void *)instance, instanceLen, context);
      }
    }

    virtual PINDEX GetOptimalDataFrameSize(PBoolean input) const
    {
      const OpalMediaFormat & fmt = (input ? inputMediaFormat : outputMediaFormat);
      if (fmt == OpalPCM16)
        return 320; // 20ms of data

      return fmt.GetFrameSize();
    }

    PBoolean UpdateMediaFormats(const OpalMediaFormat & input, const OpalMediaFormat & output)
    {
      return OpalTranscoder::UpdateMediaFormats(input, output) && UpdateOptions(input) && UpdateOptions(output);
    }

    virtual PBoolean ExecuteCommand(const OpalMediaCommand & command)
    {
      return OpalPluginTranscoder::ExecuteCommand(command);
    }

    virtual bool AcceptComfortNoise() const
    {
      return true;
    }

    virtual PBoolean ConvertFrames(const RTP_DataFrame & src, RTP_DataFrameList & dstList)
    {
      dstList.RemoveAll();

      // get the size of the output buffer
      int outputDataSize = GetOptimalDataFrameSize(true);
      unsigned flags = 0;

      const void * fromPtr;
      unsigned fromLen;
      if (inputIsRTP) {
        fromPtr = (const BYTE *)src;
        fromLen = src.GetHeaderSize() + src.GetPayloadSize();
      }
      else {
        fromPtr = src.GetPayloadPtr();
        fromLen = src.GetPayloadSize();
      }

      do {
        if (bufferRTP == NULL)
          bufferRTP = new RTP_DataFrame(outputDataSize);
        else
          bufferRTP->SetPayloadSize(outputDataSize);
        bufferRTP->SetPayloadType(GetPayloadType(false));

        // call the codec function
        void * toPtr;
        unsigned toLen;
        if (outputIsRTP) {
          toPtr = bufferRTP->GetPointer();
          toLen = bufferRTP->GetSize();
        }
        else {
          toPtr = bufferRTP->GetPayloadPtr();
          toLen = bufferRTP->GetSize() - bufferRTP->GetHeaderSize();
        }

        flags = 0;
        if (!Transcode(fromPtr, &fromLen, toPtr, &toLen, &flags))
          return false;

        unsigned hdrSize = outputIsRTP ? (unsigned)bufferRTP->GetHeaderSize() : 0;
        if (toLen > hdrSize) {
          bufferRTP->SetPayloadSize(toLen - hdrSize);

          // set the output timestamp
          unsigned timestamp = src.GetTimestamp();
          unsigned inClockRate = inputMediaFormat.GetClockRate();
          unsigned outClockRate = outputMediaFormat.GetClockRate();

          if (inClockRate != outClockRate)
            timestamp = (unsigned)((PUInt64)timestamp*outClockRate/inClockRate);
          bufferRTP->SetTimestamp(timestamp);

          dstList.Append(bufferRTP);
          bufferRTP = NULL;
        }

        fromLen = 0;

      } while ((flags & PluginCodec_ReturnCoderLastFrame) == 0);

      return true;
    }

    virtual PBoolean Convert(const RTP_DataFrame &, RTP_DataFrame &)
    {
      // Dummy function, never called
      return false;
    }

    void GetStatistics(OpalMediaStatistics & statistics) const
    {
      statistics.m_fax.m_result = -2;
      PString msg;
      if (getCodecStatistics.Call(msg.GetPointer(1000), 999, context) != 0) {
        int result, errorCorrection;
        PINDEX position = 0;
        if (ExtractValue(msg, position, result) &&
            ExtractValue(msg, position, statistics.m_fax.m_bitRate) &&
            ExtractValue(msg, position, statistics.m_fax.m_compression) &&
            ExtractValue(msg, position, errorCorrection) &&
            ExtractValue(msg, position, statistics.m_fax.m_txPages) &&
            ExtractValue(msg, position, statistics.m_fax.m_rxPages) &&
            ExtractValue(msg, position, statistics.m_fax.m_totalPages) &&
            ExtractValue(msg, position, statistics.m_fax.m_imageSize) &&
            ExtractValue(msg, position, statistics.m_fax.m_resolutionX) &&
            ExtractValue(msg, position, statistics.m_fax.m_resolutionY, 'x') &&
            ExtractValue(msg, position, statistics.m_fax.m_pageWidth) &&
            ExtractValue(msg, position, statistics.m_fax.m_pageHeight, 'x') &&
            ExtractValue(msg, position, statistics.m_fax.m_badRows) &&
            ExtractValue(msg, position, statistics.m_fax.m_mostBadRows) &&
            ExtractValue(msg, position, statistics.m_fax.m_errorCorrectionRetries))
        {
          statistics.m_fax.m_result = result; // Only set this if everything parsed correctly
          statistics.m_fax.m_errorCorrection = errorCorrection != 0;
          statistics.m_fax.m_errorText = msg(msg.Find('(')+1, msg.Find(')')-1);
        }
      }
    }
};

#endif // OPAL_T38_CAPABILITY


//////////////////////////////////////////////////////////////////////////////

OpalPluginCodecManager::OpalPluginCodecManager(PPluginManager * _pluginMgr)
  : PPluginModuleManager(PLUGIN_CODEC_GET_CODEC_FN_STR, _pluginMgr)
{
  // instantiate all of the static codecs
  {
    H323StaticPluginCodecFactory::KeyList_T keyList = H323StaticPluginCodecFactory::GetKeyList();
    H323StaticPluginCodecFactory::KeyList_T::const_iterator r;
    for (r = keyList.begin(); r != keyList.end(); ++r) {
      H323StaticPluginCodec * instance = PFactory<H323StaticPluginCodec>::CreateInstance(*r);
      if (instance == NULL) {
        PTRACE(4, "OpalPlugin\tCannot instantiate static codec plugin " << *r);
      } else {
        PTRACE(4, "OpalPlugin\tLoading static codec plugin " << *r);
        RegisterStaticCodec(*r, instance->Get_GetAPIFn(), instance->Get_GetCodecFn());
      }
    }
  }

  // cause the plugin manager to load all dynamic plugins
#ifdef _DEBUG
  // cause the plugin manager to load all dynamic plugins
  pluginMgr->AddNotifier(PCREATE_NOTIFIER(OnLoadModule), PTrue);
#endif
  this->LoadMyPlugin();
}
extern void* g_GetcodecdefPointerForH264(int* count);
extern void* g_GetcodecdefPointerForTheora(int* count);
extern void* g_GetcodecdefPointerForh263_1998(int* count);
extern void* g_GetcodecdefPointerH263(int* count);

extern  void* g_GetcodecdefPointerSpeexCodec(int* count);
extern void* g_GetcodecdefPointerForYCode(int * count);
extern "C" {
extern  void* g_GetcodecdefiLBCPointer(int* count);
extern  void* g_GetcodecdefPointer(int* count);
extern  void* g_GetcodecdefPointerCelt(int* count);
extern  void* g_GetcodecdefPointerBroadVoice(int* count);
extern  void* g_GetcodecdefPointerIVoice(int* count);
}

void OpalPluginCodecManager::LoadPluginTheora( )
{
  //PluginCodec_GetCodecFunction getCodecs;
  //{
  //  PDynaLink::Function fn;
  //  if (!dll.GetFunction(PString(signatureFunctionName), fn)) {
  //    PTRACE(2, "Transcoder\tPlugin Codec DLL " << dll.GetName() << " is not a plugin codec");
  //    return;
  //  }
  //  getCodecs = (PluginCodec_GetCodecFunction)fn;
  //}

   int count=2;
//#ifdef _WIN32
  // PluginCodec_Definition * codecs=NULL;
	//PluginCodec_Definition * codecs = (PluginCodec_Definition *)g_GetcodecdefPointerForTheora(&count);//(*getCodecs)(&count, PLUGIN_CODEC_VERSION_OPTIONS);
	PluginCodec_Definition * codecs = (PluginCodec_Definition *)g_GetcodecdefPointerForH264(&count);//(*getCodecs)(&count, PLUGIN_CODEC_VERSION_OPTIONS);
//#else
//	PluginCodec_Definition * codecs = (PluginCodec_Definition *)g_GetcodecdefPointerForTheora(&count);//(*getCodecs)(&count, PLUGIN_CODEC_VERSION_OPTIONS);
//#endif
	if (codecs == NULL || count == 0) {
    PTRACE(1, "Transcoder\tPlugin Codec DLL " << "dll.GetName()" << " contains no codec definitions");
    return;
  } 

  // get handler for this plugin type
  PString name = "g_theora_name";//dll.GetName();
  PFactory<OpalPluginCodecHandler>::KeyList_T keys = PFactory<OpalPluginCodecHandler>::GetKeyList();
  PFactory<OpalPluginCodecHandler>::KeyList_T::const_iterator r;
  OpalPluginCodecHandler * handler = NULL;
  for (r = keys.begin(); r != keys.end(); ++r) {
    if (name.Right(r->length()) *= *r) {
      PTRACE(3, "Transcoder\tUsing custom handler for codec " << name);
      handler = PFactory<OpalPluginCodecHandler>::CreateInstance(*r);
      break;
    }
  }

  if (handler == NULL) {
    PTRACE(3, "Transcoder\tUsing default handler for plugin codec " << name);
    handler = new OpalPluginCodecHandler;
  }

  switch (0) {

    // plugin loaded
    case 0:
      RegisterCodecPlugins(count, codecs, handler);
      break;

    // plugin unloaded
    case 1:
      UnregisterCodecPlugins(count, codecs, handler);
      break;

    default:
      break;
  }

  //delete handler;
}
void OpalPluginCodecManager::LoadPluginh263_1998( )
{
	//PluginCodec_GetCodecFunction getCodecs;
	//{
	//  PDynaLink::Function fn;
	//  if (!dll.GetFunction(PString(signatureFunctionName), fn)) {
	//    PTRACE(2, "Transcoder\tPlugin Codec DLL " << dll.GetName() << " is not a plugin codec");
	//    return;
	//  }
	//  getCodecs = (PluginCodec_GetCodecFunction)fn;
	//}
	
	int count=2;
#ifdef _WIN32
  PluginCodec_Definition * codecs = (PluginCodec_Definition *)g_GetcodecdefPointerH263(&count);//(*getCodecs)(&count, PLUGIN_CODEC_VERSION_OPTIONS);
#else
  PluginCodec_Definition * codecs = (PluginCodec_Definition *)g_GetcodecdefPointerH263(&count);//(*getCodecs)(&count, PLUGIN_CODEC_VERSION_OPTIONS);
	//PluginCodec_Definition * codecs=(PluginCodec_Definition *)g_GetcodecdefPointerForh263_1998(&count);
 
#endif
	if (codecs == NULL || count == 0) {
		PTRACE(1, "Transcoder\tPlugin Codec DLL " << "dll.GetName()" << " contains no codec definitions");
		return;
	} 
	
	// get handler for this plugin type
	PString name = "LoadPluginh263_1998";//dll.GetName();
	PFactory<OpalPluginCodecHandler>::KeyList_T keys = PFactory<OpalPluginCodecHandler>::GetKeyList();
	PFactory<OpalPluginCodecHandler>::KeyList_T::const_iterator r;
	OpalPluginCodecHandler * handler = NULL;
	for (r = keys.begin(); r != keys.end(); ++r) {
		if (name.Right(r->length()) *= *r) {
			PTRACE(3, "Transcoder\tUsing custom handler for codec " << name);
			handler = PFactory<OpalPluginCodecHandler>::CreateInstance(*r);
			break;
		}
	}
	
	if (handler == NULL) {
		PTRACE(3, "Transcoder\tUsing default handler for plugin codec " << name);
		handler = new OpalPluginCodecHandler;
	}
	
	switch (0) {
			
			// plugin loaded
		case 0:
			RegisterCodecPlugins(count, codecs, handler);
			break;
			
			// plugin unloaded
		case 1:
			UnregisterCodecPlugins(count, codecs, handler);
			break;
			
		default:
			break;
	}
	
	//delete handler;
}
void OpalPluginCodecManager::LoadAudioPlugin(PluginCodec_Function  pPlugin)
{
  //PluginCodec_GetCodecFunction getCodecs;
  //{
  //  PDynaLink::Function fn;
  //  if (!dll.GetFunction(PString(signatureFunctionName), fn)) {
  //    PTRACE(2, "Transcoder\tPlugin Codec DLL " << dll.GetName() << " is not a plugin codec");
  //    return;
  //  }
  //  getCodecs = (PluginCodec_GetCodecFunction)fn;
  //}
	//for(int i=0;i< 1;i++)
	{
		int count=2;
 
		PluginCodec_Definition * codecs = (PluginCodec_Definition *) pPlugin/*g_GetcodecdefPointerIVoice*//*g_GetcodecdefPointerBroadVoice*//*g_GetcodecdefPointerBroadVoice*//*g_GetcodecdefPointerCelt*/(&count);
 		//switch (i)
		//{
		//case 0:
		//	codecs = g_GetcodecdefPointerForYCode;//iCodec
		////case 1:
		////	codecs = g_GetcodecdefPointer;//G.729 we user g.729 in Icodec

		//};
		if (codecs == NULL || count == 0) {
			PTRACE(1, "Transcoder\tPlugin Codec DLL " << "dll.GetName()" << " contains no codec definitions");
			return;
		} 

		// get handler for this plugin type
		PString name = "g_729_name";//dll.GetName();
		PFactory<OpalPluginCodecHandler>::KeyList_T keys = PFactory<OpalPluginCodecHandler>::GetKeyList();
		PFactory<OpalPluginCodecHandler>::KeyList_T::const_iterator r;
		OpalPluginCodecHandler * handler = NULL;
		for (r = keys.begin(); r != keys.end(); ++r) {
			if (name.Right(r->length()) *= *r) {
				PTRACE(3, "Transcoder\tUsing custom handler for codec " << name);
				handler = PFactory<OpalPluginCodecHandler>::CreateInstance(*r);
				break;
			}
		}

		if (handler == NULL) {
			PTRACE(3, "Transcoder\tUsing default handler for plugin codec " << name);
			handler = new OpalPluginCodecHandler;
		}

		switch (0) {

			// plugin loaded
			case 0:
				RegisterCodecPlugins(count, codecs, handler);
				break;

			// plugin unloaded
			case 1:
				UnregisterCodecPlugins(count, codecs, handler);
				break;

			default:
				break;
		}
	}
  //delete handler;
}
void OpalPluginCodecManager::LoadMyPlugin ()
{
  static bool isload =false ;
  if (!isload)
  {
#ifdef _WIN32
    LoadAudioPlugin(g_GetcodecdefPointerIVoice);
     LoadAudioPlugin(g_GetcodecdefPointer);//729
#else
	  //avcodec_init();avcodec_register_all();LoadPluginh263_1998();
#endif
    isload = true;
   // LoadAudioPlugin(g_GetcodecdefPointerBroadVoice) ;
    LoadPluginTheora(); /*LoadPluginh263_1998();*/
  }
}
OpalPluginCodecManager::~OpalPluginCodecManager()
{
}

void OpalPluginCodecManager::OnShutdown()
{
  mediaFormatsOnHeap.RemoveAll();

#if OPAL_H323
  // unregister the plugin capabilities
  H323CapabilityFactory::UnregisterAll();
#endif
}

void OpalPluginCodecManager::OnLoadPlugin(PDynaLink & dll, INT code)
{
  PluginCodec_GetCodecFunction getCodecs;
  {
    PDynaLink::Function fn;
    if (!dll.GetFunction(PString(signatureFunctionName), fn)) {
      PTRACE(2, "OpalPlugin\tPlugin Codec DLL " << dll.GetName() << " is not a plugin codec");
      return;
    }
    getCodecs = (PluginCodec_GetCodecFunction)fn;
  }

  unsigned int count;
  PluginCodec_Definition * codecs = (*getCodecs)(&count, PLUGIN_CODEC_VERSION_OPTIONS);
  if (codecs == NULL || count == 0) {
    PTRACE(1, "OpalPlugin\tPlugin Codec DLL " << dll.GetName() << " contains no codec definitions");
    return;
  } 

  // get handler for this plugin type
  PString name = dll.GetName();
  PFactory<OpalPluginCodecHandler>::KeyList_T keys = PFactory<OpalPluginCodecHandler>::GetKeyList();
  PFactory<OpalPluginCodecHandler>::KeyList_T::const_iterator r;
  OpalPluginCodecHandler * handler = NULL;
  for (r = keys.begin(); r != keys.end(); ++r) {
    if (name.Right(r->length()) *= *r) {
      PTRACE(3, "OpalPlugin\tUsing custom handler for codec " << name);
      handler = PFactory<OpalPluginCodecHandler>::CreateInstance(*r);
      break;
    }
  }

  if (handler == NULL) {
    PTRACE(3, "OpalPlugin\tUsing default handler for plugin codec " << name);
    handler = new OpalPluginCodecHandler;
  }

  switch (code) {

    // plugin loaded
    case 0:
      RegisterCodecPlugins(count, codecs, handler);
      break;

    // plugin unloaded
    case 1:
      UnregisterCodecPlugins(count, codecs, handler);
      break;

    default:
      break;
  }

  delete handler;
}

void OpalPluginCodecManager::RegisterStaticCodec(
      const H323StaticPluginCodecFactory::Key_T & PTRACE_PARAM(name),
      PluginCodec_GetAPIVersionFunction /*getApiVerFn*/,
      PluginCodec_GetCodecFunction getCodecFn)
{
  unsigned int count;
  PluginCodec_Definition * codecs = (*getCodecFn)(&count, PLUGIN_CODEC_VERSION_OPTIONS);
  if (codecs == NULL || count == 0) {
    PTRACE(1, "OpalPlugin\tStatic codec " << name << " contains no codec definitions");
    return;
  } 

  OpalPluginCodecHandler * handler = new OpalPluginCodecHandler;
  RegisterCodecPlugins(count, codecs, handler);
  delete handler;
}


static bool IsEncoder(const PluginCodec_Definition & encoder)
{
  PBoolean videoSupported = encoder.version >= PLUGIN_CODEC_VERSION_VIDEO;
  PBoolean faxSupported   = encoder.version >= PLUGIN_CODEC_VERSION_FAX;

  return ((encoder.h323CapabilityType != PluginCodec_H323Codec_undefined) && (
         (
           ((encoder.flags & PluginCodec_MediaTypeMask) == PluginCodec_MediaTypeAudio) && 
            (strcmp(encoder.sourceFormat, "L16") == 0)
         ) ||
         (
           ((encoder.flags & PluginCodec_MediaTypeMask) == PluginCodec_MediaTypeAudioStreamed) && 
            (strcmp(encoder.sourceFormat, "L16") == 0)
         ) ||
         (
           videoSupported &&
           ((encoder.flags & PluginCodec_MediaTypeMask) == PluginCodec_MediaTypeVideo) && 
           (strcmp(encoder.sourceFormat, "YUV420P") == 0)
        ) ||
         (
           faxSupported &&
           ((encoder.flags & PluginCodec_MediaTypeMask) == PluginCodec_MediaTypeFax) && 
           (strcmp(encoder.sourceFormat, "L16") == 0)
        )
       ));
}


#if PTRACING
static int PlugInLogFunction(unsigned level,
                             const char * file,
                             unsigned line,
                             const char * section,
                             const char * log)
{
  if (level > PTrace::GetLevel())
    return false;

  if (log == NULL)
    return true;

  if (section == NULL)
    section = "PlugIn";

  PTrace::Begin(level, file, line) << section << '\t' << log << PTrace::End;
  return true;
}
#endif


bool OpalPluginCodecManager::AddMediaFormat(OpalPluginCodecHandler * handler,
                                            const PTime & timeNow,
                                            const PluginCodec_Definition * codecDefn,
                                            const char * fmtName)
{
  // Create (if needed) the media format
  OpalMediaFormat existingFormat = GetRawPCM(fmtName, codecDefn->sampleRate);

  // deal with codec having no info, or timestamp in future
  time_t timeStamp;
  if (codecDefn->info == NULL || (timeStamp = codecDefn->info->timestamp) > timeNow.GetTimeInSeconds())
    timeStamp = timeNow.GetTimeInSeconds();

  if (existingFormat.IsTransportable() && existingFormat.GetCodecVersionTime() > timeStamp) {
    PTRACE(2, "OpalPlugin\tNewer media format " << existingFormat << " already exists");
    existingFormat = OpalMediaFormat();
  }

  if (existingFormat.IsValid())
    return true;

  PTRACE(3, "OpalPlugin\tCreating new media format " << fmtName);

  OpalMediaFormatInternal * mediaFormatInternal = NULL;
  unsigned frameTime = codecDefn->usPerFrame*codecDefn->sampleRate/1000000;

  // manually register the new singleton type, as we do not have a concrete type
  switch (codecDefn->flags & PluginCodec_MediaTypeMask) {
#if OPAL_VIDEO
    case PluginCodec_MediaTypeVideo:
      mediaFormatInternal = handler->OnCreateVideoFormat(*this,
                                                         codecDefn,
                                                         fmtName,
                                                         codecDefn->sdpFormat,
                                                         timeStamp);
      break;
#endif
    case PluginCodec_MediaTypeAudio:
    case PluginCodec_MediaTypeAudioStreamed:
      mediaFormatInternal = handler->OnCreateAudioFormat(*this,
                                                         codecDefn,
                                                         fmtName,
                                                         codecDefn->sdpFormat,
                                                         frameTime,
                                                         codecDefn->sampleRate,
                                                         timeStamp);
      break;

#if OPAL_T38_CAPABILITY
    case PluginCodec_MediaTypeFax:
      mediaFormatInternal = handler->OnCreateFaxFormat(*this,
                                                       codecDefn,
                                                       fmtName,
                                                       codecDefn->sdpFormat,
                                                       frameTime,
                                                       codecDefn->sampleRate,
                                                       timeStamp);
      break;
#endif
    default:
      PTRACE(3, "OpalPlugin\tOnknown Media Type " << (codecDefn->flags & PluginCodec_MediaTypeMask));
      return false;
  }

  if (mediaFormatInternal == NULL) {
    PTRACE(3, "OpalPlugin\tno media format created for codec " << codecDefn->descr);
    return false;
  }

  OpalMediaFormat * mediaFormat = new OpalPluginMediaFormat(mediaFormatInternal);

  // Remember format so we can deallocate it on shut down
  mediaFormatsOnHeap.Append(mediaFormat);

  // if the codec has been flagged to use a shared RTP payload type, then find a codec with the same SDP name
  // and clock rate and use that RTP code rather than creating a new one. That prevents codecs (like Speex) from 
  // consuming dozens of dynamic RTP types
  if ((codecDefn->flags & PluginCodec_RTPTypeShared) != 0 && (codecDefn->sdpFormat != NULL)) {
    OpalMediaFormatList list = OpalMediaFormat::GetAllRegisteredMediaFormats();
    for (OpalMediaFormatList::iterator iterFmt = list.begin(); iterFmt != list.end(); ++iterFmt) {
      OpalPluginMediaFormat * opalFmt = dynamic_cast<OpalPluginMediaFormat *>(&*iterFmt);
      if (opalFmt != NULL) {
        OpalPluginMediaFormatInternal * fmt = opalFmt->GetInfo();
        if (fmt != NULL &&
            fmt->codecDef->sdpFormat != NULL &&
            codecDefn->sampleRate == fmt->codecDef->sampleRate &&
            strcasecmp(codecDefn->sdpFormat, fmt->codecDef->sdpFormat) == 0) {
          mediaFormat->SetPayloadType(opalFmt->GetPayloadType());
          break;
        }
      }
    }
  }

  OpalMediaFormat::SetRegisteredMediaFormat(*mediaFormat);
  return true;
}


void OpalPluginCodecManager::RegisterCodecPlugins(unsigned int count, const PluginCodec_Definition * codecDefn, OpalPluginCodecHandler * handler)
{
  // make sure all non-timestamped codecs have the same concept of "now"
  static PTime timeNow;

  // Make sure raw codecs are instantiated
  GetOpalPCM16();
  GetOpalPCM16_16KHZ();
  //GetOpalPCM16_32KHZ();
  //GetOpalPCM16_48KHZ();
#if OPAL_VIDEO
  GetOpalYUV420P();
#endif

  for (unsigned  i = 0; i < count; i++,codecDefn++) {
#if PTRACING
    OpalPluginControl setLogFn(codecDefn, PLUGINCODEC_CONTROL_SET_LOG_FUNCTION);
    setLogFn.Call((void *)PlugInLogFunction, sizeof(PluginCodec_LogFunction));
#endif

    if (!AddMediaFormat(handler, timeNow, codecDefn, codecDefn->destFormat) ||
        !AddMediaFormat(handler, timeNow, codecDefn, codecDefn->sourceFormat))
      continue;

    // Create transcoder factories for the codecs
    OpalMediaFormat src = GetRawPCM(codecDefn->sourceFormat, codecDefn->sampleRate);
    OpalMediaFormat dst = GetRawPCM(codecDefn->destFormat,   codecDefn->sampleRate);
    bool isEncoder = IsEncoder(*codecDefn);
    switch (codecDefn->flags & PluginCodec_MediaTypeMask) {
  #if OPAL_VIDEO
      case PluginCodec_MediaTypeVideo:
        handler->RegisterVideoTranscoder(src, dst, codecDefn, isEncoder);
        break;
  #endif
      case PluginCodec_MediaTypeAudio:
        new OpalPluginTranscoderFactory<OpalPluginFramedAudioTranscoder>::Worker(OpalTranscoderKey(src, dst), codecDefn, isEncoder);
        OpalWAVFile::AddMediaFormat(isEncoder ? dst : src);
        break;
      case PluginCodec_MediaTypeAudioStreamed:
        new OpalPluginTranscoderFactory<OpalPluginStreamedAudioTranscoder>::Worker(OpalTranscoderKey(src, dst), codecDefn, isEncoder);
        break;
  #if OPAL_T38_CAPABILITY
      case PluginCodec_MediaTypeFax:
        new OpalPluginTranscoderFactory<OpalFaxTranscoder>::Worker(OpalTranscoderKey(src, dst), codecDefn, isEncoder);
        break;
  #endif
      default:
        PTRACE(3, "OpalPlugin\tno media transcoder factory created for codec " << codecDefn->descr);
        continue;
    }

#if OPAL_H323
    RegisterCapability(codecDefn);
#endif
  }
}

void OpalPluginCodecManager::UnregisterCodecPlugins(unsigned int, const PluginCodec_Definition *, OpalPluginCodecHandler * )
{
}


/////////////////////////////////////////////////////////////////////////////

OpalPluginCodecHandler::OpalPluginCodecHandler()
{
}


OpalMediaFormatInternal * OpalPluginCodecHandler::OnCreateAudioFormat(OpalPluginCodecManager & /*mgr*/,
                                                     const PluginCodec_Definition * codecDefn,
                                                                       const char * fmtName,
                                                                       const char * rtpEncodingName,
                                                                           unsigned frameTime,
                                                                           unsigned timeUnits,
                                                                             time_t timeStamp)
{
  return new OpalPluginAudioFormatInternal(codecDefn, fmtName, rtpEncodingName, frameTime, timeUnits, timeStamp);
}

#if OPAL_VIDEO

OpalMediaFormatInternal * OpalPluginCodecHandler::OnCreateVideoFormat(OpalPluginCodecManager & /*mgr*/,
                                                     const PluginCodec_Definition * codecDefn,
                                                                       const char * fmtName,
                                                                       const char * rtpEncodingName,
                                                                             time_t timeStamp)
{
  return new OpalPluginVideoFormatInternal(codecDefn, fmtName, rtpEncodingName, timeStamp);
}

void OpalPluginCodecHandler::RegisterVideoTranscoder(const PString & src, const PString & dst, const PluginCodec_Definition * codec, bool v)
{
  new OpalPluginTranscoderFactory<OpalPluginVideoTranscoder>::Worker(OpalTranscoderKey(src, dst), codec, v);
}

#endif

#if OPAL_T38_CAPABILITY
OpalMediaFormatInternal * OpalPluginCodecHandler::OnCreateFaxFormat(OpalPluginCodecManager & /*mgr*/,
                                                   const PluginCodec_Definition * codecDefn,
                                                                     const char * fmtName,
                                                                     const char * rtpEncodingName,
                                                                         unsigned frameTime,
                                                                         unsigned timeUnits,
                                                                           time_t timeStamp)
{
  return new OpalPluginFaxFormatInternal(codecDefn, fmtName, rtpEncodingName, frameTime, timeUnits, timeStamp);
}
#endif

/////////////////////////////////////////////////////////////////////////////

#if OPAL_H323

H323AudioPluginCapability::H323AudioPluginCapability(const PluginCodec_Definition * codecDefn,
                                                     const OpalMediaFormat & mediaFormat,
                                                     unsigned subType)
  : H323AudioCapability()
  , H323PluginCapabilityInfo(codecDefn, mediaFormat)
  , pluginSubType(subType)
{ 
}

PObject * H323AudioPluginCapability::Clone() const
{
  return new H323AudioPluginCapability(*this);
}

PString H323AudioPluginCapability::GetFormatName() const
{
  return H323PluginCapabilityInfo::GetFormatName();
}

unsigned H323AudioPluginCapability::GetSubType() const
{
  return pluginSubType;
}


static H323Capability * CreateStandardAudioCap(const PluginCodec_Definition * codecDefn,
                                               const OpalMediaFormat & mediaFormat,
                                               int subType)
{
  return new H323AudioPluginCapability(codecDefn, mediaFormat, subType);
}


//////////////////////////////////////////////////////////////////////////////
//
// Class for handling G.723.1 codecs
//

H323PluginG7231Capability::H323PluginG7231Capability(const PluginCodec_Definition * codecDefn,
                                                     const OpalMediaFormat & mediaFormat)
  : H323AudioPluginCapability(codecDefn, mediaFormat, H245_AudioCapability::e_g7231)
{
}


PObject * H323PluginG7231Capability::Clone() const
{
  return new H323PluginG7231Capability(*this);
}


PBoolean H323PluginG7231Capability::OnSendingPDU(H245_AudioCapability & cap, unsigned packetSize) const
{
  cap.SetTag(H245_AudioCapability::e_g7231);
  H245_AudioCapability_g7231 & g7231 = cap;
  g7231.m_maxAl_sduAudioFrames = packetSize;
  g7231.m_silenceSuppression = GetMediaFormat().GetOptionBoolean("VAD");
  return true;
}

PBoolean H323PluginG7231Capability::OnReceivedPDU(const H245_AudioCapability & cap,  unsigned & packetSize)
{
  if (cap.GetTag() != H245_AudioCapability::e_g7231)
    return false;
  const H245_AudioCapability_g7231 & g7231 = cap;
  packetSize = g7231.m_maxAl_sduAudioFrames;
  GetWritableMediaFormat().SetOptionBoolean("VAD", g7231.m_silenceSuppression);
  return true;
}


H323Capability * CreateG7231Cap(const PluginCodec_Definition * codecDefn,
                                const OpalMediaFormat & mediaFormat,
                                int /*subType*/) 
{
  return new H323PluginG7231Capability(codecDefn, mediaFormat);
}


//////////////////////////////////////////////////////////////////////////////
//
// Class for handling GSM plugin capabilities
//

class H323GSMPluginCapability : public H323AudioPluginCapability
{
  PCLASSINFO(H323GSMPluginCapability, H323AudioPluginCapability);
  public:
    H323GSMPluginCapability(const PluginCodec_Definition * codecDefn,
                            const OpalMediaFormat & mediaFormat,
                            int _pluginSubType, int _comfortNoise, int _scrambled)
      : H323AudioPluginCapability(codecDefn, mediaFormat, _pluginSubType),
        comfortNoise(_comfortNoise), scrambled(_scrambled)
    { }

    Comparison Compare(const PObject & obj) const;

    virtual PObject * Clone() const
    { return new H323GSMPluginCapability(*this); }

    virtual PBoolean OnSendingPDU(
      H245_AudioCapability & pdu,  /// PDU to set information on
      unsigned packetSize          /// Packet size to use in capability
    ) const;

    virtual PBoolean OnReceivedPDU(
      const H245_AudioCapability & pdu,  /// PDU to get information from
      unsigned & packetSize              /// Packet size to use in capability
    );
  protected:
    int comfortNoise;
    int scrambled;
};


H323Capability * CreateNonStandardAudioCap(const PluginCodec_Definition * codecDefn,
                                           const OpalMediaFormat & mediaFormat,
                                           int /*subType*/) 
{
  PluginCodec_H323NonStandardCodecData * pluginData =  (PluginCodec_H323NonStandardCodecData *)codecDefn->h323CapabilityData;
  if (pluginData == NULL)
    return new H323CodecPluginNonStandardAudioCapability(codecDefn,
                                                         mediaFormat,
                                                         (const unsigned char *)codecDefn->descr, 
                                                         strlen(codecDefn->descr));

  if (pluginData->capabilityMatchFunction != NULL) 
    return new H323CodecPluginNonStandardAudioCapability(codecDefn,
                                                         mediaFormat,
                                                         (H323NonStandardCapabilityInfo::CompareFuncType)pluginData->capabilityMatchFunction,
                                                         pluginData->data,
                                                         pluginData->dataLength);

  return new H323CodecPluginNonStandardAudioCapability(codecDefn,
                                                       mediaFormat,
                                                       pluginData->data,
                                                       pluginData->dataLength);
}

H323Capability *CreateGenericAudioCap(const PluginCodec_Definition * codecDefn,
                                      const OpalMediaFormat & mediaFormat,
                                      int /*subType*/) 
{
  return new H323CodecPluginGenericAudioCapability(codecDefn, mediaFormat, (PluginCodec_H323GenericCodecData *)codecDefn->h323CapabilityData);
}


H323Capability * CreateGSMCap(const PluginCodec_Definition * codecDefn,
                              const OpalMediaFormat & mediaFormat, 
                              int subType) 
{
  PluginCodec_H323AudioGSMData * pluginData =  (PluginCodec_H323AudioGSMData *)codecDefn->h323CapabilityData;
  return new H323GSMPluginCapability(codecDefn, mediaFormat, subType, pluginData->comfortNoise, pluginData->scrambled);
}


/////////////////////////////////////////////////////////////////////////////

H323PluginCapabilityInfo::H323PluginCapabilityInfo(const PluginCodec_Definition * codecDefn, const OpalMediaFormat & mediaFormat)
 : m_codecDefn(codecDefn)
 , m_capabilityFormatName(mediaFormat.GetName())
{
}

//H323PluginCapabilityInfo::H323PluginCapabilityInfo(const PString & name)
// : m_codecDefn(NULL)
// , m_capabilityFormatName(name)
//{
//}


/////////////////////////////////////////////////////////////////////////////

H323CodecPluginNonStandardAudioCapability::H323CodecPluginNonStandardAudioCapability(const PluginCodec_Definition * codecDefn,
                                                                                     const OpalMediaFormat & mediaFormat,
                                                                                     H323NonStandardCapabilityInfo::CompareFuncType compareFunc,
                                                                                     const unsigned char * data, unsigned dataLen)
 : H323NonStandardAudioCapability(compareFunc,data, dataLen), 
   H323PluginCapabilityInfo(codecDefn, mediaFormat)
{
  PluginCodec_H323NonStandardCodecData * nonStdData = (PluginCodec_H323NonStandardCodecData *)codecDefn->h323CapabilityData;
  if (nonStdData->objectId != NULL) {
    oid = PString(nonStdData->objectId);
  } else {
    t35CountryCode   = nonStdData->t35CountryCode;
    t35Extension     = nonStdData->t35Extension;
    manufacturerCode = nonStdData->manufacturerCode;
  }
}

H323CodecPluginNonStandardAudioCapability::H323CodecPluginNonStandardAudioCapability(const PluginCodec_Definition * codecDefn,
                                                                                     const OpalMediaFormat & mediaFormat,
                                                                                     const unsigned char * data, unsigned dataLen)
 : H323NonStandardAudioCapability(data, dataLen), 
   H323PluginCapabilityInfo(codecDefn, mediaFormat)
{
  PluginCodec_H323NonStandardCodecData * nonStdData = (PluginCodec_H323NonStandardCodecData *)codecDefn->h323CapabilityData;
  if (nonStdData->objectId != NULL) {
    oid = PString(nonStdData->objectId);
  } else {
    t35CountryCode   = nonStdData->t35CountryCode;
    t35Extension     = nonStdData->t35Extension;
    manufacturerCode = nonStdData->manufacturerCode;
  }
}

PObject * H323CodecPluginNonStandardAudioCapability::Clone() const
{ return new H323CodecPluginNonStandardAudioCapability(*this); }

PString H323CodecPluginNonStandardAudioCapability::GetFormatName() const
{ return H323PluginCapabilityInfo::GetFormatName();}

/////////////////////////////////////////////////////////////////////////////

H323CodecPluginGenericAudioCapability::H323CodecPluginGenericAudioCapability(const PluginCodec_Definition * codecDefn,
                                                                             const OpalMediaFormat & mediaFormat,
                                                                             const PluginCodec_H323GenericCodecData *data )
  : H323GenericAudioCapability(data->standardIdentifier, data != NULL ? data->maxBitRate : 0),
    H323PluginCapabilityInfo(codecDefn, mediaFormat)
{
}

PObject * H323CodecPluginGenericAudioCapability::Clone() const
{ return new H323CodecPluginGenericAudioCapability(*this); }

PString H323CodecPluginGenericAudioCapability::GetFormatName() const
{ return H323PluginCapabilityInfo::GetFormatName();}

/////////////////////////////////////////////////////////////////////////////

PObject::Comparison H323GSMPluginCapability::Compare(const PObject & obj) const
{
  if (!PIsDescendant(&obj, H323GSMPluginCapability))
    return LessThan;

  Comparison result = H323AudioCapability::Compare(obj);
  if (result != EqualTo)
    return result;

  const H323GSMPluginCapability& other = (const H323GSMPluginCapability&)obj;
  if (scrambled < other.scrambled)
    return LessThan;
  if (comfortNoise < other.comfortNoise)
    return LessThan;
  return EqualTo;
}


PBoolean H323GSMPluginCapability::OnSendingPDU(H245_AudioCapability & cap, unsigned packetSize) const
{
  cap.SetTag(pluginSubType);
  H245_GSMAudioCapability & gsm = cap;
  gsm.m_audioUnitSize = packetSize * m_codecDefn->parm.audio.bytesPerFrame;
  gsm.m_comfortNoise  = comfortNoise;
  gsm.m_scrambled     = scrambled;

  return true;
}


PBoolean H323GSMPluginCapability::OnReceivedPDU(const H245_AudioCapability & cap, unsigned & packetSize)
{
  const H245_GSMAudioCapability & gsm = cap;
  packetSize   = gsm.m_audioUnitSize / m_codecDefn->parm.audio.bytesPerFrame;
  if (packetSize == 0)
    packetSize = 1;

  scrambled    = gsm.m_scrambled;
  comfortNoise = gsm.m_comfortNoise;

  return true;
}

/////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO

//////////////////////////////////////////////////////////////////////////////
//
// Class for handling most videoplugin capabilities
//

H323VideoPluginCapability::H323VideoPluginCapability(const PluginCodec_Definition * codecDefn,
                                                     const OpalMediaFormat & mediaFormat,
                                                     unsigned _pluginSubType)
  : H323VideoCapability()
  , H323PluginCapabilityInfo(codecDefn, mediaFormat)
  , pluginSubType(_pluginSubType)
{ 
}

PString H323VideoPluginCapability::GetFormatName() const
{
  return H323PluginCapabilityInfo::GetFormatName();
}


unsigned H323VideoPluginCapability::GetSubType() const
{
  return pluginSubType;
}

#define SET_OR_CREATE_PARM(option, val, op) \
  if (mediaFormat.GetOptionInteger(OpalVideoFormat::option()) op val) { \
    if (mediaFormat.FindOption(OpalVideoFormat::option()) == NULL) \
      mediaFormat.AddOption(new OpalMediaOptionUnsigned(OpalVideoFormat::option(), false)); \
    if (!mediaFormat.SetOptionInteger(OpalVideoFormat::option(), val)) { \
      PTRACE(5, "H263\t" #option " failed"); \
      return false; \
    } \
  } \


bool H323VideoPluginCapability::SetOptionsFromMPI(OpalMediaFormat & mediaFormat, int frameWidth, int frameHeight, int frameRate)
{
  SET_OR_CREATE_PARM(MaxRxFrameWidthOption, frameWidth, <);
  SET_OR_CREATE_PARM(MinRxFrameWidthOption, frameWidth, >);
  SET_OR_CREATE_PARM(MaxRxFrameHeightOption, frameHeight, <);
  SET_OR_CREATE_PARM(MinRxFrameHeightOption, frameHeight, >);

  if (!mediaFormat.SetOptionInteger(OpalMediaFormat::FrameTimeOption(), OpalMediaFormat::VideoClockRate * 100 * frameRate / 2997)) {
    PTRACE(5, "H263\tFrameTimeOption failed");
    return false;
  }

  return true;
}


void H323VideoPluginCapability::PrintOn(std::ostream & strm) const
{
  H323VideoCapability::PrintOn(strm);
}

/////////////////////////////////////////////////////////////////////////////

H323H261PluginCapability::H323H261PluginCapability(const PluginCodec_Definition * codecDefn, const OpalMediaFormat & mediaFormat)
  : H323VideoPluginCapability(codecDefn, mediaFormat, H245_VideoCapability::e_h261VideoCapability)
{
}

PObject::Comparison H323H261PluginCapability::Compare(const PObject & obj) const
{
  if (!PIsDescendant(&obj, H323H261PluginCapability))
    return LessThan;

  Comparison result = H323Capability::Compare(obj);
  if (result != EqualTo)
    return result;

  const H323H261PluginCapability & other = (const H323H261PluginCapability &)obj;

  const OpalMediaFormat & mediaFormat = GetMediaFormat();
  int qcifMPI = mediaFormat.GetOptionInteger(qcifMPI_tag);
  int  cifMPI = mediaFormat.GetOptionInteger(cifMPI_tag);

  const OpalMediaFormat & otherFormat = other.GetMediaFormat();
  int other_qcifMPI = otherFormat.GetOptionInteger(qcifMPI_tag);
  int other_cifMPI  = otherFormat.GetOptionInteger(cifMPI_tag);

  if ((IsValidMPI(qcifMPI) && IsValidMPI(other_qcifMPI)) ||
      (IsValidMPI( cifMPI) && IsValidMPI(other_cifMPI)))
    return EqualTo;

  if (IsValidMPI(qcifMPI))
    return LessThan;

  return GreaterThan;
}

PObject * H323H261PluginCapability::Clone() const
{ 
  return new H323H261PluginCapability(*this); 
}

PBoolean H323H261PluginCapability::OnSendingPDU(H245_VideoCapability & cap) const
{
  cap.SetTag(H245_VideoCapability::e_h261VideoCapability);

  H245_H261VideoCapability & h261 = cap;

  const OpalMediaFormat & mediaFormat = GetMediaFormat();

  /*
#if PTRACING
  ostream & traceStream = PTrace::Begin(4, __FILE__, __LINE__);
  traceStream << "H.261 extracting options from\n";
  for (PINDEX i = 0; i < mediaFormat.GetOptionCount(); i++) {
    const OpalMediaOption & option = mediaFormat.GetOption(i);
    traceStream << "         " << option.GetName() << " = " << option.AsString() << '\n';
  }
  traceStream << PTrace::End;
#endif
  */

  int qcifMPI = mediaFormat.GetOptionInteger(qcifMPI_tag, 0);
  int cifMPI = mediaFormat.GetOptionInteger(cifMPI_tag);
  if (!IsValidMPI(qcifMPI) && !IsValidMPI(cifMPI)) {
    PTRACE(2, "OpalPlugin\tCannot encode H.261 without a resolution");
    return false;
  }

  if (IsValidMPI(qcifMPI)) {
    h261.IncludeOptionalField(H245_H261VideoCapability::e_qcifMPI);
    h261.m_qcifMPI = qcifMPI > 4 ? 4 : qcifMPI;
  }
  if (IsValidMPI(cifMPI)) {
    h261.IncludeOptionalField(H245_H261VideoCapability::e_cifMPI);
    h261.m_cifMPI = cifMPI > 4 ? 4 : cifMPI;
  }

  h261.m_temporalSpatialTradeOffCapability = mediaFormat.GetOptionBoolean(h323_temporalSpatialTradeOffCapability_tag, false);
  h261.m_maxBitRate                        = (mediaFormat.GetOptionInteger(OpalMediaFormat::MaxBitRateOption(), 621700)+50)/100;
  h261.m_stillImageTransmission            = mediaFormat.GetOptionBoolean(h323_stillImageTransmission_tag, mediaFormat.GetOptionBoolean(H261_ANNEX_D));

  return true;
}


PBoolean H323H261PluginCapability::OnSendingPDU(H245_VideoMode & pdu) const
{
  pdu.SetTag(H245_VideoMode::e_h261VideoMode);
  H245_H261VideoMode & mode = pdu;

  const OpalMediaFormat & mediaFormat = GetMediaFormat();

  int qcifMPI = mediaFormat.GetOptionInteger(qcifMPI_tag, PLUGINCODEC_MPI_DISABLED);

  mode.m_resolution.SetTag(IsValidMPI(qcifMPI) ? H245_H261VideoMode_resolution::e_qcif
                                               : H245_H261VideoMode_resolution::e_cif);

  mode.m_bitRate                = (mediaFormat.GetOptionInteger(OpalMediaFormat::MaxBitRateOption(), 621700) + 50) / 1000;
  mode.m_stillImageTransmission = mediaFormat.GetOptionBoolean(h323_stillImageTransmission_tag, mediaFormat.GetOptionBoolean(H261_ANNEX_D));

  return true;
}

PBoolean H323H261PluginCapability::OnReceivedPDU(const H245_VideoCapability & cap)
{
  if (cap.GetTag() != H245_VideoCapability::e_h261VideoCapability)
    return false;

  OpalMediaFormat & mediaFormat = GetWritableMediaFormat();

  const H245_H261VideoCapability & h261 = cap;
  if (h261.HasOptionalField(H245_H261VideoCapability::e_qcifMPI)) {

    if (!mediaFormat.SetOptionInteger(qcifMPI_tag, h261.m_qcifMPI))
      return false;

    if (!SetOptionsFromMPI(mediaFormat, PVideoFrameInfo::QCIFWidth, PVideoFrameInfo::QCIFHeight, h261.m_qcifMPI))
      return false;
  }
  else {
    if (!mediaFormat.SetOptionInteger(qcifMPI_tag, PLUGINCODEC_MPI_DISABLED))
      return false;
  }

  if (h261.HasOptionalField(H245_H261VideoCapability::e_cifMPI)) {

    if (!mediaFormat.SetOptionInteger(cifMPI_tag, h261.m_cifMPI))
      return false;

    if (!SetOptionsFromMPI(mediaFormat, PVideoFrameInfo::CIFWidth, PVideoFrameInfo::CIFHeight, h261.m_cifMPI))
      return false;
  }
  else {
    if (!mediaFormat.SetOptionInteger(cifMPI_tag, PLUGINCODEC_MPI_DISABLED))
      return false;
  }

  mediaFormat.SetOptionInteger(OpalMediaFormat::MaxBitRateOption(),        h261.m_maxBitRate*100);
  mediaFormat.SetOptionBoolean(h323_temporalSpatialTradeOffCapability_tag, h261.m_temporalSpatialTradeOffCapability);
  mediaFormat.SetOptionBoolean(h323_stillImageTransmission_tag,            h261.m_stillImageTransmission);
  mediaFormat.SetOptionBoolean(H261_ANNEX_D,                               h261.m_stillImageTransmission);

  return true;
}


H323Capability * CreateH261Cap(const PluginCodec_Definition * codecDefn,
                               const OpalMediaFormat & mediaFormat,
                               int /*subType*/) 
{
  PTRACE(4, "OpalPlugin\tCreating H.261 plugin capability");
  return new H323H261PluginCapability(codecDefn, mediaFormat);
}


/////////////////////////////////////////////////////////////////////////////

H323H263PluginCapability::H323H263PluginCapability(const PluginCodec_Definition * codecDefn, const OpalMediaFormat & mediaFormat)
  : H323VideoPluginCapability(codecDefn, mediaFormat, H245_VideoCapability::e_h263VideoCapability)
{ 
}


struct H323H263CustomSize
{
  unsigned width;
  unsigned height;
  unsigned mpi;
};

typedef std::list<H323H263CustomSize> H323H263CustomSizes;

static void GetCustomMPI(const OpalMediaFormat & mediaFormat, H323H263CustomSizes & sizes)
{
  PStringArray customSizes = mediaFormat.GetOptionString(PLUGINCODEC_CUSTOM_MPI).Tokenise(';');
  for (PINDEX i = 0; i < customSizes.GetSize(); ++i) {
    PStringArray customSize = customSizes[i].Tokenise(',');
    if (customSize.GetSize() == 3) {
      H323H263CustomSize size;
      size.width  = customSize[0].AsUnsigned();
      size.height = customSize[1].AsUnsigned();
      size.mpi    = customSize[2].AsUnsigned();
      if (size.width > 15 && size.height > 15 && IsValidMPI(size.mpi))
        sizes.push_back(size);
    }
  }
}


PObject::Comparison H323H263PluginCapability::Compare(const PObject & obj) const
{
  if (!PIsDescendant(&obj, H323H263PluginCapability)) {
    PTRACE(5, "H.263\t" << *this << " != " << obj);
    return LessThan;
  }

  Comparison result = H323Capability::Compare(obj);
  if (result != EqualTo) {
    PTRACE(5, "H.263\t" << *this << " != " << obj);
    return result;
  }

  const H323H263PluginCapability & other = (const H323H263PluginCapability &)obj;

  const OpalMediaFormat & mediaFormat = GetMediaFormat();

  int sqcifMPI = mediaFormat.GetOptionInteger(sqcifMPI_tag);
  int qcifMPI  = mediaFormat.GetOptionInteger(qcifMPI_tag);
  int cifMPI   = mediaFormat.GetOptionInteger(cifMPI_tag);
  int cif4MPI  = mediaFormat.GetOptionInteger(cif4MPI_tag);
  int cif16MPI = mediaFormat.GetOptionInteger(cif16MPI_tag);
  H323H263CustomSizes customSizes;
  GetCustomMPI(mediaFormat, customSizes);

  const OpalMediaFormat & otherFormat = other.GetMediaFormat();
  int other_sqcifMPI = otherFormat.GetOptionInteger(sqcifMPI_tag);
  int other_qcifMPI  = otherFormat.GetOptionInteger(qcifMPI_tag);
  int other_cifMPI   = otherFormat.GetOptionInteger(cifMPI_tag);
  int other_cif4MPI  = otherFormat.GetOptionInteger(cif4MPI_tag);
  int other_cif16MPI = otherFormat.GetOptionInteger(cif16MPI_tag);
  H323H263CustomSizes other_customSizes;
  GetCustomMPI(otherFormat, other_customSizes);

  if ((IsValidMPI( sqcifMPI) && IsValidMPI( other_sqcifMPI)) ||
      (IsValidMPI(  qcifMPI) && IsValidMPI(  other_qcifMPI)) ||
      (IsValidMPI(   cifMPI) && IsValidMPI(   other_cifMPI)) ||
      (IsValidMPI(  cif4MPI) && IsValidMPI(  other_cif4MPI)) ||
      (IsValidMPI( cif16MPI) && IsValidMPI( other_cif16MPI))) {
    PTRACE(5, "H.263\t" << *this << " == " << other);
    return EqualTo;
  }

  for (H323H263CustomSizes::const_iterator mySize = customSizes.begin(); mySize != customSizes.end(); ++mySize) {
    for (H323H263CustomSizes::const_iterator otherSize = other_customSizes.begin(); otherSize != other_customSizes.end(); ++otherSize) {
      if (mySize->width == otherSize->width && mySize->height == otherSize->height) {
        PTRACE(5, "H.263\t" << *this << " == " << other);
        return EqualTo;
      }
    }
  }

  if ((!IsValidMPI(cif16MPI) && IsValidMPI(other_cif16MPI)) ||
      (!IsValidMPI( cif4MPI) && IsValidMPI( other_cif4MPI)) ||
      (!IsValidMPI(  cifMPI) && IsValidMPI(  other_cifMPI)) ||
      (!IsValidMPI( qcifMPI) && IsValidMPI( other_qcifMPI)) ||
      (!IsValidMPI(sqcifMPI) && IsValidMPI(other_sqcifMPI))) {
    PTRACE(5, "H.263\t" << *this << " < " << other);
    return LessThan;
  }

  PTRACE(5, "H.263\t" << *this << " > " << other);
  return GreaterThan;
}

PObject * H323H263PluginCapability::Clone() const
{ return new H323H263PluginCapability(*this); }

static bool SetTransmittedCap(const OpalMediaFormat & mediaFormat,
                              H245_H263VideoCapability & h263,
                              const char * mpiTag,
                              int mpiEnum,
                              PASN_Integer & mpi)
{
  int mpiVal = mediaFormat.GetOptionInteger(mpiTag);
  if (!IsValidMPI(mpiVal))
    return false;

  h263.IncludeOptionalField(mpiEnum);
  mpi = mpiVal;
  return true;
}


PBoolean H323H263PluginCapability::OnSendingPDU(H245_VideoCapability & cap) const
{
  cap.SetTag(H245_VideoCapability::e_h263VideoCapability);
  H245_H263VideoCapability & h263 = cap;

  const OpalMediaFormat & mediaFormat = GetMediaFormat();

  H323H263CustomSizes customSizes;
  GetCustomMPI(mediaFormat, customSizes);

  bool atLeastOneResolution = !customSizes.empty();

  if (SetTransmittedCap(mediaFormat, cap, sqcifMPI_tag, H245_H263VideoCapability::e_sqcifMPI, h263.m_sqcifMPI))
    atLeastOneResolution = true;
  if (SetTransmittedCap(mediaFormat, cap, qcifMPI_tag,  H245_H263VideoCapability::e_qcifMPI,  h263.m_qcifMPI))
    atLeastOneResolution = true;
  if (SetTransmittedCap(mediaFormat, cap, cifMPI_tag,   H245_H263VideoCapability::e_cifMPI,   h263.m_cifMPI))
    atLeastOneResolution = true;
  if (SetTransmittedCap(mediaFormat, cap, cif4MPI_tag,  H245_H263VideoCapability::e_cif4MPI,  h263.m_cif4MPI))
    atLeastOneResolution = true;
  if (SetTransmittedCap(mediaFormat, cap, cif16MPI_tag, H245_H263VideoCapability::e_cif16MPI, h263.m_cif16MPI))
    atLeastOneResolution = true;

  if (!atLeastOneResolution) {
    PTRACE(2, "OpalPlugin\tCannot encode H.263 without a resolution");
    return false;
  }

  h263.m_maxBitRate                        = (mediaFormat.GetOptionInteger(OpalMediaFormat::MaxBitRateOption(), 327600) + 50) / 100;
  h263.m_temporalSpatialTradeOffCapability = mediaFormat.GetOptionBoolean(h323_temporalSpatialTradeOffCapability_tag, false);
  h263.m_unrestrictedVector	           = mediaFormat.GetOptionBoolean(h323_unrestrictedVector_tag, false);
  h263.m_arithmeticCoding	           = mediaFormat.GetOptionBoolean(h323_arithmeticCoding_tag, false);
  h263.m_advancedPrediction	           = mediaFormat.GetOptionBoolean(h323_advancedPrediction_tag, mediaFormat.GetOptionBoolean(H263_ANNEX_F));
  h263.m_pbFrames	                   = mediaFormat.GetOptionBoolean(h323_pbFrames_tag, false);
  h263.m_errorCompensation                 = mediaFormat.GetOptionBoolean(h323_errorCompensation_tag, false);

  int hrdB = mediaFormat.GetOptionInteger(h323_hrdB_tag, -1);
  if (hrdB >= 0) {
    h263.IncludeOptionalField(H245_H263VideoCapability::e_hrd_B);
    h263.m_hrd_B = hrdB;
  }

  int bppMaxKb = mediaFormat.GetOptionInteger(h323_bppMaxKb_tag, -1);
  if (bppMaxKb >= 0) {
    h263.IncludeOptionalField(H245_H263VideoCapability::e_bppMaxKb);
    h263.m_bppMaxKb = bppMaxKb;
  }

  bool annexI = mediaFormat.GetOptionBoolean(H263_ANNEX_I);
  bool annexJ = mediaFormat.GetOptionBoolean(H263_ANNEX_J);
  bool annexT = mediaFormat.GetOptionBoolean(H263_ANNEX_T);
  if (annexI || annexJ || annexT || !customSizes.empty()) {
    h263.IncludeOptionalField(H245_H263VideoCapability::e_h263Options);
    h263.m_h263Options.m_advancedIntraCodingMode  = annexI;
    h263.m_h263Options.m_deblockingFilterMode     = annexJ;
    h263.m_h263Options.m_modifiedQuantizationMode = annexT;

    if (!customSizes.empty()) {
      h263.m_h263Options.IncludeOptionalField(H245_H263Options::e_customPictureFormat);
      h263.m_h263Options.m_customPictureFormat.SetSize(customSizes.size());
      PINDEX count = 0;
      for (H323H263CustomSizes::const_iterator it = customSizes.begin(); it != customSizes.end(); ++it) {
        H245_CustomPictureFormat & customPicture = h263.m_h263Options.m_customPictureFormat[count++];
        customPicture.m_minCustomPictureWidth = it->width;
        customPicture.m_minCustomPictureHeight = it->height;
        customPicture.m_maxCustomPictureWidth = it->width;
        customPicture.m_maxCustomPictureHeight = it->height;
        customPicture.m_mPI.IncludeOptionalField(H245_CustomPictureFormat_mPI::e_standardMPI);
        customPicture.m_mPI.m_standardMPI = it->mpi;
      }
    }
  }

  return true;
}


PBoolean H323H263PluginCapability::OnSendingPDU(H245_VideoMode & pdu) const
{
  pdu.SetTag(H245_VideoMode::e_h263VideoMode);
  H245_H263VideoMode & mode = pdu;

  const OpalMediaFormat & mediaFormat = GetMediaFormat();

  int sqcifMPI = mediaFormat.GetOptionInteger(sqcifMPI_tag);
  int qcifMPI  = mediaFormat.GetOptionInteger(qcifMPI_tag);
  int cifMPI   = mediaFormat.GetOptionInteger(cifMPI_tag);
  int cif4MPI  = mediaFormat.GetOptionInteger(cif4MPI_tag);
  int cif16MPI = mediaFormat.GetOptionInteger(cif16MPI_tag);

  H323H263CustomSizes customSizes;
  GetCustomMPI(mediaFormat, customSizes);

  if (IsValidMPI(cif16MPI))
    mode.m_resolution.SetTag(H245_H263VideoMode_resolution::e_cif16);
  else if (IsValidMPI(cif4MPI))
    mode.m_resolution.SetTag(H245_H263VideoMode_resolution::e_cif4);
  else if (IsValidMPI(cifMPI))
    mode.m_resolution.SetTag(H245_H263VideoMode_resolution::e_cif);
  else if (IsValidMPI(qcifMPI))
    mode.m_resolution.SetTag(H245_H263VideoMode_resolution::e_qcif);
  else if (IsValidMPI(sqcifMPI))
    mode.m_resolution.SetTag(H245_H263VideoMode_resolution::e_sqcif);
  else if (!customSizes.empty())
    mode.m_resolution.SetTag(H245_H263VideoMode_resolution::e_custom);
  else {
    PTRACE(2, "OpalPlugin\tCannot encode H.263 without a resolution");
    return false;
  }

  mode.m_bitRate              = (mediaFormat.GetOptionInteger(OpalMediaFormat::MaxBitRateOption(), 327600) + 50) / 100;
  mode.m_unrestrictedVector   = mediaFormat.GetOptionBoolean(h323_unrestrictedVector_tag, false);
  mode.m_arithmeticCoding     = mediaFormat.GetOptionBoolean(h323_arithmeticCoding_tag, false);
  mode.m_advancedPrediction   = mediaFormat.GetOptionBoolean(h323_advancedPrediction_tag, mediaFormat.GetOptionBoolean(H263_ANNEX_F));
  mode.m_pbFrames             = mediaFormat.GetOptionBoolean(h323_pbFrames_tag, false);
  mode.m_errorCompensation    = mediaFormat.GetOptionBoolean(h323_errorCompensation_tag, false);

  bool annexI = mediaFormat.GetOptionBoolean(H263_ANNEX_I);
  bool annexJ = mediaFormat.GetOptionBoolean(H263_ANNEX_J);
  bool annexT = mediaFormat.GetOptionBoolean(H263_ANNEX_T);
  if (annexI || annexJ || annexT || !customSizes.empty()) {
    mode.IncludeOptionalField(H245_H263VideoMode::e_h263Options);
    mode.m_h263Options.m_advancedIntraCodingMode  = annexI;
    mode.m_h263Options.m_deblockingFilterMode     = annexJ;
    mode.m_h263Options.m_modifiedQuantizationMode = annexT;

    if (!customSizes.empty()) {
      mode.m_h263Options.IncludeOptionalField(H245_H263Options::e_customPictureFormat);
      mode.m_h263Options.m_customPictureFormat.SetSize(1);
      H245_CustomPictureFormat & customPicture = mode.m_h263Options.m_customPictureFormat[0];
      customPicture.m_minCustomPictureWidth = customSizes.back().width;
      customPicture.m_minCustomPictureHeight = customSizes.back().height;
      customPicture.m_maxCustomPictureWidth = customSizes.back().width;
      customPicture.m_maxCustomPictureHeight = customSizes.back().height;
      customPicture.m_mPI.IncludeOptionalField(H245_CustomPictureFormat_mPI::e_standardMPI);
      customPicture.m_mPI.m_standardMPI = customSizes.back().mpi;
    }
  }

  return true;
}

static bool SetReceivedH263Cap(OpalMediaFormat & mediaFormat, 
                               const H245_H263VideoCapability & h263, 
                               const char * mpiTag,
                               int mpiEnum,
                               const PASN_Integer & mpi,
                               int frameWidth, int frameHeight,
                               bool & formatDefined)
{
  if (h263.HasOptionalField(mpiEnum)) {
    if (!mediaFormat.SetOptionInteger(mpiTag, mpi))
      return false;
    if (mpi != 0) {
      if (!H323VideoPluginCapability::SetOptionsFromMPI(mediaFormat, frameWidth, frameHeight, mpi))
        return false;
      formatDefined = true;
    }
  }
  else
    mediaFormat.SetOptionInteger(mpiTag, PLUGINCODEC_MPI_DISABLED);

  return true;
}


static bool OnReceivedCustomMPI(const H245_H263VideoCapability & h263,
                                int & minWidth, int & minHeight,
                                int & maxWidth, int & maxHeight,
                                int & maxMPI,
                                PString & option)
{
  if (!h263.HasOptionalField(H245_H263VideoCapability::e_h263Options))
    return false;

  if (!h263.m_h263Options.HasOptionalField(H245_H263Options::e_customPictureFormat))
    return false;

  if (h263.m_h263Options.m_customPictureFormat.GetSize() == 0)
    return false;

  minWidth = minHeight = INT_MAX;
  maxWidth = maxHeight = maxMPI = 0;

  for (PINDEX i = 0; i < h263.m_h263Options.m_customPictureFormat.GetSize(); ++i) {
    const H245_CustomPictureFormat & customPicture = h263.m_h263Options.m_customPictureFormat[i];
    if (!customPicture.m_mPI.HasOptionalField(H245_CustomPictureFormat_mPI::e_standardMPI))
      continue;

    int mpi = customPicture.m_mPI.m_standardMPI;
    if (!IsValidMPI(mpi))
      continue;
    if (maxMPI < mpi)
      maxMPI = mpi;

    int width  = customPicture.m_minCustomPictureWidth;
    if (minWidth > width)
      minWidth = width;

    int height = customPicture.m_minCustomPictureHeight;
    if (minHeight > height)
      minHeight = height;

    width  = customPicture.m_maxCustomPictureWidth;
    if (maxWidth < width)
      maxWidth = width;

    height = customPicture.m_maxCustomPictureHeight;
    if (maxHeight < height)
      maxHeight = height;

    if (!option.IsEmpty())
      option += ';';
    option.sprintf("%u,%u,%u", width, height, mpi);
  }

  return !option.IsEmpty();
}


PBoolean H323H263PluginCapability::IsMatch(const PASN_Choice & subTypePDU) const
{
  if (!H323Capability::IsMatch(subTypePDU))
    return false;

  H245_VideoCapability & video    = (H245_VideoCapability &)subTypePDU;
  H245_H263VideoCapability & h263 = (H245_H263VideoCapability &)video;

  const OpalMediaFormat & mediaFormat = GetMediaFormat();
  int minWidth  = mediaFormat.GetOptionInteger(OpalVideoFormat::MinRxFrameWidthOption());
  int minHeight = mediaFormat.GetOptionInteger(OpalVideoFormat::MinRxFrameHeightOption());
  int maxWidth  = mediaFormat.GetOptionInteger(OpalVideoFormat::MaxRxFrameWidthOption());
  int maxHeight = mediaFormat.GetOptionInteger(OpalVideoFormat::MaxRxFrameHeightOption());

  PString dummy;
  int other_minWidth, other_minHeight, other_maxWidth, other_maxHeight, other_customMPI;
  if (!OnReceivedCustomMPI(h263, other_minWidth, other_minHeight, other_maxWidth, other_maxHeight, other_customMPI, dummy)) {
    other_minWidth = other_minHeight = INT_MAX;
    other_maxWidth = other_maxHeight = 0;
  }

  static struct {
    int tag;
    int width;
    int height;
  } const Table[] = {
    { H245_H263VideoCapability::e_sqcifMPI, PVideoFrameInfo::SQCIFWidth, PVideoFrameInfo::SQCIFHeight },
    { H245_H263VideoCapability::e_qcifMPI,  PVideoFrameInfo::QCIFWidth,  PVideoFrameInfo::QCIFHeight  },
    { H245_H263VideoCapability::e_cifMPI,   PVideoFrameInfo::CIFWidth,   PVideoFrameInfo::CIFHeight   },
    { H245_H263VideoCapability::e_cif4MPI,  PVideoFrameInfo::CIF4Width,  PVideoFrameInfo::CIF4Height  },
    { H245_H263VideoCapability::e_cif16MPI, PVideoFrameInfo::CIF16Width, PVideoFrameInfo::CIF16Height }
  };
  for (PINDEX i = 0; i < PARRAYSIZE(Table); ++i) {
    if (h263.HasOptionalField(Table[i].tag)) {
      if (other_minWidth > Table[i].width)
        other_minWidth = Table[i].width;
      if (other_maxWidth < Table[i].width)
        other_maxWidth = Table[i].width;
      if (other_minHeight > Table[i].height)
        other_minHeight = Table[i].height;
      if (other_maxHeight < Table[i].height)
        other_maxHeight = Table[i].height;
    }
  }

  if (other_maxWidth  < minWidth  || other_minWidth  > maxWidth  || other_maxWidth  < other_minWidth ||
      other_maxHeight < minHeight || other_minHeight > maxHeight || other_maxHeight < other_minHeight) {
    PTRACE(5, "H.263\tNo match:\n" << setw(-1) << *this << '\n' << h263);
    return false;
  }

  PTRACE(5, "H.263\tIsMatch for plug in");
  return true;
}


PBoolean H323H263PluginCapability::OnReceivedPDU(const H245_VideoCapability & cap)
{
  if (cap.GetTag() != H245_VideoCapability::e_h263VideoCapability)
    return false;

  OpalMediaFormat & mediaFormat = GetWritableMediaFormat();

  bool formatDefined = false;

  const H245_H263VideoCapability & h263 = cap;

  if (!SetReceivedH263Cap(mediaFormat, cap, sqcifMPI_tag, H245_H263VideoCapability::e_sqcifMPI, h263.m_sqcifMPI, PVideoFrameInfo::SQCIFWidth, PVideoFrameInfo::SQCIFHeight, formatDefined)) {
    PTRACE(5, "H263\tSetReceivedH263Cap SQCIF failed");
    return false;
  }

  if (!SetReceivedH263Cap(mediaFormat, cap, qcifMPI_tag,  H245_H263VideoCapability::e_qcifMPI,  h263.m_qcifMPI,  PVideoFrameInfo::QCIFWidth, PVideoFrameInfo::QCIFHeight,  formatDefined)) {
    PTRACE(5, "H263\tSetReceivedH263Cap QCIF failed");
    return false;
  }

  if (!SetReceivedH263Cap(mediaFormat, cap, cifMPI_tag,   H245_H263VideoCapability::e_cifMPI,   h263.m_cifMPI,   PVideoFrameInfo::CIFWidth, PVideoFrameInfo::CIFHeight,   formatDefined)) {
    PTRACE(5, "H263\tSetReceivedH263Cap CIF failed");
    return false;
  }

  if (!SetReceivedH263Cap(mediaFormat, cap, cif4MPI_tag,  H245_H263VideoCapability::e_cif4MPI,  h263.m_cif4MPI,  PVideoFrameInfo::CIF4Width, PVideoFrameInfo::CIF4Height,  formatDefined)) {
    PTRACE(5, "H263\tSetReceivedH263Cap CIF4 failed");
    return false;
  }

  if (!SetReceivedH263Cap(mediaFormat, cap, cif16MPI_tag, H245_H263VideoCapability::e_cif16MPI, h263.m_cif16MPI, PVideoFrameInfo::CIF16Width, PVideoFrameInfo::CIF16Height, formatDefined)) {
    PTRACE(5, "H263\tSetReceivedH263Cap CIF16 failed");
    return false;
  }

  PString optionValue;
  int minWidth, minHeight, maxWidth, maxHeight, mpi;
  if (OnReceivedCustomMPI(h263, minWidth, minHeight, maxWidth, maxHeight, mpi, optionValue)) {
    formatDefined = true;
    SET_OR_CREATE_PARM(MaxRxFrameWidthOption, maxWidth, <);
    SET_OR_CREATE_PARM(MinRxFrameWidthOption, minWidth, >);
    SET_OR_CREATE_PARM(MaxRxFrameHeightOption, maxHeight, <);
    SET_OR_CREATE_PARM(MinRxFrameHeightOption, minHeight, >);
    mediaFormat.SetOptionInteger(OpalVideoFormat::FrameTimeOption(), OpalMediaFormat::VideoClockRate * 100 * mpi / 2997);
    mediaFormat.SetOptionString(PLUGINCODEC_CUSTOM_MPI, optionValue);
    PTRACE(4, "H263\tCustom sizes decoded: " << optionValue);
  }

  if (!formatDefined) {
    PTRACE(5, "H263\tFormat !defined");
    return false;
  }

  unsigned maxBitRate = h263.m_maxBitRate*100;
  if (!mediaFormat.SetOptionInteger(OpalMediaFormat::MaxBitRateOption(), maxBitRate)) {
    PTRACE(5, "H263\tCannot set MaxBitRateOption");
    return false;
  }
  unsigned targetBitRate = mediaFormat.GetOptionInteger(OpalVideoFormat::TargetBitRateOption());
  if (targetBitRate > maxBitRate)
    mediaFormat.SetOptionInteger(OpalVideoFormat::TargetBitRateOption(),     maxBitRate);

  mediaFormat.SetOptionBoolean(h323_unrestrictedVector_tag,                h263.m_unrestrictedVector);
  mediaFormat.SetOptionBoolean(h323_arithmeticCoding_tag,                  h263.m_arithmeticCoding);
  mediaFormat.SetOptionBoolean(h323_advancedPrediction_tag,                h263.m_advancedPrediction);
  mediaFormat.SetOptionBoolean(h323_pbFrames_tag,                          h263.m_pbFrames);
  mediaFormat.SetOptionBoolean(h323_errorCompensation_tag,                 h263.m_errorCompensation);
  mediaFormat.SetOptionBoolean(h323_temporalSpatialTradeOffCapability_tag, h263.m_temporalSpatialTradeOffCapability);

  if (h263.HasOptionalField(H245_H263VideoCapability::e_hrd_B))
    mediaFormat.SetOptionInteger(h323_hrdB_tag, h263.m_hrd_B);

  if (h263.HasOptionalField(H245_H263VideoCapability::e_bppMaxKb))
    mediaFormat.SetOptionInteger(h323_bppMaxKb_tag, h263.m_bppMaxKb);

  mediaFormat.SetOptionBoolean(H263_ANNEX_F, h263.m_advancedPrediction);
  if (h263.HasOptionalField(H245_H263VideoCapability::e_h263Options)) {
    mediaFormat.SetOptionBoolean(H263_ANNEX_I, h263.m_h263Options.m_advancedIntraCodingMode);
    mediaFormat.SetOptionBoolean(H263_ANNEX_J, h263.m_h263Options.m_deblockingFilterMode);
    mediaFormat.SetOptionBoolean(H263_ANNEX_T, h263.m_h263Options.m_modifiedQuantizationMode);
  }
  else {
    mediaFormat.SetOptionBoolean(H263_ANNEX_I, false);
    mediaFormat.SetOptionBoolean(H263_ANNEX_J, false);
    mediaFormat.SetOptionBoolean(H263_ANNEX_T, false);
  }

//PStringStream str; mediaFormat.PrintOptions(str);
//PTRACE(4, "OpalPlugin\tCreated H.263 cap from incoming PDU with format " << mediaFormat << " and options\n" << str);

  return true;
}

H323Capability * CreateH263Cap(const PluginCodec_Definition * codecDefn,
                               const OpalMediaFormat & mediaFormat,
                               int /*subType*/) 
{
  PTRACE(4, "OpalPlugin\tCreating H.263 plugin capability");
  return new H323H263PluginCapability(codecDefn, mediaFormat);
}


/////////////////////////////////////////////////////////////////////////////

H323CodecPluginNonStandardVideoCapability::H323CodecPluginNonStandardVideoCapability(const PluginCodec_Definition * codecDefn,
                                                                                     const OpalMediaFormat & mediaFormat,
                                                                                     H323NonStandardCapabilityInfo::CompareFuncType compareFunc,
                                                                                     const unsigned char * data, unsigned dataLen)
 : H323NonStandardVideoCapability(compareFunc, data, dataLen), 
   H323PluginCapabilityInfo(codecDefn, mediaFormat)
{
  PluginCodec_H323NonStandardCodecData * nonStdData = (PluginCodec_H323NonStandardCodecData *)codecDefn->h323CapabilityData;
  if (nonStdData->objectId != NULL) {
    oid = PString(nonStdData->objectId);
  } else {
    t35CountryCode   = nonStdData->t35CountryCode;
    t35Extension     = nonStdData->t35Extension;
    manufacturerCode = nonStdData->manufacturerCode;
  }
}

H323CodecPluginNonStandardVideoCapability::H323CodecPluginNonStandardVideoCapability(const PluginCodec_Definition * codecDefn,
                                                                                     const OpalMediaFormat & mediaFormat,
                                                                                     const unsigned char * data, unsigned dataLen)
 : H323NonStandardVideoCapability(data, dataLen), 
   H323PluginCapabilityInfo(codecDefn, mediaFormat)
{
  PluginCodec_H323NonStandardCodecData * nonStdData = (PluginCodec_H323NonStandardCodecData *)codecDefn->h323CapabilityData;
  if (nonStdData->objectId != NULL) {
    oid = PString(nonStdData->objectId);
  } else {
    t35CountryCode   = nonStdData->t35CountryCode;
    t35Extension     = nonStdData->t35Extension;
    manufacturerCode = nonStdData->manufacturerCode;
  }
}

PObject * H323CodecPluginNonStandardVideoCapability::Clone() const
{
  return new H323CodecPluginNonStandardVideoCapability(*this);
}

PString H323CodecPluginNonStandardVideoCapability::GetFormatName() const
{
  return H323PluginCapabilityInfo::GetFormatName();
}


/////////////////////////////////////////////////////////////////////////////

H323CodecPluginGenericVideoCapability::H323CodecPluginGenericVideoCapability(const PluginCodec_Definition * codecDefn,
                                                                             const OpalMediaFormat & mediaFormat,
                                                                             const PluginCodec_H323GenericCodecData *data)
  : H323GenericVideoCapability(data->standardIdentifier, data != NULL ? data->maxBitRate : 0),
    H323PluginCapabilityInfo(codecDefn, mediaFormat)
{
}

PObject * H323CodecPluginGenericVideoCapability::Clone() const
{
  return new H323CodecPluginGenericVideoCapability(*this);
}

PString H323CodecPluginGenericVideoCapability::GetFormatName() const
{
  return H323PluginCapabilityInfo::GetFormatName();
}


H323Capability *CreateGenericVideoCap(const PluginCodec_Definition * codecDefn,
                                      const OpalMediaFormat & mediaFormat,
                                      int /*subType*/) 
{
  return new H323CodecPluginGenericVideoCapability(codecDefn, mediaFormat, (PluginCodec_H323GenericCodecData *)codecDefn->h323CapabilityData);
}


H323Capability * CreateNonStandardVideoCap(const PluginCodec_Definition * codecDefn,
                                           const OpalMediaFormat & mediaFormat,
                                           int /*subType*/) 
{
  PluginCodec_H323NonStandardCodecData * pluginData =  (PluginCodec_H323NonStandardCodecData *)codecDefn->h323CapabilityData;
  if (pluginData == NULL)
    return new H323CodecPluginNonStandardVideoCapability(codecDefn,
                                                         mediaFormat,
                                                         (const unsigned char *)codecDefn->descr, 
                                                         strlen(codecDefn->descr));

  if (pluginData->capabilityMatchFunction != NULL) 
    return new H323CodecPluginNonStandardVideoCapability(codecDefn,
                                                         mediaFormat,
                                                         (H323NonStandardCapabilityInfo::CompareFuncType)pluginData->capabilityMatchFunction,
                                                         pluginData->data, pluginData->dataLength);

  return new H323CodecPluginNonStandardVideoCapability(codecDefn,
                                                       mediaFormat,
                                                       pluginData->data,
                                                       pluginData->dataLength);
}


#endif  // OPAL_VIDEO


///////////////////////////////////////////////////////////////////////////////

#if OPAL_T38_CAPABILITY

H323Capability * CreateT38Cap(const PluginCodec_Definition * /*codecDefn*/,
                              const OpalMediaFormat & /*mediaFormat*/,
                              int /*subType*/) 
{
  return new H323_T38Capability(H323_T38Capability::e_UDP);
}

#endif


///////////////////////////////////////////////////////////////////////////////

class H323CodecPluginCapabilityMapEntry {
  public:
    int pluginCapType;
    int h323SubType;
    H323Capability * (* createFunc)(const PluginCodec_Definition * codecDefn, const OpalMediaFormat & mediaFormat, int subType);
};

// Disambiguate table entries for video
enum {
  PluginCodec_H323Codec_nonStandardVideo = PluginCodec_H323Codec_NoH323+1,
  PluginCodec_H323Codec_genericVideo
};

static H323CodecPluginCapabilityMapEntry H323CapabilityMaps[] = {
  { PluginCodec_H323Codec_nonStandard,              H245_AudioCapability::e_nonStandard,            &CreateNonStandardAudioCap },
  { PluginCodec_H323AudioCodec_gsmFullRate,	        H245_AudioCapability::e_gsmFullRate,            &CreateGSMCap },
  { PluginCodec_H323AudioCodec_gsmHalfRate,	        H245_AudioCapability::e_gsmHalfRate,            &CreateGSMCap },
  { PluginCodec_H323AudioCodec_gsmEnhancedFullRate, H245_AudioCapability::e_gsmEnhancedFullRate,    &CreateGSMCap },
  { PluginCodec_H323AudioCodec_g711Alaw_64k,        H245_AudioCapability::e_g711Alaw64k,            &CreateStandardAudioCap },
  { PluginCodec_H323AudioCodec_g711Alaw_56k,        H245_AudioCapability::e_g711Alaw56k,            &CreateStandardAudioCap },
  { PluginCodec_H323AudioCodec_g711Ulaw_64k,        H245_AudioCapability::e_g711Ulaw64k,            &CreateStandardAudioCap },
  { PluginCodec_H323AudioCodec_g711Ulaw_56k,        H245_AudioCapability::e_g711Ulaw56k,            &CreateStandardAudioCap },
  { PluginCodec_H323AudioCodec_g7231,               H245_AudioCapability::e_g7231,                  &CreateG7231Cap },
  { PluginCodec_H323AudioCodec_g729,                H245_AudioCapability::e_g729,                   &CreateStandardAudioCap },
  { PluginCodec_H323AudioCodec_g729AnnexA,          H245_AudioCapability::e_g729AnnexA,             &CreateStandardAudioCap },
  { PluginCodec_H323AudioCodec_g728,                H245_AudioCapability::e_g728,                   &CreateStandardAudioCap }, 
  { PluginCodec_H323AudioCodec_g722_64k,            H245_AudioCapability::e_g722_64k,               &CreateStandardAudioCap },
  { PluginCodec_H323AudioCodec_g722_56k,            H245_AudioCapability::e_g722_56k,               &CreateStandardAudioCap },
  { PluginCodec_H323AudioCodec_g722_48k,            H245_AudioCapability::e_g722_48k,               &CreateStandardAudioCap },
  { PluginCodec_H323AudioCodec_g729wAnnexB,         H245_AudioCapability::e_g729wAnnexB,            &CreateStandardAudioCap }, 
  { PluginCodec_H323AudioCodec_g729AnnexAwAnnexB,   H245_AudioCapability::e_g729AnnexAwAnnexB,      &CreateStandardAudioCap },
  { PluginCodec_H323Codec_generic,                  H245_AudioCapability::e_genericAudioCapability, &CreateGenericAudioCap },

  // not implemented
  //{ PluginCodec_H323AudioCodec_g729Extensions,      H245_AudioCapability::e_g729Extensions   },
  //{ PluginCodec_H323AudioCodec_g7231AnnexC,         H245_AudioCapability::e_g7231AnnexCMode  },
  //{ PluginCodec_H323AudioCodec_is11172,             H245_AudioCapability::e_is11172AudioMode },
  //{ PluginCodec_H323AudioCodec_is13818Audio,        H245_AudioCapability::e_is13818AudioMode },

#if OPAL_VIDEO

  // video codecs
  { PluginCodec_H323Codec_nonStandardVideo,         H245_VideoCapability::e_nonStandard,            &CreateNonStandardVideoCap },
  { PluginCodec_H323VideoCodec_h261,                H245_VideoCapability::e_h261VideoCapability,    &CreateH261Cap },
  { PluginCodec_H323VideoCodec_h263,                H245_VideoCapability::e_h263VideoCapability,    &CreateH263Cap },
  { PluginCodec_H323Codec_genericVideo,             H245_VideoCapability::e_genericVideoCapability, &CreateGenericVideoCap },
/*
  PluginCodec_H323VideoCodec_h262,                // not yet implemented
  PluginCodec_H323VideoCodec_is11172,             // not yet implemented
*/

#endif  // OPAL_VIDEO

#if OPAL_T38_CAPABILITY
  { PluginCodec_H323T38Codec,                       H245_DataApplicationCapability_application::e_t38fax, &CreateT38Cap },
#endif

  { -1 }
};

void OpalPluginCodecManager::RegisterCapability(const PluginCodec_Definition * codecDefn)
{
  int capabilityType = codecDefn->h323CapabilityType;
  if (capabilityType == PluginCodec_H323Codec_NoH323 || capabilityType == PluginCodec_H323Codec_undefined)
    return;

  OpalPluginControl isValid(codecDefn, PLUGINCODEC_CONTROL_VALID_FOR_PROTOCOL);
  if (isValid.Exists() && !isValid.Call((void *)"h323", sizeof(const char *))) {
    PTRACE(2, "OpalPlugin\tNot adding H.323 capability for plugin codec " << codecDefn->descr << " as this has been specifically disabled");
    return;
  }

  if ((codecDefn->flags & PluginCodec_MediaTypeMask) == PluginCodec_MediaTypeVideo) {
    switch (capabilityType) {
      case PluginCodec_H323Codec_nonStandard :
        capabilityType = PluginCodec_H323Codec_nonStandardVideo;
        break;
      case PluginCodec_H323Codec_generic :
        capabilityType = PluginCodec_H323Codec_genericVideo;
        break;
    }
  }

  // add the capability
  for (PINDEX i = 0; H323CapabilityMaps[i].pluginCapType >= 0; i++) {
    if (H323CapabilityMaps[i].pluginCapType == capabilityType) {
      OpalMediaFormat mediaFormat = codecDefn->destFormat;
      if (!mediaFormat.IsTransportable())
        mediaFormat = codecDefn->sourceFormat;

      H323Capability * cap = NULL;
      if (H323CapabilityMaps[i].createFunc != NULL)
        cap = (*H323CapabilityMaps[i].createFunc)(codecDefn, mediaFormat, H323CapabilityMaps[i].h323SubType);
      else {
        PTRACE(2, "OpalPlugin\tNo capability creation function for " << codecDefn->descr);
      }

      // manually register the new singleton type, as we do not have a concrete type
      if (cap != NULL) {
        H323CapabilityFactory::Register(mediaFormat.GetName(), cap);
      }
      else {
        PTRACE(2, "OpalPlugin\tNo capability created for " << codecDefn->descr);
      }
      break;
    }
  }
}

#endif // OPAL_H323

/////////////////////////////////////////////////////////////////////////////

#define INCLUDE_STATIC_CODEC(name) \
extern "C" { \
extern unsigned int Opal_StaticCodec_##name##_GetAPIVersion(); \
extern struct PluginCodec_Definition * Opal_StaticCodec_##name##_GetCodecs(unsigned *,unsigned); \
}; \
class H323StaticPluginCodec_##name : public H323StaticPluginCodec \
{ \
  public: \
    PluginCodec_GetAPIVersionFunction Get_GetAPIFn() \
    { return &Opal_StaticCodec_##name##_GetAPIVersion; } \
    PluginCodec_GetCodecFunction Get_GetCodecFn() \
    { return &Opal_StaticCodec_##name##_GetCodecs; } \
}; \
static H323StaticPluginCodecFactory::Worker<H323StaticPluginCodec_##name > static##name##CodecFactory( #name ); \

#ifdef H323_EMBEDDED_GSM

INCLUDE_STATIC_CODEC(GSM_0610)

#endif

