/*
 * x224.cxx
 *
 * X.224 protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2000 Equivalence Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 21004 $
 * $Author: rjongbloed $
 * $Date: 2008-09-16 07:08:56 +0000 (Tue, 16 Sep 2008) $
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "x224.h"
#endif

#include <opal/buildopts.h>

#include <t120/x224.h>

#include <ptlib/sockets.h>


///////////////////////////////////////////////////////////////////////////////

X224::X224()
{
}


void X224::PrintOn(ostream & strm) const
{
  int indent = 2;
  strm << setprecision(indent) << "{\n"
       << setw(indent) << ' ' << "code=";
  switch (GetCode()) {
    case ConnectRequest :
      strm << "ConnectRequest";
      break;
    case ConnectConfirm :
      strm << "ConnectConfirm";
      break;
    case DataPDU :
      strm << "DataPDU";
  }

  char fillchar = strm.fill();

  strm << '\n'
       << setw(indent) << ' ' << "data: " << data.GetSize() << " bytes\n"
       << hex;

  PINDEX i = 0;
  while (i < data.GetSize()) {
    strm << setfill(' ') << setw(indent) << ' ' << setfill('0');
    PINDEX j;
    for (j = 0; j < 16; j++)
      if (i+j < data.GetSize())
        strm << setw(2) << (unsigned)data[i+j] << ' ';
      else
        strm << "   ";
    strm << "  ";
    for (j = 0; j < 16; j++) {
      if (i+j < data.GetSize()) {
        if (isprint(data[i+j]))
          strm << data[i+j];
        else
          strm << ' ';
      }
    }
    strm << '\n';
    i += 16;
  }
  strm << dec << setfill(fillchar)
       << setw(indent-1) << '}'
       << setprecision(indent-2);
}


PBoolean X224::Decode(const PBYTEArray & rawData)
{
  PINDEX packetLength = rawData.GetSize();

  PINDEX headerLength = rawData[0];
  if (packetLength < headerLength + 1) // Not enough bytes
    return PFalse;

  header.SetSize(headerLength);
  memcpy(header.GetPointer(), (const BYTE *)rawData+1, headerLength);

  packetLength -= headerLength + 1;
  data.SetSize(packetLength);
  if (packetLength > 0)
    memcpy(data.GetPointer(), (const BYTE *)rawData+headerLength+1, packetLength);

  return PTrue;
}


PBoolean X224::Encode(PBYTEArray & rawData) const
{
  PINDEX headerLength = header.GetSize();
  PINDEX dataLength = data.GetSize();

  if (!rawData.SetSize(headerLength + dataLength + 1))
    return PFalse;

  rawData[0] = (BYTE)headerLength;
  memcpy(rawData.GetPointer() + 1, header, headerLength);

  if (dataLength > 0)
    memcpy(rawData.GetPointer()+headerLength+1, data, dataLength);

  return PTrue;
}


void X224::BuildConnectRequest()
{
  data.SetSize(0);
  header.SetSize(6);
  header[0] = ConnectRequest;
  header[1] = 0;
  header[2] = 0x7b;
  header[3] = 2;
  header[4] = 0;
  header[5] = 0;
}


void X224::BuildConnectConfirm()
{
  data.SetSize(0);
  header.SetSize(6);
  header[0] = ConnectConfirm;
  header[1] = 0;
  header[2] = 0x7b;
  header[3] = 2;
  header[4] = 0;
  header[5] = 0;
}


void X224::BuildData(const PBYTEArray & d)
{
  header.SetSize(2);
  header[0] = DataPDU;
  header[1] = 0x80;
  data = d;
}



/////////////////////////////////////////////////////////////////////////////
