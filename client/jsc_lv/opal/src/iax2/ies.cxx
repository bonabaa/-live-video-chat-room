/*
 *
 * Inter Asterisk Exchange 2
 * 
 * Implementation of the classes to carry Information Elements
 * 
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2005 Indranet Technologies Ltd.
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Indranet Technologies Ltd.
 *
 * The author of this code is Derek J Smithies
 *
 * $Revision: 21293 $
 * $Author: rjongbloed $
 * $Date: 2008-10-12 23:24:41 +0000 (Sun, 12 Oct 2008) $
 */

#include <ptlib.h>
#include <opal/buildopts.h>

#if OPAL_IAX2

#ifdef P_USE_PRAGMA
#pragma implementation "ies.h"
#endif

#include <iax2/ies.h>
#include <iax2/frame.h>
#include <iax2/causecode.h>

#include <ptclib/cypher.h>

#define new PNEW

IAX2Ie::IAX2Ie()
{
  validData = PFalse;
}

IAX2Ie * IAX2Ie::BuildInformationElement(BYTE _typeCode, BYTE length, BYTE *srcData)
{
  switch (_typeCode) {
  case ie_calledNumber    : return new IAX2IeCalledNumber(length, srcData);
  case ie_callingNumber   : return new IAX2IeCallingNumber(length, srcData);
  case ie_callingAni      : return new IAX2IeCallingAni(length, srcData);
  case ie_callingName     : return new IAX2IeCallingName(length, srcData);
  case ie_calledContext   : return new IAX2IeCalledContext(length, srcData);
  case ie_userName        : return new IAX2IeUserName(length, srcData);
  case ie_password        : return new IAX2IePassword(length, srcData);
  case ie_capability      : return new IAX2IeCapability(length, srcData);
  case ie_format          : return new IAX2IeFormat(length, srcData);
  case ie_language        : return new IAX2IeLanguage(length, srcData);
  case ie_version         : return new IAX2IeVersion(length, srcData);
  case ie_adsicpe         : return new IAX2IeAdsicpe(length, srcData);
  case ie_dnid            : return new IAX2IeDnid(length, srcData);
  case ie_authMethods     : return new IAX2IeAuthMethods(length, srcData);
  case ie_challenge       : return new IAX2IeChallenge(length, srcData);
  case ie_md5Result       : return new IAX2IeMd5Result(length, srcData);
  case ie_rsaResult       : return new IAX2IeRsaResult(length, srcData);
  case ie_apparentAddr    : return new IAX2IeApparentAddr(length, srcData);
  case ie_refresh         : return new IAX2IeRefresh(length, srcData);
  case ie_dpStatus        : return new IAX2IeDpStatus(length, srcData);
  case ie_callNo          : return new IAX2IeCallNo(length, srcData);
  case ie_cause           : return new IAX2IeCause(length, srcData);
  case ie_iaxUnknown      : return new IAX2IeIaxUnknown(length, srcData);
  case ie_msgCount        : return new IAX2IeMsgCount(length, srcData);
  case ie_autoAnswer      : return new IAX2IeAutoAnswer(length, srcData);
  case ie_musicOnHold     : return new IAX2IeMusicOnHold(length, srcData);
  case ie_transferId      : return new IAX2IeTransferId(length, srcData);
  case ie_rdnis           : return new IAX2IeRdnis(length, srcData);
  case ie_provisioning    : return new IAX2IeProvisioning(length, srcData);
  case ie_aesProvisioning : return new IAX2IeAesProvisioning(length, srcData);
  case ie_dateTime        : return new IAX2IeDateTime(length, srcData);
  case ie_deviceType      : return new IAX2IeDeviceType(length, srcData);
  case ie_serviceIdent    : return new IAX2IeServiceIdent(length, srcData);
  case ie_firmwareVer     : return new IAX2IeFirmwareVer(length, srcData);
  case ie_fwBlockDesc     : return new IAX2IeFwBlockDesc(length, srcData);
  case ie_fwBlockData     : return new IAX2IeFwBlockData(length, srcData);
  case ie_provVer         : return new IAX2IeProvVer(length, srcData);
  case ie_callingPres     : return new IAX2IeCallingPres(length, srcData);
  case ie_callingTon      : return new IAX2IeCallingTon(length, srcData);
  case ie_callingTns      : return new IAX2IeCallingTns(length, srcData);
  case ie_samplingRate    : return new IAX2IeSamplingRate(length, srcData);
  case ie_causeCode       : return new IAX2IeCauseCode(length, srcData);
  case ie_encryption      : return new IAX2IeEncryption(length, srcData);
  case ie_encKey          : return new IAX2IeEncKey(length, srcData);
  case ie_codecPrefs      : return new IAX2IeCodecPrefs(length, srcData);
  case ie_recJitter       : return new IAX2IeReceivedJitter(length, srcData);
  case ie_recLoss         : return new IAX2IeReceivedLoss(length, srcData);
  case ie_recPackets      : return new IAX2IeDroppedFrames(length, srcData);
  case ie_recDelay        : return new IAX2IeReceivedDelay(length, srcData);
  case ie_recDropped      : return new IAX2IeDroppedFrames(length, srcData);
  case ie_recOoo          : return new IAX2IeReceivedOoo(length, srcData);
    
  default: PTRACE(1, "Ie\t Invalid IE type code " << ::hex << ((int)_typeCode) << ::dec);
  };
  
  return new IAX2IeInvalidElement();
}


void IAX2Ie::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " information element " ;
  else
    str << setw(17) << Class() << " information element-invalid data " ;
}


void IAX2Ie::WriteBinary(void *_data, PINDEX &writeIndex)
{
  BYTE *data = (BYTE *)_data;
  data[writeIndex] = GetKeyValue();
  data[writeIndex + 1] = GetLengthOfData();
  
  writeIndex +=2;
  
  WriteBinary(data + writeIndex);
  writeIndex += GetLengthOfData();
}

////////////////////////////////////////////////////////////////////////////////

IAX2IeNone::IAX2IeNone(BYTE /*length*/, BYTE * /*srcData*/)
{
  validData = PTrue;
}


void IAX2IeNone::PrintOn(ostream & str) const
{
  str << setw(17) << Class();
}

////////////////////////////////////////////////////////////////////////////////

IAX2IeDateAndTime::IAX2IeDateAndTime(BYTE length, BYTE *srcData)
{
  if (length != sizeof(unsigned int)) {
    validData = PFalse;
    return;
  }
  
  unsigned int tmp = (srcData[0] << 24) | (srcData[1] << 16) | (srcData[2] << 8) | (srcData[3]);
  int second = (tmp & 0x1f) << 1;
  int minute = (tmp >> 5) & 0x3f;
  int hour   = (tmp >> 11) & 0x1f;
  int day    = (tmp >> 16) & 0x1f;
  int month  = (tmp >> 21) & 0x0f;
  int year   = ((tmp >> 25) & 0x7f) + 2000;
  dataValue = PTime(second, minute, hour, day, month, year, PTime::Local);
  
  validData = PTrue;
}


void IAX2IeDateAndTime::PrintOn(ostream & str) const
{
  str << setw(17) << Class() << dataValue;
}

void IAX2IeDateAndTime::WriteBinary(BYTE *data)
{
  unsigned int second = dataValue.GetSecond() >> 1;
  unsigned int minute = dataValue.GetMinute()        << 5;
  unsigned int hour   = dataValue.GetHour()          << 11;
  unsigned int day    = dataValue.GetDay()           << 16;
  unsigned int month  = dataValue.GetMonth()         << 21;
  unsigned int year   = ((unsigned int)(dataValue.GetYear() - 2000)) << 25;
  
  unsigned int res = second | minute | hour | day | month | year;
  data[0] = (BYTE)((res >> 24) & 0xff);
  data[1] = (BYTE)((res >> 16) & 0xff);
  data[2] = (BYTE)((res >>  8) & 0xff);
  data[3] = (BYTE)((res      ) & 0xff);
}
////////////////////////////////////////////////////////////////////////////////

IAX2IeByte::IAX2IeByte(BYTE length, BYTE *srcData)
  : IAX2Ie()
{
  if (length != sizeof(BYTE)) {
    validData = PFalse;
    return;
  }
  
  validData = PTrue;
  dataValue = *((BYTE *)srcData);
}


void IAX2IeByte::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << ((int) dataValue);
  else
    str << setw(17) << Class() << " does not hold valid data" ;
}

////////////////////////////////////////////////////////////////////////////////
IAX2IeChar::IAX2IeChar(BYTE length, BYTE *srcData)
  : IAX2Ie()
{
  if (length != sizeof(BYTE)) {
    validData = PFalse;
    return;
  }
  
  validData = PTrue;
  dataValue = *((char *)srcData);
}


void IAX2IeChar::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not hold valid data" ;
}

////////////////////////////////////////////////////////////////////////////////

IAX2IeUShort::IAX2IeUShort(BYTE length, BYTE *srcData)
  : IAX2Ie()
{
  if (length != sizeof(unsigned short)) {
    validData = PFalse;
    return;
  }
  
  validData = PTrue;
  dataValue = (unsigned short)((srcData[0] << 8) | srcData[1]);
}


void IAX2IeUShort::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue << "UShort";
  else
    str << setw(17) << Class() << " does not hold valid data" ;
}

void IAX2IeUShort::WriteBinary(BYTE *data)
{
  data[0] = (BYTE)((dataValue >> 8) & 0xff);
  data[1] = (BYTE)(dataValue & 0xff);
}
////////////////////////////////////////////////////////////////////////////////

IAX2IeShort::IAX2IeShort(BYTE length, BYTE *srcData)
  : IAX2Ie()
{
  if (length != sizeof(short)) {
    validData = PFalse;
    return;
  }
  
  validData = PTrue;
  dataValue = (short)((srcData[0] << 8) | srcData[1]);
}


void IAX2IeShort::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not hold valid data" ;
}

void IAX2IeShort::WriteBinary(BYTE *data)
{
  data[0] = (BYTE)((dataValue >> 8) & 0xff);
  data[1] = (BYTE)(dataValue & 0xff);
}
////////////////////////////////////////////////////////////////////////////////

IAX2IeInt::IAX2IeInt(BYTE length, BYTE *srcData)
  : IAX2Ie()
{
  if (length != sizeof(int)) {
    validData = PFalse;
    return;
  }
  
  validData = PTrue;
  dataValue = (srcData[0] << 24) | (srcData[1] << 16) | (srcData[2] << 8) | srcData[3];
}


void IAX2IeInt::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not hold valid data" ;
}

void IAX2IeInt::WriteBinary(BYTE *data)
{
  data[0] = (BYTE)((dataValue >> 24) & 0xff);
  data[1] = (BYTE)((dataValue >> 16) & 0xff);
  data[2] = (BYTE)((dataValue >> 8) & 0xff);
  data[3] = (BYTE)(dataValue & 0xff);
}

////////////////////////////////////////////////////////////////////////////////

IAX2IeUInt::IAX2IeUInt(BYTE length, BYTE *srcData)
  : IAX2Ie()
{
  if (length != sizeof(unsigned int)) {
    validData = PFalse;
    return;
  }
  
  validData = PTrue;
  dataValue = (srcData[0] << 24) | (srcData[1] << 16) | (srcData[2] << 8) | srcData[3];     
}


void IAX2IeUInt::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not hold valid data" ;
}

void IAX2IeUInt::WriteBinary(BYTE *data)
{
  data[0] = (BYTE)((dataValue >> 24) & 0xff);
  data[1] = (BYTE)((dataValue >> 16) & 0xff);
  data[2] = (BYTE)((dataValue >> 8) & 0xff);
  data[3] = (BYTE)(dataValue & 0xff);
}
////////////////////////////////////////////////////////////////////////////////

IAX2IeString::IAX2IeString(BYTE length, BYTE *srcData)
  : IAX2Ie()
{
  validData = PTrue;
  dataValue = PString((const char *)srcData, length);
}


void IAX2IeString::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not hold valid data" ;
}

void IAX2IeString::WriteBinary(BYTE *data)
{
  if(validData)
    memcpy(data, dataValue.GetPointer(), GetLengthOfData());
}

BYTE IAX2IeString::GetLengthOfData() 
{ 
  if (dataValue.GetSize() == 0)
    return 0;
  else 
    return (BYTE)(dataValue.GetSize() - 1); 
}

void IAX2IeString::SetData(const PString & newData) 
{ 
  dataValue = newData; 
  validData = PTrue; 
}

void IAX2IeString::SetData(const char * newData) 
{ 
  dataValue = PString(newData); 
  validData = PTrue; 
}

////////////////////////////////////////////////////////////////////////////////

IAX2IeSockaddrIn::IAX2IeSockaddrIn(BYTE length, BYTE *srcData)
  : IAX2Ie()
{
  if (length != sizeof(sockaddr_in)) {
    validData = PFalse;
    return;
  }
  
  validData = PTrue;
  
  sockaddr_in a = * (sockaddr_in *)(srcData);
  portNumber = a.sin_port;
  
  dataValue = PIPSocket::Address(a.sin_addr);
}


void IAX2IeSockaddrIn::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue << ":" << portNumber;
  else
    str << setw(17) << Class() << " does not hold valid data" ;
}

void IAX2IeSockaddrIn::WriteBinary(BYTE *data)
{
  sockaddr_in a;
  a.sin_addr = (in_addr)dataValue;
  a.sin_port = (unsigned short)portNumber;
  
  *((sockaddr_in *)data) = a;
}

////////////////////////////////////////////////////////////////////////////////

IAX2IeBlockOfData::IAX2IeBlockOfData(BYTE length, BYTE *srcData)
  : IAX2Ie()
{
  validData = PTrue;
  
  dataValue = PBYTEArray(srcData, length);
}


void IAX2IeBlockOfData::PrintOn(ostream & str) const
{
  str << setw(17) << Class() << " " << dataValue;
}


void IAX2IeBlockOfData::WriteBinary(BYTE *data)
{
  memcpy(data, dataValue.GetPointer(), dataValue.GetSize());
}


////////////////////////////////////////////////////////////////////////////////

IAX2IeList::~IAX2IeList()
{
    AllowDeleteObjects();
}

IAX2Ie *IAX2IeList::RemoveIeAt(PINDEX i)
{ 
  if (i >= GetSize())
    return NULL;
  
  return (IAX2Ie *) PAbstractList::RemoveAt(i);
}

IAX2Ie *IAX2IeList::RemoveLastIe()
{
  PINDEX elems = PAbstractList::GetSize();
  if (elems > 0) {
    return RemoveIeAt(elems - 1);	  
  }
  
  return NULL;
}

void IAX2IeList::DeleteAt(PINDEX idex)
{
  if (idex >= PAbstractList::GetSize())
    return;
  
  IAX2Ie *obj = RemoveIeAt(idex);
  
  delete obj;
}

int IAX2IeList::GetBinaryDataSize() const
{
  PINDEX totalSize = 0;
  for (PINDEX i = 0; i < PAbstractList::GetSize(); i++)
    totalSize += GetIeAt(i)->GetBinarySize();
  
  return totalSize;
}

IAX2Ie *IAX2IeList::GetIeAt(int i) const
{
  if (i >= GetSize())
    return NULL;
  
  return (IAX2Ie *)GetAt(i); 
}

////////////////////////////////////////////////////////////////////////////////

void IAX2IeCalledNumber::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeCallingNumber::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeCallingAni::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeCallingName::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeCalledContext::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeUserName::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IePassword::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeCapability::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeFormat::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeLanguage::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeVersion::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeAdsicpe::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeDnid::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeAuthMethods::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeChallenge::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

IAX2IeMd5Result::IAX2IeMd5Result(IAX2Encryption & encryption)
{
  InitializeChallengePassword(encryption.ChallengeKey(), encryption.EncryptionKey());
}

IAX2IeMd5Result::IAX2IeMd5Result(const PString & challenge, const PString &password)
{
  InitializeChallengePassword(challenge, password);
}


void IAX2IeMd5Result::InitializeChallengePassword(const PString &newChallenge, const PString & newPassword)
{
  PMessageDigest5 stomach;
  stomach.Process(newChallenge);
  stomach.Process(newPassword);
  PMessageDigest5::Code digester;
  stomach.Complete(digester);

  dataBlock.SetSize(sizeof(digester));
  memcpy(dataBlock.GetPointer(), &digester, dataBlock.GetSize());
  
  PStringStream res;
  for (PINDEX i = 0; i < (PINDEX)sizeof(digester); i++) 
    res  << ::hex << ::setfill('0') << ::setw(2) << (int)(*(((BYTE *)&digester)+i));

  res.Trim();
  res.MakeMinimumSize();
  
  SetData(res);

  PTRACE(3, "IAX2IeMd5Result\tChallenge is " << newChallenge);
  PTRACE(3, "IAX2IeMd5Result\tPassword  is " << newPassword);
  PTRACE(3, "IAX2IeMd5Result\tresult    is " << res);
}

void IAX2IeMd5Result::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeRsaResult::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeApparentAddr::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeRefresh::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeDpStatus::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeCallNo::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeCause::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " \"" << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeIaxUnknown::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeMsgCount::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeAutoAnswer::PrintOn(ostream & str) const
{
  str << setw(17) << Class() << "   key(" << ((int) GetKeyValue()) << ")";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeMusicOnHold::PrintOn(ostream & str) const
{
  str << setw(17) << Class() << "    key(" << ((int) GetKeyValue()) << ")";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeTransferId::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeRdnis::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeProvisioning::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeAesProvisioning::PrintOn(ostream & str) const
{
  str << setw(17) << Class() << "   key(" << ((int) GetKeyValue()) << ")";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeDateTime::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeDeviceType::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeServiceIdent::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeFirmwareVer::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeFwBlockDesc::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeFwBlockData::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeProvVer::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeCallingPres::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeCallingTon::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeCallingTns::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeSamplingRate::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

IAX2IeEncryption::IAX2IeEncryption(IAX2IeEncryptionMethod method)
{
  SetData((unsigned short)method);
}

void IAX2IeEncryption::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeEncKey::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeCodecPrefs::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << dataValue;
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////
void IAX2IeCauseCode::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << ((int)dataValue);
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeReceivedJitter::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << ((unsigned int)dataValue);
  else
    str << setw(17) << Class() << " does not contain valid data";
}

////////////////////////////////////////////////////////////////////////////////

void IAX2IeReceivedLoss::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << ((unsigned int)dataValue);
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////
void IAX2IeReceivedFrames::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << ((unsigned int)dataValue);
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeReceivedDelay::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << ((unsigned short)dataValue);
  else
    str << setw(17) << Class() << " does not contain valid data";
}

////////////////////////////////////////////////////////////////////////////////

void IAX2IeDroppedFrames::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << ((unsigned int)dataValue);
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////

void IAX2IeReceivedOoo::PrintOn(ostream & str) const
{
  if (validData)
    str << setw(17) << Class() << " " << ((unsigned int)dataValue);
  else
    str << setw(17) << Class() << " does not contain valid data";
}
////////////////////////////////////////////////////////////////////////////////


#endif // OPAL_IAX2

////////////////////////////////////////////////////////////////////////////////
/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 4 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-file-style:linux
 * c-basic-offset:2
 * End:
 */

