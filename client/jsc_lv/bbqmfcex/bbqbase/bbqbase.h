
#ifndef _BBQ_BASE_H
#define _BBQ_BASE_H

#pragma once
#ifdef _DEBUG
#pragma warning (disable:4018)// '<' : signed/unsigned mismatch
#pragma warning (disable:4245)// '=' : conversion from 'int' to 'uint32', signed/unsigned mismatch
#endif
#include <ptlib.h>
#include <ptclib/random.h>

#include "os_related.h"

#include "rwlock.h"
#include "thread.h"
#include "yasocket.h"

#include "log.h"

#include "sfidmsg.h"

#include "bytepack.h"
#include "msgconnection.h"
#include "msgterminal.h"

#include "ip2country.h"

#include "cert_rsa_aes.h"

#include "bbqproxy.h"

#include "bbqclient.h"

#include "bbqdatabase.h"
#include "bbqsvr.h"

// if use PConfig, PThread related classes in MFC application, 
// must create a PProcess object,
// call this function in InitInstance()

class nullProcess : public PProcess { \
	PCLASSINFO( nullProcess, PProcess ); \
public: \
	 nullProcess (
      const char * manuf = "",         /// Name of manufacturer
      const char * name = "",          /// Name of product
      WORD majorVersion = 1,           /// Major version number of the product
      WORD minorVersion = 0,           /// Minor version number of the product
      CodeStatus status = ReleaseCode, /// Development status of the product
      WORD buildNumber = 1             /// Build number of the product
    ) : PProcess( manuf, name, majorVersion, minorVersion, status, buildNumber ) {}; \
	void Main( void ) {}; \
};

#define InitPProcess( inst, manuf, app, major, minor, build_type, build_number ) \
{ \
	PProcess::PreInitialise(__argc, __argv, _environ); \
	static nullProcess this_process( (manuf), (app), (major), (minor), (build_type), (build_number) ); \
	this_process._main( inst ); \
	this_process.GetFile().GetDirectory().Change(); \
}

#endif // _BBQ_BASE_H

