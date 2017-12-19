
#ifndef _USER_ID_LIST_H_
#define _USER_ID_LIST_H_


#include <stdio.h>
#include <stdlib.h>

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#include <list>
typedef std::list<unsigned int> uint_list;

#define USER_ID_LIST_INC 16

class UserIdList 
{

protected:
	int			bufcount;
	int			count;
	uint32 *	list;

public:
	UserIdList() { 
		bufcount = USER_ID_LIST_INC;
		list = new uint32[ bufcount ]; 
		count = 0; 
	}

	~UserIdList() { 
		if(list) delete[] list; 
		list = NULL; 
		bufcount = count = 0; 
	}

	uint32 * Data( void ) { return list; }
	int Count( void ) { return count; }
	int Index( uint32 uid ) {
		for( int i=0; i<count; i++ ) {
			if(list[i] == uid) return i;
		}
		return -1;
	}

	void RemoveAll( void ) { count=0; }

	bool Contain( uint32 uid ) { return (Index(uid) >= 0); }

	bool Add( uint32 uid ) {
		if( count < USERDATA_LIST_SIZE-1 ) {
			if( ! Contain(uid) ) {
				if( count +1 > bufcount ) {
					bufcount += USER_ID_LIST_INC;
					uint32 * newList = new uint32[ bufcount ];
					if( list ) {
						memcpy( newList, list, sizeof(uint32) * count);
						delete[] list;
					}
					list = newList;
				}
				list[count] = uid;
				count ++;
			}
			return true;
		}
		return false;
	}

	bool Remove( uint32 uid ) {
		int j = 0;
		for( int i=0; i<count; i++ ) {
			list[j] = list[i];
			if( list[i] != uid ) {
				j ++;
			}
		}
		bool bDone = (j < count);
		count = j;

		return bDone;
	}

	bool ExportTo( uint32 * array, int nMaxSize ) {
		if( nMaxSize > 0 ) {
			int n = min(count, nMaxSize-1);
			if( n > 0 ) {
				memcpy( array, list, sizeof(uint32) * n );
			}
			array[n] = 0;
			return true;
		}
		return false;
	}

	bool ImportFrom( uint32 * array, int nMaxSize ) {

		RemoveAll();

		for( int i=0; i<nMaxSize; i++, array++ ) {
			if( * array ) Add( * array );
			else break;
		}

		return true;
	}

	bool operator >> ( FILE * fp ) {
		fwrite( & count, sizeof(int), 1, fp );
		if( count > 0 ) {
			fwrite( list, sizeof(uint32), count, fp );
		}

		return true;
	}

	bool operator << ( FILE * fp ) {
		int n = 0;
		fread( & n, sizeof(int), 1, fp );
		if( n < 0 ) return false;
		else if( n <= bufcount ) count = 0;
		else {
			if( n >= 512 ) n = 511;
			uint32 buf[ 512 ];
			memset( buf, 0, sizeof(uint32) * (n+1) );
			if( fread( buf, sizeof(uint32), n, fp ) == (uint32)n ) {
				return ImportFrom( buf, n );
			}
		}
		
		return false;
	}
};

#endif //  _USER_ID_LIST_H_
