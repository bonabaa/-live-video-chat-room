/*
 *
 * Inter Asterisk Exchange 2
 * 
 * Class to implement thread safe handling of string lists.
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
#pragma implementation "safestrings.h"
#endif

#include <iax2/safestrings.h>

#define new PNEW

SafeStrings::SafeStrings()
{
}


SafeStrings::~SafeStrings()
{
}


void SafeStrings::AppendString(const PString & newString, PBoolean splitString)
{
  PWaitAndSignal m(accessMutex);
  
  if (!splitString) {
    data.AppendString(PString(newString));
    return;
  }
  
  for(PINDEX i = 0; i < newString.GetSize(); i++)
    data.AppendString(PString(newString[i]));
  
  return;
}

PBoolean SafeStrings::IsEmpty()
{
  PWaitAndSignal m(accessMutex);
  
  return data.GetSize() == 0;
}

PBoolean SafeStrings::GetNextString(PString & nextString)
{
  PWaitAndSignal m(accessMutex);
  
  if (data.GetSize() == 0)
    return PFalse;
  
  nextString = data[0];
  data.RemoveAt(0);
  
  return PTrue;
}


PString SafeStrings::GetFirstDeleteAll()
{
  PWaitAndSignal m(accessMutex);
  
  if (data.GetSize() == 0)
    return PString::Empty();
  
  PString res(data[0]);
  while (data.GetSize() > 0)
    data.RemoveAt(0);
  
  return res;
}

void SafeStrings::GetAllDeleteAll(PStringArray & res)
{
  PWaitAndSignal m(accessMutex);
  
  while (data.GetSize() > 0) {
    res.AppendString(PString(data[0]));
    data.RemoveAt(0);
  }
}

////////////////////////////////////////////////////////////////////////////////
void SafeString::operator=(PString newValue)
{ 
  PWaitAndSignal m(mutex); 
  internal = newValue;
}

void SafeString::PrintOn(ostream & str) const
{
  PWaitAndSignal m(mutex); 
  
  str << internal;
}

SafeString::operator PString()
{
  PWaitAndSignal m(mutex);
  
  return internal;
}

void SafeString::operator +=(PString toBeAdded)
{
  PWaitAndSignal m(mutex);

  internal += toBeAdded;
}

PBoolean SafeString::IsEmpty() const
{
  PWaitAndSignal m(mutex);

  return internal.IsEmpty();
}


PString SafeString::GetAndDelete()
{
  PWaitAndSignal m(mutex);

  PString res = internal;
  internal.MakeEmpty();

  return res;
}  
			
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

