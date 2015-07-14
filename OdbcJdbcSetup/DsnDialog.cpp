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
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 */
//
// DsnDialog.cpp : implementation file
//
#include <stdio.h>
#include <string.h>
#include "OdbcJdbcSetup.h"
#include "../IscDbc/Connection.h"
#include "CommonUtil.h"
#include "DsnDialog.h"
#include "../SetupAttributes.h"
#include "ServiceTabCtrl.h"
#include "ServiceClient.h"

namespace OdbcJdbcSetupLibrary {

extern HINSTANCE m_hInstance;
int currentCP;

TranslateString translate[] = 
{
	#include "res/resource.en"
,	
	#include "res/resource.ru"
,	
	#include "res/resource.uk"
,	
	#include "res/resource.es"
,	
	#include "res/resource.it"
};

int selectUserLCID( int userLCID )
{
	switch ( userLCID )
	{
	case 0x080a: //     esmx      // Spanish(Mexican)               //
 	case 0x0c0a: //     es        // Spanish(Spain - Modern Sort)   //
 	case 0x100a: //     esgt      // Spanish(Guatemala)             //
 	case 0x140a: //     escr      // Spanish(Costa Rica)            //
 	case 0x180a: //     espa      // Spanish(Panama)                //
 	case 0x1c0a: //     esdo      // Spanish(Dominican Republic)    //
 	case 0x200a: //     esve      // Spanish(Venezuela)             //
 	case 0x240a: //     esco      // Spanish(Colombia)              //
 	case 0x280a: //     espe      // Spanish(Peru)                  //
 	case 0x2c0a: //     esar      // Spanish(Argentina)             //
 	case 0x300a: //     esec      // Spanish(Ecuador)               //
 	case 0x340a: //     escl      // Spanish(Chile)                 //
 	case 0x380a: //     esuy      // Spanish(Uruguay)               //
 	case 0x3c0a: //     espy      // Spanish(Paraguay)              //
 	case 0x400a: //     esbo      // Spanish(Bolivia)               //
 	case 0x440a: //     essv      // Spanish(El Salvador)           //
 	case 0x480a: //     eshn      // Spanish(Honduras)              //
 	case 0x4c0a: //     esni      // Spanish(Nicaragua)             //
 	case 0x500a: //     espr      // Spanish(Puerto Rico)           //
			return 0x040a; // es  // Spanish(Spain-Traditional Sort)//

	case 0x0810: //     itch	  // Italian(Swiss)                 //
			return 0x0410; // it  // Italian(Standard)              //
	}

	return userLCID;
}

void initCodePageTranslate( int userLCID )
{
	int i;
	int count = sizeof ( translate ) / sizeof ( *translate );

	userLCID = selectUserLCID( userLCID );

	for( currentCP = -1, i = 0; i < count; i++ )
		if ( translate[i].userLCID == userLCID )
		{
			currentCP = i;
			break;
		}
}

using namespace IscDbcLibrary;

HINSTANCE instanceHtmlHelp = NULL;

BOOL CALLBACK wndprocDsnDialog( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void ProcessCDError(DWORD dwErrorCode, HWND hWnd);

CDsnDialog::CDsnDialog( HWND hDlgParent, 
					    const char **jdbcDrivers,
						const char **jdbcCharsets,
						const char **useShemasIdentifier )
{
	m_hWndDlg = NULL;
	m_hWndParent = hDlgParent;
	hwndHtmlHelp = NULL;

	m_database = "";
	m_client = "";
	m_name = "";
	m_description = "";
	m_password = "";
	m_user = "";
	m_driver = "";
	m_role = "";
	m_charset = "";
	m_locktimeout = "";
	m_useschema = "0";
	m_readonly = FALSE;
	m_nowait = FALSE;
	m_dialect3 = TRUE;
	m_quoted = TRUE;
	m_sensitive = FALSE;
	m_autoQuoted = FALSE;
	m_safeThread = FALSE;

	drivers = jdbcDrivers;
	charsets = jdbcCharsets;
	useshemas = useShemasIdentifier;
}

CDsnDialog::~CDsnDialog()
{
	if ( instanceHtmlHelp && hwndHtmlHelp )
		PostMessage( hwndHtmlHelp, WM_DESTROY, (WPARAM)0, (LPARAM)0 );
}

void CDsnDialog::SetDisabledDlgItem(HWND hDlg, int ID, BOOL bDisabled)
{
	HWND hWnd = GetDlgItem(hDlg, ID);
	int style = GetWindowLong(hWnd, GWL_STYLE);
	if ( bDisabled )style |= WS_DISABLED;
	else			style &= ~WS_DISABLED;
	SetWindowLong(hWnd, GWL_STYLE, style);
	InvalidateRect(hWnd, NULL, TRUE);
}

void CDsnDialog::UpdateData(HWND hDlg, BOOL bSaveAndValidate)
{
	HWND hWnd;

	if ( bSaveAndValidate )
	{
		GetDlgItemText(hDlg, IDC_DATABASE, m_database.getBuffer(256), 256);
		GetDlgItemText(hDlg, IDC_CLIENT, m_client.getBuffer(256), 256);
		GetDlgItemText(hDlg, IDC_NAME, m_name.getBuffer(256), 256);
		GetDlgItemText(hDlg, IDC_DESCRIPTION, m_description.getBuffer(256), 256);
		GetDlgItemText(hDlg, IDC_PASSWORD, m_password.getBuffer(256), 256);
		GetDlgItemText(hDlg, IDC_USER, m_user.getBuffer(256), 256);

		hWnd = GetDlgItem(hDlg, IDC_DRIVER);
		
		int nLen = GetWindowTextLength(hWnd);
		if (nLen > 0)
			GetWindowText(hWnd, m_driver.getBuffer(nLen), nLen+1);
		else
			GetWindowText(hWnd, m_driver.getBuffer(256), 256+1);

		GetDlgItemText(hDlg, IDC_ROLE, m_role.getBuffer(256), 256);

		hWnd = GetDlgItem(hDlg, IDC_CHARSET);
		
		nLen = GetWindowTextLength(hWnd);
		if (nLen > 0)
			GetWindowText(hWnd, m_charset.getBuffer(nLen), nLen+1);
		else
			GetWindowText(hWnd, m_charset.getBuffer(256), 256+1);

		hWnd = GetDlgItem(hDlg, IDC_COMBOBOX_USE_SCHEMA);
		
		intptr_t selectUse = SendMessage( hWnd, CB_GETCURSEL, (WPARAM)0, (LPARAM)0 );

		if ( selectUse == CB_ERR )
			selectUse = 0;

		*m_useschema.getBuffer(1) = selectUse + '0';

        m_readonly = SendDlgItemMessage(hDlg, IDC_CHECK_READ, BM_GETCHECK, 0, 0);
        m_nowait = SendDlgItemMessage(hDlg, IDC_CHECK_NOWAIT, BM_GETCHECK, 0, 0);

		hWnd = GetDlgItem(hDlg, IDC_LOCKTIMEOUT);

		nLen = GetWindowTextLength(hWnd);
		if (nLen > 0)
			GetWindowText(hWnd, m_locktimeout.getBuffer(nLen), nLen+1);
		else
			GetWindowText(hWnd, m_locktimeout.getBuffer(256), 256+1);

        m_dialect3 = IsDlgButtonChecked(hDlg, IDC_DIALECT3);

		m_quoted = SendDlgItemMessage(hDlg, IDC_CHECK_QUOTED, BM_GETCHECK, 0, 0);
		m_sensitive = SendDlgItemMessage(hDlg, IDC_CHECK_SENSITIVE, BM_GETCHECK, 0, 0);
		m_autoQuoted = SendDlgItemMessage(hDlg, IDC_CHECK_AUTOQUOTED, BM_GETCHECK, 0, 0);
		m_safeThread = SendDlgItemMessage(hDlg, IDC_CHECK_SFTHREAD, BM_GETCHECK, 0, 0);
	}
	else
	{
		SetDlgItemText( hDlg, IDC_DATABASE, (const char *)m_database );
		SetDlgItemText( hDlg, IDC_CLIENT, (const char *)m_client );
		SetDlgItemText( hDlg, IDC_NAME, (const char *)m_name );
		SetDlgItemText( hDlg, IDC_DESCRIPTION, (const char *)m_description );
		SetDlgItemText( hDlg, IDC_PASSWORD, (const char *)m_password );
		SetDlgItemText( hDlg, IDC_USER, (const char *)m_user );

		hWnd = GetDlgItem(hDlg, IDC_DRIVER);

		if (SendMessage(hWnd, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)(const char *)m_driver) == CB_ERR)
			SetWindowText(hWnd, (const char *)m_driver);

		SetDlgItemText(hDlg, IDC_ROLE, (const char *)m_role);

		hWnd = GetDlgItem(hDlg, IDC_CHARSET);

		if ( m_charset.IsEmpty() )
			SetWindowText( hWnd, (const char *)*charsets);
		else if ( SendMessage( hWnd, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)(const char *)m_charset) == CB_ERR )
			SetWindowText( hWnd, (const char *)m_charset);

		hWnd = GetDlgItem(hDlg, IDC_COMBOBOX_USE_SCHEMA);

		int selectUse = *m_useschema.getString() - '0';

		selectUse = SendMessage( hWnd, CB_SETCURSEL, (WPARAM)selectUse, (LPARAM)0 );
		
		if ( selectUse == CB_ERR )
			selectUse = 0;

		if ( selectUse == CB_ERR || m_useschema.IsEmpty() )
			SetWindowText( hWnd, _TR( IDS_USESCHEMA_NULL, (const char *)*useshemas ) );

        CheckDlgButton( hDlg, IDC_CHECK_READ, m_readonly );
        CheckDlgButton( hDlg, IDC_CHECK_NOWAIT, m_nowait );
		SetDlgItemText( hDlg, IDC_LOCKTIMEOUT, (const char *)m_locktimeout );

		if ( !m_nowait )
		{
			EnableWindow( GetDlgItem( hDlg, IDC_LOCKTIMEOUT ), TRUE );
			SetDisabledDlgItem( hDlg, IDC_STATIC_LOCKTIMEOUT, FALSE );
		}
		else
		{
			EnableWindow( GetDlgItem( hDlg, IDC_LOCKTIMEOUT ), FALSE );
			SetDisabledDlgItem( hDlg, IDC_STATIC_LOCKTIMEOUT );
		}

        CheckDlgButton( hDlg, IDC_CHECK_QUOTED, m_quoted );

		CheckRadioButton( hDlg, IDC_DIALECT3, IDC_DIALECT1, m_dialect3 ? IDC_DIALECT3 : IDC_DIALECT1 );
		if ( m_dialect3 )
		{
			SetDisabledDlgItem( hDlg, IDC_CHECK_QUOTED, FALSE );
			if ( m_quoted )
			{
				SetDisabledDlgItem( hDlg, IDC_CHECK_SENSITIVE, FALSE );
				SetDisabledDlgItem( hDlg, IDC_CHECK_AUTOQUOTED, FALSE );
			}
			else
			{
				SetDisabledDlgItem( hDlg, IDC_CHECK_SENSITIVE );
				SetDisabledDlgItem( hDlg, IDC_CHECK_AUTOQUOTED );
			}
		}
		else
		{
			SetDisabledDlgItem( hDlg, IDC_CHECK_SENSITIVE );
			SetDisabledDlgItem( hDlg, IDC_CHECK_AUTOQUOTED );
		}

        CheckDlgButton( hDlg, IDC_CHECK_SENSITIVE, m_sensitive );
        CheckDlgButton( hDlg, IDC_CHECK_AUTOQUOTED, m_autoQuoted );
        CheckDlgButton( hDlg, IDC_CHECK_SFTHREAD, m_safeThread );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDsnDialog message handlers

BOOL CDsnDialog::IsLocalhost(char * fullPathFileName, int &nSme)
{
	char * ptStr = fullPathFileName;
	if(!ptStr)
		return FALSE;

	int nOk = FALSE;
	nSme = 0;

	while(*ptStr && *ptStr == ' ')ptStr++;
    if(!strncasecmp(ptStr,"localhost",9))
	{
		ptStr += 9;
		while(*ptStr && *ptStr == ' ')ptStr++;
		if( *ptStr == ':' )
		{
			nSme = ptStr - fullPathFileName + 1;
			nOk = TRUE;
		}
	}

	while( *ptStr )
	{
		if ( *ptStr == '/')*ptStr = '\\';
		++ptStr;
	}

	return nOk;
}

void CDsnDialog::CheckRemotehost(char * fullPathFileName)
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

BOOL CDsnDialog::OnFindFile()
{
	int nSme;
	BOOL bLocalhost;
    OPENFILENAME ofn;
    char strFullPathFileName[256];
    char achPathFileName[256];
	char * szOpenFilter =	"Firebird Database Files (*.fdb;*.gdb)\0*.fdb;*.gdb\0"
							"All files (*.*)\0*.*\0"
							"\0";

	strcpy(strFullPathFileName,(const char*)m_database);

	if ( (bLocalhost = IsLocalhost(strFullPathFileName,nSme)),bLocalhost )
	{
		memmove(strFullPathFileName,&strFullPathFileName[nSme],strlen(strFullPathFileName) - nSme + 1);
	}
	else
		CheckRemotehost(strFullPathFileName);

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = NULL;
    ofn.hInstance = NULL;
    ofn.lpstrFilter = szOpenFilter; 
    ofn.lpstrCustomFilter = (LPSTR)NULL;
    ofn.nMaxCustFilter = 0L;
    ofn.nFilterIndex = 1L;              // first filter pair in list
    ofn.lpstrFile = strFullPathFileName; // we need to get the full path to open
    ofn.nMaxFile = sizeof(achPathFileName);
    ofn.lpstrFileTitle = achPathFileName;    // return final elem of name here
    ofn.nMaxFileTitle = sizeof(achPathFileName);
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = _TR( IDS_DLG_TITLE_FINDFILE_DATABASE, "Select Firebird database" );
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = "*.fdb";
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lCustData = 0;

    if (!GetOpenFileName(&ofn))
	{
		DWORD dwErrorCode = CommDlgExtendedError();

		if (dwErrorCode != FNERR_INVALIDFILENAME)
		{
			ProcessCDError(dwErrorCode, NULL );
			return FALSE;
		}

		strFullPathFileName[0] = '\0';

		if (!GetOpenFileName(&ofn))
		{
			ProcessCDError(CommDlgExtendedError(), NULL );
			return FALSE;
		}
	}

	if ( bLocalhost )
	{
		m_database = "localhost:";
		m_database += strFullPathFileName;
	}
	else
	{
		CheckRemotehost(strFullPathFileName);
		m_database = strFullPathFileName;
	}

	return TRUE;
}

BOOL CDsnDialog::OnFindFileClient()
{
    OPENFILENAME ofn;
    char strFullPathFileName[256];
    char achPathFileName[256];
	char * szOpenFilter =	"Firebird Client Files (*.dll)\0*.dll\0"
							"All files (*.*)\0*.*\0"
							"\0";

	strcpy(strFullPathFileName,(const char*)m_client);

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = NULL;
    ofn.hInstance = NULL;
    ofn.lpstrFilter = szOpenFilter; 
    ofn.lpstrCustomFilter = (LPSTR)NULL;
    ofn.nMaxCustFilter = 0L;
    ofn.nFilterIndex = 1L;              // first filter pair in list
    ofn.lpstrFile = strFullPathFileName; // we need to get the full path to open
    ofn.nMaxFile = sizeof(achPathFileName);
    ofn.lpstrFileTitle = achPathFileName;    // return final elem of name here
    ofn.nMaxFileTitle = sizeof(achPathFileName);
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = _TR( IDS_DLG_TITLE_FINDFILE_CLIENT, "Select Firebird client" );
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = "*.dll";
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lCustData = 0;

    if (!GetOpenFileName(&ofn))
	{
		ProcessCDError(CommDlgExtendedError(), NULL );
		return FALSE;
	}

	m_client = strFullPathFileName;

	return TRUE;
}

BOOL CDsnDialog::OnInitDialog( HWND hDlg )
{
	HWND hWndBox = GetDlgItem( hDlg, IDC_DRIVER );

	m_hWndDlg = hDlg;

	for (const char **driver = drivers; *driver; ++driver)
		SendMessage( hWndBox, CB_ADDSTRING, 0, (LPARAM)*driver );

	hWndBox = GetDlgItem( hDlg, IDC_CHARSET );

	for (const char **charset = charsets; *charset; ++charset)
		SendMessage( hWndBox, CB_ADDSTRING, 0, (LPARAM)*charset );

	hWndBox = GetDlgItem( hDlg, IDC_COMBOBOX_USE_SCHEMA );

	const char **useshema = useshemas;

	SendMessage( hWndBox, CB_ADDSTRING, 0, (LPARAM)_TR( IDS_USESCHEMA_NULL, *useshema++ ) );
	SendMessage( hWndBox, CB_ADDSTRING, 1, (LPARAM)_TR( IDS_USESCHEMA_DEL , *useshema++ ) );
	SendMessage( hWndBox, CB_ADDSTRING, 2, (LPARAM)_TR( IDS_USESCHEMA_FULL, *useshema++ ) );

	return TRUE;
}

#ifdef _WINDOWS

#ifndef _WIN64
#define DWORD_PTR DWORD
#endif

void CDsnDialog::WinHtmlHelp( HWND hDlg )
{
#ifdef UNICODE
	#define HTMLHELP_PROC "HtmlHelpW"
#else
	#define HTMLHELP_PROC "HtmlHelpA"
#endif

	if ( !instanceHtmlHelp )
	{
		instanceHtmlHelp = LoadLibrary("hhctrl.ocx");
	
		if ( !instanceHtmlHelp )
			return;
	}

	typedef HWND (WINAPI *HtmlHelpProc)( HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData);

	HtmlHelpProc fn = (HtmlHelpProc)GetProcAddress( instanceHtmlHelp, HTMLHELP_PROC );

	if ( !fn )
	{
		FreeLibrary( instanceHtmlHelp );
		instanceHtmlHelp = NULL;
		return;
	}

	char fileName [512];

	GetModuleFileName( m_hInstance, fileName, sizeof ( fileName ) );

	char *tail = strrchr( fileName, '\\' ) + 1;
	sprintf( tail, "%s.chm", DRIVER_NAME );

	hwndHtmlHelp = fn( hDlg, (LPCSTR)fileName, 0, 0 );
}

#endif

BOOL CALLBACK wndprocDsnDialog( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	CDsnDialog *dsnDialog = (CDsnDialog *)GetWindowLongPtr( hDlg, GW_USERDATA );

	switch ( message )
	{
    case WM_INITDIALOG:

	    SetWindowLongPtr( hDlg, GW_USERDATA, lParam ); 
		if ( !((CDsnDialog*)lParam)->OnInitDialog( hDlg ) )
			return FALSE;
		((CDsnDialog*)lParam)->UpdateData( hDlg, FALSE );
		return TRUE;

	case WM_COMMAND:
        switch ( LOWORD( wParam ) ) 
		{
        case IDCANCEL:
			EndDialog( hDlg, FALSE );
			return TRUE;

		case IDC_FIND_FILE:
			dsnDialog->UpdateData( hDlg );
			if ( dsnDialog->OnFindFile() )
				dsnDialog->UpdateData( hDlg, FALSE );
			break;

		case IDC_FIND_FILE_CLIENT:
			dsnDialog->UpdateData( hDlg );
			if ( dsnDialog->OnFindFileClient() )
				dsnDialog->UpdateData( hDlg, FALSE );
			break;

        case IDC_BUTTON_SERVICE:
			{
				CServiceTabCtrl dialog( dsnDialog->m_hWndDlg );

				dsnDialog->UpdateData( hDlg );
				dialog.client = dsnDialog->m_client;
				dialog.database = dsnDialog->m_database;
				dialog.user = dsnDialog->m_user;
				dialog.password = dsnDialog->m_password;
				dialog.role = dsnDialog->m_role;

				dialog.DoModal();
			}
			break;

        case IDC_TEST_CONNECTION:
			dsnDialog->UpdateData( hDlg );
			dsnDialog->OnTestConnection( hDlg );
			break;

		case IDC_DIALECT1:
			dsnDialog->SetDisabledDlgItem( hDlg, IDC_CHECK_SENSITIVE );
			dsnDialog->SetDisabledDlgItem( hDlg, IDC_CHECK_AUTOQUOTED );
			break;

		case IDC_DIALECT3:
			dsnDialog->UpdateData( hDlg );
			if ( dsnDialog->m_quoted )
			{
				dsnDialog->SetDisabledDlgItem( hDlg, IDC_CHECK_SENSITIVE, FALSE );
				dsnDialog->SetDisabledDlgItem( hDlg, IDC_CHECK_AUTOQUOTED, FALSE );
			}
			break;

		case IDC_CHECK_QUOTED:
			dsnDialog->UpdateData( hDlg );
			if ( !dsnDialog->m_quoted )
			{
				dsnDialog->SetDisabledDlgItem( hDlg, IDC_CHECK_SENSITIVE );
				dsnDialog->SetDisabledDlgItem( hDlg, IDC_CHECK_AUTOQUOTED );
			}
			else if ( dsnDialog->m_dialect3 )
			{
				dsnDialog->SetDisabledDlgItem( hDlg, IDC_CHECK_SENSITIVE, FALSE );
				dsnDialog->SetDisabledDlgItem( hDlg, IDC_CHECK_AUTOQUOTED, FALSE );
			}
			break;

		case IDC_CHECK_NOWAIT:
			dsnDialog->UpdateData( hDlg );
			if ( !dsnDialog->m_nowait )
			{
				EnableWindow( GetDlgItem( hDlg, IDC_LOCKTIMEOUT ), TRUE );
				dsnDialog->SetDisabledDlgItem( hDlg, IDC_STATIC_LOCKTIMEOUT, FALSE );
			}
			else
			{
				EnableWindow( GetDlgItem( hDlg, IDC_LOCKTIMEOUT ), FALSE );
				dsnDialog->SetDisabledDlgItem( hDlg, IDC_STATIC_LOCKTIMEOUT );
			}
			break;

        case IDC_HELP_ODBC:
			dsnDialog->WinHtmlHelp( hDlg );
			break;

        case IDOK:
			dsnDialog->UpdateData( hDlg );
			EndDialog( hDlg, TRUE );
            return TRUE;
        }
        break;
	}
    return FALSE ;
}

#ifdef _WINDOWS

void CDsnDialog::removeNameFileDBfromMessage(char * message)
{
	char * pt = message;

	while ( (pt = strstr ( pt, "for file" )) )
	{
		while ( *pt && *pt != '"' ) ++pt;
		if ( *pt && *pt == '"' )
		{
			char * beg = pt++;

			while ( *pt && *pt != '"' ) ++pt;
			if ( *pt && *pt == '"' )
			{
				++pt;
				*beg++ = 'D';
				*beg++ = 'B';
				memmove ( beg, pt, strlen(pt) + 1 );
			}
		}
	}
}

void CDsnDialog::OnTestConnection( HWND hDlg )
{
    char strHeadDlg[256];

	GetWindowText( hDlg, strHeadDlg, sizeof ( strHeadDlg ) );

	try
	{
		CServiceClient services;

		UpdateData( hDlg );

		if ( !services.initServices( m_driver ) )
		{
			JString text;
			text.Format ( _TR( IDS_ERROR_MESSAGE_01, "Unable to connect to data source: library '%s' failed to load" ), (const char *)m_driver );
			MessageBox( hDlg, text, TEXT(strHeadDlg), MB_ICONERROR|MB_OK);
			return;
		}

		if ( !services.checkVersion() )
		{
			JString text;
			text.Format ( _TR( IDS_ERROR_MESSAGE_03, " Unable to load %s Library : can't find ver. %s " ), (const char *)m_driver, DRIVER_VERSION );
			MessageBox( hDlg, (const char*)text, _TR( IDS_MESSAGE_02, "Connection failed!" ), MB_ICONINFORMATION|MB_OK);
			return;
		}

		if ( !m_user.IsEmpty() )
			services.putParameterValue( SETUP_USER, m_user );
		if ( !m_password.IsEmpty() )
			services.putParameterValue( SETUP_PASSWORD, m_password );
		if ( !m_role.IsEmpty() )
			services.putParameterValue( SETUP_ROLE, m_role );
		if ( !m_charset.IsEmpty() )
			services.putParameterValue( KEY_DSN_CHARSET, m_charset );
		if ( !m_client.IsEmpty() )
			services.putParameterValue( SETUP_CLIENT, m_client );
		
		services.putParameterValue( SETUP_DIALECT, m_dialect3 ? "3" : "1" );
		services.putParameterValue( SETUP_DBNAME, m_database );

		services.openDatabase();

		MessageBox( hDlg, _TR( IDS_MESSAGE_01, "Connection successful!" ), TEXT( strHeadDlg ), MB_ICONINFORMATION | MB_OK );
	}
	catch ( std::exception &ex )
	{
		SQLException &exception = (SQLException&)ex;
		char buffer[2048];
		JString text = exception.getText();

		sprintf( buffer, "%s\n%s", _TR( IDS_MESSAGE_02, "Connection failed!" ), (const char*)text );
		removeNameFileDBfromMessage ( buffer );

		MessageBox( hDlg, TEXT( buffer ), TEXT( strHeadDlg ), MB_ICONERROR | MB_OK );
	}
}
#endif

void ProcessCDError(DWORD dwErrorCode, HWND hWnd)
{
	LPCTSTR  stringID;

	switch(dwErrorCode)
	{
	case CDERR_DIALOGFAILURE:   stringID="Creation of 'CD' failed on call to DialogBox()";   break;
	case CDERR_STRUCTSIZE:      stringID="Invalid structure size passed to CD";      break;
	case CDERR_INITIALIZATION:  stringID="Failure initializing CD.  Possibly\n\r do to insufficient memory.";  break;
	case CDERR_NOTEMPLATE:      stringID="Failure finding custom template for CD";      break;
	case CDERR_NOHINSTANCE:     stringID="Instance handle not passed to CD";     break;
	case CDERR_LOADSTRFAILURE:  stringID="Failure loading specified string";  break;
	case CDERR_FINDRESFAILURE:  stringID="Failure finding specified resource";  break;
	case CDERR_LOADRESFAILURE:  stringID="Failure loading specified resource";  break;
	case CDERR_LOCKRESFAILURE:  stringID="Failure locking specified resource";  break;
	case CDERR_MEMALLOCFAILURE: stringID="Failure allocating memory for internal CD structure"; break;
	case CDERR_MEMLOCKFAILURE:  stringID="Failure locking memory for internal CD structure";  break;
	case CDERR_NOHOOK:          stringID="No hook function passed to CD but ENABLEHOOK\n\r was passed as a flag";          break;
	case PDERR_SETUPFAILURE:    stringID="Failure setting up resources for CD";    break;
	case PDERR_PARSEFAILURE:    stringID="Failure parsing strings in [devices]\n\rsection of WIN.INI";    break;
	case PDERR_RETDEFFAILURE:   stringID="PD_RETURNDEFAULT flag was set but either the \n\rhDevMode or hDevNames field was nonzero";   break;
	case PDERR_LOADDRVFAILURE:  stringID="Failure loading the printers device driver";  break;
	case PDERR_GETDEVMODEFAIL:  stringID="The printer driver failed to initialize a DEVMODE data structure";  break;
	case PDERR_INITFAILURE:     stringID="Print CD failed during initialization";     break;
	case PDERR_NODEVICES:       stringID="No printer device drivers were found";       break;
	case PDERR_NODEFAULTPRN:    stringID="No default printer was found";    break;
	case PDERR_DNDMMISMATCH:    stringID="Data in DEVMODE contradicts data in DEVNAMES";    break;
	case PDERR_CREATEICFAILURE: stringID="Failure creating an IC"; break;
	case PDERR_PRINTERNOTFOUND: stringID="Printer not found"; break;
	case CFERR_NOFONTS:         stringID="No fonts exist";         break;
	case FNERR_SUBCLASSFAILURE: stringID="Failure subclassing during initialization of CD"; break;
	case FNERR_INVALIDFILENAME: stringID="Invalide filename passed to FileOpen"; break;
	case FNERR_BUFFERTOOSMALL:  stringID="Buffer passed to CD too small to accomodate string";  break;

	case 0:   //User may have hit CANCEL or we got a *very* random error
		return;

	default:
		stringID="Unknown error.";
	}

	MessageBox( hWnd, stringID, TEXT( _TR( IDS_DLG_TITLE_SETUP, "Firebird ODBC Setup" ) ), MB_OK );
}

intptr_t CDsnDialog::DoModal()
{
	WORD  *p, *pdlgtemplate;
	int   nchar;
	DWORD lStyle;

	pdlgtemplate = p = (PWORD)LocalAlloc( LPTR, 4096 );
	lStyle = DS_SETFONT | DS_MODALFRAME | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE;

	*p++ = LOWORD (lStyle);
	*p++ = HIWORD (lStyle);
	*p++ = 0;          // LOWORD (lExtendedStyle)
	*p++ = 0;          // HIWORD (lExtendedStyle)

	*p++ = 40;         // NumberOfItems

	*p++ = 0;          // x
	*p++ = 0;          // y
	*p++ = 310;        // cx
	*p++ = 252;        // cy
	*p++ = 0;          // Menu
	*p++ = 0;          // Class

	// copy the title of the dialog
	nchar = nCopyAnsiToWideChar (p, TEXT( _TR( IDS_DLG_TITLE_SETUP, "Firebird ODBC Setup" ) ) );
	p += nchar;

	*p++ = 8;          // FontSize
	nchar = nCopyAnsiToWideChar (p, TEXT("MS Sans Serif"));
	p += nchar;

    TMP_EDITTEXT      ( IDC_NAME,7,12,296,12,ES_AUTOHSCROLL )
    TMP_LTEXT         ( _TR( IDS_STATIC_DESCRIPTION, "Description" ), IDC_STATIC,7,26,218,8 )
    TMP_EDITTEXT      ( IDC_DESCRIPTION,7,36,296,12,ES_AUTOHSCROLL )
    TMP_EDITTEXT      ( IDC_DATABASE,7,61,231,12,ES_AUTOHSCROLL )
    TMP_PUSHBUTTON    ( _TR( IDS_BUTTON_FIND_FILE, "Browse" ), IDC_FIND_FILE,243,60,60,14 )
    TMP_EDITTEXT      ( IDC_CLIENT,7,86,231,12,ES_AUTOHSCROLL )
    TMP_PUSHBUTTON    ( _TR( IDS_BUTTON_FIND_FILE, "Browse" ), IDC_FIND_FILE_CLIENT,243,85,60,14 )
    TMP_EDITTEXT      ( IDC_USER,7,111,105,12,ES_UPPERCASE | ES_AUTOHSCROLL )
    TMP_EDITTEXT      ( IDC_PASSWORD,118,111,90,12,ES_PASSWORD | ES_AUTOHSCROLL )
    TMP_EDITTEXT      ( IDC_ROLE,213,111,90,12,ES_AUTOHSCROLL )
    TMP_COMBOBOX      ( IDC_CHARSET,7,135,106,120,CBS_DROPDOWN | WS_VSCROLL | WS_TABSTOP )
    TMP_BUTTONCONTROL ( _TR( IDS_CHECK_READ, "read (default write)" ), IDC_CHECK_READ,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,14,168,136,10 )
    TMP_BUTTONCONTROL ( _TR( IDS_CHECK_NOWAIT, "nowait (default wait)" ), IDC_CHECK_NOWAIT,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,14,178,136,10 )
    TMP_EDITTEXT      ( IDC_LOCKTIMEOUT,24,188,23,10,ES_AUTOHSCROLL )
    TMP_LTEXT         ( _TR( IDS_STATIC_LOCKTIMEOUT, "Lock Timeout" ), IDC_STATIC_LOCKTIMEOUT,50,189,86,8 )
    TMP_DEFPUSHBUTTON ( _TR( IDS_BUTTON_OK, "OK" ), IDOK,86,233,60,14 )
    TMP_PUSHBUTTON    ( _TR( IDS_BUTTON_CANCEL, "Cancel" ), IDCANCEL,154,233,60,14 )
    TMP_LTEXT         ( _TR( IDS_STATIC_DSN, "Data Source Name (DSN)" ), IDC_STATIC,7,2,167,8 )
    TMP_LTEXT         ( _TR( IDS_STATIC_DATABASE, "Database" ), IDC_STATIC,7,51,218,8 )
    TMP_LTEXT         ( _TR( IDS_STATIC_ACCOUNT, "Database Account" ), IDC_STATIC,7,101,107,8 )
    TMP_LTEXT         ( _TR( IDS_STATIC_PASSWORD, "Password" ), IDC_STATIC,118,101,72,8 )
    TMP_LTEXT         ( _TR( IDS_STATIC_ROLE, "Role" ), IDC_STATIC,213,101,105,8 )
    TMP_LTEXT         ( _TR( IDS_STATIC_CHARSET, "Character Set" ), IDC_STATIC,7,125,98,8 )
    TMP_GROUPBOX      ( _TR( IDS_GROUPBOX_OPTIONS, "Options" ), IDC_STATIC,7,151,296,77 )
    TMP_GROUPBOX      ( _TR( IDS_GROUPBOX_INIT_TRANSACTION, "Transaction" ), IDC_STATIC,10,159,142,42 )
    TMP_LTEXT         ( _TR( IDS_STATIC_CLIENT, "Client" ), IDC_STATIC,7,76,218,8 )
    TMP_GROUPBOX      ( "", IDC_STATIC,10,196,142,17 )
    TMP_LTEXT         ( _TR( IDS_GROUPBOX_DIALECT, "Dialect" ), IDC_STATIC,14,202,41,10 )
    TMP_RADIOCONTROL  ( "3",IDC_DIALECT3,"Button",BS_AUTORADIOBUTTON,61,202,16,10 )
    TMP_RADIOCONTROL  ( "1",IDC_DIALECT1,"Button",BS_AUTORADIOBUTTON,91,202,16,10 )
    TMP_GROUPBOX      ( "", IDC_STATIC,10,208,142,17 )
    TMP_BUTTONCONTROL ( _TR( IDS_CHECK_SFTHREAD, "safe thread" ), IDC_CHECK_SFTHREAD,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,14,214,136,10 )
    TMP_GROUPBOX      ( _TR( IDS_GROUPBOX_EXT_PROPERTY, "Extended identifier properties" ), IDC_STATIC,154,159,146,66 )
    TMP_BUTTONCONTROL ( _TR( IDS_CHECK_QUOTED, "quoted identifiers" ), IDC_CHECK_QUOTED,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,159,169,139,9 )
    TMP_BUTTONCONTROL ( _TR( IDS_CHECK_SENSITIVE, "sensitive identifier" ), IDC_CHECK_SENSITIVE,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,159,181,139,9 )
    TMP_BUTTONCONTROL ( _TR( IDS_CHECK_AUTOQUOTED, "autoquoted identifier" ), IDC_CHECK_AUTOQUOTED,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,159,193,139,9 )
    TMP_COMBOBOX      ( IDC_COMBOBOX_USE_SCHEMA,159,207,136,60,CBS_DROPDOWN | WS_VSCROLL | WS_TABSTOP )
    TMP_PUSHBUTTON    ( _TR( IDS_BUTTON_TEST_CONNECTION, "Test connection" ), IDC_TEST_CONNECTION,213,130,90,18 )
    TMP_PUSHBUTTON    ( _TR( IDS_BUTTON_SERVICES, "Services" ), IDC_BUTTON_SERVICE,118,130,90,18 )
    TMP_PUSHBUTTON    ( _TR( IDS_BUTTON_HELP_ODBC, "Help" ), IDC_HELP_ODBC,243,233,60,14 )

	intptr_t nRet = DialogBoxIndirectParam(m_hInstance, (LPDLGTEMPLATE) pdlgtemplate, m_hWndParent, (DLGPROC)wndprocDsnDialog, (LPARAM)this );
	LocalFree (LocalHandle (pdlgtemplate));

	return nRet;
}

}; // end namespace OdbcJdbcSetupLibrary
