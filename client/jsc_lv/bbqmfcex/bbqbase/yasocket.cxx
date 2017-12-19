//////////////////////////////////////////////////////////////////
//
// yasocket.cxx
//
// Copyright (c) Citron Network Inc. 2002-2003
//
// This work is published under the GNU Public License (GPL)
// see file COPYING for details.
// We also explicitely grant the right to link this code
// with the OpenH323 library.
//
// initial author: Chih-Wei Huang <cwhuang@linux.org.tw>
// initial version: 03/14/2003
//
//////////////////////////////////////////////////////////////////

#include "bbqbase.h"

//#include "yasocket.h"

#ifdef LARGE_FDSET

#include "stl_supp.h"

// class YaSelectList

YaSelectList::YaSelectList(YaSocket *s) : maxfd(0)
{
	fds.reserve(512);
	Append(s);
}

void YaSelectList::Append(YaSocket *s)
{
	if (s) {
		fds.push_back(s);
		if (s->GetHandle() > maxfd)
			maxfd = s->GetHandle();
	}
}

bool YaSelectList::Select(SelectType t, const PTimeInterval & timeout)
{
	large_fd_set fdset;
	for_each(fds.begin(), fds.end(),
		 compose1(bind1st(mem_fun(&large_fd_set::add), &fdset), mem_fun(&YaSocket::GetHandle)));

	fd_set *readfds, *writefds;
	if (t == Read)
		readfds = fdset, writefds = 0;
	else
		writefds = fdset, readfds = 0;

	DWORD msec = timeout.GetInterval();
	struct timeval tval;
	tval.tv_usec = (msec % 1000) * 1000;
	tval.tv_sec  = msec / 1000;
	bool r = ::select(maxfd + 1, readfds, writefds, 0, &tval) > 0;
	if (r) {
		iterator last = remove_if(fds.begin(), fds.end(),
					  not1(compose1(bind1st(mem_fun(&large_fd_set::has), &fdset), mem_fun(&YaSocket::GetHandle))));
		fds.erase(last, fds.end());
	}
	return r;
}

// class YaSocket
YaSocket::YaSocket() : os_handle(-1)
{
	lastReadCount = lastWriteCount = 0;
}

YaSocket::~YaSocket()
{
	Close();
}

bool YaSocket::Close()
{
	if (!IsOpen())
		return false;

	// send a shutdown to the other end
	::shutdown(os_handle, SHUT_RDWR);
	::close(os_handle);
	os_handle = -1;
	return true;
}

bool YaSocket::Read(void *buf, int sz)
{
	int r = os_recv(buf, sz);
	lastReadCount = ConvertOSError(r, PSocket::LastReadError) ? r : 0;
	return lastReadCount > 0;
}

bool YaSocket::ReadBlock(void *buf, int len)
{
	// lazy implementation, but it is enough for us...
	return Read(buf, len) && (lastReadCount == len);
}

bool YaSocket::Write(const void *buf, int sz)
{
	lastWriteCount = 0;
	if (!YaSelectList(this).Select(YaSelectList::Write, writeTimeout)) {
		errno = EAGAIN;
		return ConvertOSError(-1, PSocket::LastWriteError);
	}
	int r = os_send(buf, sz);
	if (ConvertOSError(r, PSocket::LastWriteError))
		lastWriteCount = r;
	return lastWriteCount == sz;
}

void YaSocket::GetLocalAddress(Address & addr) const
{
	WORD pt;
	GetLocalAddress(addr, pt);
}

void YaSocket::GetLocalAddress(Address & addr, WORD & pt) const
{
	sockaddr_in inaddr;
	socklen_t insize = sizeof(inaddr);
	if (::getsockname(os_handle, (struct sockaddr*)&inaddr, &insize) == 0) {
		addr = inaddr.sin_addr;
		pt = ntohs(inaddr.sin_port);
	}
}

PString YaSocket::GetErrorText(PSocket::ErrorGroup group) const
{
	return PSocket::GetErrorText(GetErrorCode(group));
}

bool YaSocket::ConvertOSError(int libReturnValue, PSocket::ErrorGroup group)
{
	if ((libReturnValue < 0) && (errno == EAGAIN)) {
		lastErrorCode[group] = PSocket::Timeout;
		lastErrorNumber[group] = errno;
		return false;
	}
	return PSocket::ConvertOSError(libReturnValue, lastErrorCode[group], lastErrorNumber[group]);
}

bool YaSocket::SetNonBlockingMode()
{
	if (!IsOpen())
		return false;
	int cmd = 1;
	if (::ioctl(os_handle, FIONBIO, &cmd) == 0 && ::fcntl(os_handle, F_SETFD, 1) == 0)
		return true;
	Close();
	return false;

}

// class YaTCPSocket
YaTCPSocket::YaTCPSocket(WORD pt)
{
	peeraddr.sin_family = AF_INET;
	SetPort(pt);
}

void YaTCPSocket::GetPeerAddress(Address & addr) const
{
	addr = peeraddr.sin_addr;
}

void YaTCPSocket::GetPeerAddress(Address & addr, WORD & pt) const
{
	addr = peeraddr.sin_addr;
	pt = ntohs(peeraddr.sin_port);
}

int YaTCPSocket::os_recv(void *buf, int sz)
{
	return ::recv(os_handle, buf, sz, 0);
}

int YaTCPSocket::os_send(const void *buf, int sz)
{
	return ::send(os_handle, buf, sz, 0);
}

// class YaUDPSocket
YaUDPSocket::YaUDPSocket()
{
	sendaddr.sin_family = AF_INET;
}

bool YaUDPSocket::Listen(unsigned, WORD pt)
{
	return Listen(INADDR_ANY, 0, pt);
}

bool YaUDPSocket::Listen(const Address & addr, unsigned, WORD pt)
{
	os_handle = ::socket(PF_INET, SOCK_DGRAM, 0);
	if (!ConvertOSError(os_handle))
		return false;

	SetNonBlockingMode();
	SetSendAddress(addr, pt);
	int value = 1;
	return ConvertOSError(::bind(os_handle, (struct sockaddr *)&sendaddr, sizeof(sendaddr))) && ConvertOSError(::setsockopt(os_handle, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)));
}

void YaUDPSocket::GetLastReceiveAddress(Address & addr, WORD & pt) const
{
	addr = recvaddr.sin_addr;
	pt = ntohs(recvaddr.sin_port);
}

void YaUDPSocket::SetSendAddress(const Address & addr, WORD pt)
{
	sendaddr.sin_addr = addr;
	sendaddr.sin_port = htons(pt);
}

bool YaUDPSocket::ReadFrom(void *buf, PINDEX len, Address & addr, WORD pt)
{
	bool result = Read(buf, len);
	if (result)
		GetLastReceiveAddress(addr, pt);
	return result;
}

bool YaUDPSocket::WriteTo(const void *buf, PINDEX len, const Address & addr, WORD pt)
{
	SetSendAddress(addr, pt);
	return Write(buf, len);
}

int YaUDPSocket::os_recv(void *buf, int sz)
{
	socklen_t addrlen = sizeof(recvaddr);
	return ::recvfrom(os_handle, buf, sz, 0, (struct sockaddr *)&recvaddr, &addrlen);
}

int YaUDPSocket::os_send(const void *buf, int sz)
{
	return ::sendto(os_handle, buf, sz, 0, (struct sockaddr *)&sendaddr, sizeof(sendaddr));
}

#else

SocketSelectList::SocketSelectList(PIPSocket *s)
{
	if (s)
		Append(s);
}

bool SocketSelectList::Select(SelectType t, const PTimeInterval & timeout)
{
	SocketSelectList dumb, *rlist, *wlist;
	if (t == Read)
		rlist = this, wlist = &dumb;
	else
		wlist = this, rlist = &dumb;
#if PTRACING
	PSocket::Errors r = PSocket::Select(*rlist, *wlist, timeout);
	PTRACE_IF(3, r != PSocket::NoError, "ProxyH\tSelect error: " << r);
#endif
	return !IsEmpty();
}

PSocket *SocketSelectList::operator[](int i) const
{
	typedef PSocket::SelectList PSocketSelectList; // stupid VC...
	return &PSocketSelectList::operator[](i);
}

#endif // LARGE_FDSET

