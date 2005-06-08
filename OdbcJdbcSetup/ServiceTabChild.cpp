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

// ServiceTabChild.cpp: Service Backup Manager class.
//
//////////////////////////////////////////////////////////////////////
#include <windows.h>
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
	parent = NULL;
	resource = NULL;
}

CServiceTabChild::~CServiceTabChild()
{
	if ( resource )
		LocalFree( LocalHandle( resource ) );
}

void CServiceTabChild::SetDisabledDlgItem( HWND hDlg, int ID, BOOL bDisabled )
{
	HWND hWnd = GetDlgItem( hDlg, ID );
	int style = GetWindowLong( hWnd, GWL_STYLE );
	if ( bDisabled )style |= WS_DISABLED;
	else			style &= ~WS_DISABLED;
	SetWindowLong( hWnd, GWL_STYLE, style );
	InvalidateRect( hWnd, NULL, TRUE );
}

void CServiceTabChild::updateData( HWND hDlg, BOOL bSaveAndValidate )
{
	if ( bSaveAndValidate )
	{
		GetDlgItemText( hDlg, IDC_DATABASE, tabCtrl->database.getBuffer( 256 ), 256 );
		GetDlgItemText( hDlg, IDC_PASSWORD, tabCtrl->password.getBuffer( 256 ), 256 );
		GetDlgItemText( hDlg, IDC_USER, tabCtrl->user.getBuffer( 256 ), 256 );
		GetDlgItemText( hDlg, IDC_ROLE, tabCtrl->role.getBuffer( 256 ), 256 );
	}
	else
	{
		SetDlgItemText( hDlg, IDC_DATABASE, (const char *)tabCtrl->database );
		SetDlgItemText( hDlg, IDC_PASSWORD, (const char *)tabCtrl->password );
		SetDlgItemText( hDlg, IDC_USER, (const char *)tabCtrl->user );
		SetDlgItemText( hDlg, IDC_ROLE, (const char *)tabCtrl->role );
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
    if(!memicmp(ptStr,"localhost",9))
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

	strcpy( strFullPathFileName, (const char*)tabCtrl->database );

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
		tabCtrl->database = "localhost:";
		tabCtrl->database += strFullPathFileName;
	}
	else
	{
		CheckRemotehost( strFullPathFileName );
		tabCtrl->database = strFullPathFileName;
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

bool CServiceTabChild::createDialogIndirect( CServiceTabCtrl *parentTabCtrl )
{
	tabCtrl = parentTabCtrl;
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

CServiceTabChild* CServiceTabChild::getObject()
{
	return this;
}

}; // end namespace OdbcJdbcSetupLibrary
