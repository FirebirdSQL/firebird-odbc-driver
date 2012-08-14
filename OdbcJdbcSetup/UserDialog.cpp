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

// UserDialog.cpp: User Dialog Manager class.
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

CUserDialog::CUserDialog( CUsersTabUsers *parentTab, const char *headDlg )
{
	parent = parentTab;
	headerDlg = headDlg;
	hDlg = NULL;
}

CUserDialog::~CUserDialog()
{
}

void CUserDialog::updateData( HWND hDlg, BOOL bSaveAndValidate )
{
	if ( bSaveAndValidate )
	{
		GetDlgItemText( hDlg, IDC_EDIT_USER_NAME, parent->userName.getBuffer(256), 256);
		GetDlgItemText( hDlg, IDC_EDIT_PASSWORD, parent->password.getBuffer(256), 256);
		GetDlgItemText( hDlg, IDC_EDIT_CONF_PASSWORD, parent->passwordConfirm.getBuffer(256), 256);
		GetDlgItemText( hDlg, IDC_EDIT_USER_FIRST_NAME, parent->firstName.getBuffer(256), 256);
		GetDlgItemText( hDlg, IDC_EDIT_USER_MIDDLE_NAME, parent->middleName.getBuffer(256), 256);
		GetDlgItemText( hDlg, IDC_EDIT_USER_LAST_NAME, parent->lastName.getBuffer(256), 256);
		GetDlgItemText( hDlg, IDC_EDIT_USER_ID, parent->userId.getBuffer(256), 256);
		GetDlgItemText( hDlg, IDC_EDIT_GROUP_ID, parent->groupId.getBuffer(256), 256);
	}
	else
	{
		SetDlgItemText( hDlg, IDC_EDIT_USER_NAME, (const char *)parent->userName );
		SetDlgItemText( hDlg, IDC_EDIT_PASSWORD, (const char *)parent->password );
		SetDlgItemText( hDlg, IDC_EDIT_CONF_PASSWORD, (const char *)parent->passwordConfirm );
		SetDlgItemText( hDlg, IDC_EDIT_USER_FIRST_NAME, (const char *)parent->firstName );
		SetDlgItemText( hDlg, IDC_EDIT_USER_MIDDLE_NAME, (const char *)parent->middleName );
		SetDlgItemText( hDlg, IDC_EDIT_USER_LAST_NAME, (const char *)parent->lastName );
		SetDlgItemText( hDlg, IDC_EDIT_USER_ID, (const char *)parent->userId );
		SetDlgItemText( hDlg, IDC_EDIT_GROUP_ID, (const char *)parent->groupId );
	}
}

bool CUserDialog::OnInitDialog( HWND hWndDlg )
{
	hDlg = hWndDlg;
	return true;
}

bool CUserDialog::validateFields()
{
	JString text;

	do
	{
		if ( parent->userName.IsEmpty() )
		{
			text = "Bad User name.";
			break;
		}
		else
		{
			const char *ch = parent->userName;

			while( (*ch++ == ' ') );

			if ( !*--ch )
			{
				text = "Bad User name.";
				break;
			}
		}

		if ( !parent->password.IsEmpty() )
		{
			const char *ch = parent->password;

			while( (*ch++ == ' ') );

			if ( !*--ch )
			{
				text = "Bad Password.";
				break;
			}
		}

		if ( parent->password != parent->passwordConfirm )
		{
			text = "Bad Password.";
			break;
		}

		return true;

	} while ( false );

	MessageBox( NULL, text, TEXT( "Error!" ), MB_ICONERROR | MB_OK );
	return false;
}

bool CUserDialog::onCommand( HWND hWnd, int nCommand )
{
	switch ( nCommand ) 
	{
	case IDOK:
		return true;
	}

	return false;
}

BOOL CALLBACK wndproCUserDialog( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch ( message )
	{
    case WM_INITDIALOG:

	    SetWindowLongPtr( hDlg, GW_USERDATA, lParam ); 

		if ( !((CUserDialog*)lParam)->OnInitDialog( hDlg ) )
			return FALSE;

		((CUserDialog*)lParam)->updateData( hDlg, FALSE );
		return TRUE;

	case WM_COMMAND:
        switch ( LOWORD( wParam ) ) 
		{
        case IDCANCEL:
			EndDialog( hDlg, FALSE );
			return TRUE;

        case IDOK:
			{
				CUserDialog *dlg = (CUserDialog*)GetWindowLongPtr( hDlg, GW_USERDATA );
				dlg->updateData( hDlg );

				if ( !dlg->validateFields() )
				    return FALSE;

				EndDialog( hDlg, TRUE );
			}
            return TRUE;
        }
        break;
	}
    return FALSE;
}

intptr_t CUserDialog::DoModal()
{
	char bufHeader[128];
	WORD *p, *pdlgtemplate;
	int nchar;
	DWORD lStyle;

	pdlgtemplate = p = (PWORD)LocalAlloc( LPTR, 1024 );

	lStyle = DS_SETFONT | DS_MODALFRAME | DS_CENTER | WS_CAPTION | WS_SYSMENU;

	*p++ = LOWORD (lStyle);
	*p++ = HIWORD (lStyle);
	*p++ = 0;          // LOWORD (lExtendedStyle)
	*p++ = 0;          // HIWORD (lExtendedStyle)

	*p++ = 18;          // NumberOfItems

	*p++ = 0;          // x
	*p++ = 0;          // y
	*p++ = 237;        // cx
	*p++ = 197;        // cy
	*p++ = 0;          // Menu
	*p++ = 0;          // Class

	/* copy the title of the dialog */
	sprintf( bufHeader, "%s(%s)", TEXT( _TR( IDS_DLG_TITLE_SETUP, "Firebird ODBC Service" ) ), headerDlg );
	nchar = nCopyAnsiToWideChar( p, bufHeader );
	p += nchar;

	*p++ = 8;          // FontSize
	nchar = nCopyAnsiToWideChar( p, TEXT( "MS Sans Serif" ) );
	p += nchar;

    TMP_EDITTEXT      ( IDC_EDIT_USER_NAME,85,10,90,12,ES_UPPERCASE | ES_AUTOHSCROLL )
    TMP_EDITTEXT      ( IDC_EDIT_PASSWORD,85,30,90,12,ES_PASSWORD | ES_AUTOHSCROLL )
    TMP_EDITTEXT      ( IDC_EDIT_CONF_PASSWORD,85,50,90,12,ES_PASSWORD | ES_AUTOHSCROLL )
    TMP_EDITTEXT      ( IDC_EDIT_USER_FIRST_NAME,85,70,140,12,ES_AUTOHSCROLL )
    TMP_EDITTEXT      ( IDC_EDIT_USER_MIDDLE_NAME,85,90,140,12,ES_AUTOHSCROLL )
    TMP_EDITTEXT      ( IDC_EDIT_USER_LAST_NAME,85,110,140,12,ES_AUTOHSCROLL )
    TMP_EDITTEXT      ( IDC_EDIT_USER_ID,85,130,50,12,ES_AUTOHSCROLL )
    TMP_EDITTEXT      ( IDC_EDIT_GROUP_ID,85,150,50,12,ES_AUTOHSCROLL )
    TMP_DEFPUSHBUTTON ( "OK",IDOK,63,175,50,14 )
    TMP_PUSHBUTTON    ( "Cancel",IDCANCEL,125,175,50,14 )
    TMP_LTEXT         ( "User name",IDC_STATIC,5,15,75,8 )
    TMP_LTEXT         ( "Password",IDC_STATIC,5,35,75,8 )
    TMP_LTEXT         ( "Confirm password",IDC_STATIC,5,55,71,8 )
    TMP_LTEXT         ( "First name",IDC_STATIC,5,75,75,8 )
    TMP_LTEXT         ( "Middle name",IDC_STATIC,5,95,75,8 )
    TMP_LTEXT         ( "Last name",IDC_STATIC,5,115,75,8 )
    TMP_LTEXT         ( "User ID",IDC_STATIC,5,135,75,8 )
    TMP_LTEXT         ( "Group ID",IDC_STATIC,5,155,75,8 )

	intptr_t nRet = DialogBoxIndirectParam( m_hInstance, (LPDLGTEMPLATE) pdlgtemplate, parent->hDlg, (DLGPROC)wndproCUserDialog, (LPARAM)this );
	LocalFree( LocalHandle( pdlgtemplate ) );

	return nRet;
}

}; // end namespace OdbcJdbcSetupLibrary
