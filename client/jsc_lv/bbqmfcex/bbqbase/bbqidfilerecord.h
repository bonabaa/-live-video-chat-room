
#ifndef _SFID_FILERECORD_H
#define _SFID_FILERECORD_H

#include "sfidmsg.h"
#include "bbqdatabase.h"

#pragma pack(1)

#define		SFIDFileVersion(x,y,z)		((((x)&0xff)<<16) | (((y)&0xff)<<8) | ((z)&0xff))

#define		SFIDFileVersionMajor(c)		(((c)>>16) & 0xff)
#define		SFIDFileVersionMinor(c)		(((c)>>8) & 0xff)
#define		SFIDFileVersionBuild(c)		((c) & 0xff)

#define		SFID_FILEHEADER_SIZE		128

#define		SFID_MAGIC					0x44494653

typedef union SFIDFileHeader {
	struct {
		uint32			magic;
		uint32			version;
	};
	char			reserved[ SFID_FILEHEADER_SIZE ];
} PACKED SFIDFileHeader;

// ------------------- current version ----------------------------

#define		SFID_FILERECORD_SIZE	4096

#if( SFID_RECORD_SIZE > SFID_FILERECORD_SIZE )
#error SFID_FILERECORD_SIZE not big enough to contain SFIDRecord.
#endif

typedef union SFIDFileRecord
{
	BBQUserFullRecord		full;				// 2973
	char					padding[ SFID_FILERECORD_SIZE ];
} PACKED SFIDFileRecord;

#define SFID_FILERECORD_CURRENTVERSION		SFIDFileRecord

#define	SFID_FILE_CURRENTVERSION			SFIDFileVersion(1, 7, 0)

#pragma pack()

#endif

