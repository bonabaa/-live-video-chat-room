
#ifndef _BBQ_SWITCH_H
#define _BBQ_SWITCH_H

#include <ptlib.h>

#include "bbqidmsg.h"
#include "msgterminal.h"
#include "bbqidrecord.h"
#include "bbqdatabase.h"
#include "md5.h"
typedef std::map<uint32, uint32/*status*/>	UserBuddysInfo;//userid --> status
typedef std::map<uint32, UserBuddysInfo>	DomainBuddysInfo;//domainid --> userid
// bbq switch is a place to exchange location information
//#define D_SUPPORT_ARRAY_OPTIMZE 
struct SwitchUserEntry
{
	SWITCHINFO info;
	//domain id
	DWORD m_domainID;
	//online buddys
	DomainBuddysInfo m_OnlineBuddys; 
};
typedef std::map<uint32, SwitchUserEntry*>	SwitchInfoMap;

class BBQSwitch
{
public:
	mutable PMutex		m_mutex;

#ifdef D_SUPPORT_ARRAY_OPTIMZE
	// most in this range, in this array
	uint32				m_nBase;
	uint32				m_nMax;
	SWITCHINFO *		m_pAddrLib;
#endif
	// some special out of range in this mapping
	SwitchInfoMap		m_exptAddrLib;

	// loading, map IP to user numbers
	//typedef std::map<uint32, uint32>		LoadingInfoMap;
	//LoadingInfoMap		m_mapUserNumbers;

public:
	BBQSwitch();
	~BBQSwitch();

	bool Initialize( uint32 nMin, uint32 nMax );
	bool IsInitialized( void );

	bool Put( uint32 id, SWITCHINFO & info );
	bool Get( uint32 id, SWITCHINFO & info );
};

#endif // _BBQ_SWITCH_H
