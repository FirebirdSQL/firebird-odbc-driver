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

// UsersTabChild.cpp: Users Tab Child class.
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

CUsersTabChild::CUsersTabChild()
{
	parent = NULL;
	hDlg = NULL;
	resource = NULL;
}

CUsersTabChild::~CUsersTabChild()
{
	if ( resource ) {
		LocalFree( LocalHandle( resource ) );
		resource = NULL;
	}
}

CUsersTabChild* CUsersTabChild::getObject()
{
	return this;
}

void CUsersTabChild::updateData( HWND hDlg, BOOL bSaveAndValidate )
{
	tabCtrl->updateData( tabCtrl->hDlg, bSaveAndValidate );
}

bool CUsersTabChild::onCommand( HWND hWnd, int nCommand )
{
	return false;
}

bool CUsersTabChild::validateAccountFields()
{
	JString text;

	tabCtrl->updateData( tabCtrl->hDlg );

	do
	{
		if ( tabCtrl->user.IsEmpty() )
		{
			text = "Bad Database Account.";
			break;
		}
		else
		{
			const char *ch = tabCtrl->user;

			while( (*ch++ == ' ') );

			if ( !*--ch )
			{
				text = "Bad Database Account.";
				break;
			}
		}

		if ( tabCtrl->password.IsEmpty() )
		{
			text = "Bad Password.";
			break;
		}
		else
		{
			const char *ch = tabCtrl->password;

			while( (*ch++ == ' ') );

			if ( !*--ch )
			{
				text = "Bad Password.";
				break;
			}
		}

		return true;

	} while ( false );

	MessageBox( NULL, text, TEXT( "Error!" ), MB_ICONERROR | MB_OK );
	return false;
}

bool CUsersTabChild::isAdmin( const char *userName )
{
	QUAD adm1 = (QUAD)71752869960019.0;
	QUAD adm2 = (QUAD)107075219978611.0;
	QUAD user = (QUAD)0;
	memcpy( (void *)&user, userName, 6 );

	return user == adm1 || user == adm2;
}

bool CUsersTabChild::createDialogIndirect( CServiceTabUsers *parentTabCtrl )
{
	tabCtrl = parentTabCtrl;
	hDlg = NULL;
	return true;
}

void CUsersTabChild::addColumnToListView( HWND hWnd, int i, char *name, int width )
{
    LV_COLUMN col;

    col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    col.fmt  = LVCFMT_LEFT;
    col.pszText = name;
    col.cchTextMax = 0;
    col.cx = width;
    col.iSubItem = 0;
    ListView_InsertColumn( hWnd, i, &col );

	SendMessage( hWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 
					   LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES |
					   LVS_EX_TWOCLICKACTIVATE | LVS_EX_SUBITEMIMAGES,
					   LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES |
					   LVS_EX_TWOCLICKACTIVATE | LVS_EX_SUBITEMIMAGES );
}

void CUsersTabChild::notImplemented( HWND hWndLV )
{
    LV_ITEM lvi = { 0 };
    lvi.mask = LVIF_TEXT;

	lvi.iItem = 0;
	lvi.iSubItem = 0;
	lvi.pszText = (LPSTR)"Not Implemented";
	ListView_InsertItem( hWndLV, &lvi );
}

bool CUsersTabChild::buildDlgChild( HWND hWndParent )
{
	parent = hWndParent;
	return true;
}

}; // end namespace OdbcJdbcSetupLibrary
