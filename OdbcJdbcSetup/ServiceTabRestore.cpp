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

// ServiceTabRestore.cpp: Service Restore Manager class.
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

CServiceTabRestore::CServiceTabRestore() : CServiceTabChild()
{
	restoreParameters = 0;
	noReadOnly = true;
}

CServiceTabRestore::~CServiceTabRestore()
{
}

void CServiceTabRestore::updateData( HWND hDlg, BOOL bSaveAndValidate )
{
	CServiceTabChild::updateData( hDlg, bSaveAndValidate );

	if ( bSaveAndValidate )
	{
		GetDlgItemText( hDlg, IDC_BACKUP_FILE, backupPathFile.getBuffer( 256 ), 256 );

		restoreParameters = 0;

        if ( SendDlgItemMessage( hDlg, IDC_CHECK_NO_INDEX, BM_GETCHECK, 0, 0 ) )
			restoreParameters |= enDeactivateIndexes;
        if ( SendDlgItemMessage( hDlg, IDC_CHECK_NO_SHADOW, BM_GETCHECK, 0, 0 ) )
			restoreParameters |= enNoShadow;
        if ( SendDlgItemMessage( hDlg, IDC_CHECK_NO_VALIDITY, BM_GETCHECK, 0, 0 ) )
			restoreParameters |= enNoValidityCheck;
        if ( SendDlgItemMessage( hDlg, IDC_CHECK_COMMIT, BM_GETCHECK, 0, 0 ) )
			restoreParameters |= enOneRelationAtATime;
        if ( SendDlgItemMessage( hDlg, IDC_CHECK_REPLACE, BM_GETCHECK, 0, 0 ) )
			restoreParameters |= enReplace;
        if ( SendDlgItemMessage( hDlg, IDC_CHECK_FULL_SPACE, BM_GETCHECK, 0, 0 ) )
			restoreParameters |= enUseAllSpace;
        if ( SendDlgItemMessage( hDlg, IDC_CHECK_ONLY_METADATA, BM_GETCHECK, 0, 0 ) )
			restoreParameters |= enMetadataOnly;

        if ( SendDlgItemMessage( hDlg, IDC_CHECK_NO_READONLY, BM_GETCHECK, 0, 0 ) )
			noReadOnly = false;
		else
			noReadOnly = true;

		HWND hWnd = GetDlgItem( hDlg, IDC_COMBOBOX_PAGE_SIZE );
		int nLen = GetWindowTextLength( hWnd );
		if ( nLen > 0 )
			GetWindowText( hWnd, pageSize.getBuffer( nLen ), nLen + 1 );
		else
			GetWindowText( hWnd, pageSize.getBuffer( 256 ), 256 + 1 );

		hWnd = GetDlgItem( hDlg, IDC_SIZE_BUFFERS );
		nLen = GetWindowTextLength( hWnd );
		if ( nLen > 0 )
			GetWindowText( hWnd, buffersSize.getBuffer( nLen ), nLen + 1 );
		else
			GetWindowText( hWnd, buffersSize.getBuffer( 256 ), 256 + 1 );
	}
	else
	{
		EnableWindow( GetDlgItem( hDlg, IDC_BUTTON_VIEW_LOG ), !logPathFile.IsEmpty() );
		SetDlgItemText( hDlg, IDC_BACKUP_FILE, (const char*)backupPathFile );

        CheckDlgButton( hDlg, IDC_CHECK_NO_INDEX, restoreParameters & enDeactivateIndexes );
        CheckDlgButton( hDlg, IDC_CHECK_NO_SHADOW, restoreParameters & enNoShadow );
        CheckDlgButton( hDlg, IDC_CHECK_NO_VALIDITY, restoreParameters & enNoValidityCheck );
        CheckDlgButton( hDlg, IDC_CHECK_COMMIT, restoreParameters & enOneRelationAtATime );
        CheckDlgButton( hDlg, IDC_CHECK_REPLACE, restoreParameters & enReplace );
        CheckDlgButton( hDlg, IDC_CHECK_FULL_SPACE, restoreParameters & enUseAllSpace );
        CheckDlgButton( hDlg, IDC_CHECK_ONLY_METADATA, restoreParameters & enMetadataOnly );

        CheckDlgButton( hDlg, IDC_CHECK_NO_READONLY, !noReadOnly );

		SetDlgItemText(hDlg, IDC_SIZE_BUFFERS, (const char *)buffersSize );
	}
}

BOOL CALLBACK wndproCServiceTabRestoreChild( HWND hWndChildTab, UINT message, WPARAM wParam, LPARAM lParam )
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
			{
				char buffer[10];
				HWND hWndBox = GetDlgItem( hWndChildTab, IDC_COMBOBOX_PAGE_SIZE );
				for ( int i = 0; i < 6; i++ )
				{
					sprintf( buffer, "%d", 1024 << i );
					SendMessage( hWndBox, CB_ADDSTRING, 0, (LPARAM)buffer );
				}
				SendMessage( hWndBox, CB_SETCURSEL, 3, 0 ); // 8192
			}
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

bool CServiceTabRestore::onCommand( HWND hWnd, int nCommand )
{
	if ( CServiceTabChild::onCommand( hWnd, nCommand ) )
		return true;

	switch ( nCommand ) 
	{
	case IDC_FIND_FILE_BACKUP:
		updateData( hWnd );
		if ( OnFindFileBackup() )
			updateData( hWnd, FALSE );
		return true;

	case IDC_BUTTON_VIEW_LOG:
		viewLogFile();
		return true;

	case IDOK:
		updateData( hWnd );
		onStartRestore();
		return true;
	}

	return false;
}

void CServiceTabRestore::addParameters( CServiceClient &services )
{
	CServiceTabChild::addParameters( services );

	services.putParameterValue( "backupFile", backupPathFile );
	services.putParameterValue( "serverName", server );
	services.putParameterValue( SETUP_PAGE_SIZE, pageSize );
	services.putParameterValue( "buffersSize", buffersSize );
	services.putParameterValue( "noReadOnly", noReadOnly ? "Y" : "N" );
}

void CServiceTabRestore::onStartRestore()
{
	CServiceClient services;

	if ( backupPathFile.IsEmpty() || database.IsEmpty() 
		|| user.IsEmpty() || password.IsEmpty() )
	{
		// add error message
		return;
	}

	try
	{
		DWORD dwWritten;
		int lengthOut;
		int pos = 0;
		char bufferHead[80];
		char buffer[1024];
		HWND hWndBar = GetDlgItem( hDlg, IDC_PROGRESS_BAR );

		deleteTempLogFile();
		EnableWindow( GetDlgItem( hDlg, IDC_BUTTON_VIEW_LOG ), !logPathFile.IsEmpty() );

		if ( !services.initServices() )
		{
			// add error message
			return;
		}

		SendMessage( hWndBar, PBM_SETPOS, (WPARAM)0 , (LPARAM)NULL );
		addParameters( services );
		services.startRestoreDatabase( restoreParameters );

		if ( createTempLogFile() )
		{
			EnableWindow( GetDlgItem( hDlg, IDOK ), FALSE );

			while ( services.nextQuery( buffer, sizeof ( buffer ), lengthOut, countError ) )
			{
				char *pt = buffer;

				if ( services.checkIncrementForRestore( pt, bufferHead ) )
				{
					int lengthPt = (int)strlen( bufferHead );
					pos += 8;
					SendMessage( hWndBar, PBM_SETPOS, (WPARAM)pos , (LPARAM)NULL );
					WriteFile( hTmpFile, bufferHead, lengthPt, &dwWritten, NULL );
				}

				strcpy( &buffer[lengthOut], "<br>" );
				lengthOut += 4;
				WriteFile( hTmpFile, buffer, lengthOut, &dwWritten, NULL );
			}

			services.exitRestoreDatabase();

			if ( !noReadOnly )
			{
				char *pt = "<UL><B>Database is Read Only</UL></B>";
				WriteFile( hTmpFile, pt, (DWORD)strlen( pt ), &dwWritten, NULL );
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

bool CServiceTabRestore::OnFindFileBackup()
{
	char * szCaption    = "Select Firebird backup file";
	char * szOpenFilter = "Firebird Backup Files (*.fbk;*.gbk)\0*.fbk;*.gbk\0"
                          "All files (*.*)\0*.*\0"
                          "\0";
	char * szDefExt     = "*.fbk";

	return CServiceTabChild::OnFindFile( szCaption, szOpenFilter, szDefExt, backupPathFile );
}

bool CServiceTabRestore::createDialogIndirect( CServiceTabCtrl *parentTabCtrl )
{
	CServiceTabChild::createDialogIndirect( parentTabCtrl );

	if ( backupPathFile.IsEmpty() )
		setDefaultName( "fbk", backupPathFile );

	hDlg = CreateDialogIndirect( m_hInstance,
                                 resource,
                                 parent,
                                 (DLGPROC)wndproCServiceTabRestoreChild );
	return true;
}

bool CServiceTabRestore::buildDlgChild( HWND hWndParent )
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

	*p++ = 28;         // NumberOfItems

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

    TMP_EDITTEXT      ( IDC_BACKUP_FILE,7,10,246,12,ES_AUTOHSCROLL )
    TMP_PUSHBUTTON    ( "Browse",IDC_FIND_FILE_BACKUP,259,9,60,14 )
    TMP_EDITTEXT      ( IDC_DATABASE,7,35,246,12,ES_AUTOHSCROLL )
    TMP_PUSHBUTTON    ( "Browse",IDC_FIND_FILE,259,34,60,14 )
    TMP_EDITTEXT      ( IDC_USER,7,60,107,12,ES_UPPERCASE | ES_AUTOHSCROLL )
    TMP_EDITTEXT      ( IDC_PASSWORD,125,60,74,12,ES_PASSWORD | ES_AUTOHSCROLL )
    TMP_EDITTEXT      ( IDC_ROLE,212,60,107,12,ES_AUTOHSCROLL )
    TMP_COMBOBOX      ( IDC_COMBOBOX_PAGE_SIZE,90,77,50,120,CBS_DROPDOWN | WS_VSCROLL | WS_TABSTOP )
    TMP_BUTTONCONTROL ( "Deactivate indexes",IDC_CHECK_NO_INDEX,"Button", BS_AUTOCHECKBOX | WS_TABSTOP,16,97,74,10 )
    TMP_BUTTONCONTROL ( "Don't create shadows",IDC_CHECK_NO_SHADOW,"Button", BS_AUTOCHECKBOX | WS_TABSTOP,16,110,81,10 )
    TMP_BUTTONCONTROL ( "Don't validate constraints",IDC_CHECK_NO_VALIDITY, "Button",BS_AUTOCHECKBOX | WS_TABSTOP,16,123,91,10 )
    TMP_BUTTONCONTROL ( "Replace existing database",IDC_CHECK_REPLACE,"Button", BS_AUTOCHECKBOX | WS_TABSTOP,16,136,97,10 )
    TMP_BUTTONCONTROL ( "Use all space",IDC_CHECK_FULL_SPACE,"Button", BS_AUTOCHECKBOX | WS_TABSTOP,170,97,58,10 )
    TMP_BUTTONCONTROL ( "Creates a read only database",IDC_CHECK_NO_READONLY, "Button",BS_AUTOCHECKBOX | WS_TABSTOP,170,110,105,10 )
    TMP_BUTTONCONTROL ( "Commit each table",IDC_CHECK_COMMIT,"Button", BS_AUTOCHECKBOX | WS_TABSTOP,170,123,72,10 )
    TMP_BUTTONCONTROL ( "Restore metadata only",IDC_CHECK_ONLY_METADATA,"Button", BS_AUTOCHECKBOX | WS_TABSTOP,170,136,83,10 )
    TMP_EDITTEXT      ( IDC_SIZE_BUFFERS,280,77,39,12,ES_AUTOHSCROLL )
    TMP_DEFPUSHBUTTON ( "Start restore",IDOK,72,172,88,14 )
    TMP_PUSHBUTTON    ( "View log",IDC_BUTTON_VIEW_LOG,169,172,85,14 )
    TMP_LTEXT         ( "Backup file",IDC_STATIC,7,0,218,8 )
    TMP_LTEXT         ( "Database",IDC_STATIC,7,25,218,8 )
    TMP_LTEXT         ( "Database Account",IDC_STATIC,7,50,107,8 )
    TMP_LTEXT         ( "Password",IDC_STATIC,126,50,72,8 )
    TMP_LTEXT         ( "Role",IDC_STATIC,214,50,105,8 )
    TMP_LTEXT         ( "Page size",IDC_STATIC,7,78,65,8 )
    TMP_GROUPBOX      ( "Restore options",IDC_STATIC,7,89,312,61 )
    TMP_NAMECONTROL   ( "ProgressBar", IDC_PROGRESS_BAR, "msctls_progress32",WS_BORDER,7,154,312,13 )
    TMP_LTEXT         ( "Buffers",IDC_STATIC,200,78,65,8 )

	return true;
}

}; // end namespace OdbcJdbcSetupLibrary
