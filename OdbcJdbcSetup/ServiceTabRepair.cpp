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
	isVisibleValidateOptions = true;
	repairParameters = enValidateDb;
	validateParameters = 0;
}

CServiceTabRepair::~CServiceTabRepair()
{
}

void CServiceTabRepair::updateData( HWND hDlg, BOOL bSaveAndValidate )
{
	CServiceTabChild::updateData( hDlg, bSaveAndValidate );

	if ( bSaveAndValidate )
	{
		repairParameters = 0;
        if ( SendDlgItemMessage( hDlg, IDC_RADIO_VALIDATE_DB, BM_GETCHECK, 0, 0 ) )
		{
			repairParameters = enValidateDb;
			validateParameters = 0;
			if ( SendDlgItemMessage( hDlg, IDC_CHECK_IGNORE_CHECKSUM, BM_GETCHECK, 0, 0 ) )
				validateParameters |= enIgnoreChecksum;
			if ( SendDlgItemMessage( hDlg, IDC_CHECK_KILL_SHADOWS, BM_GETCHECK, 0, 0 ) )
				validateParameters |= enKillShadows;
			if ( SendDlgItemMessage( hDlg, IDC_CHECK_VALIDATE_RECORD, BM_GETCHECK, 0, 0 ) )
				validateParameters |= enFull;
			if ( SendDlgItemMessage( hDlg, IDC_CHECK_READONLY, BM_GETCHECK, 0, 0 ) )
				validateParameters |= enCheckDb;
		}

        if ( SendDlgItemMessage( hDlg, IDC_RADIO_SWEEP_DB, BM_GETCHECK, 0, 0 ) )
			repairParameters = enSweepDb;
        if ( SendDlgItemMessage( hDlg, IDC_RADIO_PREPARE_DB, BM_GETCHECK, 0, 0 ) )
			repairParameters = enMendDb;
        if ( SendDlgItemMessage( hDlg, IDC_RADIO_LIST_LIMBO_TR, BM_GETCHECK, 0, 0 ) )
			repairParameters = enListLimboTrans;
        if ( SendDlgItemMessage( hDlg, IDC_RADIO_REPAIR_LIMBO_TR, BM_GETCHECK, 0, 0 ) )
			repairParameters = enFixListLimboTrans;
	}
	else
	{
		EnableWindow( GetDlgItem( hDlg, IDC_BUTTON_VIEW_LOG ), !logPathFile.IsEmpty() );

        CheckDlgButton( hDlg, IDC_RADIO_VALIDATE_DB, repairParameters & enValidateDb );

		if ( repairParameters == enValidateDb )
		{
			CheckDlgButton( hDlg, IDC_CHECK_IGNORE_CHECKSUM, validateParameters & enIgnoreChecksum );
			CheckDlgButton( hDlg, IDC_CHECK_KILL_SHADOWS, validateParameters & enKillShadows );
			CheckDlgButton( hDlg, IDC_CHECK_VALIDATE_RECORD, validateParameters & enFull );
			CheckDlgButton( hDlg, IDC_CHECK_READONLY, validateParameters & enCheckDb );
		}

        CheckDlgButton( hDlg, IDC_RADIO_SWEEP_DB, repairParameters & enSweepDb );
        CheckDlgButton( hDlg, IDC_RADIO_PREPARE_DB, repairParameters & enMendDb );
        CheckDlgButton( hDlg, IDC_RADIO_LIST_LIMBO_TR, repairParameters & enListLimboTrans );
        CheckDlgButton( hDlg, IDC_RADIO_REPAIR_LIMBO_TR, repairParameters & enFixListLimboTrans );
	}
}

BOOL CALLBACK wndproCServiceTabRepairChild( HWND hWndChildTab, UINT message, WPARAM wParam, LPARAM lParam )
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
	}

    return FALSE;
}

bool CServiceTabRepair::onCommand( HWND hWnd, int nCommand )
{
	if ( CServiceTabChild::onCommand( hWnd, nCommand ) )
		return true;

	switch ( nCommand ) 
	{
	case IDC_BUTTON_VIEW_LOG:
		viewLogFile();
		return true;

	case IDOK:
		deleteTempLogFile();
		updateData( hWnd );
		startRepairDatabase();
		return true;

	case IDC_RADIO_VALIDATE_DB:
		hideValidateOptions( false );
		return true;

	case IDC_RADIO_SWEEP_DB:
	case IDC_RADIO_PREPARE_DB:
	case IDC_RADIO_LIST_LIMBO_TR:
	case IDC_RADIO_REPAIR_LIMBO_TR:
		hideValidateOptions( true );
		return true;
	}

	return false;
}

void CServiceTabRepair::hideValidateOptions( bool hide )
{
	if ( (isVisibleValidateOptions && !hide) || (!isVisibleValidateOptions && hide) )
		return;

	int nCmdShow = hide ? SW_HIDE : SW_SHOW;
	isVisibleValidateOptions = !hide;

	ShowWindow( GetDlgItem( hDlg, IDC_CHECK_IGNORE_CHECKSUM ), nCmdShow );
	ShowWindow( GetDlgItem( hDlg, IDC_CHECK_KILL_SHADOWS ), nCmdShow );
	ShowWindow( GetDlgItem( hDlg, IDC_CHECK_VALIDATE_RECORD ), nCmdShow );
	ShowWindow( GetDlgItem( hDlg, IDC_CHECK_READONLY ), nCmdShow );
	ShowWindow( GetDlgItem( hDlg, IDC_GROUPBOX_VALIDATE_OPTIONS ), nCmdShow );
}

void CServiceTabRepair::addParameters( CServiceClient &services )
{
	CServiceTabChild::addParameters( services );

	services.putParameterValue( "serverName", server );
}

void CServiceTabRepair::startRepairDatabase()
{
	CServiceClient services;

	if ( database.IsEmpty() || user.IsEmpty() || password.IsEmpty() )
	{
		// add error message
		return;
	}

	try
	{
		DWORD dwWritten;
		int lengthOut;
		int pos = 0;
		char buffer[1024];
		HWND hWndBar = GetDlgItem( hDlg, IDC_PROGRESS_BAR );

		EnableWindow( GetDlgItem( hDlg, IDC_BUTTON_VIEW_LOG ), !logPathFile.IsEmpty() );

		if ( !services.initServices() )
		{
			// add error message
			return;
		}

		SendMessage( hWndBar, PBM_SETPOS, (WPARAM)0 , (LPARAM)NULL );
		addParameters( services );

		switch ( repairParameters )
		{
		case enValidateDb:
			services.startRepairDatabase( repairParameters, validateParameters );
			break;

		case enMendDb:
			services.startRepairDatabase( repairParameters, enIgnoreChecksum );
			break;

		case enFixListLimboTrans:
			services.startRepairDatabase( enListLimboTrans, 0 );
			break;

		default:
			services.startRepairDatabase( repairParameters, 0 );
			break;
		}

		if ( createTempLogFile() )
		{
			ULONG countRows = 0;
			EnableWindow( GetDlgItem( hDlg, IDOK ), FALSE );

			switch( repairParameters )
			{
			case enListLimboTrans:
			case enFixListLimboTrans:
				while ( services.nextQueryLimboTransactionInfo( buffer, sizeof ( buffer ), lengthOut ) )
				{
					strcpy( &buffer[lengthOut], "<br>" );
					lengthOut += 4;
					WriteFile( hTmpFile, buffer, lengthOut, &dwWritten, NULL );
					++countRows;
				}
				break;

			default:
				while ( services.nextQuery( buffer, sizeof ( buffer ), lengthOut, countError ) )
				{
					strcpy( &buffer[lengthOut], "<br>" );
					lengthOut += 4;
					WriteFile( hTmpFile, buffer, lengthOut, &dwWritten, NULL );
					++countRows;
				}
			}

			if ( !countRows )
			{
				char *pt;
				int lengthPt;

				switch ( repairParameters )
				{
				case enValidateDb:
					pt = "<UL><B>Validate database - SUCCESS</UL></B>";
					break;
				case enSweepDb:
					pt = "<UL><B>Sweep database - SUCCESS</UL></B>";
					break;
				case enMendDb:
					pt = "<UL><B>Prepare for backup - SUCCESS</UL></B>";
					break;
				case enListLimboTrans:
					pt = "<UL><B>List limbo transactions - I have not</UL></B>";
					break;
				default:
					pt = "<UL><B>Repair limbo trans - SUCCESS</UL></B>";
					break;
				}

				lengthPt = (int)strlen( pt );
				WriteFile( hTmpFile, pt, lengthPt, &dwWritten, NULL );
			}

			writeFooterToLogFile();
			EnableWindow( GetDlgItem( hDlg, IDOK ), TRUE );
		}

		SendMessage( hWndBar, PBM_SETPOS, (WPARAM)100 , (LPARAM)NULL );
		EnableWindow( GetDlgItem( hDlg, IDC_BUTTON_VIEW_LOG ), !logPathFile.IsEmpty() );
	}
	catch ( std::exception &ex )
	{
		writeFooterToLogFile();
		EnableWindow( GetDlgItem( hDlg, IDOK ), TRUE );
		EnableWindow( GetDlgItem( hDlg, IDC_BUTTON_VIEW_LOG ), !logPathFile.IsEmpty() );

		char buffer[1024];
		SQLException &exception = (SQLException&)ex;
		JString text = exception.getText();
		sprintf(buffer, "sqlcode %d, fbcode %d - %s", exception.getSqlcode(), exception.getFbcode(), (const char*)text );
		MessageBox( NULL, buffer, TEXT( "Error!" ), MB_ICONERROR | MB_OK );
	}

	services.closeService();
}

bool CServiceTabRepair::createDialogIndirect( CServiceTabCtrl *parentTabCtrl )
{
	CServiceTabChild::createDialogIndirect( parentTabCtrl );

	hDlg = CreateDialogIndirect( m_hInstance,
                                 resource,
                                 parent,
                                 (DLGPROC)wndproCServiceTabRepairChild );
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

	*p++ = 24;         // NumberOfItems

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
    TMP_RADIOCONTROL  ( "Validate database",IDC_RADIO_VALIDATE_DB,"Button", BS_AUTORADIOBUTTON,15,78,137,10 )
    TMP_RADIOCONTROL  ( "Sweep database",IDC_RADIO_SWEEP_DB,"Button", BS_AUTORADIOBUTTON,15,92,137,10 )
    TMP_RADIOCONTROL  ( "Prepare for backup",IDC_RADIO_PREPARE_DB,"Button", BS_AUTORADIOBUTTON,15,106,137,10 )
    TMP_RADIOCONTROL  ( "List limbo transactions",IDC_RADIO_LIST_LIMBO_TR,"Button", BS_AUTORADIOBUTTON,15,120,137,10 )
    TMP_RADIOCONTROL  ( "Repair limbo trans",IDC_RADIO_REPAIR_LIMBO_TR,"Button", BS_AUTORADIOBUTTON,15,134,137,10 )
    TMP_BUTTONCONTROL ( "Ignore checksums",IDC_CHECK_IGNORE_CHECKSUM,"Button", BS_AUTOCHECKBOX | WS_TABSTOP,170,82,143,10 )
    TMP_BUTTONCONTROL ( "Kill unavaliable shadows",IDC_CHECK_KILL_SHADOWS,"Button", BS_AUTOCHECKBOX | WS_TABSTOP,170,97,143,10 )
    TMP_BUTTONCONTROL ( "Validate record fragments",IDC_CHECK_VALIDATE_RECORD, "Button",BS_AUTOCHECKBOX | WS_TABSTOP,170,113,143,10 )
    TMP_BUTTONCONTROL ( "Read only validation",IDC_CHECK_READONLY,"Button", BS_AUTOCHECKBOX | WS_TABSTOP,170,129,143,10 )
    TMP_DEFPUSHBUTTON ( "Execute",IDOK,72,172,88,14 )
    TMP_PUSHBUTTON    ( "View log",IDC_BUTTON_VIEW_LOG,169,172,85,14 )
    TMP_LTEXT         ( "Database",IDC_STATIC,7,0,218,8 )
    TMP_LTEXT         ( "Database Account",IDC_STATIC,7,25,107,8 )
    TMP_LTEXT         ( "Password",IDC_STATIC,126,25,72,8 )
    TMP_LTEXT         ( "Role",IDC_STATIC,214,25,105,8 )
    TMP_LTEXT         ( "Note: To validate a dabase disconnect all database connections first.", IDC_STATIC,7,53,312,8 )
    TMP_GROUPBOX      ( "Operation",IDC_STATIC,7,66,148,83 )
    TMP_GROUPBOX      ( "Validation options",IDC_GROUPBOX_VALIDATE_OPTIONS,160,66,159,83 )
    TMP_NAMECONTROL   ( "ProgressBar", IDC_PROGRESS_BAR, "msctls_progress32",WS_BORDER,7,154,312,13 )

	return true;
}

}; // end namespace OdbcJdbcSetupLibrary
