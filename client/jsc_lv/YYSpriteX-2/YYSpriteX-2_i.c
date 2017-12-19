

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Thu Mar 17 10:54:51 2016
 */
/* Compiler settings for YYSpriteX-2.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 7.00.0555 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, LIBID_YYSpriteX2Lib,0x979C0907,0xD1B5,0x4830,0x87,0x03,0xC6,0x5D,0x8D,0x75,0xF4,0x4D);


MIDL_DEFINE_GUID(IID, DIID__DYYSpriteX2,0x83A7866F,0xF1E8,0x4EFD,0x81,0xB9,0x34,0xC3,0x0D,0x35,0x61,0x99);


MIDL_DEFINE_GUID(IID, DIID__DYYSpriteX2Events,0x0AB76D3A,0x9CAA,0x44CA,0x9C,0x0D,0xB7,0x28,0x43,0xC1,0x4C,0x58);


MIDL_DEFINE_GUID(CLSID, CLSID_YYSpriteX2,0x74E59815,0x8639,0x4E51,0x99,0xA1,0x47,0xF1,0xA5,0x71,0x60,0xD9);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



