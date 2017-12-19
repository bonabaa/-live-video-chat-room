#pragma once
#include <ptlib.h>
#include "sfbbqclientinterface.h"
#include <list>
struct AudioDataEx
{
	char* data;
	int len;
  AudioDataEx()
  {
    data= 0;
  };
  ~AudioDataEx()
  {
    //if (data){
    //  delete data;data=NULL;
    //}
  };
};
class yCoreAudioMgr
{
public:
  yCoreAudioMgr(void);
  ~yCoreAudioMgr(void);
protected:
	  PMutex m_mutexDatalist;
	  std::list<AudioDataEx> m_yyDatalist;
	  bool YY_transport_my_ReadPacket(AudioDataEx& data);
	  void yyDestroyAudioDatalist();
    int YY_error_callback_error_handler(int errCode);
    //by Ivoice engine send
    void YY_transport_my_SendPacket(int channel, const void *data, int len);
 };
