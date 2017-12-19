/*
 * semaphor.h
 *
 * Thread synchronisation semaphore class.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
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
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 20385 $
 * $Author: rjongbloed $
 * $Date: 2008-06-04 10:40:38 +0000 (Wed, 04 Jun 2008) $
 */

  public:
    unsigned GetInitial() const { return initialVar; }
    unsigned GetMaxCount() const     { return maxCountVar; }

  protected:
    unsigned initialVar;
    unsigned maxCountVar;

#if defined(P_MAC_MPTHREADS)
  protected:
    mutable MPSemaphoreID semId;
#elif defined(P_PTHREADS)

    enum PXClass { PXSemaphore, PXMutex, PXSyncPoint } pxClass;
    PXClass GetSemClass() const { return pxClass; }

  protected:
    PSemaphore(PXClass);
    mutable pthread_mutex_t mutex;
    mutable pthread_cond_t  condVar;
    
#if defined(P_HAS_SEMAPHORES)
    mutable sem_t semId;
#elif defined(P_HAS_NAMED_SEMAPHORES)
    mutable sem_t *semId;
    sem_t *CreateSem(unsigned initialValue);
#else
    mutable unsigned currentCount;
    mutable unsigned maximumCount;
    mutable unsigned queuedLocks;
  friend void PThread::Terminate();
#endif

#elif defined(__BEOS__)

  public:
    PSemaphore(PBoolean fNested); 
    void Create(unsigned initial);

  protected:
    PBoolean mfNested; // Does it support recursive locks?
    thread_id mOwner; // delete can be called by owner thread
    sem_id semId;
    volatile int32 mCount;

#elif defined(VX_TASKS)

  public:
    PSemaphore( SEM_ID anId );
  protected:
    mutable SEM_ID semId;

#else

  protected:
    PQUEUE(ThreadQueue, PThread);
    ThreadQueue waitQueue;

#endif

// End Of File ////////////////////////////////////////////////////////////////
