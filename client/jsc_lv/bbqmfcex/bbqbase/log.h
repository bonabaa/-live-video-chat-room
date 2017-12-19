
#ifndef _MSG_LOG_H
#define _MSG_LOG_H

#include <ptlib.h>

class PLog : public PObject
{
	PCLASSINFO(PLog, PObject);

public:
	enum Level {
		StdError	= -1,
		Fatal		= 0,   
		Error		= 1,    
		Warning		= 2,  
		Info		= 3,     
		Debug		= 4,    
		Debug2		= 5,   
		Debug3		= 6,   
		Debug4		= 7,   
		Debug5		= 8,   
		Debug6		= 9,   
		NumLogLevels
	};

	const char * ToString( int n ) {
		switch( n ) {
		case StdError:	return "StdError";
		case Fatal:		return "Fatal";
		case Error:		return "Error";
		case Warning:	return "Warning";
		case Info:		return "Info";
		case Debug:		return "Debug";
		case Debug2:	return "Debug2";
		case Debug3:	return "Debug3";
		case Debug4:	return "Debug4";
		case Debug5:	return "Debug5";
		case Debug6:	return "Debug6";
		}
		return "NumLogLevels";
	}

	PLog( int level = Info ) { logLevel = level; }
	~PLog() {}

	void SetLevel( int level ) { logLevel = level; }
	int GetLevel() const { return logLevel; }

	// override this function to do real log
	virtual void Output( int level, const char * msg );

protected:
	int		logLevel;
};

#define		SIZE_MB_1		1048576
#define		SIZE_KB_10		10240
#define		SIZE_KB_100		102400

class PFileLog : public PLog
{
	PCLASSINFO( PFileLog, PLog )

protected:
	mutable PMutex m_mutexLog;

	FILE		* fpLog;

	PString		m_strPath; // xxx.log, 
	int			nMaxSize;	// if file size larger than this value, it will be backup to xxx.bak
	int			nMaxBakFiles; // more backup files will be supported, reserved for future

protected:

	void Close( void );
	void Backup( void );

public:
	PFileLog();
	~PFileLog();

	static bool TouchFile( const char * path );

	bool Open( const char * path, int maxsize = SIZE_MB_1, int maxbak = 1 );
	void Clear( void );
	virtual void Output( int level, const char * msg );

	PString GetPath( void );
	int		GetSize( void );
	int GetTailText( char * lpBuf, int nBytes = SIZE_KB_10 /* 10 KB */ );
};

#define PLOG( pOb, lvl, msg ) \
{ \
	int level = (int)(PLog::lvl - PLog::Info); \
	if( level < 1 ) level = 1; \
	PTRACE( level, msg ); \
	if( (pOb) && ((pOb)->GetLevel() >= PLog::lvl) ) { \
		(pOb)->Output( PLog::lvl, msg ); \
	} \
}

#endif // _MSG_LOG_H



