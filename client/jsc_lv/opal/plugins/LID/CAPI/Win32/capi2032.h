/*
 * capi2032.h
 */

# ifndef __CAPI2032_H
# define __CAPI2032_H

# ifdef __cplusplus
extern "C" {
# endif

#include <windows.h>

// Function prototypes for early or late binding on DLL

#define CAPI_DLL_NAME "CAPI2032.DLL"



#define CAPI_REGISTER_ORDINAL 1

typedef DWORD (WINAPI *CAPI_REGISTER_FUNCTION)(DWORD MessageBufferSize,
                                                 DWORD MaxLogicalConnection,
                                                 DWORD MaxBDataBlocks,
                                                 DWORD MaxBDataLen,
                                                 DWORD *pApplID);

DWORD WINAPI CAPI_REGISTER(DWORD MessageBufferSize,
                             DWORD MaxLogicalConnection,
                             DWORD MaxBDataBlocks,
                             DWORD MaxBDataLen,
                             DWORD *pApplID);



#define CAPI_RELEASE_ORDINAL 2

typedef DWORD (WINAPI *CAPI_RELEASE_FUNCTION)(DWORD ApplID);

DWORD WINAPI CAPI_RELEASE(DWORD ApplID);



#define CAPI_PUT_MESSAGE_ORDINAL 3

typedef DWORD (WINAPI *CAPI_PUT_MESSAGE_FUNCTION)(DWORD ApplID, PVOID pCAPIMessage);

DWORD WINAPI CAPI_PUT_MESSAGE(DWORD ApplID, PVOID pCAPIMessage);



#define CAPI_GET_MESSAGE_ORDINAL 4

typedef DWORD (WINAPI *CAPI_GET_MESSAGE_FUNCTION)(DWORD ApplID, PVOID *ppCAPIMessage);

DWORD WINAPI CAPI_GET_MESSAGE(DWORD ApplID, PVOID *ppCAPIMessage);



#define CAPI_WAIT_FOR_SIGNAL_ORDINAL 5

typedef DWORD (WINAPI *CAPI_WAIT_FOR_SIGNAL_FUNCTION)(DWORD ApplID);

DWORD WINAPI CAPI_WAIT_FOR_SIGNAL(DWORD ApplID);



#define CAPI_GET_MANUFACTURER_ORDINAL 6

typedef DWORD (WINAPI *CAPI_GET_MANUFACTURER_FUNCTION)(char *szBuffer);

DWORD WINAPI CAPI_GET_MANUFACTURER(char *szBuffer);



#define CAPI_GET_VERSION_ORDINAL 7

typedef DWORD (WINAPI *CAPI_GET_VERSION_FUNCTION)(DWORD *pCAPIMajor,
                                                    DWORD *pCAPIMinor,
                                                    DWORD *pManufacturerMajor,
                                                    DWORD *pManufacturerMinor);

DWORD WINAPI CAPI_GET_VERSION(DWORD *pCAPIMajor,
                                DWORD *pCAPIMinor,
                                DWORD *pManufacturerMajor,
                                DWORD *pManufacturerMinor);



#define CAPI_GET_SERIAL_NUMBER_ORDINAL 8

typedef DWORD (WINAPI *CAPI_GET_SERIAL_NUMBER_FUNCTION)(char *szBuffer);

DWORD WINAPI CAPI_GET_SERIAL_NUMBER(char *szBuffer);



#define CAPI_GET_PROFILE_ORDINAL 9

typedef DWORD (WINAPI *CAPI_GET_PROFILE_FUNCTION)(PVOID pBuffer, DWORD CtrlNr);

DWORD WINAPI CAPI_GET_PROFILE(PVOID pBuffer, DWORD CtrlNr);



#define CAPI_INSTALLED_ORDINAL 10

typedef DWORD (WINAPI *CAPI_INSTALLED_FUNCTION)(VOID);

DWORD WINAPI CAPI_INSTALLED(VOID);



#ifdef __cplusplus
}
#endif

#ifndef __NO_CAPIUTILS__
#include "capiutils.h"
#endif

#endif /* __CAPI2032_H */

