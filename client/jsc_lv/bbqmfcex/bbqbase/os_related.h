
#ifndef _OS_RELATED_H_
#define _OS_RELATED_H_

// ------- cross platform compile ---------

#ifdef CROSS
#define PACKED __attribute__((__packed__))  
#else  
//#error
#define PACKED  
#endif


// --------- basic types --------------
#ifdef _WIN32

typedef unsigned __int64	uint64;
typedef unsigned __int32	uint32;
typedef unsigned __int16	uint16;
typedef unsigned __int8		uint8;

typedef __int64				int64;
typedef	__int64				MS_TIME;

#else

typedef unsigned long long	uint64;
typedef unsigned int	uint32;
typedef unsigned short	uint16;
typedef unsigned char	uint8;

typedef long long		int64;
typedef	long long		MS_TIME;

#endif

// --------- socket ----------------------

#ifdef _WIN32

#define startup_socket() { WSADATA wsaData; WSAStartup(MAKEWORD(2,1), &wsaData); }
#define cleanup_socket() { WSACleanup(); }
#define set_socket_nonblock(fd) { DWORD fionbio = 1; int ret = ioctlsocket( fd, FIONBIO, & fionbio ); }

#else

#define startup_socket()
#define cleanup_socket()
#define	closesocket(x)	close(x)
#define set_socket_nonblock(fd) {	int flags = fcntl( fd, F_GETFL, 0 ); int ret = fcntl( fd, F_SETFL, flags | O_NONBLOCK ); }

#ifndef SOCKET_ERROR
#define	SOCKET_ERROR -1
#endif

#endif

// ------- sleep --------------

#ifdef _WIN32
#define sleep_ms(ms)	if(ms>=0){::Sleep(ms);} // under windows, sleep 0 means give up CPU
#else
#define sleep_ms(ms)	if(ms>0){ struct timespec ts; ts.tv_sec = ms / 1000; ts.tv_nsec = (ms % 1000) * 1000000; nanosleep( & ts, NULL ); } // under linux, do not sleep if ms == 0
#endif

// ----- CRT functions -----------

#ifdef _WIN32
#define stricmp(s1,s2)				_stricmp(s1,s2)
#define strnicmp(s1,s2,n)				_strnicmp(s1,s2,n)
//#ifndef strcasecmp
//#define	strcasecmp(s1,s2)			_stricmp(s1,s2)
//#define	strncasecmp(s1,s2,n)		_strnicmp(s1,s2,n)
//#endif

#include <direct.h>
#define stat						_stat
#define	unlink						_unlink
#define strdup						_strdup
#if _MSC_VER >= 1400				// from vistual studio 2005, the functions are deprecated and display warning
//#define _CRT_SECURE_NO_WARNINGS
#endif

#else
#include <unistd.h>
//typedef unsigned int size_t;
#endif

// ------ time ------------

#ifdef _WIN32
#include <sys/utime.h>
#else
#include <utime.h>
#endif

#ifdef _WIN32
// windows 
#else
// unix/linux
#include <unistd.h>
#include <fcntl.h>
#endif

// --------- STL -------------
#ifdef _WIN32
// x erase(x) return next
#define	STL_ERASE(var, iter_type, iter) (iter = (var).erase(iter))
#else
// void erase(x)
#define STL_ERASE(var, iter_type, iter) { iter_type _tmp = iter; iter ++; (var).erase(_tmp); }
#endif


#endif // _OS_RELATED_H_
