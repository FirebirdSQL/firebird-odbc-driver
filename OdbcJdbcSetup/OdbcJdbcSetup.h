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


// OdbcJdbcSetup.h : main header file for the ODBCJDBCSETUP DLL
//

#if !defined(AFX_ODBCJDBCSETUP_H__23E7040B_14AB_11D4_98DD_0000C01D2301__INCLUDED_)
#define AFX_ODBCJDBCSETUP_H__23E7040B_14AB_11D4_98DD_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// COdbcJdbcSetupApp
// See OdbcJdbcSetup.cpp for the implementation of this class
//

class COdbcJdbcSetupApp : public CWinApp
{
public:
	COdbcJdbcSetupApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COdbcJdbcSetupApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(COdbcJdbcSetupApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ODBCJDBCSETUP_H__23E7040B_14AB_11D4_98DD_0000C01D2301__INCLUDED_)
