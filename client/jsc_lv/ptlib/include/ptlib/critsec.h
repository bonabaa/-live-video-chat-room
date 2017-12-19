/*
 * critsec.h
 *
 * Critical section mutex class.
 *
 * Portable Windows Library
 *
 * Copyright (C) 2004 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 22638 $
 * $Author: rjongbloed $
 * $Date: 2009-05-18 08:01:33 +0000 (Mon, 18 May 2009) $
 */

#ifndef PTLIB_CRITICALSECTION_H
#define PTLIB_CRITICALSECTION_H

#include <ptlib/psync.h>

#if defined(SOLARIS) && !defined(__GNUC__)
#include <atomic.h>
#endif

#if P_HAS_ATOMIC_INT

#if defined(__GNUC__)
#  if __GNUC__ >= 4 && __GNUC_MINOR__ >= 2
#     include <ext/atomicity.h>
#  else
#     include <bits/atomicity.h>
#  endif
#endif

#if P_NEEDS_GNU_CXX_NAMESPACE
#define EXCHANGE_AND_ADD(v,i)   __gnu_cxx::__exchange_and_add(v,i)
#else
#define EXCHANGE_AND_ADD(v,i)   __exchange_and_add(v,i)
#endif

#endif // P_HAS_ATOMIC_INT


/** This class implements critical section mutexes using the most
  * efficient mechanism available on the host platform.
  * For Windows, CriticalSection is used.
  * On other platforms, pthread_mutex_t is used
  */

#ifdef _WIN32

class PCriticalSection : public PSync
{
  PCLASSINFO(PCriticalSection, PSync);

  public:
  /**@name Construction */
  //@{
    /**Create a new critical section object .
     */
    PCriticalSection();

    /**Allow copy constructor, but it actually does not copy the critical section,
       it creates a brand new one as they cannot be shared in that way.
     */
    PCriticalSection(const PCriticalSection &);

    /**Destroy the critical section object
     */
    ~PCriticalSection();

    /**Assignment operator is allowed but does nothing. Overwriting the old critical
       section information would be very bad.
      */
    PCriticalSection & operator=(const PCriticalSection &) { return *this; }
  //@}

  /**@name Operations */
  //@{
    /** Create a new PCriticalSection
      */
    PObject * Clone() const
    {
      return new PCriticalSection();
    }

    /** Enter the critical section by waiting for exclusive access.
     */
    void Wait();
    inline void Enter() { Wait(); }

    /** Leave the critical section by unlocking the mutex
     */
    void Signal();
    inline void Leave() { Signal(); }

    /** Try to enter the critical section for exlusive access. Does not wait.
        @return true if cirical section entered, leave/Signal must be called.
      */
    bool Try();
  //@}


#include "msos/ptlib/critsec.h"

};

#endif

typedef PWaitAndSignal PEnterAndLeave;

/** This class implements an integer that can be atomically 
  * incremented and decremented in a thread-safe manner.
  * On Windows, the integer is of type long and this class is implemented using InterlockedIncrement
  * and InterlockedDecrement integer is of type long.
  * On Unix systems with GNU std++ support for EXCHANGE_AND_ADD, the integer is of type _Atomic_word (normally int)
  * On all other systems, this class is implemented using PCriticalSection and the integer is of type int.
  */

class PAtomicInteger 
{
  public:
#if defined(_WIN32)
    typedef long IntegerType;
#elif defined(_STLP_INTERNAL_THREADS_H) && defined(_STLP_ATOMIC_INCREMENT) && defined(_STLP_ATOMIC_DECREMENT)
    typedef __stl_atomic_t IntegerType;
#elif defined(SOLARIS) && !defined(__GNUC__)
    typedef uint32_t IntegerType;
#elif defined(__GNUC__) && P_HAS_ATOMIC_INT
    typedef _Atomic_word IntegerType;
#else
    typedef int IntegerType;
  protected:
    pthread_mutex_t m_mutex;
#endif

  protected:
    IntegerType m_value;

  public:
    /** Create a PAtomicInteger with the specified initial value
      */
    explicit PAtomicInteger(
      IntegerType value = 0                     ///< initial value
    );

    /// Destroy the atomic integer
    ~PAtomicInteger();

    /// @return Returns the value of the atomic integer
    __inline operator IntegerType() const { return m_value; }

    /// Assign a value to the atomic integer
    __inline PAtomicInteger & operator=(const PAtomicInteger & ref) { SetValue(ref); return *this; }

    /// Set the value of the atomic integer
    void SetValue(
      IntegerType value  ///< value to set
    );

    /**
      * Test if an atomic integer has a zero value. Note that this
      * is a non-atomic test - use the return value of the operator++() or
      * operator--() tests to perform atomic operations
      *
      * @return PTrue if the integer has a value of zero
      */
    __inline bool IsZero() const { return m_value == 0; }

    /// Test if atomic integer has a non-zero value.
    __inline bool operator!() const { return m_value != 0; }


    /**
      * atomically pre-increment the integer value
      *
      * @return Returns the value of the integer after the increment
      */
    IntegerType operator++();

    /**
      * atomically post-increment the integer value
      *
      * @return Returns the value of the integer before the increment
      */
    IntegerType operator++(int);

    /**
      * atomically pre-decrement the integer value
      *
      * @return Returns the value of the integer after the decrement
      */
    IntegerType operator--();

    /**
      * atomically post-decrement the integer value
      *
      * @return Returns the value of the integer before the decrement
      */
    IntegerType operator--(int);
};


#if defined(_WIN32) || defined(DOC_PLUS_PLUS)
__inline PAtomicInteger::PAtomicInteger(IntegerType value) : m_value(value) { }
__inline PAtomicInteger::~PAtomicInteger()                                  { }
__inline PAtomicInteger::IntegerType PAtomicInteger::operator++()           { return InterlockedIncrement(&m_value); }
__inline PAtomicInteger::IntegerType PAtomicInteger::operator++(int)        { return InterlockedExchangeAdd(&m_value, 1); }
__inline PAtomicInteger::IntegerType PAtomicInteger::operator--()           { return InterlockedDecrement(&m_value); }
__inline PAtomicInteger::IntegerType PAtomicInteger::operator--(int)        { return InterlockedExchangeAdd(&m_value, -1); }
__inline void PAtomicInteger::SetValue(IntegerType value)                   { m_value = value; }
#elif defined(_STLP_INTERNAL_THREADS_H) && defined(_STLP_ATOMIC_INCREMENT) && defined(_STLP_ATOMIC_DECREMENT)
__inline PAtomicInteger::PAtomicInteger(IntegerType value) : m_value(value) { }
__inline PAtomicInteger::~PAtomicInteger()                                  { }
__inline PAtomicInteger::IntegerType PAtomicInteger::operator++()           { return _STLP_ATOMIC_INCREMENT(&m_value); }
__inline PAtomicInteger::IntegerType PAtomicInteger::operator++(int)        { return _STLP_ATOMIC_INCREMENT(&m_value)-1; }
__inline PAtomicInteger::IntegerType PAtomicInteger::operator--()           { return _STLP_ATOMIC_DECREMENT(&m_value); }
__inline PAtomicInteger::IntegerType PAtomicInteger::operator--(int)        { return _STLP_ATOMIC_DECREMENT(&m_value)+1; }
__inline void PAtomicInteger::SetValue(IntegerType value)                   { m_value = value; }
#elif defined(SOLARIS) && !defined(__GNUC__)
__inline PAtomicInteger::PAtomicInteger(IntegerType value) : m_value(value) { }
__inline PAtomicInteger::~PAtomicInteger()                                  { }
__inline PAtomicInteger::IntegerType PAtomicInteger::operator++()           { return atomic_add_32_nv((&m_value), 1); }
__inline PAtomicInteger::IntegerType PAtomicInteger::operator++(int)        { return atomic_add_32_nv((&m_value), 1)-1; }
__inline PAtomicInteger::IntegerType PAtomicInteger::operator--()           { return atomic_add_32_nv((&m_value), -1); }
__inline PAtomicInteger::IntegerType PAtomicInteger::operator--(int)        { return atomic_add_32_nv((&m_value), -1)+1; }
__inline void PAtomicInteger::SetValue(IntegerType value)                   { m_value = value; }
#elif defined(__GNUC__) && P_HAS_ATOMIC_INT
__inline PAtomicInteger::PAtomicInteger(IntegerType value) : m_value(value) { }
__inline PAtomicInteger::~PAtomicInteger()                                  { }
__inline PAtomicInteger::IntegerType PAtomicInteger::operator++()           { return EXCHANGE_AND_ADD(&m_value, 1)+1; }
__inline PAtomicInteger::IntegerType PAtomicInteger::operator++(int)        { return EXCHANGE_AND_ADD(&m_value, 1); }
__inline PAtomicInteger::IntegerType PAtomicInteger::operator--()           { return EXCHANGE_AND_ADD(&m_value, -1)-1; }
__inline PAtomicInteger::IntegerType PAtomicInteger::operator--(int)        { return EXCHANGE_AND_ADD(&m_value, -1); }
__inline void PAtomicInteger::SetValue(IntegerType value)                   { m_value = value; }
#else
__inline PAtomicInteger::PAtomicInteger(IntegerType value) : m_value(value) { pthread_mutex_init(&m_mutex, NULL); }
__inline PAtomicInteger::~PAtomicInteger()                                  { pthread_mutex_destroy(&m_mutex); }
__inline PAtomicInteger::IntegerType PAtomicInteger::operator++()           { pthread_mutex_lock(&m_mutex); int retval = ++m_value; pthread_mutex_unlock(&m_mutex); return retval; }
__inline PAtomicInteger::IntegerType PAtomicInteger::operator++(int)        { pthread_mutex_lock(&m_mutex); int retval = m_value++; pthread_mutex_unlock(&m_mutex); return retval; }
__inline PAtomicInteger::IntegerType PAtomicInteger::operator--()           { pthread_mutex_lock(&m_mutex); int retval = --m_value; pthread_mutex_unlock(&m_mutex); return retval; }
__inline PAtomicInteger::IntegerType PAtomicInteger::operator--(int)        { pthread_mutex_lock(&m_mutex); int retval = m_value--; pthread_mutex_unlock(&m_mutex); return retval; }
__inline void PAtomicInteger::SetValue(IntegerType v)                       { pthread_mutex_lock(&m_mutex); m_value = v; pthread_mutex_unlock(&m_mutex); }
#endif


#endif // PTLIB_CRITICALSECTION_H


// End Of File ///////////////////////////////////////////////////////////////
