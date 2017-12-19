#include "StdAfx.h"
#include "bbqclient2011.h"
#include ".\sfbbqclientinterface.h"
#include <map>

#include "GipsVoiceEngineLib.h"

using namespace std;
bool g_bInit=false;
void SFinit()
{
	char szBuff[_MAX_PATH];
	//::GetModuleFileName(NULL, szBuff, _MAX_PATH);
  //if (strstr(szBuff, "CCvision") == 0)
  //  return ;
  if (!g_bInit){
    g_bInit = true;
  }
}
void SFUninit()
{
  if (g_bInit){
    g_bInit = false;
  }

}

SF_HANDLE mm_create()
{
   return 0;
}
void mm_destroy(const SF_HANDLE handle)
{
   return ;
}
 
int mm_getlasterror(const SF_HANDLE handle)
{
  return 0;
}
 //bool mm_callback(const SF_HANDLE handle, OnUserDefineMsg fun)
 //{
 //  return false;
 //}
OnCB yEror_hanle;

class YY_error_callback:public error_callback
{
public:
	virtual void error_handler(int errCode)
  {
    yEror_hanle(errCode);
  };

	// Destructor
	virtual ~YY_error_callback(){};
};
OnSendPacket ySendPacket;
class  YY_transport_my: public GIPS_transport
{
public:
	//void SetMediaStream(YYAudioMediaStream* m){m_MS = m;};
  virtual void SendPacket(int channel, const void *data, int len)
  {
    ySendPacket(channel,data,len);
  };
	virtual ~YY_transport_my()
	{

	};


protected:
};

GipsVoiceEngineLib* m_yyVoice;
YY_error_callback errCallback;
YY_transport_my yyTransport;
int YMM_GetLastError() 
{
  return m_yyVoice->GIPSVE_GetLastError();
}
int YMM_Init()
{
	char szBuff[_MAX_PATH];
	::GetModuleFileName(NULL, szBuff, _MAX_PATH);
  //if (strstr(szBuff, "CCVision") == 0)
  ///  return -1;
    m_yyVoice =  &GetGipsVoiceEngineLib();
    return m_yyVoice->GIPSVE_Init();

}
int YMM_SetSd(unsigned int WaveInDevice, unsigned int WaveOutDevice)
{
  return m_yyVoice->GIPSVE_SetSoundDevices(WaveInDevice, WaveOutDevice);
}
int YMM_SetCB (OnCB fun) 
{
  yEror_hanle= fun;
  return m_yyVoice->SetObserver(errCallback);
}
int YMM_SetVADStyle(int mode) 
{
  return m_yyVoice->GIPSVE_SetVADStatus(mode);
}
int YMM_SetECStyle(int mode) 
{
  return m_yyVoice->GIPSVE_SetECStatus(mode);
}
int YMM_SetECType(int type)
{
  return m_yyVoice->GIPSVE_SetECType(type);
}
int YMM_SetAGCStyle(int mode) 
{
  return m_yyVoice->GIPSVE_SetAGCStatus(mode);
}
int YMM_SetNRStyle(int mode) 
{
  return m_yyVoice->GIPSVE_SetNRStatus(mode);
}
int YMM_CreateStyle() 
{
  return m_yyVoice->GIPSVE_CreateChannel();
}
int YMM_SetSCodec(int channel, int codec_inst) 
{
	#if 1
		GIPSVE_CodecInst codecInst;
		if(m_yyVoice->GIPSVE_GetCodec(codec_inst,&codecInst) == -1)
      return -2;
			//	PTRACE(3,"YYAudioMediaStream\tGet codec failed.");
#else
 GIPSVE_CodecInst codecInst;
 int i=0;
while (m_yyVoice->GIPSVE_GetCodec(i,&codecInst) != -1)
{
	i++;
	
}
#endif
  return m_yyVoice->GIPSVE_SetSendCodec(channel, &codecInst);
}
int YMM_SetSTransport(int channel, OnSendPacket fun) 
{
  ySendPacket= fun;
  return m_yyVoice->GIPSVE_SetSendTransport(channel, yyTransport) ;
}
int YMM_DeleteStyle(int channel) 
{
  return m_yyVoice->GIPSVE_DeleteChannel(channel);
}
int YMM_RPacket(int channel, const void *data, int len) 
{
  return m_yyVoice->GIPSVE_ReceivedRTPPacket(channel, data, len);
}
int YMM_StopSnd(int channel) 
{
  return m_yyVoice->GIPSVE_StopSend(channel);
}
int YMM_Stopout(int channel) 
{
  return m_yyVoice->GIPSVE_StopPlayout(channel);
}
int YMM_StartSnd(int channel) 
{
  return m_yyVoice->GIPSVE_StartSend(channel);
}
int YMM_Startout(int channel) 
{
  return m_yyVoice->GIPSVE_StartPlayout(channel);
}

int YMM_GetICL(int c) 
{
  return m_yyVoice->GIPSVE_GetInputLevel();
}
SF_DLL_API int YMM_GetICL2(int c) 
{
  return m_yyVoice->GIPSVE_GetOutputLevel(c);
}

//SF_DLL_API int YMM_GetData(int type,int* data)
//{
//  switch(type)
//  {
//  case 1:
//    return 
//    break;
//  };
//  return 0;
//}
//SF_DLL_API int YMM_SetData(int type,int data) ;

/*
	virtual int GIPSVE_SetSpeakerVolume(unsigned int level) = 0;
	virtual int GIPSVE_GetSpeakerVolume() = 0;
	virtual int GIPSVE_SetMicVolume(unsigned int level) = 0;
	virtual int GIPSVE_GetMicVolume() = 0;
*/


