#pragma once
#include "..\CppSQLite3_2\Common\CppSQLite3.h"
#include <ptlib.h>
#include "bbqbase.h"
class CSQLDb
{
public:
  CSQLDb( );
  ~CSQLDb(void);
public:
  bool Open(unsigned int uid);
  bool SaveIM(const BBQOfflineIMMessage* pIM, const PString& strIMValue, bool bSenderOrReceiver );
  bool GetIM(int nStart, int nEnd, int uIDChatWithMe  ,PString& strIMValue/* out*/);
protected:
  CppSQLite3DB m_db;
  PMutex m_mxDB;
  int m_nOwerUID;
  unsigned m_uidfrom, m_uidto,m_send_time;
};
