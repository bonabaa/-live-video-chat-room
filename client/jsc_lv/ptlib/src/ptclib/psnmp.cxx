/*
 * psnmp.cxx
 *
 * SNMP base and support classes.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-2002 Equivalence Pty. Ltd.
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
 * $Revision: 20385 $
 * $Author: rjongbloed $
 * $Date: 2008-06-04 10:40:38 +0000 (Wed, 04 Jun 2008) $
 */

#ifdef __GNUC__
#pragma implementation "psnmp.h"
#endif

#include <ptlib.h>
#include <ptbuildopts.h>

#ifdef P_SNMP
#include <ptclib/psnmp.h>

#define new PNEW


static char const * const SnmpErrorCodeTable[] = 
{
  "no error",
  "too big",
  "no such name",
  "bad value",
  "read only",
  "gen err",

  "no response",
  "malformed response",
  "send failed",
  "rx buff too small",
  "tx data too big"
};

static char const * const TrapCodeToText[PSNMP::NumTrapTypes] = {
  "Cold Start",
  "Warm Start",
  "Link Down",
  "Link Up",
  "Auth Fail",
  "EGP Loss",
  "Enterprise"
};


///////////////////////////////////////////////////////////////
//
//  PSNMPVarBindingList
//

void PSNMPVarBindingList::Append(const PString & objectID)
{
  objectIds.AppendString(objectID);
  values.Append(new PASNNull());
}


void PSNMPVarBindingList::Append(const PString & objectID, PASNObject * obj)
{
  objectIds.AppendString(objectID);
  values.Append(obj);
}


void PSNMPVarBindingList::AppendString(const PString & objectID, const PString & str)
{
  Append(objectID, new PASNString(str));
}


void PSNMPVarBindingList::RemoveAll()
{
  objectIds.RemoveAll();
  values.RemoveAll();
}


PINDEX PSNMPVarBindingList::GetSize() const
{
  return objectIds.GetSize();
}

PINDEX PSNMPVarBindingList::GetIndex(const PString & objectID) const
{
  return objectIds.GetStringsIndex(objectID);
}


PASNObject & PSNMPVarBindingList::operator[](PINDEX idx) const
{
  return values[idx];
}


PString PSNMPVarBindingList::GetObjectID(PINDEX idx) const
{ 
  return objectIds[idx];
}


void PSNMPVarBindingList::PrintOn(ostream & strm) const
{
  for (PINDEX i = 0; i < GetSize(); i++) 
    strm << objectIds[i] 
         << " = "
         << values[i];
}


PString PSNMP::GetTrapTypeText(PINDEX code)
{
  PString str;
  if (code >= NumTrapTypes)
    return "Unknown";
  else
    return TrapCodeToText[code];
}


PString PSNMP::GetErrorText(ErrorType err) 
{
  if (err >= NumErrors)
    return "unknown error";
  else
    return SnmpErrorCodeTable[err];
}

#endif // P_SNMP

// End Of File ///////////////////////////////////////////////////////////////
