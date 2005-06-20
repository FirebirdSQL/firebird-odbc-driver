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

// UsersTabMemberShips.h interface for the UsersTabMemberShips class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(_UsersTabMemberShips_h_)
#define _UsersTabMemberShips_h_

namespace OdbcJdbcSetupLibrary {

using namespace classJString;

/////////////////////////////////////////////////////////////////////////////
// CUsersTabMemberShips dialog

class CUsersTabMemberShips : public CUsersTabChild
{
public:
	CUsersTabMemberShips();
	virtual ~CUsersTabMemberShips();

public:
	CUsersTabMemberShips* getObject();
	bool buildDlgChild( HWND hWndParent );
	bool createDialogIndirect( CServiceTabUsers *parentTabCtrl );
	bool OnInitDialog();
	bool addRowToListView();

public:
};

}; // end namespace OdbcJdbcSetupLibrary

#endif // !defined(_UsersTabMemberShips_h_)
