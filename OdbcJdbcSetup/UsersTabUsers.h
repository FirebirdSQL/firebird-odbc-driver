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

// UsersTabUsers.h interface for the UsersTabUsers class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(_UsersTabUsers_h_)
#define _UsersTabUsers_h_

namespace OdbcJdbcSetupLibrary {

using namespace classJString;

/////////////////////////////////////////////////////////////////////////////
// CUsersTabUsers dialog

class CUsersTabUsers : public CUsersTabChild
{
	enum enumEditUser
	{ 	
		enAddUser			= 0x01,
		enModUser			= 0x02,
		enDelUser			= 0x04
	};

public:
	CUsersTabUsers();
	virtual ~CUsersTabUsers();

protected:
	void clearFields();
	bool isUserSelected();

public:
	CUsersTabUsers* getObject();
	bool onCommand( HWND hWnd, int nCommand );
	void onEditUser( enumEditUser enOption );
	void onGetUsersList();
	bool buildDlgChild( HWND hWndParent );
	void addParameters( CServiceClient &services );
	bool createDialogIndirect( CServiceTabUsers *parentTabCtrl );
	bool OnInitDialog();
	void updateData( HWND hDlg, BOOL bSaveAndValidate = TRUE );
	bool addRowToListView();

public:
	JString userName;
	JString firstName;
	JString middleName;
	JString lastName;
	JString password;
	JString passwordConfirm;
	JString roleName;
	JString groupId;
	JString userId;
};

}; // end namespace OdbcJdbcSetupLibrary

#endif // !defined(_UsersTabUsers_h_)
