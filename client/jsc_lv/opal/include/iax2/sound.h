/*
 *
 *
 * Inter Asterisk Exchange 2
 * 
 * A thread safe list of sound packets.
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

#ifndef OPAL_IAX2_SOUND_H
#define OPAL_IAX2_SOUND_H

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal/buildopts.h>

#if OPAL_IAX2

#ifdef P_USE_PRAGMA
#pragma interface
#endif

class IAX2Frame;
class IAXConnection;
class PSoundChannel;

////////////////////////////////////////////////////////////////////////////////

/**The soundlist is a list of PBYTEArray pointers, which are the
   medium of data exchange with the play/record devices.
   
   New sound blocks are added to the beginning of the list.
   
   Old sound blocks are removed from the end of the list.
   
   The IAX2SoundList is a thread safe storage of PBYTEArray structures.
*/
PDECLARE_LIST(IAX2SoundList, PBYTEArray *)
#ifdef DOC_PLUS_PLUS                           //This makes emacs bracket matching code happy.
class IAX2SoundList : public PBYTEArray *
{
#endif
 public:
  /**Destructor, which deletes all sound packets on this list*/
  ~IAX2SoundList();
  
  /**Removing item from list will not automatically delete it */
  void Initialise() {  DisallowDeleteObjects(); }
  
  /**Return a pointer to the last element on the list. 
     This will remove this element from the list.
     Returns NULL if the list is empty */
  PBYTEArray *GetLastEntry();
  
  /** Place a new data block at the beginning of the last. */
  void AddNewEntry(PBYTEArray *newEntry);
  
  /**Return a copy of all entries, and purge list */
  void GetAllDeleteAll(IAX2SoundList &dest);
  
  /** Thread safe read of the size of the list */
  PINDEX GetSize() { PWaitAndSignal m(mutex); return PAbstractList::GetSize(); }
  
 protected:
  /**Mutex to give thread safety. */
  PMutex      mutex;     
};


#endif // OPAL_IAX2

#endif // OPAL_IAX2_SOUND_H

/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 4 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-file-style:linux
 * c-basic-offset:2
 * End:
 */
