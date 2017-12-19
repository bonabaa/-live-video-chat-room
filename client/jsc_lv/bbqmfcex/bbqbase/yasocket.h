//////////////////////////////////////////////////////////////////
//
// yasocket.h
//
// Copyright (c) Citron Network Inc. 2002-2003
//
// This work is published under the GNU Public License (GPL)
// see file COPYING for details.
// We also explicitely grant the right to link this code
// with the OpenH323 library.
//
// initial author: Chin-Wei Huang <cwhuang@linux.org.tw>
// initial version: 03/14/2003
//
//////////////////////////////////////////////////////////////////

#ifndef __yasocket_h__
#define __yasocket_h__

#include <ptlib.h>
#include <ptlib/sockets.h>


//#define LARGE_FDSET 32768
#ifdef LARGE_FDSET

#include <vector>

// yet another socket class to replace PSocket
class YaSocket;
class YaSelectList;

class YaSocket {
public:
	typedef PIPSocket::Address Address;

	YaSocket();
	virtual ~YaSocket() = 0;

	int GetHandle() const { return os_handle; }
	bool IsOpen() const { return os_handle > 0; }
	bool Close();

	void SetReadTimeout(const PTimeInterval & time) { readTimeout = time; }
	bool Read(void *, int);
	bool ReadBlock(void *, int);
	int GetLastReadCount() const { return lastReadCount; }

	void SetWriteTimeout(const PTimeInterval & time) { writeTimeout = time; }
	bool Write(const void *, int);
	int GetLastWriteCount() const { return lastWriteCount; }

	void GetLocalAddress(Address &) const;
	void GetLocalAddress(Address &, WORD &) const;

	PSocket::Errors GetErrorCode(PSocket::ErrorGroup group) const { return lastErrorCode[group]; }
	int GetErrorNumber(PSocket::ErrorGroup group) const { return lastErrorNumber[group]; }
	PString GetErrorText(PSocket::ErrorGroup) const;
	bool ConvertOSError(int libReturnValue, PSocket::ErrorGroup = PSocket::LastGeneralError);

protected:
	virtual int os_recv(void *, int) = 0;
	virtual int os_send(const void *, int) = 0;
	bool SetNonBlockingMode();

	int os_handle;
	int lastReadCount, lastWriteCount;

	PTimeInterval readTimeout, writeTimeout;

	PSocket::Errors lastErrorCode[PSocket::NumErrorGroups];
	int lastErrorNumber[PSocket::NumErrorGroups];
};

class YaTCPSocket : public YaSocket {
public:
	YaTCPSocket(WORD = 0);

	void GetPeerAddress(Address &) const;
	void GetPeerAddress(Address &, WORD &) const;

	WORD GetPort() const { return ntohs(peeraddr.sin_port); }
	void SetPort(WORD pt) { peeraddr.sin_port = htons(pt); }

protected:
	sockaddr_in peeraddr;

private:
	// override from class YaSocket
	virtual int os_recv(void *, int);
	virtual int os_send(const void *, int);
};

class YaUDPSocket : public YaSocket {
public:
	YaUDPSocket();

	bool Listen(unsigned, WORD);
	bool Listen(const Address &, unsigned, WORD);
	void GetLastReceiveAddress(Address &, WORD &) const;
	void SetSendAddress(const Address &, WORD);

	virtual bool ReadFrom(void *, PINDEX, Address &, WORD);
	virtual bool WriteTo(const void *, PINDEX, const Address &, WORD);

private:
	// override from class YaSocket
	virtual int os_recv(void *, int);
	virtual int os_send(const void *, int);

	sockaddr_in recvaddr, sendaddr;
};

class YaSelectList {
public:
	typedef std::vector<YaSocket *>::iterator iterator;
	typedef std::vector<YaSocket *>::const_iterator const_iterator;

	YaSelectList(YaSocket * = 0);

	void Append(YaSocket *);

	bool IsEmpty() const { return fds.empty(); }
	int GetSize() const { return fds.size(); }
	YaSocket *operator[](int i) const { return fds[i]; }

	enum SelectType {
		Read,
		Write
	};

	bool Select(SelectType, const PTimeInterval &);

	struct large_fd_set {
		large_fd_set() { memset(this, 0, sizeof(large_fd_set)); }
		void add(int fd) { if (fd > 0) FD_SET(fd, &__fdset__); }
		bool has(int fd) { return (fd > 0) ? FD_ISSET(fd, &__fdset__) : false; }
		operator fd_set *() { return &__fdset__; }

		union {
			fd_set __fdset__;
			char __mem__[LARGE_FDSET / 8];
		};
	};

private:
	std::vector<YaSocket *> fds;
	int maxfd;
};

typedef YaSelectList SocketSelectList;
typedef YaSocket IPSocket;
typedef YaTCPSocket TCPSocket;
typedef YaUDPSocket UDPSocket;

#else

class SocketSelectList : public PSocket::SelectList {
public:
	enum SelectType {
		Read,
		Write
	};
	SocketSelectList(PIPSocket *s = 0);
	bool Select(SelectType, const PTimeInterval &);
	PSocket *operator[](int i) const;
};

class SocketSelectList;
typedef PIPSocket IPSocket;
typedef PTCPSocket TCPSocket;
typedef PUDPSocket UDPSocket;

#endif // LARGE_FDSET

#endif // __yasocket_h__

