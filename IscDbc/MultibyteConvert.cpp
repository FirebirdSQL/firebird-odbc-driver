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
#include <sql.h>
#include <time.h>
#include <string.h>
#include "IscDbc.h"
#include "Mlist.h"
#include "MultibyteConvert.h"

#ifdef _WINDOWS
#define CODE_CHARSETS(name,code,size,codepage,codeset)	{#name,sizeof(#name)-1,code,size,codepage},
#else
#define CODE_CHARSETS(name,code,size,codepage,codeset)	{#name,sizeof(#name)-1,code,size,codeset},
#endif // _WINDOWS

namespace
{

#ifdef _WINDOWS
// We cannot left it undefined
#define SQLWCHAR_ENCODING nullptr
#else
#ifndef __BYTE_ORDER__
#error "Undefined byte order"
#endif
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
const char constexpr *SQLWCHAR_ENCODING = sizeof(SQLWCHAR) == 2 ? "UTF16LE" : "UTF32LE";
#else
const char constexpr *SQLWCHAR_ENCODING = sizeof(SQLWCHAR) == 2 ? "UTF16BE" : "UTF32BE";
#endif
#endif

struct IntlCharsets
{
	static constexpr unsigned INVALID_CP = 0xBADC0DE;
	char	*name;
	short	lengthName;
	short	code;
	short	size;
#ifdef _WINDOWS
	unsigned	codePage;
#else
	const char* codeset;
#endif // _WINDOWS

} constexpr listCharsets[] = {

	CODE_CHARSETS( NONE			,  0, 1, 0, "" )
	CODE_CHARSETS( OCTETS		,  1, 1, IntlCharsets::INVALID_CP, nullptr )
	CODE_CHARSETS( ASCII		,  2, 1, 20127, "ANSI_X3.4-1968" )
	CODE_CHARSETS( UNICODE_FSS	,  3, 3, 65001, "UTF-8" )
	CODE_CHARSETS( UTF8			,  4, 4, 65001, "UTF-8" )
	CODE_CHARSETS( SJIS_0208	,  5, 2, 932, "CP932" )
	CODE_CHARSETS( EUJC_0208	,  6, 2, 20932, "EUJP-MS" )
	CODE_CHARSETS( JIS_0208		,  7, 2, 20932, "EUC-JP" )
	CODE_CHARSETS( UNICODE_UCS2	,  8, 3, IntlCharsets::INVALID_CP, "UCS-2" )
	CODE_CHARSETS( DOS737		,  9, 1, 737, "CP737" )
	CODE_CHARSETS( DOS437		, 10, 1, 437, "CP437" )
	CODE_CHARSETS( DOS850		, 11, 1, 850, "CP850" )
	CODE_CHARSETS( DOS865		, 12, 1, 865, "CP865" )
	CODE_CHARSETS( DOS860		, 13, 1, 860, "CP860" )
	CODE_CHARSETS( DOS863		, 14, 1, 863, "CP863" )
	CODE_CHARSETS( DOS775		, 15, 1, 775, "CP775" )
	CODE_CHARSETS( DOS858		, 16, 1, 858, "CP858" )
	CODE_CHARSETS( DOS862		, 17, 1, 862, "CP862" )
	CODE_CHARSETS( DOS864		, 18, 1, 864, "CP864" )
	CODE_CHARSETS( NEXT			, 19, 1, IntlCharsets::INVALID_CP, nullptr )
	CODE_CHARSETS( UNUSED		, 20, 1, IntlCharsets::INVALID_CP, nullptr )
	CODE_CHARSETS( ISO8859_1	, 21, 1, 28591, "ISO-8859_1" )
	CODE_CHARSETS( ISO8859_2	, 22, 1, 28592, "ISO-8859_2" )
	CODE_CHARSETS( ISO8859_3	, 23, 1, 28593, "ISO-8859_3" )
	CODE_CHARSETS( UNUSED		, 24, 1, IntlCharsets::INVALID_CP, nullptr )
	CODE_CHARSETS( UNUSED		, 25, 1, IntlCharsets::INVALID_CP, nullptr )
	CODE_CHARSETS( UNUSED		, 26, 1, IntlCharsets::INVALID_CP, nullptr )
	CODE_CHARSETS( UNUSED		, 27, 1, IntlCharsets::INVALID_CP, nullptr )
	// Abuse this otherwise unused code for platform-dependent excoding of SQLWCHAR
	CODE_CHARSETS( UNUSED		, 28, 2, IntlCharsets::INVALID_CP, SQLWCHAR_ENCODING )
	CODE_CHARSETS( UNUSED		, 29, 2, IntlCharsets::INVALID_CP, nullptr )
	CODE_CHARSETS( UNUSED		, 30, 1, IntlCharsets::INVALID_CP, nullptr )
	CODE_CHARSETS( UNUSED		, 31, 1, IntlCharsets::INVALID_CP, nullptr )
	CODE_CHARSETS( UNUSED		, 32, 1, IntlCharsets::INVALID_CP, nullptr )
	CODE_CHARSETS( UNUSED		, 33, 1, IntlCharsets::INVALID_CP, nullptr )
	CODE_CHARSETS( ISO8859_4	, 34, 1, 28594, "ISO-8859_4" )
	CODE_CHARSETS( ISO8859_5	, 35, 1, 28595, "ISO-8859_5" )
	CODE_CHARSETS( ISO8859_6	, 36, 1, 28596, "ISO-8859_6" )
	CODE_CHARSETS( ISO8859_7	, 37, 1, 28597, "ISO-8859_7" )
	CODE_CHARSETS( ISO8859_8	, 38, 1, 28598, "ISO-8859_8" )
	CODE_CHARSETS( ISO8859_9	, 39, 1, 28599, "ISO-8859_9" )
	CODE_CHARSETS( ISO8859_13	, 40, 1, 28603, "ISO-8859_13" )
	CODE_CHARSETS( UNUSED		, 41, 1, IntlCharsets::INVALID_CP, nullptr )
	CODE_CHARSETS( UNUSED		, 42, 1, IntlCharsets::INVALID_CP, nullptr )
	CODE_CHARSETS( UNUSED		, 43, 1, IntlCharsets::INVALID_CP, nullptr )
	CODE_CHARSETS( KSC_5601		, 44, 2, 949, "CP949" )
	CODE_CHARSETS( DOS852		, 45, 1, 852, "CP852" )
	CODE_CHARSETS( DOS857		, 46, 1, 857, "CP857" )
	CODE_CHARSETS( DOS861		, 47, 1, 861, "CP861" )
	CODE_CHARSETS( DOS866		, 48, 1, 866, "CP866" )
	CODE_CHARSETS( DOS869		, 49, 1, 869, "CP869" )
	CODE_CHARSETS( CYRL			, 50, 1, 1251, "CP1251" )
	CODE_CHARSETS( WIN1250		, 51, 1, 1250, "CP1250" )
	CODE_CHARSETS( WIN1251		, 52, 1, 1251, "CP1251" )
	CODE_CHARSETS( WIN1252		, 53, 1, 1252, "CP1252" )
	CODE_CHARSETS( WIN1253		, 54, 1, 1253, "CP1253" )
	CODE_CHARSETS( WIN1254		, 55, 1, 1254, "CP1254" )
	CODE_CHARSETS( BIG_5		, 56, 2, 950, "BIG-5" )
	CODE_CHARSETS( GB2312		, 57, 2, 936, "GB2312" )
	CODE_CHARSETS( WIN1255		, 58, 1, 1255, "CP1255" )
	CODE_CHARSETS( WIN1256		, 59, 1, 1256, "CP1256" )
	CODE_CHARSETS( WIN1257		, 60, 1, 1257, "CP1257" )
	CODE_CHARSETS( UNUSED		, 61, 1, IntlCharsets::INVALID_CP, nullptr )
	CODE_CHARSETS( UNUSED		, 62, 1, IntlCharsets::INVALID_CP, nullptr )
	CODE_CHARSETS( KOI8R		, 63, 1, 20866, "KOI8R" )
	CODE_CHARSETS( KOI8U		, 64, 1, 21866, "KOI8U" )
	CODE_CHARSETS( WIN1258		, 65, 1, 1258, "CP1258" )
	CODE_CHARSETS( TIS620		, 66, 1, 874, "TIS620" )
	CODE_CHARSETS( GBK			, 67, 1, IntlCharsets::INVALID_CP, "GBK" )
	CODE_CHARSETS( CP943C		, 68, 1, IntlCharsets::INVALID_CP, nullptr )
	CODE_CHARSETS( GB18030		, 69, 1, 54936, "GB18030" )
};

#define SIZE_OF_LISTCHARSETS ( sizeof( listCharsets ) / sizeof( *listCharsets ) )

static constexpr bool checkTheList()
{
	for (int i = 0; i < SIZE_OF_LISTCHARSETS; i++)
	{
		if (listCharsets[i].code != i)
			return false;
	}
	return true;
}

static_assert(checkTheList(), "Some charset missing");

typedef struct
{
	unsigned int		cmask;
	unsigned int		cval;
	unsigned int		shift;
	unsigned int		lmask;
	unsigned int		lval;

} Tab;

static Tab tab[] =
{
	0x80, 0x00, 0, 0x7F,               0, // 1 byte sequence
	0xE0, 0xC0, 1, 0x7FF,           0x80, // 2 byte sequence
	0xF0, 0xE0, 2, 0xFFFF,         0x800, // 3 byte sequence
	0xF8, 0xF0, 3, 0x1FFFFF,     0x10000, // 4 byte sequence
	0xFC, 0xF8, 4, 0x3FFFFFF,   0x200000, // 5 byte sequence
	0xFE, 0xFC, 5, 0x7FFFFFFF, 0x4000000, // 6 byte sequence
	0,
};

// These functions almost match IConv::convert() specs, except
// wcsLength is in characters, not bytes
static int fss_mbstowcs(const char* &mbs, ssize_t &mbsLength, SQLWCHAR* &wcs, size_t& wcsLength)
{
	size_t length = 0;

	const char *mbsEnd = mbs + mbsLength;

	if (wcs != nullptr)
	{
		while (mbs < mbsEnd && length < wcsLength)
		{
			unsigned c0 = static_cast<unsigned char>(*mbs);
			SQLWCHAR l = c0;

			// If we have an incomplete input, mbs must not be advanced.

			for (Tab* t = tab; t->cmask; t++)
			{
				if ( ( c0 & t->cmask ) == t->cval )
				{
					if (mbs + t->shift >= mbsEnd)
					{
						// Incomplete input sequence
						wcsLength = length;
						return Convert::Error::CS_INCOMPLETE_INPUT;
					}

					l &= ~t->cmask;

					// This routine is supposed to convert data from server and
					// the data is already validated so invalid sequence is impossible.
					// Omit check for that.

					switch (t->shift)
					{
					case 5:
						{
							l << 6;
							l |= *(++mbs) & 0x3f;
							[[fallthrough]];
						}
					case 4:
						{
							l << 6;
							l |= *(++mbs) & 0x3f;
							[[fallthrough]];
						}
					case 3:
						{
							l << 6;
							l |= *(++mbs) & 0x3f;
							[[fallthrough]];
						}
					case 2:
						{
							l << 6;
							l |= *(++mbs) & 0x3f;
							[[fallthrough]];
						}
					case 1:
						{
							l << 6;
							l |= *(++mbs) & 0x3f;
							mbsLength -= t->shift;
							break;
						}
					}

					// Ignore overlong encoding

					// Get out of inner loop
					break;
				}
			}
			// If this loop ended "naturally", input sequence is invalid.
			// Disregard this case and consume one byte in any case.

			++mbs;
			--mbsLength;
			*wcs++ = l;
			++length;
		}
	}
	else
	{
		// Just calculate total length without further checks
		while (mbs < mbsEnd)
		{
			unsigned c0 = static_cast<unsigned char>(*mbs);

			for (Tab* t = tab; t->cmask; t++)
			{
				if ( ( c0 & t->cmask ) == t->cval )
				{
					if (mbs + t->shift >= mbsEnd)
					{
						// Incomplete input sequence
						wcsLength = length;
						return Convert::Error::CS_INCOMPLETE_INPUT;
					}

					mbs += t->shift;
					mbsLength -= t->shift;

					break;
				}
			}
			++mbs;
			--mbsLength;
			++length;
		}
	}

	wcsLength = length;

	return 0;
}

static int fss_wcstombs(const SQLWCHAR* &wcs, ssize_t& wcsLength, char* &mbs, size_t& mbsLength)
{
	size_t length = 0;

	// Conditions "wcs == nullptr" and "wcsLength == 0" should be already checked by calling code

	if (mbs != NULL)
	{
		do
		{
			unsigned l = *wcs;

			for (const Tab* t = tab; t->cmask; t++ )
			{
				if ( l <= t->lmask )
				{
					unsigned c = t->shift;
					if (length < mbsLength - c) // "All or nothing"
					{
						*mbs++ = (char)(t->cval | ( l >> (c * 6) ));
						++length;
						while ( c > 0 )
						{
							--c;
							*(mbs++) = (char)(0x80 | ( ( l >> (c * 6) ) & 0x3F ));
							++length;
						}
						++wcs;
					}
					else
					{
						mbsLength = length;
						return 0;
					}
					break;
				}
			}

		} while ( --wcsLength > 0 );
	}
	else
	{
		do
		{
			unsigned l = *wcs++;

			for (const Tab* t = tab; t->cmask; t++ )
			{
				if ( l <= t->lmask )
				{
					// Just count needed bytes
					length += t->shift + 1;
					break;
				}
			}

		} while ( --wcsLength > 0 );
	}

	mbsLength = length;
	return 0;
}

#define	CS_CANT_MAP		0		// Flag table entries that don't map

#define U_IS_SURROGATE_LEAD(c) (((c)&0x400)==0)
#define U_IS_SURROGATE(c) (((c)&0xfffff800)==0xd800)

#define U_IS_UNICODE_NONCHAR(c) \
    ((c)>=0xfdd0 && \
     ((uint32_t)(c)<=0xfdef || ((c)&0xfffe)==0xfffe) && \
     (uint32_t)(c)<=0x10ffff)

#define U16_LEAD(supplementary) (uint32_t)(((supplementary)>>10)+0xd7c0)
#define U16_TRAIL(supplementary) (uint32_t)(((supplementary)&0x3ff)|0xdc00)
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
    5, 5        // illegal bytes 0xfe and 0xff
};

//static const UChar32
//utf8_minLegal[4]={ 0, 0x80, 0x800, 0x10000 };
#define UTF8_COUNT_TRAIL_BYTES(leadByte) (utf8_countTrailBytes[(uint8_t)leadByte])
#define UTF8_MASK_LEAD_BYTE(leadByte, countTrailBytes) ((leadByte)&=(1<<(7-(countTrailBytes)))-1)
#define UTF8_ERROR_VALUE_1 0x15
#define UTF8_ERROR_VALUE_2 0x9f
#define UTF_ERROR_VALUE 0xffff
#define U_SENTINEL (-1)

//static const UChar32 utf8_errorValue[6] =
//{
//    UTF8_ERROR_VALUE_1,
//	UTF8_ERROR_VALUE_2,
//	UTF_ERROR_VALUE,
//	0x10ffff,
//    0x3ffffff,
//	0x7fffffff
//};

static int utf8_mbstowcs(const char* &mbs, ssize_t& mbsLength, SQLWCHAR* &wcs, size_t& wcsLength)
{
	int err_code = 0;

	const char* const mbsEnd = mbs + mbsLength;
	const SQLWCHAR* const wcsEnd = wcs + wcsLength;
	size_t length = 0;

	while (mbs < mbsEnd)
	{
		uint32_t c = static_cast<unsigned char>(*mbs);
		uint8_t count = UTF8_COUNT_TRAIL_BYTES( c );

		if (mbs + count < mbsEnd)
		{
			uint8_t trail = 0, illegal = 0;
			size_t i = 0;

			UTF8_MASK_LEAD_BYTE( c, count );

			switch ( count )
			{
			// each branch falls through to the next one
			case 5:
			case 4:
				// count>=4 is always illegal: no more than 3 trail bytes in Unicode's UTF-8
				illegal = 1;
				break;

			case 3:
				trail = mbs[++i];
				c = ( c <<  6) | ( trail & 0x3f );
				illegal |= ( trail & 0xc0 ) ^ 0x80;
				[[fallthrough]];
			case 2:
				trail = mbs[++i];
				c = ( c <<  6) | ( trail & 0x3f );
				illegal |= ( trail & 0xc0 ) ^ 0x80;
				[[fallthrough]];
			case 1:
				trail = mbs[++i];
				c = ( c <<  6) | ( trail & 0x3f );
				illegal |= ( trail & 0xc0 ) ^ 0x80;
				break;

			case 0:
				break;
			}

/* Firebird is a trusted source, skip validation
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
			else if ( c > 0x10ffff )	// code point>0x10ffff, outside Unicode
			{
				error_code = Convert::Error::CS_BAD_INPUT;
				break;
			}
			else if ( strict > 0 && U_IS_UNICODE_NONCHAR( c ) )
			{
				//  strict: forbid non-characters like U+fffe
				c = utf8_errorValue[ count ];
			}
*/
		}
		else //  too few bytes left
		{
			err_code = Convert::Error::CS_INCOMPLETE_INPUT;
			break;
		}

		// Atomic flush character if fits
		if constexpr (sizeof(SQLWCHAR) == 4) // iODBC
		{
			if (wcs != nullptr)
			{
				if (wcs >= wcsEnd)
				{
					break;
				}
				*wcs++ = c;
			}
			++length;
		}
		else if ( c <= 0xFFFF )
		{
			if (wcs != nullptr)
			{
				if (wcs >= wcsEnd)
				{
					break;
				}
				*wcs++ = c;
			}
			++length;
		}
		else
		{
			if (wcs != nullptr)
			{
				if (wcs + 1 >= wcsEnd)
				{
					break;
				}
				*wcs++ = U16_LEAD( c );
				*wcs++ = U16_TRAIL( c );
			}
			length += 2;
		}
		++count; // Include leading byte
		mbs += count;
		mbsLength -= count;
	}

	wcsLength = length;

	return err_code;
}

static int utf8_wcstombs(const SQLWCHAR* &wcs, ssize_t& wcsLength, char* &mbs, size_t& mbsLength)
{
	int err_code = 0;

	// if ( !wcs || !*wcs ) already checked in the calling code

	const SQLWCHAR* wcsOrg = wcs;
	const SQLWCHAR* wcsEnd = wcs + wcsLength;
	const char* const mbsEnd = mbs + mbsLength;
	size_t length = 0;

	while (wcsLength > 0)
	{
		uint32_t c = *wcs;
		size_t wcsLen = 1;

		if constexpr (sizeof(SQLWCHAR) == 2) // UTF-16
		{
			if ( U_IS_SURROGATE( c ) )
			{
				uint32_t c2;

				if ( U_IS_SURROGATE_LEAD( c ) )
				{
					if (wcs + 1 >= wcsEnd)
					{
						err_code = Convert::Error::CS_INCOMPLETE_INPUT;
						break;
					}
					if (U16_IS_TRAIL( c2 = wcs[1] ) )
					{
						c = U16_GET_SUPPLEMENTARY( c, c2 );
						wcsLen = 2;
					}
					else
					{
						err_code = Convert::Error::CS_BAD_INPUT;
						break;
					}
				}
			}

		}
		else // UTF-32
		{
			// Actually nothing to do here, no surrogates in UTF-32
		}

		size_t mbsLen = U8_LENGTH( c );
		if (mbs != nullptr)
		{
			if (mbsLen <= mbsEnd - mbs)
			{
				switch (mbsLen)
				{
				case 1:
					{
						*mbs++ = (uint8_t)c;
						break;
					}
				case 2:
					{
						*mbs++ = (uint8_t)( (c >> 6) | 0xc0 );
						*mbs++ = (uint8_t)( ( c & 0x3f ) | 0x80 );
						break;
					}
				case 3:
					{
						*mbs++ = (uint8_t)( ( c >> 12 ) | 0xe0 );
						*mbs++ = (uint8_t)((( c >> 6 ) & 0x3f ) | 0x80 );
						*mbs++ = (uint8_t)( ( c & 0x3f ) | 0x80 );
						break;
					}
				case 4:
					{
						*mbs++ = (uint8_t)( ( c >> 18 ) | 0xf0 );
						*mbs++ = (uint8_t)((( c >> 12 ) & 0x3f ) | 0x80 );
						*mbs++ = (uint8_t)((( c >> 6 ) & 0x3f ) | 0x80 );
						*mbs++ = (uint8_t)( ( c & 0x3f ) | 0x80 );
					}
				default: // WTH to do with length 0???
					break;
				}

			}
			else
			{
				// Truncated output is not an error
				break;
			}
		}

		length += mbsLen;
		wcsLength -= wcsLen;
		wcs += wcsLen;
	}

	mbsLength = length;
	return err_code;
}

} // end namespace

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

int Convert::fromWcs(const SQLWCHAR* wcs, ssize_t wcsLength, char* mbs, size_t& mbsLength)
{
	if (wcs == nullptr)
	{
		mbsLength = 0;
		return 0;
	}

	if (wcsLength == SQL_NTS)
	{
		wcsLength = SQLWCHAR_len(wcs);
	}

	if (wcsLength == 0)
	{
		mbsLength = 0;
		return 0;
	}

	if (mbs == nullptr)
	{
		// Ignore garbage
		mbsLength = 0;
		return 0;
	}

	int ret = 0; // ?

	switch (charsetCode)
	{
	case Charset::Code::Fss:
		{
			ret = fss_wcstombs(wcs, wcsLength, mbs, mbsLength);
			break;
		}
	case Charset::Code::Utf8:
		{
			ret = utf8_wcstombs(wcs, wcsLength, mbs, mbsLength);
			break;
		}
	default:
		{
			int ret = iMbs.init(Charset::Code::SqlWchar, charsetCode);
			if (ret != 0)
			{
				return ret;
			}
			const char* inBuf = reinterpret_cast<const char*>(wcs);
			ssize_t inLen = wcsLength * sizeof(SQLWCHAR);

			ret = iMbs.convert(inBuf, inLen, mbs, mbsLength);
			if (inLen != 0)
			{
				// Some bytes left unconverted but they will get no second chance.
				// Reset possible non-clear state.
				iMbs.reset();
			}

			wcsLength = inLen / sizeof(SQLWCHAR);
			break;
		}
	}

	if (ret != 0)
	{
		return ret;
	}

	if (wcsLength != 0)
	{
		// Some data left unconverted.
		ret = Convert::Error::CS_TRUNCATION_ERROR;
	}
	return ret;
}

int Convert::toWcs(const char* mbs, ssize_t mbsLength, SQLWCHAR* wcs, size_t& wcsLength)
{
	if (mbs == nullptr)
	{
		if (wcs != nullptr)
		{
			*wcs = '\0';
		}
		wcsLength = 0;
		return 0;
	}

	if (mbsLength == SQL_NTS)
	{
		mbsLength = strlen(mbs);
	}

	if (mbsLength == 0)
	{
		if (wcs != nullptr)
		{
			*wcs = '\0';
		}
		wcsLength = 0;
		return 0;
	}

	int ret = 0;
	size_t length = 0;

	if (wcs == nullptr)
	{
		wcsLength = 0;
	}
	else
	{
		length = wcsLength - 1; // Rezerve one codepoint to <NUL>
		switch (charsetCode)
		{
		case Charset::Code::Fss:
			{
				ret = fss_mbstowcs(mbs, mbsLength, wcs, length);
				break;
			}
		case Charset::Code::Utf8:
			{
				ret = utf8_mbstowcs(mbs, mbsLength, wcs, length);
				break;
			}
		default:
			{
				int ret = iWcs.init(charsetCode, Charset::Code::SqlWchar);
				if (ret != 0)
				{
					return ret;
				}
				char* outBuf = reinterpret_cast<char*>(wcs);
				size_t outLen = length * sizeof(SQLWCHAR);

				ret = iWcs.convert(mbs, mbsLength, outBuf, outLen);

				wcs = reinterpret_cast<SQLWCHAR*>(outBuf);
				length = outLen / sizeof(SQLWCHAR);
				break;
			}
		}
		// Force null-termination per ODBC specs
		*wcs = '\0';
	}

	if (ret != 0 && ret != Convert::Error::CS_TRUNCATION_ERROR)
	{
		return ret;
	}

	if (mbsLength != 0)
	{
		// Some data left unconverted, calculate total length.
		size_t extraLength = 0;
		switch (charsetCode)
		{
		case Charset::Code::Fss:
			{
				SQLWCHAR* outBuf = nullptr;
				ret = fss_mbstowcs(mbs, mbsLength, outBuf, extraLength);
				break;
			}
		case Charset::Code::Utf8:
			{
				SQLWCHAR* outBuf = nullptr;
				ret = utf8_mbstowcs(mbs, mbsLength, outBuf, extraLength);
				break;
			}
		default:
			{
				// Previous block can be skipped so this initialization is not redunant
				int ret = iWcs.init(charsetCode, Charset::Code::SqlWchar);
				if (ret != 0)
				{
					return ret;
				}

				char* outBuf = nullptr;
				size_t outLen = 0;

				ret = iWcs.convert(mbs, mbsLength, outBuf, outLen);

				extraLength = outLen / sizeof(SQLWCHAR);
				break;
			}
		}
		length += extraLength;
	}
	wcsLength = length;

	return ret;
}

void Convert::setCharsetCode(short code)
{
	if (code >= 0 && code < SIZE_OF_LISTCHARSETS)
	{
		charsetCode = code;
	}
	// else just ignore it because here is no way to report any error.
}


#ifndef _WINDOWS
int Convert::IConv::init(short fromCharsetCode, short toCharsetCode)
{
	if (cd == (iconv_t)-1)
	{
		cd = iconv_open(listCharsets[toCharsetCode].codeset, listCharsets[fromCharsetCode].codeset);
		if (cd == (iconv_t)-1)
		{
			return errno;
		}
	}
	return 0;
}

int Convert::IConv::convert(const char* &inBuf, ssize_t& inBytesLeft, char* &outBuf, size_t& outBytes)
{
	if (outBuf == nullptr)
	{
		// Just calculate number of needed bytes.
		// It is pity that iconv cannot operate without output buffer.
		char buf[1024];
		outBytes = 0;

		while (inBytesLeft > 0)
		{
			char* p = buf;
			size_t l = sizeof(buf);
			int res = iconv(cd, const_cast<char**>(&inBuf), reinterpret_cast<size_t*>(&inBytesLeft), &p, &l);
			if (res == (size_t)-1)
			{
				if (errno != E2BIG)
				{
					// Reset conversion state to prevent dirty output on the next call
					iconv(cd, nullptr, nullptr, nullptr, nullptr);
					return errno;
				}
			}
			outBytes += p - buf;
		}
	}
	else
	{
		char* begin = outBuf;
		int res = iconv(cd, const_cast<char**>(&inBuf), reinterpret_cast<size_t*>(&inBytesLeft), &outBuf, &outBytes);
		if (res == (size_t)-1)
		{
			if (errno != E2BIG)
			{
				// Reset conversion state to prevent dirty output on the next call
				iconv(cd, nullptr, nullptr, nullptr, nullptr);
				return errno;
			}
		}
		outBytes = outBuf - begin;
	}

	return 0;
}

void Convert::IConv::reset()
{
	iconv(cd, nullptr, nullptr, nullptr, nullptr);
}
#else

#endif
