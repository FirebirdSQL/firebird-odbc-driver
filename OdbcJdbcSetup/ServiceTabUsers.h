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

// ServiceTabUsers.h interface for the Service Users class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(_ServiceTabUsers_h_)
#define _ServiceTabUsers_h_

namespace OdbcJdbcSetupLibrary {

/////////////////////////////////////////////////////////////////////////////
// CServiceTabUsers dialog

class CServiceTabUsers;
class CUsersTabChild;

typedef struct _USERS_DIALOG_HEADER
{ 
	CServiceTabUsers   *tabCtrl;
	HWND               hWndTab;
	HWND               hWndChildTab;
	CUsersTabChild*    childTab[3];

} USERS_DIALOG_HEADER, *PUSERS_DIALOG_HEADER;

class CServiceTabUsers : public CServiceTabChild
{
	enum
	{ 	
		enValidateDb        = 0x0001,
		enSweepDb           = 0x0002,
		enMendDb            = 0x0004,
		enListLimboTrans    = 0x0008,
		enCheckDb           = 0x0010,
		enIgnoreChecksum    = 0x0020,
		enKillShadows       = 0x0040,
		enFull              = 0x0080,

		enFixListLimboTrans = 0x1000
	};

	enum enumUsersLimboTransactions
	{ 	
		enCommitTrans       = 0x01,
		enRollbackTrans     = 0x02,
		enRecoverTwoPhase   = 0x04
	};

public:
	CServiceTabUsers();
	~CServiceTabUsers();

public:
	void updateData( HWND hDlg, BOOL bSaveAndValidate = TRUE );
	bool OnInitDialog( HWND hDlg );
	bool onCommand( HWND hWnd, int nCommand );
	void addParameters( CServiceClient &services );
	bool createDialogIndirect( CServiceTabCtrl *parentTabCtrl );
	bool buildDlgChild( HWND hWndParent );
	bool buildDlgTabUsers( void );

public:
	HWND                  hWndDlg;
	HWND                  hWndParent;
	USERS_DIALOG_HEADER   tabData;
	CUsersTabMemberShips  members;
	CUsersTabRoles        roles;
	CUsersTabUsers        users;
};

}; // end namespace OdbcJdbcSetupLibrary

#endif // !defined(_ServiceTabUsers_h_)
