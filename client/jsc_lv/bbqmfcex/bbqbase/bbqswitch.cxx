
#include "bbqbase.h"

#include "bbqswitch.h"
BBQSwitch::BBQSwitch()
{
#ifdef D_SUPPORT_ARRAY_OPTIMZE
	m_nBase = m_nMax = 0;
	m_pAddrLib = NULL;
#endif
}

BBQSwitch::~BBQSwitch()
{
	LockIt safe( m_mutex );
#ifdef D_SUPPORT_ARRAY_OPTIMZE

	if( m_pAddrLib ) {
		delete[] m_pAddrLib;
		m_nBase = m_nMax = 0;
	}
#endif
	for (SwitchInfoMap::iterator itor = m_exptAddrLib.begin(); itor!=  m_exptAddrLib.end();++itor)
		delete itor->second;

	m_exptAddrLib.clear();
}

bool BBQSwitch::Initialize( uint32 nMin, uint32 nMax )
{
	LockIt safe( m_mutex );
	m_exptAddrLib.clear();
#ifdef D_SUPPORT_ARRAY_OPTIMZE

	if( m_pAddrLib ) {
		delete[] m_pAddrLib;
		m_nBase = m_nMax = 0;
	}
	uint32 n = min( nMin, nMax );
	uint32 m = max( nMin, nMax );
	uint32 nSize = (m > n) ? (m - n) : 1;

	m_pAddrLib = new SWITCHINFO[ nSize ];
	if( m_pAddrLib ) {
		memset( m_pAddrLib, 0, sizeof(SWITCHINFO) * nSize );
		m_nBase = n;
		m_nMax = m; 

	}else
		return false;

#endif
		return true;
}

bool BBQSwitch::IsInitialized( void )
{
	LockIt safe( m_mutex );
#ifdef D_SUPPORT_ARRAY_OPTIMZE

	return (m_pAddrLib != NULL);
#else
	return true;
#endif
}

bool BBQSwitch::Put( uint32 id, SWITCHINFO & info )
{
	LockIt safe( m_mutex );

	if( ! IsInitialized() ) return false;

	//SWITCHINFO	oldInfo;
#ifdef D_SUPPORT_ARRAY_OPTIMZE
	if( id >= m_nBase && id < m_nMax ) {
		uint32 index = id - m_nBase;
		oldInfo = m_pAddrLib[ index ];

		if( info.logout || info.dropoffline ) {
			// if record not match, maybe already updated, so ignore
			if( oldInfo.addr.ip != info.addr.ip ) return false;
		}

		m_pAddrLib[ index ] = info;
	} else 
#endif
	{
		SwitchInfoMap::iterator it = m_exptAddrLib.find( id );
		if( it != m_exptAddrLib.end() ) {
			//memcpy(&oldInfo , it->second, sizeof(oldInfo));

			//if( info.logout || info.dropoffline ) {
			//	// record not match, maybe already updated, so ignore
			//	if( oldInfo.addr.ip != info.addr.ip ) return false;
			//}

			memcpy(it->second, &info, sizeof(info));
		} else {
			//memset( & oldInfo, 0, sizeof(oldInfo) );
			SwitchUserEntry* pNewEntry =new SwitchUserEntry();
			 pNewEntry->info = info;
			m_exptAddrLib[ id ] = pNewEntry;
		}
	}

	// now, update the loading stat 

	//if( oldInfo.addr.ip != 0 ) {
	//	LoadingInfoMap::iterator it = m_mapUserNumbers.find( oldInfo.addr.ip );
	//	if( it != m_mapUserNumbers.end() ) {
	//		if( it->second > 0 ) {
	//			if( oldInfo.login ) it->second --;
	//		}
	//	} else {
	//		m_mapUserNumbers[ oldInfo.addr.ip ] = 0;
	//	}
	//}

	//if( info.addr.ip != 0 ) {
	//	LoadingInfoMap::iterator it = m_mapUserNumbers.find( info.addr.ip );
	//	if( it != m_mapUserNumbers.end() ) {
	//		if( it->second >= 0 ) {
	//			if( info.login ) it->second ++;
	//		}
	//	} else {
	//		m_mapUserNumbers[ info.addr.ip ] = info.login ? 1 : 0;
	//	}
	//}

	return true;
}

bool BBQSwitch::Get( uint32 id, SWITCHINFO & info )
{
	LockIt safe( m_mutex );

	if( ! IsInitialized() ) return false;
#ifdef D_SUPPORT_ARRAY_OPTIMZE
	if( id >= m_nBase && id < m_nMax ) {
		uint32 index = id - m_nBase;
		info = m_pAddrLib[ index ];
		return (info.addr.ip != 0);
	} else
#endif
	{
		SwitchInfoMap::iterator iter = m_exptAddrLib.find( id );
		if( iter != m_exptAddrLib.end() ) {
			memcpy(&info , iter->second, sizeof(info));
			return true;
		}
	}

	return false;
}
