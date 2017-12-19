

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0500 */
/* at Mon Apr 04 11:27:18 2011
 */
/* Compiler settings for .\YYSpriteX-2.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__


#ifndef __YYSpriteidl_h__
#define __YYSpriteidl_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef ___DYYSpriteX2_FWD_DEFINED__
#define ___DYYSpriteX2_FWD_DEFINED__
typedef interface _DYYSpriteX2 _DYYSpriteX2;
#endif 	/* ___DYYSpriteX2_FWD_DEFINED__ */


#ifndef ___DYYSpriteX2Events_FWD_DEFINED__
#define ___DYYSpriteX2Events_FWD_DEFINED__
typedef interface _DYYSpriteX2Events _DYYSpriteX2Events;
#endif 	/* ___DYYSpriteX2Events_FWD_DEFINED__ */


#ifndef __YYSpriteX2_FWD_DEFINED__
#define __YYSpriteX2_FWD_DEFINED__

#ifdef __cplusplus
typedef class YYSpriteX2 YYSpriteX2;
#else
typedef struct YYSpriteX2 YYSpriteX2;
#endif /* __cplusplus */

#endif 	/* __YYSpriteX2_FWD_DEFINED__ */


#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __YYSpriteX2Lib_LIBRARY_DEFINED__
#define __YYSpriteX2Lib_LIBRARY_DEFINED__

/* library YYSpriteX2Lib */
/* [control][helpstring][helpfile][version][uuid] */ 


EXTERN_C const IID LIBID_YYSpriteX2Lib;

#ifndef ___DYYSpriteX2_DISPINTERFACE_DEFINED__
#define ___DYYSpriteX2_DISPINTERFACE_DEFINED__

/* dispinterface _DYYSpriteX2 */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__DYYSpriteX2;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("83A7866F-F1E8-4EFD-81B9-34C30D356199")
    _DYYSpriteX2 : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _DYYSpriteX2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _DYYSpriteX2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _DYYSpriteX2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _DYYSpriteX2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _DYYSpriteX2 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _DYYSpriteX2 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _DYYSpriteX2 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _DYYSpriteX2 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } _DYYSpriteX2Vtbl;

    interface _DYYSpriteX2
    {
        CONST_VTBL struct _DYYSpriteX2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _DYYSpriteX2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define _DYYSpriteX2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define _DYYSpriteX2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define _DYYSpriteX2_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define _DYYSpriteX2_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define _DYYSpriteX2_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define _DYYSpriteX2_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___DYYSpriteX2_DISPINTERFACE_DEFINED__ */


#ifndef ___DYYSpriteX2Events_DISPINTERFACE_DEFINED__
#define ___DYYSpriteX2Events_DISPINTERFACE_DEFINED__

/* dispinterface _DYYSpriteX2Events */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__DYYSpriteX2Events;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("0AB76D3A-9CAA-44CA-9C0D-B72843C14C58")
    _DYYSpriteX2Events : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _DYYSpriteX2EventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _DYYSpriteX2Events * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _DYYSpriteX2Events * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _DYYSpriteX2Events * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _DYYSpriteX2Events * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _DYYSpriteX2Events * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _DYYSpriteX2Events * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _DYYSpriteX2Events * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } _DYYSpriteX2EventsVtbl;

    interface _DYYSpriteX2Events
    {
        CONST_VTBL struct _DYYSpriteX2EventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _DYYSpriteX2Events_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define _DYYSpriteX2Events_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define _DYYSpriteX2Events_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define _DYYSpriteX2Events_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define _DYYSpriteX2Events_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define _DYYSpriteX2Events_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define _DYYSpriteX2Events_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___DYYSpriteX2Events_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_YYSpriteX2;

#ifdef __cplusplus

class DECLSPEC_UUID("74E59815-8639-4E51-99A1-47F1A57160D9")
YYSpriteX2;
#endif
#endif /* __YYSpriteX2Lib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


