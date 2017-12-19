// GipsVoiceEngineLib.h
//
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// 
//  Created by: Fredrik Galschiödt
//  Date      : 011202
//
//	Public API for GIPS Voice Engine on a PC platform.
// 
//  Copyright (c) 2001
//  Global IP Sound AB, Organization number: 5565739017
//  Rosenlundsgatan 54, SE-118 63 Stockholm, Sweden
//  All rights reserved.
//  
//////////////////////////////////////////////////////////////////////

#ifndef PUBLIC_GIPS_VOICE_ENGINE_LIB_H
#define PUBLIC_GIPS_VOICE_ENGINE_LIB_H

#ifdef GIPS_EXPORT
#define VOICEENGINE_DLLEXPORT _declspec(dllexport)
#elif GIPS_DLL
#define VOICEENGINE_DLLEXPORT _declspec(dllimport)
#else
#define VOICEENGINE_DLLEXPORT
#endif




// Constants for SRTP


#define CIPHER_NULL 0
#define CIPHER_AES_128_COUNTER_MODE 1

#define AUTH_NULL 0
#define AUTH_HMAC_SHA1 3

#define NO_PROTECTION 0
#define ENCRYPTION 1
#define AUTHENTICATION 2
#define ENCRYPTION_AND_AUTHENTICATION 3



// Constants for file playout

#define FILE_FORMAT_PCM_FILE 0
#define FILE_FORMAT_WAV_FILE 1
#define FILE_FORMAT_COMPRESSED_FILE 2

// Constants for AMRcodec
#define GIPSAMR_RFC3267_BWEFFICIENT		0
#define GIPSAMR_RFC3267_OCTETALIGNED	1
#define GIPSAMR_RFC3267_FILESTORAGE		2


//////////////////////////////////////////////////////////////////////
// GIPSVE_CodecInst
//
// Each codec supported by the GIPS VoiceEngine can be 
// described by this structure.
//
// The following codecs are supported today:
//
// - G.711 u-Law
// - G.711 a-Law
// - GIPS Enhanced G.711 u-Law
// - GIPS Enhanced G.711 A-Law
// - GIPS iPCM-wb
// - GIPS iLBC
// - GIPS iSAC
//
// The following codec are supported if an external implementation
// is provided. Note that GIPS do not provide the patent rights for 
// these codecs.
// 
// - G.729
// - G.723.1
// - G.722
// - G.722.1
// - GSM fullrate
// - GSM AMR
//
// Note that the GIPS NetEQ is included on the receiving side for all
// codecs. NetEQ is a patented GIPS technology that adaptively compen-,
// sates for jitter, and at the same time conceals errors due to lost
// packets. NetEQ delivers improvements in sound quality while mini-
// mizing buffering latency.
//////////////////////////////////////////////////////////////////////

struct GIPSVE_CodecInst
{
	int pltype;
	char plname[32];
	int plfreq;
	int pacsize;
	int channels;
	int rate; // Only needed for AMR codec and iSAC for non adaptive mode
};

//////////////////////////////////////////////////////////////////////
// GIPSVE_PTTState
//////////////////////////////////////////////////////////////////////

struct GIPSVE_PTTState
{
	int				port;			// remote source port
	char			address[16];	// remote IP address
	unsigned long	SSRC;			// remote SSRC
};

//////////////////////////////////////////////////////////////////////
// GIPSVE_CallStatistics
//////////////////////////////////////////////////////////////////////

struct GIPSVE_CallStatistics
{
	unsigned short fraction_lost;
	unsigned long cum_lost;
	unsigned long ext_max;
	unsigned long jitter;
	int RTT;
	int bytesSent;
	int packetsSent;
	int bytesReceived;
	int packetsReceived;
};


// GIPS_encryption
// This is a class that should be overloaded to enable encryption

class VOICEENGINE_DLLEXPORT GIPS_encryption
{
public:
    virtual void encrypt(int channel_no, unsigned char * in_data, 
		unsigned char * out_data, int bytes_in, int * bytes_out) = 0;

    virtual void decrypt(int channel_no, unsigned char * in_data, 
		unsigned char * out_data, int bytes_in, int * bytes_out) = 0;

	virtual void encrypt_rtcp(int channel_no, unsigned char * in_data, 
		unsigned char * out_data, int bytes_in, int * bytes_out) = 0;

    virtual void decrypt_rtcp(int channel_no, unsigned char * in_data, 
		unsigned char * out_data, int bytes_in, int * bytes_out) = 0;

	virtual ~GIPS_encryption();
};

// External transport protocol
// This is a class that should be implemented by the customer IF
// a different transport protocol than IP/UDP/RTP is wanted. The
// standard data transport protocol used by VoiceEngine is IP/UDP/RTP
// according to the SIP-standard.
class VOICEENGINE_DLLEXPORT GIPS_transport
{
public:
    virtual void SendPacket(int channel, const void *data, int len) = 0;
	virtual ~GIPS_transport();
};

// External read and write functions
// This is a class that should be implemented by the customer IF
// the functions GIPSVE_ConvertWavToPCM() or GIPSVE_PlayPCM() are used

class VOICEENGINE_DLLEXPORT InStream {
public:
	virtual int Read(void *buf,int len) = 0;
	// len - size in bytes that should be read
	// returns the size in bytes read (=len before end and =[0..len-1] at end similar to fread)


	// Only internal usage
	virtual int Rewind();

	// Destructor
	virtual ~InStream();
};
class VOICEENGINE_DLLEXPORT OutStream {
public:
	virtual bool Write(const void *buf,int len) = 0;
	// true for ok, false for fail

	// Only internal usage
	virtual int Rewind();

	// Destructor
	virtual ~OutStream();
};

// GIPS_media_process
// This is a class that should be overloaded to modify audio in recording or playout path
class VOICEENGINE_DLLEXPORT GIPS_media_process
{
public:
    virtual void process(int channel_no, short* audio_10ms_16kHz, int len, int sampfreq) = 0;
	virtual ~GIPS_media_process();
};
// Constants for external media processing
#define PLAYBACK_PER_CHANNEL 0
#define PLAYBACK_ALL_CHANNELS_MIXED 1
#define RECORDING_PER_CHANNEL 2
#define RECORDING_ALL_CHANNELS_MIXED 3


//////////////////////////////////////////////////////////////////////
// error_callback
//
// Delivers runtime error that are not a direct effect of an API call
//////////////////////////////////////////////////////////////////////

class VOICEENGINE_DLLEXPORT error_callback
{
public:
    virtual void error_handler(int errCode) = 0;

	// Destructor
	virtual ~error_callback();
};

// VQmon alert notification handler function prototype.
typedef void (*vqmon_alert)(int channel,void* pAlertDesc);

//////////////////////////////////////////////////////////////////////
// GipsVoiceEngineLib
//
// Public interface to the GIPS Voice Engine for PC platforms
//////////////////////////////////////////////////////////////////////

#ifndef NULL
#define NULL 0L
#endif

class VOICEENGINE_DLLEXPORT GipsVoiceEngineLib
{
public:
	virtual int GIPSVE_Init() = 0;
	virtual int GIPSVE_SetNetworkStatus(int networktype) = 0;
	virtual int GIPSVE_GetNetworkStatus() = 0;
	virtual int GIPSVE_SetVADStatus(int mode) = 0;
	virtual int GIPSVE_GetVADStatus() = 0;
	virtual int GIPSVE_SetECStatus(int mode) = 0;
	virtual int GIPSVE_GetECStatus() = 0;
	virtual int GIPSVE_GetECActivity() = 0;
	virtual int GIPSVE_CreateChannel() = 0;
	virtual int GIPSVE_DeleteChannel(int channel) = 0;
	virtual int GIPSVE_GetCodec(short listnr, GIPSVE_CodecInst *codec_inst) = 0;
	virtual int GIPSVE_GetNofCodecs() = 0;
	virtual int GIPSVE_SetSendCodec(int channel, GIPSVE_CodecInst *codec_inst) = 0;
	virtual int GIPSVE_GetCurrentSendCodec(short channel, GIPSVE_CodecInst *gipsve_inst) = 0;
	virtual int GIPSVE_GetRecCodec(int channel, GIPSVE_CodecInst *recCodec) = 0;
	//virtual int GIPSVE_SetRecCodec(int channel, GIPSVE_CodecInst *codec_inst) = 0;
	virtual int GIPSVE_SetRecPort(int channel, int portnr, char * multiCastAddr = NULL, char * ip = NULL) = 0;
	virtual int GIPSVE_GetRecPort(int channel) = 0;
	virtual int GIPSVE_SetSendPort(int channel, int portnr) = 0;
	virtual int GIPSVE_SetSrcPort(int channel, int portnr) = 0;
	virtual int GIPSVE_GetSendPort(int channel) = 0;
	virtual int GIPSVE_SetSendIP(int channel, char *ipadr) = 0;
	virtual int GIPSVE_GetSendIP(int channel, char *ipadr, int bufsize) = 0;
	virtual int GIPSVE_StartListen(int channel) = 0;
	virtual int GIPSVE_StartPlayout(int channel) = 0;
	virtual int GIPSVE_StartSend(int channel) = 0;
	virtual int GIPSVE_StopListen(int channel) = 0;
	virtual int GIPSVE_StopPlayout(int channel) = 0;
	virtual int GIPSVE_StopSend(int channel) = 0;
	virtual int GIPSVE_GetLastError() = 0;
	virtual int GIPSVE_SetSpeakerVolume(unsigned int level) = 0;
	virtual int GIPSVE_GetSpeakerVolume() = 0;
	virtual int GIPSVE_SetMicVolume(unsigned int level) = 0;
	virtual int GIPSVE_GetMicVolume() = 0;
	virtual int GIPSVE_SetChannelOutputVolumeScale(int channel, float level) = 0;
	virtual float GIPSVE_GetChannelOutputVolumeScale(int channel) = 0;
	virtual int GIPSVE_SetAGCStatus(int mode) = 0;
	virtual int GIPSVE_GetAGCStatus() = 0;
	virtual int GIPSVE_SetNRStatus(int mode) = 0;
	virtual int GIPSVE_GetNRStatus() = 0;
	//virtual int GIPSVE_SetNRframes(int frames) = 0;
	virtual int GIPSVE_SetNRpolicy(int mode) = 0;
	virtual int GIPSVE_GetVersion(char *version, int buflen) = 0;
	virtual int GIPSVE_Terminate() = 0;
	virtual int GIPSVE_SetDTMFPayloadType(int channel, int payloadType) = 0;
	virtual int GIPSVE_SendDTMF(int channel, int eventnr, int inBand) = 0;
	virtual int GIPSVE_PlayDTMFTone(int eventnr) = 0;	
	virtual unsigned short GIPSVE_GetFromPort(int channel)=0;
	virtual int GIPSVE_SetFilterPort(int channel,unsigned short filter) = 0;
	virtual unsigned short GIPSVE_GetFilterPort(int channel) = 0;
	virtual int GIPSVE_SetRecPayloadType(short channel, GIPSVE_CodecInst *codec_inst)=0;
	virtual int GIPSVE_SetSendCNPayloadType(int channel,short payloadType)=0;
	virtual int GIPSVE_RTCPStat(int channel, unsigned short *fraction_lost, unsigned long *cum_lost, unsigned long *ext_max, unsigned long *jitter, int *RTT)=0;
	virtual int GIPSVE_RTCPStat(int channel, GIPSVE_CallStatistics * stats) =0;
	virtual int GIPSVE_SetSoundDevices(unsigned int WaveInDevice, unsigned int WaveOutDevice)= 0;
	virtual int GIPSVE_InitEncryption (GIPS_encryption &encr_obj) = 0;
	virtual int GIPSVE_EnableEncryption(int channel) = 0;
	virtual int GIPSVE_DisableEncryption(int channel) = 0;
	virtual int GIPSVE_SetDTMFFeedbackStatus(int mode) = 0;
	virtual int GIPSVE_GetDTMFFeedbackStatus() = 0;
	virtual int GIPSVE_GetNoOfChannels() = 0;
	virtual int GIPSVE_SetECType(int type) = 0;
	virtual int GIPSVE_GetECType() = 0;
	virtual int GIPSVE_GetInputLevel() = 0;
	virtual int GIPSVE_GetOutputLevel(int channel = -1) = 0;
	virtual int GIPSVE_SetSendTOS(int channel, int TOS) = 0;
	virtual int GIPSVE_GetSendTOS(int channel) = 0;
	virtual int GIPSVE_SetSendGQOS(int channel, bool enable, int servicetype) = 0;
	virtual int GIPSVE_GetSendGQOS(int channel) = 0;
	virtual int GIPSVE_MuteMic(int channel,int Mute) = 0;
	virtual int GIPSVE_PutOnHold(int channel,bool enable) = 0;
	virtual int GIPSVE_AddToConference(int channel,bool enable) = 0;
	virtual int GIPSVE_SetTrace(int mode) = 0;
	virtual int GIPSVE_SetTraceFileName(char * fileName) = 0; 
	virtual int GIPSVE_PlayPCM(int channel, InStream *pcm,int file_format = FILE_FORMAT_PCM_FILE, float volume_scaling = 1.0) = 0;
	virtual int GIPSVE_PlayPCM(int channel, char * fileName, bool loop = false ,int file_format = FILE_FORMAT_PCM_FILE, float volume_scaling = 1.0) = 0;
	virtual int GIPSVE_PlayPCMAsMicrophone(int channel, InStream *pcm, bool mixWithMic = false,int file_format = FILE_FORMAT_PCM_FILE, float volume_scaling = 1.0) = 0;
	virtual int GIPSVE_PlayPCMAsMicrophone(int channel, char * fileName, bool loop = false , bool mixWithMic = false,int file_format = FILE_FORMAT_PCM_FILE, float volume_scaling = 1.0) = 0;
	virtual int GIPSVE_IsPlayingFile(int channel) = 0;
	virtual int GIPSVE_StopPlayingFile(int channel) = 0;
	virtual int GIPSVE_IsPlayingFileAsMicrophone(int channel) = 0;
	virtual int GIPSVE_StopPlayingFileAsMicrophone(int channel) = 0;
	

	virtual int GIPSVE_CheckIfAudioIsAvailable(int checkPlay, int checkRec) = 0;
	
	// Video Synchronization
	virtual unsigned long int GIPSVE_GetPlayoutTimeStamp(int channel) = 0;
	virtual int GIPSVE_SetMinDelay(int channel,int msDelay) = 0;
	virtual int GIPSVE_getRemoteRTCPData(int channel, unsigned long * NTP_high, unsigned long * NTP_low, unsigned long * timeStamp, unsigned long * playoutTimeStamp) = 0;

	// RTCP calls
	virtual int GIPSVE_EnableRTCP(int channel, int enable) = 0;
	virtual int GIPSVE_SetRTCPCNAME(int channel, char * str) = 0;
	virtual int GIPSVE_getRemoteRTCPCNAME(int channel, char * str) = 0;

	// SRTP calls
	virtual int GIPSVE_EnableSRTPSend(int channel,int cipher_type,int cipher_key_len,int auth_type, int auth_key_len,int auth_tag_len, int security, const unsigned char key[16]) = 0;
	virtual int GIPSVE_DisableSRTPSend(int channel) = 0;
	virtual int GIPSVE_EnableSRTPReceive(int channel,int cipher_type,int cipher_key_len,int auth_type, int auth_key_len,int auth_tag_len, int security, const unsigned char key[16]) = 0;
	virtual int GIPSVE_DisableSRTPReceive(int channel) = 0;

	// Push-to-Talk (PTT) calls
	virtual int GIPSVE_StartPTTPlayout(int channel) = 0;
	virtual int GIPSVE_GetPTTActivity(int channel) = 0;
	virtual int GIPSVE_GetPTTSession(int channel, GIPSVE_PTTState *state) = 0;

	// Use these function calls ONLY when a customer specific transport protocol is going to be used
	virtual int GIPSVE_SetSendTransport(int channel, GIPS_transport &transport) = 0;
	virtual int GIPSVE_ReceivedRTPPacket(int channel, const void *data, int len) = 0;

	virtual int SetObserver (error_callback &observer) = 0;

	// Call for dll authentication
	virtual int GIPSVE_Authenticate(char *auth_string, int len) = 0;

	// Record the played audio
	virtual int GIPSVE_StartRecording(int channel,char * fileName,GIPSVE_CodecInst *gipsve_inst = NULL) = 0;
	virtual int GIPSVE_StartRecording(int channel,OutStream *pcm,GIPSVE_CodecInst *gipsve_inst = NULL) = 0;
	virtual int GIPSVE_StopRecording(int channel) = 0;

	// Local recording 
	virtual int GIPSVE_StartRecordingMicrophone(char * fileName,GIPSVE_CodecInst *codec_inst = NULL) = 0;
	virtual int GIPSVE_StartRecordingMicrophone(OutStream *pcm,GIPSVE_CodecInst *codec_inst = NULL) = 0;
	virtual int GIPSVE_StopRecordingMicrophone() = 0;

	// Call recording 
	virtual int GIPSVE_StartRecordingCall(char * fileName,GIPSVE_CodecInst *codec_inst = NULL) = 0;
	virtual int GIPSVE_StartRecordingCall(OutStream *pcm,GIPSVE_CodecInst *codec_inst = NULL) = 0;
	virtual int GIPSVE_PauseRecordingCall(bool isPaused) = 0;	
	virtual int GIPSVE_StopRecordingCall() = 0;

	// File conversion
	virtual int GIPSVE_ConvertWavToPCM(char *InfileName, char *OutfileName) = 0;
	virtual int GIPSVE_ConvertWavToPCM(InStream *wav, OutStream *pcm) = 0;
	virtual int GIPSVE_ConvertPCMToWav(char *InfileName, char *OutfileName) = 0;
	virtual int GIPSVE_ConvertPCMToWav(InStream *pcm, OutStream *wav,int lenghtInBytes) = 0;
	virtual int GIPSVE_ConvertPCMToCompressed(char *InfileName, char *OutfileName,GIPSVE_CodecInst *codec_inst) = 0;
	virtual int GIPSVE_ConvertPCMToCompressed(InStream *in, OutStream *out,GIPSVE_CodecInst *codec_inst) = 0;
	virtual int GIPSVE_ConvertCompressedToPCM(char *InfileName, char *OutfileName) = 0;
	virtual int GIPSVE_ConvertCompressedToPCM(InStream *in, OutStream *out) = 0;

	// VQMon calls
	virtual int GIPSVE_EnableVQMon(int channel, bool enable) = 0;
	virtual int GIPSVE_EnableRTCP_XR(int channel, bool enable) = 0;
	virtual int GIPSVE_GetVoIPMetrics(int channel, unsigned char *dst, unsigned int bufSize) = 0;
	virtual int GIPSVE_InstallAlertHandler(vqmon_alert alert_callback) = 0;
	virtual int GIPSVE_SetAlert(int channel, int type, int param1, int param2, int param3) = 0;
	virtual int GIPSVE_RemoveAlert(int channel, int type) = 0;

	// AMR specific calls
	virtual int GIPSVE_SetAMR_enc_format(int channel,int mode) = 0;
	virtual int GIPSVE_SetAMR_dec_format(int channel,int mode) = 0;

	// Send extra packet over RTP / RTCP channel (no RTP headers added)
	virtual int sendExtraPacket_RTP(int channel, unsigned char* data, int nbytes) = 0;
	virtual int sendExtraPacket_RTCP(int channel, unsigned char* data, int nbytes) = 0;

	// External recording device
	virtual int GIPSVE_Enable_External_Recording(bool enable) = 0;
	virtual int GIPSVE_External_Recording_SendData(short * speech_10ms, int sampFreq, int currentRecordingDelayinMS) = 0;

	// External playout device, instead of sound card
	virtual int GIPSVE_Enable_External_Playout(bool enable) = 0;
	virtual int GIPSVE_GetPlayData(short * speech_10ms, int sampFreq, int currentPlayDelayinMS) = 0;
	
	// External media processing
	virtual int GIPSVE_EnableExternalMediaProcessing(bool enable, int where, int channel, GIPS_media_process &procObj) = 0;

	// RFC 2198 Forward error correctionvirtual 
	virtual int GIPSVE_EnableFEC(int channel, bool enable, int REDpayloadtype = -1) = 0;

	virtual ~GipsVoiceEngineLib();
};

//////////////////////////////////////////////////////////////////////
// Factory method
//////////////////////////////////////////////////////////////////////

VOICEENGINE_DLLEXPORT GipsVoiceEngineLib &GetGipsVoiceEngineLib();

//////////////////////////////////////////////////////////////////////
// Extra instances
//////////////////////////////////////////////////////////////////////

VOICEENGINE_DLLEXPORT GipsVoiceEngineLib * GetNewVoiceEngineLib();



#endif // PUBLIC_GIPS_VOICE_ENGINE_LIB_H
