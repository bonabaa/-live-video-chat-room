#include "StdAfx.h"
#include ".\ycoreaudiomgr.h"

yCoreAudioMgr::yCoreAudioMgr(void)
{
}

yCoreAudioMgr::~yCoreAudioMgr(void)
{
}
bool yCoreAudioMgr::YY_transport_my_ReadPacket(AudioDataEx& data)
{
	bool bResult = true;
	m_mutexDatalist.Wait();
	if (m_yyDatalist.size()>0)
	{
		data = m_yyDatalist.front();
    m_yyDatalist.front().data=0;
		m_yyDatalist.pop_front();
  }else{
		bResult = false;
    data.len=0;
  }
	m_mutexDatalist.Signal();
	return bResult;
}

void yCoreAudioMgr::yyDestroyAudioDatalist()
{
	m_mutexDatalist.Wait();
  for(std::list<AudioDataEx>::iterator i=m_yyDatalist.begin();i!= m_yyDatalist.end();i++ ){
    delete i->data ;
  }
  m_yyDatalist.clear();
	m_mutexDatalist.Signal();
}
int  yCoreAudioMgr::YY_error_callback_error_handler(int errCode)
{
  if (errCode == 10020){
    //initAudio();
  }
		PTRACE(3,"YYAudioMediaStream\an error_handler called, errcode is ." <<  errCode);
    return 0;
}
//by Ivoice engine send
void  yCoreAudioMgr::YY_transport_my_SendPacket(int channel, const void *data, int len)
{
	m_mutexDatalist.Wait();
	AudioDataEx d;
	d.data = new char[len];
	memcpy(d.data, data, len);
	d.len= len;
  if (m_yyDatalist.size()> 16) { delete m_yyDatalist.front().data; m_yyDatalist.front().data=0;m_yyDatalist.pop_front();};
	m_yyDatalist.push_back(d);
	m_mutexDatalist.Signal();
  d.data=0;//delete when poped data 
	//sprintf(szValue,"transport data len is %d, channel is is %d\n", len, channel);
	//OutputDebugStr(szValue);
};
 