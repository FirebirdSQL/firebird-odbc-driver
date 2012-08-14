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

// ServiceTabBackup.cpp: Service Backup Manager class.
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

CServiceTabBackup::CServiceTabBackup() : CServiceTabChild()
{
	backupParameters = 0;
}

CServiceTabBackup::~CServiceTabBackup()
{
}

void CServiceTabBackup::updateData( HWND hDlg, BOOL bSaveAndValidate )
{
	CServiceTabChild::updateData( hDlg, bSaveAndValidate );

	if ( bSaveAndValidate )
	{
		GetDlgItemText( hDlg, IDC_BACKUP_FILE, backupPathFile.getBuffer( 256 ), 256 );

		backupParameters = 0;
        if ( SendDlgItemMessage( hDlg, IDC_CHECK_IGNORE_CHECKSUM, BM_GETCHECK, 0, 0 ) )
			backupParameters |= enIgnoreChecksums;
        if ( SendDlgItemMessage( hDlg, IDC_CHECK_IGNORE_TRANS_LIMBO, BM_GETCHECK, 0, 0 ) )
			backupParameters |= enIgnoreLimbo;
        if ( SendDlgItemMessage( hDlg, IDC_CHECK_ONLY_METADATA, BM_GETCHECK, 0, 0 ) )
			backupParameters |= enMetadataOnly;
        if ( SendDlgItemMessage( hDlg, IDC_CHECK_NO_GARBAGE, BM_GETCHECK, 0, 0 ) )
			backupParameters |= enNoGarbageCollect;
        if ( SendDlgItemMessage( hDlg, IDC_CHECK_OLD_METADATA, BM_GETCHECK, 0, 0 ) )
			backupParameters |= enOldDescriptions;
        if ( SendDlgItemMessage( hDlg, IDC_CHECK_NO_TRANSPORTABLE, BM_GETCHECK, 0, 0 ) )
			backupParameters |= enNonTransportable;
        if ( SendDlgItemMessage( hDlg, IDC_CHECK_CONV_EXT_TABLE, BM_GETCHECK, 0, 0 ) )
			backupParameters |= enConvert;
        if ( SendDlgItemMessage( hDlg, IDC_CHECK_NO_COMPRESS, BM_GETCHECK, 0, 0 ) )
			backupParameters |= enExpand;

		HWND hWnd = GetDlgItem( hDlg, IDC_BLOCKING_FACTOR );
		int nLen = GetWindowTextLength( hWnd );
		if ( nLen > 0 )
			GetWindowText( hWnd, blockingFactor.getBuffer( nLen ), nLen + 1 );
		else
			GetWindowText( hWnd, blockingFactor.getBuffer( 256 ), 256 + 1 );
	}
	else
	{
		EnableWindow( GetDlgItem( hDlg, IDC_BUTTON_VIEW_LOG ), !logPathFile.IsEmpty() );
		SetDlgItemText( hDlg, IDC_BACKUP_FILE, (const char*)backupPathFile );

        CheckDlgButton( hDlg, IDC_CHECK_IGNORE_CHECKSUM, backupParameters & enIgnoreChecksums );
        CheckDlgButton( hDlg, IDC_CHECK_IGNORE_TRANS_LIMBO, backupParameters & enIgnoreLimbo );
        CheckDlgButton( hDlg, IDC_CHECK_ONLY_METADATA, backupParameters & enMetadataOnly );
        CheckDlgButton( hDlg, IDC_CHECK_NO_GARBAGE, backupParameters & enNoGarbageCollect );
        CheckDlgButton( hDlg, IDC_CHECK_OLD_METADATA, backupParameters & enOldDescriptions );
        CheckDlgButton( hDlg, IDC_CHECK_NO_TRANSPORTABLE, backupParameters & enNonTransportable );
        CheckDlgButton( hDlg, IDC_CHECK_CONV_EXT_TABLE, backupParameters & enConvert );
        CheckDlgButton( hDlg, IDC_CHECK_NO_COMPRESS, backupParameters & enExpand );

		SetDlgItemText( hDlg, IDC_BLOCKING_FACTOR, (const char *)blockingFactor );
	}
}

BOOL CALLBACK wndproCServiceTabBackup( HWND hWndChildTab, UINT message, WPARAM wParam, LPARAM lParam )
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
		return child->onCommand( hWndChildTab, LOWORD( wParam ) );
	}

    return FALSE;
}

bool CServiceTabBackup::onCommand( HWND hWnd, int nCommand )
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
		onStartBackup();
		return true;
	}

	return false;
}

void CServiceTabBackup::addParameters( CServiceClient &services )
{
	CServiceTabChild::addParameters( services );

	services.putParameterValue( "backupFile", backupPathFile );
	services.putParameterValue( "serverName", server );
	services.putParameterValue( "blockingFactor", blockingFactor );
}

void CServiceTabBackup::onStartBackup()
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
		services.startBackupDatabase( backupParameters );

		if ( createTempLogFile() )
		{
			EnableWindow( GetDlgItem( hDlg, IDOK ), FALSE );

			while ( services.nextQuery( buffer, sizeof ( buffer ), lengthOut, countError ) )
			{
				char *pt = buffer;

				if ( services.checkIncrementForBackup( pt ) )
				{
					pos += 5;
					SendMessage( hWndBar, PBM_SETPOS, (WPARAM)pos , (LPARAM)NULL );
					*pt = UPPER( *pt );
					pt -= 7; // offest '<UL><B>' size = 7
					lengthOut -= pt - buffer;
					memcpy( pt, "<UL><B>", 7 );
					strcpy( &pt[lengthOut], "</UL></B>" );
					lengthOut += 9; // offest '</UL></B>' size = 9
					WriteFile( hTmpFile, pt, lengthOut, &dwWritten, NULL );
					continue;
				}

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

bool CServiceTabBackup::OnFindFileBackup()
{
	char * szCaption    = "Select Firebird backup file";
	char * szOpenFilter = "Firebird Backup Files (*.fbk;*.gbk)\0*.fbk;*.gbk\0"
                          "All files (*.*)\0*.*\0"
                          "\0";
	char * szDefExt     = "*.fbk";

	return CServiceTabChild::OnFindFile( szCaption, szOpenFilter, szDefExt, backupPathFile );
}

bool CServiceTabBackup::createDialogIndirect( CServiceTabCtrl *parentTabCtrl )
{
	CServiceTabChild::createDialogIndirect( parentTabCtrl );
	
	if ( backupPathFile.IsEmpty() )
		setDefaultName( "fbk", backupPathFile );
	
	hDlg = CreateDialogIndirect( m_hInstance,
                                 resource,
                                 parent,
                                 (DLGPROC)wndproCServiceTabBackup );
	return true;
}

bool CServiceTabBackup::buildDlgChild( HWND hWndParent )
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

	*p++ = 26;         // NumberOfItems

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
    TMP_EDITTEXT      ( IDC_BACKUP_FILE,7,35,246,12,ES_AUTOHSCROLL )
    TMP_PUSHBUTTON    ( "Browse",IDC_FIND_FILE_BACKUP,259,34,60,14 )
    TMP_EDITTEXT      ( IDC_USER,7,60,107,12,ES_UPPERCASE | ES_AUTOHSCROLL )
    TMP_EDITTEXT      ( IDC_PASSWORD,125,60,74,12,ES_PASSWORD | ES_AUTOHSCROLL )
    TMP_EDITTEXT      ( IDC_ROLE,212,60,107,12,ES_AUTOHSCROLL )
    TMP_EDITTEXT      ( IDC_BLOCKING_FACTOR,125,77,73,12,ES_AUTOHSCROLL )
    TMP_BUTTONCONTROL ( "Ignore checksums",IDC_CHECK_IGNORE_CHECKSUM,"Button", BS_AUTOCHECKBOX | WS_TABSTOP,16,97,71,10 )
    TMP_BUTTONCONTROL ( "Ignore transactions in limbo", IDC_CHECK_IGNORE_TRANS_LIMBO,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,16,110,97,10 )
    TMP_BUTTONCONTROL ( "Backup metadata only",IDC_CHECK_ONLY_METADATA,"Button", BS_AUTOCHECKBOX | WS_TABSTOP,16,123,83,10 )
    TMP_BUTTONCONTROL ( "Don't garbage collect database",IDC_CHECK_NO_GARBAGE, "Button",BS_AUTOCHECKBOX | WS_TABSTOP,16,136,111,10 )
    TMP_BUTTONCONTROL ( "Use old metadata format",IDC_CHECK_OLD_METADATA,"Button", BS_AUTOCHECKBOX | WS_TABSTOP,170,97,90,10 )
    TMP_BUTTONCONTROL ( "Non transportable format",IDC_CHECK_NO_TRANSPORTABLE, "Button",BS_AUTOCHECKBOX | WS_TABSTOP,170,110,90,10 )
    TMP_BUTTONCONTROL ( "Convert external tables",IDC_CHECK_CONV_EXT_TABLE, "Button",BS_AUTOCHECKBOX | WS_TABSTOP,170,123,84,10 )
    TMP_BUTTONCONTROL ( "Do not compress backup",IDC_CHECK_NO_COMPRESS,"Button", BS_AUTOCHECKBOX | WS_TABSTOP,170,136,91,10 )
	TMP_DEFPUSHBUTTON ( "Start backup",IDOK,72,172,88,14 )
    TMP_PUSHBUTTON    ( "View log",IDC_BUTTON_VIEW_LOG,169,172,85,14 )
    TMP_LTEXT         ( "Database",IDC_STATIC,7,0,218,8 )
    TMP_LTEXT         ( "Backup file",IDC_STATIC,7,25,218,8 )
    TMP_LTEXT         ( "Database Account",IDC_STATIC,7,50,107,8 )
    TMP_LTEXT         ( "Password",IDC_STATIC,126,50,72,8 )
    TMP_LTEXT         ( "Role",IDC_STATIC,214,50,105,8 )
    TMP_LTEXT         ( "Blocking factor (tape volumes)",IDC_STATIC,7,77,95,8 )
    TMP_GROUPBOX      ( "Backup options",IDC_STATIC,7,89,312,61 )
    TMP_NAMECONTROL   ( "ProgressBar", IDC_PROGRESS_BAR, "msctls_progress32",WS_BORDER,7,154,312,13 )

	return true;
}

}; // end namespace OdbcJdbcSetupLibrary
