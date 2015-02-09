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
 *
 *  2002-10-11  IscDbc.h
 *              Contributed by C G Alvarez
 *              Added MAX_***** constants that simplify
 *              much coding in other classes.
 *
 *
 */

#ifndef __ISCDBC_H_
#define __ISCDBC_H_

#include <stdlib.h>
#include <string.h>
#include <ibase.h>
#include "JString.h"
#include "LoadFbClientDll.h"

#ifndef SQL_BOOLEAN
#define SQL_BOOLEAN		32764
#endif

#ifndef blr_bool
#define blr_bool		(unsigned char)23
#endif

#ifndef NULL
#define NULL			0
#endif

#define SQLEXCEPTION		SQLError
#define NOT_YET_IMPLEMENTED	throw SQLEXCEPTION (FEATURE_NOT_YET_IMPLEMENTED, "not yet implemented")
#define NOT_SUPPORTED(type,rellen,rel,collen,col) throw SQLEXCEPTION (UNSUPPORTED_DATATYPE, "datatype is not supported in ODBC: %s column %*s.%*s", type,rellen,rel,collen,col)
#define THROW_ISC_EXCEPTION(connection, statusVector) throw SQLEXCEPTION ( connection->GDS->_sqlcode( statusVector ), statusVector [1], connection->getIscStatusText (statusVector))
#define OFFSET(type,fld)	(size_t)&(((type*)0)->fld)
#define MAX(a,b)			((a > b) ? a : b)
#define MIN(a,b)			((a < b) ? a : b)
#define ABS(n)				(((n) >= 0) ? (n) : -(n))
#define MASK(n)				(1 << (n))
#define ISLOWER(c)			((c) >= 'a' && (c) <= 'z')
#define ISUPPER(c)			((c) >= 'A' && (c) <= 'Z')
#define ISDIGIT(c)			((c) >= '0' && (c) <= '9')
#define UPPER(c)			((ISLOWER (c)) ? (c) - 'a' + 'A' : (c))
#define ROUNDUP(n,b)		(((n) + (b) - 1) & ~((b) - 1))
#define BOUNDARY			4

#define PUNCT				1
#define WHITE				2
#define DIGIT				4
#define LETTER				8
#define QUOTE				16
#define IDENT				32

#define IS_POINT(p)			((p) == '.')
#define IS_QUOTE(p)			(charTable [(p)] == QUOTE)
#define IS_WHITE(p)			(charTable [(p)] == WHITE)
#define IS_LETTER(p)		(charTable [(p)] & LETTER)
#define IS_IDENT(p)			(charTable [(p)] & IDENT)
#define IS_END_TOKEN(p)		((p) == '\0' || (charTable [(p)] & (PUNCT | WHITE)))
#define SKIP_WHITE(p)		while (charTable [*p] == WHITE) ++p
#define SKIP_NO_WHITE(p)	while ( *p && charTable [*p] != WHITE) ++p

#define TOKEN_LENGTH(token)	( sizeof ( token ) - 1 )
#define IS_MATCH(str,token)	( !strncasecmp( str, token, TOKEN_LENGTH( token ) ) && IS_END_TOKEN(*(str + TOKEN_LENGTH( token ) ) ) )

#define FB_COMPILER_MESSAGE_STR(x) #x
#define FB_COMPILER_MESSAGE_STR2(x)   FB_COMPILER_MESSAGE_STR(x)
#define FB_COMPILER_MESSAGE(desc) message(__FILE__ "("	\
									FB_COMPILER_MESSAGE_STR2(__LINE__) "):" desc)

#ifdef DEBUG
#define CONVERSION_CHECK_DEBUG(bool_assign)							\
	if ( !(bool_assign) )												\
		throw SQLEXCEPTION (CONVERSION_ERROR, "Error conversion")
#else
#define CONVERSION_CHECK_DEBUG(bool_assign)
#endif																

#ifdef _WINDOWS

#if _MSC_VER >= 1400 // VC80 and later
#define strcasecmp		_stricmp
#define strncasecmp		_strnicmp
#else
#define strcasecmp		stricmp
#define strncasecmp		strnicmp
#endif // _MSC_VER >= 1400

#define snprintf		_snprintf
#define fcvt			_fcvt
#define gcvt			_gcvt

#else

#define __int64			long long
#define _stdcall
#define OutputDebugString(string)	fputs (string, stdout)
#endif

typedef unsigned char	UCHAR;
typedef unsigned long	ULONG;
typedef __int64			QUAD;
typedef unsigned __int64			UQUAD;

#include "JavaType.h"
#include "Connection.h"
#include "IscConnection.h"

#define MAX_COUNT_DBC_SHARE			16

#define DEFAULT_BLOB_BUFFER_LENGTH	16384

#define MAX_ARRAY_LENGTH			100000000
#define MAX_BLOB_LENGTH				2147483647
#define MAX_WLONGVARCHAR_LENGTH		715827882
#define MAX_CHAR_LENGTH				32767
#define MAX_VARCHAR_LENGTH			32765
#define MAX_WCHAR_LENGTH			10922
#define MAX_WVARCHAR_LENGTH			10921
#define MAX_NUMERIC_SHORT_LENGTH	4
#define MAX_NUMERIC_LONG_LENGTH		9
#define MAX_NUMERIC_DOUBLE_LENGTH	15
#define MAX_NUMERIC_LENGTH			18
#define MAX_DECIMAL_SHORT_LENGTH	4
#define MAX_DECIMAL_LONG_LENGTH		9
#define MAX_DECIMAL_DOUBLE_LENGTH	15
#define MAX_DECIMAL_LENGTH			18
#define MAX_BOOLEAN_LENGTH			1
#define MAX_TINYINT_LENGTH			3
#define MAX_SMALLINT_LENGTH			5
#define MAX_INT_LENGTH				10
#define MAX_FLOAT_LENGTH			24
#define MAX_DOUBLE_LENGTH			53
#define MAX_FLOAT_DIGIT_LENGTH		7
#define MAX_DOUBLE_DIGIT_LENGTH		15
#define MAX_DATE_LENGTH				10
#define MAX_TIME_LENGTH				13
#define MAX_TIMESTAMP_LENGTH		24
#define MAX_QUAD_LENGTH				18

namespace IscDbcLibrary 
{
int getTypeStatement(IscConnection *connection, isc_stmt_handle Stmt,const void * buffer, int bufferLength, int *lengthPtr);
int getInfoCountRecordsStatement(IscConnection *connection, isc_stmt_handle Stmt,const void * buffer, int bufferLength, int *lengthPtr);
int getPlanStatement(IscConnection *connection, isc_stmt_handle statementHandle,const void * value, int bufferLength, int *lengthPtr);
int getPageDatabase(IscConnection *connection, const void * info_buffer, int bufferLength,short *lengthPtr);
int getWalDatabase(IscConnection *connection, const void * info_buffer, int bufferLength,short *lengthPtr);
int strBuildStatInformations(const void * info_buffer, int bufferLength,short *lengthPtr);
void getStatInformations(IscConnection *connection, char bVanCall);
int getStatInformations(IscConnection *connection, const void * info_buffer, int bufferLength,short *lengthPtr);
int findCharsetsCode( const char *charset );
int getCharsetSize( int charsetCode );
}; // end namespace IscDbcLibrary

#endif
