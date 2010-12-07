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
 *  The Original Code was created by Vladimir Tsvigun for IBPhoenix.
 *
 *  Copyright (c) 2003 Vladimir Tsvigun
 *  All Rights Reserved.
 */

// SupportFunctions.cpp: implementation of the SupportFunctions class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _WINDOWS
#include <windows.h>
#endif
#include <time.h>
#include <string.h>
#include <sql.h>
#include <sqlext.h>
#include "Mlist.h"
#include "SupportFunctions.h"

namespace IscDbcLibrary {

#define ADD_SUPPORT_FN( typeFn, key, nameSql, nameFb, translateFn )																	\
	fn.set( SupportFunctions::typeFn, key, nameSql, sizeof(nameSql)-1, nameFb, sizeof(nameFb)-1, &SupportFunctions::translateFn );	\
	if ( j = listSupportFunctions.SearchAndInsert( &fn ), j < 0 )																	\
		listSupportFunctions[~j] = fn																								\

SupportFunctions supportFn;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SupportFunctions::SupportFunctions()
{
	CSupportFunction fn;
	int j;
	supportFn = NULL;

//  String functions

    ADD_SUPPORT_FN( STR_FN, SQL_FN_STR_BIT_LENGTH, 		"BIT_LENGTH", 		"BIT_LENGTH",		defaultTranslator);
    ADD_SUPPORT_FN( STR_FN, SQL_FN_STR_CHAR_LENGTH, 	"CHAR_LENGTH", 		"CHAR_LENGTH",		defaultTranslator);
    ADD_SUPPORT_FN( STR_FN, SQL_FN_STR_CHARACTER_LENGTH,"CHARACTER_LENGTH", "CHARACTER_LENGTH",	defaultTranslator);
    ADD_SUPPORT_FN( STR_FN, SQL_FN_STR_OCTET_LENGTH, 	"OCTET_LENGTH", 	"OCTET_LENGTH",		defaultTranslator);
    ADD_SUPPORT_FN( STR_FN, SQL_FN_STR_POSITION, 		"POSITION", 		"POSITION",			defaultTranslator);
    ADD_SUPPORT_FN( STR_FN, SQL_FN_STR_ASCII, 			"ASCII", 			"ASCII",			defaultTranslator);
    ADD_SUPPORT_FN( STR_FN, SQL_FN_STR_CHAR, 			"CHAR", 			"CHAR",				defaultTranslator);
    ADD_SUPPORT_FN( STR_FN, SQL_FN_STR_CONCAT, 			"CONCAT", 			"CONCAT",			defaultTranslator);
    ADD_SUPPORT_FN( STR_FN, SQL_FN_STR_DIFFERENCE, 		"DIFFERENCE", 		"DIFFERENCE",		defaultTranslator);
    ADD_SUPPORT_FN( STR_FN, SQL_FN_STR_INSERT, 			"INSERT", 			"INSERT",			defaultTranslator);
    ADD_SUPPORT_FN( STR_FN, SQL_FN_STR_LCASE, 			"LCASE", 			"LOWER",			defaultTranslator);
    ADD_SUPPORT_FN( STR_FN, SQL_FN_STR_LEFT, 			"LEFT", 			"LEFT",				defaultTranslator);
    ADD_SUPPORT_FN( STR_FN, SQL_FN_STR_LENGTH, 			"LENGTH", 			"LENGTH",			defaultTranslator);
    ADD_SUPPORT_FN( STR_FN, SQL_FN_STR_LOCATE, 			"LOCATE", 			"LOCATE",			defaultTranslator);
    ADD_SUPPORT_FN( STR_FN, SQL_FN_STR_LOCATE_2, 		"LOCATE_2", 		"LOCATE_2",			defaultTranslator);
    ADD_SUPPORT_FN( STR_FN, SQL_FN_STR_LTRIM, 			"LTRIM", 			"LTRIM",			defaultTranslator);
    ADD_SUPPORT_FN( STR_FN, SQL_FN_STR_REPEAT, 			"REPEAT", 			"REPEAT",			defaultTranslator);
    ADD_SUPPORT_FN( STR_FN, SQL_FN_STR_REPLACE, 		"REPLACE", 			"REPLACE",			defaultTranslator);
    ADD_SUPPORT_FN( STR_FN, SQL_FN_STR_RIGHT, 			"RIGHT", 			"RIGHT",			defaultTranslator);
    ADD_SUPPORT_FN( STR_FN, SQL_FN_STR_RTRIM, 			"RTRIM", 			"RTRIM",			defaultTranslator);
    ADD_SUPPORT_FN( STR_FN, SQL_FN_STR_SOUNDEX, 		"SOUNDEX", 			"SOUNDEX",			defaultTranslator);
    ADD_SUPPORT_FN( STR_FN, SQL_FN_STR_SPACE, 			"SPACE", 			"SPACE",			defaultTranslator);
    ADD_SUPPORT_FN( STR_FN, SQL_FN_STR_SUBSTRING, 		"SUBSTRING", 		"SUBSTRING",		defaultTranslator);
    ADD_SUPPORT_FN( STR_FN, SQL_FN_STR_UCASE, 			"UCASE", 			"UPPER",			defaultTranslator);

//  Numeric functions

	ADD_SUPPORT_FN( NUM_FN, SQL_FN_NUM_ABS, 		"ABS", 		"ABS",		defaultTranslator);
    ADD_SUPPORT_FN( NUM_FN, SQL_FN_NUM_ACOS, 		"ACOS", 	"ACOS",		defaultTranslator);
    ADD_SUPPORT_FN( NUM_FN, SQL_FN_NUM_ASIN, 		"ASIN", 	"ASIN",		defaultTranslator);
    ADD_SUPPORT_FN( NUM_FN, SQL_FN_NUM_ATAN, 		"ATAN",		"ATAN",		defaultTranslator);
    ADD_SUPPORT_FN( NUM_FN, SQL_FN_NUM_ATAN2, 		"ATAN2", 	"ATAN2",	defaultTranslator);
    ADD_SUPPORT_FN( NUM_FN, SQL_FN_NUM_CEILING,		"CEILING",	"CEILING",	defaultTranslator);
    ADD_SUPPORT_FN( NUM_FN, SQL_FN_NUM_COS,			"COS",		"COS",		defaultTranslator);
    ADD_SUPPORT_FN( NUM_FN, SQL_FN_NUM_COT, 		"COT", 		"COT",		defaultTranslator);
    ADD_SUPPORT_FN( NUM_FN, SQL_FN_NUM_DEGREES, 	"DEGREES", 	"DEGREES",	defaultTranslator);
    ADD_SUPPORT_FN( NUM_FN, SQL_FN_NUM_EXP, 		"EXP", 		"EXP",		defaultTranslator);
    ADD_SUPPORT_FN( NUM_FN, SQL_FN_NUM_FLOOR, 		"FLOOR", 	"FLOOR",	defaultTranslator);
    ADD_SUPPORT_FN( NUM_FN, SQL_FN_NUM_LOG, 		"LOG", 		"LOG",		defaultTranslator);
    ADD_SUPPORT_FN( NUM_FN, SQL_FN_NUM_LOG10, 		"LOG10", 	"LOG10",	defaultTranslator);
    ADD_SUPPORT_FN( NUM_FN, SQL_FN_NUM_MOD, 		"MOD", 		"MOD",		defaultTranslator);
    ADD_SUPPORT_FN( NUM_FN, SQL_FN_NUM_PI, 			"PI", 		"PI",		defaultTranslator);
    ADD_SUPPORT_FN( NUM_FN, SQL_FN_NUM_POWER, 		"POWER", 	"POWER",	defaultTranslator);
    ADD_SUPPORT_FN( NUM_FN, SQL_FN_NUM_RADIANS, 	"RADIANS", 	"RADIANS",	defaultTranslator);
    ADD_SUPPORT_FN( NUM_FN, SQL_FN_NUM_RAND, 		"RAND", 	"RAND",		defaultTranslator);
    ADD_SUPPORT_FN( NUM_FN, SQL_FN_NUM_ROUND, 		"ROUND", 	"ROUND",	defaultTranslator);
    ADD_SUPPORT_FN( NUM_FN, SQL_FN_NUM_SIGN, 		"SIGN", 	"SIGN",		defaultTranslator);
    ADD_SUPPORT_FN( NUM_FN, SQL_FN_NUM_SIN, 		"SIN", 		"SIN",		defaultTranslator);
    ADD_SUPPORT_FN( NUM_FN, SQL_FN_NUM_SQRT, 		"SQRT", 	"SQRT",		defaultTranslator);
    ADD_SUPPORT_FN( NUM_FN, SQL_FN_NUM_TAN, 		"TAN", 		"TAN",		defaultTranslator);
    ADD_SUPPORT_FN( NUM_FN, SQL_FN_NUM_TRUNCATE,	"TRUNCATE", "TRUNCATE", defaultTranslator);

//	Time and Date functions

    ADD_SUPPORT_FN( TD_FN, SQL_FN_TD_CURRENT_DATE, 		"CURRENT_DATE", 	" CURRENT_DATE ",			fullreplaceTranslator);
    ADD_SUPPORT_FN( TD_FN, SQL_FN_TD_CURRENT_TIME, 		"CURRENT_TIME", 	" CURRENT_TIME ",			fullreplaceTranslator);
    ADD_SUPPORT_FN( TD_FN, SQL_FN_TD_CURRENT_TIMESTAMP, "CURRENT_TIMESTAMP"," CURRENT_TIMESTAMP ",		fullreplaceTranslator);
    ADD_SUPPORT_FN( TD_FN, SQL_FN_TD_CURDATE, 			"CURDATE", 			" cast('now' as date)",		fullreplaceTranslator);
    ADD_SUPPORT_FN( TD_FN, SQL_FN_TD_CURTIME, 			"CURTIME", 			" cast('now' as time)",		fullreplaceTranslator);
    ADD_SUPPORT_FN( TD_FN, SQL_FN_TD_DAYNAME, 			"DAYNAME", 			"DAYNAME",				defaultTranslator);
    ADD_SUPPORT_FN( TD_FN, SQL_FN_TD_DAYOFMONTH, 		"DAYOFMONTH", 		" extract(day from ",		bracketfromTranslator);
    ADD_SUPPORT_FN( TD_FN, SQL_FN_TD_DAYOFWEEK, 		"DAYOFWEEK", 		" 1 + extract(weekday from ",	bracketfromTranslator);
    ADD_SUPPORT_FN( TD_FN, SQL_FN_TD_DAYOFYEAR, 		"DAYOFYEAR", 		" extract(yearday from ",	bracketfromTranslator);
//  ADD_SUPPORT_FN( TD_FN, SQL_FN_TD_EXTRACT, 			"EXTRACT", 			"EXTRACT",				defaultTranslator);
    ADD_SUPPORT_FN( TD_FN, SQL_FN_TD_HOUR, 				"HOUR", 			" extract(hour from ",		bracketfromTranslator);
    ADD_SUPPORT_FN( TD_FN, SQL_FN_TD_MINUTE, 			"MINUTE", 			" extract(minute from ",	bracketfromTranslator);
    ADD_SUPPORT_FN( TD_FN, SQL_FN_TD_MONTH, 			"MONTH", 			" extract(month from ",		bracketfromTranslator);
    ADD_SUPPORT_FN( TD_FN, SQL_FN_TD_MONTHNAME, 		"MONTHNAME", 		"MONTHNAME",			defaultTranslator);
    ADD_SUPPORT_FN( TD_FN, SQL_FN_TD_NOW, 				"NOW", 				" cast('now' as timestamp)",fullreplaceTranslator);
    ADD_SUPPORT_FN( TD_FN, SQL_FN_TD_QUARTER,			"QUARTER",			"QUARTER",				defaultTranslator);
    ADD_SUPPORT_FN( TD_FN, SQL_FN_TD_SECOND, 			"SECOND", 			" extract(second from ",	bracketfromTranslator);
    ADD_SUPPORT_FN( TD_FN, SQL_FN_TD_TIMESTAMPADD, 		"TIMESTAMPADD", 	"TIMESTAMPADD",			defaultTranslator);
    ADD_SUPPORT_FN( TD_FN, SQL_FN_TD_TIMESTAMPDIFF, 	"TIMESTAMPDIFF", 	"TIMESTAMPDIFF",		defaultTranslator);
    ADD_SUPPORT_FN( TD_FN, SQL_FN_TD_WEEK, 				"WEEK", 			"WEEK",					defaultTranslator);
    ADD_SUPPORT_FN( TD_FN, SQL_FN_TD_YEAR, 				"YEAR", 			" extract(year from ",		bracketfromTranslator);

//  System functions

    ADD_SUPPORT_FN( SYS_FN, SQL_FN_SYS_DBNAME, 			"DBNAME", 			"DBNAME",			defaultTranslator);
    ADD_SUPPORT_FN( SYS_FN, SQL_FN_SYS_IFNULL, 			"IFNULL", 			"COALESCE",			defaultTranslator);
    ADD_SUPPORT_FN( SYS_FN, SQL_FN_SYS_USERNAME, 		"USER", 			"CURRENT_USER",		fullreplaceTranslator);

//  Convert functions

    ADD_SUPPORT_FN( CVT_FN, SQL_FN_CVT_CONVERT, 		"CONVERT", 			"CONVERT",			convertTranslator);
}

void SupportFunctions::translateNativeFunction ( char *&ptIn, char *&ptOut )
{
	CSupportFunction fn;

	while( *ptIn == ' ' )ptIn++;

	fn.nameSqlFn = ptIn;
	char * end = ptIn;

	while( *end && *end != ' ' && *end != '(' )end++;
	fn.lenSqlFn = (int)( end - ptIn );

	if ( fn.lenSqlFn )
	{
		int ret = listSupportFunctions.Search( &fn );
		if( ret != -1 )
		{
			supportFn = &listSupportFunctions[ ret ];
			(this->*supportFn->translate)( ptIn, ptOut );
		}
	}
}

void SupportFunctions::defaultTranslator ( char *&ptIn, char *&ptOut )
{
	int offset = (int)( ptIn - ptOut );
	lenOut = (int)strlen ( ptOut );
	lenSqlFn = supportFn->lenSqlFn;
	lenFbFn = supportFn->lenFbFn;

	lenSqlFn += offset;
	writeResult ( supportFn->nameFbFn, ptOut );
	ptIn = ptOut;
}

void SupportFunctions::fullreplaceTranslator ( char *&ptIn, char *&ptOut )
{
	lenFbFn = supportFn->lenFbFn;
	lenOut = (int)strlen ( ptOut );

	while( *ptIn && *ptIn != ')' && *ptIn != '}' )ptIn++;

	if( *ptIn != ')' && *ptIn != '}' )
		return;

	lenSqlFn = (int)( ptIn - ptOut );

	if( *ptIn == ')' )
		lenSqlFn++;

	writeResult ( supportFn->nameFbFn, ptOut );
	ptIn = ptOut;
}

// translate {fn CONVERT(value,SQL_INTEGER) }
// to cast(value as integer)
void SupportFunctions::convertTranslator ( char *&ptIn, char *&ptOut )
{
	lenFbFn = supportFn->lenFbFn;
	lenOut = (int)strlen ( ptOut );
	char * paramSqlType, * paramValue, * end;
	int lenSqlType, lenValue;
	const char * type = NULL;

	paramValue = ptIn + supportFn->lenSqlFn;

	while( *paramValue && *paramValue != '(' )
		paramValue++;

	if ( *paramValue != '(' )
		return;

	paramValue++; // '('

	while( *paramValue == ' ' )
		paramValue++;

	end = paramValue;

	while( *end && *end != ',' ) end++;

	if ( *end != ',' )
		return;

	lenValue = (int)( end - paramValue );
	end++; // ','

	paramSqlType = end;

	while( *paramSqlType == ' ' )
		paramSqlType++;

	end = paramSqlType;

	while ( *end && *end!=' ' && *end!=')' ) end++;

	lenSqlType = (int)( end - paramSqlType );

	switch ( lenSqlType )
	{
	case 7:
		if ( !strncasecmp ( paramSqlType, "SQL_BIT", lenSqlType) )
			type = "char character set octets";
		break;
	case 8:
		if ( !strncasecmp ( paramSqlType, "SQL_CHAR", lenSqlType) )
			type = "char";
		else if ( !strncasecmp ( paramSqlType, "SQL_REAL", lenSqlType) )
			type = "float";
		else if ( !strncasecmp ( paramSqlType, "SQL_DATE", lenSqlType) )
			type = "date";
		break;
	case 9:
		if ( !strncasecmp ( paramSqlType, "SQL_FLOAT", lenSqlType) )
			type = "double precision";
		break;
	case 10:
		if ( !strncasecmp ( paramSqlType, "SQL_BIGINT", lenSqlType) )
			type = "bigint";
		else if ( !strncasecmp ( paramSqlType, "SQL_DOUBLE", lenSqlType) )
			type = "double precision";
		else if ( !strncasecmp ( paramSqlType, "SQL_BINARY", lenSqlType) )
			type = "blob";
		break;
	case 11:
		if ( !strncasecmp ( paramSqlType, "SQL_INTEGER", lenSqlType) )
			type = "integer";
		else if ( !strncasecmp ( paramSqlType, "SQL_VARCHAR", lenSqlType) )
			type = "varchar";
		else if ( !strncasecmp ( paramSqlType, "SQL_DECIMAL", lenSqlType) )
			type = "bigint";
		else if ( !strncasecmp ( paramSqlType, "SQL_NUMERIC", lenSqlType) )
			type = "bigint";
		else if ( !strncasecmp ( paramSqlType, "SQL_TINYINT", lenSqlType) )
			type = "char character set octets";
		break;
	case 12:
		if ( !strncasecmp ( paramSqlType, "SQL_SMALLINT", lenSqlType) )
			type = "smallint";
		break;
	case 13:
		if ( !strncasecmp ( paramSqlType, "SQL_VARBINARY", lenSqlType) )
			type = "blob";
		else if ( !strncasecmp ( paramSqlType, "SQL_TYPE_DATE", lenSqlType) )
			type = "date";
		else if ( !strncasecmp ( paramSqlType, "SQL_TYPE_TIME", lenSqlType) )
			type = "time";
		else if ( !strncasecmp ( paramSqlType, "SQL_TIMESTAMP", lenSqlType) )
			type = "timestamp";
		break;
	case 15:
		if ( !strncasecmp ( paramSqlType, "SQL_LONGVARCHAR", lenSqlType) )
			type = "blob sub_type 1";
		break;
//	case 16:
//		if ( !strncasecmp ( paramSqlType, "SQL_INTERVAL_DAY", lenSqlType) )
//			type = "interval sub_type 3";
//		break;
	case 17:
		if ( !strncasecmp ( paramSqlType, "SQL_LONGVARBINARY", lenSqlType) )
			type = "blob";
//		else if ( !strncasecmp ( paramSqlType, "SQL_INTERVAL_YEAR", lenSqlType) )
//			type = "interval sub_type 1";
//		else if ( !strncasecmp ( paramSqlType, "SQL_INTERVAL_HOUR", lenSqlType) )
//			type = "interval sub_type 4";
		break;
	case 18:
		if ( !strncasecmp ( paramSqlType, "SQL_TYPE_TIMESTAMP", lenSqlType) )
			type = "timestamp";
//		else if ( !strncasecmp ( paramSqlType, "SQL_INTERVAL_MONTH", lenSqlType) )
//			type = "interval sub_type 2";
		break;
//	case 19:
//		if ( !strncasecmp ( paramSqlType, "SQL_INTERVAL_MINUTE", lenSqlType) )
//			type = "interval sub_type 5";
//		else if ( !strncasecmp ( paramSqlType, "SQL_INTERVAL_SECOND", lenSqlType) )
//			type = "interval sub_type 6";
//		break;
//	case 24:
//		if ( !strncasecmp ( paramSqlType, "SQL_INTERVAL_DAY_TO_HOUR", lenSqlType) )
//			type = "interval sub_type 8";
//		break;
//	case 26:
//		if ( !strncasecmp ( paramSqlType, "SQL_INTERVAL_YEAR_TO_MONTH", lenSqlType) )
//			type = "interval sub_type 3";
//		else if ( !strncasecmp ( paramSqlType, "SQL_INTERVAL_DAY_TO_MINUTE", lenSqlType) )
//			type = "interval sub_type 9";
//		else if ( !strncasecmp ( paramSqlType, "SQL_INTERVAL_DAY_TO_SECOND", lenSqlType) )
//			type = "interval sub_type 10";
//		break;
//	case 27:
//		if ( !strncasecmp ( paramSqlType, "SQL_INTERVAL_HOUR_TO_MINUTE", lenSqlType) )
//			type = "interval sub_type 11";
//		else if ( !strncasecmp ( paramSqlType, "SQL_INTERVAL_HOUR_TO_SECOND", lenSqlType) )
//			type = "interval sub_type 12";
//		break;
//	case 28:
//		if ( !strncasecmp ( paramSqlType, "SQL_INTERVAL_MINUTE_TO_SECOND", lenSqlType) )
//			type = "interval sub_type 13";
//		break;
	default:
		return;
	}

	if ( !type )
		return;

	while( *end && *end != ')' ) end++;

	if ( *end != ')' )
		return;

	end++; // ')'

	memcpy( ptOut, " cast(", 6 ); ptOut += 6 ;
	memcpy( ptOut, paramValue, lenValue ); ptOut += lenValue ;

	lenSqlFn = (int)( end - ptOut );
// allSize : ' as ' : lentype : ') '
//             4   + lentype +  2
	lenSqlType = (int)strlen(type);
	lenFbFn = 6 + lenSqlType;

	if ( lenSqlFn > lenFbFn )
		memmove ( ptOut, ptOut + lenSqlFn - lenFbFn, lenOut + lenFbFn - lenSqlFn + 1 );
	else if ( lenSqlFn < lenFbFn )
		memmove ( ptOut - lenSqlFn + lenFbFn, ptOut, lenOut + 1 );

	memcpy( ptOut, " as ", 4 ); ptOut += 4 ;
	memcpy( ptOut, type, lenSqlType ); ptOut += lenSqlType ;
	memcpy( ptOut, ") ", 2 ); ptOut += 2 ;
	ptIn = ptOut;
}

void SupportFunctions::bracketfromTranslator ( char *&ptIn, char *&ptOut )
{
	lenFbFn = supportFn->lenFbFn;
	lenOut = (int)strlen ( ptOut );

	while( *ptIn && *ptIn != '(' )ptIn++;

	if( *ptIn != '(' )
		return;

	ptIn++; // '('
	lenSqlFn = (int)( ptIn - ptOut );

	writeResult ( supportFn->nameFbFn, ptOut );
	ptIn = ptOut;
}

}; // end namespace IscDbcLibrary
