/*
 * pvidfile.cxx
 *
 * Video file implementation
 *
 * Portable Windows Library
 *
 * Copyright (C) 2004 Post Increment
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
 * The Initial Developer of the Original Code is
 * Craig Southeren <craigs@postincrement.com>
 *
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 22443 $
 * $Author: rjongbloed $
 * $Date: 2009-04-20 23:47:22 +0000 (Mon, 20 Apr 2009) $
 */

#ifdef __GNUC__
#pragma implementation "pvidfile.h"
#endif

#include <ptlib.h>

#if P_VIDEO
#if P_VIDFILE

#include <ptclib/pvidfile.h>
#include <ptlib/videoio.h>


///////////////////////////////////////////////////////////////////////////////

PVideoFile::PVideoFile()
  : unknownFrameSize(PTrue)
  , frameBytes(CalculateFrameBytes())
  , headerOffset(0)
{
}


PBoolean PVideoFile::SetFrameSize(unsigned width, unsigned height)   
{ 
  if (!PVideoFrameInfo::SetFrameSize(width, height))
    return PFalse;

  unknownFrameSize = PFalse;
  frameBytes = CalculateFrameBytes();
  return frameBytes > 0;
}


PBoolean PVideoFile::Open(const PFilePath & name, PFile::OpenMode mode, int opts)
{
  if (unknownFrameSize)
    ExtractHints(name, *this);
  return file.Open(name, mode, opts);
}


PBoolean PVideoFile::WriteFrame(const void * frame)
{
  return file.Write(frame, frameBytes);
}


PBoolean PVideoFile::ReadFrame(void * frame)
{
  if (file.Read(frame, frameBytes) && file.GetLastReadCount() == frameBytes)
    return PTrue;

  PTRACE(4, "YUVFILE\tError reading file " << file.GetErrorText(file.GetErrorCode(PFile::LastReadError)));
  return PFalse;
}


off_t PVideoFile::GetLength() const
{
  off_t len = file.GetLength();
  return len < headerOffset ? 0 : ((len - headerOffset)/frameBytes);
}


PBoolean PVideoFile::SetLength(off_t len)
{
  return file.SetLength(len*frameBytes + headerOffset);
}


off_t PVideoFile::GetPosition() const
{
  off_t pos = file.GetPosition();
  return pos < headerOffset ? 0 : ((pos - headerOffset)/frameBytes);
}


PBoolean PVideoFile::SetPosition(off_t pos, PFile::FilePositionOrigin origin)
{
  pos *= frameBytes;
  if (origin == PFile::Start)
    pos += headerOffset;

  return file.SetPosition(pos, origin);
}


PBoolean PVideoFile::ExtractHints(const PFilePath & fn, PVideoFrameInfo & info)
{
  static PRegularExpression  qcif  ("_qcif[^a-z0-9]",       PRegularExpression::Extended|PRegularExpression::IgnoreCase);
  static PRegularExpression   cif  ("_cif[^a-z0-9]",        PRegularExpression::Extended|PRegularExpression::IgnoreCase);
  static PRegularExpression sqcif  ("_sqcif[^a-z0-9]",      PRegularExpression::Extended|PRegularExpression::IgnoreCase);
  static PRegularExpression   cif4 ("_cif4[^a-z0-9]",       PRegularExpression::Extended|PRegularExpression::IgnoreCase);
  static PRegularExpression   cif16("_cif16[^a-z0-9]",      PRegularExpression::Extended|PRegularExpression::IgnoreCase);
  static PRegularExpression   XbyY ("_[0-9]+x[0-9]+[^a-z]", PRegularExpression::Extended|PRegularExpression::IgnoreCase);
  static PRegularExpression fps    ("_[0-9]+fps[^a-z]",     PRegularExpression::Extended|PRegularExpression::IgnoreCase);

  PCaselessString str = fn;
  PBoolean foundHint = PFalse;
  PINDEX pos;

  if (str.FindRegEx(qcif) != P_MAX_INDEX)
    foundHint = info.SetFrameSize(QCIFWidth, QCIFHeight);
  else if (str.FindRegEx(cif) != P_MAX_INDEX)
    foundHint = info.SetFrameSize(CIFWidth, CIFHeight);
  else if (str.FindRegEx(sqcif) != P_MAX_INDEX)
    foundHint = info.SetFrameSize(SQCIFWidth, SQCIFHeight);
  else if (str.FindRegEx(cif4) != P_MAX_INDEX)
    foundHint = info.SetFrameSize(CIF4Width, CIF4Height);
  else if (str.FindRegEx(cif16) != P_MAX_INDEX)
    foundHint = info.SetFrameSize(CIF16Width, CIF16Height);
  else if ((pos = str.FindRegEx(XbyY)) != P_MAX_INDEX) {
    unsigned width, height;
    if (sscanf(str.Mid(pos+1), "%ux%u", &width, &height) == 2)
      foundHint = info.SetFrameSize(width, height);
  }

  if ((pos = str.FindRegEx(fps)) != P_MAX_INDEX) {
    unsigned rate = str.Mid(pos+1).AsUnsigned();
    foundHint = info.SetFrameRate(rate);
  }

  return foundHint;
}


///////////////////////////////////////////////////////////////////////////////

PFACTORY_CREATE(PFactory<PVideoFile>, PYUVFile, "yuv", false);
static PFactory<PVideoFile>::Worker<PYUVFile> y4mFileFactory("y4m");


PYUVFile::PYUVFile()
  : y4mMode(PFalse)
{
}


PBoolean PYUVFile::Open(const PFilePath & name, PFile::OpenMode mode, int opts)
{
  if (!PVideoFile::Open(name, mode, opts))
    return PFalse;

  y4mMode = name.GetType() *= ".y4m";

  if (y4mMode) {
    int ch;
    do {
      if ((ch = file.ReadChar()) < 0)
        return PFalse;
    }
    while (ch != '\n');
    headerOffset = file.GetPosition();
  }

  return PTrue;
}


PBoolean PYUVFile::WriteFrame(const void * frame)
{
  if (y4mMode)
    file.WriteChar('\n');

  return file.Write(frame, frameBytes);
}


PBoolean PYUVFile::ReadFrame(void * frame)
{
  if (y4mMode) {
    int ch;
    do {
      if ((ch = file.ReadChar()) < 0)
        return PFalse;
    }
    while (ch != '\n');
  }

  return PVideoFile::ReadFrame(frame);
}


#endif  // P_VIDFILE
#endif
