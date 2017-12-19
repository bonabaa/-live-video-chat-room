/*
 * machdep.h
 *
 * Unix machine dependencies
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

#ifndef _PMACHDEP_H
#define _PMACHDEP_H

///////////////////////////////////////////////////////////////////////////////
#if defined(P_LINUX)

#include <paths.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/termios.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>
#include <dlfcn.h>


#define HAS_IFREQ
#define PSETPGRP()  setpgrp()

#if __GNU_LIBRARY__ < 6
#define	P_LINUX_LIB_OLD
typedef int socklen_t;
#endif

#ifdef PPC
typedef size_t socklen_t;
#endif

///////////////////////////////////////////////////////////////////////////////
#elif defined(P_FREEBSD)

#if defined(P_PTHREADS)
#define _THREAD_SAFE
#define P_THREAD_SAFE_CLIB

#include <pthread.h>
extern "C" int sigwait(sigset_t *set);
#endif

#include <paths.h>
#include <errno.h>
#include <dlfcn.h>
#include <termios.h>
#include <sys/fcntl.h>
#include <sys/filio.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/signal.h>
#include <net/if.h>

#define HAS_IFREQ

#define PSETPGRP()  setpgrp(0, 0)

typedef int socklen_t;

///////////////////////////////////////////////////////////////////////////////
#elif defined(P_SOLARIS)

#include <errno.h>
#include <sys/sockio.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/filio.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>
#include <dlfcn.h>
#include <net/if.h>
#include <sys/sockio.h>

#define PSETPGRP()  setpgrp()

#define	INADDR_NONE	-1
#if P_SOLARIS < 7
typedef int socklen_t;
#endif

#define HAS_IFREQ

extern "C" {

int ftime (struct timeb *);
pid_t wait3(int *status, int options, struct rusage *rusage);
int gethostname(char *, int);

};

///////////////////////////////////////////////////////////////////////////////
#elif defined (P_SUN4)

#include <errno.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <net/if.h>
#include <sys/sockio.h>

#define HAS_IFREQ
#define PSETPGRP()  setpgrp(0, 0)
#define raise(s)    kill(getpid(),s)

#error P_SUN4

extern "C" {

char *mktemp(char *);
int accept(int, struct sockaddr *, int *);
int connect(int, struct sockaddr *, int);
int ioctl(int, int, void *);
int recv(int, void *, int, int);
int recvfrom(int, void *, int, int, struct sockaddr *, int *);
int select(int, fd_set *, fd_set *, fd_set *, struct timeval *);
int sendto(int, const void *, int, int, const struct sockaddr *, int);
int send(int, const void *, int, int);
int shutdown(int, int);
int socket(int, int, int);
int vfork();
void bzero(void *, int);
void closelog();
void gettimeofday(struct timeval * tv, struct timezone * tz);
void openlog(const char *, int, int);
void syslog(int, char *, ...);
int setpgrp(int, int);
pid_t wait3(int *status, int options, struct rusage *rusage);
int bind(int, struct sockaddr *, int);
int listen(int, int);
int getsockopt(int, int, int, char *, int *);
int setsockopt(int, int, int, char *, int);
int getpeername(int, struct sockaddr *, int *);
int gethostname(char *, int);
int getsockname(int, struct sockaddr *, int *);
char * inet_ntoa(struct in_addr);

int ftime (struct timeb *);

struct hostent * gethostbyname(const char *);
struct hostent * gethostbyaddr(const char *, int, int);
struct servent * getservbyname(const char *, const char *);

#include <sys/termios.h>
#undef NL0
#undef NL1
#undef CR0
#undef CR1
#undef CR2
#undef CR3
#undef TAB0
#undef TAB1
#undef TAB2
#undef XTABS
#undef BS0
#undef BS1
#undef FF0
#undef FF1
#undef ECHO
#undef NOFLSH
#undef TOSTOP
#undef FLUSHO
#undef PENDIN
};


#elif __BEOS__

#include <errno.h>
#include <termios.h>
#include <sys/socket.h>
#include <OS.h>

#define SOCK_RAW 3 // raw-protocol interface, not suported in R4
#define PF_INET AF_INET
typedef int socklen_t;
#define INADDR_NONE INADDR_BROADCAST
#define wait3(s, o, r) waitpid(-1, s, o)
#define PSETPGRP()  setpgid(0,0)

#else

// Other operating systems here

#endif

#ifdef __NUCLEUS_MNT__
#pragma message ("<netdb.h> not included")
#define P_PLATFORM_HAS_THREADS
// The windows version of errno.h, which this will find, should do for us -
// it contains lots of things from Unix!!!
#include <errno.h>
#else
#ifdef __NUCLEUS_PLUS__
#define P_PLATFORM_HAS_THREADS
#endif
#ifndef __NUCLEUS_PLUS__
#include <netdb.h>
#endif
#endif


#if defined(P_PTHREADS)
#define P_PLATFORM_HAS_THREADS
#ifndef __NUCLEUS_PLUS__
#include <pthread.h>
#endif
#endif

// If we're running effectively 'doze, then it's little endian.  If PoserPC,
// big endian.
#ifdef __NUCLEUS_MNT__
#define PBYTE_ORDER PLITTLE_ENDIAN
#else
//#define PBYTE_ORDER PBIG_ENDIAN
#endif

#if 0
#ifdef __NUCLEUS_PLUS__
// Other things
#define	INADDR_NONE	-1
#endif
#endif

#ifdef __NUCLEUS_MNT__
#define BREAKPOINT _asm int 3;
#endif
#ifdef __ppc
#define BREAKPOINT asm("sc");
#endif

#endif // _PMACHDEP_H

// End of file
