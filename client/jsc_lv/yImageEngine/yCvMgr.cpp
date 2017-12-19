#include "StdAfx.h"
#include "yCvMgr.h"

 #include "cv.h" //  OpenCV 的基本函数头文件  
 #include"highgui.h" //OpenCV 的图像显示函数头文件  
#include "CvxText.h"

yCvMgr::yCvMgr(void)
{
  m_pImgAudio=0;
}


yCvMgr::~yCvMgr(void)
{
 IplImage *img= ((IplImage *)m_pImgAudio);
   cvReleaseImage(&img );
}
void yCvMgr::SetAudioStatus( BYTE* buffer, int bufLen, int w, int h, int x)
 {
   IplImage * img= (IplImage *)m_pImgAudio;
   if (img==NULL || img->width!= w|| img->height!= h ){
     if (m_pImgAudio!=NULL)
       cvReleaseImage(&img);
     // 
     m_pImgAudio = img=cvCreateImage(cvSize(w, h), 8,3);
     
   }

   ///draw it 
   {
      //cvLine(m_pImgAudio,cvPoint(20,20),cvPoint(100,20),cvScalar(255,255,255),4);
     memcpy(img->imageData, buffer, bufLen);
#define D_W_LINE  3
     int x_w= x>0&&x<D_W_LINE?3:x ;
     int y1=20, y2=20-D_W_LINE;
     for(int i=0;i< x_w/D_W_LINE;i++){
      
      cvLine(img,cvPoint(290,y1-1),cvPoint( 290 ,y2-1 ),cvScalar(0,255,0),3, CV_AA  );
      cvLine(img,cvPoint(298,y1 ),cvPoint( 298 ,y2 ),cvScalar(0,255,0),3, CV_AA  );
      cvLine(img,cvPoint(306,y1-1),cvPoint( 306 ,y2 -1),cvScalar(0,255,0),3, CV_AA  );
      y1 =y2-3;
      y2  =  y1-D_W_LINE;
     }
     memcpy(buffer,img->imageData,  bufLen);
     // cvShowImage( "window", g_image);
   }
 }