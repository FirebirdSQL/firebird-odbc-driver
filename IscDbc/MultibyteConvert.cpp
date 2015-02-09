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

#ifdef _WINDOWS
#include <windows.h>
#else
#include <wchar.h>
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

} const listCharsets[] = {

	CODE_CHARSETS( NONE			,  0, 1 )
	CODE_CHARSETS( OCTETS		,  1, 1 )
	CODE_CHARSETS( ASCII		,  2, 1 )
	CODE_CHARSETS( UNICODE_FSS	,  3, 3 )
	CODE_CHARSETS( UTF8			,  4, 4 )
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
	const IntlCharsets *p = listCharsets;
	const IntlCharsets *end = listCharsets + SIZE_OF_LISTCHARSETS;

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

int getCharsetSize( int charsetCode )
{
	if ( charsetCode > 0 ) charsetCode &= 0xff;
	if ( charsetCode < 0 || charsetCode > SIZE_OF_LISTCHARSETS )
		return 1;
	return listCharsets[ charsetCode ].size;
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

WCSTOMBS adressWcsToMbs( int charsetCode )
{
	switch ( charsetCode )
	{
	case 3: // UNICODE_FSS
		return (WCSTOMBS)fss_wcstombs;
	case 4: // UTF8
		return (WCSTOMBS)utf8_wcstombs;
	case 0: // NONE
	default:
		break;
	}

#ifdef _WINDOWS
	return _WcsToMbs;
#else
	return wcstombs;
#endif
}

MBSTOWCS adressMbsToWcs( int charsetCode )
{
	switch ( charsetCode )
	{
	case 3: // UNICODE_FSS
		return (MBSTOWCS)fss_mbstowcs;
	case 4: // UTF8
		return (MBSTOWCS)utf8_mbstowcs;
	case 0: // NONE
	default:
		break;
	}

#ifdef _WINDOWS
	return _MbsToWcs;
#else
	return mbstowcs;
#endif
}

typedef struct
{ 
	int		cmask; 
	int		cval; 
	int		shift; 
	int		lmask; 
	int		lval; 

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

	if ( !mbs || !*mbs )
		return 0; 

	const char *mbsEnd = mbs + lengthForMBS;

	if ( wcs != NULL )
	{
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
	}
	else
	{
		do
		{
			l = c0 = *mbs & 0xff;

			for ( t = tab; t->cmask; t++)
			{ 
				++mbs;
				if ( mbs > mbsEnd )
				{
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
	}

	return length;
}

unsigned int fss_wcstombs( char *mbs, const wchar_t *wcs, unsigned int lengthForMBS )
{ 
	int l; 
	int c;
	Tab *t; 
	unsigned int length = 0;

	if ( !wcs || !*wcs )
		return 0; 

	if ( mbs != NULL )
	{
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
					while ( c > 0 )
					{ 
						c -= 6; 
						*(mbs++) = (char)(0x80 | ( ( l >> c ) & 0x3F ));
						++length;
					} 
					break;
				} 
			} 

		} while ( *(++wcs) != L'\0' );
	}
	else
	{
		do
		{
			l = *wcs; 

			for ( t = tab; t->cmask; t++ )
			{ 
				if ( l <= t->lmask )
				{ 
					c = t->shift; 
					++length;
					while ( c > 0 )
					{ 
						c -= 6; 
						++length;
					} 
					break;
				} 
			} 

		} while ( *(++wcs) != L'\0' );
	}

	return length;
}


// Conversion error codes
#define	CS_TRUNCATION_ERROR	1	// output buffer too small
#define	CS_CONVERT_ERROR	2	// can't remap a character
#define	CS_BAD_INPUT		3	// input string detected as bad

#define	CS_CANT_MAP		0		// Flag table entries that don't map

#ifndef USHORT
typedef unsigned short USHORT;
#endif
typedef signed int int32_t;
typedef int32_t UChar32;
typedef wchar_t UChar;
typedef signed char int8_t;
typedef int8_t UBool;
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

UChar32  utf8_nextCharSafeBody( const uint8_t *s,
							    int32_t *pi,
								int32_t length,
								UChar32 c,
								UBool strict );

#define U_IS_SURROGATE_LEAD(c) (((c)&0x400)==0)
#define U_IS_SURROGATE(c) (((c)&0xfffff800)==0xd800)

#define U_IS_UNICODE_NONCHAR(c) \
    ((c)>=0xfdd0 && \
     ((uint32_t)(c)<=0xfdef || ((c)&0xfffe)==0xfffe) && \
     (uint32_t)(c)<=0x10ffff)

#define U16_LEAD(supplementary) (UChar)(((supplementary)>>10)+0xd7c0)
#define U16_TRAIL(supplementary) (UChar)(((supplementary)&0x3ff)|0xdc00)
#define U16_IS_TRAIL(c) (((c)&0xfffffc00)==0xdc00)
#define U16_SURROGATE_OFFSET ((0xd800<<10UL)+0xdc00-0x10000)
#define U16_GET_SUPPLEMENTARY(lead, trail) \
    (((lead)<<10UL)+(trail)-U16_SURROGATE_OFFSET)

#define U8_IS_TRAIL(c) (((c)&0xc0)==0x80)

#define U8_LENGTH(c) \
    ((uint32_t)(c) <= 0x7f ? 1 : \
        ((uint32_t)(c) <= 0x7ff ? 2 : \
            ((uint32_t)(c) <= 0xd7ff ? 3 : \
                ((uint32_t)(c) <= 0xdfff || (uint32_t)(c) > 0x10ffff ? 0 : \
                    ((uint32_t)(c)<=0xffff ? 3 : 4)\
                ) \
            ) \
        ) \
    )

//
// This table could be replaced on many machines by
// a few lines of assembler code using an
// "index of first 0-bit from msb" instruction and
// one or two more integer instructions.
//
// For example, on an i386, do something like
// - MOV AL, leadByte
// - NOT AL         (8-bit, leave b15..b8==0..0, reverse only b7..b0)
// - MOV AH, 0
// - BSR BX, AX     (16-bit)
// - MOV AX, 6      (result)
// - JZ finish      (ZF==1 if leadByte==0xff)
// - SUB AX, BX (result)
// -finish:
// (BSR: Bit Scan Reverse, scans for a 1-bit, starting from the MSB)
//
// In Unicode, all UTF-8 byte sequences with more than 4 bytes are illegal;
// lead bytes above 0xf4 are illegal.
// We keep them in this table for skipping long ISO 10646-UTF-8 sequences.
//
const uint8_t utf8_countTrailBytes[256] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,

    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3,
    3, 3, 3,    // illegal in Unicode
    4, 4, 4, 4, // illegal in Unicode
    5, 5,       // illegal in Unicode
    0, 0        // illegal bytes 0xfe and 0xff
};

static const UChar32
utf8_minLegal[4]={ 0, 0x80, 0x800, 0x10000 };
#define UTF8_COUNT_TRAIL_BYTES(leadByte) (utf8_countTrailBytes[(uint8_t)leadByte])
#define UTF8_MASK_LEAD_BYTE(leadByte, countTrailBytes) ((leadByte)&=(1<<(6-(countTrailBytes)))-1)
#define UTF8_ERROR_VALUE_1 0x15
#define UTF8_ERROR_VALUE_2 0x9f
#define UTF_ERROR_VALUE 0xffff
#define U_SENTINEL (-1)

static const UChar32 utf8_errorValue[6] = 
{
    UTF8_ERROR_VALUE_1,
	UTF8_ERROR_VALUE_2,
	UTF_ERROR_VALUE,
	0x10ffff,
    0x3ffffff,
	0x7fffffff
};

unsigned int utf8_mbstowcs( wchar_t *wcs, const char *mbs, unsigned int lengthForMBS )
{
	USHORT err_code = 0;
	ULONG err_position = 0;

	if ( !wcs )
		return lengthForMBS * sizeof( *wcs );

	const UCHAR* mbsOrg = (const UCHAR*)mbs;
	const UCHAR* const mbsEnd = mbsOrg + lengthForMBS;
	const USHORT* const wcsStart = (const USHORT*)wcs;

	for ( ULONG i = 0; i < lengthForMBS; )
	{
		UChar32 c = mbsOrg[i++];

		if ( c <= 0x7F )
		{
			if ( !c )
				break;
			*wcs++ = c;
		}
		else
		{
			err_position = i - 1;

			c = utf8_nextCharSafeBody( mbsOrg,
									   reinterpret_cast<int32_t*>(&i),
									   lengthForMBS,
									   c,
									   -1 );

			if ( c < 0 )
			{
				err_code = CS_BAD_INPUT;
				break;
			}
			else if ( c <= 0xFFFF )
				*wcs++ = c;
			else
			{
				*wcs++ = U16_LEAD( c );
				*wcs++ = U16_TRAIL( c );
			}
		}
	}

	*wcs = L'\0';
	return (const USHORT*)wcs - wcsStart;
}

unsigned int utf8_wcstombs( char *mbs, const wchar_t *wcs, unsigned int lengthForMBS )
{
	USHORT err_code = 0;
	ULONG err_position = 0;
	ULONG wcsLen = (ULONG)wcslen( wcs );

	if ( !wcs || !*wcs )
		return 0; 

	if ( !mbs )
		return wcsLen * 4;

	const USHORT* wcsOrg = (const USHORT*)wcs;
	const USHORT* const wcsEnd = wcsOrg + wcsLen;
	UCHAR* mbsOrg = (UCHAR*)mbs;
	const UCHAR* const mbsStart = (const UCHAR*)mbsOrg;
	const UCHAR* const mbsEnd = (const UCHAR*)mbsOrg + lengthForMBS;

	for ( ULONG i = 0; i < wcsLen; )
	{
		if ( !(mbsEnd - mbsOrg) )
		{
			err_code = CS_TRUNCATION_ERROR;
			err_position = i * sizeof( *wcsOrg );
			break;
		}

		UChar32 c = wcsOrg[i++];

		if ( c <= 0x7F )
		{
			if ( !c )
				break;
			*mbsOrg++ = c;
		}
		else
		{
			err_position = (i - 1) * sizeof( *wcsOrg );

			if ( U_IS_SURROGATE( c ) )
			{
				UChar32 c2;

				if ( U_IS_SURROGATE_LEAD( c ) 
					&& wcsOrg + i < wcsEnd
					&& U16_IS_TRAIL( c2 = wcsOrg[i] ) )
				{
					++i;
					c = U16_GET_SUPPLEMENTARY( c, c2 );
				}
				else
				{
					err_code = CS_BAD_INPUT;
					break;
				}
			}

			if ( U8_LENGTH( c ) <= mbsEnd - mbsOrg )
			{
				if ( (uint32_t)c <= 0x7f )
					*mbsOrg++ = (uint8_t)c;
				else
				{
					if ( (uint32_t)c <= 0x7ff )
						*mbsOrg++ = (uint8_t)( (c >> 6) | 0xc0 );
					else
					{
						if ( (uint32_t)c <= 0xffff )
							*mbsOrg++ = (uint8_t)( ( c >> 12 ) | 0xe0 );
						else
						{
							*mbsOrg++ = (uint8_t)( ( c >> 18 ) | 0xf0 );
							*mbsOrg++ = (uint8_t)(( ( c >> 12 ) & 0x3f ) | 0x80 );
						}
						*mbsOrg++ = (uint8_t)(( ( c >> 6 ) & 0x3f ) | 0x80 );
					}
					*mbsOrg++ = (uint8_t) ( ( c & 0x3f ) | 0x80 );
				}
			}
			else
			{
				err_code = CS_TRUNCATION_ERROR;
				break;
			}
		}
	}

	*mbsOrg = '\0';
	return (unsigned int)( ( mbsOrg - mbsStart ) * sizeof( *mbs ) );
}

UChar32  utf8_nextCharSafeBody( const uint8_t *s,
							    int32_t *pi,
								int32_t length,
								UChar32 c,
								UBool strict )
{
    int32_t i = *pi;
    uint8_t count = UTF8_COUNT_TRAIL_BYTES( c );

    if ( i + count <= length )
	{
        uint8_t trail, illegal = 0;

        UTF8_MASK_LEAD_BYTE( c, count );

        // count==0 for illegally leading trail bytes and the illegal bytes 0xfe and 0xff
        switch ( count )
		{
		// each branch falls through to the next one 
        case 5:
        case 4:
            // count>=4 is always illegal: no more than 3 trail bytes in Unicode's UTF-8
            illegal = 1;
            break;

        case 3:
            trail = s[i++];
            c = ( c <<  6) | ( trail & 0x3f );
            if ( c < 0x110 )
                illegal |= ( trail & 0xc0 ) ^ 0x80;
			else
			{
                // code point>0x10ffff, outside Unicode
                illegal = 1;
                break;
            }
        case 2:
            trail = s[i++];
            c = ( c <<  6) | ( trail & 0x3f );
            illegal |= ( trail & 0xc0 ) ^ 0x80;
        case 1:
            trail = s[i++];
            c = ( c <<  6) | ( trail & 0x3f );
            illegal |= ( trail & 0xc0 ) ^ 0x80;
            break;

        case 0:
            if( strict >= 0 )
                return UTF8_ERROR_VALUE_1;
			else
                return U_SENTINEL;
        // no default branch to optimize switch()  - all values are covered
        }

         // All the error handling should return a value
         // that needs count bytes so that UTF8_GET_CHAR_SAFE() works right.
         // 
         // Starting with Unicode 3.0.1, non-shortest forms are illegal.
         // Starting with Unicode 3.2, surrogate code points must not be
         // encoded in UTF-8, and there are no irregular sequences any more.
         // 
         // U8_ macros (new in ICU 2.4) return negative values for error conditions.

        //  correct sequence - all trail bytes have (b7..b6)==(10)?
        //  illegal is also set if count>=4
		// 
        if ( illegal || c < utf8_minLegal[count] || U_IS_SURROGATE( c ) )
		{
            //  error handling
            uint8_t errorCount = count;
            //  don't go beyond this sequence
            i = *pi;

            while ( count > 0 && U8_IS_TRAIL( s[i] ) )
			{
                ++i;
                --count;
            }
            
			if ( strict >= 0 )
                c = utf8_errorValue[ errorCount - count ];
			else 
                c = U_SENTINEL;
        }
		else if ( strict > 0 && U_IS_UNICODE_NONCHAR( c ) )
		{
            //  strict: forbid non-characters like U+fffe
            c = utf8_errorValue[ count ];
        }
    }
	else //  too few bytes left
	{
        //  error handling
        int32_t i0 = i;
        //  don't just set (i)=(length) in case there is an illegal sequence

        while( i < length && U8_IS_TRAIL( s[i] ) )
            ++i;

        if ( strict >= 0 )
            c = utf8_errorValue[ i - i0 ];
		else
            c = U_SENTINEL;
    }

    *pi = i;
    return c;
}

}; // end namespace IscDbcLibrary
