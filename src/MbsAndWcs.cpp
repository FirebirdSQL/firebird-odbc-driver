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
 *  Copyright (c) 2007 Vladimir Tsvigun
 *  All Rights Reserved.
 */

#ifdef _WINDOWS
#include <windows.h>
#include <stdio.h>

#include "IscDbc/Connection.h"

extern UINT codePage; // from Main.cpp

// Phase 12 (12.1.1): Use ODBC_SQLWCHAR* (always 16-bit) instead of wchar_t*.
// On Windows, ODBC_SQLWCHAR == wchar_t == 2 bytes, so this is binary-compatible.
size_t _MbsToWcs( ODBC_SQLWCHAR *wcstr, const char *mbstr, size_t count )
{
	if (wcstr && !count) return count;

	size_t len = MultiByteToWideChar( codePage,
									  0,
									  mbstr,
									  -1,
									  (LPWSTR)wcstr,
									  !wcstr ? 0 : (int)count );
	if ( len > 0 )
		len--;
	else if ( wcstr )
		len = count;

	return len;
}

size_t _WcsToMbs( char *mbstr,  const ODBC_SQLWCHAR *wcstr, size_t count )
{
	if (mbstr && !count) return count;

	size_t len = WideCharToMultiByte( codePage,
									  0,
									  (LPCWSTR)wcstr,
									  -1,
									  (LPSTR)mbstr,
									  !mbstr ? 0 : (int)count,
									  NULL,
									  NULL );
	if ( len > 0 )
		len--;
	else if ( mbstr )
		len = count;

	return len;
}

#endif // _WINDOWS
