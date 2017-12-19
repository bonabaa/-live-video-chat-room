#pragma once
//class IplImage;

#ifdef YIMAGEENGINE_EXPORTS
#define LPN_DLL_API2 __declspec(dllexport)
#  else
#define LPN_DLL_API2 __declspec(dllimport)
#  endif

 class LPN_DLL_API2 yCvMgr
{
public:
  yCvMgr(void);
  ~yCvMgr(void);
  void SetAudioStatus( BYTE* buffer,int bufLen, int w, int h, int x);
private:
  int m_nCapW, m_nCapH;

  void* m_pImgAudio ;
};

