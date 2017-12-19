/*
 * Common-ISDN API LID for OPAL
 *
 * Copyright (C) 2006 Post Increment, All Rights Reserved
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
 * The Original Code is OPAL.
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 20527 $
 * $Author: rjongbloed $
 * $Date: 2008-07-02 12:07:41 +0000 (Wed, 02 Jul 2008) $
 */

#define _CRT_SECURE_NO_DEPRECATE
#define PLUGIN_DLL_EXPORTS
#include <lids/lidplugin.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32

#include <process.h>
#include <windows.h>

typedef BYTE  uint8_t;
typedef WORD  uint16_t;
typedef DWORD uint32_t;

class Mutex : CRITICAL_SECTION
{
public:
  inline Mutex()       { InitializeCriticalSection(this); }
  inline ~Mutex()      { DeleteCriticalSection(this); }
  inline void Lock()   { EnterCriticalSection(this); }
  inline void Unlock() { LeaveCriticalSection(this); }
};

class Semaphore
{
private:
  HANDLE m_hSemaphore;
public:
  inline Semaphore(unsigned count = 1) { m_hSemaphore = CreateSemaphore(NULL, 0, count, NULL); }
  inline ~Semaphore()                  { CloseHandle(m_hSemaphore); }
  inline void Signal()                 { ReleaseSemaphore(m_hSemaphore,1, NULL); }
  inline bool Wait(unsigned timeout)   { return WaitForSingleObject(m_hSemaphore, timeout) == WAIT_OBJECT_0; }
};


#include "win32/capi2032.h"

class CAPI
{
private:
  HMODULE m_hDLL;
  CAPI_REGISTER_FUNCTION          m_REGISTER;
  CAPI_RELEASE_FUNCTION           m_RELEASE;
  CAPI_PUT_MESSAGE_FUNCTION       m_PUT_MESSAGE;
  CAPI_GET_MESSAGE_FUNCTION       m_GET_MESSAGE;
  CAPI_WAIT_FOR_SIGNAL_FUNCTION   m_WAIT_FOR_SIGNAL;
  CAPI_GET_MANUFACTURER_FUNCTION  m_GET_MANUFACTURER;
  CAPI_GET_VERSION_FUNCTION       m_GET_VERSION;
  CAPI_GET_SERIAL_NUMBER_FUNCTION m_GET_SERIAL_NUMBER;
  CAPI_GET_PROFILE_FUNCTION       m_GET_PROFILE;
  CAPI_INSTALLED_FUNCTION         m_INSTALLED;

public:
  typedef DWORD Result;
  typedef DWORD ApplID;
  typedef DWORD UInt;
  enum {
    InvalidApplId = 0
  };

  CAPI()
  {
    memset(this, 0, sizeof(*this));
    m_hDLL = LoadLibrary(CAPI_DLL_NAME);
  }

  ~CAPI()
  {
    if (m_hDLL != NULL)
      FreeLibrary(m_hDLL);
  }

  #define DEF_FN(fn, def, arg) \
    Result fn def \
    { \
      return m_hDLL != NULL && \
            (m_##fn != NULL || (m_##fn = (CAPI_##fn##_FUNCTION)GetProcAddress(m_hDLL, (LPCSTR)CAPI_##fn##_ORDINAL)) != NULL) \
            ? m_##fn arg : 0x10000; \
    }

  #define DEF_FN1(fn, t,p)                        DEF_FN(fn, (t p), (p))
  #define DEF_FN2(fn, t1,p1, t2,p2)               DEF_FN(fn, (t1 p1, t2 p2), (p1, p2))
  #define DEF_FN4(fn, t1,p1, t2,p2, t3,p3, t4,p4) DEF_FN(fn, (t1 p1, t2 p2, t3 p3, t4 p4), (p1, p2, p3, p4))
  #define DEF_FN5(fn, t1,p1, t2,p2, t3,p3, t4,p4, t5,p5) DEF_FN(fn, (t1 p1, t2 p2, t3 p3, t4 p4, t5 p5), (p1, p2, p3, p4, p5))

  DEF_FN5(REGISTER, UInt,bufferSize, UInt,maxLogicalConnection, UInt,maxBDataBlocks, UInt,maxBDataLen, ApplID*,pApplID);
  DEF_FN1(RELEASE, ApplID,applId);
  DEF_FN2(PUT_MESSAGE, ApplID,applId, void *,pCAPIMessage);
  DEF_FN2(GET_MESSAGE, ApplID,applId, void **,ppCAPIMessage);
  DEF_FN1(WAIT_FOR_SIGNAL, ApplID,applId);
  DEF_FN1(GET_MANUFACTURER, char *,szBuffer);
  DEF_FN4(GET_VERSION, UInt*,pCAPIMajor, UInt*,pCAPIMinor, UInt*,pManufacturerMajor, UInt*,pManufacturerMinor);
  DEF_FN1(GET_SERIAL_NUMBER, char*,szBuffer);
  DEF_FN2(GET_PROFILE, void*,pBuffer, UInt,CtrlNr);
  DEF_FN(INSTALLED, (), ());

#else

// Assume Linux
#include <pthread.h>
#include <semaphore.h>

class Mutex
{
private:
  pthread_mutex_t m_Mutex;
public:
  inline Mutex()       { pthread_mutex_init(&m_Mutex, NULL); }
  inline ~Mutex()      { pthread_mutex_destroy(&m_Mutex); }
  inline void Lock()   { pthread_mutex_lock(&m_Mutex); }
  inline void Unlock() { pthread_mutex_unlock(&m_Mutex); }
};

class Semaphore
{
private:
  sem_t m_Semaphore;
public:
  inline Semaphore(unsigned count = 1) { sem_init(&m_Semaphore, 0, count); }
  inline ~Semaphore()                  { sem_destroy(&m_Semaphore); }
  inline void Signal()                 { sem_post(&m_Semaphore); }
  inline bool Wait(unsigned timeout)
  {
      // Apologies to Posix enthusiasts, but this is ridiculous!
      struct timespec when;
      clock_gettime(CLOCK_REALTIME, &when);
      when.tv_nsec += timeout*1000000;
      if (when.tv_nsec >= 1000000000) {
        when.tv_nsec -= 1000000000;
        when.tv_nsec++;
      }
      return sem_timedwait(&m_Semaphore, &when) == 0;
  }
};


#include <sys/types.h>
#include <capi20.h>

class CAPI
{
public:
  typedef unsigned Result;
  typedef unsigned ApplID;
  typedef unsigned UInt;
  enum {
    InvalidApplId = 0
  };

  Result REGISTER(UInt bufferSize, UInt maxLogicalConnection, UInt maxBDataBlocks, UInt maxBDataLen, ApplID* pApplID)
  {
    return capi20_register(maxLogicalConnection, maxBDataBlocks, maxBDataLen, pApplID);
  }

  Result RELEASE(ApplID ApplID)
  {
    return capi20_release(ApplID);
  }

  Result PUT_MESSAGE(ApplID ApplID, void * pCAPIMessage)
  {
    return capi20_put_message(ApplID, (unsigned char *)pCAPIMessage);
  }

  Result GET_MESSAGE(ApplID ApplID, void ** ppCAPIMessage)
  {
    return capi20_get_message(ApplID, (unsigned char **)ppCAPIMessage);
  }

  Result WAIT_FOR_SIGNAL(ApplID ApplID)
  {
    return capi20_waitformessage(ApplID, NULL);
  }

  Result GET_MANUFACTURER(char * szBuffer)
  {
    capi20_get_manufacturer(0, (unsigned char *)szBuffer);
  }

  Result GET_VERSION(UInt *pCAPIMajor, UInt *pCAPIMinor, UInt *pManufacturerMajor, UInt *pManufacturerMinor)
  {
    unsigned char buffer[4*sizeof(uint32_t)];
    if (capi20_get_version(0, buffer) == NULL)
      return 0x100;

    if (pCAPIMajor) *pCAPIMajor = (UInt)(((uint32_t *)buffer)[0]);
    if (pCAPIMinor) *pCAPIMinor = (UInt)(((uint32_t *)buffer)[1]);
    if (pManufacturerMajor) *pManufacturerMajor = (UInt)(((uint32_t *)buffer)[2]);
    if (pManufacturerMinor) *pManufacturerMinor = (UInt)(((uint32_t *)buffer)[3]);

    return 0;
  }

  Result GET_SERIAL_NUMBER(char * szBuffer)
  {
      return capi20_get_serial_number(0, (unsigned char *)szBuffer) == NULL ? 0x100 : 0;
  }

  Result GET_PROFILE(void * pBuffer, UInt CtrlNr)
  {
    return capi20_get_profile(CtrlNr, (unsigned char *)pBuffer);
  }

  Result INSTALLED()
  {
    return capi20_isinstalled();
  }

#endif

  // Rest of CAPI class definition is outside of platform dependent bit
  // For portability reasons we have to define the message structures ourselves!

  #pragma pack(1)

  struct Message {
    uint16_t m_Length;
    uint16_t m_ApplId;
    uint8_t  m_Command;
    uint8_t  m_Subcommand;
    uint16_t m_Number;

    union Params {
      struct ListenReq {
        uint32_t m_Controller;
        uint32_t m_InfoMask;
        uint32_t m_CIPMask;
        uint32_t m_CIPMask2;
      } listen_req;

      struct ListenConf {
        uint32_t m_Controller;
        uint16_t m_Info;
      } listen_conf;

      struct ConnectReq {
        uint32_t m_Controller;
        uint16_t m_CIPValue;
      } connect_req;

      struct ConnectConf {
        uint32_t m_PLCI;
        uint16_t m_Info;
      } connect_conf;

      struct ConnectInd {
        uint32_t m_PLCI;
        uint16_t m_CIPValue;
      } connect_ind;

      struct ConnectResp {
        uint32_t m_PLCI;
        uint16_t m_Reject;
      } connect_resp;

      uint8_t  m_Params[200]; // Space for command specific parameters
    } param;


    Message(ApplID applId, unsigned command, unsigned subcommand, size_t fixedParamSize)
      : m_Length(8+fixedParamSize)
      , m_ApplId((uint16_t)applId)
      , m_Command((uint8_t)command)
      , m_Subcommand((uint8_t)subcommand)
      , m_Number(0)
    {
      memset(&param, 0, sizeof(param));
    }

    void Add(const char * value, int length = -1)
    {
      if (length < 0)
        length = strlen(value);
      char * param = ((char *)this)+ m_Length;
      *param++ = length;
      if (length > 0)
        memcpy(param, value, length);
      m_Length += length+1;
    }
  };

  struct Profile {
    uint16_t m_NumControllers;     // # of installed Controllers
    uint16_t m_NumBChannels;       // # of supported B-channels
    uint32_t m_GlobalOpttions;     // Global options
    uint32_t m_B1ProtocolOptions;  // B1 protocol support
    uint32_t m_B2ProtocolOptions;  // B2 protocol support
    uint32_t m_B3ProtocolOptions;  // B3 protocol support
    uint8_t  m_Reserved[64];       // Make sure struct is big enough
  };

};


class MutexedSection
{
private:
  Mutex & m_Mutex;
public:
  MutexedSection(Mutex & mutex) : m_Mutex(mutex) { mutex.Lock(); }
  ~MutexedSection() { m_Mutex.Unlock(); }
};


static const char G711ULawMediaFmt[] = "G.711-uLaw-64k";


class Context
{
  protected:
    enum {
        MaxLineCount = 30,
        MaxBlockCount = 2,
        MaxBlockSize = 128
    };

    CAPI         m_CAPI;
    CAPI::ApplID m_ApplicationId;
    unsigned     m_ControllerNumber;
    unsigned     m_LineCount;
    Mutex        m_Mutex;
    Semaphore    m_ListenCompleted;

    // This is damned annoying, but I don't want to include the entire PWLib just for one thread!
#ifdef _WIN32
    HANDLE m_hThread;
    static void ThreadMainStatic(void * arg)
    {
        ((Context *)arg)->ThreadMain();
    }
    bool StartThread()
    {
      return (m_hThread = (HANDLE)_beginthread(ThreadMainStatic, 0, this)) != NULL;
    }
    void WaitForThreadExit()
    {
        WaitForSingleObject(m_hThread, INFINITE);
    }
    void YieldThread()
    {
        Sleep(10);
    }
#else
    pthread_t m_hThread;
    static void * ThreadMainStatic(void * arg)
    {
        ((Context *)arg)->ThreadMain();
        return 0;
    }
    bool StartThread()
    {
      return pthread_create(&m_hThread, NULL, ThreadMainStatic, this) == 0;
    }
    void WaitForThreadExit()
    {
      pthread_join(m_hThread, NULL);
    }
    void YieldThread()
    {
      static const struct timespec ten_milliseconds = { 0, 10000000 };
      nanosleep(&ten_milliseconds, NULL);
    }
#endif

    void ThreadMain()
    {
      while (m_ApplicationId != CAPI::InvalidApplId) {
          CAPI::Message * pMessage = NULL;    

        unsigned result = m_CAPI.WAIT_FOR_SIGNAL(m_ApplicationId);
        if (result == CapiNoError)
          result = m_CAPI.GET_MESSAGE(m_ApplicationId, (void **)&pMessage);

        switch (result) {
          case 0x1101 : // Illegal application number
            // Probably closing down as another thread set m_ApplicationId to CAPI::InvalidApplId
            return;

          case 0x1104 : // Queue is empty
            // Really should not have happened if WAIT_FOR_SIGNAL returned!!
            break;

          case CapiNoError :
            m_Mutex.Lock();
            switch (pMessage->m_Subcommand) {
              case CAPI_IND : // Indications from CAPI
                switch (pMessage->m_Command) {
                  case CAPI_CONNECT :
                    {
                      size_t line = GetFreeLine();
                      if (line < MaxLineCount)
                        m_Line[line].ConnectIndication(*pMessage);
                      else
                        SendConnectResponse(pMessage->param.connect_ind.m_PLCI, 4); // Reject, circuit/channel not available
                    }
                    break;
                }
                break;

              case CAPI_CONF : // Confirmations of requests we have sent
                switch (pMessage->m_Command) {
                  case CAPI_LISTEN :
                    if (pMessage->param.listen_conf.m_Info == CapiNoError)
                      m_ControllerNumber = pMessage->param.listen_conf.m_Controller;
                    m_ListenCompleted.Signal();
                    break;

                  case CAPI_CONNECT :
                    m_Line[pMessage->m_Number].ConnectConf(*pMessage);
                    break;
                }
                break;
            }
            m_Mutex.Unlock();
            break;
        }
      }
    }

    struct Line
    {
      enum {
        e_Idle,
        e_Incoming,
        e_Outgoing,
        e_BearerUp
      }         m_State;
      uint32_t  m_PLCI;
      uint32_t  m_NCCI;
      Semaphore m_DialCompleted;

      Line()
        : m_State(e_Idle)
        , m_PLCI(0)
        , m_NCCI(0)
      {
      }

      void ConnectIndication(const CAPI::Message & message)
      {
          m_PLCI = message.param.connect_ind.m_PLCI;
          m_State = e_Incoming;
      }

      void ConnectConf(const CAPI::Message & message)
      {
        if (message.param.connect_conf.m_Info != CapiNoError)
          m_State = e_Idle;
        else
          m_PLCI = message.param.connect_conf.m_PLCI;
      }

      bool Disconnect()
      {
        if (m_NCCI != 0) {
        }
        if (m_PLCI != 0) {
        }
        return true;
      }
    } m_Line[MaxLineCount];


    size_t GetFreeLine() const
    {
      for (size_t i = 0; i < MaxLineCount; i++) {
        if (m_Line[i].m_State == Line::e_Idle)
          return i;
      }
      return MaxLineCount;
    }

    bool SendConnectResponse(uint32_t plci, uint16_t reason)
    {
      CAPI::Message message(m_ApplicationId, CAPI_CONNECT, CAPI_RESP, sizeof(CAPI::Message::Params::ConnectResp));
      message.param.connect_resp.m_PLCI = plci;
      message.param.connect_resp.m_Reject = reason;
      message.Add(NULL, 0); // B protocol
      message.Add(""); // Connected party number
      message.Add(""); // Connected party subaddress
      message.Add(NULL, 0); // Low Layer Compatibility
      message.Add(NULL, 0); // Additional Info

      return m_CAPI.PUT_MESSAGE(m_ApplicationId, &message) == CapiNoError;
    }


  public:
    PLUGIN_LID_CTOR()
    {
      m_ApplicationId = CAPI::InvalidApplId;
      m_ControllerNumber = 0;
      m_LineCount = 0;
    }

    PLUGIN_LID_DTOR()
    {
      Close();
    }


    PLUGIN_FUNCTION_ARG3(GetDeviceName,unsigned,index, char *,name, unsigned,size)
    {
      if (name == NULL || size == 0)
        return PluginLID_InvalidParameter;

      CAPI::Profile profile;
      if (m_CAPI.GET_PROFILE(&profile, 0) != CapiNoError)
        return PluginLID_InternalError;

      if (index >= profile.m_NumControllers)
        return PluginLID_NoMoreNames;

      if (size < 3)
        return PluginLID_BufferTooSmall;

      sprintf(name, "%u", index+1);
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG1(Open,const char *,device)
    {
      Close();

      int controller = atoi(device);
      if (controller <= 0)
        return PluginLID_NoSuchDevice;

      CAPI::Profile profile;
      if (m_CAPI.GET_PROFILE(&profile, controller) != CapiNoError)
        return PluginLID_NoSuchDevice;

      m_LineCount = profile.m_NumBChannels;

      if (m_CAPI.REGISTER(MaxLineCount*MaxBlockCount*MaxBlockSize+1024,
                          MaxLineCount,
                          MaxBlockCount,
                          MaxBlockSize,
                          &m_ApplicationId) != CapiNoError)
        return PluginLID_InternalError;

      if (!StartThread())
        return PluginLID_InternalError;

      // Start listening for incoming calls
      CAPI::Message message(m_ApplicationId, CAPI_LISTEN, CAPI_REQ, sizeof(CAPI::Message::Params::ListenReq));
      message.param.listen_req.m_Controller = controller;
      message.param.listen_req.m_InfoMask = 0;
      message.param.listen_req.m_CIPMask = 0xFFF81FF;
      message.param.listen_req.m_CIPMask2 = 0;
      message.Add(""); // Calling party number
      message.Add(""); // Calling party subaddress

      if (m_CAPI.PUT_MESSAGE(m_ApplicationId, &message) != CapiNoError) {
        Close();
        return PluginLID_InternalError;
      }

      // Wait for listen command to complete, one way or t'other, should be done inside a few seconds
      m_ListenCompleted.Wait(5000);

      return m_ControllerNumber != 0 ? PluginLID_NoError : PluginLID_InternalError;
    }


    PLUGIN_FUNCTION_ARG0(Close)
    {
      m_LineCount = 0;
      m_ControllerNumber = 0;

      if (m_ApplicationId != CAPI::InvalidApplId) {
        CAPI::ApplID oldId = m_ApplicationId;
        m_ApplicationId = CAPI::InvalidApplId;
        m_CAPI.RELEASE(oldId);
        WaitForThreadExit();
      }

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG1(GetLineCount, unsigned *,count)
    {
      if (count == NULL)
        return PluginLID_InvalidParameter;

      if (m_ControllerNumber == 0)
        return PluginLID_DeviceNotOpen;

      *count = m_LineCount;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(IsLineTerminal, unsigned,line, PluginLID_Boolean *,isTerminal)
    {
      if (isTerminal == NULL)
        return PluginLID_InvalidParameter;

      if (m_ControllerNumber == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      *isTerminal = FALSE;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG3(IsLinePresent, unsigned,line, PluginLID_Boolean,forceTest, PluginLID_Boolean *,present)
    {
      if (present == NULL)
        return PluginLID_InvalidParameter;

      if (m_ControllerNumber == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      *present = TRUE;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(IsLineOffHook, unsigned,line, PluginLID_Boolean *,offHook)
    {
      if (offHook == NULL)
        return PluginLID_InvalidParameter;

      if (m_ControllerNumber == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      m_Mutex.Lock();
      *offHook = m_Line[line].m_State != Line::e_Idle;
      m_Mutex.Unlock();

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetLineOffHook, unsigned,line, PluginLID_Boolean,newState)
    {
      if (m_ControllerNumber == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      bool ok;

      m_Mutex.Lock();
      switch (m_Line[line].m_State)
      {
        case Line::e_Incoming :
          ok = SendConnectResponse(m_Line[line].m_PLCI, newState ? 0 : 1); // Answer/Reject, circuit/channel not available
          break;

        default :
            ok = false;
      }

      m_Mutex.Unlock();

      return ok ? PluginLID_NoError : PluginLID_InternalError;
    }


    //PLUGIN_FUNCTION_ARG2(HookFlash, unsigned,line, unsigned,flashTime)
    //PLUGIN_FUNCTION_ARG2(HasHookFlash, unsigned,line, PluginLID_Boolean *,flashed)

    PLUGIN_FUNCTION_ARG2(IsLineRinging, unsigned,line, unsigned long *,cadence)
    {
      if (cadence == NULL)
        return PluginLID_InvalidParameter;

      if (m_ControllerNumber == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      m_Mutex.Lock();
      *cadence = m_Line[line].m_State == Line::e_Incoming ? 1 : 0;
      m_Mutex.Unlock();

      return PluginLID_NoError;
    }

    //PLUGIN_FUNCTION_ARG4(RingLine, unsigned,line, unsigned,nCadence, const unsigned *,pattern, unsigned,frequency)
    
    PLUGIN_FUNCTION_ARG3(IsLineDisconnected, unsigned,line, PluginLID_Boolean,checkForWink, PluginLID_Boolean *,disconnected)
    {
      if (disconnected == NULL)
        return PluginLID_InvalidParameter;

      if (m_ControllerNumber == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      m_Mutex.Lock();
      *disconnected = m_Line[line].m_State != Line::e_BearerUp;
      m_Mutex.Unlock();

      return PluginLID_NoError;
    }

    //PLUGIN_FUNCTION_ARG3(SetLineToLineDirect, unsigned,line1, unsigned,line2, PluginLID_Boolean,connect)
    //PLUGIN_FUNCTION_ARG3(IsLineToLineDirect, unsigned,line1, unsigned,line2, PluginLID_Boolean *,connected)


    PLUGIN_FUNCTION_ARG3(GetSupportedFormat, unsigned,index, char *,mediaFormat, unsigned,size)
    {
      if (mediaFormat == NULL || size == 0)
        return PluginLID_InvalidParameter;

      if (index > 0)
        return PluginLID_NoMoreNames;

      if (size < sizeof(G711ULawMediaFmt))
        return PluginLID_BufferTooSmall;

      strcpy(mediaFormat, G711ULawMediaFmt);
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetReadFormat, unsigned,line, const char *,mediaFormat)
    {
      if (mediaFormat == NULL)
        return PluginLID_InvalidParameter;

      if (m_ControllerNumber == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      return strcmp(mediaFormat, G711ULawMediaFmt) == 0 ? PluginLID_NoError : PluginLID_UnsupportedMediaFormat;
    }


    PLUGIN_FUNCTION_ARG2(SetWriteFormat, unsigned,line, const char *,mediaFormat)
    {
      if (mediaFormat == NULL)
        return PluginLID_InvalidParameter;

      if (m_ControllerNumber == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      return strcmp(mediaFormat, G711ULawMediaFmt) == 0 ? PluginLID_NoError : PluginLID_UnsupportedMediaFormat;
    }


    PLUGIN_FUNCTION_ARG3(GetReadFormat, unsigned,line, char *,mediaFormat, unsigned,size)
    {
      if (mediaFormat == NULL || size == 0)
        return PluginLID_InvalidParameter;

      if (m_ControllerNumber == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      if (size < sizeof(G711ULawMediaFmt))
        return PluginLID_BufferTooSmall;

      strcpy(mediaFormat, G711ULawMediaFmt);
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG3(GetWriteFormat, unsigned,line, char *,mediaFormat, unsigned,size)
    {
      if (mediaFormat == NULL || size == 0)
        return PluginLID_InvalidParameter;

      if (m_ControllerNumber == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      if (size < sizeof(G711ULawMediaFmt))
        return PluginLID_BufferTooSmall;

      strcpy(mediaFormat, G711ULawMediaFmt);
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG1(StopReading, unsigned,line)
    {
      if (m_ControllerNumber == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG1(StopWriting, unsigned,line)
    {
      if (m_ControllerNumber == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetReadFrameSize, unsigned,line, unsigned,frameSize)
    {
      if (m_ControllerNumber == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(SetWriteFrameSize, unsigned,line, unsigned,frameSize)
    {
      if (m_ControllerNumber == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(GetReadFrameSize, unsigned,line, unsigned *,frameSize)
    {
      if (frameSize == NULL)
        return PluginLID_InvalidParameter;

      if (m_ControllerNumber == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      *frameSize = MaxBlockSize;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG2(GetWriteFrameSize, unsigned,line, unsigned *,frameSize)
    {
      if (frameSize == NULL)
        return PluginLID_InvalidParameter;

      if (m_ControllerNumber == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      *frameSize = MaxBlockSize;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG3(ReadFrame, unsigned,line, void *,buffer, unsigned *,count)
    {
      if (buffer == NULL || count == NULL)
        return PluginLID_InvalidParameter;

      if (m_ControllerNumber == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      *count = MaxBlockSize;
      return PluginLID_NoError;
    }


    PLUGIN_FUNCTION_ARG4(WriteFrame, unsigned,line, const void *,buffer, unsigned,count, unsigned *,written)
    {
      if (buffer == NULL || written == NULL || count != MaxBlockSize)
        return PluginLID_InvalidParameter;

      if (m_ControllerNumber == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      *written = MaxBlockSize;
      return PluginLID_NoError;
    }



    //PLUGIN_FUNCTION_ARG3(GetAverageSignalLevel, unsigned,line, PluginLID_Boolean,playback, unsigned *,signal)
    //PLUGIN_FUNCTION_ARG2(EnableAudio, unsigned,line, PluginLID_Boolean,enable)
    //PLUGIN_FUNCTION_ARG2(IsAudioEnabled, unsigned,line, PluginLID_Boolean *,enable)
    //PLUGIN_FUNCTION_ARG2(SetRecordVolume, unsigned,line, unsigned,volume)
    //PLUGIN_FUNCTION_ARG2(SetPlayVolume, unsigned,line, unsigned,volume)
    //PLUGIN_FUNCTION_ARG2(GetRecordVolume, unsigned,line, unsigned *,volume)
    //PLUGIN_FUNCTION_ARG2(GetPlayVolume, unsigned,line, unsigned *,volume)
    //PLUGIN_FUNCTION_ARG2(GetAEC, unsigned,line, unsigned *,level)
    //PLUGIN_FUNCTION_ARG2(SetAEC, unsigned,line, unsigned,level)
    //PLUGIN_FUNCTION_ARG2(GetVAD, unsigned,line, PluginLID_Boolean *,enable)
    //PLUGIN_FUNCTION_ARG2(SetVAD, unsigned,line, PluginLID_Boolean,enable)
    //PLUGIN_FUNCTION_ARG4(GetCallerID, unsigned,line, char *,idString, unsigned,size, PluginLID_Boolean,full)
    //PLUGIN_FUNCTION_ARG2(SetCallerID, unsigned,line, const char *,idString)
    //PLUGIN_FUNCTION_ARG2(SendVisualMessageWaitingIndicator, unsigned,line, PluginLID_Boolean,on)
    //PLUGIN_FUNCTION_ARG4(PlayDTMF, unsigned,line, const char *,digits, unsigned,onTime, unsigned,offTime)
    //PLUGIN_FUNCTION_ARG2(ReadDTMF, unsigned,line, char *,digit)
    //PLUGIN_FUNCTION_ARG2(GetRemoveDTMF, unsigned,line, PluginLID_Boolean *,removeTones)
    //PLUGIN_FUNCTION_ARG2(SetRemoveDTMF, unsigned,line, PluginLID_Boolean,removeTones)

    PLUGIN_FUNCTION_ARG2(IsToneDetected, unsigned,line, int *,tone)
    {
      if (tone == NULL)
        return PluginLID_InvalidParameter;

      if (m_ControllerNumber == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      *tone = PluginLID_NoTone;

      return PluginLID_NoError;
    }

    //PLUGIN_FUNCTION_ARG3(WaitForToneDetect, unsigned,line, unsigned,timeout, int *,tone)
    //PLUGIN_FUNCTION_ARG3(WaitForTone, unsigned,line, int,tone, unsigned,timeout)
    //PLUGIN_FUNCTION_ARG7(SetToneFilterParameters, unsigned        ,line,
    //                                              unsigned        ,tone,
    //                                              unsigned        ,lowFrequency,
    //                                              unsigned        ,highFrequency,
    //                                              unsigned        ,numCadences,
    //                                              const unsigned *,onTimes,
    //                                              const unsigned *,offTimes)
    //PLUGIN_FUNCTION_ARG2(PlayTone, unsigned,line, unsigned,tone)
    //PLUGIN_FUNCTION_ARG2(IsTonePlaying, unsigned,line, PluginLID_Boolean *,playing)
    //PLUGIN_FUNCTION_ARG1(StopTone, unsigned,line)

    PLUGIN_FUNCTION_ARG3(DialOut, unsigned,line, const char *,number, struct PluginLID_DialParams *,params)
    {
      if (number == NULL)
        return PluginLID_InvalidParameter;

      if (m_ControllerNumber == 0)
        return PluginLID_DeviceNotOpen;

      if (line >= m_LineCount)
        return PluginLID_NoSuchLine;

      bool ok = false;

      m_Mutex.Lock();

      if (m_Line[line].m_State == Line::e_Idle) {
        m_Line[line].m_State = Line::e_Outgoing;

        // Start connect for outgoing call
        CAPI::Message message(m_ApplicationId, CAPI_CONNECT, CAPI_REQ, sizeof(CAPI::Message::Params::ConnectReq));
        message.m_Number = line;
        message.param.connect_req.m_Controller = m_ControllerNumber;
        message.param.connect_req.m_CIPValue = 1; // G.711 speach
        message.Add(number); // Called party number
        message.Add(""); // Calling party number
        message.Add(""); // Called party subaddress
        message.Add(""); // Calling party subaddress
        message.Add(NULL, 0); // B protocol
        message.Add(NULL, 0); // Bearer Capabilities
        message.Add(NULL, 0); // Low Layer Compatibility
        message.Add(NULL, 0); // High Layer Compatibility
        message.Add(NULL, 0); // Additional Info

        if (m_CAPI.PUT_MESSAGE(m_ApplicationId, &message) != CapiNoError)
          m_Line[line].m_State = Line::e_Idle;
        else {
          m_Line[line].m_DialCompleted.Wait(params->m_progressTimeout);
          ok = m_Line[line].m_State == Line::e_BearerUp;
          if (!ok)
            m_Line[line].Disconnect();
        }
      }

      m_Mutex.Unlock();

      return ok ? PluginLID_NoError : PluginLID_InternalError;
    }

    //PLUGIN_FUNCTION_ARG2(GetWinkDuration, unsigned,line, unsigned *,winkDuration)
    //PLUGIN_FUNCTION_ARG2(SetWinkDuration, unsigned,line, unsigned,winkDuration)
    //PLUGIN_FUNCTION_ARG1(SetCountryCode, unsigned,country)
    //PLUGIN_FUNCTION_ARG2(GetSupportedCountry, unsigned,index, unsigned *,countryCode)
};



/////////////////////////////////////////////////////////////////////////////

static struct PluginLID_Definition definition[1] =
{

  { 
    // encoder
    PLUGIN_LID_VERSION,               // API version

    1083666706,                       // timestamp = Tue 04 May 2004 10:31:46 AM UTC = 

    "CAPI",                           // LID name text
    "Common-ISDN API",                // LID description text
    "Generic",                        // LID manufacturer name
    "Generic",                        // LID model name
    "1.0",                            // LID hardware revision number
    "",                               // LID email contact information
    "",                               // LID web site

    "Robert Jongbloed, Post Increment",                          // source code author
    "robertj@postincrement.com",                                 // source code email
    "http://www.postincrement.com",                              // source code URL
    "Copyright (C) 2006 by Post Increment, All Rights Reserved", // source code copyright
    "MPL 1.0",                                                   // source code license
    "1.0",                                                       // source code version

    NULL,                             // user data value

    Context::Create,
    Context::Destroy,
    Context::GetDeviceName,
    Context::Open,
    Context::Close,
    Context::GetLineCount,
    Context::IsLineTerminal,
    Context::IsLinePresent,
    Context::IsLineOffHook,
    Context::SetLineOffHook,
    NULL, //Context::HookFlash,
    NULL, //Context::HasHookFlash,
    Context::IsLineRinging,
    NULL, //Context::RingLine,
    Context::IsLineDisconnected,
    NULL, //Context::SetLineToLineDirect,
    NULL, //Context::IsLineToLineDirect,
    Context::GetSupportedFormat,
    Context::SetReadFormat,
    Context::SetWriteFormat,
    Context::GetReadFormat,
    Context::GetWriteFormat,
    Context::StopReading,
    Context::StopWriting,
    Context::SetReadFrameSize,
    Context::SetWriteFrameSize,
    Context::GetReadFrameSize,
    Context::GetWriteFrameSize,
    Context::ReadFrame,
    Context::WriteFrame,
    NULL,//Context::GetAverageSignalLevel,
    NULL,//Context::EnableAudio,
    NULL,//Context::IsAudioEnabled,
    NULL,//Context::SetRecordVolume,
    NULL,//Context::SetPlayVolume,
    NULL,//Context::GetRecordVolume,
    NULL,//Context::GetPlayVolume,
    NULL,//Context::GetAEC,
    NULL,//Context::SetAEC,
    NULL,//Context::GetVAD,
    NULL,//Context::SetVAD,
    NULL,//Context::GetCallerID,
    NULL,//Context::SetCallerID,
    NULL,//Context::SendVisualMessageWaitingIndicator,
    NULL,//Context::PlayDTMF,
    NULL,//Context::ReadDTMF,
    NULL,//Context::GetRemoveDTMF,
    NULL,//Context::SetRemoveDTMF,
    NULL,//Context::IsToneDetected,
    NULL,//Context::WaitForToneDetect,
    NULL,//Context::WaitForTone,
    NULL,//Context::SetToneFilterParameters,
    NULL,//Context::PlayTone,
    NULL,//Context::IsTonePlaying,
    NULL,//Context::StopTone,
    Context::DialOut,
    NULL,//Context::GetWinkDuration,
    NULL,//Context::SetWinkDuration,
    NULL,//Context::SetCountryCode,
    NULL,//Context::GetSupportedCountry
  }

};

PLUGIN_LID_IMPLEMENTATION(definition);

/////////////////////////////////////////////////////////////////////////////
