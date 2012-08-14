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

// ServiceTabStatistics.cpp: Service Statistics Manager class.
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

CServiceTabStatistics::CServiceTabStatistics() : CServiceTabChild()
{
	statisticParameters = 0;
}

CServiceTabStatistics::~CServiceTabStatistics()
{
}

void CServiceTabStatistics::updateData( HWND hDlg, BOOL bSaveAndValidate )
{
	CServiceTabChild::updateData( hDlg, bSaveAndValidate );

	if ( bSaveAndValidate )
	{
		statisticParameters = 0;
        if ( SendDlgItemMessage( hDlg, IDC_RADIO_DATA_PAGES, BM_GETCHECK, 0, 0 ) )
			statisticParameters = enDataPages;
        if ( SendDlgItemMessage( hDlg, IDC_RADIO_HEADER_PAGES, BM_GETCHECK, 0, 0 ) )
			statisticParameters = enHdrPages;
        if ( SendDlgItemMessage( hDlg, IDC_RADIO_INDEX_PAGES, BM_GETCHECK, 0, 0 ) )
			statisticParameters = enIdxPages;
        if ( SendDlgItemMessage( hDlg, IDC_RADIO_SYS_RELATIONS, BM_GETCHECK, 0, 0 ) )
			statisticParameters = enSysRelations;
        if ( SendDlgItemMessage( hDlg, IDC_RADIO_RECORD_VERSIONS, BM_GETCHECK, 0, 0 ) )
			statisticParameters = enRecordVersions;
        if ( SendDlgItemMessage( hDlg, IDC_RADIO_DATABASE_LOG, BM_GETCHECK, 0, 0 ) )
			statisticParameters = enDbLog;
        if ( SendDlgItemMessage( hDlg, IDC_RADIO_SHOW_DATABASE_LOG, BM_GETCHECK, 0, 0 ) )
			statisticParameters = enShowDbLog;
        if ( SendDlgItemMessage( hDlg, IDC_RADIO_ALL_OPTIONS, BM_GETCHECK, 0, 0 ) )
			statisticParameters = 0;
	}
	else
	{
		EnableWindow( GetDlgItem( hDlg, IDC_BUTTON_VIEW_LOG ), !logPathFile.IsEmpty() );

        CheckDlgButton( hDlg, IDC_RADIO_DATA_PAGES, statisticParameters & enDataPages );
        CheckDlgButton( hDlg, IDC_RADIO_HEADER_PAGES, statisticParameters & enHdrPages );
        CheckDlgButton( hDlg, IDC_RADIO_INDEX_PAGES, statisticParameters & enIdxPages );
        CheckDlgButton( hDlg, IDC_RADIO_SYS_RELATIONS, statisticParameters & enSysRelations );
        CheckDlgButton( hDlg, IDC_RADIO_RECORD_VERSIONS, statisticParameters & enRecordVersions );
        CheckDlgButton( hDlg, IDC_RADIO_DATABASE_LOG, statisticParameters & enDbLog );
        CheckDlgButton( hDlg, IDC_RADIO_SHOW_DATABASE_LOG, statisticParameters & enShowDbLog );
        CheckDlgButton( hDlg, IDC_RADIO_ALL_OPTIONS, !statisticParameters );
	}
}

BOOL CALLBACK wndproCServiceTabStatisticChild( HWND hWndChildTab, UINT message, WPARAM wParam, LPARAM lParam )
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

bool CServiceTabStatistics::onCommand( HWND hWnd, int nCommand )
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
		onStartStatistics();
		return true;

	case IDC_RADIO_DATA_PAGES:
	case IDC_RADIO_HEADER_PAGES:
	case IDC_RADIO_INDEX_PAGES:
	case IDC_RADIO_SYS_RELATIONS:
	case IDC_RADIO_RECORD_VERSIONS:
	case IDC_RADIO_DATABASE_LOG:
	case IDC_RADIO_SHOW_DATABASE_LOG:
	case IDC_RADIO_ALL_OPTIONS:
		{
			HWND hWndBar = GetDlgItem( hDlg, IDC_PROGRESS_BAR );
			updateData( hWnd );
			SendMessage( hWndBar, PBM_SETPOS, (WPARAM)0 , (LPARAM)NULL );
			deleteTempLogFile();
			updateData( hWnd, FALSE );
		}
		return true;
	}

	return false;
}

void CServiceTabStatistics::addParameters( CServiceClient &services )
{
	CServiceTabChild::addParameters( services );

	services.putParameterValue( "serverName", server );
}

void CServiceTabStatistics::onStartStatistics()
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

		if ( !(statisticParameters & enShowDbLog) )
			services.startStaticticsDatabase( statisticParameters );
		else
			services.startShowDatabaseLog();

		if ( createTempLogFile() )
		{
			EnableWindow( GetDlgItem( hDlg, IDOK ), FALSE );

			while ( services.nextQuery( buffer, sizeof ( buffer ), lengthOut, countError ) )
			{
				strcpy( &buffer[lengthOut], "<br>" );
				lengthOut += 4;
				WriteFile( hTmpFile, buffer, lengthOut, &dwWritten, NULL );
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

bool CServiceTabStatistics::createDialogIndirect( CServiceTabCtrl *parentTabCtrl )
{
	CServiceTabChild::createDialogIndirect( parentTabCtrl );

	hDlg = CreateDialogIndirect( m_hInstance,
                                 resource,
                                 parent,
                                 (DLGPROC)wndproCServiceTabStatisticChild );
	return true;
}

bool CServiceTabStatistics::buildDlgChild( HWND hWndParent )
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

	*p++ = 21;         // NumberOfItems

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
	TMP_RADIOCONTROL  ( "All Options",IDC_RADIO_ALL_OPTIONS,"Button", BS_AUTOCHECKBOX | WS_TABSTOP,15,82,143,10 )
    TMP_RADIOCONTROL  ( "Data pages",IDC_RADIO_DATA_PAGES,"Button", BS_AUTOCHECKBOX | WS_TABSTOP,15,97,143,10 )
    TMP_RADIOCONTROL  ( "Header pages",IDC_RADIO_HEADER_PAGES, "Button",BS_AUTOCHECKBOX | WS_TABSTOP,15,113,143,10 )
    TMP_RADIOCONTROL  ( "Index pages",IDC_RADIO_INDEX_PAGES,"Button", BS_AUTOCHECKBOX | WS_TABSTOP,15,129,143,10 )
    TMP_RADIOCONTROL  ( "System relations",IDC_RADIO_SYS_RELATIONS,"Button", BS_AUTOCHECKBOX | WS_TABSTOP,185,82,130,10 )
    TMP_RADIOCONTROL  ( "Record versions",IDC_RADIO_RECORD_VERSIONS,"Button", BS_AUTOCHECKBOX | WS_TABSTOP,185,97,130,10 )
    TMP_RADIOCONTROL  ( "Database log",IDC_RADIO_DATABASE_LOG, "Button",BS_AUTOCHECKBOX | WS_TABSTOP,185,113,130,10 )
    TMP_RADIOCONTROL  ( "Show Database log",IDC_RADIO_SHOW_DATABASE_LOG, "Button",BS_AUTOCHECKBOX | WS_TABSTOP,185,129,130,10 )
	TMP_DEFPUSHBUTTON ( "Execute",IDOK,72,172,88,14 )
    TMP_PUSHBUTTON    ( "View log",IDC_BUTTON_VIEW_LOG,169,172,85,14 )
    TMP_LTEXT         ( "Database",IDC_STATIC,7,0,218,8 )
    TMP_LTEXT         ( "Database Account",IDC_STATIC,7,25,107,8 )
    TMP_LTEXT         ( "Password",IDC_STATIC,126,25,72,8 )
    TMP_LTEXT         ( "Role",IDC_STATIC,214,25,105,8 )
    TMP_GROUPBOX      ( "Statistics options",IDC_STATIC,7,66,312,83 )
    TMP_NAMECONTROL   ( "ProgressBar", IDC_PROGRESS_BAR, "msctls_progress32",WS_BORDER,7,154,312,13 )

	return true;
}

}; // end namespace OdbcJdbcSetupLibrary
