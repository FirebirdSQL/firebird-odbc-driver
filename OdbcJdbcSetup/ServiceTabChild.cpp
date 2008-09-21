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

// ServiceTabChild.cpp:
//
//////////////////////////////////////////////////////////////////////
#include <windows.h>
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

void ProcessCDError( DWORD dwErrorCode, HWND hWnd );

CServiceTabChild::CServiceTabChild()
{
	tabCtrl = NULL;
	parent = NULL;
	hDlg = NULL;
	resource = NULL;
	hTmpFile = NULL;
	countError = 0;
}

CServiceTabChild::~CServiceTabChild()
{
	if ( resource ) {
		LocalFree( LocalHandle( resource ) );
		resource = NULL;
	}

	deleteTempLogFile();
}

bool CServiceTabChild::createTempLogFile()
{
	SECURITY_ATTRIBUTES sa = { sizeof ( SECURITY_ATTRIBUTES ), NULL, TRUE };
	char bufferTmpDir[MAX_PATH];
	char bufferTmpFileName[MAX_PATH];

	deleteTempLogFile();

	GetTempPath( MAX_PATH, bufferTmpDir );
	GetTempFileName( bufferTmpDir, "OFB", 0, bufferTmpFileName );
	DeleteFile( bufferTmpFileName );
	strcpy( strrchr( bufferTmpFileName, '.' ) + 1, "htm");

	hTmpFile = CreateFile( bufferTmpFileName,
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
						   &sa,
						   CREATE_ALWAYS,
						   FILE_ATTRIBUTE_TEMPORARY,
						   NULL );

	if ( hTmpFile == INVALID_HANDLE_VALUE )
	{
		hTmpFile = NULL;
		return false;
	}

	logPathFile = bufferTmpFileName;
	writeHeadToLogFile();
	return true;
}

void CServiceTabChild::deleteTempLogFile()
{
	countError = 0;

	if ( hTmpFile )
	{
		CloseHandle( hTmpFile );
		hTmpFile = NULL;
	}

	if ( !logPathFile.IsEmpty() )
	{
		DeleteFile( (const char*)logPathFile );
		logPathFile.setString( NULL );
	}
}

void CServiceTabChild::SetDisabledDlgItem( HWND hDlg, int ID, BOOL bDisabled )
{
	HWND hWnd = GetDlgItem( hDlg, ID );
	int style = GetWindowLong( hWnd, GWL_STYLE );
	if ( bDisabled )
		style |= WS_DISABLED;
	else			
		style &= ~WS_DISABLED;
	SetWindowLong( hWnd, GWL_STYLE, style );
	InvalidateRect( hWnd, NULL, TRUE );
}

void CServiceTabChild::updateData( HWND hDlg, BOOL bSaveAndValidate )
{
	if ( bSaveAndValidate )
	{
		GetDlgItemText( hDlg, IDC_DATABASE, database.getBuffer( 256 ), 256 );
		setServerName();
		GetDlgItemText( hDlg, IDC_PASSWORD, password.getBuffer( 256 ), 256 );
		GetDlgItemText( hDlg, IDC_USER, user.getBuffer( 256 ), 256 );
		GetDlgItemText( hDlg, IDC_ROLE, role.getBuffer( 256 ), 256 );
	}
	else
	{
		SetDlgItemText( hDlg, IDC_DATABASE, (const char *)database );
		SetDlgItemText( hDlg, IDC_PASSWORD, (const char *)password );
		SetDlgItemText( hDlg, IDC_USER, (const char *)user );
		SetDlgItemText( hDlg, IDC_ROLE, (const char *)role );
	}
}

bool CServiceTabChild::IsLocalhost( char *fullPathFileName, int &offset )
{
	char * ptStr = fullPathFileName;
	if(!ptStr)
		return false;

	bool nOk = false;
	offset = 0;

	while(*ptStr && *ptStr == ' ')ptStr++;
    if(!strncasecmp(ptStr,"localhost",9))
	{
		ptStr += 9;
		while(*ptStr && *ptStr == ' ')ptStr++;
		if( *ptStr == ':' )
		{
			offset = ptStr - fullPathFileName + 1;
			nOk = true;
		}
	}

	while( *ptStr )
	{
		if ( *ptStr == '/')*ptStr = '\\';
		++ptStr;
	}

	return nOk;
}

bool CServiceTabChild::setServerName()
{
	const char *pt = database;
	const char *end = pt;

	if ( database.IsEmpty() )
		return false;

	while ( *++end && *end != ':' );

	int length = end - pt;

	if ( *end == ':' && length > 1 )
	{
		server.Format( "%.*s", length, pt );
		return true;
	}

	server.setString( NULL );
	return false;
}

void CServiceTabChild::CheckRemotehost( char *fullPathFileName )
{
	char * ptCh, * ptStr = fullPathFileName;
	if(!ptStr)
		return;

	ptCh = ptStr;
	while( *ptCh == ' ' ) ++ptCh;

	if ( *(short*)ptCh == 0x5c5c ) // if '\\'
	{	// after "Browse"
		ptCh+=2;
		// name server	
		while( *ptCh && *ptCh != '\\' )
			*ptStr++ = *ptCh++;

		*ptCh++; //		*ptCh == '\\' alwaus
		*ptStr++ = ':';

		// name disk
		while( *ptCh && *ptCh != '\\' )
			*ptStr++ = *ptCh++;

		//	*ptCh == '\\' alwaus
		*ptStr++ = ':';

		while( *ptCh )
		{
			if ( *ptCh == '\\' )
				*ptStr++ = '/', *ptCh++;
			else
				*ptStr++ = *ptCh++;
		}

		*ptStr = '\0';
	}
	else
	{// find ':' without '\\';   c:\ it's not server
		while( *ptCh && *ptCh != ':') ++ptCh;

		if ( *ptCh == ':' && *(ptCh+1) != '\\')
		{
			char * ptNext;
			memmove(ptStr+2,ptStr,strlen(ptStr)+1);
			*(short*)ptStr = 0x5c5c;
			ptCh += 2;
			*ptCh++ = '\\';
			ptNext = ptCh;

			while( *ptNext )
			{
				if ( *ptNext == '/' ) 
					*ptCh++ = '\\', ++ptNext;
				else if ( *ptNext == ':' )
					++ptNext;
				else
					*ptCh++ = *ptNext++;
			}

			*ptCh = '\0';
		}
	}
}

bool CServiceTabChild::OnFindFileDatabase()
{
	int offset;
	BOOL bLocalhost;
    OPENFILENAME ofn;
    char strFullPathFileName[256];
    char achPathFileName[256];
	char * szOpenFilter =	"Firebird Database Files (*.fdb;*.gdb)\0*.fdb;*.gdb\0"
							"All files (*.*)\0*.*\0"
							"\0";

	strcpy( strFullPathFileName, (const char*)database );

	if ( bLocalhost = IsLocalhost( strFullPathFileName, offset ), bLocalhost )
	{
		memmove( strFullPathFileName, &strFullPathFileName[offset], strlen( strFullPathFileName ) - offset + 1 );
	}
	else
		CheckRemotehost( strFullPathFileName );

    ofn.lStructSize = sizeof ( OPENFILENAME );
    ofn.hwndOwner = NULL;
    ofn.hInstance = NULL;
    ofn.lpstrFilter = szOpenFilter; 
    ofn.lpstrCustomFilter = (LPSTR)NULL;
    ofn.nMaxCustFilter = 0L;
    ofn.nFilterIndex = 1L;              // first filter pair in list
    ofn.lpstrFile = strFullPathFileName; // we need to get the full path to open
    ofn.nMaxFile = sizeof ( achPathFileName );
    ofn.lpstrFileTitle = achPathFileName;    // return final elem of name here
    ofn.nMaxFileTitle = sizeof ( achPathFileName );
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = _TR( IDS_DLG_TITLE_FINDFILE_DATABASE, "Select Firebird database" );
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = "*.fdb";
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lCustData = 0;

    if ( !GetOpenFileName( &ofn ) )
	{
		ProcessCDError( CommDlgExtendedError(), NULL );
		return false;
	}

	if ( bLocalhost )
	{
		database = "localhost:";
		database += strFullPathFileName;
	}
	else
	{
		CheckRemotehost( strFullPathFileName );
		database = strFullPathFileName;
	}

	return true;
}

bool CServiceTabChild::OnFindFile( char *szCaption, char *szOpenFilter, char *szDefExt, JString &pathFile )
{
    OPENFILENAME ofn;
    char strFullPathFileName[256];
    char achPathFileName[256];

	strcpy( strFullPathFileName, (const char*)pathFile );

    ofn.lStructSize = sizeof ( OPENFILENAME );
    ofn.hwndOwner = NULL;
    ofn.hInstance = NULL;
    ofn.lpstrFilter = szOpenFilter; 
    ofn.lpstrCustomFilter = (LPSTR)NULL;
    ofn.nMaxCustFilter = 0L;
    ofn.nFilterIndex = 1L;              // first filter pair in list
    ofn.lpstrFile = strFullPathFileName; // we need to get the full path to open
    ofn.nMaxFile = sizeof ( achPathFileName );
    ofn.lpstrFileTitle = achPathFileName;    // return final elem of name here
    ofn.nMaxFileTitle = sizeof ( achPathFileName );
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = szCaption;
    ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = szDefExt;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lCustData = 0;

    if ( !GetOpenFileName( &ofn ) )
	{
		ProcessCDError( CommDlgExtendedError(), NULL );
		return false;
	}

	pathFile = strFullPathFileName;
	return true;
}

bool CServiceTabChild::setDefaultName( char *szDefExt, JString &pathFile )
{
	if ( database.IsEmpty() )
		return false;

	const char *pt = database;
	const char *chPoint = pt + database.length();

	if ( !server.IsEmpty() )
		pt += server.length() + 1; // name and ':'

	while ( chPoint > pt && *--chPoint != '.' );

	if ( *chPoint != '.' )
		return false;

	pathFile.Format( "%.*s.%s", chPoint - pt, pt, szDefExt );
	return true;
}

bool CServiceTabChild::createDialogIndirect( CServiceTabCtrl *parentTabCtrl )
{
	if ( !tabCtrl )
	{
		tabCtrl = parentTabCtrl;
		client = tabCtrl->client;
		database = tabCtrl->database;
		setServerName();
		password = tabCtrl->password;
		user = tabCtrl->user;
		role = tabCtrl->role;
	}

	hDlg = NULL;
	return true;
}

bool CServiceTabChild::buildDlgChild( HWND hWndParent )
{
	parent = hWndParent;
	return true;
}

bool CServiceTabChild::onCommand( HWND hWnd, int nCommand )
{
	switch ( nCommand ) 
	{
	case IDC_FIND_FILE:
		updateData( hWnd );
		if ( OnFindFileDatabase() )
			updateData( hWnd, FALSE );
		return true;
	}
	return false;
}

void CServiceTabChild::addParameters( CServiceClient &services )
{
	if ( !user.IsEmpty() )
		services.putParameterValue( SETUP_USER, user );
	if ( !password.IsEmpty() )
		services.putParameterValue( SETUP_PASSWORD, password );
	if ( !role.IsEmpty() )
		services.putParameterValue( SETUP_ROLE, role );
	if ( !database.IsEmpty() )
		services.putParameterValue( SETUP_DBNAME, database );
	if ( !client.IsEmpty() )
		services.putParameterValue( SETUP_CLIENT, client );
}

bool CServiceTabChild::viewLogFile()
{
	if ( !logPathFile.IsEmpty() )
	{
		SHELLEXECUTEINFO sh;
		memset( &sh, 0, sizeof ( SHELLEXECUTEINFO ) );
		sh.cbSize = sizeof ( SHELLEXECUTEINFO );
		sh.fMask = SEE_MASK_NOCLOSEPROCESS;
		sh.hwnd = parent;
		sh.lpVerb = "open";
		sh.lpParameters = (const char*)logPathFile;
		sh.lpFile = (const char*)tabCtrl->executorViewLogFile;
		sh.lpDirectory = NULL;
		sh.nShow = SW_SHOWNORMAL;

		if ( !(ShellExecuteEx( &sh ) && ((uintptr_t)sh.hInstApp > 32)) )
			return false;
	}
	return true;
}

void CServiceTabChild::writeHeadToLogFile()
{
	if ( hTmpFile )
	{
		char *head =
			"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">"
			"<HTML>"
			"<HEAD>"
			"<META HTTP-EQUIV=\"Content-Type\" Content=\"text/html; charset=Windows-1252\">"
			"<TITLE>OdbcFb log file</TITLE>"
			"</HEAD>"
			"<BODY BGCOLOR=\"#FFFFFF\" TEXT=\"#000000\">";

		DWORD dwWritten = 0;
		WriteFile( hTmpFile, head, (DWORD)strlen( head ), &dwWritten, NULL );
	}
}

void CServiceTabChild::writeFooterToLogFile()
{
	if ( hTmpFile )
	{
		char *footer = "</BODY></HTML>";
		DWORD dwWritten = 0;
		WriteFile( hTmpFile, footer, (DWORD)strlen( footer ), &dwWritten, NULL );
		FlushFileBuffers( hTmpFile );
		CloseHandle( hTmpFile );
		hTmpFile = NULL;
	}
}

CServiceTabChild* CServiceTabChild::getObject()
{
	return this;
}

}; // end namespace OdbcJdbcSetupLibrary
