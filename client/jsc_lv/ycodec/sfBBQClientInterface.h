#pragma once
#ifdef BBQCLIENT_EXPORTS
#define SF_DLL_API __declspec(dllexport)
#  else
#define SF_DLL_API __declspec(dllimport)
#  endif

#include "sf.h"
 extern "C"{
SF_DLL_API void SFinit();
SF_DLL_API void SFUninit();
SF_DLL_API SF_HANDLE mm_create();
SF_DLL_API void mm_destroy(const SF_HANDLE handle);
//bool mm_login(const SF_HANDLE handle,const SIMD_CS_LOGIN_EX * in, SIMD_SC_LOGIN_EX * out  );
  SF_DLL_API int mm_getlasterror(const SF_HANDLE handle);
 //audio core
SF_DLL_API int YMM_Init() ;
SF_DLL_API int YMM_SetSd(unsigned int WaveInDevice, unsigned int WaveOutDevice);
SF_DLL_API int YMM_SetCB (OnCB fun) ;
SF_DLL_API int YMM_SetVADStyle(int mode) ;
SF_DLL_API int YMM_SetECStyle(int mode) ;
SF_DLL_API int YMM_SetECType(int type) ;
SF_DLL_API int YMM_SetAGCStyle(int mode) ;
SF_DLL_API int YMM_SetNRStyle(int mode) ;
SF_DLL_API int YMM_CreateStyle() ;
SF_DLL_API int YMM_SetSCodec(int sock, int codec_inst) ;
SF_DLL_API int YMM_SetSTransport(int sock, OnSendPacket fun) ;
SF_DLL_API int YMM_DeleteStyle(int sock) ;
SF_DLL_API int YMM_RPacket(int sock, const void *data, int len) ;
SF_DLL_API int YMM_StopSnd(int sock) ;
SF_DLL_API int YMM_Stopout(int sock) ;
SF_DLL_API int YMM_StartSnd(int sock) ;
SF_DLL_API int YMM_Startout(int sock) ;
SF_DLL_API int YMM_GetLastError() ;
SF_DLL_API int YMM_GetLastError() ;
SF_DLL_API int YMM_GetICL(int c) ;
SF_DLL_API int YMM_GetICL2(int c) ;
//SF_DLL_API int YMM_GetData(int type,int* data) ;
//SF_DLL_API int YMM_SetData(int type,int data) ;
};

 