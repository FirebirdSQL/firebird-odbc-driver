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

// UsersTabMemberShips.cpp: Users Tab MemberShips class.
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

CUsersTabMemberShips::CUsersTabMemberShips() : CUsersTabChild()
{
}

CUsersTabMemberShips::~CUsersTabMemberShips()
{
}

CUsersTabMemberShips* CUsersTabMemberShips::getObject()
{
	return this;
}

bool CUsersTabMemberShips::OnInitDialog()
{
	HWND hWndListViewLeft = GetDlgItem( hDlg, IDC_USERS_LISTVIEW_LEFT );
	HWND hWndListViewRight = GetDlgItem( hDlg, IDC_USERS_LISTVIEW_RIGHT );

	addColumnToListView( hWndListViewLeft, 0, "Role name", 200 );
	addColumnToListView( hWndListViewLeft, 1, "Granted", 65 );
	addColumnToListView( hWndListViewLeft, 2, "Admin option", 95 );
	notImplemented( hWndListViewLeft );

	addColumnToListView( hWndListViewRight, 0, "User name", 240 );
	notImplemented( hWndListViewRight );

	return true;
}

bool CUsersTabMemberShips::addRowToListView()
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

BOOL CALLBACK wndproCUsersTabMemberShips( HWND hWndChildTab, UINT message, WPARAM wParam, LPARAM lParam )
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

	case WM_COMMAND:
		if ( child->onCommand( hWndChildTab, LOWORD( wParam ) ) )
			return TRUE;
	}

    return FALSE;
}

bool CUsersTabMemberShips::createDialogIndirect( CServiceTabUsers *parentTabCtrl )
{
	CUsersTabChild::createDialogIndirect( parentTabCtrl );

	hDlg = CreateDialogIndirect( m_hInstance,
                                 resource,
                                 parent,
                                 (DLGPROC)wndproCUsersTabMemberShips );
	OnInitDialog();
	return true;
}

bool CUsersTabMemberShips::buildDlgChild( HWND hWndParent )
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

	*p++ = 3;          // NumberOfItems

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

    TMP_NAMECONTROL   ( "ListControl", IDC_USERS_LISTVIEW_LEFT, "SysListView32",LVS_SHOWSELALWAYS | LVS_REPORT | LVS_ALIGNLEFT | WS_BORDER | WS_TABSTOP,3,3,180,94 )
    TMP_NAMECONTROL   ( "ListControl", IDC_USERS_LISTVIEW_RIGHT, "SysListView32",LVS_SHOWSELALWAYS | LVS_REPORT | LVS_ALIGNLEFT | WS_BORDER | WS_TABSTOP,185,3,120,94 )
    TMP_DEFPUSHBUTTON ( "Get info", IDC_BUTTON_GET_INFO,14,100,60,14 )

	return true;
}

}; // end namespace OdbcJdbcSetupLibrary
