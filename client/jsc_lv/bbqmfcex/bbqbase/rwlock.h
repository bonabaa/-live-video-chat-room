//////////////////////////////////////////////////////////////////
//
// rwlock.h
//
//////////////////////////////////////////////////////////////////

#ifndef __rwlock_h__
#define __rwlock_h__

#include <ptlib.h>

class LockIt {
	PMutex & mutex;
public:
	LockIt( PMutex & m ) : mutex(m) { 
		mutex.Wait(); 
	}
	~LockIt() { 
		mutex.Signal(); 
	}
};

class UnlockIt {
	PMutex & mutex;
public:
	UnlockIt( PMutex & m ) : mutex(m) { 
		mutex.Signal(); 
	}
	~UnlockIt() { 
		mutex.Wait(); 
	}
};

// Utility for PReadWriteMutex usage

class ReadLock {
	PReadWriteMutex & mutex;
  public:
	ReadLock(PReadWriteMutex & m) : mutex(m) { mutex.StartRead(); }
	~ReadLock() { mutex.EndRead(); }
};

class WriteLock {
	PReadWriteMutex & mutex;
  public:
	WriteLock(PReadWriteMutex & m) : mutex(m) { mutex.StartWrite(); }
	~WriteLock() { mutex.EndWrite(); }
};

class ReadUnlock {
	PReadWriteMutex & mutex;
  public:
	ReadUnlock(PReadWriteMutex & m) : mutex(m) { mutex.EndRead(); }
	~ReadUnlock() { mutex.StartRead(); }
};

class WriteUnlock {
	PReadWriteMutex & mutex;
  public:
	WriteUnlock(PReadWriteMutex & m) : mutex(m) { mutex.EndWrite(); }
	~WriteUnlock() { mutex.StartWrite(); }
};

#endif // __rwlock_h__

