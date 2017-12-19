
#ifndef _IP_TO_COUNTRY_H_
#define _IP_TO_COUNTRY_H_

#include <ptlib.h>

#include "os_related.h"

#pragma pack(1)

union CountryCode {
	uint32		nCode;
	char		szCode[4];
};

struct IpCountryRecord {
	uint32		ipFrom;
	uint32		ipTo;
	CountryCode	country;
};

struct CountryRecord {
	CountryCode	country;
	char		abbrName[ 4 ];
	char		fullName[ 64 ];
};

#define		CSV_KEYS_MAX	12

typedef struct CSV_FORMAT_ORDER {
	int		nCount;
	int		nOrders[ CSV_KEYS_MAX ];
} CSV_FORMAT_ORDER;

#pragma pack()

#include <map>

typedef std::map<uint32, CountryRecord *>	CountryMap;

class Ip2CountryUtility
{
public:
	Ip2CountryUtility();
	~Ip2CountryUtility();

	typedef enum {
		Okay = 0,
		FileError = 1,
		InvalidFormat = 2,
		UnknownError = 3,
	} ErrCode;

	int		ImportCSV( const char * lpszCSVFile );
	int		ExportCSV( const char * lpszCSVFile );

	int		ImportCountryInfo( const char * lpszCSVFile );
	int		ExportCountryInfo( const char * lpszCSVFile );

	int		ImportBinI2C( const char * lpszI2CFile );
	int		ExportBinI2C( const char * lpszI2CFile );

	bool	Merge( Ip2CountryUtility & other );

	int		GetIp2CountryRecordCount();
	int		GetCountryRecordCount();

	uint32	Ip2Country( uint32 ip ); // the ip here must be network order

	bool	FindCountry( CountryRecord & record ); // fill countryCode before calling

protected:
	void	ResetI2C( void );
	void	ResetCountryInfo( void );

	bool	UpdateCountry( CountryRecord & record, bool bReplac = false );

	bool	LineToCsvFormat( char * szLine, CSV_FORMAT_ORDER & cfo );
	bool	IpCountryLineToRecord( char * szLine, CSV_FORMAT_ORDER & cfo, IpCountryRecord & icr, CountryRecord & cr );

	bool	CountryLineToRecord( char * szLine, CountryRecord & cr );

protected:
	mutable PMutex		m_mutexVars;

	int					m_nRecords;
	IpCountryRecord *	m_pRecords;				// must in sorted order

	CountryMap			m_mapCountries;
};

#endif // _IP_TO_COUNTRY_H_
