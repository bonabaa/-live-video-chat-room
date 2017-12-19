#ifndef __SF_H
#define __SF_H
typedef unsigned SF_HANDLE;

typedef int (*OnCB)(int errorCode);
typedef void (*OnSendPacket)(int channel, const void *data, int len);
#endif