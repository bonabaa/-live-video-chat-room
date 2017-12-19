/*
 * asnber.cxx
 *
 * Abstract Syntax Notation 1 Encoding Rules
 *
 * Portable Windows Library
 *
 */

///////////////////////////////////////////////////////////////////////
//#include "ptclib/asnber.h"
PBoolean PBER_Stream::NullDecode(PASN_Null & value)
{
  unsigned len;
  if (!HeaderDecode(value, len))
    return PFalse;

  byteOffset += len;
  return PTrue;
}


void PBER_Stream::NullEncode(const PASN_Null & value)
{
  HeaderEncode(value);
}

///////////////////////////////////////////////////////////////////////

PBoolean PBER_Stream::BooleanDecode(PASN_Boolean & value)
{
  unsigned len;
  if (!HeaderDecode(value, len))
    return PFalse;

  while (len-- > 0) {
    if (IsAtEnd())
      return PFalse;
    value = (PBoolean)ByteDecode();
  }

  return PTrue;
}


void PBER_Stream::BooleanEncode(const PASN_Boolean & value)
{
  HeaderEncode(value);
  ByteEncode((PBoolean)value);
}

///////////////////////////////////////////////////////////////////////

PBoolean PBER_Stream::IntegerDecode(PASN_Integer & value)
{
  unsigned len;
  if (!HeaderDecode(value, len) || len == 0 || IsAtEnd())
    return PFalse;

  int accumulator = (char)ByteDecode(); // sign extended first byte
  while (--len > 0) {
    if (IsAtEnd())
      return PFalse;
    accumulator = (accumulator << 8) | ByteDecode();
  }

  value = accumulator;
  return PTrue;
}


void PBER_Stream::IntegerEncode(const PASN_Integer & value)
{
  HeaderEncode(value);
  // output the integer bits
  for (int count = GetIntegerDataLength(value)-1; count >= 0; count--)
    ByteEncode(value >> (count*8));
}

///////////////////////////////////////////////////////////////////////

PBoolean PBER_Stream::EnumerationDecode(PASN_Enumeration & value)
{
  unsigned len;
  if (!HeaderDecode(value, len) || len == 0 || IsAtEnd())
    return PFalse;

  unsigned val = 0;
  while (len-- > 0) {
    if (IsAtEnd())
      return PFalse;
    val = (val << 8) | ByteDecode();
  }

  value = val;
  return PTrue;
}


void PBER_Stream::EnumerationEncode(const PASN_Enumeration & value)
{
  HeaderEncode(value);
  // output the integer bits
  for (int count = GetIntegerDataLength(value)-1; count >= 0; count--)
    ByteEncode(value >> (count*8));
}

///////////////////////////////////////////////////////////////////////

PBoolean PBER_Stream::RealDecode(PASN_Real & value)
{
  unsigned len;
  if (!HeaderDecode(value, len) || len == 0 || IsAtEnd())
    return PFalse;

  PAssertAlways(PUnimplementedFunction);
  byteOffset += len;

  return PTrue;
}


void PBER_Stream::RealEncode(const PASN_Real &)
{
  PAssertAlways(PUnimplementedFunction);
}

///////////////////////////////////////////////////////////////////////

PBoolean PBER_Stream::ObjectIdDecode(PASN_ObjectId & value)
{
  unsigned len;
  if (!HeaderDecode(value, len))
    return PFalse;

  return value.CommonDecode(*this, len);
}


void PBER_Stream::ObjectIdEncode(const PASN_ObjectId & value)
{
  HeaderEncode(value);
  PBYTEArray data;
  value.CommonEncode(data);
  BlockEncode(data, data.GetSize());
}

///////////////////////////////////////////////////////////////////////

PBoolean PASN_BitString::DecodeBER(PBER_Stream & strm, unsigned len)
{
  totalBits = len*8 - strm.ByteDecode();
  unsigned nBytes = (totalBits+7)/8;
  return strm.BlockDecode(bitData.GetPointer(nBytes), nBytes) == nBytes;
}


void PASN_BitString::EncodeBER(PBER_Stream & strm) const
{
  if (totalBits == 0)
    strm.ByteEncode(0);
  else {
    strm.ByteEncode(8-totalBits%8);
    strm.BlockEncode(bitData, (totalBits+7)/8);
  }
}

///////////////////////////////////////////////////////////////////////

PBoolean PBER_Stream::BitStringDecode(PASN_BitString & value)
{
  unsigned len;
  if (!HeaderDecode(value, len) || len == 0 || IsAtEnd())
    return PFalse;

  return value.DecodeBER(*this, len);
}


void PBER_Stream::BitStringEncode(const PASN_BitString & value)
{
  HeaderEncode(value);
  value.EncodeBER(*this);
}

///////////////////////////////////////////////////////////////////////

PBoolean PBER_Stream::OctetStringDecode(PASN_OctetString & value)
{
  unsigned len;
  if (!HeaderDecode(value, len))
    return PFalse;

  return BlockDecode(value.GetPointer(len), len) == len;
}


void PBER_Stream::OctetStringEncode(const PASN_OctetString & value)
{
  HeaderEncode(value);
  BlockEncode(value, value.GetSize());
}

///////////////////////////////////////////////////////////////////////

PBoolean PASN_ConstrainedString::DecodeBER(PBER_Stream & strm, unsigned len)
{
  return strm.BlockDecode((BYTE *)value.GetPointer(len+1), len) == len;
}


void PASN_ConstrainedString::EncodeBER(PBER_Stream & strm) const
{
  strm.BlockEncode(value, value.GetSize()-1);
}

///////////////////////////////////////////////////////////////////////

PBoolean PBER_Stream::ConstrainedStringDecode(PASN_ConstrainedString & value)
{
  unsigned len;
  if (!HeaderDecode(value, len))
    return PFalse;

  return value.DecodeBER(*this, len);
}


void PBER_Stream::ConstrainedStringEncode(const PASN_ConstrainedString & value)
{
  HeaderEncode(value);
  value.EncodeBER(*this);
}

///////////////////////////////////////////////////////////////////////

PBoolean PASN_BMPString::DecodeBER(PBER_Stream & strm, unsigned len)
{
  value.SetSize(len/2);
  return strm.BlockDecode((BYTE *)value.GetPointer(len), len) == len;
}


void PASN_BMPString::EncodeBER(PBER_Stream & strm) const
{
  strm.BlockEncode((const BYTE *)(const wchar_t *)value, value.GetSize()*2);
}

///////////////////////////////////////////////////////////////////////

PBoolean PBER_Stream::BMPStringDecode(PASN_BMPString & value)
{
  unsigned len;
  if (!HeaderDecode(value, len))
    return PFalse;

  return value.DecodeBER(*this, len);
}


void PBER_Stream::BMPStringEncode(const PASN_BMPString & value)
{
  HeaderEncode(value);
  value.EncodeBER(*this);
}

///////////////////////////////////////////////////////////////////////

PBoolean PBER_Stream::ChoiceDecode(PASN_Choice & value)
{
  PINDEX savedPosition = GetPosition();

  unsigned tag;
  PASN_Object::TagClass tagClass;
  PBoolean primitive;
  unsigned entryLen;
  if (!HeaderDecode(tag, tagClass, primitive, entryLen))
    return PFalse;

  SetPosition(savedPosition);

  value.SetTag(tag, tagClass);
  if (value.IsValid())
    return value.GetObject().Decode(*this);

  return PTrue;
}

void PBER_Stream::ChoiceEncode(const PASN_Choice & value)
{
  if (value.IsValid())
    value.GetObject().Encode(*this);
}

///////////////////////////////////////////////////////////////////////

PBoolean PASN_Sequence::PreambleDecodeBER(PBER_Stream & strm)
{
  fields.RemoveAll();

  unsigned len;
  if (!strm.HeaderDecode(*this, len))
    return PFalse;

  endBasicEncoding = strm.GetPosition() + len;
  return !strm.IsAtEnd();
}


void PASN_Sequence::PreambleEncodeBER(PBER_Stream & strm) const
{
  strm.HeaderEncode(*this);
}


PBoolean PASN_Sequence::KnownExtensionDecodeBER(PBER_Stream & strm, PINDEX, PASN_Object & field)
{
  if (strm.GetPosition() >= endBasicEncoding)
    return PFalse;

  return field.Decode(strm);
}


void PASN_Sequence::KnownExtensionEncodeBER(PBER_Stream & strm, PINDEX, const PASN_Object & field) const
{
  field.Encode(strm);
}


PBoolean PASN_Sequence::UnknownExtensionsDecodeBER(PBER_Stream & strm)
{
  while (strm.GetPosition() < endBasicEncoding) {
    PINDEX savedPosition = strm.GetPosition();

    unsigned tag;
    PASN_Object::TagClass tagClass;
    PBoolean primitive;
    unsigned entryLen;
    if (!strm.HeaderDecode(tag, tagClass, primitive, entryLen))
      return PFalse;

    PINDEX nextEntryPosition = strm.GetPosition() + entryLen;
    strm.SetPosition(savedPosition);

    PASN_Object * obj = strm.CreateObject(tag, tagClass, primitive);
    if (obj == NULL)
      strm.SetPosition(nextEntryPosition);
    else {
      if (!obj->Decode(strm))
        return PFalse;

      fields.Append(obj);
    }
  }

  return PTrue;
}


void PASN_Sequence::UnknownExtensionsEncodeBER(PBER_Stream & strm) const
{
  for (PINDEX i = 0; i < fields.GetSize(); i++)
    fields[i].Encode(strm);
}

///////////////////////////////////////////////////////////////////////

PBoolean PBER_Stream::SequencePreambleDecode(PASN_Sequence & seq)
{
  return seq.PreambleDecodeBER(*this);
}


void PBER_Stream::SequencePreambleEncode(const PASN_Sequence & seq)
{
  seq.PreambleEncodeBER(*this);
}


PBoolean PBER_Stream::SequenceKnownDecode(PASN_Sequence & seq, PINDEX fld, PASN_Object & field)
{
  return seq.KnownExtensionDecodeBER(*this, fld, field);
}


void PBER_Stream::SequenceKnownEncode(const PASN_Sequence & seq, PINDEX fld, const PASN_Object & field)
{
  seq.KnownExtensionEncodeBER(*this, fld, field);
}


PBoolean PBER_Stream::SequenceUnknownDecode(PASN_Sequence & seq)
{
  return seq.UnknownExtensionsDecodeBER(*this);
}


void PBER_Stream::SequenceUnknownEncode(const PASN_Sequence & seq)
{
  seq.UnknownExtensionsEncodeBER(*this);
}

///////////////////////////////////////////////////////////////////////

PBoolean PBER_Stream::ArrayDecode(PASN_Array & array)
{
  array.RemoveAll();

  unsigned len;
  if (!HeaderDecode(array, len))
    return PFalse;

  PINDEX endOffset = byteOffset + len;
  PINDEX count = 0;
  while (byteOffset < endOffset) {
    if (!array.SetSize(count+1))
      return PFalse;
    if (!array[count].Decode(*this))
      return PFalse;
    count++;
  }

  byteOffset = endOffset;

  return PTrue;
}


void PBER_Stream::ArrayEncode(const PASN_Array & array)
{
  HeaderEncode(array);
  for (PINDEX i = 0; i < array.GetSize(); i++)
    array[i].Encode(*this);
}

///////////////////////////////////////////////////////////////////////

PBER_Stream::PBER_Stream()
{
}


PBER_Stream::PBER_Stream(const PBYTEArray & bytes)
  : PASN_Stream(bytes)
{
}


PBER_Stream::PBER_Stream(const BYTE * buf, PINDEX size)
  : PASN_Stream(buf, size)
{
}


PBER_Stream & PBER_Stream::operator=(const PBYTEArray & bytes)
{
  PBYTEArray::operator=(bytes);
  ResetDecoder();
  return *this;
}

PBoolean PBER_Stream::Read(PChannel & chan)
{
  SetSize(0);
  PINDEX offset = 0;

  // read the sequence header
  int b;
  if ((b = chan.ReadChar()) < 0)
    return PFalse;

  SetAt(offset++, (char)b);

  // only support direct read of simple sequences
  if ((b&0x1f) == 0x1f) {
    do {
      if ((b = chan.ReadChar()) < 0)
        return PFalse;
      SetAt(offset++, (char)b);
    } while ((b & 0x80) != 0);
  }

  // read the first byte of the ASN length
  if ((b = chan.ReadChar()) < 0)
    return PFalse;

  SetAt(offset++, (char)b);

  // determine how many bytes in the length
  PINDEX dataLen = 0;
  if ((b & 0x80) == 0)
    dataLen = b;
  else {
    PINDEX lenLen = b&0x7f;
    SetSize(lenLen+2);
    while (lenLen-- > 0) {
      // read the length
      if ((b = chan.ReadChar()) < 0)
        return PFalse;
      dataLen = (dataLen << 8) | b;
      SetAt(offset++, (char)b);
    }
  }

  // read the data, all of it
  BYTE * bufptr = GetPointer(dataLen+offset) + offset;
  while (dataLen > 0) {
    if (!chan.Read(bufptr, dataLen))
      return PFalse;
    PINDEX readbytes = chan.GetLastReadCount();
    bufptr += readbytes;
    dataLen -= readbytes;
  }
  return PTrue;
}


PBoolean PBER_Stream::Write(PChannel & chan)
{
  CompleteEncoding();
  return chan.Write(theArray, GetSize());
}


PASN_Object * PBER_Stream::CreateObject(unsigned tag,
                                        PASN_Object::TagClass tagClass,
                                        PBoolean primitive) const
{
  if (tagClass == PASN_Object::UniversalTagClass) {
    switch (tag) {
      case PASN_Object::UniversalBoolean :
        return new PASN_Boolean();

      case PASN_Object::UniversalInteger :
        return new PASN_Integer();

      case PASN_Object::UniversalBitString :
        return new PASN_BitString();

      case PASN_Object::UniversalOctetString :
        return new PASN_OctetString();

      case PASN_Object::UniversalNull :
        return new PASN_Null();

      case PASN_Object::UniversalObjectId :
        return new PASN_ObjectId();

      case PASN_Object::UniversalReal :
        return new PASN_Real();

      case PASN_Object::UniversalEnumeration :
        return new PASN_Enumeration();

      case PASN_Object::UniversalSequence :
        return new PASN_Sequence();

      case PASN_Object::UniversalSet :
        return new PASN_Set();

      case PASN_Object::UniversalNumericString :
        return new PASN_NumericString();

      case PASN_Object::UniversalPrintableString :
        return new PASN_PrintableString();

      case PASN_Object::UniversalIA5String :
        return new PASN_IA5String();

      case PASN_Object::UniversalVisibleString :
        return new PASN_VisibleString();

      case PASN_Object::UniversalGeneralString :
        return new PASN_GeneralString();

      case PASN_Object::UniversalBMPString :
        return new PASN_BMPString();
    }
  }

  if (primitive)
    return new PASN_OctetString(tag, tagClass);
  else
    return new PASN_Sequence(tag, tagClass, 0, PFalse, 0);
}

PBoolean PBER_Stream::HeaderDecode(unsigned & tagVal,
                               PASN_Object::TagClass & tagClass,
                               PBoolean & primitive,
                               unsigned & len)
{
  BYTE ident = ByteDecode();
  tagClass = (PASN_Object::TagClass)(ident>>6);
  primitive = (ident&0x20) == 0;
  tagVal = ident&31;
  if (tagVal == 31) {
    BYTE b;
    tagVal = 0;
    do {
      if (IsAtEnd())
        return PFalse;

      b = ByteDecode();
      tagVal = (tagVal << 7) | (b&0x7f);
    } while ((b&0x80) != 0);
  }

  if (IsAtEnd())
    return PFalse;

  BYTE len_len = ByteDecode();
  if ((len_len & 0x80) == 0) {
    len = len_len;
    return PTrue;
  }

  len_len &= 0x7f;

  len = 0;
  while (len_len-- > 0) {
    if (IsAtEnd())
      return PFalse;

    len = (len << 8) | ByteDecode();
  }

  return PTrue;
}


PBoolean PBER_Stream::HeaderDecode(PASN_Object & obj, unsigned & len)
{
  PINDEX pos = byteOffset;

  unsigned tagVal;
  PASN_Object::TagClass tagClass;
  PBoolean primitive;
  if (HeaderDecode(tagVal, tagClass, primitive, len) &&
              tagVal == obj.GetTag() && tagClass == obj.GetTagClass())
    return PTrue;

  byteOffset = pos;
  return PFalse;
}


void PBER_Stream::HeaderEncode(const PASN_Object & obj)
{
  BYTE ident = (BYTE)(obj.GetTagClass() << 6);
  if (!obj.IsPrimitive())
    ident |= 0x20;
  unsigned tag = obj.GetTag();
  if (tag < 31)
    ByteEncode(ident|tag);
  else {
    ByteEncode(ident|31);
    unsigned count = (CountBits(tag)+6)/7;
    while (count-- > 1)
      ByteEncode((tag >> (count*7))&0x7f);
    ByteEncode(tag&0x7f);
  }

  PINDEX len = obj.GetDataLength();
  if (len < 128)
    ByteEncode(len);
  else {
    PINDEX count = (CountBits(len+1)+7)/8;
    ByteEncode(count|0x80);
    while (count-- > 0)
      ByteEncode(len >> (count*8));
  }
}

///////////////////////////////////////////////////////////////////////
