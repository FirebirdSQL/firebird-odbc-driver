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

#include <ibase.h>
#include "JString.h"

#ifndef NULL
#define NULL		0
#endif

#define SQLEXCEPTION		SQLError
#define NOT_YET_IMPLEMENTED	throw SQLEXCEPTION (FEATURE_NOT_YET_IMPLEMENTED, "not yet implemented")
#define NOT_SUPPORTED(type,rellen,rel,collen,col) throw SQLEXCEPTION (UNSUPPORTED_DATATYPE, "datatype is not supported in ODBC: %s column %*s.%*s", type,rellen,rel,collen,col)
#define THROW_ISC_EXCEPTION(statusVector)			throw SQLEXCEPTION (statusVector [1], IscConnection::getIscStatusText (statusVector))
#define ROUNDUP(a,b)		((a + b - 1) / b * b)
#define MIN(a,b)			(((a) < (b)) ? (a) : (b))
#define MAX(a,b)			(((a) > (b)) ? (a) : (b))

#ifdef _WIN32

#define strcasecmp		stricmp
#define strncasecmp		strnicmp

#else

#define __int64			long long
#define _stdcall
#endif

typedef unsigned char	UCHAR;
typedef unsigned long	ULONG;
typedef __int64			QUAD;
typedef unsigned __int64			UQUAD;

/*
 *		Sql types (actually from java.sql.types)
 */

#define JDBC_NULL		   0 

#define JDBC_BIT 		  -7
#define JDBC_TINYINT 	  -6
#define JDBC_SMALLINT	   5
#define JDBC_INTEGER 	   4
#define JDBC_BIGINT 	  -5

#define JDBC_FLOAT 		   6
#define JDBC_REAL 		   7
#define JDBC_DOUBLE 	   8

#define JDBC_NUMERIC 	   2
#define JDBC_DECIMAL	   3

#define JDBC_CHAR		   1
#define JDBC_VARCHAR 	  12
#define JDBC_LONGVARCHAR  -1

#define JDBC_DATE 		  91
#define JDBC_TIME 		  92
#define JDBC_TIMESTAMP 	  93

#define JDBC_BINARY		  -2
#define JDBC_VARBINARY 	  -3
#define JDBC_LONGVARBINARY 	  -4

#define MAX_ARRAY_LENGTH		100000000
#define MAX_BLOB_LENGTH			2147483647
#define MAX_CHAR_LENGTH			32767
#define MAX_VARCHAR_LENGTH		32765
#define MAX_NUMERIC_LENGTH		18
#define MAX_DECIMAL_LENGTH		18
#define MAX_SMALLINT_LENGTH		5
#define MAX_INT_LENGTH			10
#define MAX_FLOAT_LENGTH		24
#define MAX_DOUBLE_LENGTH		53
#define MAX_DATE_LENGTH			10
#define MAX_TIME_LENGTH			8
#define MAX_TIMESTAMP_LENGTH	19
#define MAX_QUAD_LENGTH			19

#endif
