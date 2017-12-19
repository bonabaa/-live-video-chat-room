/*
 * sockagg.h
 *
 * Generalised Socket Aggregation functions
 *
 * Portable Windows Library
 *
 * Copyright (C) 2005 Post Increment
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
 * Portions of this code were written with the financial assistance of 
 * Metreos Corporation (http://www.metros.com).
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 20385 $
 * $Author: rjongbloed $
 * $Date: 2008-06-04 10:40:38 +0000 (Wed, 04 Jun 2008) $
 */


#ifndef _SOCKAGG_H
#define _SOCKAGG_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>
#include <ptlib/sockets.h>

#include <list>
#include <map>


/*
 *  These classes and templates implement a generic thread pooling mechanism
 */

class PThreadPoolBase;

class PThreadPoolWorkerBase : public PThread
{
  public:
    PThreadPoolWorkerBase(PThreadPoolBase & threadPool);

    virtual unsigned GetWorkSize() const = 0;
    virtual void Shutdown() = 0;

    //virtual void OnAddWork(work_base *);
    //virtual void OnRemoveWork(work_base *);

    PThreadPoolBase & pool;
    PBoolean shutdown;
    PMutex workerMutex;
};

class PThreadPoolBase : public PObject
{
  public:
    PThreadPoolBase(unsigned _max = 10);
    ~PThreadPoolBase();

    virtual PThreadPoolWorkerBase * CreateWorkerThread() = 0;

    virtual PThreadPoolWorkerBase * AllocateWorker();

  protected:
    virtual bool CheckWorker(PThreadPoolWorkerBase * worker);
    void StopWorker(PThreadPoolWorkerBase * worker);
    PMutex listMutex;
    typedef std::vector<PThreadPoolWorkerBase *> WorkerList_t;
    WorkerList_t workers;

    unsigned maxWorkerSize;
};

template <class WorkUnit_T, class WorkerThread_T>
class PThreadPool : public PThreadPoolBase
{
  PCLASSINFO(PThreadPool, PThreadPoolBase);
  public:
    typedef typename std::map<WorkUnit_T *, WorkerThread_T *> WorkUnitMap_T;

    PThreadPool(unsigned _max = 10)
      : PThreadPoolBase(_max) { }

    virtual PThreadPoolWorkerBase * CreateWorkerThread()
    { return new WorkerThread_T(*this); }

    bool AddWork(WorkUnit_T * workUnit)
    {
      PWaitAndSignal m(listMutex);

      PThreadPoolWorkerBase * _worker = AllocateWorker();
      if (_worker == NULL)
        return false;

      WorkerThread_T * worker = dynamic_cast<WorkerThread_T *>(_worker);
      workUnitMap.insert(typename WorkUnitMap_T::value_type(workUnit, worker));

      worker->OnAddWork(workUnit);

      return true;
    }

    bool RemoveWork(WorkUnit_T * workUnit)
    {
      PWaitAndSignal m(listMutex);

      // find worker with work unit to remove
      typename WorkUnitMap_T::iterator r = workUnitMap.find(workUnit);
      if (r == workUnitMap.end())
        return false;

      WorkerThread_T * worker = dynamic_cast<WorkerThread_T *>(r->second);

      workUnitMap.erase(r);

      worker->OnRemoveWork(workUnit);

      CheckWorker(worker);

      return true;
    }

  protected:
    WorkUnitMap_T workUnitMap;
};

#if 0 

// aggregator code disabled pending reimplementation

/////////////////////////////////////////////////////////////////////////////////////

/*

These classes implements a generalised method for aggregating sockets so that they can be handled by a single thread. It is
intended to provide a backwards-compatible mechanism to supplant the "one socket - one thread" model used by OpenH323
and OPAL with a model that provides a better thread to call ratio. A more complete explanation of this model can be
found in the white paper "Increasing the Maximum Call Density of OpenH323 and OPAL" which can be at:

         http://www.voxgratia.org/docs/call%20thread%20handling%20model%201.0.pdf

There are two assumptions made by this code:

  1) The most efficient way to handle I/O is for a thread to be blocked on I/O. Any sort of timer or other
     polling mechanism is less efficient

  2) The time taken to handle a received PDU is relatively small, and will not interfere with the handling of
     other calls that are handled in the same thread

UDP and TCP sockets are aggregated in different ways. UDP sockets are aggregated on the basis of a simple loop that looks
for received datagrams and then processes them. TCP sockets are more complex because there is no intrinsic record-marking 
protocol, so it is difficult to know when a complete PDU has been received. This problem is solved by having the loop collect
received data into a buffer until the handling routine decides that a full PDU has been received.

At the heart of each socket aggregator is a select statement that contains all of the file descriptors that are managed
by the thread. One extra handle for a pipe (or on Windows, a local socket) is added to each handle list so that the thread can
be woken up in order to allow the addition or removal of sockets to the list

*/

#include <ptlib.h>
#include <functional>
#include <vector>

/////////////////////////////////////////////////////////////////////////////////////
//
// this class encapsulates the system specific handle needed to specifiy a socket.
// On Unix systems, this is a simple file handle. This file handle is used to uniquely
// identify the socket and used in the "select" system call
// On Windows systems the SOCKET handle is used to identify the socket, but a seperate WSAEVENT
// handle is needed for the WSWaitForMultpleEvents call.
// This is further complicated by the fact that we need to treat some pairs of sockets as a single
// entity (i.e. RTP sockets) to avoid rewriting the RTP handler code.
//

class PAggregatedHandle;

class PAggregatorFD 
{
  public:
#ifdef _WIN32
    typedef WSAEVENT FD;
    typedef SOCKET FDType;
    SOCKET socket;
#else
    typedef int FD;
    typedef int FDType;
#endif

    PAggregatorFD(FDType fd);

    FD fd;

    ~PAggregatorFD();
    bool IsValid();
};

typedef std::vector<PAggregatorFD *> PAggregatorFDList_t;

/////////////////////////////////////////////////////////////////////////////////////
//
// This class defines an abstract class used to define a handle that can be aggregated
//
// Normally this will correspond directly to a socket, but for RTP this actually corresponds to two sockets
// which greatly complicates things
//

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4127)
#endif

class PAggregatedHandle : public PObject
{
  PCLASSINFO(PAggregatedHandle, PObject);
  public:
    PAggregatedHandle(PBoolean _autoDelete = PFalse)
      : autoDelete(_autoDelete), closed(PFalse), beingProcessed(PFalse), preReadDone(PFalse)
    { }

    virtual PAggregatorFDList_t GetFDs() = 0;

    virtual PTimeInterval GetTimeout()
    { return PMaxTimeInterval; }

    virtual PBoolean Init()      { return PTrue; }
    virtual PBoolean PreRead()   { return PTrue; }
    virtual PBoolean OnRead() = 0;
    virtual void DeInit()    { }
    virtual void OnClose()   { }

    virtual PBoolean IsPreReadDone() const
    { return preReadDone; }

    virtual void SetPreReadDone(PBoolean v = PTrue)
    { preReadDone = v; }

    PBoolean autoDelete;
    PBoolean closed;
    PBoolean beingProcessed;

  protected:
    PBoolean preReadDone;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif


/////////////////////////////////////////////////////////////////////////////////////
//
// This class is the actual socket aggregator
//

#ifdef _WIN32

class EventBase
{
  public:
    EventBase()
    { 
      event = ::CreateEvent(NULL, PTrue, PFalse,NULL); 
      PAssert(event != NULL, "CreateEvent failed");
    }

    ~EventBase()
    { CloseHandle(event); }

    PAggregatorFD::FD GetHandle()
    { return (PAggregatorFD::FD)event; }

    void Set()
    { SetEvent(event);  }

    void Reset()
    { ResetEvent(event); }

  protected:
    HANDLE event;
};

#endif

typedef std::list<PAggregatedHandle *> PAggregatedHandleList_t;

class PAggregatorWorker : public PThreadPoolWorkerBase
{
  public:
    PAggregatorWorker(PThreadPoolBase & _pool);

    unsigned GetWorkSize() const;
    void Shutdown();

    void OnAddWork(PAggregatedHandle *);
    void OnRemoveWork(PAggregatedHandle *);

    void Main();
    PAggregatedHandleList_t handleList;

    void Trigger()  { localEvent.Set(); }
    
    PBoolean listChanged;
};

typedef PThreadPool<PAggregatedHandle, PAggregatorWorker> PHandleAggregatorBase;

class PHandleAggregator : public PHandleAggregatorBase
{
  PCLASSINFO(PHandleAggregator, PHandleAggregatorBase)
  public:
    typedef std::list<PAggregatedHandle *> PAggregatedHandleList_t;

    PHandleAggregator(unsigned _max = 10);

    PBoolean AddHandle(PAggregatedHandle * handle);

    PBoolean RemoveHandle(PAggregatedHandle * handle);
};


/////////////////////////////////////////////////////////////////////////////////////
//
// This template class allows the creation of aggregators for sockets that are
// descendants of PIPSocket
//

#if 0

template <class PSocketType>
class PSocketAggregator : public PHandleAggregator
{
  PCLASSINFO(PSocketAggregator, PHandleAggregator)
  public:
    class AggregatedPSocket : public PAggregatedHandle
    {
      public:
        AggregatedPSocket(PSocketType * _s)
          : psocket(_s), fd(_s->GetHandle()) { }

        PBoolean OnRead()
        { return psocket->OnRead(); }

        PAggregatorFDList_t GetFDs()
        { PAggregatorFDList_t list; list.push_back(&fd); return list; }

      protected:
        PSocketType * psocket;
        PAggregatorFD fd;
    };

    typedef std::map<PSocketType *, AggregatedPSocket *> SocketList_t;
    SocketList_t socketList;

    PBoolean AddSocket(PSocketType * sock)
    { 
      PWaitAndSignal m(listMutex);

      AggregatedPSocket * handle = new AggregatedPSocket(sock);
      if (AddHandle(handle)) {
        socketList.insert(SocketList_t::value_type(sock, handle));
        return true;
      }

      delete handle;
      return false;
    }

    PBoolean RemoveSocket(PSocketType * sock)
    { 
      PWaitAndSignal m(listMutex);

      typename SocketList_t::iterator r = socketList.find(sock);
      if (r == socketList.end()) 
        return PFalse;

      AggregatedPSocket * handle = r->second;
      RemoveHandle(handle);
      delete handle;
      socketList.erase(r);
      return PTrue;
    }
};
#endif  // #if 0

#endif

// aggregator code disabled pending reimplementation

#endif // _SOCKAGG_H
