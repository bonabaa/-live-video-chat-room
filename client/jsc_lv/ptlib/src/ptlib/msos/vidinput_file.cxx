/* vidinput_app.cxx
 *
 *
 * CCFile Implementation for the PTLib Project.
 *
 * Copyright (c) 2007 ISVO (Asia) Pte Ltd. All Rights Reserved.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 *
 *
 * Contributor(s): Craig Southeren, Post Increment (C) 2008
 *
 * $Revision: 20385 $
 * $Author: rjongbloed $
 * $Date: 2008-06-04 10:40:38 +0000 (Wed, 04 Jun 2008) $
 */


#include <ptlib.h>

#if 1
#include <ptlib/videoio.h>
#include <ptlib/msos/ptlib/vidinput_file.h>
#include <ptlib/vconvert.h>

 
 
//////////////////////////////////////////////////////////////////////////////////////
#if 0
#else
//PCREATE_VIDINPUT_PLUGIN(CCFile, PVideoInputDevice, &PVideoInputDevice_CCFile_descriptor);
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// Input device

PVideoInputDevice_CCFile::PVideoInputDevice_CCFile()
{
  SetColourFormat("BGR24"/*32*/);
  SetFrameRate(10);
	//m_nFullWidth=GetSystemMetrics(SM_CXSCREEN); m_nFullHeight=GetSystemMetrics(SM_CYSCREEN); 

}

PVideoInputDevice_CCFile::~PVideoInputDevice_CCFile()
{
  Close();
}

PStringArray PVideoInputDevice_CCFile::GetDeviceNames() const
{ 
  return GetInputDeviceNames(); 
}
 

PStringArray PVideoInputDevice_CCFile::GetInputDeviceNames()
{
  PStringArray names;

  names += "CCFile";

 
  return names;
}

PBoolean PVideoInputDevice_CCFile::GetDeviceCapabilities(const PString & /*deviceName*/, Capabilities * /*caps*/)  
{ 
  return false; 
}

PBoolean PVideoInputDevice_CCFile::Open(const PString & deviceName, PBoolean /*startImmediate*/)
{
  Close();
  PString strFileName;
  //int w,h;
  if (deviceName.Find(":")!=P_MAX_INDEX){
    strFileName= deviceName.Mid(deviceName.Find(":"));
  }else{
     
    strFileName="cc.dat";
  }
  //read header
  if (m_file.IsOpen() ==false ){
   if( m_file.Open(strFileName, PFile::ReadOnly))
   {
     m_file.ReadBlock(&m_header, sizeof(m_header));
   }
  }
  //SetFrameSize(rect.right-rect.left, rect.bottom-rect.top);
  SetFrameSize(m_header.w, m_header.h);
  

  return true;
}

PBoolean PVideoInputDevice_CCFile::IsOpen()
{
  return m_file.IsOpen();
}

PBoolean PVideoInputDevice_CCFile::Close()
{
  if (!IsOpen())
    return false;

  m_file.Close(); 
  return true;
}

PBoolean PVideoInputDevice_CCFile::Start()
{
  return true;
}

PBoolean PVideoInputDevice_CCFile::Stop()
{
  return true;
}

PBoolean PVideoInputDevice_CCFile::IsCapturing()
{
  return IsOpen();
}
 
PBoolean PVideoInputDevice_CCFile::SetVideoFormat(VideoFormat newFormat)
{
  return PVideoDevice::SetVideoFormat(newFormat);
}

PBoolean PVideoInputDevice_CCFile::SetColourFormat(const PString & colourFormat)
{
  if ((colourFormat *= "YUV420P")    )
    return PVideoDevice::SetColourFormat(colourFormat);

  return PFalse;
}

PBoolean PVideoInputDevice_CCFile::SetFrameRate(unsigned Rate)
{
  return PVideoDevice::SetFrameRate(Rate);
}

PBoolean PVideoInputDevice_CCFile::SetFrameSize(unsigned width, unsigned height)
{
  
  return PVideoDevice::SetFrameSize(width, height);
}

PINDEX PVideoInputDevice_CCFile::GetMaxFrameBytes()
{
  return GetMaxFrameBytesConverted(CalculateFrameBytes(frameWidth, frameHeight, colourFormat));
}

PBoolean PVideoInputDevice_CCFile::GetFrameData(
      BYTE * buffer,                 
      PINDEX * bytesReturned   
    )
{
    grabDelay.Delay(1000/GetFrameRate());
    return GetFrameDataNoDelay(buffer,bytesReturned);
}

 
 PBoolean PVideoInputDevice_CCFile::GetFrameDataNoDelay(BYTE * buffer, PINDEX * bytesReturned)
{
  //PWaitAndSignal m(lastFrameMutex);
  bool retVal=false ;

  if (m_file.IsOpen() ){
    if (m_file.IsEndOfFile())
      m_file.SetPosition(sizeof( st_CCFileHeader));


    retVal = m_file.ReadBlock(buffer, m_header.nDatabock);
    *bytesReturned =  m_header.nDatabock;
  }

  ////////////////////////////////////////////////////

  return retVal;
}


PBoolean PVideoInputDevice_CCFile::TestAllFormats()
{
    return true;
}

PBoolean PVideoInputDevice_CCFile::SetChannel(int /*newChannel*/)
{
  return true;
}

 
PBoolean PVideoInputDevice_CCFile::SetFrameSizeConverter(unsigned width, unsigned height, ResizeMode resizeMode)
{
  
   

  //PTRACE(3,"PVidDev\tColour converter used from " << converter->GetSrcFrameWidth() << 'x' << converter->GetSrcFrameHeight() << " [" << converter->GetSrcColourFormat() << "]" << " to " << converter->GetDstFrameWidth() << 'x' << converter->GetDstFrameHeight() << " [" << converter->GetDstColourFormat() << "]");

  return PTrue;
}
PFile* PVideoInputDevice_CCFile::g_recordfile=0;
bool PVideoInputDevice_CCFile::CCRecoreFile_Open(int h, int w, int nfps, int nDataBlock)
{
  if (g_recordfile==NULL){
    g_recordfile = new PFile();
  }
  else
    return true;

  g_recordfile->Open("d:\\cc.dat", PFile::ReadWrite, PFile::Create);

  st_CCFileHeader header;
  header.h =h;
  header.w = w;
  header.nfps = nfps;
  header.nDatabock = nDataBlock;
  g_recordfile->Write(&header, sizeof(header));
  return true;
}
bool PVideoInputDevice_CCFile::CCRecoreFile_Write(void * data, int datalen)
{

  g_recordfile->Write(data, datalen);
 
  return true;
}
bool PVideoInputDevice_CCFile::CCRecoreFile_Close()
{
  if (g_recordfile)
  {
    g_recordfile->Close();
    delete g_recordfile;g_recordfile=0;
  }
  return true;
}


#endif  // P_APPSHARE
