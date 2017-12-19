
#include "bbqbase.h"

#include "ip2country.h"

enum {
	N_IP_FROM		= 0,
	N_IP_TO			= 1,
	N_COUNTRY_CODE	= 2,
	N_COUNTRY_ABBR	= 3,
	N_COUNTRY_NAME	= 4,
};

const char * DEFAULT_CSV_FORMAT_KEYS[ CSV_KEYS_MAX ] = {
	"ipfrom",
	"ipto",
	"countrycode",
	"countryabbr",
	"countryname",
	"registry",
	"assigned",
	"region",
	"city",
	"isp",
	"latitude",
	"longitude",
};

#define	DEFAULT_CSV_FORMAT		"\"%u\",\"%u\",\"%2s\",\"%3s\",\"%s\"\r\n"
#define	DEFAULT_COUNTRY_FORMAT	"\"%2s\",\"%3s\",\"%s\"\r\n"

#define	MAJOR_V			0x1
#define	MINOR_V			0x0
#define	M_VERSION		((MAJOR_V << 16) | MINOR_V)

#define	MAGIC_STR		"IP TO COUNTRY"

typedef struct IP2COUNTRY_FILEHEADER 
{
	char			magic[ 32 ];	// valid file identifier, must be "IP TO COUNTRY"
	unsigned int	version;		// version of file format
	unsigned int	headerlength;	// the length of the file header
	unsigned int	records;		// total records contained in this file
} IP2COUNTRY_FILEHEADER;

Ip2CountryUtility::Ip2CountryUtility()
{
	m_pRecords = NULL;
	m_nRecords = 0;
}

Ip2CountryUtility::~Ip2CountryUtility()
{
	ResetI2C();
	ResetCountryInfo();
}

void Ip2CountryUtility::ResetI2C( void )
{
	LockIt safe( m_mutexVars );

	if( m_pRecords ) {
		delete m_pRecords;
		m_pRecords = NULL;
	}
	m_nRecords = 0;
}

void Ip2CountryUtility::ResetCountryInfo( void )
{
	LockIt safe( m_mutexVars );

	CountryMap::iterator eit = m_mapCountries.end();
	for( CountryMap::iterator it = m_mapCountries.begin(); it != eit; /**/ ) {
		CountryRecord * p = it->second;
		STL_ERASE( m_mapCountries, CountryMap::iterator, it );
		delete p;
	}
}

uint32	Ip2CountryUtility::Ip2Country( uint32 ip )
{
	LockIt safe( m_mutexVars );

	uint32 nCode = 0;
	int n = 0;

	int x = 0;
	int y = m_nRecords - 1;

	ip = ntohl(ip); // convert network order to host order

	while( x <= y ) {
		n ++;

		int z = (x + y) / 2;

		IpCountryRecord * p = & m_pRecords[z];

		// debug test only
		//printf( "%d, %x, [%x,%x], [%s, %s], %s\n", n, ip, p->ipFrom, p->ipTo, (const char *)BBQMsgTerminal::ToString( htonl(p->ipFrom) ),  (const char *)BBQMsgTerminal::ToString( htonl(p->ipTo) ), p->country.szCode );

		if( ip < p->ipFrom ) {
			y = z-1;
			continue;
		} else if ( ip > p->ipTo ) {
			x = z+1;
			continue;
		} else { // ipFrom <= ip <= ipTo, we found it
			return p->country.nCode;
		}
	}

	//printf( "end searching %s after comparing %d times.\n", (const char *)BBQMsgTerminal::ToString( htonl(ip) ), n );

	return nCode;
}

int	Ip2CountryUtility::GetIp2CountryRecordCount()
{
	return m_nRecords;
}

int	Ip2CountryUtility::GetCountryRecordCount()
{
	return m_mapCountries.size();
}

bool Ip2CountryUtility::UpdateCountry( CountryRecord & record, bool bReplace )
{
	CountryMap::iterator it = m_mapCountries.find( record.country.nCode );
	if( it != m_mapCountries.end() ) { // found 
		if( bReplace ) {
			* it->second = record;
		}
	} else { // not found
		CountryRecord * p = new CountryRecord;
		if( p ) {
			* p = record;
			m_mapCountries[ p->country.nCode ] = p;
		}
	}

	return true;
}

bool Ip2CountryUtility::FindCountry( CountryRecord & record )
{
	CountryMap::iterator it = m_mapCountries.find( record.country.nCode );
	if( it == m_mapCountries.end() ) { // not found
		return false;
	}

	record = * it->second;
	return true;
}

// "201674096","201674111","CA","CAN","CANADA"
bool Ip2CountryUtility::IpCountryLineToRecord( char * szLine, CSV_FORMAT_ORDER & cfo, IpCountryRecord & icr, CountryRecord & cr )
{
	char * items[ CSV_KEYS_MAX ];
	memset( & items[0], 0, sizeof(char*) * CSV_KEYS_MAX );

	char * p = szLine;

	// cut items to words
	int nCount, i;
	for( i=0; i<CSV_KEYS_MAX; /**/ ) {
		while( (*p!='"') && (*p!='\0') ) p++; 
		if(*p=='"') p++;
		else if(*p=='\0') break;
		items[ i++ ] = p;
		while( (*p!='"') && (*p!='\0') ) p++; 
		if(*p=='"') { *p='\0'; p++; }
	}

	nCount = i;
	if( cfo.nCount != nCount ) return false; // invalid format

	for( int i=0; i<CSV_KEYS_MAX; i++ ) {
		int n = cfo.nOrders[i];
		if( (0 <= n) && (n < nCount) ) {
			switch( i ) {
			case N_IP_FROM:	
				icr.ipFrom = (uint32) atof( items[n] ); 
				break;
			case N_IP_TO:	
				icr.ipTo = (uint32) atof( items[n] ); 
				break;
			case N_COUNTRY_CODE: 
				strncpy( icr.country.szCode, items[n], sizeof(icr.country) ); 
				icr.country.szCode[ sizeof(icr.country)-1 ] = '\0';
				cr.country.nCode = icr.country.nCode;
				break;
			case N_COUNTRY_ABBR: 
				strncpy( cr.abbrName, items[n], sizeof(cr.abbrName) );
				cr.abbrName[ sizeof(cr.abbrName) -1 ] = '\0';
				break;
			case N_COUNTRY_NAME: 
				strncpy( cr.fullName, items[n], sizeof(cr.fullName) );
				cr.fullName[ sizeof(cr.fullName) -1 ] = '\0';
				break;
			}
		} else {
			// the field does not exist, ignore
		}
	}

	return true;
}

// <ipfrom> <ipto> <registry> <assigned> <countrycode> <countryabbr> <countryname> <region> <city> <isp> <latitude> <longitude>
bool Ip2CountryUtility::LineToCsvFormat( char * szLine, CSV_FORMAT_ORDER & cfo )
{
	char * items[ CSV_KEYS_MAX ];
	memset( & items[0], 0, sizeof(char*) * CSV_KEYS_MAX );

	char * p = szLine;
	int i, j;

	// cut items to words
	for( j=0; j<CSV_KEYS_MAX; /**/ ) {
		while( (*p!='<') && (*p!='\0') ) p++; 
		if(*p=='<') p++;
		else if(*p=='\0') break;
		items[ j++ ] = p;
		while( (*p!='>') && (*p!='\0') ) p++; 
		if(*p=='>') { *p='\0'; p++; }
	}
	cfo.nCount = j;

	// get the order of keywords
	for( i=0; i<CSV_KEYS_MAX; i++ ) {
		cfo.nOrders[i] = -1;
		for( j=0; j<cfo.nCount; j++ ) {
			if( 0 == strcasecmp( DEFAULT_CSV_FORMAT_KEYS[i], items[j] ) ) {
				cfo.nOrders[i] = j;
				break;
			}
		}
	}

	return true;
}

int Ip2CountryUtility::ImportCSV( const char * lpszCSVFile )
{
	LockIt safe( m_mutexVars );

	ResetI2C();
	ResetCountryInfo();

	// init CSV format to default format, <ipfrom> <ipto> <countrycode> <countryabbr> <countryname>
	CSV_FORMAT_ORDER cfo;
	memset( & cfo, 0, sizeof(cfo) );
	cfo.nCount = 5;
	for( int i=0; i<cfo.nCount; i++ ) cfo.nOrders[i] = i;

	ErrCode ret = Okay;

	FILE * fp = fopen( lpszCSVFile, "rb" );
	if( fp ) {
		char szLine[ 256 ];

		// let's count how many lines
		int nLines = 0;
		while( fgets( szLine, 256, fp ) ) {
			if( szLine[0] == '"' ) { // lines begin with " is data
				nLines ++;
			} else if ( szLine[0] == '#' ) { // line begin with # is comment
				if( (strstr( szLine, "<ipfrom>" ) != NULL) && (strstr( szLine, "<ipto>" ) != NULL) ) {
					// get CSV file format
					LineToCsvFormat( szLine, cfo );
				}
			} else {
				// ignore other line
			}
		}

		if( nLines > 0 ) {
			m_pRecords = new IpCountryRecord[ nLines ];
			if( m_pRecords ) {
				memset( m_pRecords, 0, sizeof(IpCountryRecord) * nLines );
				m_nRecords = 0;

				IpCountryRecord icr, ok;
				memset( & icr, 0, sizeof(icr) );
				memset( & ok, 0, sizeof(ok) );

				CountryRecord cr;
				memset( & cr, 0, sizeof(cr) );

				rewind( fp );
				int i = 0;
				while( fgets( szLine, 255, fp ) ) {
					szLine[255] = '\0';
					if( szLine[0] == '"' ) { // only handle lines begin with "

						if( IpCountryLineToRecord( szLine, cfo, icr, cr ) ) {

							// check whether the data is sorted valid or not
							if( icr.ipFrom > icr.ipTo ) {
								// invalid line, skip
								continue;
							}

							if( icr.ipFrom < ok.ipTo ) {
								// small data appear after bigger one, invalid line, skip
								continue;
							}

							// the data is valid
							m_pRecords[ i ++ ] = ok = icr;
							UpdateCountry( cr );

						} else {
							// ret = InvalidFormat; break;
						}

						// debug test only
						//printf( "%s, %x, %s\n", icr.country.szCode, icr.country.nCode, cr.fullName );
						//break;
					}
				}

				m_nRecords = i;
			}
		} else {
			ret = InvalidFormat;
		}

		fclose(fp);
	} else {
		ret = FileError;
	}

	return ret;
}

int Ip2CountryUtility::ExportCSV( const char * lpszCSVFile )
{
	LockIt safe( m_mutexVars );

	ErrCode ret = Okay;

	FILE * fp = fopen( lpszCSVFile, "wb" );
	if( fp ) {
		// write format info
		fprintf( fp, "#\r\n# ip to country info\r\n#\r\n# %d records, file format: \r\n# ", m_nRecords );
		for( int j=N_IP_FROM; j<=N_COUNTRY_NAME; j++ ) {
			fprintf( fp, "<%s> ", DEFAULT_CSV_FORMAT_KEYS[j] );
		}
		fprintf( fp, "\r\n#\r\n" );

		// write data
		if( (m_pRecords != NULL) && (m_nRecords > 0) ) {
			CountryRecord cr;
			for( int i=0; i<m_nRecords; i++ ) {
				IpCountryRecord icr = m_pRecords[i];
				cr.country = icr.country;
				if( FindCountry( cr ) ) {
					fprintf( fp, DEFAULT_CSV_FORMAT, icr.ipFrom, icr.ipTo, icr.country.szCode, cr.abbrName, cr.fullName );
				}
			}
		}

		fclose( fp );

	} else {
		ret = FileError;
	}

	return ret;
}

int Ip2CountryUtility::ExportCountryInfo( const char * lpszCountryName )
{
	LockIt safe( m_mutexVars );

	ErrCode ret = Okay;

	if( (m_mapCountries.size() > 0) && (lpszCountryName != NULL) ) {
		FILE * fp = fopen( lpszCountryName, "wb" );
		if( fp ) {
			// write format info
			fprintf( fp, "#\r\n# country info\r\n#\r\n# %d records, file format: \r\n# ", m_mapCountries.size() );
			for( int j=N_COUNTRY_CODE; j<=N_COUNTRY_NAME; j++ ) {
				fprintf( fp, "<%s> ", DEFAULT_CSV_FORMAT_KEYS[j] );
			}
			fprintf( fp, "\r\n#\r\n" );

			for( CountryMap::iterator it = m_mapCountries.begin(), eit = m_mapCountries.end(); it != eit; it ++ ) {
				CountryRecord * p = it->second;
				if( p != NULL ) {
					fprintf( fp, DEFAULT_COUNTRY_FORMAT, p->country.szCode, p->abbrName, p->fullName );
				}
			}

			fclose( fp );
		} else {
			ret = FileError;
		}
	}

	return ret;
}

int Ip2CountryUtility::ExportBinI2C( const char * lpszI2CFile )
{
	LockIt safe( m_mutexVars );

	ErrCode ret = Okay;

	if( (m_pRecords != NULL) && (m_nRecords > 0) && (lpszI2CFile != NULL) ) {
		FILE * fp = fopen( lpszI2CFile, "wb" );
		if( fp ) {

			IP2COUNTRY_FILEHEADER fh;
			memset( & fh, 0, sizeof(fh) );
			strcpy( fh.magic, MAGIC_STR );
			fh.version = M_VERSION;
			fh.headerlength = sizeof(fh);
			fh.records = m_nRecords;

			fwrite( & fh, sizeof(fh), 1, fp );
			fwrite( & m_pRecords[0], sizeof(IpCountryRecord), m_nRecords, fp );
		
			fclose( fp );
		} else {
			ret = FileError;
		}
	}

	return ret;
}

// "CA","CAN","CANADA"
bool Ip2CountryUtility::CountryLineToRecord( char * szLine, CountryRecord & cr )
{
	char * items[5] = { NULL, NULL, NULL, NULL, NULL };

	char * p = szLine;

	for( int i=0; i<3; i++ ) {
		while( (*p!='"') && (*p!='\0') ) p++; 
		if(*p=='"') p++;
		items[i] = p;
		while( (*p!='"') && (*p!='\0') ) p++; 
		if(*p=='"') { *p='\0'; p++; }

		if( items[i] == '\0' ) return false;
	}

	for( int i=0; i<3; i++ ) {
		switch( i ) {
		case 0: 
			strncpy( cr.country.szCode, items[i], sizeof(cr.country) ); 
			cr.country.szCode[ sizeof(cr.country)-1 ] = '\0';
			break;
		case 1: 
			strncpy( cr.abbrName, items[i], sizeof(cr.abbrName) );
			cr.abbrName[ sizeof(cr.abbrName) -1 ] = '\0';
			break;
		case 2: 
			strncpy( cr.fullName, items[i], sizeof(cr.fullName) );
			cr.fullName[ sizeof(cr.fullName) -1 ] = '\0';
			break;
		}
	}

	return true;
}

int Ip2CountryUtility::ImportCountryInfo( const char * lpszCountryName )
{
	ResetCountryInfo();

	LockIt safe( m_mutexVars );

	ErrCode ret = Okay;

	// import country info
	if( lpszCountryName != NULL ) {
		FILE * fp = fopen( lpszCountryName, "rb" );
		if( fp ) {

			char szLine[ 256 ];
			CountryRecord cr;

			while( fgets( szLine, 255, fp ) ) {
				szLine[255] = '\0';
				if( szLine[0] != '"' ) continue;

				if( CountryLineToRecord( szLine, cr ) ) {
					UpdateCountry( cr );
				}
			}

			fclose( fp );
		} else {
			ret = FileError;
		}
	}

	return ret;
}

int Ip2CountryUtility::ImportBinI2C( const char * lpszI2CFile )
{
	ResetI2C();

	LockIt safe( m_mutexVars );

	ErrCode ret = Okay;

	// import records
	if( lpszI2CFile != NULL ) {
		FILE * fp = fopen( lpszI2CFile, "rb" );
		if( fp ) {
			IP2COUNTRY_FILEHEADER fh;
			memset( & fh, 0, sizeof(fh) );

			if( fread( & fh, sizeof(fh), 1, fp ) == 1 ) {
				if( (0 == strcmp(fh.magic, MAGIC_STR)) && (fh.headerlength == sizeof(fh)) && (fh.version <= M_VERSION) ) {

					if( m_pRecords != NULL ) {
						delete m_pRecords;
						m_pRecords = NULL;
						m_nRecords = 0;
					}

					unsigned int n = fh.records;
					m_pRecords = new IpCountryRecord[ n ];
					if( m_pRecords != NULL ) {
						if( fread( & m_pRecords[0], sizeof(IpCountryRecord), n, fp ) == n ) {
							m_nRecords = n;
						}
					}

				}
			} else {
				ret = FileError;
			}

			fclose( fp );
		} else {
			ret = FileError;
		}
	}
		
	return ret;
}

bool Ip2CountryUtility::Merge( Ip2CountryUtility & other )
{
	// empty table, no need to merge
	if( (other.m_pRecords == NULL) || (other.m_nRecords == 0) ) return true;

	LockIt safe1( this->m_mutexVars );
	LockIt safe2( other.m_mutexVars );

	int nTotal = this->m_nRecords + other.m_nRecords;
	IpCountryRecord * pTemp = new IpCountryRecord[ nTotal * 2 ];
	if( ! pTemp ) return false;

	int nCount = 0;

	if( m_pRecords ) {
		IpCountryRecord * pi = & this->m_pRecords[0];
		IpCountryRecord * pj = & other.m_pRecords[0];
		IpCountryRecord * pk = & pTemp[0];

		IpCountryRecord ok, cur; 
		memset( & ok, 0, sizeof(ok) );
		memset( & cur, 0, sizeof(cur) );

		int i = 0, j = 0, k = 0;
		int nduplicated = 0, nmerge = 0, nsplit = 0, nskip = 0;

		while( true ) {

			// pick the smaller one from both queues
			if( i < this->m_nRecords ) {
				if( j < other.m_nRecords ) {
					if( pi->ipFrom < pj->ipFrom ) {
						cur = * pi ++; i ++;
					} else if ( pi->ipFrom > pj->ipFrom ) {
						cur = * pj ++; j ++;
					} else { // if ipFrom is same, then pick the small ipTo
						if( pi->ipTo < pj->ipTo ) {
							cur = * pi ++; i ++;
						} else {
							cur = * pj ++; j ++;
						}
					}
				} else {
					cur = * pi ++; i ++;
				}
			} else {
				if( j < other.m_nRecords ) {
					cur = * pj ++; j ++;
				} else {
					// end of both queues
					if( ok.ipTo != 0 ) {
						* pk ++ = ok; k ++;
					}

					break; // break while(true)
				}
			}

			if ( cur.ipFrom < ok.ipFrom ) { // cur smaller than ok, invalid data, ignore
				nskip ++;

			} else if( ok.ipTo < cur.ipFrom) { // [] {}
				if( ok.ipTo != 0 ) {
					* pk ++ = ok; k ++;
				}

				ok = cur;

			} else { // overlapped

				if ( (ok.ipFrom == cur.ipFrom) && (ok.ipTo == cur.ipTo) ) { // same ip range
					// just ignore other
					nduplicated ++;

				} else if( ok.country.nCode == cur.country.nCode ) { // same country
					// just merge
					ok.ipFrom = min( ok.ipFrom, cur.ipFrom );
					ok.ipTo = max( ok.ipTo, cur.ipTo );

					nmerge ++;

				} else {
					if( ok.ipFrom <= cur.ipFrom ) { // overlapped ip range will be splitted 

						if( cur.ipFrom != 0 ) {
							IpCountryRecord tmp = ok;
							tmp.ipTo = cur.ipFrom -1;
							* pk ++ = tmp; k ++;
						}

						if( ok.ipTo <= cur.ipTo ) {
							ok = cur;
						} else {
							* pk ++ = cur; k ++;

							ok.ipFrom = cur.ipTo +1;
						}

						nsplit ++;

					}  else { // cur smaller than ok, invalid data
						nskip ++;

					}

				}

			}

		}
		nCount = k;

		//printf( "%d items duplated, merged %d items, splitted %d items, skipped %d items.\n", nduplicated, nmerge, nsplit, nskip );

		delete this->m_pRecords;
		this->m_pRecords = NULL;

	} else {
		memcpy( pTemp, other.m_pRecords, sizeof(IpCountryRecord) * other.m_nRecords );
		nCount = other.m_nRecords;
	}

	this->m_pRecords = pTemp;
	this->m_nRecords = nCount;

	// merge country info
	for( CountryMap::iterator it = other.m_mapCountries.begin(), eit = other.m_mapCountries.end(); it != eit; it ++ ) {
		UpdateCountry( * it->second, false );
	}

	return true;
}











