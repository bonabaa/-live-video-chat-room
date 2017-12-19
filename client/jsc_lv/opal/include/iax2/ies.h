/*
 *
 * Inter Asterisk Exchange 2
 * 
 * The classes used to hold Information Elements.
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
 * $Revision: 23295 $
 * $Author: rjongbloed $
 * $Date: 2009-08-28 06:38:49 +0000 (Fri, 28 Aug 2009) $
 */

#ifndef OPAL_IAX2_IES_H
#define OPAL_IAX2_IES_H

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal/buildopts.h>

#if OPAL_IAX2

#include <ptlib/sockets.h>
#include <iax2/iedata.h>

#ifdef P_USE_PRAGMA
#pragma interface
#endif


class IAX2Ie;
class IAX2Encryption;

/**Ie class is for handling information elements*/
class IAX2Ie : public PObject
{ 
  PCLASSINFO(IAX2Ie, PObject);
 public:
  /** Each of the 45 possible Information Element types */
  enum IAX2IeTypeCode {    
    ie_calledNumber      = 1,     /*!< Number or extension that is being being called  (string)    */
    ie_callingNumber     = 2,     /*!< The number of the node initating the call r (string)   */
    ie_callingAni        = 3,     /*!< The ANI (calling number) to use for billing   (string)   */
    ie_callingName       = 4,     /*!< The callers name (string)    */
    ie_calledContext     = 5,     /*!< The context we are calling to (string) */
    ie_userName          = 6,     /*!< UserName (peer or user) to use in the  authentication process (string)    */
    ie_password          = 7,     /*!< Password - which is used in the authentication process (string)    */
    ie_capability        = 8,     /*!< Bitmask of the codecs sending end supports or senders capability  (unsigned int)    */
    ie_format            = 9,     /*!< Bitmask of the desired codec  (unsigned int)    */
    ie_language          = 10,    /*!< Sending ends preferred language string)    */
    ie_version           = 11,    /*!< Sending ends protocol version - typically 2.  (short)    */
    ie_adsicpe           = 12,    /*!< CPE ADSI capability (short)    */
    ie_dnid              = 13,    /*!< Originally dialed DNID (string)    */
    ie_authMethods       = 14,    /*!< Bitmask of the available Authentication method(s)  (short)    */
    ie_challenge         = 15,    /*!< The challenge data used in  MD5/RSA authentication (string)    */
    ie_md5Result         = 16,    /*!< MD5 challenge (authentication process) result (string)    */
    ie_rsaResult         = 17,    /*!< RSA challenge (authentication process) result (string)    */
    ie_apparentAddr      = 18,    /*!< The peer's apparent address. (struct sockaddr_in)    */
    ie_refresh           = 19,    /*!< The frequency of on with to refresh registration, in units of seconds  (short)    */
    ie_dpStatus          = 20,    /*!< The dialplan status (short)    */
    ie_callNo            = 21,    /*!< Call number of the  peer  (short)    */
    ie_cause             = 22,    /*!< Description of why hangup or authentication or other failure happened (string)    */
    ie_iaxUnknown        = 23,    /*!< An IAX command that is unknown (byte)    */
    ie_msgCount          = 24,    /*!< A report on the number of  messages waiting (short)    */
    ie_autoAnswer        = 25,    /*!< auto-answering is requested (no type required, as this is a request) */
    ie_musicOnHold       = 26,    /*!< Demand for music on hold combined with  QUELCH (string or none)    */
    ie_transferId        = 27,    /*!< Identifier for a Transfer Request (int)    */
    ie_rdnis             = 28,    /*!< DNIS of the referring agent (string)    */
    ie_provisioning      = 29,    /*!< Info to be used for provisioning   (binary data)  */
    ie_aesProvisioning   = 30,    /*!< Info for provisioning AES      (binary data) */
    ie_dateTime          = 31,    /*!< Date and Time  (which is stored in binary format defined in IAX2IeDateTime)    */
    ie_deviceType        = 32,    /*!< The type of device  (string)    */
    ie_serviceIdent      = 33,    /*!< The Identifier for service (string)   */
    ie_firmwareVer       = 34,    /*!< The version of firmware    (unsigned 32 bit number)    */
    ie_fwBlockDesc       = 35,    /*!< The description for a block of firmware (unsigned 32 bit number ) */
    ie_fwBlockData       = 36,    /*!< Binary data for a block of fw   () */
    ie_provVer           = 37,    /*!< The version of provisiong     (unsigned 32 bit number)    */
    ie_callingPres       = 38,    /*!< Presentation of calling (unsigned 8 bit) */
    ie_callingTon        = 39,    /*!< Type of number calling (unsigned 8 bit)   */
    ie_callingTns        = 40,    /*!< Transit Network Select for calling (unsigned 16 bit)  */
    ie_samplingRate      = 41,    /*!< Bitmask of supported Rate of sampling . Sampling defaults to 8000 hz, (unsigned 16)    */
    ie_causeCode         = 42,    /*!< Code value which describes why the registration failed, or call hungup etc (unsigned 8 bit) */
    ie_encryption        = 43,    /*!< The method for encrption the remote code wants to use (U16)    */
    ie_encKey            = 44,    /*!< the Key for ncryption (raw binary data)    */
    ie_codecPrefs        = 45,    /*!< The codec we would prefer to use (unsigned 8 bit)    */
    ie_recJitter         = 46,    /*!< From rfc 1889, the received jitter (unsigned 32 bit number) */
    ie_recLoss           = 47,    /*!< The received loss rate, where the high byte is the loss packt, low 24 bits loss count, from rfc1889 (unsigned 32 bit)*/
    ie_recPackets        = 48,    /*!< Recevied number of frames (total frames received) (unsigned 32 bit) */
    ie_recDelay          = 49,    /*!< Received frame largest playout delay (in ms) (unsigned 16 bit)*/
    ie_recDropped        = 50,    /*!< The number of dropped received frames by the jitter buf, so it is a measure of the number of late frames (unsigned 32 bit) */
    ie_recOoo            = 51     /*!< The number of received frames which were Out Of Order (unsigned 32 number) */
  };
  
  /**@name construction/destruction*/
  //@{
  
  /**Constructor*/
  IAX2Ie();
  
  /**Destructor*/
  virtual ~IAX2Ie() { };
  //@{
  
  /**@name Worker methods*/
  //@{
  /** Given an arbitrary type code, build & initialise the IAX2Ie descendant class */
  static IAX2Ie *BuildInformationElement(BYTE _typeCode, BYTE length, BYTE *srcData);     
  
  /** returns PTrue if contains an initialised    information element */
  virtual PBoolean IsValid() { return validData; }
  
  /**return the number of bytes to hold this data element */
  virtual BYTE GetLengthOfData() { return 0; }
  
  /**return the number of bytes to hold this Information Element when stored in a packet*/
  int GetBinarySize() { return 2 + GetLengthOfData(); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /** Get the enum value for this information element class */
  virtual BYTE GetKeyValue() const  { return 255; }
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(int &/*newData*/) { PAssertAlways("Ie class cannnot set data value"); };
  
  /**Read the value of the stored data for this class */
  int ReadData() { PAssertAlways("Ie class cannot read the internal data value"); return 0; };
  
  /** Take the data from this IAX2Ie and copy into the memory region.
      When finished, increment the writeIndex appropriately. */
  void WriteBinary(void *data, PINDEX & writeIndex);
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &/*res*/) { PAssertAlways(PUnimplementedFunction); }     
  //@}
  
 protected:
  /** Take the data value for this particular IAX2Ie and copy into the memory region.*/
  virtual void WriteBinary(BYTE * /*data*/) { PAssertAlways(PUnimplementedFunction); }
  
  /**A flag indicating if the data was read from the supplied bytes
     correctly, or if this structure is yet to be initialised */
  PBoolean   validData;
};

/////////////////////////////////////////////////////////////////////////////    
/**An Information Element that identifies an invalid code in the processed binary data.*/
class IAX2IeInvalidElement : public IAX2Ie
{
  PCLASSINFO(IAX2IeInvalidElement, IAX2Ie);
 public:
  IAX2IeInvalidElement() : IAX2Ie() {};
  
  /**Access function to get the length of data, which is used when writing to network packet*/
  virtual BYTE GetlengthOfData() { return 0; }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const
    { str << "Invlalid Information Element" << endl; }
 protected:
  /** Take the data value for this particular IAX2Ie and copy into the memory region.
      For an IAX2IeInvalidElement, there is no work done, as the data is invalid. */
  virtual void WriteBinary(BYTE * /*data*/) {  }
};
/////////////////////////////////////////////////////////////////////////////    
/**An Information Element that contains no data. */
class IAX2IeNone : public IAX2Ie
{
  PCLASSINFO(IAX2IeNone, IAX2Ie);
  /**@name construction/destruction*/
  //@{
  
  /**Constructor - read data from source array. 
     
  Contents are valid if source array is valid. */
  IAX2IeNone(BYTE length, BYTE *srcData);     
  
  /**Constructor to an invalid and empty result*/
  IAX2IeNone() : IAX2Ie() {}
  //@}
  
  /**@name Worker methods*/
  //@{  
  /**return the number of bytes to hold this data element */
  virtual BYTE GetLengthOfData() { return 0; }
  
  /**Report the value stored in this class */
  BYTE GetValue() { return 0; }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(void * /*newData*/) { PAssertAlways("IeNone cannot set data"); }
  
  /**Read the value of the stored data for this class */
  int ReadData() { PAssertAlways("IeNone cannot read the internal data value"); return 0; }
  
  //@}
 protected:
  /** Take the data value for this particular IAX2Ie and copy into the memory region.
      For this IAX2Ie (IAX2IeNone) there is no work to do as there is no data. */
  virtual void WriteBinary(BYTE * /*data*/) {  }
};

/////////////////////////////////////////////////////////////////////////////    
/**An Information Element that contains one byte of data*/
class IAX2IeByte : public IAX2Ie
{
  PCLASSINFO(IAX2IeByte, IAX2Ie);
  /**@name construction/destruction*/
  //@{
  
  /**Constructor - read data from source array. 
     
  Contents are valid if source array is valid. */  
  IAX2IeByte(BYTE length, BYTE *srcData);     
  
  /**Constructor to a specific value */
  IAX2IeByte(BYTE newValue) : IAX2Ie() { SetData(newValue); }
  
  /**Constructor to an invalid and empty result*/
  IAX2IeByte() : IAX2Ie() { }
  //@}
  
  /**@name Worker methods*/
  //@{
  /**return the number of bytes to hold this data element */
  virtual BYTE GetLengthOfData() { return sizeof(dataValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(BYTE newData) { dataValue = newData; validData = PTrue; }
  
  /**Report the value of the stored data for this class */
  BYTE ReadData() { return dataValue; }
  
  //@}
 protected:
  /** Take the data value for this particular IAX2Ie and copy into the memory region.*/
  virtual void WriteBinary(BYTE *data) { data[0] = dataValue; }
  
  /**The actual data stored in a IAX2IeByte class */
  BYTE dataValue;
};

/////////////////////////////////////////////////////////////////////////////    
/**An Information Element that contains one character of data*/
class IAX2IeChar : public IAX2Ie
{
  PCLASSINFO(IAX2IeChar, IAX2Ie);
  /**@name construction/destruction*/
  //@{
  
  /**Constructor - read data from source array. 
     
  Contents are valid if source array is valid. */
  IAX2IeChar(BYTE length, BYTE *srcData);     
  
  /**Construct to an initialised value */
  IAX2IeChar(char newValue) : IAX2Ie() { SetData(newValue); }
  
  /**Constructor to an invalid and empty result*/
  IAX2IeChar() : IAX2Ie() { }
  //@}
  
  /**@name Worker methods*/
  //@{
  /**return the number of bytes to hold this data element */
  virtual BYTE GetLengthOfData() { return sizeof(dataValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(char newData) { dataValue = newData; validData = PTrue; }
  
  /**Report the value of the stored data for this class */
  char ReadData() { return dataValue; }
  
  //@}
 protected:
  /** Take the data value for this particular IAX2Ie and copy into the memory region.*/
  virtual void WriteBinary(BYTE *data) { data[0] = dataValue; }
  
  /**The actual data stored in a IAX2IeChar class */
  char dataValue;
};

/////////////////////////////////////////////////////////////////////////////    
/**An Information Element that contains one short (signed 16 bits) of data*/
class IAX2IeShort : public IAX2Ie
{
  PCLASSINFO(IAX2IeShort, IAX2Ie);
  /**@name construction/destruction*/
  //@{
  
  /**Constructor - read data from source array. 
     
  Contents are valid if source array is valid. */
  IAX2IeShort(BYTE length, BYTE *srcData);     
  
  /**Construct to an initialied value */
  IAX2IeShort(short newValue) : IAX2Ie() { SetData(newValue); }
  
  /**Constructor to an invalid and empty result*/
  IAX2IeShort() : IAX2Ie() { }
  //@}
  
  /**@name Worker methods*/
  //@{
  /**return the number of bytes to hold this data element */
  virtual BYTE GetLengthOfData() { return sizeof(dataValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(short newData) { dataValue = newData; validData = PTrue; }
  
  /**Report the value of the stored data for this class */
  short ReadData() { return dataValue; }  
  //@}
 protected:
  /** Take the data value for this particular IAX2Ie and copy into the memory region.*/
  virtual void WriteBinary(BYTE *data);
  
  /**The actual data stored in a IAX2IeShort class */
  short dataValue;
};
/////////////////////////////////////////////////////////////////////////////    
/**An Information Element that contains one integer (signed 16 bits) of data*/
class IAX2IeInt : public IAX2Ie
{
  PCLASSINFO(IAX2IeInt, IAX2Ie);
  /**@name construction/destruction*/
  //@{
  
  /**Constructor - read data from source array. 
     
  Contents are valid if source array is valid. */
  IAX2IeInt(BYTE length, BYTE *srcData);     
  
  /**Construct to an initialised value */
  IAX2IeInt(int  newValue) : IAX2Ie() { SetData(newValue); }
  
  /**Constructor to an invalid and empty result*/
  IAX2IeInt() : IAX2Ie() { }
  //@}
  
  /**@name Worker methods*/
  //@{
  /**return the number of bytes to hold this data element */
  virtual BYTE GetLengthOfData() { return sizeof(dataValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(int newData) { dataValue = newData; validData = PTrue; }
  
  /**Report the value of the stored data for this class */
  int ReadData() { return dataValue; }
  
  //@}
 protected:
  /** Take the data value for this particular IAX2Ie and copy into the memory region.*/
  virtual void WriteBinary(BYTE *data);
  
  /**The actual data stored in a IAX2IeInt class */
  int dataValue;
};
////////////////////////////////////////////////////////////////////////////////
/**An Information Element that contains an unsigned short (16 bits) of data*/
class IAX2IeUShort : public IAX2Ie
{
  PCLASSINFO(IAX2IeUShort, IAX2Ie);
  /**@name construction/destruction*/
  //@{
  
  /**Constructor - read data from source array. 
     
  Contents are valid if source array is valid. */
  IAX2IeUShort(BYTE length, BYTE *srcData);     
  
  /**Construct to an initialised value */
  IAX2IeUShort(unsigned short newValue) : IAX2Ie() { SetData(newValue); }
  
  /**Constructor to an invalid and empty result*/
  IAX2IeUShort() : IAX2Ie() {}
  //@}
  
  /**@name Worker methods*/
  //@{
  /**return the number of bytes to hold this data element */
  virtual BYTE GetLengthOfData() { return sizeof(dataValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(unsigned short newData) { dataValue = newData; validData = PTrue; }
  
  /**Report the value of the stored data for this class */
  unsigned short ReadData() { return dataValue; }         
  //@}
 protected:
  /** Take the data value for this particular IAX2Ie and copy into the memory region.*/
  virtual void WriteBinary(BYTE *data);
  
  /**The actual data stored in a IAX2IeUShort class */
  unsigned short dataValue;
};
////////////////////////////////////////////////////////////////////////////////
/**An Information Element that contains an unsigned int (32 bits) of data*/
class IAX2IeUInt : public IAX2Ie
{
  PCLASSINFO(IAX2IeUInt, IAX2Ie);
  /**@name construction/destruction*/
  //@{
  
  /**Constructor - read data from source array. 
     
  Contents are valid if source array is valid. */
  IAX2IeUInt(BYTE length, BYTE *srcData);     
  
  /**Constructor to an invalid and empty result*/
  IAX2IeUInt() : IAX2Ie() {}
  
  /**Constructor to an initialised value, (Typically used prior to transmission)*/
  IAX2IeUInt(unsigned int newValue) { SetData(newValue); }
  //@}
  
  /**@name Worker methods*/
  //@{
  /**return the number of bytes to hold this data element */
  virtual BYTE GetLengthOfData() { return sizeof(dataValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(unsigned int &newData) { dataValue = newData; validData = PTrue; }
  
  /**Report the value of the stored data for this class */
  unsigned int ReadData() { return dataValue; }          
  //@}
  
 protected:
  /** Take the data value for this particular IAX2Ie and copy into the memory region.*/
  virtual void WriteBinary(BYTE *data);
  
  /**The actual data stored in a IAX2IeUInt class */
  unsigned int dataValue;
};

////////////////////////////////////////////////////////////////////////////////
/**An Information Element that contains an array of characters. */
class IAX2IeString : public IAX2Ie
{
  PCLASSINFO(IAX2IeString, IAX2Ie);
  /**@name construction/destruction*/
  //@{
  
 public:
  /**Constructor - read data from source array. 
     
  Contents are valid if source array is valid. */
  IAX2IeString(BYTE length, BYTE *srcData);     
  
  /**Construct to an initialised value */
  IAX2IeString(const PString & newValue) : IAX2Ie() { SetData(newValue); }
  
  /**Construct to an initialised value */
  IAX2IeString(const char * newValue) : IAX2Ie() { SetData(newValue); }
  
  /**Constructor to an invalid and empty result*/
  IAX2IeString() : IAX2Ie() {}
  //@}
  
  /**@name Worker methods*/
  //@{  
  /**return the number of bytes to hold this data element */
  virtual BYTE GetLengthOfData();
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(const PString & newData);
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(const char * newData);
  
  /**Report the value of the stored data for this class */
  PString ReadData() { return dataValue; }          
  //@}
  
 protected:
  /** Take the data value for this particular IAX2Ie and copy into the memory region.*/
  virtual void WriteBinary(BYTE *data);
  
  /**The actual data stored in a IAX2IeString class */
  PString dataValue;
};
////////////////////////////////////////////////////////////////////////////////
/**An Information Element that contains the date and time, from a 32 bit long representation*/
class IAX2IeDateAndTime : public IAX2Ie
{
  PCLASSINFO(IAX2IeDateAndTime, IAX2Ie);
  /**@name construction/destruction*/
  //@{
  
 public:
  /**Constructor - read data from source array. 
     
  Contents are valid if source array is valid. */
  IAX2IeDateAndTime(BYTE length, BYTE *srcData);     
  
  /**Construct to an initialized value */
  IAX2IeDateAndTime(const PTime & newValue) : IAX2Ie() { SetData(newValue); }
  
  /**Constructor to an invalid and empty result*/
  IAX2IeDateAndTime() : IAX2Ie() {}
  //@}
  
  /**@name Worker methods*/
  //@{
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**return the number of bytes to hold this data element */
  virtual BYTE GetLengthOfData() { return 4; }
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(const PTime & newData) { dataValue = newData; validData = PTrue; }
  
  /**Report the value of the stored data for this class */
  PTime ReadData() { return dataValue; }
  //@}
 protected:
  /** Take the data value for this particular IAX2Ie and copy into the memory region.*/
  virtual void WriteBinary(BYTE *data);
  
  /**The actual data stored in a IAX2IeDateAndTime class */
  PTime dataValue;
};
////////////////////////////////////////////////////////////////////////////////
/**An Information Element that contains an array of BYTES (with possible nulls in middle) */
class IAX2IeBlockOfData : public IAX2Ie
{
  PCLASSINFO(IAX2IeBlockOfData, IAX2Ie);
  /**@name construction/destruction*/
  //@{
  
 public:
  /**Constructor - read data from source array. 
     
  Contents are valid if source array is valid. */
  IAX2IeBlockOfData(BYTE length, BYTE *srcData);     
  
  /**Construct to an initialized value */
  IAX2IeBlockOfData(const PBYTEArray & newData) : IAX2Ie() { SetData(newData); }
  
  /**Constructor to an invalid and empty result*/
  IAX2IeBlockOfData() : IAX2Ie() {}
  //@}
  
  /**@name Worker methods*/
  //@{
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**return the number of bytes to hold this data element */
  virtual BYTE GetLengthOfData() { return (BYTE)dataValue.GetSize(); }
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(const PBYTEArray & newData) { dataValue = newData; validData = PTrue; }
  
  /**Report the value of the stored data for this class */
  PBYTEArray ReadData() { return dataValue; }
  
  //@}
 protected:
  /** Take the data value for this particular IAX2Ie and copy into the memory region.*/
  virtual void WriteBinary(BYTE *data);
  
  /**The actual data stored in a IAX2IeBlockOfData class */
  PBYTEArray dataValue;
};
////////////////////////////////////////////////////////////////////////////////
/**An Information Element that contains an Ip address and port */
class IAX2IeSockaddrIn : public IAX2Ie
{
  PCLASSINFO(IAX2IeSockaddrIn, IAX2Ie);
  /**@name construction/destruction*/
  //@{
  
 public:
  /**Constructor - read data from source array. 
     
  Contents are valid if source array is valid. */
  IAX2IeSockaddrIn(BYTE length, BYTE *srcData);     
  
  /**Construct to an initialized value */
  IAX2IeSockaddrIn(const PIPSocket::Address & addr, PINDEX port) : IAX2Ie() { SetData(addr, port); }
  
  /**Constructor to an invalid and empty result*/
  IAX2IeSockaddrIn() : IAX2Ie() {}
  
  /**Destructor*/
  ~IAX2IeSockaddrIn() { } ;
  //@}
  
  /**@name Worker methods*/
  //@{
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**return the number of bytes to hold this data element */
  virtual BYTE GetLengthOfData() { return sizeof(struct sockaddr_in); }
  
  
  /**Take the supplied data and copy contents into this IE */
  void SetData(const PIPSocket::Address & newAddr, PINDEX newPort) 
    { dataValue = newAddr; portNumber = newPort; validData = PTrue; }
  
  /**Report the value of the stored data for this class */
  PIPSocket::Address ReadData() { return dataValue; }
  
  //@}
 protected:
  /** Take the data value for this particular IAX2Ie and copy into the memory region.*/
  virtual void WriteBinary(BYTE *data);
  
  /**The actual ip address data stored in a IAX2IeSockaddrIn class */
  PIPSocket::Address dataValue;
  
  /**The actual port number data stored in a IAX2IeSockaddrIn class */  
  PINDEX               portNumber;
};

////////////////////////////////////////////////////////////////////////////////
/**An Information Element that contains the number/extension being called.*/
class IAX2IeCalledNumber : public IAX2IeString
{
  PCLASSINFO(IAX2IeCalledNumber, IAX2IeString);
 public:
  /**Constructor from data read from the network.
     
  Contents are undefined if network data is bogus.*/
  IAX2IeCalledNumber(BYTE length, BYTE *srcData) : IAX2IeString(length, srcData) {};
  
  /**Initialise to the supplied string value */
  IAX2IeCalledNumber(const PString & newValue) { SetData(newValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_calledNumber; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.calledNumber = dataValue; }
 protected:
};
////////////////////////////////////////////////////////////////////////////////
/**An Information Element that contains the Calling number (number trying to make call?)*/
class IAX2IeCallingNumber : public IAX2IeString
{
  PCLASSINFO(IAX2IeCallingNumber, IAX2IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus. */
  IAX2IeCallingNumber(BYTE length, BYTE *srcData) : IAX2IeString(length, srcData) { };
  
  /**Initialise to the supplied string value */
  IAX2IeCallingNumber(const PString & newValue)  { SetData(newValue); } 
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_callingNumber; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.callingNumber = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the calling number ANI (for billing)*/
class IAX2IeCallingAni : public IAX2IeString
{
  PCLASSINFO(IAX2IeCallingAni, IAX2IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus. */
  IAX2IeCallingAni(BYTE length, BYTE *srcData) : IAX2IeString(length, srcData) { };
  
  /**Initialise to the supplied string value */
  IAX2IeCallingAni(const PString & newValue) { SetData(newValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_callingAni; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.callingAni = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains name of the the person making the call*/
class IAX2IeCallingName : public IAX2IeString
{
  PCLASSINFO(IAX2IeCallingName, IAX2IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeCallingName(BYTE length, BYTE *srcData) : IAX2IeString(length, srcData) { };
  
  /**Initialise to the supplied string value */
  IAX2IeCallingName(const PString & newValue) { SetData(newValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_callingName; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.callingName = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the context for this number*/
class IAX2IeCalledContext : public IAX2IeString
{
  PCLASSINFO(IAX2IeCalledContext, IAX2IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeCalledContext(BYTE length, BYTE *srcData) : IAX2IeString(length, srcData) { };
  
  /**Initialise to the supplied string value */
  IAX2IeCalledContext(const PString & newValue) { SetData(newValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_calledContext; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.calledContext = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the userName (peer or user) for authentication*/
class IAX2IeUserName : public IAX2IeString
{
  PCLASSINFO(IAX2IeUserName, IAX2IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeUserName(BYTE length, BYTE *srcData) : IAX2IeString(length, srcData) { };
  
  /**Initialise to the supplied string value */
  IAX2IeUserName(const PString & newValue)  { SetData(newValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_userName; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.userName = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the password (for authentication)*/
class IAX2IePassword : public IAX2IeString
{
  PCLASSINFO(IAX2IePassword, IAX2IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IePassword(BYTE length, BYTE *srcData) : IAX2IeString(length, srcData) { };
  
  /**Initialise to the supplied string value */
  IAX2IePassword(const PString & newValue) { SetData(newValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_password; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.password = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the actual codecs available*/
class IAX2IeCapability : public IAX2IeUInt
{
  PCLASSINFO(IAX2IeCapability, IAX2IeUInt);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeCapability(BYTE length, BYTE *srcData) : IAX2IeUInt(length, srcData) { };
  
  /**Construct with a predefined value (Typically used prior to transmission)*/
  IAX2IeCapability(unsigned int newValue) : IAX2IeUInt(newValue) { }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_capability; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.capability = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the desired codec format*/
class IAX2IeFormat : public IAX2IeUInt
{
  PCLASSINFO(IAX2IeFormat, IAX2IeUInt);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeFormat(BYTE length, BYTE *srcData) : IAX2IeUInt(length, srcData) { };
  
  /**Construct with a predefined value (Typically used prior to transmission)*/
  IAX2IeFormat(unsigned int newValue) : IAX2IeUInt(newValue) { }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_format; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.format = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the desired language*/
class IAX2IeLanguage : public IAX2IeString
{
  PCLASSINFO(IAX2IeLanguage, IAX2IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeLanguage(BYTE length, BYTE *srcData) : IAX2IeString(length, srcData) { };
  
  /**Initialise to the supplied string value */
  IAX2IeLanguage(const PString & newValue) { SetData(newValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_language; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.language = dataValue; }     
 protected:
};

//////////////////////////////////////////////////////////////////////
/**An Information Element that contains the protocol version*/
class IAX2IeVersion : public IAX2IeShort
{
  PCLASSINFO(IAX2IeVersion, IAX2IeShort);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeVersion(BYTE length, BYTE *srcData) : IAX2IeShort(length, srcData) { };
  
  /**Construct to the one possible value - version 2 */
  IAX2IeVersion() { dataValue = 2; validData = PTrue; }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_version; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.version = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the CPE ADSI capability*/
class IAX2IeAdsicpe : public IAX2IeShort
{
  PCLASSINFO(IAX2IeAdsicpe, IAX2IeShort);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeAdsicpe(BYTE length, BYTE *srcData) : IAX2IeShort(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_adsicpe; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.adsicpe = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains  the originally dialed DNID*/
class IAX2IeDnid : public IAX2IeString
{
  PCLASSINFO(IAX2IeDnid, IAX2IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeDnid(BYTE length, BYTE *srcData) : IAX2IeString(length, srcData) { };
  
  /**Initialise to the supplied string value */
  IAX2IeDnid(const PString & newValue)  { SetData(newValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_dnid; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.dnid = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the authentication methods */
class IAX2IeAuthMethods : public IAX2IeShort
{
  PCLASSINFO(IAX2IeAuthMethods, IAX2IeShort);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeAuthMethods(BYTE length, BYTE *srcData) : IAX2IeShort(length, srcData) { };
  
  /** Initialise to the supplied short value, which is usually done prior to transmission*/
  IAX2IeAuthMethods(short newValue) { SetData(newValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_authMethods; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.authMethods = dataValue; }     
  
  /**Return true if the supplied value has the RSA key on*/
  static PBoolean IsRsaAuthentication(short testValue) { return InternalIsRsa(testValue); }
  
  /**Return true if the supplied value has the MD5 key on*/
  static PBoolean IsMd5Authentication(short testValue) { return InternalIsMd5(testValue); }
  
  /**Return true if the supplied value has the plain text  key on*/
  static PBoolean IsPlainTextAuthentication(short testValue) { return InternalIsPlainText(testValue); }     
  
  /**Return true if the internal value has the RSA key on*/
  PBoolean IsRsaAuthentication() { if (IsValid()) return InternalIsRsa(dataValue); else return PFalse; }
  
  /**Return true if the internal value has the MD5 key on*/
  PBoolean IsMd5Authentication() { if (IsValid()) return InternalIsMd5(dataValue); else return PFalse; }
  
  /**Return true if the internal value has the plain text  key on*/
  PBoolean IsPlainTextAuthentication() { if (IsValid()) return InternalIsPlainText(dataValue); else return PFalse; }
  
 protected:
  
  /**Return true if the supplied value has the RSA key on*/
  static PBoolean InternalIsRsa(short testValue) { return testValue  & 0x04; }
  
  /**Return true if the supplied value has the MD5 key on*/
  static PBoolean InternalIsMd5(short testValue) { return testValue  & 0x02; }
  
  /**Return true if the supplied value has the plain text  key on*/
  static PBoolean InternalIsPlainText(short testValue) { return testValue  & 0x01; }
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the challenge data for MD5/RSA*/
class IAX2IeChallenge : public IAX2IeString
{
  PCLASSINFO(IAX2IeChallenge, IAX2IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeChallenge(BYTE length, BYTE *srcData) : IAX2IeString(length, srcData) { };
  
  /**Initialise to the supplied string value */
  IAX2IeChallenge(const PString & newValue) { SetData(newValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_challenge; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.challenge = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the MD5 chaallenge result*/
class IAX2IeMd5Result : public IAX2IeString
{
  PCLASSINFO(IAX2IeMd5Result, IAX2IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeMd5Result(BYTE length, BYTE *srcData) : IAX2IeString(length, srcData) { };
  
  /**Initialise to the supplied string value */
  IAX2IeMd5Result(const PString & newValue) { SetData(newValue); }
  
  /**Take the challenge and password, calculate the result, and store */
  IAX2IeMd5Result(const PString & challenge, const PString & password);
  
  /**Take the supplied Iax2Encrption arguement, calculate the result, and store */
  IAX2IeMd5Result(IAX2Encryption & encryption);
  
  /**Initialize the internal structurees */
  void InitializeChallengePassword(const PString & newChallenge, const PString & newPassword);

  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_md5Result; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.md5Result = dataValue; }     

  /**GetAccess to the stomach, which is the concatanation of the various
     components used to make a key */
  PBYTEArray & GetDataBlock() { return dataBlock; }

 protected:

  /**The contents of the stomach in a binary block */
  PBYTEArray dataBlock;
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the RSA challenge result*/
class IAX2IeRsaResult : public IAX2IeString
{
  PCLASSINFO(IAX2IeRsaResult, IAX2IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeRsaResult(BYTE length, BYTE *srcData) : IAX2IeString(length, srcData) { };
  
  /**Initialise to the supplied string value */
  IAX2IeRsaResult(const PString & newValue) { SetData(newValue); }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_rsaResult; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.rsaResult = dataValue; }
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the apparent daddress of peer*/
class IAX2IeApparentAddr : public IAX2IeSockaddrIn
{
  PCLASSINFO(IAX2IeApparentAddr, IAX2IeSockaddrIn);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeApparentAddr(BYTE length, BYTE *srcData) : IAX2IeSockaddrIn(length, srcData) { };
  
  /**Desttructor, which does nothing */
  ~IAX2IeApparentAddr() { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_apparentAddr; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.apparentAddr = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the time when to refresh registration*/
class IAX2IeRefresh : public IAX2IeShort
{
  PCLASSINFO(IAX2IeRefresh, IAX2IeShort);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeRefresh(BYTE length, BYTE *srcData) : IAX2IeShort(length, srcData) { };
  
  /** Constructor for an outgoing refresh information element */
  IAX2IeRefresh(short refreshTime) : IAX2IeShort(refreshTime) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_refresh; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.refresh = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the dial plan status*/
class IAX2IeDpStatus : public IAX2IeShort
{
  PCLASSINFO(IAX2IeDpStatus, IAX2IeShort);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeDpStatus(BYTE length, BYTE *srcData) : IAX2IeShort(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_dpStatus; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.dpStatus = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains  the call number of peer*/
class IAX2IeCallNo : public IAX2IeShort
{
  PCLASSINFO(IAX2IeCallNo, IAX2IeShort);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeCallNo(BYTE length, BYTE *srcData) : IAX2IeShort(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_callNo; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.callNo = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the cause*/
class IAX2IeCause : public IAX2IeString
{
  PCLASSINFO(IAX2IeCause, IAX2IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeCause(BYTE length, BYTE *srcData) : IAX2IeString(length, srcData) { };
  
  /**Construct with a predefined value (Typically used prior to transmission)*/
  IAX2IeCause(const PString & newValue) : IAX2IeString(newValue) { }
  
  /**Construct with a predefined value (Typically used prior to transmission)*/
  IAX2IeCause(const char *newValue) : IAX2IeString(newValue) { }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_cause; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.cause = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains  an unknown IAX command*/
class IAX2IeIaxUnknown : public IAX2IeByte
{
  PCLASSINFO(IAX2IeIaxUnknown, IAX2IeByte);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeIaxUnknown(BYTE length, BYTE *srcData) : IAX2IeByte(length, srcData) { };
  
  /**Constructor when sending an unknown message*/
  IAX2IeIaxUnknown(BYTE newValue) : IAX2IeByte(newValue) { }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_iaxUnknown; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.iaxUnknown = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains how many messages are waiting*/
class IAX2IeMsgCount : public IAX2IeShort
{
  PCLASSINFO(IAX2IeMsgCount, IAX2IeShort);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeMsgCount(BYTE length, BYTE *srcData) : IAX2IeShort(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_msgCount; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.msgCount = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains a request for auto-answering*/
class IAX2IeAutoAnswer : public IAX2IeNone
{
  PCLASSINFO(IAX2IeAutoAnswer, IAX2IeNone);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeAutoAnswer(BYTE length, BYTE *srcData) : IAX2IeNone(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_autoAnswer; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.autoAnswer = PTrue;; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains a request for music on hold with Quelch*/
class IAX2IeMusicOnHold : public IAX2IeNone
{
  PCLASSINFO(IAX2IeMusicOnHold, IAX2IeNone);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeMusicOnHold(BYTE length, BYTE *srcData) : IAX2IeNone(length, srcData) { };
  
  /**Construct with a predefined value (Typically used prior to transmission)*/
  IAX2IeMusicOnHold() : IAX2IeNone() { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_musicOnHold; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.musicOnHold = PTrue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains a transfer request identifier*/
class IAX2IeTransferId : public IAX2IeInt
{
  PCLASSINFO(IAX2IeTransferId, IAX2IeInt);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeTransferId(BYTE length, BYTE *srcData) : IAX2IeInt(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_transferId; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.transferId = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the referring DNIs*/
class IAX2IeRdnis : public IAX2IeString
{
  PCLASSINFO(IAX2IeRdnis, IAX2IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeRdnis(BYTE length, BYTE *srcData) : IAX2IeString(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_rdnis; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.rdnis = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains Provisioning - a great big data block */
class IAX2IeProvisioning : public IAX2IeBlockOfData
{
  PCLASSINFO(IAX2IeProvisioning, IAX2IeBlockOfData);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeProvisioning(BYTE length, BYTE *srcData) : IAX2IeBlockOfData   (length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_provisioning; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &/*res*/) {  }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains aes provisioning info*/
class IAX2IeAesProvisioning : public IAX2IeNone
{
  PCLASSINFO(IAX2IeAesProvisioning, IAX2IeNone);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeAesProvisioning(BYTE length, BYTE *srcData) : IAX2IeNone(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_aesProvisioning; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &/*res*/) {  }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the date and time (in 32 bits)*/
class IAX2IeDateTime : public IAX2IeDateAndTime
{
  PCLASSINFO(IAX2IeDateTime, IAX2IeDateAndTime);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeDateTime(BYTE length, BYTE *srcData) : IAX2IeDateAndTime(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_dateTime; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.dateTime = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the device type*/
class IAX2IeDeviceType : public IAX2IeString
{
  PCLASSINFO(IAX2IeDeviceType, IAX2IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeDeviceType(BYTE length, BYTE *srcData) : IAX2IeString(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_deviceType; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.deviceType = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the service identifier*/
class IAX2IeServiceIdent : public IAX2IeString
{
  PCLASSINFO(IAX2IeServiceIdent, IAX2IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeServiceIdent(BYTE length, BYTE *srcData) : IAX2IeString(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_serviceIdent; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.serviceIdent = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the firmware version*/
class IAX2IeFirmwareVer : public IAX2IeShort
{
  PCLASSINFO(IAX2IeFirmwareVer, IAX2IeShort);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeFirmwareVer(BYTE length, BYTE *srcData) : IAX2IeShort(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_firmwareVer; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.firmwareVer = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains firmware block description*/
class IAX2IeFwBlockDesc : public IAX2IeUInt
{
  PCLASSINFO(IAX2IeFwBlockDesc, IAX2IeUInt);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeFwBlockDesc(BYTE length, BYTE *srcData) : IAX2IeUInt(length, srcData) { };
  
  /**Construct with a predefined value (Typically used prior to transmission)*/
  IAX2IeFwBlockDesc(unsigned int newValue) : IAX2IeUInt(newValue) { }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_fwBlockDesc; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.fwBlockDesc = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the block of firmware data */
class IAX2IeFwBlockData : public IAX2IeBlockOfData
{
  PCLASSINFO(IAX2IeFwBlockData, IAX2IeBlockOfData);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeFwBlockData(BYTE length, BYTE *srcData) : IAX2IeBlockOfData(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_fwBlockData; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.fwBlockData = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the Provisioning version*/
class IAX2IeProvVer : public IAX2IeUInt
{
  PCLASSINFO(IAX2IeProvVer, IAX2IeUInt);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeProvVer(BYTE length, BYTE *srcData) : IAX2IeUInt(length, srcData) { };
  
  /**Construct with a predefined value (Typically used prior to transmission)*/
  IAX2IeProvVer(unsigned int newValue) : IAX2IeUInt(newValue) { }
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_provVer; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.provVer = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the calling presentation*/
class IAX2IeCallingPres : public IAX2IeByte
{
  PCLASSINFO(IAX2IeCallingPres, IAX2IeByte);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeCallingPres(BYTE length, BYTE *srcData) : IAX2IeByte(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_callingPres; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.callingPres = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the calling type of number*/
class IAX2IeCallingTon : public IAX2IeByte
{
  PCLASSINFO(IAX2IeCallingTon, IAX2IeByte);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeCallingTon(BYTE length, BYTE *srcData) : IAX2IeByte(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_callingTon; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.callingTon = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the calling transit network select */
class IAX2IeCallingTns : public IAX2IeUShort
{
  PCLASSINFO(IAX2IeCallingTns, IAX2IeUShort);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeCallingTns(BYTE length, BYTE *srcData) : IAX2IeUShort(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_callingTns; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.callingTns = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the supported sampling rates*/
class IAX2IeSamplingRate : public IAX2IeUShort
{
  PCLASSINFO(IAX2IeSamplingRate, IAX2IeUShort);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeSamplingRate(BYTE length, BYTE *srcData) : IAX2IeUShort(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_samplingRate; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.samplingRate = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the encryption format*/
class IAX2IeEncryption : public IAX2IeUShort
{
  PCLASSINFO(IAX2IeEncryption, IAX2IeUShort);
 public:
  /**Specify the different encryption methods */
  enum IAX2IeEncryptionMethod {
    encryptAes128 = 1    /*!< Specify to use AES 128 bit encryption */
  };

  /**Constructor to specify a particular encryption method */
  IAX2IeEncryption(IAX2IeEncryptionMethod method = encryptAes128);

  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeEncryption(BYTE length, BYTE *srcData) : IAX2IeUShort(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_encryption; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.encryptionMethods = dataValue; }
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the encryption key (raw)*/
class IAX2IeEncKey : public IAX2IeString
{
  PCLASSINFO(IAX2IeEncKey, IAX2IeString);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeEncKey(BYTE length, BYTE *srcData) : IAX2IeString(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_encKey; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.encKey = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains data for codec negotiation. */
class IAX2IeCodecPrefs : public IAX2IeByte
{
  PCLASSINFO(IAX2IeCodecPrefs, IAX2IeByte);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeCodecPrefs(BYTE length, BYTE *srcData) : IAX2IeByte(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_codecPrefs; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.codecPrefs = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the received jitter */
class IAX2IeReceivedJitter : public IAX2IeUInt
{
  PCLASSINFO(IAX2IeReceivedJitter, IAX2IeUInt);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeReceivedJitter(BYTE length, BYTE *srcData) : IAX2IeUInt(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_recJitter; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.receivedJitter = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the received loss */
class IAX2IeReceivedLoss : public IAX2IeUInt
{
  PCLASSINFO(IAX2IeReceivedLoss, IAX2IeUInt);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeReceivedLoss(BYTE length, BYTE *srcData) : IAX2IeUInt(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_recLoss; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.receivedLoss = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the received frames */
class IAX2IeReceivedFrames : public IAX2IeUInt
{
  PCLASSINFO(IAX2IeReceivedFrames, IAX2IeUInt);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeReceivedFrames(BYTE length, BYTE *srcData) : IAX2IeUInt(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_recPackets; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.receivedPackets = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the received delay */
class IAX2IeReceivedDelay : public IAX2IeUShort
{
  PCLASSINFO(IAX2IeReceivedDelay, IAX2IeUShort);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeReceivedDelay(BYTE length, BYTE *srcData) : IAX2IeUShort(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_recDelay; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.receivedDelay = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the number of dropped frames */
class IAX2IeDroppedFrames : public IAX2IeUInt
{
  PCLASSINFO(IAX2IeDroppedFrames, IAX2IeUInt);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeDroppedFrames(BYTE length, BYTE *srcData) : IAX2IeUInt(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_recDropped; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.receivedDropped = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////
/**An Information Element that contains the number of frames received out of order */
class IAX2IeReceivedOoo : public IAX2IeUInt
{
  PCLASSINFO(IAX2IeReceivedOoo, IAX2IeUInt);
 public:
  /** Constructor from data read from the network.
      
  Contents are undefined if the network data is bogus/invalid */
  IAX2IeReceivedOoo(BYTE length, BYTE *srcData) : IAX2IeUInt(length, srcData) { };
  
  /**print this class (nicely) to the designated stream*/
  void PrintOn(ostream & str) const;
  
  /**Get the key value for this particular Information Element class */
  virtual BYTE GetKeyValue() const  { return ie_recOoo; }
  
  /** Take the data from this IAX2Ie, and copy it into the IAX2IeData structure.
      This is done on processing an incoming frame which contains IAX2Ie in the data section. */
  virtual void StoreDataIn(IAX2IeData &res) { res.receivedOoo = dataValue; }     
 protected:
};

///////////////////////////////////////////////////////////////////////


PDECLARE_LIST (IAX2IeList, IAX2Ie *)
#ifdef DOC_PLUS_PLUS 
/**An array of IE* elements are stored in this list */
class IAX2IeList : public IAX2Ie *
{
#endif
 public:
  /**Destructor, so all eleents are destroyed on destruction */
  ~IAX2IeList();
  
  /**Access method, get pointer to information element at index. 
     Returns NULL if index is out of bounds.
     This will remove the specified IAX2Ie from the list. */
  IAX2Ie *RemoveIeAt(PINDEX i);
  
  /**Access method, get pointer to last information element in the list.
     Returns NULL if index is out of bounds.
     This will remove the specified IAX2Ie from the list. */
  IAX2Ie *RemoveLastIe();
  
  /**Initialisation - Objects are not automatically deleted on removal */
  void Initialise() {  DisallowDeleteObjects(); }
  
  /**Delete item at a particular index */
  void DeleteAt(PINDEX idex);
  
  /**Test to see if list is empty - returns PTrue if no elements stored in this list */
  PBoolean Empty() const { return GetSize() == 0; }
  
  /**Test to see if list is empty - returns PTrue if no elements stored in this list */
  PBoolean IsEmpty() const { return GetSize() == 0; }
  
  /**Add a new IAX2Ie to the list */
  void AppendIe(IAX2Ie *newMember) { Append(newMember);}
  
  /**Get the number of bytes to store all these IAX2Ie's in a network packet */
  int GetBinaryDataSize() const;
  
  /**Get a pointer to the IAX2Ie which is stored at index i*/
  IAX2Ie * GetIeAt(int i) const;
  
 protected:
  
};


#endif // OPAL_IAX2

#endif // OPAL_IAX2_IES_H

/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 4 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-file-style:linux
 * c-basic-offset:2
 * End:
 */
