// bbqmfcex.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
void g_socketClose(unsigned int& pChannel){

} 
 int g_GetErrorNumber(const unsigned int pChannel, int errorcode)
{
  return 0;
}
unsigned int g_udpWriteTo(const void * buf, int len , const unsigned int pChannel)
{
  return 0;
}
unsigned int g_udpRecvFrom(void * buf, int len,unsigned int& addr, unsigned int&port, const unsigned int pChannel)
{
  return 0;
}
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}

