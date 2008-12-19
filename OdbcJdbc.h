/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *     Software distributed under the License is distributed on 
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *     express or implied.  See the License for the specific 
 *     language governing rights and limitations under the License.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 */

#ifndef __ODBCJDBC_H
#define __ODBCJDBC_H


#ifdef _WINDOWS
#include <windows.h>

#if _MSC_VER >= 1400 // VC80 and later
#define strcasecmp		_stricmp
#define strncasecmp		_strnicmp
#else
#define strcasecmp		stricmp
#define strncasecmp		strnicmp
#endif // _MSC_VER >= 1400

#define snprintf		_snprintf
#define swprintf		_snwprintf

#else
#define OutputDebugString(string)	fputs (string, stdout)
#endif

#ifdef DEBUG 

#define LOG_MSG(msg)	OutputDebugString( msg )

#ifdef LOGGING

#ifdef _WINDOWS
#define LOG_FILE "c:\\odbcjdbc.log"
#else
#define LOG_FILE "/tmp/odbcjdbc.log"
#endif

#define LOG_PRINT(msg)														\
		{																	\
			if ( !logFile )													\
			{																\
				logFile = fopen( LOG_FILE, "a+" );							\
				if ( logFile )												\
				{															\
					fprintf( logFile, "*\n*\n*\n* New Session\n*\n*\n*\n" );\
					fflush( logFile );										\
				}															\
			}																\
																			\
			if ( !logFile )													\
				OutputDebugString( "log file create failed\n" );			\
			else															\
			{																\
				fprintf msg;												\
				fflush( logFile );											\
			}																\
		}

#else // LOGGING

#define LOG_PRINT(msg)

#endif

#else // DEBUG

#define LOG_MSG(msg)
#define LOG_PRINT(msg)

#endif

#include <sql.h>
#include <sqlext.h>
#include <sqlucode.h>
#include "SetupAttributes.h"
#include "IscDbc/JavaType.h"

#ifndef _WINDOWS
#if (SIZEOF_LONG == 4)
#define	SDWORD		long int
#define	UDWORD		unsigned long int
#else
#define	SDWORD		int
#define	UDWORD		unsigned int
#endif
#endif

#ifndef SQL_BOOLEAN
#define SQL_BOOLEAN		16
#endif

#ifndef NULL
#define NULL			0
#endif

#ifndef _ASSERT
#define _ASSERT(what)
#endif

#undef ASSERT
#define ASSERT(b)			_ASSERT (b)
#define MAX(a,b)			((a > b) ? a : b)
#define MIN(a,b)			((a < b) ? a : b)
#define ABS(n)				(((n) >= 0) ? (n) : -(n))
#define MASK(n)				(1 << (n))
#define ISLOWER(c)			((c) >= 'a' && (c) <= 'z')
#define ISUPPER(c)			((c) >= 'A' && (c) <= 'Z')
#define ISDIGIT(c)			((c) >= '0' && (c) <= '9')
#define UPPER(c)			((ISLOWER (c)) ? (c) - 'a' + 'A' : (c))
#define ROUNDUP(n,b)		(((n) + (b) - 1) & ~((b) - 1))

#define SQL_FBGETSTMT_PLAN				11999
#define SQL_FBGETSTMT_TYPE				11998
#define SQL_FBGETSTMT_INFO				11997

// ext env attribute
#define SQL_ATTR_HANDLE_DBC_SHARE		4000

#define DRIVER_LOCKED_LEVEL_NONE   		0
#define DRIVER_LOCKED_LEVEL_ENV         1
#define DRIVER_LOCKED_LEVEL_CONNECT     2

#ifndef DRIVER_LOCKED_LEVEL
#define DRIVER_LOCKED_LEVEL		DRIVER_LOCKED_LEVEL_CONNECT
#endif

#define FB_COMPILER_MESSAGE_STR(x) #x
#define FB_COMPILER_MESSAGE_STR2(x)   FB_COMPILER_MESSAGE_STR(x)
#define FB_COMPILER_MESSAGE(desc) message(__FILE__ "("	\
									FB_COMPILER_MESSAGE_STR2(__LINE__) "):" desc)


#endif


