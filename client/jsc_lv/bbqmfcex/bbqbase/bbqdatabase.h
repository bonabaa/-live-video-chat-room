/*
 *
 *	bbqdatabase.h
 *
 */
#ifndef BBQ_DATABASE_H
#define BBQ_DATABASE_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <list>
#include <string>

#include "bbquserinfo.h"
#include "md5.h"
#include "bbqmcc.h"

typedef std::list<unsigned int> uint_list;
typedef std::list<std::string>	string_list;

typedef std::list<BBQ_MeetingInfo *> meeting_list;
typedef std::list<BBQ_MeetingInfo_Ext *> meetingext_list;

// this defines a pure common interface to access all kinds of BBQDatabase, 
// such as:
// file://
// mysql://
// ldap://
// oracle://
// etc.

#define MAX_BINDED_PRODUCTKEY	5
#define DEFAULT_SESSION_NUM		10

class BBQDatabase {

public:
	virtual const char * GetName( void ) { return "null"; }	// file, mysql, oracle, ldap, etc.
	virtual void Delete( void ) { delete this; }	// all child should define this for deleting using parent pointer

protected:
	int			m_nLastErrorCode;
	char		m_strLastErrorMsg[ 256 ];

	void		SetLastError( int nCode, const char * msg ) {
		m_nLastErrorCode = nCode;

		int nLen = (int)strlen( msg );
		if( nLen >= 255 ) {
			nLen = 255;
		}
		strncpy( m_strLastErrorMsg, msg, nLen );
		m_strLastErrorMsg[ nLen ] = '\0';
	}

public:
	enum ErrorCode {
		NoError,
		BadSyntax,
		RecordNotFound,
		TableNotFound,
		DBNotFound,
		ServerFail,
	};

	int	GetLastErrorCode( void ) { return m_nLastErrorCode; }
	const char * GetLastErrorMsg( void ) { return m_strLastErrorMsg; }

	static const char * ErrorCodeToString( int n ) {
		switch( n ) {
		case NoError:			return "NoError";
		case BadSyntax:			return "BadSyntax";
		case RecordNotFound:	return "RecordNotFound";
		case TableNotFound:		return "TableNotFound";
		case DBNotFound:		return "DBNotFound";
		case ServerFail:		return "ServerFail";
		}
		return "DbError";
	}

public:
	BBQDatabase() {
		m_nLastErrorCode = NoError;
		m_strLastErrorMsg[0] = '\0';
	}

	// database configuration URL format:
	// file://c:/bbqserver/bbq.db
	// mysql://user:password@host/database
	// ldap://...
	// oracle://...
	virtual bool Open( const char * lpszDBURL, int nSessions = DEFAULT_SESSION_NUM ) = 0;

	virtual void Close( void ) = 0;

	virtual bool IsDbReady( void ){ return false; }

	virtual bool EnableServerActivity( bool bEnable = true ) { return true; }

	// ============= Server configuration parameters interface ========

	virtual unsigned int  GetCountOfUserProfile( unsigned int nId = 0xffffffff ){ return nId; }

	virtual bool ServerConfig_LoadString( const char * lpszName, char * lpszBuf, int nBufLen ) { return false; }

	virtual bool ServerConfig_SaveString( const char * lpszName, const char * lpszValue ) { return false; }

	virtual bool ServerConfig_ReadBoolean( const char * lpszName, bool default_value ) {
		char tmp[ 32 ];
		if( ServerConfig_LoadString( lpszName, tmp, 32 ) ) {
			return (0 == strcmp( lpszName, "TRUE") );
		}
		return default_value;
	}

	virtual long ServerConfig_ReadInteger( const char * lpszName, long default_value ){
		char tmp[ 32 ];
		if( ServerConfig_LoadString( lpszName, tmp, 32 ) ) {
			return strtol( tmp, NULL, 0 );
		}
		return default_value;
	}

	virtual unsigned long ServerConfig_ReadDWORD( const char * lpszName, unsigned long default_value ) {
		char tmp[ 32 ];
		if( ServerConfig_LoadString( lpszName, tmp, 32 ) ) {
			unsigned long value;
			sscanf( tmp, "%X", & value );
			return value;
		}
		return default_value;
	}

	virtual bool ServerConfig_WriteBoolean( const char * lpszName, bool value ){
		return ServerConfig_SaveString( lpszName, (value ? "TRUE" : "FALSE") );
	}

	virtual bool ServerConfig_WriteInteger( const char * lpszName, long value ){
		char tmp[32];
		sprintf( tmp, "%d", value );
		return ServerConfig_SaveString( lpszName, tmp );
	}

	virtual bool ServerConfig_WriteDWORD( const char * lpszName, unsigned long value ){
		char tmp[32];
		sprintf( tmp, "%X", value );
		return ServerConfig_SaveString( lpszName, tmp );
	}

	// ============= Get/Set Next User Id counter ==================

	virtual uint32 GetNextUserId( void ) { return 0; }
	virtual uint32 SetNextUserId( uint32 uid ) { return uid; }

	// ============= User data interface ===========================

	// load user record from database by user id,
	//
	// before calling Read & Write.
	// the following fields should be initalized: 
	// pBuf->uid,
	// pBuf->valid_flags. 
	virtual bool Read( BBQUserFullRecord *pBuf ) = 0;

	// update user info into database.
	virtual bool Write( const BBQUserFullRecord *pUser ) = 0;
	
	typedef enum {
		BY_HARDWAREID,
		BY_HANDPHONE,
		BY_EMAIL,
	} KEY_TYPE;

	// load user record by some special unique key, such as hardware ID, email address, handphone number
	// so that user can login with such unique key.
	//
	// before calling the function,
	// the following fields should be initalized: 
	// pBuf->valid_flags. 
	virtual bool ReadByKey( BBQUserFullRecord *pBuf, const char * lpszKey, KEY_TYPE nKey ) = 0;

	// keyType here might be a field name, such as "PCCW_CVG", "PCCW_BB", "PCCW_IVC", etc.
	// keyValue here might be sth. like, "1212345", "user@host.com", etc.
	//
	// before calling the function,
	// the following fields should be initalized: 
	// pBuf->valid_flags. 
	virtual bool ReadByKey( BBQUserFullRecord *pBuf, const char * keyType, const char * keyVaue ) { return false; }

	// Create user and return the userid, none zero if succeeded.
	virtual uint32 CreateUser( const BBQUserFullRecord *pUser, uint32 nSpecialID = 0 ) = 0;

	virtual bool DestroyUser( VFONID uid ) { return false; }

	// Search
	virtual int Search( const BBQSearchCondition* condition, BBQSearchResult* buf, int nMaxCount, int nStart = 0, unsigned long *pTotal = 0 ) { return 0; }

	// Bind user by special unique key
	virtual bool BindByKey( VFONID uid, const char *lpszKey, KEY_TYPE nKey ){ return false; }

	virtual bool BindByKey( VFONID uid, const VfonIdBindInfo * bindInfo ) { return false; }

	virtual bool GetBindInfoById( VFONID uid, VfonIdBindInfo * bindInfo ) { return false; }

	virtual bool GetIdByBindInfo( const VfonIdBindInfo * bindInfo, VFONID * pId ) { return false; }

	virtual bool WriteUserActionLog( const VfonUserActionLog * pLog ) { return false; }

	typedef enum ID_RELATION {
		PARENT_ID,
		CHILD_ID,
	} ID_RELATION;

	virtual bool ReadRelationIdList( VFONID uid, ID_RELATION listType, uint_list * ulist ) { return false; }

	virtual bool WriteRelationIdList( VFONID uid, ID_RELATION listType, uint_list * ulist ) { return false; }

	// offline message

	virtual bool WriteOfflineIM( BBQOfflineIMMessage * pIM ) { return false; }

	virtual bool ReadOfflineIM( offlineim_list & lstIMs, uint64 to_id ) { return false; }
	virtual bool DeleteOfflineIM( uint64 to_id ) { return false; }

	// ========= Multi-point Conference Control related =======================================

	// obsolete

	// virtual bool ReadConferenceBooking( meeting_list & lstResult, time_t tStartAfter = 0, time_t tStartBefore = 0, uint32 nRoomId = 0, const char * lpszMeetingId = NULL ) { return false; }
	// virtual bool UpdateConferenceBooking( const BBQ_MeetingInfo * pInfo ) { return false; }

	virtual bool ReadConferenceBookingExt( meetingext_list & lstResult, time_t tStartAfter = 0, time_t tStartBefore = 0, uint32 nRoomId = 0, const char * lpszMeetingId = NULL ) { return false; }
	virtual bool UpdateConferenceBookingExt( const BBQ_MeetingInfo_Ext * pInfo ) { return false; }

	// ========= x-domain info ==============================
	virtual bool ReadVfonDomainInfo( VfonDomainInfo * pInfo ) { return false; } // read password by specifying domain id or domain name

	virtual bool ReadClientPersonalData( VFONID uid, const char *key, const char *digest, BBQDataBlock *buf ) { return false; };
	virtual bool WriteClientPersonalData( VFONID uid, const char *key, const char *digest, const BBQDataBlock *buf ) { return false; };

public:
	// ========= User Account Authentication via HTTPS ================
	typedef struct UserAuthInfo{
		char	uid[64];
		char	passwd[32];
		char    bindtype[32]; // "PCCW_CVG", "PCCW_BB", "PCCW_IVC", "ARCHIEVA"
	} UserAuthInfo;

	typedef enum AuthStatus{
		AUTH_FAILED  = 0,
		AUTH_SUCCEED = 1,
		AUTH_SERVICE_EXPIRED = 100,
	}AuthStatus;
	
	typedef enum UserServiceType {
		SERVICE_VMEET = 4,
		SERVICE_VFON = 5,
	} UserServiceType;

	typedef struct UserAuthResult {
		int			status;		// see AuthStatus
		int			type;		// see UserServiceType

		char		result[64];	// Authentication result detail

		bool		isTrialUser;// true if it's trial user
		VFONID		inviterID;	// the inviter vfon id

		struct {
			unsigned int		flag_accountid : 1;
			unsigned int		flag_dayoftrial : 1;	// Flag to indicate 'dayoftrial' returned or not, 1 for returned, 0 for not.
			unsigned int		flag_balance : 1;		// Flag to indicate 'balance' returned or not, 1 for returned, 0 for not.
			unsigned int		flag_buddylist : 1;		// flag to indicate buddy list is valid

			unsigned int		flag_padding : 28;
		};
		
		int				accountid;
		int				dayoftrial;		// Free trial days left
		float			balance;		// Archieva will return balance;
		string_list *	pBuddyList;		// buddy name list, each field is the bind id
		int				invisible;		// number of invisible users exclude those in pBuddyList

	} UserAuthResult;

	// https://service.imsbiz.com/webconference/checkwc.jsp
	virtual bool OpenUserAuthChannel( const char * lpszAuthURL = "", int nSessions = DEFAULT_SESSION_NUM ) { return false; }
	virtual void CloseAuthChannel(){}
	virtual bool IsAuthGatewayReady(){ return false; }
	virtual bool Authenticate(const UserAuthInfo *auth, UserAuthResult *result ) { return false; }

	// Enable/disable user VMeet service according to UserAuthResult.type
	virtual bool EnableUserVMeetService( VFONID nUserID, bool bEnable =  true){ return false;}

public:
	// ========= Billing Interface ================

	// if billing system used, some communication data will be sent to billing system
	// else, it is ignored.
	virtual bool OpenBillingSystem( const char * lpszBillingURL = "", int nSessions = DEFAULT_SESSION_NUM ) { return false; }

	virtual void CloseBillingSystem( void ) {}

	virtual bool IsBillingReady( void ) { return false; }

	typedef enum {
		ST_INVALID,		// account balance <= 0
		ST_PAUSED,		// account paused
		ST_NOTEXIST,	// specified userid not exist

		ST_BASIC,
		ST_PRO,
	} ST_CODE;

	typedef struct BillingInfo {
		int			statusCode;
		int			points;
		char		reserved[ 24 ];
	} BillingInfo;

	typedef struct BillingUserInfo {
		VFONID		vfonid;
		char		bindtype[USERDATA_NAME_SIZE];
	}BillingUserInfo;

	//virtual bool CheckBillingStatus( VFONID nUserID, BillingInfo * bInfo ) { return false; }
	virtual bool CheckBillingStatus( const BillingUserInfo *account, BillingInfo * bInfo ) { return false; }

	virtual void LogVfonSession( const VfonSession * pSession ) {}
};

// BBQDatabase may also be implemented into bbqdb.XXX.DLL/.so for dynamically loading.
// bbqdb_file.dll, 
// bbqdb_mysql.dll, 
// bbqdb_oracle.dll, 
// bbqdb_ldap.dll
//
// The .DLL must provide a entry function:
// BBQDatabase obj = CreateDatabaseObject(...),
// obj->GetName(),
// obj->Delete(), ...

// BBQDatabaseCenter, a class to preload and create obj

class BBQDatabaseCenter
{
protected:
	// a mapping structure with info including: 
	// type, module_object, module_path, 

	// a list of db object created, keep here for later to release before quit app.

	// load/unload module file
	bool LoadModule( const char * lpModulePath );
	bool UnloadModule( const char * lpModulePath );

public:
	BBQDatabaseCenter( const char * lpszDefaultModuleDir = "" );
	virtual ~BBQDatabaseCenter();

	BBQDatabase * CreateDatabaseObject( const char * lpTypeOrURL );	// mysql, or mysql://xxx
	void DeleteDatabaseObject( BBQDatabase * pDbObj );
};

#endif /* BBQ_DATABASE_H */

