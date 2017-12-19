/*
 * safecoll.cxx
 *
 * Thread safe collection classes.
 *
 * Portable Windows Library
 *
 * Copyright (c) 2002 Equivalence Pty. Ltd.
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
 * $Revision: 23517 $
 * $Author: rjongbloed $
 * $Date: 2009-09-23 06:48:15 +0000 (Wed, 23 Sep 2009) $
 */

#ifdef __GNUC__
#pragma implementation "safecoll.h"
#endif

#include <ptlib.h>
#include <ptlib/safecoll.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

PSafeObject::PSafeObject(PSafeObject * indirectLock)
  : safeReferenceCount(0)
  , safelyBeingRemoved(PFalse)
  , safeInUse(indirectLock != NULL ? indirectLock->safeInUse : &safeInUseMutex)
{
}


PBoolean PSafeObject::SafeReference()
{
#if PTRACING
  unsigned tracedReferenceCount;
#endif

  {
    PWaitAndSignal mutex(safetyMutex);
    if (safelyBeingRemoved)
      return PFalse;
    safeReferenceCount++;
#if PTRACING
    tracedReferenceCount = safeReferenceCount;
#endif
  }

  PTRACE(6, "SafeColl\tIncrement reference count to " << tracedReferenceCount << " for " << GetClass() << ' ' << (void *)this);
  return PTrue;
}


PBoolean PSafeObject::SafeDereference()
{
  PBoolean mayBeDeleted = PFalse;
#if PTRACING
  unsigned tracedReferenceCount;
#endif

  safetyMutex.Wait();
  if (PAssert(safeReferenceCount > 0, PLogicError)) {
    safeReferenceCount--;
    mayBeDeleted = safeReferenceCount == 0 && !safelyBeingRemoved;
  }
#if PTRACING
  tracedReferenceCount = safeReferenceCount;
#endif
  safetyMutex.Signal();

  PTRACE(6, "SafeColl\tDecrement reference count to " << tracedReferenceCount << " for " << GetClass() << ' ' << (void *)this);

  return mayBeDeleted;
}


PBoolean PSafeObject::LockReadOnly() const
{
  PTRACE(6, "SafeColl\tWaiting read ("<<(void *)this<<")");
  safetyMutex.Wait();

  if (safelyBeingRemoved) {
    safetyMutex.Signal();
    PTRACE(6, "SafeColl\tBeing removed while waiting read ("<<(void *)this<<")");
    return PFalse;
  }

  safetyMutex.Signal();
  safeInUse->StartRead();
  PTRACE(6, "SafeColl\tLocked read ("<<(void *)this<<")");
  return PTrue;
}


void PSafeObject::UnlockReadOnly() const
{
  PTRACE(6, "SafeColl\tUnlocked read ("<<(void *)this<<")");
  safeInUse->EndRead();
}


PBoolean PSafeObject::LockReadWrite()
{
  PTRACE(6, "SafeColl\tWaiting readWrite ("<<(void *)this<<")");
  safetyMutex.Wait();

  if (safelyBeingRemoved) {
    safetyMutex.Signal();
    PTRACE(6, "SafeColl\tBeing removed while waiting readWrite ("<<(void *)this<<")");
    return PFalse;
  }

  safetyMutex.Signal();
  safeInUse->StartWrite();
  PTRACE(6, "SafeColl\tLocked readWrite ("<<(void *)this<<")");
  return PTrue;
}


void PSafeObject::UnlockReadWrite()
{
  PTRACE(6, "SafeColl\tUnlocked readWrite ("<<(void *)this<<")");
  safeInUse->EndWrite();
}


void PSafeObject::SafeRemove()
{
  safetyMutex.Wait();
  safelyBeingRemoved = PTrue;
  safetyMutex.Signal();
}


PBoolean PSafeObject::SafelyCanBeDeleted() const
{
  PWaitAndSignal mutex(safetyMutex);
  return safelyBeingRemoved && safeReferenceCount == 0;
}


bool PSafeObject::GarbageCollection()
{
  return true;
}


/////////////////////////////////////////////////////////////////////////////

PSafeLockReadOnly::PSafeLockReadOnly(const PSafeObject & object)
  : safeObject((PSafeObject &)object)
{
  locked = safeObject.LockReadOnly();
}


PSafeLockReadOnly::~PSafeLockReadOnly()
{
  if (locked)
    safeObject.UnlockReadOnly();
}


PBoolean PSafeLockReadOnly::Lock()
{
  locked = safeObject.LockReadOnly();
  return locked;
}


void PSafeLockReadOnly::Unlock()
{
  if (locked) {
    safeObject.UnlockReadOnly();
    locked = PFalse;
  }
}



/////////////////////////////////////////////////////////////////////////////

PSafeLockReadWrite::PSafeLockReadWrite(const PSafeObject & object)
  : safeObject((PSafeObject &)object)
{
  locked = safeObject.LockReadWrite();
}


PSafeLockReadWrite::~PSafeLockReadWrite()
{
  if (locked)
    safeObject.UnlockReadWrite();
}


PBoolean PSafeLockReadWrite::Lock()
{
  locked = safeObject.LockReadWrite();
  return locked;
}


void PSafeLockReadWrite::Unlock()
{
  if (locked) {
    safeObject.UnlockReadWrite();
    locked = PFalse;
  }
}


/////////////////////////////////////////////////////////////////////////////

PSafeCollection::PSafeCollection(PCollection * coll)
{
  collection = coll;
  collection->DisallowDeleteObjects();
  toBeRemoved.DisallowDeleteObjects();
  deleteObjects = PTrue;
}


PSafeCollection::~PSafeCollection()
{
  deleteObjectsTimer.Stop();

  RemoveAll();

  /* Delete objects moved to deleted list in RemoveAll(), we don't use
     DeleteObjectsToBeRemoved() as that will do a garbage collection which might
     prevent deletion. Need to be a bit more forceful here. */
  for (PList<PSafeObject>::iterator i = toBeRemoved.begin(); i != toBeRemoved.end(); ++i) {
    i->GarbageCollection();
    if (i->SafelyCanBeDeleted())
      delete &*i;
    else {
      // If anything still has a PSafePtr .. "detach" it from the collection so
      // will be deleted whan that PSafePtr finally goes out of scope.
      i->safelyBeingRemoved = false;
    }
  }

  delete collection;
}


PBoolean PSafeCollection::SafeRemove(PSafeObject * obj)
{
  if (obj == NULL)
    return PFalse;

  PWaitAndSignal mutex(collectionMutex);
  if (!collection->Remove(obj))
    return PFalse;

  SafeRemoveObject(obj);
  return PTrue;
}


PBoolean PSafeCollection::SafeRemoveAt(PINDEX idx)
{
  PWaitAndSignal mutex(collectionMutex);
  PSafeObject * obj = PDownCast(PSafeObject, collection->RemoveAt(idx));
  if (obj == NULL)
    return PFalse;

  SafeRemoveObject(obj);
  return PTrue;
}


void PSafeCollection::RemoveAll(PBoolean synchronous)
{
  collectionMutex.Wait();

  while (collection->GetSize() > 0)
    SafeRemoveObject(PDownCast(PSafeObject, collection->RemoveAt(0)));

  collectionMutex.Signal();

  if (synchronous) {
    // Have unfortunate busy loop here, but it should be very
    // rare that it will be here for long
    while (!DeleteObjectsToBeRemoved())
      PThread::Sleep(100);
  }
}


void PSafeCollection::SafeRemoveObject(PSafeObject * obj)
{
  if (obj == NULL)
    return;

  // Make sure SfeRemove() called before SafeDereference() to avoid race condition
  if (deleteObjects) {
    obj->SafeRemove();

    removalMutex.Wait();
    toBeRemoved.Append(obj);
    removalMutex.Signal();
  }

  /* Even though we are marked as not to delete objects, we still need to obey
     the rule that if there are no references left the object is deleted. If
     the object is still "owned" by a PSafeCollection that has NOT got
     deleteObjects false, then SafeDereference returns false so we don't delete
     is here. If there are no PSafePtr()s or PSafeCollections()'s anywhere we
     need to delete it.
     */
  if (obj->SafeDereference() && !deleteObjects)
    delete obj;
}


PBoolean PSafeCollection::DeleteObjectsToBeRemoved()
{
  PWaitAndSignal lock(removalMutex);

  PList<PSafeObject>::iterator i = toBeRemoved.begin();
  while (i != toBeRemoved.end()) {
    if (i->GarbageCollection() && i->SafelyCanBeDeleted()) {
      PObject * obj = &*i;
      toBeRemoved.Remove(obj);
      removalMutex.Signal();
      DeleteObject(obj);
      removalMutex.Wait();

      i = toBeRemoved.begin(); // Restart looking through list
    }
    else
      i++;
  }

  return toBeRemoved.IsEmpty() && collection->IsEmpty();
}


void PSafeCollection::DeleteObject(PObject * object) const
{
  delete object;
}


void PSafeCollection::SetAutoDeleteObjects()
{
  if (deleteObjectsTimer.IsRunning())
    return;

  deleteObjectsTimer.SetNotifier(PCREATE_NOTIFIER(DeleteObjectsTimeout));
  deleteObjectsTimer.RunContinuous(1000); // EVery second
}


void PSafeCollection::DeleteObjectsTimeout(PTimer &, INT)
{
  DeleteObjectsToBeRemoved();
}


PINDEX PSafeCollection::GetSize() const
{
  PWaitAndSignal lock(collectionMutex);
  return collection->GetSize();
}


/////////////////////////////////////////////////////////////////////////////

PSafePtrBase::PSafePtrBase(PSafeObject * obj, PSafetyMode mode)
{
  collection = NULL;
  currentObject = obj;
  lockMode = mode;

  EnterSafetyMode(WithReference);
}


PSafePtrBase::PSafePtrBase(const PSafeCollection & safeCollection,
                           PSafetyMode mode,
                           PINDEX idx)
{
  collection = &safeCollection;
  currentObject = NULL;
  lockMode = mode;

  Assign(idx);
}


PSafePtrBase::PSafePtrBase(const PSafeCollection & safeCollection,
                           PSafetyMode mode,
                           PSafeObject * obj)
{
  collection = &safeCollection;
  currentObject = NULL;
  lockMode = mode;

  Assign(obj);
}


PSafePtrBase::PSafePtrBase(const PSafePtrBase & enumerator)
{
  collection = enumerator.collection;
  currentObject = enumerator.currentObject;
  lockMode = enumerator.lockMode;

  EnterSafetyMode(WithReference);
}


PSafePtrBase::~PSafePtrBase()
{
  ExitSafetyMode(WithDereference);
}


PObject::Comparison PSafePtrBase::Compare(const PObject & obj) const
{
  const PSafePtrBase * other = PDownCast(const PSafePtrBase, &obj);
  if (other == NULL)
    return GreaterThan;

  if (currentObject < other->currentObject)
    return LessThan;
  if (currentObject > other->currentObject)
    return GreaterThan;
  return EqualTo;
}


void PSafePtrBase::Assign(const PSafePtrBase & enumerator)
{
  if (this == &enumerator)
    return;

  // lockCount ends up zero after this
  ExitSafetyMode(WithDereference);

  collection = enumerator.collection;
  currentObject = enumerator.currentObject;
  lockMode = enumerator.lockMode;

  EnterSafetyMode(WithReference);
}


void PSafePtrBase::Assign(const PSafeCollection & safeCollection)
{
  // lockCount ends up zero after this
  ExitSafetyMode(WithDereference);

  collection = &safeCollection;
  currentObject = NULL;
  lockMode = PSafeReadWrite;

  Assign((PINDEX)0);
}


void PSafePtrBase::Assign(PSafeObject * newObj)
{
  ExitSafetyMode(WithDereference);

  currentObject = newObj;

  if (newObj == NULL)
    return;

  if (collection == NULL) {
    lockMode = PSafeReference;
    if (!EnterSafetyMode(WithReference))
      currentObject = NULL;
    return;
  }

  collection->collectionMutex.Wait();

  if (collection->collection->GetObjectsIndex(newObj) == P_MAX_INDEX) {
    collection->collectionMutex.Signal();
    collection = NULL;
    lockMode = PSafeReference;
    if (!EnterSafetyMode(WithReference))
      currentObject = NULL;
  }
  else {
    if (!newObj->SafeReference())
      currentObject = NULL;
    collection->collectionMutex.Signal();
    EnterSafetyMode(AlreadyReferenced);
  }
}


void PSafePtrBase::Assign(PINDEX idx)
{
  ExitSafetyMode(WithDereference);

  currentObject = NULL;

  if (collection == NULL)
    return;

  collection->collectionMutex.Wait();

  while (idx < collection->collection->GetSize()) {
    currentObject = (PSafeObject *)collection->collection->GetAt(idx);
    if (currentObject != NULL) {
      if (currentObject->SafeReference())
        break;
      currentObject = NULL;
    }
    idx++;
  }

  collection->collectionMutex.Signal();

  EnterSafetyMode(AlreadyReferenced);
}


void PSafePtrBase::Next()
{
  if (collection == NULL || currentObject == NULL)
    return;

  ExitSafetyMode(NoDereference);

  collection->collectionMutex.Wait();

  PINDEX idx = collection->collection->GetObjectsIndex(currentObject);

  currentObject->SafeDereference();
  currentObject = NULL;

  if (idx != P_MAX_INDEX) {
    while (++idx < collection->collection->GetSize()) {
      currentObject = (PSafeObject *)collection->collection->GetAt(idx);
      if (currentObject != NULL) {
        if (currentObject->SafeReference())
          break;
        currentObject = NULL;
      }
    }
  }

  collection->collectionMutex.Signal();

  EnterSafetyMode(AlreadyReferenced);
}


void PSafePtrBase::Previous()
{
  if (collection == NULL || currentObject == NULL)
    return;

  ExitSafetyMode(NoDereference);

  collection->collectionMutex.Wait();

  PINDEX idx = collection->collection->GetObjectsIndex(currentObject);

  currentObject->SafeDereference();
  currentObject = NULL;

  if (idx != P_MAX_INDEX) {
    while (idx-- > 0) {
      currentObject = (PSafeObject *)collection->collection->GetAt(idx);
      if (currentObject != NULL) {
        if (currentObject->SafeReference())
          break;
        currentObject = NULL;
      }
    }
  }

  collection->collectionMutex.Signal();

  EnterSafetyMode(AlreadyReferenced);
}


void PSafePtrBase::SetNULL()
{
  // lockCount ends up zero after this
  ExitSafetyMode(WithDereference);

  collection = NULL;
  currentObject = NULL;
  lockMode = PSafeReference;
}


PBoolean PSafePtrBase::SetSafetyMode(PSafetyMode mode)
{
  if (lockMode == mode)
    return PTrue;

  ExitSafetyMode(NoDereference);
  lockMode = mode;
  return EnterSafetyMode(AlreadyReferenced);
}


PBoolean PSafePtrBase::EnterSafetyMode(EnterSafetyModeOption ref)
{
  if (currentObject == NULL)
    return PFalse;

  if (ref == WithReference && !currentObject->SafeReference()) {
    currentObject = NULL;
    return PFalse;
  }

  switch (lockMode) {
    case PSafeReadOnly :
      if (currentObject->LockReadOnly())
        return PTrue;
      break;

    case PSafeReadWrite :
      if (currentObject->LockReadWrite())
        return PTrue;
      break;

    case PSafeReference :
      return PTrue;
  }

  currentObject->SafeDereference();
  currentObject = NULL;
  return PFalse;
}


void PSafePtrBase::ExitSafetyMode(ExitSafetyModeOption ref)
{
  if (currentObject == NULL)
    return;

  switch (lockMode) {
    case PSafeReadOnly :
      currentObject->UnlockReadOnly();
      break;

    case PSafeReadWrite :
      currentObject->UnlockReadWrite();
      break;

    case PSafeReference :
      break;
  }

  if (ref == WithDereference && currentObject->SafeDereference()) {
    PSafeObject * objectToDelete = currentObject;
    currentObject = NULL;
    PTRACE(6, "SafeColl\tDeleting object (" << objectToDelete << ')');
    delete objectToDelete;
  }
}


/////////////////////////////////////////////////////////////////////////////

PSafePtrMultiThreaded::PSafePtrMultiThreaded(PSafeObject * obj, PSafetyMode mode)
  : PSafePtrBase(NULL, mode)
{
  m_mutex.Wait();

  currentObject = obj;
  EnterSafetyMode(WithReference);

  m_mutex.Signal();
}


PSafePtrMultiThreaded::PSafePtrMultiThreaded(const PSafeCollection & safeCollection,
                                             PSafetyMode mode,
                                             PINDEX idx)
  : PSafePtrBase(NULL, mode)
{
  m_mutex.Wait();

  collection = &safeCollection;
  Assign(idx);

  m_mutex.Signal();
}


PSafePtrMultiThreaded::PSafePtrMultiThreaded(const PSafeCollection & safeCollection,
                                             PSafetyMode mode,
                                             PSafeObject * obj)
  : PSafePtrBase(NULL, mode)
{
  m_mutex.Wait();

  collection = &safeCollection;
  Assign(obj);

  m_mutex.Signal();
}


PSafePtrMultiThreaded::PSafePtrMultiThreaded(const PSafePtrMultiThreaded & enumerator)
{
  enumerator.m_mutex.Wait();
  m_mutex.Wait();

  collection = enumerator.collection;
  currentObject = enumerator.currentObject;
  lockMode = enumerator.lockMode;

  EnterSafetyMode(WithReference);

  m_mutex.Signal();
  enumerator.m_mutex.Signal();
}


PSafePtrMultiThreaded::~PSafePtrMultiThreaded()
{
  m_mutex.Wait();
  ExitSafetyMode(WithDereference);
  currentObject = NULL;
  m_mutex.Signal();
}


PObject::Comparison PSafePtrMultiThreaded::Compare(const PObject & obj) const
{
  PWaitAndSignal mutex(m_mutex);
  return PSafePtrBase::Compare(obj);
}


void PSafePtrMultiThreaded::SetNULL()
{
  m_mutex.Wait();
  PSafePtrBase::SetNULL();
  m_mutex.Signal();
}


PBoolean PSafePtrMultiThreaded::SetSafetyMode(PSafetyMode mode)
{
  PWaitAndSignal mutex(m_mutex);
  return PSafePtrBase::SetSafetyMode(mode);
}


void PSafePtrMultiThreaded::Assign(const PSafePtrMultiThreaded & ptr)
{
  ptr.m_mutex.Wait();
  m_mutex.Wait();
  PSafePtrBase::Assign(ptr);
  m_mutex.Signal();
  ptr.m_mutex.Signal();
}


void PSafePtrMultiThreaded::Assign(const PSafePtrBase & ptr)
{
  m_mutex.Wait();
  PSafePtrBase::Assign(ptr);
  m_mutex.Signal();
}


void PSafePtrMultiThreaded::Assign(const PSafeCollection & safeCollection)
{
  m_mutex.Wait();
  PSafePtrBase::Assign(safeCollection);
  m_mutex.Signal();
}


void PSafePtrMultiThreaded::Assign(PSafeObject * obj)
{
  m_mutex.Wait();
  PSafePtrBase::Assign(obj);
  m_mutex.Signal();
}


void PSafePtrMultiThreaded::Assign(PINDEX idx)
{
  m_mutex.Wait();
  PSafePtrBase::Assign(idx);
  m_mutex.Signal();
}


void PSafePtrMultiThreaded::Next()
{
  m_mutex.Wait();
  PSafePtrBase::Next();
  m_mutex.Signal();
}


void PSafePtrMultiThreaded::Previous()
{
  m_mutex.Wait();
  PSafePtrBase::Previous();
  m_mutex.Signal();
}


// End of File ///////////////////////////////////////////////////////////////
