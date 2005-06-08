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

// ServiceTabRepair.cpp: Service Repair Manager class.
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

CServiceTabRepair::CServiceTabRepair() : CServiceTabChild()
{
}

CServiceTabRepair::~CServiceTabRepair()
{
}

void CServiceTabRepair::updateData( HWND hDlg, BOOL bSaveAndValidate )
{
	CServiceTabChild::updateData( hDlg, bSaveAndValidate );

	if ( bSaveAndValidate )
	{
	}
	else
	{
		SetDisabledDlgItem( hDlg, IDOK );
	}
}

BOOL CALLBACK wndproCServiceTabRepairChild( HWND hWndChildTab, UINT message, UINT wParam, LONG lParam )
{
	HWND hWndParent = GetParent( hWndChildTab );
	PTAG_DIALOG_HEADER tabData = (PTAG_DIALOG_HEADER)GetWindowLong( hWndParent, GWL_USERDATA );
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
		{
			//ASSERT( tabData->hWndChildTab == hWndChildTab );
		}
		return TRUE;

	case WM_COMMAND:
		if ( child->onCommand( hWndChildTab, LOWORD( wParam ) ) )
			return TRUE;

        switch ( LOWORD( wParam ) ) 
		{
        case IDCANCEL:
			return TRUE;

        case IDOK:
            return TRUE;
        }
        break;
	}

    return FALSE;
}

bool CServiceTabRepair::onCommand( HWND hWnd, int nCommand )
{
	if ( CServiceTabChild::onCommand( hWnd, nCommand ) )
		return true;

	return false;
}

bool CServiceTabRepair::createDialogIndirect( CServiceTabCtrl *parentTabCtrl )
{
	CServiceTabChild::createDialogIndirect( parentTabCtrl );

	CreateDialogIndirect( m_hInstance,
						  resource,
						  parent,
						  wndproCServiceTabRepairChild );
	return true;
}

bool CServiceTabRepair::buildDlgChild( HWND hWndParent )
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

	*p++ = 25;         // NumberOfItems

	*p++ = 0;          // x
	*p++ = 0;          // y
	*p++ = 320;        // cx
	*p++ = 190;        // cy
	*p++ = 0;          // Menu
	*p++ = 0;          // Class

	/* copy the title of the dialog */
	nchar = nCopyAnsiToWideChar( p, TEXT( _TR( IDS_DLG_TITLE_SETUP, "FireBird ODBC Service" ) ) );
	p += nchar;

	*p++ = 8;          // FontSize
	nchar = nCopyAnsiToWideChar( p, TEXT( "MS Sans Serif" ) );
	p += nchar;

    TMP_EDITTEXT      ( IDC_DATABASE,7,10,246,12,ES_AUTOHSCROLL )
    TMP_PUSHBUTTON    ( "Browse",IDC_FIND_FILE,259,9,60,14 )
    TMP_EDITTEXT      ( IDC_USER,7,35,107,12,ES_UPPERCASE | ES_AUTOHSCROLL )
    TMP_EDITTEXT      ( IDC_PASSWORD,125,35,74,12,ES_PASSWORD | ES_AUTOHSCROLL )
    TMP_EDITTEXT      ( IDC_ROLE,212,35,107,12,ES_AUTOHSCROLL )
    TMP_RADIOCONTROL  ( "Validate database",IDC_RADIO_VALIDATE_DB,"Button", BS_AUTORADIOBUTTON,15,78,137,10 )
    TMP_RADIOCONTROL  ( "Sweep database",IDC_RADIO_SWEEP_DB,"Button", BS_AUTORADIOBUTTON,15,92,137,10 )
    TMP_RADIOCONTROL  ( "Prepare for backup",IDC_RADIO_PREPARE_DB,"Button", BS_AUTORADIOBUTTON,15,106,137,10 )
    TMP_RADIOCONTROL  ( "List limbo transactions",IDC_RADIO_LIST_LIMBO_TR,"Button", BS_AUTORADIOBUTTON,15,120,137,10 )
    TMP_RADIOCONTROL  ( "Repair limbo trans",IDC_RADIO_REPAIR_LIMBO_TR,"Button", BS_AUTORADIOBUTTON,15,134,137,10 )
    TMP_BUTTONCONTROL ( "Ignore checksums",IDC_CHECK_IGNORE_CHECKSUM,"Button", BS_AUTOCHECKBOX | WS_TABSTOP,170,82,143,10 )
    TMP_BUTTONCONTROL ( "Kill unavaliable shadows",IDC_CHECK_KILL_SHADOWS,"Button", BS_AUTOCHECKBOX | WS_TABSTOP,170,97,143,10 )
    TMP_BUTTONCONTROL ( "Validate record fragments",IDC_CHECK_VALIDATE_RECORD, "Button",BS_AUTOCHECKBOX | WS_TABSTOP,170,113,143,10 )
    TMP_BUTTONCONTROL ( "Read only validation",IDC_CHECK_READONLY,"Button", BS_AUTOCHECKBOX | WS_TABSTOP,170,129,143,10 )
    TMP_DEFPUSHBUTTON ( "Execute",IDOK,12,153,83,14 )
    TMP_PUSHBUTTON    ( "View log",IDC_BUTTON_VIEW_LOG,72,172,88,14 )
    TMP_PUSHBUTTON    ( "Save log",IDC_BUTTON_SAVE_LOG,169,172,85,14 )
    TMP_LTEXT         ( "Database",IDC_STATIC,7,0,218,8 )
    TMP_LTEXT         ( "Database Account",IDC_STATIC,7,25,107,8 )
    TMP_LTEXT         ( "Password",IDC_STATIC,126,25,72,8 )
    TMP_LTEXT         ( "Role",IDC_STATIC,214,25,105,8 )
    TMP_LTEXT         ( "Note: To validate a dabase disconnect all database connections first.", IDC_STATIC,7,53,312,8 )
    TMP_GROUPBOX      ( "Operation",IDC_STATIC,7,66,148,83 )
    TMP_GROUPBOX      ( "Validation options",IDC_STATIC,160,66,159,83 )
    TMP_EDITTEXT      ( IDC_EDIT_VIEW_LOG_LINE,100,154,219,13,ES_AUTOHSCROLL )

	return true;
}

}; // end namespace OdbcJdbcSetupLibrary
