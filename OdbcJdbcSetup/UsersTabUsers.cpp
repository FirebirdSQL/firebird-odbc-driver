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

// UsersTabUsers.cpp: Users Tab Users class.
//
//////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>
#include "OdbcJdbcSetup.h"
#include "../IscDbc/Connection.h"
#include "CommonUtil.h"
#include "../SetupAttributes.h"
#include "ServiceClient.h"
#include "ServiceTabCtrl.h"

#undef _TR
#define _TR( id, msg ) msg

namespace OdbcJdbcSetupLibrary {

CUsersTabUsers::CUsersTabUsers() : CUsersTabChild()
{
}

CUsersTabUsers::~CUsersTabUsers()
{
}

CUsersTabUsers* CUsersTabUsers::getObject()
{
	return this;
}

bool CUsersTabUsers::OnInitDialog()
{
	HWND hWndLV = GetDlgItem( hDlg, IDC_USERS_LISTVIEW_LEFT );

	addColumnToListView( hWndLV, 0, "User name", 120 );
	addColumnToListView( hWndLV, 1, "First Name", 120 );
	addColumnToListView( hWndLV, 2, "Middle Name", 120 );
	addColumnToListView( hWndLV, 3, "Last Name", 120 );
	addColumnToListView( hWndLV, 4, "User ID", 60 );
	addColumnToListView( hWndLV, 5, "Group ID", 80 );
	onGetUsersList();

	return true;
}

void CUsersTabUsers::updateData( HWND hDlg, BOOL bSaveAndValidate )
{
	CUsersTabChild::updateData( hDlg, bSaveAndValidate );

	if ( bSaveAndValidate )
	{
		HWND hWndLV = GetDlgItem( hDlg, IDC_USERS_LISTVIEW_LEFT );
		LONG selected = ListView_GetNextItem( hWndLV, (ULONG)-1, LVNI_SELECTED | LVIS_FOCUSED );

		if ( selected != -1 )
		{
			ListView_GetItemText( hWndLV, selected, 0, userName.getBuffer(256), 256 );
			ListView_GetItemText( hWndLV, selected, 1, firstName.getBuffer(256), 256 );
			ListView_GetItemText( hWndLV, selected, 2, middleName.getBuffer(256), 256 );
			ListView_GetItemText( hWndLV, selected, 3, lastName.getBuffer(256), 256 );
			ListView_GetItemText( hWndLV, selected, 4, groupId.getBuffer(256), 256 );
			ListView_GetItemText( hWndLV, selected, 5, userId.getBuffer(256), 256 );
			password.setString( NULL );
			passwordConfirm.setString( NULL );
		}
		else
		{
			clearFields();
		}
	}
}

void CUsersTabUsers::clearFields()
{
	userName.setString( NULL );
	password.setString( NULL );
	passwordConfirm.setString( NULL );
	firstName.setString( NULL );
	middleName.setString( NULL );
	lastName.setString( NULL );
	groupId.setString( NULL );
	userId.setString( NULL );
}

bool CUsersTabUsers::isUserSelected()
{
	HWND hWndLV = GetDlgItem( hDlg, IDC_USERS_LISTVIEW_LEFT );
	return -1 != ListView_GetNextItem( hWndLV, (ULONG)-1, LVNI_SELECTED | LVIS_FOCUSED );
}

bool CUsersTabUsers::addRowToListView()
{
	HWND hWndLV = GetDlgItem( hDlg, IDC_USERS_LISTVIEW_LEFT );

    LV_ITEM lvi = { 0 };
    lvi.mask = LVIF_TEXT;
	int iColumn = 0;

	lvi.iItem = iColumn;
	lvi.iSubItem = 0;
	lvi.pszText = (LPSTR)"User";
	lvi.iItem = ListView_InsertItem( hWndLV, &lvi );
	ListView_SetItemText( hWndLV, lvi.iItem, 1, (LPSTR)"USER" );

	return true;
}

BOOL CALLBACK wndproCUsersTabUsers( HWND hWndChildTab, UINT message, WPARAM wParam, LPARAM lParam )
{
	HWND hWndParent = GetParent( hWndChildTab );
	PUSERS_DIALOG_HEADER tabData = (PUSERS_DIALOG_HEADER)GetWindowLongPtr( hWndParent, GW_USERDATA );
	int iPage = TabCtrl_GetCurSel( hWndParent );
	CUsersTabChild *child = tabData->childTab[iPage];

	switch ( message )
	{
    case WM_INITDIALOG:
		{
			RECT rcTabHdr;

			SetRectEmpty( &rcTabHdr ); 
			TabCtrl_AdjustRect( hWndParent, TRUE, &rcTabHdr );
			tabData->hWndChildTab = hWndChildTab;
			SetWindowPos( tabData->hWndChildTab, NULL, -rcTabHdr.left, -rcTabHdr.top, 0, 0, 
						  SWP_NOSIZE | SWP_NOZORDER );
			child->updateData( hWndChildTab, FALSE );
		}
		return TRUE;

	case WM_DESTROY:
		child->updateData( hWndChildTab );
		return TRUE;

	case WM_NOTIFY:
		if ( IDC_USERS_LISTVIEW_LEFT == (int) wParam )
		{
			LPNMHDR nm = (LPNMHDR)lParam;
			if ( LVN_ITEMCHANGED == nm->code )
			{
				LPNMLISTVIEW nmItem = (LPNMLISTVIEW)lParam;

				if ( ( LVIS_SELECTED | LVIS_FOCUSED ) == nmItem->uNewState )
				{
					JString userName;
					ListView_GetItemText( nm->hwndFrom, nmItem->iItem, 0, userName.getBuffer(256), 256 );

					bool enable = !child->isAdmin( userName );

					EnableWindow( GetDlgItem( child->hDlg, IDC_BUTTON_MOD_USER ), enable );
					EnableWindow( GetDlgItem( child->hDlg, IDC_BUTTON_DEL_USER ), enable );
				}
			}
		}
		return TRUE;

	case WM_COMMAND:
		if ( !child->validateAccountFields() )
			return TRUE;

		if ( child->onCommand( hWndChildTab, LOWORD( wParam ) ) )
			return TRUE;
	}

    return FALSE;
}

bool CUsersTabUsers::onCommand( HWND hWnd, int nCommand )
{
	if ( CUsersTabChild::onCommand( hWnd, nCommand ) )
		return true;

	switch ( nCommand ) 
	{
	case IDC_BUTTON_GET_INFO:
		updateData( hWnd );
		onGetUsersList();
		return true;

	case IDC_BUTTON_ADD_USER:
		{
			CUserDialog dlg( this, "New User" );
			clearFields();
			if ( IDOK == dlg.DoModal() )
			{
				onEditUser( enAddUser );
				onGetUsersList();
			}
		}
		return true;

	case IDC_BUTTON_MOD_USER:
		if ( isUserSelected() )
		{
			CUserDialog dlg( this, "Modify User" );
			updateData( hWnd );
			if ( IDOK == dlg.DoModal() )
			{
				onEditUser( enModUser );

				if ( tabCtrl->user == userName )
				{
					tabCtrl->password = password;
					tabCtrl->updateData( tabCtrl->hDlg, FALSE );
				}

				onGetUsersList();
			}
		}
		return true;

	case IDC_BUTTON_DEL_USER:
		if ( isUserSelected() )
		{
			CUserDialog dlg( this, "Delete User" );
			updateData( hWnd );
			if ( IDOK == dlg.DoModal() )
			{
				onEditUser( enDelUser );
				onGetUsersList();
			}
		}
		return true;
	}

	return false;
}

void CUsersTabUsers::addParameters( CServiceClient &services )
{
	tabCtrl->addParameters( services );

	services.putParameterValue( "userName", userName );
	services.putParameterValue( "userPassword", password );
	services.putParameterValue( "firstName", firstName );
	services.putParameterValue( "middleName", middleName );
	services.putParameterValue( "lastName", lastName );
	services.putParameterValue( "groupId", groupId );
	services.putParameterValue( "userId", userId );
}

void CUsersTabUsers::onEditUser( enumEditUser enOption )
{
	CServiceClient services;
	int countError;
	int lengthOut;
	char bufferOut[1024];

	try
	{

		if ( !services.initServices() )
		{
			// add error message
			return;
		}

		addParameters( services );

		switch ( enOption )
		{
		case enAddUser:
			services.putParameterValue( "addUser", "Y" );
			break;

		case enModUser:
			services.putParameterValue( "modifyUser", "Y" );
			break;

		case enDelUser:
			services.putParameterValue( "deleteUser", "Y" );
			break;

		default:
			// add error message
			return;
		}

		services.startUsersQuery();

		EnableWindow( GetDlgItem( hDlg, IDC_BUTTON_GET_INFO ), FALSE );

		while ( services.nextQuery( bufferOut, sizeof ( bufferOut ), lengthOut, countError ) )
		{
		}

		EnableWindow( GetDlgItem( hDlg, IDC_BUTTON_GET_INFO ), TRUE );
	}
	catch ( std::exception &ex )
	{
		EnableWindow( GetDlgItem( hDlg, IDC_BUTTON_GET_INFO ), TRUE );

		char buffer[1024];
		SQLException &exception = (SQLException&)ex;
		JString text = exception.getText();
		sprintf(buffer, "sqlcode %d, fbcode %d - %s", exception.getSqlcode(), exception.getFbcode(), (const char*)text );
		MessageBox( NULL, buffer, TEXT( "Error!" ), MB_ICONERROR | MB_OK );
	}

	services.closeService();
}

void CUsersTabUsers::onGetUsersList()
{
	CServiceClient services;
	int lengthOut;
	const int sizeOut = 65535;
	char *bufferOut = new char[sizeOut];

	if ( !bufferOut )
	{
		// add error message
		return;
	}

	if ( tabCtrl->user.IsEmpty() || tabCtrl->password.IsEmpty() )
	{
		// add error message
		delete[] bufferOut;
		return;
	}

	try
	{

		if ( !services.initServices() )
		{
			// add error message
			delete[] bufferOut;
			return;
		}

		tabCtrl->addParameters( services );
		services.putParameterValue( "displayUser", "Y" );
		services.startUsersQuery();

		EnableWindow( GetDlgItem( hDlg, IDC_BUTTON_GET_INFO ), FALSE );

		if ( services.nextQueryUserInfo( bufferOut, sizeOut, lengthOut ) )
		{
			int numRow = 0;
			HWND hWndLV = GetDlgItem( hDlg, IDC_USERS_LISTVIEW_LEFT );
			UserInfo *info = (UserInfo*)bufferOut;

            ListView_DeleteAllItems( hWndLV );

			while ( info )
			{
				char tmpBuf[32];
				LV_ITEM lvi = { 0 };
				
				lvi.mask = LVIF_TEXT;

				lvi.iItem = numRow++;
				lvi.iSubItem = 0;
				lvi.pszText = info->userName;
				lvi.iItem = ListView_InsertItem( hWndLV, &lvi );

				if ( info->firstName && *info->firstName )
					ListView_SetItemText( hWndLV, lvi.iItem, 1, info->firstName );

				if ( info->middleName && *info->middleName )
					ListView_SetItemText( hWndLV, lvi.iItem, 2, info->middleName );

				if ( info->lastName && *info->lastName )
					ListView_SetItemText( hWndLV, lvi.iItem, 3, info->lastName );

				sprintf( tmpBuf, "%d", info->userId );
				ListView_SetItemText( hWndLV, lvi.iItem, 4, tmpBuf );

				sprintf( tmpBuf, "%d", info->groupId );
				ListView_SetItemText( hWndLV, lvi.iItem, 5, tmpBuf );

				info = info->next;
			}
		}

		EnableWindow( GetDlgItem( hDlg, IDC_BUTTON_GET_INFO ), TRUE );
	}
	catch ( std::exception &ex )
	{
		EnableWindow( GetDlgItem( hDlg, IDC_BUTTON_GET_INFO ), TRUE );

		char buffer[1024];
		SQLException &exception = (SQLException&)ex;
		JString text = exception.getText();
		sprintf(buffer, "sqlcode %d, fbcode %d - %s", exception.getSqlcode(), exception.getFbcode(), (const char*)text );
		MessageBox( NULL, buffer, TEXT( "Error!" ), MB_ICONERROR | MB_OK );
	}

	delete[] bufferOut;
	services.closeService();
}

bool CUsersTabUsers::createDialogIndirect( CServiceTabUsers *parentTabCtrl )
{
	CUsersTabChild::createDialogIndirect( parentTabCtrl );

	hDlg = CreateDialogIndirect( m_hInstance,
                                 resource,
                                 parent,
                                 (DLGPROC)wndproCUsersTabUsers );
	OnInitDialog();
	return true;
}

bool CUsersTabUsers::buildDlgChild( HWND hWndParent )
{
	WORD *p;
	int nchar;
	DWORD lStyle;

	parent = hWndParent;
	resource = (LPDLGTEMPLATE)LocalAlloc( LPTR, 2048 );
	p = (PWORD)resource;

	lStyle = DS_SETFONT | WS_CHILD | WS_VISIBLE;

	*p++ = LOWORD (lStyle);
	*p++ = HIWORD (lStyle);
	*p++ = 0;          // LOWORD (lExtendedStyle)
	*p++ = 0;          // HIWORD (lExtendedStyle)

	*p++ = 5;          // NumberOfItems

	*p++ = 0;          // x
	*p++ = 0;          // y
	*p++ = 308;        // cx
	*p++ = 114;        // cy
	*p++ = 0;          // Menu
	*p++ = 0;          // Class

	/* copy the title of the dialog */
	nchar = nCopyAnsiToWideChar( p, TEXT( _TR( IDS_DLG_TITLE_SETUP, "Firebird ODBC Service" ) ) );
	p += nchar;

	*p++ = 8;          // FontSize
	nchar = nCopyAnsiToWideChar( p, TEXT( "MS Sans Serif" ) );
	p += nchar;

    TMP_NAMECONTROL   ( "ListControl", IDC_USERS_LISTVIEW_LEFT, "SysListView32",LVS_SHOWSELALWAYS | LVS_REPORT | LVS_ALIGNLEFT | WS_BORDER | WS_TABSTOP,3,3,302,94 )
    TMP_DEFPUSHBUTTON ( "Get info", IDC_BUTTON_GET_INFO,14,100,60,14 )
    TMP_PUSHBUTTON    ( "Add user", IDC_BUTTON_ADD_USER,84,100,60,14 )
    TMP_PUSHBUTTON    ( "Modify user", IDC_BUTTON_MOD_USER,150,100,60,14 )
    TMP_PUSHBUTTON    ( "Delete user", IDC_BUTTON_DEL_USER,216,100,60,14 )

	return true;
}

}; // end namespace OdbcJdbcSetupLibrary
