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
 *  The Original Code was created by Vladimir Tsvigun for IBPhoenix.
 *
 *  Copyright (c) 2003 Vladimir Tsvigun
 *  All Rights Reserved.
 */

// SupportFunctions.h
//
//////////////////////////////////////////////////////////////////////

#if !defined(_SUPPORTFUNCTIONS_H_)
#define _SUPPORTFUNCTIONS_H_

#include <stdlib.h>

namespace IscDbcLibrary {

#ifdef _WIN32
#define strcasecmp		stricmp
#define strncasecmp		strnicmp
#endif

class SupportFunctions;
typedef void (SupportFunctions::*ADRESS_FUNCTION)( char *&ptIn, char *&ptOut );

class CSupportFunction
{
public:
	int				typeFn;
	int				keySqlFn;
	const char		*nameSqlFn;
	int				lenSqlFn;
	const char		*nameFbFn;
	int				lenFbFn;
	ADRESS_FUNCTION translate;

	CSupportFunction()
	{
		remove();
	}
	void set( int type, long key, const char * SqlName, int lenSqlName, const char * FbName, int lenFbName, ADRESS_FUNCTION transl )
	{
		typeFn = type;
		keySqlFn = key;
		nameSqlFn = SqlName;
		lenSqlFn = lenSqlName;
		nameFbFn = FbName;
		lenFbFn = lenFbName;
		translate = transl;
	}
	void remove()
	{ 
		typeFn = 0;
		keySqlFn = 0;
		nameSqlFn = NULL;
		lenSqlFn = 0;
		nameFbFn = NULL;
		lenFbFn = 0;
		translate = NULL;
	}
	CSupportFunction & operator =(const CSupportFunction & src)
	{ 
		typeFn = src.typeFn;
		keySqlFn = src.keySqlFn;
		nameSqlFn = src.nameSqlFn;
		lenSqlFn = src.lenSqlFn;
		nameFbFn = src.nameFbFn;
		lenFbFn = src.lenFbFn;
		translate = src.translate;
		return  *this;
	}
};

class CSupportFunctionComparator
{
public:
	static int compare(const CSupportFunction *a, const CSupportFunction *b) 
	{
	    if ( a->lenSqlFn < b->lenSqlFn ) return -1;
	    else if ( a->lenSqlFn > b->lenSqlFn ) return 1;
	    return strncasecmp ( a->nameSqlFn, b->nameSqlFn, a->lenSqlFn );
	}
};

typedef MList<CSupportFunction, CSupportFunctionComparator> ListSupportFunctions;

class SupportFunctions
{
public:
	enum ScalarFunctions { STR_FN, NUM_FN, TD_FN, SYS_FN };
	CSupportFunction * supportFn;

	SupportFunctions();

	ListSupportFunctions listSupportFunctions;

	void translateNativeFunction ( char *&ptIn, char *&ptOut );

	void defaultTranslator (  char *&ptIn, char *&ptOut )
	{
		int lenSqlFn = supportFn->lenSqlFn;
		int lenFbFn = supportFn->lenFbFn;
		int offset = ptIn - ptOut;
		int lendst = strlen ( ptOut );

		lenSqlFn += offset;

		if ( lenSqlFn > lenFbFn )
			memmove ( ptOut, ptOut + lenSqlFn - lenFbFn, lendst + lenFbFn - lenSqlFn + 1 );
		else if ( lenSqlFn < lenFbFn )
			memmove ( ptOut - lenSqlFn + lenFbFn, ptOut, lendst + 1 );

		const char * src = supportFn->nameFbFn;
		while ( *src )
			*ptOut++ = *src++;
	}
};

}; // end namespace IscDbcLibrary

#endif // !defined(_SUPPORTFUNCTIONS_H_)
