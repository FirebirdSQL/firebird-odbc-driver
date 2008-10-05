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

// CommonUtil.cpp: Service Util class.
//
//////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "OdbcJdbcSetup.h"
#include "CommonUtil.h"

int nCopyAnsiToWideChar( LPWORD lpWCStr, LPCSTR lpAnsiIn )
{
	int cchAnsi = lstrlen( lpAnsiIn );
	return MultiByteToWideChar( GetACP(), MB_PRECOMPOSED, lpAnsiIn, cchAnsi, (LPWSTR)lpWCStr, cchAnsi ) + 1;
}

LPWORD lpwAlign( LPWORD lpIn )
{
	uintptr_t ul;

	ul = (uintptr_t)lpIn;
	ul += 3;
	ul >>= 2;
	ul <<= 2;
	return (LPWORD)ul;
}
