/*
 *
 *	bbqdatabase.h
 *
 */
#ifndef _BBQ_AUTHGW_H_
#define _BBQ_AUTHGW_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <string>
#include <strstream>
#include "bbquserinfo.h"
#include "md5.h"

using std::list;
using std::string;

#include "bbquserinfo.h"
#include "md5.h"

// this defines a pure common interface to access all kinds of authentication gateway, 
// such as:
// file://
// http://
// https://
// ldap://
// etc.

class BBQAuthGateway
{
protected:
	
public:
	BBQAuthGateway();
	virtual ~BBQAuthGateway();

	virtual const char * GetName( void ) { return "null"; }	// file, mysql, oracle, ldap, etc.
	virtual void Delete( void ) { delete this; }	// all child should define this for deleting using parent pointer

public:
	virtual bool Open( const char * lpszURL, int nSessions = 10 ) { return true; }
	virtual bool Close( void ) { return false; }

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

	typedef std::list<string>	string_list;

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

	} UserAuthResult;

	virtual bool Authenticate( const UserAuthInfo *auth, UserAuthResult *result ) { return false; }
};

// BBQAuthGateway may also be implemented into bbqauth_XXX.DLL/.so for dynamically loading.
// bbqauth_http.dll,
// bbqauth_https.dll,
// bbqauth_samba.dll,
// bbqauth_ldap.dll
//
// The .DLL must provide two entry function:
// BBQAuthGateway * obj = CreateAuthGatewayObject(...),
// obj->GetName(),
// obj->Delete(), ...

#endif /* _BBQ_AUTHGW_H_ */

