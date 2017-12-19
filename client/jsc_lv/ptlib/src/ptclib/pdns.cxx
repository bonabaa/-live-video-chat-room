/*
 * pdns.cxx
 *
 * Portable Windows Library
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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
 * Contributor(s): ______________________________________.
 *
 * Copyright 2003 Equivalence Pty. Ltd.
 *
 * $Revision: 22297 $
 * $Author: rjongbloed $
 * $Date: 2009-03-26 00:37:59 +0000 (Thu, 26 Mar 2009) $
 */

#ifdef __GNUC__
#pragma implementation "pdns.h"
#endif

#include <ptlib.h>
#include <ptclib/pdns.h>
#include <ptclib/url.h>
#include <ptlib/ipsock.h>

#define new PNEW


#if P_DNS


/////////////////////////////////////////////////

#ifdef P_HAS_RESOLVER

static PBoolean GetDN(const BYTE * reply, const BYTE * replyEnd, BYTE * & cp, char * buff)
{
  int len = dn_expand(reply, replyEnd, cp, buff, MAXDNAME);
  if (len < 0)
    return PFalse;
  cp += len;
  return PTrue;
}

static PBoolean ProcessDNSRecords(
        const BYTE * reply,
        const BYTE * replyEnd,
              BYTE * cp,
            PINDEX anCount,
            PINDEX nsCount,
            PINDEX arCount,
     PDNS_RECORD * results)
{
  PDNS_RECORD lastRecord = NULL;

  PINDEX rrCount = anCount + nsCount + arCount;
  nsCount += anCount;
  arCount += nsCount;

  PINDEX i;
  for (i = 0; i < rrCount; i++) {

    int section;
    if (i < anCount)
      section = DnsSectionAnswer;
    else if (i < nsCount)
      section = DnsSectionAuthority;
    else // if (i < arCount)
      section = DnsSectionAdditional;

    // get the name
    char pName[MAXDNAME];
    if (!GetDN(reply, replyEnd, cp, pName)) 
      return PFalse;

    // get other common parts of the record
    WORD  type;
    WORD  dnsClass;
    DWORD ttl;
    WORD  dlen;

    GETSHORT(type,     cp);
    GETSHORT(dnsClass, cp);
    GETLONG (ttl,      cp);
    GETSHORT(dlen,     cp);

    BYTE * data = cp;
    cp += dlen;

    PDNS_RECORD newRecord  = NULL;

    switch (type) {
      default:
        newRecord = (PDNS_RECORD)malloc(sizeof(DnsRecord) + sizeof(DWORD) + dlen);
        newRecord->Data.Null.dwByteCount = dlen;
        memcpy(&newRecord->Data, data, dlen);
        break;

      case T_SRV:
        newRecord = (PDNS_RECORD)malloc(sizeof(DnsRecord)); 
        memset(newRecord, 0, sizeof(DnsRecord));
        GETSHORT(newRecord->Data.SRV.wPriority, data);
        GETSHORT(newRecord->Data.SRV.wWeight, data);
        GETSHORT(newRecord->Data.SRV.wPort, data);
        if (!GetDN(reply, replyEnd, data, newRecord->Data.SRV.pNameTarget)) {
          free(newRecord);
          return PFalse;
        }
        break;

      case T_MX:
        newRecord = (PDNS_RECORD)malloc(sizeof(DnsRecord)); 
        memset(newRecord, 0, sizeof(DnsRecord));
        GETSHORT(newRecord->Data.MX.wPreference,  data);
        if (!GetDN(reply, replyEnd, data, newRecord->Data.MX.pNameExchange)) {
          free(newRecord);
          return PFalse;
        }
        break;

      case T_A:
        newRecord = (PDNS_RECORD)malloc(sizeof(DnsRecord)); 
        memset(newRecord, 0, sizeof(DnsRecord));
        GETLONG(newRecord->Data.A.IpAddress, data);
        break;

      case T_NS:
        newRecord = (PDNS_RECORD)malloc(sizeof(DnsRecord)); 
        memset(newRecord, 0, sizeof(DnsRecord));
        if (!GetDN(reply, replyEnd, data, newRecord->Data.NS.pNameHost)) {
          delete newRecord;
          return PFalse;
        }
        break;
    }

    // initialise the new record
    if (newRecord != NULL) {
      newRecord->wType = type;
      newRecord->Flags.S.Section = section;
      newRecord->pNext = NULL;
      strcpy(newRecord->pName, pName);

      if (*results == NULL)
        *results = newRecord;

      if (lastRecord != NULL)
        lastRecord->pNext = newRecord;

      lastRecord = newRecord;
      newRecord = NULL;
    }
  }

  return PTrue;
}

void DnsRecordListFree(PDNS_RECORD rec, int /* FreeType */)
{
  while (rec != NULL) {
    PDNS_RECORD next = rec->pNext;
    free(rec);
    rec = next;
  }
}

#if ! P_HAS_RES_NINIT

static PMutex & GetDNSMutex()
{
  static PMutex mutex;
  return mutex;
}

#endif

DNS_STATUS DnsQuery_A(const char * service,
                              WORD requestType,
                             DWORD options,
                            PIP4_ARRAY,
                     PDNS_RECORD * results,
                            void *)
{
#if defined(P_NETBSD)
  struct __res_state myRes;
#endif
  if (results == NULL)
    return -1;

  *results = NULL;

#if P_HAS_RES_NINIT
#if defined(P_NETBSD)
  res_ninit(&myRes);
#else
  res_ninit(&_res);
#endif
#else
  res_init();
  GetDNSMutex().Wait();
#endif

  union {
    HEADER hdr;
    BYTE buf[PACKETSZ];
  } reply;

#if P_HAS_RES_NINIT
  int replyLen = res_nsearch(
#if defined(P_NETBSD)
      &myRes,
#else
      &_res,
#endif
      service, C_IN, requestType, (BYTE *)&reply, sizeof(reply));
#else
  int replyLen = res_search(service, C_IN, requestType, (BYTE *)&reply, sizeof(reply));
  GetDNSMutex().Signal();
#endif

  if (replyLen < 1)
    return -1;

  BYTE * replyStart = reply.buf;
  BYTE * replyEnd   = reply.buf + replyLen;
  BYTE * cp         = reply.buf + sizeof(HEADER);

  // ignore questions in response
  uint16_t i;
  for (i = 0; i < ntohs(reply.hdr.qdcount); i++) {
    char qName[MAXDNAME];
    if (!GetDN(replyStart, replyEnd, cp, qName))
      return -1;
    cp += QFIXEDSZ;
  }

  if (!ProcessDNSRecords(
       replyStart,
       replyEnd,
       cp,
       ntohs(reply.hdr.ancount),
       ntohs(reply.hdr.nscount),
       ntohs(reply.hdr.arcount),
       results)) {
    DnsRecordListFree(*results, 0);
    return -1;
  }

  return 0;
}


#endif // P_HAS_RESOLVER

PObject::Comparison PDNS::SRVRecord::Compare(const PObject & obj) const
{
  const SRVRecord * other = dynamic_cast<const SRVRecord *>(&obj);

  if (other == NULL)
    return LessThan;

  if (priority < other->priority)
    return LessThan;
  else if (priority > other->priority)
    return GreaterThan;

  if (weight < other->weight)
    return LessThan;
  else if (weight > other->weight)
    return GreaterThan;

  return EqualTo;
}

void PDNS::SRVRecord::PrintOn(ostream & strm) const
{
  strm << "host=" << hostName << ":" << port << "(" << hostAddress << "), "
       << "priority=" << priority << ", "
       << "weight=" << weight;
}

/////////////////////////////////////////////////

PDNS::SRVRecord * PDNS::SRVRecordList::HandleDNSRecord(PDNS_RECORD dnsRecord, PDNS_RECORD results)
{
  PDNS::SRVRecord * record = NULL;

  if (
      (dnsRecord->Flags.S.Section == DnsSectionAnswer) && 
      (dnsRecord->wType == DNS_TYPE_SRV) &&
#ifndef _WIN32_WCE
      (strlen(dnsRecord->Data.SRV.pNameTarget) > 0) &&
      (strcmp(dnsRecord->Data.SRV.pNameTarget, ".") != 0)
#else
      (wcslen(dnsRecord->Data.SRV.pNameTarget) > 0) &&
      (wcscmp(dnsRecord->Data.SRV.pNameTarget, L".") != 0)
#endif
      ) {
    record = new SRVRecord();
    record->hostName = PString(dnsRecord->Data.SRV.pNameTarget);
    record->port     = dnsRecord->Data.SRV.wPort;
    record->priority = dnsRecord->Data.SRV.wPriority;
    record->weight   = dnsRecord->Data.SRV.wWeight;

    // see if any A records match this hostname
    PDNS_RECORD aRecord = results;
    while (aRecord != NULL) {
      if ((dnsRecord->Flags.S.Section == DnsSectionAdditional) && (dnsRecord->wType == DNS_TYPE_A)) {
        record->hostAddress = PIPSocket::Address(dnsRecord->Data.A.IpAddress);
        break;
      }
      aRecord = aRecord->pNext;
    }

    // if no A record found, then get address the hard way
    if (aRecord == NULL)
      PIPSocket::GetHostAddress(record->hostName, record->hostAddress);
  }

  return record;
}

void PDNS::SRVRecordList::PrintOn(ostream & strm) const
{
  PINDEX i;
  for (i = 0; i < GetSize(); i++) 
    strm << (*this)[i] << endl;
}

PDNS::SRVRecord * PDNS::SRVRecordList::GetFirst()
{
  if (GetSize() == 0)
    return NULL;

  // create a list of all prioities, to save time
  priPos = 0;
  priList.SetSize(0);

  PINDEX i;
  if (GetSize() > 0) {
    priList.SetSize(1);
    WORD lastPri = (*this)[0].priority;
    priList[0] = lastPri;
    (*this)[0].used = PFalse;
    for (i = 1; i < GetSize(); i++) {
      (*this)[i].used = PFalse;
      if ((*this)[i].priority != lastPri) {
        priPos++;
        priList.SetSize(priPos);
        lastPri = (*this)[i].priority;
        priList[priPos] = lastPri;
      }
    }
  }
  
  priPos = 0;
  return GetNext();
}

PDNS::SRVRecord * PDNS::SRVRecordList::GetNext()
{
  if (priList.GetSize() == 0)
    return NULL;

  while (priPos < priList.GetSize()) {

    WORD currentPri = priList[priPos];

    // find first record at current priority
    PINDEX firstPos;
    for (firstPos = 0; (firstPos < GetSize()) && ((*this)[firstPos].priority != currentPri); firstPos++) 
      ;
    if (firstPos == GetSize())
      return NULL;

    // calculate total of all unused weights at this priority
    unsigned totalWeight = (*this)[firstPos].weight;
    PINDEX i = firstPos + 1;
    PINDEX count = 1;
    while (i < GetSize() && ((*this)[i].priority == currentPri)) {
      if (!(*this)[i].used) {
        totalWeight += (*this)[i].weight;
        count ++;
      }
      ++i;
    }

    // if no matches found, go to the next priority level
    if (count == 0) {
      priPos++;
      continue;
    }

    // selected the correct item
    if (totalWeight > 0) {
      unsigned targetWeight = PRandom::Number() % (totalWeight+1);
      totalWeight = 0;
      for (i = 0; i < GetSize() && ((*this)[i].priority == currentPri); i++) {
        if (!(*this)[i].used) {
          totalWeight += (*this)[i].weight;
          if (totalWeight >= targetWeight) {
            (*this)[i].used = PTrue;
            return &(*this)[i];
          }
        }
      }
    }

    // pick a random item at this priority
    PINDEX j = (count <= 1) ? 0 : (PRandom::Number() % count);
    count = 0;
    for (i = firstPos; i < GetSize() && ((*this)[i].priority == currentPri); i++) {
      if (!(*this)[i].used) {
        if (count == j) {
          (*this)[i].used = PTrue;
          return &(*this)[i];
        }
        count++;
      }
    }

    // go to the next priority level
    priPos++;
  }

  return NULL;
}

PBoolean PDNS::GetSRVRecords(
  const PString & _service,
  const PString & type,
  const PString & domain,
  PDNS::SRVRecordList & recordList
)
{
  if (_service.IsEmpty())
    return PFalse;

  PStringStream service;
  if (_service[0] != '_')
    service << '_';

  service << _service << "._" << type << '.' << domain;

  return GetSRVRecords(service, recordList);
}

PBoolean PDNS::LookupSRV(
           const PURL & url,
        const PString & service,
          PStringList & returnList)
{
  WORD defaultPort = url.GetPort();
  PIPSocketAddressAndPortVector info;

  if (!LookupSRV(url.GetHostName(), service, defaultPort, info)) {
    PTRACE(2,"DNS\tSRV Lookup Fail no domain " << url );
    return PFalse;
  }

  PString user = url.GetUserName();
  if (user.GetLength() > 0)
    user = user + "@";

  PIPSocketAddressAndPortVector::const_iterator r;
  for (r = info.begin(); r != info.end(); ++r) 
    returnList.AppendString(user + r->AsString(':'));

  return returnList.GetSize() != 0;;
}

PBoolean PDNS::LookupSRV(
         const PString & domain,            ///< domain to lookup
         const PString & service,           ///< service to use
                    WORD defaultPort,       ///< default port to use
         PIPSocketAddressAndPortVector & addrList  ///< list of sockets and ports
)
{
  if (domain.IsEmpty()) {
    PTRACE(1,"DNS\tSRV lookup failed - no domain specified");
    return PFalse;
  }

  PTRACE(4,"DNS\tSRV Lookup " << domain << " service " << service);
  
  PString srvLookupStr = service;
  if (srvLookupStr.Right(1) != ".")
    srvLookupStr += ".";
  srvLookupStr += domain;
  
  return LookupSRV(srvLookupStr, defaultPort, addrList);
}

PBoolean PDNS::LookupSRV(
              const PString & srvLookupStr,
              WORD defaultPort,
              PIPSocketAddressAndPortVector & addrList
)
{

  PDNS::SRVRecordList srvRecords;
  PBoolean found = PDNS::GetRecords(srvLookupStr, srvRecords);
  if (found) {
    PTRACE(5,"DNS\tSRV Record found " << srvLookupStr);
    PDNS::SRVRecord * recPtr = srvRecords.GetFirst();
    while (recPtr != NULL) {
      PIPSocketAddressAndPort addrAndPort(recPtr->hostAddress, recPtr->port > 0 ? recPtr->port : defaultPort);
      addrList.push_back(addrAndPort);

      recPtr = srvRecords.GetNext();
    }
  } 

  return found;
}

///////////////////////////////////////////////////////

PObject::Comparison PDNS::MXRecord::Compare(const PObject & obj) const
{
  const MXRecord * other = dynamic_cast<const MXRecord *>(&obj);
  if (other == NULL)
    return LessThan;

  if (preference < other->preference)
    return LessThan;
  else if (preference > other->preference)
    return GreaterThan;

  return EqualTo;
}

void PDNS::MXRecord::PrintOn(ostream & strm) const
{
  strm << "host=" << hostName << "(" << hostAddress << "), "
       << "preference=" << preference;
}

///////////////////////////////////////////////////////

PDNS::MXRecord * PDNS::MXRecordList::HandleDNSRecord(PDNS_RECORD dnsRecord, PDNS_RECORD results)
{
  MXRecord * record = NULL;

  if (
      (dnsRecord->Flags.S.Section == DnsSectionAnswer) &&
      (dnsRecord->wType == DNS_TYPE_MX) &&
#ifndef _WIN32_WCE
      (strlen(dnsRecord->Data.MX.pNameExchange) > 0)
#else
      (wcslen(dnsRecord->Data.MX.pNameExchange) > 0)
#endif
     ) {
    record = new MXRecord();
    record->hostName   = PString(dnsRecord->Data.MX.pNameExchange);
    record->preference = dnsRecord->Data.MX.wPreference;

    // see if any A records match this hostname
    PDNS_RECORD aRecord = results;
    while (aRecord != NULL) {
      if ((dnsRecord->Flags.S.Section == DnsSectionAdditional) && (dnsRecord->wType == DNS_TYPE_A)) {
        record->hostAddress = PIPSocket::Address(dnsRecord->Data.A.IpAddress);
        break;
      }
      aRecord = aRecord->pNext;
    }

    // if no A record found, then get address the hard way
    if (aRecord == NULL)
      PIPSocket::GetHostAddress(record->hostName, record->hostAddress);
  }

  return record;
}

void PDNS::MXRecordList::PrintOn(ostream & strm) const
{
  PINDEX i;
  for (i = 0; i < GetSize(); i++) 
    strm << (*this)[i] << endl;
}

PDNS::MXRecord * PDNS::MXRecordList::GetFirst()
{
  PINDEX i;
  for (i = 0; i < GetSize(); i++) 
    (*this)[i].used = PFalse;

  lastIndex = 0;

  return GetNext();
}

PDNS::MXRecord * PDNS::MXRecordList::GetNext()
{
  if (GetSize() == 0)
    return NULL;

  if (lastIndex >= GetSize())
    return NULL;

  return (PDNS::MXRecord *)GetAt(lastIndex++);
}

#endif // P_DNS


// End Of File ///////////////////////////////////////////////////////////////
