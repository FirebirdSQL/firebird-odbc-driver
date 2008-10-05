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

// UserDialog.h interface for the User Dialog class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(_UserDialog_h_)
#define _UserDialog_h_

namespace OdbcJdbcSetupLibrary {

using namespace classJString;

class CUsersTabUsers;
/////////////////////////////////////////////////////////////////////////////
// CUserDialog dialog

class CUserDialog
{
public:
	CUserDialog( CUsersTabUsers *parentTab, const char *headDlg );
	~CUserDialog();

public:
	intptr_t  DoModal();
	bool OnInitDialog( HWND hWndDlg );
	void updateData( HWND hDlg, BOOL bSaveAndValidate = TRUE );
	bool onCommand( HWND hWnd, int nCommand );
	bool validateFields();

public:
	CUsersTabUsers  *parent;
	HWND            hDlg;
	const char      *headerDlg;
};

}; // end namespace OdbcJdbcSetupLibrary

#endif // !defined(_UserDialog_h_)
