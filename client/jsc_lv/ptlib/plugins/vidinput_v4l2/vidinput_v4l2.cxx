/*
 * vidinput_v4l2.cxx
 *
 * Classes to support streaming video input (grabbing) and output.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1998-2000 Equivalence Pty. Ltd.
 * Copyright (c) 2003 March Networks
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * First V4L2 capture code written by March Networks 
 * (http://www.marchnetworks.com) 
 *
 * This code is based on the Video4Linux 1 code.
 *
 * Contributor(s): Guilhem Tardy (gtardy@salyens.com)
 *  Nicola Orru' <nigu@itadinanta.it>
 *
 * $Revision: 22071 $
 * $Author: rjongbloed $
 * $Date: 2009-02-16 02:56:21 +0000 (Mon, 16 Feb 2009) $
 */

#pragma implementation "vidinput_v4l2.h"

#include "vidinput_v4l2.h"
#include <sys/utsname.h>

PCREATE_VIDINPUT_PLUGIN(V4L2);

#include "vidinput_names.h" 

#ifdef HAS_LIBV4L
#include <libv4l2.h>
#else
#define v4l2_fd_open(fd, flags) (fd)
#define v4l2_close close
#define v4l2_ioctl ioctl
#define v4l2_read read
#define v4l2_mmap mmap
#define v4l2_munmap munmap
#endif  

class V4L2Names : public V4LXNames
{

  PCLASSINFO(V4L2Names, V4LXNames);

public:
  
  V4L2Names() { kernelVersion=KUNKNOWN; };

  virtual void Update ();
  
protected:
  
  virtual PString BuildUserFriendly(PString devname);

  enum KernelVersionEnum {
    K2_4, 
    K2_6,
    KUNKNOWN,
  } kernelVersion;

};


PMutex creationMutex;
static 
V4L2Names & GetNames()
{
  PWaitAndSignal m(creationMutex);
  static V4L2Names names;
  names.Update();
  return names;
}

///////////////////////////////////////////////////////////////////////////////
// PVideoInputDevice_V4L2

PVideoInputDevice_V4L2::PVideoInputDevice_V4L2()
{
  videoFd = -1;
  canRead = PFalse;
  canStream = PFalse;
  canSelect = PFalse;
  canSetFrameRate = PFalse;
  isMapped = PFalse;
  started = PFalse;
}

PVideoInputDevice_V4L2::~PVideoInputDevice_V4L2()
{
  Close();
}


#ifndef V4L2_PIX_FMT_H263
#define V4L2_PIX_FMT_H263       v4l2_fourcc('H','2','6','3')
#endif


static struct {
  const char * colourFormat;
#ifdef SOLARIS
  uint32_t code;
#else
  __u32 code;
#endif
} colourFormatTab[] = {
    { "Grey", V4L2_PIX_FMT_GREY },   //Entries in this table correspond
    { "RGB32", V4L2_PIX_FMT_RGB32 }, //(line by line) to those in the 
    { "BGR32", V4L2_PIX_FMT_BGR32 }, //PVideoDevice ColourFormat table.
    { "RGB24", V4L2_PIX_FMT_RGB24 }, 
    { "BGR24", V4L2_PIX_FMT_BGR24 },
    { "RGB565", V4L2_PIX_FMT_RGB565 },
    { "RGB555", V4L2_PIX_FMT_RGB555 },
    { "YUV411", V4L2_PIX_FMT_Y41P },
    { "YUV411P", V4L2_PIX_FMT_YUV411P },
    { "YUV420", V4L2_PIX_FMT_NV21 },
    { "YUV420P", V4L2_PIX_FMT_YUV420 },
    { "YUV422", V4L2_PIX_FMT_YUYV },   /* Note: YUV422 is for compatibility */
    { "YUV422P", V4L2_PIX_FMT_YUV422P },
    { "YUY2", V4L2_PIX_FMT_YUYV },
    { "JPEG", V4L2_PIX_FMT_JPEG },
    { "H263", V4L2_PIX_FMT_H263 },
    { "SBGGR8", V4L2_PIX_FMT_SBGGR8 },
    { "MJPEG", V4L2_PIX_FMT_MJPEG},
    { "UYVY422", V4L2_PIX_FMT_UYVY}
};


PBoolean PVideoInputDevice_V4L2::Open(const PString & devName, PBoolean startImmediate)
{
  struct utsname buf;
  PString version;
  int libv4l2_fd;
  
  uname (&buf);

  if (buf.release)
    version = PString (buf.release);

  PTRACE(1,"PVidInDev\tOpen()\tvideoFd:" << videoFd);
  Close();

  PString name = GetNames().GetDeviceName(devName);
  PTRACE(1,"PVidInDev\tOpen()\tdevName:" << name << "  videoFd:" << videoFd);
  
  videoFd = ::open((const char *)name, O_RDWR);
  if (videoFd < 0) {
    PTRACE(1,"PVidInDev\topen failed : " << ::strerror(errno));
    return PFalse;
  }
  
  PTRACE(6,"PVidInDev\topen, fd=" << videoFd);
  deviceName=name;

  // Don't share the camera device with subprocesses - they could cause
  // EBUSY errors on VIDIOC_STREAMON if the parent tries to close and reopen
  // the camera while the child is still running.
  ::fcntl(videoFd, F_SETFD, FD_CLOEXEC);

  /* Note the v4l2_xxx functions are designed so that if they get passed an
     unknown fd, the will behave exactly as their regular xxx counterparts, so
     if v4l2_fd_open fails, we continue as normal (missing the libv4l2 custom
     cam format to normal formats conversion). Chances are big we will still
     fail then though, as normally v4l2_fd_open only fails if the device is not
     a v4l2 device. */
  libv4l2_fd = v4l2_fd_open(videoFd, 0);
  if (libv4l2_fd != -1)
    videoFd = libv4l2_fd;

  // get the device capabilities
  if (v4l2_ioctl(videoFd, VIDIOC_QUERYCAP, &videoCapability) < 0) {
    PTRACE(1,"PVidInDev\tQUERYCAP failed : " << ::strerror(errno));
    v4l2_close (videoFd);
    videoFd = -1;
    return PFalse;
  }
    
  canRead = videoCapability.capabilities & V4L2_CAP_READWRITE;
  canStream = videoCapability.capabilities & V4L2_CAP_STREAMING;
  canSelect = videoCapability.capabilities & V4L2_CAP_ASYNCIO;

  // set height and width
  frameHeight = QCIFHeight;
  frameWidth  = QCIFWidth;


  // get the capture parameters
  videoStreamParm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (v4l2_ioctl(videoFd, VIDIOC_G_PARM, &videoStreamParm) < 0)  {

    PTRACE(1,"PVidInDev\tG_PARM failed : " << ::strerror(errno));
    canSetFrameRate = PFalse;

  } else {

    canSetFrameRate = videoStreamParm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME;
    if (canSetFrameRate)
      PVideoDevice::SetFrameRate (videoStreamParm.parm.capture.timeperframe.denominator / videoStreamParm.parm.capture.timeperframe.numerator);
  }
  
  return PTrue;
}


PBoolean PVideoInputDevice_V4L2::IsOpen() 
{
  return videoFd >= 0;
}


PBoolean PVideoInputDevice_V4L2::Close()
{
  PTRACE(1,"PVidInDev\tClose()\tvideoFd:" << videoFd << "  started:" << started);
  if (!IsOpen())
    return PFalse;

  Stop();
  ClearMapping();
  v4l2_close(videoFd);

  PTRACE(6,"PVidInDev\tclose, fd=" << videoFd);

  videoFd = -1;
  canRead = PFalse;
  canStream = PFalse;
  canSelect = PFalse;
  canSetFrameRate = PFalse;
  isMapped = PFalse;

  PTRACE(1,"PVidInDev\tClose()\tvideoFd:" << videoFd << "  started:" << started);
  return PTrue;
}


PBoolean PVideoInputDevice_V4L2::Start()
{
  // automatically set mapping
  if (!isMapped && !SetMapping()) {
    ClearMapping();
    canStream = PFalse; // don't try again
    return PFalse;
  }

  if (!started) {
    PTRACE(6,"PVidInDev\tstart queuing all buffers, fd=" << videoFd);

    /* Queue all buffers */
    currentvideoBuffer = 0;

    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    for (unsigned int i=0; i<videoBufferCount; i++) {

       buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
       buf.memory = V4L2_MEMORY_MMAP;
       buf.index = i;

       if (v4l2_ioctl(videoFd, VIDIOC_QBUF, &buf) < 0) {
	  PTRACE(3,"PVidInDev\tVIDIOC_QBUF failed for buffer " << i << ": " << ::strerror(errno));
	  return PFalse;
       }
    }

    /* Start streaming */
    PTRACE(6,"PVidInDev\tstart streaming, fd=" << videoFd);
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (v4l2_ioctl(videoFd, VIDIOC_STREAMON, &type) < 0) {
       PTRACE(3,"PVidInDev\tSTREAMON failed : " << ::strerror(errno));
       return PFalse;
    }

    started = PTrue;

  }
  return PTrue;
}


PBoolean PVideoInputDevice_V4L2::Stop()
{
  if (started) {
    PTRACE(6,"PVidInDev\tstop streaming, fd=" << videoFd);

    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    started = PFalse;

    if (v4l2_ioctl(videoFd, VIDIOC_STREAMOFF, &type) < 0) {
      PTRACE(3,"PVidInDev\tSTREAMOFF failed : " << ::strerror(errno));
      return PFalse;
    }

    // no need to dequeue filled buffers, as this is handled by V4L2 at the next VIDIOC_STREAMON
  }

  return PTrue;
}


PBoolean PVideoInputDevice_V4L2::IsCapturing()
{
  return started;
}

PStringList PVideoInputDevice_V4L2::GetInputDeviceNames()
{
  return GetNames().GetInputDeviceNames();  
}


PBoolean PVideoInputDevice_V4L2::SetVideoFormat(VideoFormat newFormat)
{
  if (newFormat == Auto) {
    if (SetVideoFormat(PAL) ||
      SetVideoFormat(NTSC) ||
      SetVideoFormat(SECAM))
      return PTrue;
    else
      return PFalse;
  }

  if (!PVideoDevice::SetVideoFormat(newFormat)) {
    PTRACE(1,"PVideoDevice::SetVideoFormat failed for format " << newFormat);
    return PFalse;
  }

  struct {
#ifdef SOLARIS
    uint32_t code;
#else
    __u32 code;
#endif
    const char * name;
  } static const fmt[3] = { {V4L2_STD_PAL, "PAL"},
      {V4L2_STD_NTSC, "NTSC"},
      {V4L2_STD_SECAM, "SECAM"} };

  // set the video standard
  if (v4l2_ioctl(videoFd, VIDIOC_S_STD, &fmt[newFormat].code) < 0) {
    PTRACE(1,"VideoInputDevice\tS_STD failed : " << ::strerror(errno));
  }

  PTRACE(6,"PVidInDev\tset video format \"" << fmt[newFormat].name << "\", fd=" << videoFd);

  return PTrue;
}


int PVideoInputDevice_V4L2::GetNumChannels() 
{
  // if opened, return the capability value, else 1 as in videoio.cxx
  if (IsOpen ()) {

    struct v4l2_input videoEnumInput;
    videoEnumInput.index = 0;
    while (1) {
      if (v4l2_ioctl(videoFd, VIDIOC_ENUMINPUT, &videoEnumInput) < 0) {
        break;
      }
      else
        videoEnumInput.index++;
    }

    return videoEnumInput.index;
  }
  else
    return 1;
}


PBoolean PVideoInputDevice_V4L2::SetChannel(int newChannel)
{
  if (!PVideoDevice::SetChannel(newChannel)) {
    PTRACE(1,"PVideoDevice::SetChannel failed for channel " << newChannel);
    return PFalse;
  }

  // set the channel
  if (v4l2_ioctl(videoFd, VIDIOC_S_INPUT, &channelNumber) < 0) {
    PTRACE(1,"VideoInputDevice\tS_INPUT failed : " << ::strerror(errno));    
    return PFalse;
  }

  PTRACE(6,"PVidInDev\tset channel " << newChannel << ", fd=" << videoFd);

  return PTrue;
}


PBoolean PVideoInputDevice_V4L2::SetVideoChannelFormat (int newChannel, VideoFormat videoFormat) 
{
  if (!SetChannel(newChannel) ||
      !SetVideoFormat(videoFormat))
    return PFalse;

  return PTrue;
}


PBoolean PVideoInputDevice_V4L2::SetColourFormat(const PString & newFormat)
{
  PINDEX colourFormatIndex = 0;
  while (newFormat != colourFormatTab[colourFormatIndex].colourFormat) {
    colourFormatIndex++;
    if (colourFormatIndex >= PARRAYSIZE(colourFormatTab))
      return PFalse;
  }

  if (!PVideoDevice::SetColourFormat(newFormat)) {
    PTRACE(3,"PVidInDev\tSetColourFormat failed for colour format " << newFormat);
    return PFalse;
  }

  PBoolean resume = started;
  Stop();
  ClearMapping();

  struct v4l2_format videoFormat;
  memset(&videoFormat, 0, sizeof(struct v4l2_format));
  videoFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  // get the frame rate so we can preserve it throughout the S_FMT call
  struct v4l2_streamparm streamParm;
  unsigned int fi_n = 0, fi_d = 0;
  streamParm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (v4l2_ioctl(videoFd, VIDIOC_G_PARM, &streamParm) == 0 &&
        streamParm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME) {
    fi_n = streamParm.parm.capture.timeperframe.numerator;
    fi_d = streamParm.parm.capture.timeperframe.denominator;
  } else {
    PTRACE(1,"PVidInDev\tG_PARM failed (preserving frame rate may not work) : " << ::strerror(errno));
  }

  // get the colour format
  if (v4l2_ioctl(videoFd, VIDIOC_G_FMT, &videoFormat) < 0) {
    PTRACE(1,"PVidInDev\tG_FMT failed : " << ::strerror(errno));
    return PFalse;
  }

  videoFormat.fmt.pix.pixelformat = colourFormatTab[colourFormatIndex].code;

  // set the colour format
  if (v4l2_ioctl(videoFd, VIDIOC_S_FMT, &videoFormat) < 0) {
    PTRACE(1,"PVidInDev\tS_FMT failed : " << ::strerror(errno));
    PTRACE(1,"\tused code of " << videoFormat.fmt.pix.pixelformat << " for palette: " << colourFormatTab[colourFormatIndex].colourFormat);
    return PFalse;
  }

  // get the colour format again to be careful about broken drivers
  if (v4l2_ioctl(videoFd, VIDIOC_G_FMT, &videoFormat) < 0) {
    PTRACE(1,"PVidInDev\tG_FMT failed : " << ::strerror(errno));
    return PFalse;
  }

  if (videoFormat.fmt.pix.pixelformat != colourFormatTab[colourFormatIndex].code) {
    PTRACE(3,"PVidInDev\tcolour format mismatch.");
    return PFalse;
  }

  // reset the frame rate because it may have been overridden by the call to S_FMT
  if (fi_n == 0 || fi_d == 0 || v4l2_ioctl(videoFd, VIDIOC_S_PARM, &streamParm) < 0) {
    PTRACE(3,"PVidInDev\tunable to reset frame rate.");
  } else if (streamParm.parm.capture.timeperframe.numerator != fi_n ||
             streamParm.parm.capture.timeperframe.denominator  != fi_d) {
    PTRACE(3, "PVidInDev\tnew frame interval (" << streamParm.parm.capture.timeperframe.numerator
              << "/" << streamParm.parm.capture.timeperframe.denominator
              << ") differs from what was requested (" << fi_n << "/" << fi_d << ").");
  }

  frameBytes = videoFormat.fmt.pix.sizeimage;

  PTRACE(6,"PVidInDev\tset colour format \"" << newFormat << "\", fd=" << videoFd);

  if (resume)
    return Start();

  return PTrue;
}


PBoolean PVideoInputDevice_V4L2::SetFrameRate(unsigned rate)
{
  if (!PVideoDevice::SetFrameRate(rate)) {
    PTRACE(3,"PVidInDev\tSetFrameRate failed for rate " << rate);
    return PTrue; // Ignore
  }

  if (canSetFrameRate) {
    videoStreamParm.parm.capture.timeperframe.numerator = 1;
    videoStreamParm.parm.capture.timeperframe.denominator = (rate ? rate : 1);

    // set the stream parameters
    if (v4l2_ioctl(videoFd, VIDIOC_S_PARM, &videoStreamParm) < 0)  {
      PTRACE(1,"PVidInDev\tS_PARM failed : "<< ::strerror(errno));
      return PTrue;
    }

    PTRACE(6,"PVidInDev\tset frame rate " << rate << "fps, fd=" << videoFd);
  }

  return PTrue;
}


PBoolean PVideoInputDevice_V4L2::GetFrameSizeLimits(unsigned & minWidth,
                                                unsigned & minHeight,
                                                unsigned & maxWidth,
                                                unsigned & maxHeight) 
{
  minWidth=0;
  maxWidth=65535;
  minHeight=0;
  maxHeight=65535;

  // Before 2.6.19 there is no official way to enumerate frame sizes
  // in V4L2, but we can use VIDIOC_TRY_FMT to find the largest supported
  // size. This is roughly what the kernel V4L1 compatibility layer does.

  struct v4l2_format fmt;
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (v4l2_ioctl(videoFd, VIDIOC_G_FMT, &fmt) < 0) {
    return PFalse;
  }

  fmt.fmt.pix.width = fmt.fmt.pix.height = 10000;
  if (v4l2_ioctl(videoFd, VIDIOC_TRY_FMT, &fmt) < 0) {
    return PFalse;
  }
  maxWidth = fmt.fmt.pix.width;
  maxHeight = fmt.fmt.pix.height;
  return PTrue;
}


PBoolean PVideoInputDevice_V4L2::SetFrameSize(unsigned width, unsigned height)
{
  if (!PVideoDevice::SetFrameSize(width, height)) {
    PTRACE(3,"PVidInDev\tSetFrameSize failed for size " << width << "x" << height);
    return PFalse;
  }

  PBoolean resume = started;
  Stop();
  ClearMapping();

  if (!VerifyHardwareFrameSize(width, height)) {
    PTRACE(3,"PVidInDev\tVerifyHardwareFrameSize failed for size " << width << "x" << height);
    return PFalse;
  }

  PTRACE(6,"PVidInDev\tset frame size " << width << "x" << height << ", fd=" << videoFd);

  if (resume)
    return Start();

  return PTrue;
}


PINDEX PVideoInputDevice_V4L2::GetMaxFrameBytes()
{
  return GetMaxFrameBytesConverted(frameBytes);
}


PBoolean PVideoInputDevice_V4L2::SetMapping()
{
  if (!canStream)
    return PFalse;

  struct v4l2_requestbuffers reqbuf;
  reqbuf.count = NUM_VIDBUF;
  reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  reqbuf.memory = V4L2_MEMORY_MMAP;

  if (v4l2_ioctl(videoFd, VIDIOC_REQBUFS, &reqbuf) < 0) {
    PTRACE(3,"PVidInDev\tREQBUFS failed : " << ::strerror(errno));
    return PFalse;
  }
  if (reqbuf.count < 1) {
    PTRACE(3,"PVidInDev\tNot enough video buffer available. (got " << reqbuf.count << ")");
    return PFalse;
  }
  if (reqbuf.count > NUM_VIDBUF) {
    PTRACE(3,"PVidInDev\tToo much video buffer allocated. (got " << reqbuf.count << ")");
    return PFalse;
  }

  struct v4l2_buffer buf;
  memset(&buf, 0, sizeof(buf));
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;
  
  videoBufferCount = reqbuf.count;
  for (buf.index = 0; buf.index < videoBufferCount; buf.index++) {
    if (v4l2_ioctl(videoFd, VIDIOC_QUERYBUF, &buf) < 0) {
      PTRACE(3,"PVidInDev\tQUERYBUF failed : " << ::strerror(errno));
      return PFalse;
    }

    if ((videoBuffer[buf.index] = (BYTE *)v4l2_mmap(0, buf.length, PROT_READ|PROT_WRITE, MAP_SHARED, videoFd, buf.m.offset)) == MAP_FAILED) {
      PTRACE(3,"PVidInDev\tmmap failed : " << ::strerror(errno));
      return PFalse;
    }
  }

  isMapped = PTrue;

  PTRACE(7,"PVidInDev\tset mapping for " << videoBufferCount << " buffers, fd=" << videoFd);


  return PTrue;
}


void PVideoInputDevice_V4L2::ClearMapping()
{
  if (!canStream) // 'isMapped' wouldn't handle partial mappings
    return;

  struct v4l2_buffer buf;
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;

  for (buf.index = 0; ; buf.index++) {
    if (v4l2_ioctl(videoFd, VIDIOC_QUERYBUF, &buf) < 0)
      break;

#ifdef SOLARIS
    ::v4l2_munmap((char*)videoBuffer[buf.index], buf.length);
#else
    ::v4l2_munmap(videoBuffer[buf.index], buf.length);
#endif
  }

  isMapped = PFalse;

  PTRACE(7,"PVidInDev\tclear mapping, fd=" << videoFd);
}


PBoolean PVideoInputDevice_V4L2::GetFrameData(BYTE * buffer, PINDEX * bytesReturned)
{
  PTRACE(8,"PVidInDev\tGetFrameData()");

  m_pacing.Delay(1000/GetFrameRate());
  return GetFrameDataNoDelay(buffer, bytesReturned);
}


PBoolean PVideoInputDevice_V4L2::GetFrameDataNoDelay(BYTE * buffer, PINDEX * bytesReturned)
{
  PTRACE(8,"PVidInDev\tGetFrameDataNoDelay()\tstarted:" << started << "  canSelect:" << canSelect);

  if (!started)
    return NormalReadProcess(buffer, bytesReturned);

  struct v4l2_buffer buf;
  memset(&buf, 0, sizeof(struct v4l2_buffer));
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;
  buf.index = currentvideoBuffer;

  if (v4l2_ioctl(videoFd, VIDIOC_DQBUF, &buf) < 0) {
    // strace resistance
    if (errno == EINTR) {
        if (v4l2_ioctl(videoFd, VIDIOC_DQBUF, &buf) < 0) {
          PTRACE(1,"PVidInDev\tDQBUF failed : " << ::strerror(errno));
          return PFalse;
        }
    }
  }

  currentvideoBuffer = (currentvideoBuffer+1) % NUM_VIDBUF;

  // If converting on the fly do it from frame store to output buffer,
  // otherwise do straight copy.
  if (converter != NULL)
    converter->Convert(videoBuffer[buf.index], buffer, buf.bytesused, bytesReturned);
  else {
    memcpy(buffer, videoBuffer[buf.index], buf.bytesused);
    if (bytesReturned != NULL)
      *bytesReturned = buf.bytesused;
  }

  PTRACE(8,"PVidInDev\tget frame data of " << buf.bytesused << "bytes, fd=" << videoFd);

  // requeue the buffer
  if (v4l2_ioctl(videoFd, VIDIOC_QBUF, &buf) < 0) {
    PTRACE(1,"PVidInDev\tQBUF failed : " << ::strerror(errno));
  }

  return PTrue;
}


// This video device does not support memory mapping - so use
// normal read process to extract a frame of video data.
PBoolean PVideoInputDevice_V4L2::NormalReadProcess(BYTE * buffer, PINDEX * bytesReturned)
{ 
  if (!canRead)
    return PFalse;

  ssize_t bytesRead;

  do
    bytesRead = v4l2_read(videoFd, buffer, frameBytes);
  while (bytesRead < 0 && errno == EINTR && IsOpen());

  if (bytesRead < 0) {
    
    PTRACE(1,"PVidInDev\tread failed (read = "<<bytesRead<< " expected " << frameBytes <<")");
    bytesRead = frameBytes;
  }

  if ((PINDEX)bytesRead != frameBytes) {
    PTRACE(1,"PVidInDev\tread returned fewer bytes than expected");
    // May result from a compressed format, otherwise indicates an error.
  }

  if (converter != NULL)
    return converter->ConvertInPlace(buffer, bytesReturned);

  if (bytesReturned != NULL)
    *bytesReturned = (PINDEX)bytesRead;

  return PTrue;
}

PBoolean PVideoInputDevice_V4L2::VerifyHardwareFrameSize(unsigned width, unsigned height)
{
  struct v4l2_format videoFormat;
  videoFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  struct v4l2_streamparm streamParm;
  unsigned int fi_n = 0, fi_d = 0;
  streamParm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  // get the frame size
  if (v4l2_ioctl(videoFd, VIDIOC_G_FMT, &videoFormat) < 0) {
    PTRACE(1,"PVidInDev\tG_FMT failed : " << ::strerror(errno));
    return PFalse;
  }

  // get the frame rate so we can preserve it throughout the S_FMT call
  // Sidenote: V4L2 gives us the frame interval, i.e. 1/fps.
  if (v4l2_ioctl(videoFd, VIDIOC_G_PARM, &streamParm) == 0 &&
        streamParm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME) {
    fi_n = streamParm.parm.capture.timeperframe.numerator;
    fi_d = streamParm.parm.capture.timeperframe.denominator;
  } else {
    PTRACE(1,"PVidInDev\tG_PARM failed (preserving frame rate may not work) : " << ::strerror(errno));
  }

  videoFormat.fmt.pix.width = width;
  videoFormat.fmt.pix.height = height;

  // set the frame size
  if (v4l2_ioctl(videoFd, VIDIOC_S_FMT, &videoFormat) < 0) {
    PTRACE(1,"PVidInDev\tS_FMT failed : " << ::strerror(errno));
    PTRACE(1,"\tused frame size of " << videoFormat.fmt.pix.width << "x" << videoFormat.fmt.pix.height);
    return PFalse;
  }

  // get the frame size again to be careful about broken drivers
  if (v4l2_ioctl(videoFd, VIDIOC_G_FMT, &videoFormat) < 0) {
    PTRACE(1,"PVidInDev\tG_FMT failed : " << ::strerror(errno));
    return PFalse;
  }

  if ((videoFormat.fmt.pix.width != width) || (videoFormat.fmt.pix.height != height)) {
    PTRACE(3,"PVidInDev\tframe size mismatch.");
    // allow the device to return actual frame size
    PVideoDevice::SetFrameSize(videoFormat.fmt.pix.width, videoFormat.fmt.pix.height);
    return PFalse;
  }

  // reset the frame rate because it may have been overridden by the call to S_FMT
  if (fi_n == 0 || fi_d == 0 || v4l2_ioctl(videoFd, VIDIOC_S_PARM, &streamParm) < 0) {
    PTRACE(3,"PVidInDev\tunable to reset frame rate.");
  } else if (streamParm.parm.capture.timeperframe.numerator != fi_n ||
             streamParm.parm.capture.timeperframe.denominator  != fi_d) {
    PTRACE(3, "PVidInDev\tnew frame interval (" << streamParm.parm.capture.timeperframe.numerator
              << "/" << streamParm.parm.capture.timeperframe.denominator
              << ") differs from what was requested (" << fi_n << "/" << fi_d << ").");
  }

  frameBytes = videoFormat.fmt.pix.sizeimage;
  return PTrue;
}

/**
 * Query the current control setting
 * @param control is v4l2 control id (V4L2_CID_BRIGHTNESS, V4L2_CID_WHITENESS, ...)
 * @return  -1 control is unknown, or an error occured
 *         >=0 current value in a range [0-65535]
 */
int PVideoInputDevice_V4L2::GetControlCommon(unsigned int control, int *value) 
{
  if (!IsOpen())
    return -1;

  struct v4l2_queryctrl q;
  memset(&q, 0, sizeof(struct v4l2_queryctrl));
  q.id = control;
  if (v4l2_ioctl(videoFd, VIDIOC_QUERYCTRL, &q) < 0)
    return -1;

  struct v4l2_control c;
  memset(&c, 0, sizeof(struct v4l2_control));
  c.id = control;
  if (v4l2_ioctl(videoFd, VIDIOC_G_CTRL, &c) < 0)
    return -1;

  *value = (int)((float)(c.value - q.minimum) / (q.maximum-q.minimum) * (1<<16));

  return *value;
}

int PVideoInputDevice_V4L2::GetBrightness() 
{ 
  return GetControlCommon(V4L2_CID_BRIGHTNESS, &frameBrightness);
}

int PVideoInputDevice_V4L2::GetWhiteness() 
{
  return GetControlCommon(V4L2_CID_WHITENESS, &frameWhiteness);
}

int PVideoInputDevice_V4L2::GetColour() 
{ 
  return GetControlCommon(V4L2_CID_SATURATION, &frameColour);
}

int PVideoInputDevice_V4L2::GetContrast() 
{
  return GetControlCommon(V4L2_CID_CONTRAST, &frameContrast);
}

int PVideoInputDevice_V4L2::GetHue() 
{
  return GetControlCommon(V4L2_CID_HUE, &frameHue);
}

/**
 * Set a control to a new value
 *
 * @param control: V4L2_CID_BRIGHTNESS, V4L2_CID_WHITENESS, ...
 * @param newValue: 0-65535 Set this control to this range
 *                  -1 Set the default value
 * @return PFalse, if an error occur or the control is not supported
 */
PBoolean PVideoInputDevice_V4L2::SetControlCommon(unsigned int control, int newValue)
{
#ifdef __sun
 PTRACE(1,"PVidInDev\t" << __FILE__ << ":" << __LINE__  << "() videoFd=" << videoFd);
#else
  PTRACE(1,"PVidInDev\t" << __FUNCTION__  << "() videoFd=" << videoFd);
#endif
  if (!IsOpen())
    return PFalse;

  struct v4l2_queryctrl q;
  memset(&q, 0, sizeof(struct v4l2_queryctrl));
  q.id = control;
  if (v4l2_ioctl(videoFd, VIDIOC_QUERYCTRL, &q) < 0)
    return PFalse;

  struct v4l2_control c;
  memset(&c, 0, sizeof(struct v4l2_control));
  c.id = control;
  if (newValue < 0)
    c.value = q.default_value;
  else
    c.value = (float)(q.minimum + ((q.maximum-q.minimum) * ((float)newValue) / (1<<16)));

  if (v4l2_ioctl(videoFd, VIDIOC_S_CTRL, &c) < 0)
    return PFalse;

  return PTrue;
}

PBoolean PVideoInputDevice_V4L2::SetBrightness(unsigned newBrightness) 
{ 
  if (!SetControlCommon(V4L2_CID_BRIGHTNESS, newBrightness))
    return PFalse;
  frameBrightness = newBrightness;
  return PTrue;
}

PBoolean PVideoInputDevice_V4L2::SetWhiteness(unsigned newWhiteness) 
{ 
  if (!SetControlCommon(V4L2_CID_WHITENESS, newWhiteness))
    return PFalse;

  frameWhiteness = newWhiteness;
  return PTrue;
}

PBoolean PVideoInputDevice_V4L2::SetColour(unsigned newColour) 
{ 
  if (!SetControlCommon(V4L2_CID_SATURATION, newColour))
    return PFalse;
  frameColour = newColour;
  return PTrue;
}

PBoolean PVideoInputDevice_V4L2::SetContrast(unsigned newContrast) 
{ 
  if (!SetControlCommon(V4L2_CID_CONTRAST, newContrast))
    return PFalse;
  frameContrast = newContrast;
  return PTrue;
}

PBoolean PVideoInputDevice_V4L2::SetHue(unsigned newHue) 
{
  if (!SetControlCommon(V4L2_CID_HUE, newHue))
    return PFalse;
  frameHue=newHue;
  return PTrue;
}

PBoolean PVideoInputDevice_V4L2::GetParameters (int *whiteness, int *brightness, int *colour, int *contrast, int *hue)
{
  if (!IsOpen())
    return PFalse;

  frameWhiteness = -1;
  frameBrightness = -1;
  frameColour = -1;
  frameContrast = -1;
  frameHue = -1;
  GetWhiteness();
  GetBrightness();
  GetColour();
  GetContrast();
  GetHue();

  *whiteness  = frameWhiteness;
  *brightness = frameBrightness;
  *colour     = frameColour;
  *contrast   = frameContrast;
  *hue        = frameHue;

  return PTrue;
}

PBoolean PVideoInputDevice_V4L2::TestAllFormats()
{
  return PTrue;
}



// this is used to get more userfriendly names:

void
V4L2Names::Update()
{
  PTRACE(1,"Detecting V4L2 devices");
  PDirectory   procvideo2_4("/proc/video/dev");
  PDirectory   procvideo2_6("/sys/class/video4linux");
  PDirectory * procvideo;
  PString      entry;
  PStringList  devlist;
  PString      oldDevName;
  // Try and guess kernel version
  if (procvideo2_6.Exists()) {
    kernelVersion = K2_6;
    procvideo=&procvideo2_6;
  }
  else if (procvideo2_4.Exists()) {
    kernelVersion=K2_4;
    procvideo=&procvideo2_4;
  } 
  else {
    kernelVersion=KUNKNOWN;
    procvideo=0;
  }
  PWaitAndSignal m(mutex);
  inputDeviceNames.RemoveAll (); // flush the previous run
  if (procvideo) {
    PTRACE(2,"PV4L2Plugin\tdetected device metadata at "<<*procvideo);
    if (((kernelVersion==K2_6 && procvideo->Open(PFileInfo::SubDirectory)) || 
        (procvideo->Open(PFileInfo::RegularFile)))) {
      do {
        entry = procvideo->GetEntryName();
        if ((entry.Left(5) == "video")) {
          PString thisDevice = "/dev/" + entry;
          int videoFd=::open((const char *)thisDevice, O_RDONLY | O_NONBLOCK);
          if ((videoFd > 0) || (errno == EBUSY)) {
            PBoolean valid = PFalse;
            struct v4l2_capability videoCaps;
            memset(&videoCaps,0,sizeof(videoCaps));
            if ((errno == EBUSY) ||
                (v4l2_ioctl(videoFd, VIDIOC_QUERYCAP, &videoCaps) >= 0 &&
                (videoCaps.capabilities & V4L2_CAP_VIDEO_CAPTURE))) {
              PTRACE(1,"PV4L2Plugin\tdetected capture device " << videoCaps.card);
              valid = PTrue;
            }
            else {
              PTRACE(1,"PV4L2Plugin\t" << thisDevice << "is not deemed valid");
            }
            if (videoFd>0)
              ::close(videoFd);
            if(valid)
              inputDeviceNames += thisDevice;
          }
          else {
            PTRACE(1,"PV4L2Plugin\tcould not open " << thisDevice);
          }
        }
      } while (procvideo->Next());
    }
  }
  else {
    PTRACE(1,"Unable to detect v4l2 directory");
  }
  if (inputDeviceNames.GetSize() == 0) {
    POrdinalToString vid;
    ReadDeviceDirectory("/dev/", vid);

    for (PINDEX i = 0; i < vid.GetSize(); i++) {
      PINDEX cardnum = vid.GetKeyAt(i);
      int fd = ::open(vid[cardnum], O_RDONLY | O_NONBLOCK);
      if ((fd >= 0) || (errno == EBUSY)) {
        if (fd >= 0)
          ::close(fd);
        inputDeviceNames += vid[cardnum];
      }
    }
  }
  PopulateDictionary();
}

PString V4L2Names::BuildUserFriendly(PString devname)
{
  PString Result;

  int fd = ::open((const char *)devname, O_RDONLY);
  if(fd < 0) {
    return devname;
  }

  struct v4l2_capability videocap;
  memset(&videocap,0,sizeof(videocap));
  if (v4l2_ioctl(fd, VIDIOC_QUERYCAP, &videocap) < 0)  {
      ::close(fd);
      return devname;
    }
  
  ::close(fd);
  PString ufname((const char*)videocap.card);

  return ufname;
}

// End Of File ///////////////////////////////////////////////////////////////
