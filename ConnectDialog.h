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
 *  The Original Code was created by Vladimir Tsvigun for IBPhoenix.
 *
 *  Copyright (c) 2003 Vladimir Tsvigun
 *  All Rights Reserved.
 */

#if !defined(_CONDIALOG_H_INCLUDED_)
#define _CONDIALOG_H_INCLUDED_

#ifdef _WIN32

// ConnectDialog.h : header file
//
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CConnectDialog dialog

class CConnectDialog
{
// Construction
public:
	CConnectDialog();   // standard constructor
	~CConnectDialog();

// Dialog Data
	enum { IDD = IDD_CON_PROPERTIES };
	JString	m_user;
	JString	m_password;
	JString	m_role;

public:
	int DoModal();

	void UpdateData(HWND hDlg, BOOL bSaveAndValidate = TRUE );
	BOOL OnInitDialog(HWND hDlg);
};

#endif // _WIN32

#endif // !defined(_CONDIALOG_H_INCLUDED_)
