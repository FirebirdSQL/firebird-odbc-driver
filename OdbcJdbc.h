/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
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


#ifdef _WIN32
#include <windows.h>
#define strcasecmp		stricmp
#define strncasecmp		strnicmp
#define snprintf		_snprintf
#define fcvt			_fcvt
#define gcvt			_gcvt

#else
#define OutputDebugString(string)	fputs (string, stdout)
#endif

#ifdef DEBUG 
#ifdef LOGGING

#ifdef _WIN32
#define LOG_FILE "c:\\odbc.log"
#else
#define LOG_FILE "/tmp/odbc.log"
#endif

void logMsg (const char *msg);
#define LOG_MSG(msg)	logMsg (msg)

#else
#define LOG_MSG(msg)	OutputDebugString (msg)
#endif

#else
#define LOG_MSG(msg)
#endif

#include <sql.h>
#include <sqlext.h>
#include "IscDbc/JavaType.h"

#ifndef NULL
#define NULL				0
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

#define DRIVER_LOCKED_LEVEL_ENV         4
#define DRIVER_LOCKED_LEVEL_CONNECT     3
#define DRIVER_LOCKED_LEVEL				DRIVER_LOCKED_LEVEL_CONNECT

#define FB_COMPILER_MESSAGE_STR(x) #x
#define FB_COMPILER_MESSAGE_STR2(x)   FB_COMPILER_MESSAGE_STR(x)
#define FB_COMPILER_MESSAGE(desc) message(__FILE__ "("	\
									FB_COMPILER_MESSAGE_STR2(__LINE__) "):" desc)


#endif

