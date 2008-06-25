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
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 */


// OdbcJdbcSetup.h : main header file for the ODBCJDBCSETUP DLL
//

#if !defined(_ODBCJDBCSETUP_H_INCLUDED_)
#define _ODBCJDBCSETUP_H_INCLUDED_

#include <windows.h>
#include "../IscDbc/JString.h"

#ifdef _WINDOWS
#include <windows.h>

#if _MSC_VER >= 1400 // VC80 and later
#define strcasecmp		_stricmp
#define strncasecmp		_strnicmp
#else
#define strcasecmp		stricmp
#define strncasecmp		strnicmp
#endif // _MSC_VER >= 1400

#define snprintf		_snprintf
#define swprintf		_snwprintf
#endif

#define ISLOWER(c)			((c) >= 'a' && (c) <= 'z')
#define UPPER(c)			((ISLOWER (c)) ? (c) - 'a' + 'A' : (c))
#define IS_END_TOKEN(c)		((c) == '\0' || (c) == ';' || (c) == '\n' || (c) == '\r' || (c) == '\t')
#define IS_CHECK_YES(c)		((c) == 'Y' || (c) == '1')
#define IS_CHECK_NO(c)		((c) == 'N' || (c) == '0')

#include "resource.h"		// main symbols

#endif // !defined(_ODBCJDBCSETUP_H_INCLUDED_)
