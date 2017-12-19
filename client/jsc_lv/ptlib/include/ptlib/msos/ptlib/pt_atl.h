/*
 * pt_atl.h
 *
 * File needed to fake some Win32 ATL stuff.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-2001 Equivalence Pty. Ltd.
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
 * Contributor(s):
 *
 * $Revision: 31544 $
 * $Author: ededu $
 * $Date: 2014-03-01 23:21:06 +1100 (Sat, 01 Mar 2014) $
 */

#ifndef PTLIB_ATL_H
#define PTLIB_ATL_H

#ifdef P_WIN_COM

#ifndef __MINGW32__
  #include <comdef.h>
  #include <initguid.h>
#endif

#ifdef P_ATL
  #pragma warning(disable:4127)
  #include <cguid.h>
  #include <atlbase.h>
  #include <atlcomcli.h>
  #include <oleacc.h>
  #pragma warning(default:4127)
#else

// We are using express edition of MSVC which does not come with ATL support
// So hand implement just enough for some code (e.g. SAPI) to work.
#define __ATLBASE_H__

#ifndef _WIN32_DCOM
  #define _WIN32_DCOM 1
#endif

#include <objbase.h>

typedef WCHAR OLECHAR;
typedef OLECHAR *LPOLESTR;
typedef const OLECHAR *LPCOLESTR;
typedef struct IUnknown IUnknown;
typedef IUnknown *LPUNKNOWN;

template <class T> class CComPtr
{
    T * m_pointer;
  public:
    CComPtr()                    : m_pointer(NULL) { }
    CComPtr(T * ptr)             : m_pointer(NULL) { Attach(ptr); }
    CComPtr(const CComPtr & ptr) : m_pointer(NULL) { Attach(ptr.m_pointer); }
    ~CComPtr() { Release(); }

    operator T *()          const { return  m_pointer; }
    T & operator*()         const { return *m_pointer; }
    T* operator->()         const { return  m_pointer; }
    T** operator&()               { return &m_pointer; }
    bool operator!()        const { return  m_pointer == NULL; }
    bool operator<(T* ptr)  const { return  m_pointer <  ptr; }
    bool operator==(T* ptr) const { return  m_pointer == ptr; }
    bool operator!=(T* ptr) const { return  m_pointer != ptr; }

    CComPtr & operator=(T * ptr)
    {
      Attach(ptr);
      return *this;
    }
    CComPtr & operator=(const CComPtr & ptr)
    {
      if (&ptr != this)
        Attach(ptr.m_pointer);
      return *this;
    }

    void Attach(T * ptr)
    {
      Release();
      if (ptr != NULL)
        ptr->AddRef();
      m_pointer = ptr;
    }

    T * Detach()
    {
      T * ptr = m_pointer;
      m_pointer = NULL;
      return ptr;
    }

    void Release()
    {
      T * ptr = m_pointer;
      if (ptr != NULL) {
        m_pointer = NULL;
        ptr->Release();
      }
    }

// mingw32 does not know __ parameters (yet)
#ifndef __MINGW32__
    __checkReturn HRESULT CoCreateInstance(
      __in     REFCLSID  rclsid,
      __in_opt LPUNKNOWN pUnkOuter    = NULL,
      __in     DWORD     dwClsContext = CLSCTX_ALL,
      __in     REFIID    riid         = __uuidof(T))
    {
      return ::CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, (void**)&m_pointer);
    }
#endif
};


#endif // P_ATL

class PComResult
{
  public:
    PComResult(HRESULT result = ERROR_SUCCESS) : m_result(result) { }
    PComResult & operator=(HRESULT result) { m_result = result; return *this; }

    bool Succeeded() const
    {
      return SUCCEEDED(m_result);
    }

    bool Succeeded(HRESULT result)
    {
      m_result = result;
      return Succeeded();
    }

    bool Failed() const
    {
      return FAILED(m_result);
    }

    bool Failed(HRESULT result)
    {
      m_result = result;
      return Failed();
    }

    int GetErrorNumber() const { return (int)m_result; }

    friend std::ostream & operator<<(std::ostream & strm, const PComResult & result);

  #if PTRACING
    bool Succeeded(HRESULT result, const char * func, const char * file, int line, HRESULT nomsg1 = ERROR_SUCCESS, HRESULT nomsg2 = ERROR_SUCCESS);
    bool Failed   (HRESULT result, const char * func, const char * file, int line, HRESULT nomsg1 = ERROR_SUCCESS, HRESULT nomsg2 = ERROR_SUCCESS)
    {
      return !Succeeded(result, func, file, line, nomsg1, nomsg2);
    }
    #define PCOM_SUCCEEDED_EX(res,fn,args,...)  (res       ).Succeeded(fn args, #fn, __FILE__, __LINE__, __VA_ARGS__)
    #define PCOM_FAILED_EX(res,fn,args,...)     (res       ).Failed   (fn args, #fn, __FILE__, __LINE__, __VA_ARGS__)
    #define PCOM_SUCCEEDED(fn,args,...)         PComResult().Succeeded(fn args, #fn, __FILE__, __LINE__, __VA_ARGS__)
#ifdef __MINGW32__
    #define PCOM_FAILED(fn,args,...)            PComResult().Failed   (fn args, #fn, __FILE__, __LINE__, ##__VA_ARGS__)
#else
    #define PCOM_FAILED(fn,args,...)            PComResult().Failed   (fn args, #fn, __FILE__, __LINE__, __VA_ARGS__)
#endif
  #else
    #define PCOM_SUCCEEDED_EX(res,fn,args,...)  (res).Succeeded(fn args)
    #define PCOM_FAILED_EX(res,fn,args,...)     (res).Failed(fn args)
    #define PCOM_SUCCEEDED(fn,args,...)         SUCCEEDED(fn args)
    #define PCOM_FAILED(fn,args,...)            FAILED(fn args)
  #endif
  #define PCOM_RETURN_ON_FAILED(fn,args) if (PCOM_FAILED(fn,args)) return false

  protected:
    HRESULT m_result;
};


struct PComVariant : public VARIANT
{
  PComVariant() { VariantInit(this); }
  ~PComVariant() { VariantClear(this); }

  friend std::ostream & operator<<(std::ostream & strm, const PComVariant & var);

  PString AsString() const { PStringStream s; s<<*this; return s; }
};

#endif // P_WIN_COM

#endif //PTLIB_ATL_H
