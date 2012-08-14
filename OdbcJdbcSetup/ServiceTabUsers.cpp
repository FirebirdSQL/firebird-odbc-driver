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

// ServiceTabUsers.cpp: Service Users Manager class.
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

extern HINSTANCE m_hInstance;
extern int currentCP;

CServiceTabUsers::CServiceTabUsers() : CServiceTabChild()
{
}

CServiceTabUsers::~CServiceTabUsers()
{
}

void CServiceTabUsers::updateData( HWND hDlg, BOOL bSaveAndValidate )
{
	CServiceTabChild::updateData( hDlg, bSaveAndValidate );

	if ( bSaveAndValidate )
	{
	}
	else
	{
	}
}

bool CServiceTabUsers::OnInitDialog( HWND hDlg )
{
	HWND hWndTab = GetDlgItem( hDlg, IDC_USERS_TABCTRL );
    TCITEM tie;

	hWndDlg = hDlg;
    tie.mask = TCIF_TEXT | TCIF_IMAGE; 
    tie.iImage = -1; 
	tabData.tabCtrl = this;
	tabData.hWndTab = hWndTab;
    tie.lParam = (uintptr_t)&tabData;

	tabData.childTab[0] = users.getObject();
	users.buildDlgChild( hWndTab );
    tie.pszText = " Users "; 
    TabCtrl_InsertItem( hWndTab, 0, &tie );
/*
	tabData.childTab[1] = roles.getObject();
	roles.buildDlgChild( hWndTab );
    tie.pszText = " Roles "; 
    TabCtrl_InsertItem( hWndTab, 1, &tie );

	tabData.childTab[2] = members.getObject();
	members.buildDlgChild( hWndTab );
    tie.pszText = " MemberShips "; 
    TabCtrl_InsertItem( hWndTab, 2, &tie );
*/
    SetWindowLongPtr( hWndTab, GW_USERDATA, (intptr_t)&tabData ); 
	users.createDialogIndirect( this );

	return true;
}

BOOL CALLBACK wndproCServiceTabUsersChild( HWND hWndChildTab, UINT message, WPARAM wParam, LPARAM lParam )
{
	HWND hWndParent = GetParent( hWndChildTab );
	PTAG_DIALOG_HEADER tabData = (PTAG_DIALOG_HEADER)GetWindowLongPtr( hWndParent, GW_USERDATA );
	int iPage = TabCtrl_GetCurSel( hWndParent );
	CServiceTabChild *child = tabData->childTab[iPage];

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

    case WM_NOTIFY:
		{
			if ( wParam == IDC_USERS_TABCTRL )
			{
				NMHDR *hdr = (NMHDR*)lParam;
				HWND hWndTab = hdr->hwndFrom;
				PUSERS_DIALOG_HEADER tabData = (PUSERS_DIALOG_HEADER)GetWindowLongPtr( hWndTab, GW_USERDATA );
				int iPage = TabCtrl_GetCurSel( hWndTab );

				switch ( hdr->code )
				{
				case TCN_SELCHANGE:
					if ( !tabData->hWndChildTab )
						tabData->childTab[iPage]->createDialogIndirect( tabData->tabCtrl );
					break;

				case TCN_SELCHANGING:
					if ( tabData->hWndChildTab )
					{
						DestroyWindow( tabData->hWndChildTab ); 
						tabData->hWndChildTab = NULL;
					}
					break;
				}
			}
		}
        break; 
	}

    return FALSE;
}

bool CServiceTabUsers::onCommand( HWND hWnd, int nCommand )
{
	if ( CServiceTabChild::onCommand( hWnd, nCommand ) )
		return true;

	switch ( nCommand ) 
	{
	case IDC_BUTTON_VIEW_LOG:
		viewLogFile();
		return true;

	case IDOK:
		return true;
	}

	return false;
}

void CServiceTabUsers::addParameters( CServiceClient &services )
{
	CServiceTabChild::addParameters( services );
}

bool CServiceTabUsers::createDialogIndirect( CServiceTabCtrl *parentTabCtrl )
{
	CServiceTabChild::createDialogIndirect( parentTabCtrl );

	hDlg = CreateDialogIndirect( m_hInstance,
                                 resource,
                                 parent,
                                 (DLGPROC)wndproCServiceTabUsersChild );
	OnInitDialog( hDlg );
	return true;
}

bool CServiceTabUsers::buildDlgChild( HWND hWndParent )
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

	*p++ = 10;         // NumberOfItems

	*p++ = 0;          // x
	*p++ = 0;          // y
	*p++ = 320;        // cx
	*p++ = 190;        // cy
	*p++ = 0;          // Menu
	*p++ = 0;          // Class

	/* copy the title of the dialog */
	nchar = nCopyAnsiToWideChar( p, TEXT( _TR( IDS_DLG_TITLE_SETUP, "Firebird ODBC Service" ) ) );
	p += nchar;

	*p++ = 8;          // FontSize
	nchar = nCopyAnsiToWideChar( p, TEXT( "MS Sans Serif" ) );
	p += nchar;

    TMP_EDITTEXT      ( IDC_DATABASE,7,10,246,12,ES_AUTOHSCROLL )
    TMP_PUSHBUTTON    ( "Browse",IDC_FIND_FILE,259,9,60,14 )
    TMP_EDITTEXT      ( IDC_USER,7,35,107,12,ES_UPPERCASE | ES_AUTOHSCROLL )
    TMP_EDITTEXT      ( IDC_PASSWORD,125,35,74,12,ES_PASSWORD | ES_AUTOHSCROLL )
    TMP_EDITTEXT      ( IDC_ROLE,212,35,107,12,ES_AUTOHSCROLL )
    TMP_LTEXT         ( "Database",IDC_STATIC,7,0,218,8 )
    TMP_LTEXT         ( "Database Account",IDC_STATIC,7,25,107,8 )
    TMP_LTEXT         ( "Password",IDC_STATIC,126,25,72,8 )
    TMP_LTEXT         ( "Role",IDC_STATIC,214,25,105,8 )
    TMP_NAMECONTROL   ( "TabControl", IDC_USERS_TABCTRL, "SysTabControl32",0x0,7,53,312,132 )

	return true;
}

}; // end namespace OdbcJdbcSetupLibrary
