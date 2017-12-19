
#include "bbqbase.h"
#include "bbqidmsg.h"

//// --- BBQUserBasicInfo
//
//PBytePack & operator<< (PBytePack & pack, BBQUserBasicInfo & ubi)
//{
//	pack << ubi.image << ubi.age << ubi.gender;
//	pack.PackString( ubi.alias );
//	pack.PackString( ubi.country );
//	pack.PackString( ubi.state );
//	pack.PackString( ubi.city );
//
//	return pack;
//}
//
//PBytePack & operator>> (PBytePack & pack, BBQUserBasicInfo & ubi)
//{
//	pack >> ubi.image >> ubi.age >> ubi.gender;
//	pack.UnpackString( ubi.alias, USERDATA_NAME_SIZE );
//	pack.UnpackString( ubi.country, USERDATA_NAME_SIZE );
//	pack.UnpackString( ubi.state, USERDATA_NAME_SIZE );
//	pack.UnpackString( ubi.city, USERDATA_NAME_SIZE );
//
//	return pack;
//}
//
//// --- BBQUserContactInfo
//
//PBytePack & operator<< (PBytePack & pack, BBQUserContactInfo & uci )
//{
//	pack.PackString( uci.email );
//	pack.PackString( uci.address );
//	pack.PackString( uci.postcode );
//	pack.PackString( uci.phone );
//	pack.PackString( uci.handphone );
//	pack << uci.privacy;
//
//	return pack;
//}
//
//PBytePack & operator>> (PBytePack & pack, BBQUserContactInfo & uci )
//{
//	pack.UnpackString( uci.email, USERDATA_ADDRESS_SIZE );
//	pack.UnpackString( uci.address, USERDATA_ADDRESS_SIZE );
//	pack.UnpackString( uci.postcode, USERDATA_PHONE_SIZE );
//	pack.UnpackString( uci.phone, USERDATA_PHONE_SIZE );
//	pack.UnpackString( uci.handphone, USERDATA_PHONE_SIZE );
//	pack >> uci.privacy;
//
//	return pack;
//}
//
//// --- BBQUserDetailedInfo
//
//PBytePack & operator<< (PBytePack & pack, BBQUserDetailedInfo & udi )
//{
//	pack << udi.attribute << udi.bloodtype << udi.constellation;
//	pack.PackString( udi.realname );
//	pack.PackString( udi.school );
//	pack.PackString( udi.occupation );
//	pack.PackString( udi.webpage );
//	pack.PackString( udi.comment );
//
//	return pack;
//}
//
//PBytePack & operator>> (PBytePack & pack, BBQUserDetailedInfo & udi )
//{
//	pack >> udi.attribute >> udi.bloodtype >> udi.constellation;
//	pack.UnpackString( udi.realname, USERDATA_NAME_SIZE );
//	pack.UnpackString( udi.school, USERDATA_NAME_SIZE );
//	pack.UnpackString( udi.occupation, USERDATA_NAME_SIZE );
//	pack.UnpackString( udi.webpage, USERDATA_ADDRESS_SIZE );
//	pack.UnpackString( udi.comment, USERDATA_COMMENT_SIZE );
//
//	return pack;
//}
//
//// --- BBQUserInfo
//
//PBytePack & operator<< (PBytePack & pack, BBQUserInfo & ui )
//{
//	pack << ui.basic << ui.contact << ui.detail << ui.security;
//	return pack;
//}
//
//PBytePack & operator>> (PBytePack & pack, BBQUserInfo & ui )
//{
//	pack >> ui.basic >> ui.contact >> ui.detail >> ui.security;
//	return pack;
//}

// --- BBQSearchCondition

PBytePack & operator<< (PBytePack & pack, BBQSearchCondition & sc )
{
	pack << sc.video << sc.audio << sc.gender << sc.age_min << sc.age_max;

	pack.PackString( sc.alias );
	pack.PackString( sc.email );

	pack.PackString( sc.country );
	pack.PackString( sc.state );
	pack.PackString( sc.city );

	pack.PackString( sc.school );
	pack.PackString( sc.occupation );

	return pack;
}

PBytePack & operator>> (PBytePack & pack, BBQSearchCondition & sc )
{
	pack >> sc.video >> sc.audio >> sc.gender >> sc.age_min >> sc.age_max;

	pack.UnpackString( sc.alias, USERDATA_NAME_SIZE );
	pack.UnpackString( sc.email, USERDATA_ADDRESS_SIZE );

	pack.UnpackString( sc.country, USERDATA_NAME_SIZE );
	pack.UnpackString( sc.state, USERDATA_NAME_SIZE );
	pack.UnpackString( sc.city, USERDATA_NAME_SIZE );

	pack.UnpackString( sc.school, USERDATA_NAME_SIZE );
	pack.UnpackString( sc.occupation, USERDATA_NAME_SIZE );

	return pack;
}

// --- BBQSearchResult

PBytePack & operator<< (PBytePack & pack, BBQSearchResult & sr )
{
	pack << sr.uid << sr.gender << sr.age << sr.video << sr.audio;
	pack.PackString( sr.alias );
	pack.PackString( sr.state );
	pack.PackString( sr.city );

	return pack;
}

PBytePack & operator>> (PBytePack & pack, BBQSearchResult & sr )
{
	pack >> sr.uid >> sr.gender >> sr.age >> sr.video >> sr.audio;
	pack.UnpackString( sr.alias, USERDATA_NAME_SIZE );
	pack.UnpackString( sr.state, USERDATA_NAME_SIZE );
	pack.UnpackString( sr.city, USERDATA_NAME_SIZE );

	return pack;
}
PBytePack & operator<< (PBytePack & pack, SIMD_SC_STRINGS & sr )
{
  pack << *((uint32*)(&sr.flags)) ;
  pack.PackString( sr.key );

	return pack;
}

PBytePack & operator>> (PBytePack & pack, SIMD_SC_STRINGS & sr )
{
	pack >>  *((uint32*)(&sr.flags)) ;
	pack.UnpackString( sr.key, sizeof(sr.key)  );

	return pack;
}

PBytePack & operator<< (PBytePack & pack, SIM_MESSAGE_COMPR::simCompress  & sr )
{
	pack.Pack(&sr, sizeof(SIM_MESSAGE_COMPR::simCompress));
	return pack;
}

PBytePack & operator>> (PBytePack & pack, SIM_MESSAGE_COMPR::simCompress  & sr )
{
	pack.Unpack(&sr, sizeof(SIM_MESSAGE_COMPR::simCompress));

	return pack;
}
