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
 */

// OdbcJdbcSetup.cpp : Defines the initialization routines for the DLL.
//
#include "OdbcJdbcSetup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//
// DllMain should return a value of 1 if
//
HINSTANCE m_hInstance = NULL;

BOOL APIENTRY DllMain(  HINSTANCE hinstDLL, DWORD fdwReason, LPVOID )
{
	if ( fdwReason == DLL_PROCESS_ATTACH )
		m_hInstance = hinstDLL;

    return TRUE;
}
