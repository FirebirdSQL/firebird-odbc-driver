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

extern UINT codePage; // from Main.cpp

size_t _MbsToWcs( wchar_t *wcstr, const char *mbstr, size_t count )
{
	size_t len = MultiByteToWideChar( codePage,
									  0,
									  mbstr,
									  -1,
									  wcstr,
									  !wcstr ? 0 : (int)count );
	if ( len > 0 )
		len--;
	else if ( wcstr )
		len = count;

	return len;
}

size_t _WcsToMbs( char *mbstr,  const wchar_t *wcstr, size_t count )
{
	size_t len = WideCharToMultiByte( codePage,
									  0,
									  wcstr,
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
