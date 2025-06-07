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

// MultibyteConvert.h
//
//////////////////////////////////////////////////////////////////////
//
//  Firebird API uses data in a connection charset (mostly)
//  ODBC API uses data either in ANSI or in Unicode
//  thus we have two conversion vectors: ANSI <-> CS and WIDE <-> CS.
//
//  ODBC specs require strings returned from driver to be null-terminated
//  while Firebird API has separate length variable(s)
//  thus conversions WIDE -> CS and ANSI -> CS don't need to terminate the result
//  (or rather must not because there is no room in Firebird message buffer for that)
//  while conversions CS -> WIDE and CS -> ANSI must do it.
//
//  ODBC has conception of piecewise data transfer and generally ODBC strings
//  can be much longer that Firebird so conversions to/from CLOB must be piecewise
//  with special care about blob segments border cut multibyte sequence in half.
//
//  Fortunately, conversions ANSI <-> WIDE are not needed (?)
//
//  A side note: what must not be overseen is error messages. They can contain
//  object names and data, and are converted into connection charset. With connection
//  charset NONE a message about UC violation can become quite messy with constraint
//  and field names in UTF-8 but data in field's charset (WIN1252 for example).
//  Because of this it is better to enforce usage of a connection charset defaulting
//  it to UTF-8

#pragma once

#ifndef _WINDOWS
#include <iconv.h>
#else
#include <winerror.h>
#endif
#include <errno.h>

#include <sqltypes.h>

namespace Charset
{
	enum Code : short // To avoid magic numbers in code. Must correspond to IntlCharsets.code
	{
		None = 0,
		Fss = 3,
		Utf8 = 4,
		SqlWchar = 28	// Either UTF-16 or UTF-32 depending on size of SQLWCHAR
	};
}

// May be not optimal but it solves the problematic case sizeof(SQLWCHAR) != sizeof(wchar_t)
// which prevent standard functions from been used.
inline size_t SQLWCHAR_len(const SQLWCHAR* wcs)
{
	for (size_t result = 0; ; ++result)
	{
		if (*wcs++ == 0)
		{
			return result;
		}
	}
}

class Convert
{
	struct IConv
	{
	#ifndef _WINDOWS
		iconv_t cd = (iconv_t)-1;

		~IConv()
		{
			if (cd != (iconv_t)-1)
			{
				iconv_close(cd);
			}
		}
	#else
		UINT srcCodepage;
		UINT tgtCodepage;
	#endif

		int init(short fromCharsetCode, short toCharsetCode);
		// On input outBytes is size of outBuf, on return outBytes is number of used bytes.
		// If outBuf is nullptr, input value of outBytes is ignored.
		// On return inBuf is advanced and inBytesLeft is number of bytes left unconverted.
		int convert(const char* &inBuf, ssize_t& inBytesLeft, char* &outBuf, size_t& outBytes);
		// Force reset conversion state.
		void reset();
	};

	IConv iMbs, iWcs;
	short charsetCode = Charset::Code::None;

public:
	static constexpr unsigned maxUtfBytes = 4;

	// Conversion error codes
	enum Error
	{
#ifdef _WINDOWS
		CS_TRUNCATION_ERROR	= ERROR_BUFFER_TOO_SMALL,
		CS_BAD_INPUT		= ERROR_NO_UNICODE_TRANSLATION,
		CS_INCOMPLETE_INPUT	= EINVAL	// Not the best choice but MultiByteToWideChar() doesn't treat incomplete sequence separately from invalid sequence anyway
#else
		CS_TRUNCATION_ERROR	= E2BIG,
		CS_BAD_INPUT		= EILSEQ,	// input string detected as bad
		CS_INCOMPLETE_INPUT	= EINVAL
#endif
	};

	Convert() = default;
	Convert(short charset)
	{
		setCharsetCode(charset);
	}

	// Has no effect after any conversion has been done.
	void setCharsetCode(short code);

	// wcsLength is length of wcs in characters (SQL_NTS is ok).
	// mbsLength on input is size of buffer in bytes, on output used number of bytes.
	// Return value is 0 on success, an error code otherwise.
	int fromWcs(const SQLWCHAR* wcs, ssize_t wcsLength, char* mbs, size_t& mbsLength);

	// These functions always convert as much as possible and always terminate output string with <NUL>.
	// Still they return full length of output as per ODBC specs.
	// Insufficient buffer is not considered an error and should be detected by returned xxxLength.

	// wcsLength is in characters, the rest is as above.
	int toWcs(const char* mbs, ssize_t mbsLength, SQLWCHAR* wcs, size_t& wcsLength);

	// In contrast to function above stops as soon as output buffer is full.

	// mbsLength on return is the number of consumed bytes. It should make calling code
	// simpler in the case if input is a stream, not just a big buffer.
	int toWcsPiecewise(const char* mbs, ssize_t& mbsLength, SQLWCHAR* wcs, size_t& wcsLength);
};

int findCharsetsCode(const char* charset);
int getCharsetSize(int charsetCode);
