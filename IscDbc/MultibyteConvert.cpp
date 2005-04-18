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
 *  Copyright (c) 2005 Vladimir Tsvigun
 *  All Rights Reserved.
 */

// MultibyteConvert.cpp: implementation of the MultibyteConvert class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#include <windows.h>
#endif
#include <time.h>
#include <string.h>
#include "IscDbc.h"
#include "Mlist.h"
#include "MultibyteConvert.h"

#define CODE_CHARSETS(name,code,size)	{#name,sizeof(#name)-1,code,size},

namespace IscDbcLibrary {

struct IntlCharsets
{
	char	*name;
	short	lengthName;
	short	code;
	short	size;

} listCharsets[] = {

	CODE_CHARSETS( NONE			,  0, 1 )
	CODE_CHARSETS( OCTETS		,  1, 1 )
	CODE_CHARSETS( ASCII		,  2, 1 )
	CODE_CHARSETS( UNICODE_FSS	,  3, 3 )
	CODE_CHARSETS( NEXT			,  4, 1 )
	CODE_CHARSETS( SJIS_0208	,  5, 2 )
	CODE_CHARSETS( EUJC_0208	,  6, 2 )
	CODE_CHARSETS( JIS_0208		,  7, 2 )
	CODE_CHARSETS( UNICODE_UCS2	,  8, 3 )
	CODE_CHARSETS( DOS737		,  9, 1 )
	CODE_CHARSETS( DOS437		, 10, 1 )
	CODE_CHARSETS( DOS850		, 11, 1 )
	CODE_CHARSETS( DOS865		, 12, 1 )
	CODE_CHARSETS( DOS860		, 13, 1 )
	CODE_CHARSETS( DOS863		, 14, 1 )
	CODE_CHARSETS( DOS775		, 15, 1 )
	CODE_CHARSETS( DOS858		, 16, 1 )
	CODE_CHARSETS( DOS862		, 17, 1 )
	CODE_CHARSETS( DOS864		, 18, 1 )
	CODE_CHARSETS( NEXT			, 19, 1 )
	CODE_CHARSETS( NEXT			, 20, 1 )
	CODE_CHARSETS( ISO8859_1	, 21, 1 )
	CODE_CHARSETS( ISO8859_2	, 22, 1 )
	CODE_CHARSETS( ISO8859_3	, 23, 1 )
	CODE_CHARSETS( NEXT			, 24, 1 )
	CODE_CHARSETS( NEXT			, 25, 1 )
	CODE_CHARSETS( NEXT			, 26, 1 )
	CODE_CHARSETS( NEXT			, 27, 1 )
	CODE_CHARSETS( NEXT			, 28, 1 )
	CODE_CHARSETS( NEXT			, 29, 1 )
	CODE_CHARSETS( NEXT			, 30, 1 )
	CODE_CHARSETS( NEXT			, 31, 1 )
	CODE_CHARSETS( NEXT			, 32, 1 )
	CODE_CHARSETS( NEXT			, 33, 1 )
	CODE_CHARSETS( ISO8859_4	, 34, 1 )
	CODE_CHARSETS( ISO8859_5	, 35, 1 )
	CODE_CHARSETS( ISO8859_6	, 36, 1 )
	CODE_CHARSETS( ISO8859_7	, 37, 1 )
	CODE_CHARSETS( ISO8859_8	, 38, 1 )
	CODE_CHARSETS( ISO8859_9	, 39, 1 )
	CODE_CHARSETS( ISO8859_13	, 40, 1 )
	CODE_CHARSETS( NEXT			, 41, 1 )
	CODE_CHARSETS( NEXT			, 42, 1 )
	CODE_CHARSETS( NEXT			, 43, 1 )
	CODE_CHARSETS( KSC_5601		, 44, 2 )
	CODE_CHARSETS( DOS852		, 45, 1 )
	CODE_CHARSETS( DOS857		, 46, 1 )
	CODE_CHARSETS( DOS861		, 47, 1 )
	CODE_CHARSETS( DOS866		, 48, 1 )
	CODE_CHARSETS( DOS869		, 49, 1 )
	CODE_CHARSETS( CYRL			, 50, 1 )
	CODE_CHARSETS( WIN1250		, 51, 1 )
	CODE_CHARSETS( WIN1251		, 52, 1 )
	CODE_CHARSETS( WIN1252		, 53, 1 )
	CODE_CHARSETS( WIN1253		, 54, 1 )
	CODE_CHARSETS( WIN1254		, 55, 1 )
	CODE_CHARSETS( BIG_5		, 56, 2 )
	CODE_CHARSETS( GB2312		, 57, 2 )
	CODE_CHARSETS( WIN1255		, 58, 1 )
	CODE_CHARSETS( WIN1256		, 59, 1 )
	CODE_CHARSETS( WIN1257		, 60, 1 )
};

#define SIZE_OF_LISTCHARSETS ( sizeof( listCharsets ) / sizeof( *listCharsets ) )

int findCharsetsCode( const char *charset )
{
	IntlCharsets *p = listCharsets;
	IntlCharsets *end = listCharsets + SIZE_OF_LISTCHARSETS;

	while ( p < end )
	{
		if ( !strncasecmp( charset, p->name, p->lengthName ) )
		{
			return p->code;
		}
		++p;
	}

	return 0;
}

int getCharsetSize( const int charsetCode )
{
	if ( charsetCode < 0 || charsetCode > SIZE_OF_LISTCHARSETS )
		return 1;
	return listCharsets[ charsetCode ].size;
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

typedef struct
{ 
	int		cmask; 
	int		cval; 
	int		shift; 
	long	lmask; 
	long	lval; 

} Tab; 

static Tab tab[] = 
{
	0x80, 0x00, 0*6, 0x7F,               0, // 1 byte sequence
	0xE0, 0xC0, 1*6, 0x7FF,           0x80, // 2 byte sequence
	0xF0, 0xE0, 2*6, 0xFFFF,         0x800, // 3 byte sequence
	0xF8, 0xF0, 3*6, 0x1FFFFF,     0x10000, // 4 byte sequence
	0xFC, 0xF8, 4*6, 0x3FFFFFF,   0x200000, // 5 byte sequence
	0xFE, 0xFC, 5*6, 0x7FFFFFFF, 0x4000000, // 6 byte sequence
	0,
}; 

unsigned int fss_mbstowcs( wchar_t *wcs, const char *mbs, unsigned int lengthForMBS )
{
	int l, c0, c;
	bool bContinue = true;
	Tab *t; 
	unsigned int length = 0;

	if ( !mbs || !wcs )
		return 0; 

	const char *mbsEnd = mbs + lengthForMBS;

	do
	{
		l = c0 = *mbs & 0xff;

		for ( t = tab; t->cmask; t++)
		{ 
			++mbs;
			if ( mbs > mbsEnd )
			{
				*wcs = L'\0';
				return length;
			}

			if ( ( c0 & t->cmask ) == t->cval )
			{ 
				l &= t->lmask; 

				if ( l < t->lval )
				{
					bContinue = false;
					break;
				}
				
				*wcs++ = (wchar_t)l;

				if ( (wchar_t)l == L'\0' )
					return length;

				length++;
				break;
			} 

			if ( !*mbs )
			{
				bContinue = false;
				break;
			}

			c = ( *mbs ^ 0x80 ) & 0xFF; 

			if ( c & 0xC0 )
			{
				break;
			}

			l = ( l << 6 ) | c; 
		}

	} while ( bContinue );

	return length;
}

unsigned int fss_wcstombs( char *mbs, const wchar_t *wcs, unsigned int lengthForMBS )
{ 
	long l; 
	int c;
	bool bContinue = true;
	Tab *t; 
	unsigned int length = 0;

	if ( !mbs || !wcs )
		return 0; 

	do
	{
		l = *wcs; 

		for ( t = tab; t->cmask; t++ )
		{ 
			if ( l <= t->lmask )
			{ 
				c = t->shift; 
				*mbs++ = (char)(t->cval | ( l >> c )); 
				++length;
				while ( c > 0 && length < lengthForMBS )
				{ 
					c -= 6; 
					*(mbs++) = 0x80 | ( ( l >> c ) & 0x3F ); 
					++length;
				} 
				break;
			} 
		} 

	} while ( *(++wcs) != L'\0' && bContinue && length < lengthForMBS );

	return length;
}

}; // end namespace IscDbcLibrary
