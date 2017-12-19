//////////////////////////////////////////////////////////////////
//
// thread.cxx
//
//////////////////////////////////////////////////////////////////

//#include "thread.h"

#include "bbqbase.h"

#ifdef WIN32
inline DWORD getpid()
{
	return GetCurrentThreadId();
}
#endif

// class MyPThread
MyPThread::MyPThread(bool del)
      : PThread(5000, del ? AutoDeleteThread : NoAutoDeleteThread), isOpen(true)
{
}

void MyPThread::Main()
{
	PTRACE(2, GetClass() << ' ' << getpid() << " started");
	while (isOpen)
		Exec();

	PTRACE(2, GetClass() << ' ' << getpid() << " closed");
}

void MyPThread::Close()
{
	isOpen = false;
	sync.Signal();
}

bool MyPThread::Wait()
{
	sync.Wait();
	return isOpen;
}

void MyPThread::Go()
{
	if (sync.WillBlock())
		sync.Signal();
}

bool MyPThread::Destroy()
{
	Close();
	WaitForTermination();
	delete this;
	return true; // useless, workaround for VC
}

