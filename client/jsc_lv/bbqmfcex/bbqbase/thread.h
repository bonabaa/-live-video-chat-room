//////////////////////////////////////////////////////////////////
//
// thread.h
//
// Copyright (c) Citron Network Inc. 2001-2003
//
// This work is published under the GNU Public License (GPL)
// see file COPYING for details.
// We also explicitely grant the right to link this code
// with the OpenH323 library.
//
// initial author: Chin-Wei Huang <cwhuang@linux.org.tw>
// initial version: 3/18/2003
//
//////////////////////////////////////////////////////////////////

#ifndef _MY_THREAD_H
#define _MY_THREAD_H

#include <ptlib.h>
#include <ptlib/thread.h>

class MyPThread : public PThread {
public:
	PCLASSINFO ( MyPThread, PThread )

	MyPThread(bool = false);
	virtual ~MyPThread() {}

	// override from class PThread
	virtual void Main();

	virtual void Close();
	virtual void Exec() = 0;

	bool Wait();
	void Go();

	bool Destroy();

protected:
	PSyncPoint sync;
	bool isOpen;
};

#endif // _MY_THREAD_H

