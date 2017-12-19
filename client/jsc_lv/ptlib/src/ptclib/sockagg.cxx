/*
 * sockagg.cxx
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

#ifdef __GNUC__
#pragma implementation "sockagg.h"
#endif


#include <ptlib.h>
#include <ptclib/sockagg.h>

#define new PNEW


PThreadPoolBase::PThreadPoolBase(unsigned _max)
  : maxWorkerSize(_max)
{
}

PThreadPoolBase::~PThreadPoolBase()
{
  for (;;) {
    PWaitAndSignal m(listMutex);
    if (workers.size() == 0)
      break;

    PThreadPoolWorkerBase * worker = workers[0];
    worker->Shutdown();
    workers.erase(workers.begin());
    StopWorker(worker);
  }
}


PThreadPoolWorkerBase * PThreadPoolBase::AllocateWorker()
{
  // find the worker thread with the minimum number of handles
  WorkerList_t::iterator minWorker = workers.end();
  size_t minSizeFound = 0x7ffff;
  WorkerList_t::iterator r;
  for (r = workers.begin(); r != workers.end(); ++r) {
    PThreadPoolWorkerBase & worker = **r;
    PWaitAndSignal m2(worker.workerMutex);
    if (!worker.shutdown && (worker.GetWorkSize() <= minSizeFound)) {
      minSizeFound = worker.GetWorkSize();
      minWorker     = r;
      if (minSizeFound == 0)
        break;
    }
  }

  // if the number of workers is at the maximum, or the there is 
  // an idle worker, then use the least busy worker thread
  if ((workers.size() >= maxWorkerSize) || (r != workers.end())) 
    return *minWorker;

  // no worker threads usable, create a new one
  PThreadPoolWorkerBase * worker = CreateWorkerThread();
  worker->Resume();
  workers.push_back(worker);

  return worker;
}

bool PThreadPoolBase::CheckWorker(PThreadPoolWorkerBase * worker)
{
  {
    PWaitAndSignal m(listMutex);

    // find worker in list
    WorkerList_t::iterator r;
    for (r = workers.begin(); r != workers.end(); ++r)
      if (*r == worker)
        break;
    if (r == workers.end())
      return false;

    // if the worker thread has enough work to keep running, leave it alone
    if (worker->GetWorkSize() > 0) 
      return true;

    // but don't shut down the last thread, so we don't have the overhead of starting it up again
    if (workers.size() == 1)
      return true;

    worker->Shutdown();
    workers.erase(r);
  }

  StopWorker(worker);

  return true;
}

void PThreadPoolBase::StopWorker(PThreadPoolWorkerBase * worker)
{
  // the worker is now finished
  if (!worker->WaitForTermination(10000)) {
    PTRACE(4, "SockAgg\tWorker did not terminate promptly");
  }
  PTRACE(4, "ThreadPool\tDestroying pool thread");
  delete worker;
}

////////////////////////////////////////////////////////////////

PThreadPoolWorkerBase::PThreadPoolWorkerBase(PThreadPoolBase & _pool)
  : PThread(100, NoAutoDeleteThread, NormalPriority, "Aggregator")
  , pool(_pool)
  , shutdown(PFalse)
{
}

////////////////////////////////////////////////////////////////

#if 0   // aggregation disabled pending reimplmentation

PHandleAggregator::PHandleAggregator(unsigned _max)
  : PHandleAggregatorBase(_max)
{ 
}

PBoolean PHandleAggregator::AddHandle(PAggregatedHandle * handle)
{
  // perform the handle init function
  if (!handle->Init())
    return PFalse;

  PTRACE(4, "SockAgg\tAdding handle " << (void *)handle << " to new aggregator");

  return AddWork(handle);
}

PBoolean PHandleAggregator::RemoveHandle(PAggregatedHandle * handle)
{
  return RemoveWork(handle);
}

////////////////////////////////////////////////////////////////

typedef std::vector<PAggregatorFD::FD> fdList_t;

#ifdef _WIN32
#define	FDLIST_SIZE	WSA_MAXIMUM_WAIT_EVENTS
#else
#define	FDLIST_SIZE	64
#endif


PAggregatorWorker::PAggregatorWorker(PThreadPoolBase & _pool)
  : PThreadPoolWorkerBase(_pool), listChanged(PTrue)
{ 
}

void PAggregatorWorker::OnAddWork(PAggregatedHandle * handle)
{
  PWaitAndSignal m(workerMutex);

  handleList.push_back(handle);
  listChanged = PTrue;
  Trigger();

  PTRACE(4, "SockAgg\tAdding handle " << (void *)handle << " to aggregator - " << handleList.size() << " handles");
}

void PAggregatorWorker::OnRemoveWork(PAggregatedHandle * handle)
{
  PAssert(!handle->beingProcessed, "trying to close handle that is in use");

  PWaitAndSignal m(workerMutex);

  // do the de-init action
  handle->DeInit();

  // remove the handle from the worker's list of handles
  PAggregatedHandleList_t::iterator s = ::find(handleList.begin(), handleList.end(), handle);
  if (s != handleList.end())
    handleList.erase(s);

  // delete the handle if autodelete enabled
  if (handle->autoDelete)
    delete handle;

  // if list still has items, trigger it to refresh
  if (handleList.size() > 0) {
    PTRACE(4, "SockAgg\tRemoved handle " << (void *)handle << " from aggregator - " << handleList.size() << " handles remaining");
    listChanged = PTrue;
    Trigger();
  }
}

unsigned PAggregatorWorker::GetWorkSize() const
{
  PWaitAndSignal m(workerMutex);
  return handleList.size();
}

void PAggregatorWorker::Shutdown()
{
  PWaitAndSignal m(workerMutex);
  shutdown = PTrue;
  Trigger();
}

void PAggregatorWorker::Main()
{
  PTRACE(4, "SockAgg\taggregator started");

  fdList_t                  fdList;
  PAggregatorFDList_t       aggregatorFdList;

  typedef std::map<PAggregatorFD::FD, PAggregatedHandle *> PAggregatorFdToHandleMap_t;
  PAggregatorFdToHandleMap_t aggregatorFdToHandleMap;

  for (;;) {

    // create the list of fds to wait on and find minimum timeout
    PTimeInterval timeout(PMaxTimeInterval);
    PAggregatedHandle * timeoutHandle = NULL;

#ifndef _WIN32
    fd_set rfds;
    FD_ZERO(&rfds);
    int maxFd = 0;
#endif

    {
      PWaitAndSignal m(workerMutex);

      // check for shutdown
      if (shutdown)
        break;

      // if the list of handles has changed, clear the list of handles
      if (listChanged) {
        aggregatorFdList.erase       (aggregatorFdList.begin(),      aggregatorFdList.end());
        aggregatorFdList.reserve     (FDLIST_SIZE);
        fdList.erase                 (fdList.begin(),                fdList.end());
        fdList.reserve               (FDLIST_SIZE);
        aggregatorFdToHandleMap.erase(aggregatorFdToHandleMap.begin(),         aggregatorFdToHandleMap.end());
      }

      PAggregatedHandleList_t::iterator r;
      for (r = handleList.begin(); r != handleList.end(); ++r) {
        PAggregatedHandle * handle = *r;

        if (handle->closed)
          continue;

        if (listChanged) {
          PAggregatorFDList_t fds = handle->GetFDs();
          PAggregatorFDList_t::iterator s;
          for (s = fds.begin(); s != fds.end(); ++s) {
            fdList.push_back          ((*s)->fd);
            aggregatorFdList.push_back((*s));
            aggregatorFdToHandleMap.insert(PAggregatorFdToHandleMap_t::value_type((*s)->fd, handle));
          }
        }

        if (!handle->IsPreReadDone()) {
          handle->PreRead();
          handle->SetPreReadDone();
        }

        PTimeInterval t = handle->GetTimeout();
        if (t < timeout) {
          timeout = t;
          timeoutHandle = handle;
        }
      }

      // add in the event fd
      if (listChanged) {
        fdList.push_back(localEvent.GetHandle());
        listChanged = PFalse;
      }

#ifndef _WIN32
      // create the list of FDs
      fdList_t::iterator s;
      for (s = fdList.begin(); s != fdList.end(); ++s) {
        FD_SET(*s, &rfds);
        maxFd = PMAX(maxFd, *s);
      }
#endif
    } // workerMutex goes out of scope

#ifdef _WIN32
    DWORD nCount = fdList.size();
    DWORD ret = WSAWaitForMultipleEvents(nCount, 
                                         &fdList[0], 
                                         false, 
                                         (timeout == PMaxTimeInterval) ? WSA_INFINITE : (DWORD)timeout.GetMilliSeconds(), 
                                         PFalse);

    if (ret == WAIT_FAILED) {
      PTRACE(1, "SockAgg\tWSAWaitForMultipleEvents error " << WSAGetLastError());
    }

    {
      PWaitAndSignal m(workerMutex);

      // check for shutdown
      if (shutdown)
        break;

      if (ret == WAIT_TIMEOUT) {
        // make sure the handle did not disappear while we were waiting
        PAggregatedHandleList_t::iterator s = find(handleList.begin(), handleList.end(), timeoutHandle);
        if (s == handleList.end()) {
          PTRACE(4, "SockAgg\tHandle was removed while waiting");
        } 
        else {
          PTime start;

          timeoutHandle->beingProcessed = PTrue;
          timeoutHandle->closed = !timeoutHandle->OnRead();
          timeoutHandle->beingProcessed = PFalse;
  
          unsigned duration = (unsigned)(PTime() - start).GetMilliSeconds();
          if (duration > 50) {
            PTRACE(4, "SockAgg\tWarning - aggregator read routine was of extended duration = " << duration << " msecs");
          }
          if (!timeoutHandle->closed)
            timeoutHandle->SetPreReadDone(PFalse);
        }
      }

      else if (WAIT_OBJECT_0 <= ret && ret <= (WAIT_OBJECT_0 + nCount - 1)) {
        DWORD index = ret - WAIT_OBJECT_0;

        // if the event was triggered, redo the select
        if (index == nCount-1) {
          localEvent.Reset();
          continue;
        }

        PAggregatorFD * fd = aggregatorFdList[index];
        PAssert(fdList[index] == fd->fd, "Mismatch in fd lists");

        PAggregatorFdToHandleMap_t::iterator r = aggregatorFdToHandleMap.find(fd->fd);
        if (r != aggregatorFdToHandleMap.end()) {
          PAggregatedHandle * handle = r->second;
          PAggregatedHandleList_t::iterator s = find(handleList.begin(), handleList.end(), handle);
          if (s == handleList.end()) {
            PTRACE(4, "SockAgg\tHandle was removed while waiting");
          }
          else {
            WSANETWORKEVENTS events;
            WSAEnumNetworkEvents(fd->socket, fd->fd, &events);
            if (events.lNetworkEvents != 0) {

              // check for read events first so we process any data that arrives before closing
              if ((events.lNetworkEvents & FD_READ) != 0) {

                PTime start;

                handle->beingProcessed = PTrue;
                handle->closed = !handle->OnRead();
                handle->beingProcessed = PFalse;
    
#if PTRACING
                if (PTrace::CanTrace(4)) {
                  unsigned duration = (unsigned)(PTime() - start).GetMilliSeconds();
                  if (duration > 50)
                    PTRACE(4, "SockAgg\tWarning - aggregator read routine was of extended duration = " << duration << " msecs");
                }
#endif
              }

              // check for socket close
              if ((events.lNetworkEvents & FD_CLOSE) != 0)
                handle->closed = PTrue;

              if (!handle->closed) {
                // prepare for next read
                handle->SetPreReadDone(PFalse);
              } else {
                handle->beingProcessed = PTrue;
                handle->OnClose();
                handle->beingProcessed = PFalse;

                // make sure the list is refreshed without the closed socket
                listChanged = PTrue;
              }
            }
          }
        }
      }
    } // workerMutex goes out of scope

#else

#error "aggregation not yet implemented on Unix"

#if 0

    P_timeval pv = timeout;
    int ret = ::select(maxFd+1, &rfds, NULL, NULL, pv);

    if (ret < 0) {
      PTRACE(1, "SockAgg\tSelect failed with error " << errno);
    }

    // loop again if nothing was ready
    if (ret <= 0)
      continue;

    {
      PWaitAndSignal m(workerMutex);

      // check for shutdown
      if (shutdown)
        break;

      if (ret == 0) {
        PTime start;
        PBoolean closed = !timeoutHandle->OnRead();
        unsigned duration = (unsigned)(PTime() - start).GetMilliSeconds();
        if (duration > 50) {
          PTRACE(4, "SockAgg\tWarning - aggregator read routine was of extended duration = " << duration << " msecs");
        }
        if (!closed)
          timeoutHandle->SetPreReadDone(PFalse);
      }

      // check the event first
      else if (FD_ISSET(event.GetHandle(), &rfds)) {
        event.Reset();
        continue;
      }

      else {
        PAggregatorFD * fd = aggregatorFdList[ret];
        PAssert(fdList[ret] == fd->fd, "Mismatch in fd lists");
        PAggregatorFdToHandleMap_t::iterator r = aggregatorFdToHandleMap.find(fd->fd);
        if (r != aggregatorFdToHandleMap.end()) {
          PAggregatedHandle * handle = r->second;
          PTime start;
          PBoolean closed = !handle->OnRead();
          unsigned duration = (unsigned)(PTime() - start).GetMilliSeconds();
          if (duration > 50) {
            PTRACE(4, "SockAgg\tWarning - aggregator read routine was of extended duration = " << duration << " msecs");
          }
          if (!closed)
            handle->SetPreReadDone(PFalse);
        }
      }
    } // workerMutex goes out of scope
#endif // #if 0
#endif
  }

  PTRACE(4, "SockAgg\taggregator finished");
}


////////////////////////////////////////////////////////////////

#if _WIN32

#pragma comment(lib, "Ws2_32.lib")

#if 0

class LocalEvent : public EventBase
{
  public:
    LocalEvent()
    { 
      event = CreateEvent(NULL, PTrue, PFalse,NULL); 
      PAssert(event != NULL, "CreateEvent failed");
    }

    ~LocalEvent()
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

PAggregatorFD::PAggregatorFD(SOCKET v)
  : socket(v) 
{ 
  fd = WSACreateEvent(); 
  PAssert(WSAEventSelect(socket, fd, FD_READ | FD_CLOSE) == 0, "WSAEventSelect failed"); 
}

PAggregatorFD::~PAggregatorFD()
{ 
  WSACloseEvent(fd); 
}

bool PAggregatorFD::IsValid()
{ 
  return socket != INVALID_SOCKET; 
}

#else // #if _WIN32

#include <fcntl.h>

class LocalEvent : public PHandleAggregator::EventBase
{
  public:
    LocalEvent()
    { ::pipe(fds); }

    virtual ~LocalEvent()
    {
      close(fds[0]);
      close(fds[1]);
    }

    PAggregatorFD::FD GetHandle()
    { return fds[0]; }

    void Set()
    { char ch; write(fds[1], &ch, 1); }

    void Reset()
    { char ch; read(fds[0], &ch, 1); }

  protected:
    int fds[2];
};

PAggregatorFD::PAggregatorFD(int v)
  : fd(v) 
{ 
}

PAggregatorFD::~PAggregatorFD()
{
}

bool PAggregatorFD::IsValid()
{ 
  return fd >= 0; 
}

#endif  // aggregation disabled pending reimplmeentation

#endif // #endif _WIN32
