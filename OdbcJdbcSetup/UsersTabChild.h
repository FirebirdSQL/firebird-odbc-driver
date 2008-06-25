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

// UsersTabChild.h interface for the UsersTabChild class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(_UsersTabChild_h_)
#define _UsersTabChild_h_

namespace OdbcJdbcSetupLibrary {

using namespace classJString;

/////////////////////////////////////////////////////////////////////////////
// CUsersTabChild dialog
class CServiceTabUsers;

class CUsersTabChild
{
public:
	CUsersTabChild();
	virtual ~CUsersTabChild();
	virtual void updateData( HWND hDlg, BOOL bSaveAndValidate = TRUE );
	virtual bool onCommand( HWND hWnd, int nCommand );
	virtual bool validateAccountFields();
	virtual bool buildDlgChild( HWND hWndParent );
	virtual bool isAdmin( const char *userName );
	virtual bool createDialogIndirect( CServiceTabUsers *parentTabCtrl );
	void addColumnToListView( HWND hWnd, int i, char *name, int width );

	void notImplemented( HWND hWndLV );

public:
	CUsersTabChild* getObject();

public:
	CServiceTabUsers *tabCtrl;
	HWND             parent;
	HWND             hDlg;
	LPDLGTEMPLATE    resource;
};

extern HINSTANCE m_hInstance;

}; // end namespace OdbcJdbcSetupLibrary

#endif // !defined(_UsersTabChild_h_)
