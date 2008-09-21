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

// ServiceTabCtrl.h interface for the ServiceTabCtrl class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(_ServiceTabCtrl_h_)
#define _ServiceTabCtrl_h_

#include "ServiceTabChild.h"
#include "ServiceTabBackup.h"
#include "ServiceTabRestore.h"
#include "ServiceTabRepair.h"
#include "ServiceTabStatistics.h"
#include "UserDialog.h"
#include "UsersTabChild.h"
#include "UsersTabMemberShips.h"
#include "UsersTabRoles.h"
#include "UsersTabUsers.h"
#include "ServiceTabUsers.h"

namespace OdbcJdbcSetupLibrary {

/////////////////////////////////////////////////////////////////////////////
// CServiceTabCtrl dialog

class CServiceTabCtrl
{
public:
	CServiceTabCtrl( HWND hDlgParent );
	~CServiceTabCtrl();

public:
	intptr_t DoModal();

	void SetDisabledDlgItem( HWND hDlg, int ID, BOOL bDisabled = TRUE );
	void UpdateData( HWND hDlg, BOOL bSaveAndValidate = TRUE );
	bool OnInitDialog( HWND hDlg );
	bool setExecutorForViewLogFile();

public:
	HWND                  hWndDlg;
	HWND                  hWndParent;
	TAG_DIALOG_HEADER     tabData;
	CServiceTabBackup     backup;
	CServiceTabRestore    restore;
	CServiceTabStatistics statistic;
	CServiceTabRepair     repair;
	CServiceTabUsers      users;
	JString               client;
	JString               database;
	JString               password;
	JString               user;
	JString               role;
	JString               executorViewLogFile;
};

}; // end namespace OdbcJdbcSetupLibrary

#endif // !defined(_ServiceTabCtrl_h_)
