
#ifndef _BBQ_SVR_USER_H
#define _BBQ_SVR_USER_H

#include "sfidmsg.h"
#include "bbqidmsg.h"

#include <list>

typedef std::list<VFONID>			VfonIdList;
typedef std::list<VfonSession *>	VfonSessionList;

class VfonUserOnlineObject 
{
public:
	VfonUserOnlineObject();
	~VfonUserOnlineObject();

protected:
	class BBQServer;

	SIM_SESSIONINFO			sessionInfo;		// 8

	int						socket;
	IPSOCKADDR				netWAN;				// 6
	IPSOCKADDR				netLAN;				// 6

	uint16					proxySlot;			// 2, empty proxy channel slot, that can serve others. 
	uint16					proxyMax;			// 2, max proxy channel.

	BBQUserInfo				userInfo;
	BBQUserServiceInfo		service;
	BBQUserTechParamInfo	param;

	VfonIdList *			pFriendList;
	VfonIdList *			pBlockList;

	VfonSessionList *		pSessionList;

} VfonUserDetail;

// this will be used to replace old version of SFIDRecord
typedef struct VfonUser {

	VFONID					vfonid;

	// user's online status
	int						onlineStatus;

	// user's timestamp, it maybe updated by local heartbeat, 
	// or broadcast from the server that the user is login.
	time_t					timestamp;

	// if vfonid.server is not zero, that means the user is login to another server,
	// here is the address of that server.
	IPSOCKADDR				netServer;			
	bool					isTcp;

	// NULL, if the user is offline or in another server,
	// online object, if the user has login into this server, all operation should work on this object.
	VfonUserOnlineObject *	object;

} VfonUser;

#endif /* _BBQ_SVR_USER_H */
