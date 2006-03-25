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
#include "DsnDialog.h"
#include "../SetupAttributes.h"

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

using namespace classJString;
using namespace IscDbcLibrary;

CDsnDialog * m_ptDsnDialog = NULL;
HINSTANCE instanceHtmlHelp = NULL;

BOOL CALLBACK wndprocDsnDialog(HWND hDlg, UINT message, WORD wParam, LONG lParam);
void ProcessCDError(DWORD dwErrorCode, HWND hWnd);

CDsnDialog::CDsnDialog( const char **jdbcDrivers,
						const char **jdbcCharsets,
						const char **useShemasIdentifier )
{
	hwndHtmlHelp = NULL;

	m_database = "";
	m_client = "";
	m_name = "";
	m_password = "";
	m_user = "";
	m_driver = "";
	m_role = "";
	m_charset = "";
	m_useschema = "0";
	m_readonly = FALSE;
	m_nowait = FALSE;
	m_dialect3 = TRUE;
	m_quoted = TRUE;
	m_sensitive = FALSE;
	m_autoQuoted = FALSE;

	drivers = jdbcDrivers;
	charsets = jdbcCharsets;
	useshemas = useShemasIdentifier;
	m_ptDsnDialog = this;
}

CDsnDialog::~CDsnDialog()
{
	if ( instanceHtmlHelp && hwndHtmlHelp )
		PostMessage( hwndHtmlHelp, WM_DESTROY, (WPARAM)0, (LPARAM)0 );

	m_ptDsnDialog = NULL;
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
		
		int selectUse = SendMessage( hWnd, CB_GETCURSEL, (WPARAM)0, (LPARAM)0 );

		if ( selectUse == CB_ERR )
			selectUse = 0;

		*m_useschema.getBuffer(1) = selectUse + '0';

        m_readonly = SendDlgItemMessage(hDlg, IDC_CHECK_READ, BM_GETCHECK, 0, 0);
        m_nowait = SendDlgItemMessage(hDlg, IDC_CHECK_NOWAIT, BM_GETCHECK, 0, 0);

        m_dialect3 = IsDlgButtonChecked(hDlg, IDC_DIALECT3);

		m_quoted = SendDlgItemMessage(hDlg, IDC_CHECK_QUOTED, BM_GETCHECK, 0, 0);
		m_sensitive = SendDlgItemMessage(hDlg, IDC_CHECK_SENSITIVE, BM_GETCHECK, 0, 0);
		m_autoQuoted = SendDlgItemMessage(hDlg, IDC_CHECK_AUTOQUOTED, BM_GETCHECK, 0, 0);
	}
	else
	{
		SetDlgItemText(hDlg, IDC_DATABASE, (const char *)m_database);
		SetDlgItemText(hDlg, IDC_CLIENT, (const char *)m_client);
		SetDlgItemText(hDlg, IDC_NAME, (const char *)m_name);
		SetDlgItemText(hDlg, IDC_PASSWORD, (const char *)m_password);
		SetDlgItemText(hDlg, IDC_USER, (const char *)m_user);

		hWnd = GetDlgItem(hDlg, IDC_DRIVER);

		if (SendMessage(hWnd, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)(const char *)m_driver) == CB_ERR)
			SetWindowText(hWnd, (const char *)m_driver);

		SetDlgItemText(hDlg, IDC_ROLE, (const char *)m_role);

		hWnd = GetDlgItem(hDlg, IDC_CHARSET);

		if ( m_charset.IsEmpty() )
			SetWindowText(hWnd, (const char *)*charsets);
		else if (SendMessage(hWnd, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)(const char *)m_charset) == CB_ERR)
			SetWindowText(hWnd, (const char *)m_charset);

		hWnd = GetDlgItem(hDlg, IDC_COMBOBOX_USE_SCHEMA);

		int selectUse = *m_useschema.getString() - '0';

		selectUse = SendMessage( hWnd, CB_SETCURSEL, (WPARAM)selectUse, (LPARAM)0 );
		
		if ( selectUse == CB_ERR )
			selectUse = 0;

		if ( selectUse == CB_ERR || m_useschema.IsEmpty() )
			SetWindowText( hWnd, _TR( IDS_USESCHEMA_NULL, (const char *)*useshemas ) );

        CheckDlgButton(hDlg, IDC_CHECK_READ, m_readonly);
        CheckDlgButton(hDlg, IDC_CHECK_NOWAIT, m_nowait);
        CheckDlgButton ( hDlg, IDC_CHECK_QUOTED, m_quoted );

		CheckRadioButton(hDlg, IDC_DIALECT3, IDC_DIALECT1, m_dialect3 ? IDC_DIALECT3 : IDC_DIALECT1);
		if ( m_dialect3 )
		{
			SetDisabledDlgItem ( hDlg, IDC_CHECK_QUOTED, FALSE );
			if ( m_quoted )
			{
				SetDisabledDlgItem ( hDlg, IDC_CHECK_SENSITIVE, FALSE );
				SetDisabledDlgItem ( hDlg, IDC_CHECK_AUTOQUOTED, FALSE );
			}
			else
			{
				SetDisabledDlgItem ( hDlg, IDC_CHECK_SENSITIVE );
				SetDisabledDlgItem ( hDlg, IDC_CHECK_AUTOQUOTED );
			}
		}
		else
		{
			SetDisabledDlgItem ( hDlg, IDC_CHECK_SENSITIVE );
			SetDisabledDlgItem ( hDlg, IDC_CHECK_AUTOQUOTED );
		}

        CheckDlgButton ( hDlg, IDC_CHECK_SENSITIVE, m_sensitive );
        CheckDlgButton ( hDlg, IDC_CHECK_AUTOQUOTED, m_autoQuoted );
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
    if(!memicmp(ptStr,"localhost",9))
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
		ProcessCDError(CommDlgExtendedError(), NULL );
		return FALSE;
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

int DialogBoxDynamic();

int CDsnDialog::DoModal()
{
	return DialogBoxDynamic();
}

BOOL CDsnDialog::OnInitDialog(HWND hDlg) 
{
	HWND hWndBox = GetDlgItem(hDlg, IDC_DRIVER);

	for (const char **driver = drivers; *driver; ++driver)
		SendMessage(hWndBox, CB_ADDSTRING, 0, (LPARAM)*driver);

	hWndBox = GetDlgItem(hDlg, IDC_CHARSET);

	for (const char **charset = charsets; *charset; ++charset)
		SendMessage(hWndBox, CB_ADDSTRING, 0, (LPARAM)*charset);

	hWndBox = GetDlgItem( hDlg, IDC_COMBOBOX_USE_SCHEMA );

	const char **useshema = useshemas;

	SendMessage( hWndBox, CB_ADDSTRING, 0, (LPARAM)_TR( IDS_USESCHEMA_NULL, *useshema++ ) );
	SendMessage( hWndBox, CB_ADDSTRING, 1, (LPARAM)_TR( IDS_USESCHEMA_DEL , *useshema++ ) );
	SendMessage( hWndBox, CB_ADDSTRING, 2, (LPARAM)_TR( IDS_USESCHEMA_FULL, *useshema++ ) );

	return TRUE;
}

#ifdef _WIN32

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

BOOL CALLBACK wndprocDsnDialog( HWND hDlg, UINT message, WORD wParam, LONG lParam )
{
	switch (message) 
	{
    case WM_INITDIALOG:

		if ( !m_ptDsnDialog->OnInitDialog(hDlg) )
			return FALSE;

		m_ptDsnDialog->UpdateData(hDlg, FALSE);

		return TRUE;

	case WM_COMMAND:
        switch (LOWORD(wParam)) 
		{
        case IDCANCEL:
			EndDialog(hDlg, FALSE);
			return(TRUE);

		case IDC_FIND_FILE:
			m_ptDsnDialog->UpdateData(hDlg);
			if ( m_ptDsnDialog->OnFindFile() )
				m_ptDsnDialog->UpdateData(hDlg, FALSE);
			break;

		case IDC_FIND_FILE_CLIENT:
			m_ptDsnDialog->UpdateData(hDlg);
			if ( m_ptDsnDialog->OnFindFileClient() )
				m_ptDsnDialog->UpdateData(hDlg, FALSE);
			break;

        case IDC_TEST_CONNECTION:
			m_ptDsnDialog->OnTestConnection(hDlg);
			break;

		case IDC_DIALECT1:
			m_ptDsnDialog->SetDisabledDlgItem(hDlg, IDC_CHECK_SENSITIVE);
			m_ptDsnDialog->SetDisabledDlgItem(hDlg, IDC_CHECK_AUTOQUOTED);
			break;

		case IDC_DIALECT3:
			m_ptDsnDialog->UpdateData(hDlg);
			if ( m_ptDsnDialog->m_quoted )
			{
				m_ptDsnDialog->SetDisabledDlgItem(hDlg, IDC_CHECK_SENSITIVE, FALSE);
				m_ptDsnDialog->SetDisabledDlgItem(hDlg, IDC_CHECK_AUTOQUOTED, FALSE);
			}
			break;

		case IDC_CHECK_QUOTED:
			m_ptDsnDialog->UpdateData(hDlg);
			if ( !m_ptDsnDialog->m_quoted )
			{
				m_ptDsnDialog->SetDisabledDlgItem(hDlg, IDC_CHECK_SENSITIVE);
				m_ptDsnDialog->SetDisabledDlgItem(hDlg, IDC_CHECK_AUTOQUOTED);
			}
			else if ( m_ptDsnDialog->m_dialect3 )
			{
				m_ptDsnDialog->SetDisabledDlgItem(hDlg, IDC_CHECK_SENSITIVE, FALSE);
				m_ptDsnDialog->SetDisabledDlgItem(hDlg, IDC_CHECK_AUTOQUOTED, FALSE);
			}
			break;

        case IDC_HELP_ODBC:
			m_ptDsnDialog->WinHtmlHelp( hDlg );
			break;

        case IDOK:
			m_ptDsnDialog->UpdateData(hDlg);
            EndDialog(hDlg, TRUE);
            return TRUE;
        }
        break;
	}
    return FALSE ;
}

#ifdef _WIN32

typedef Connection* (*ConnectFn)();

int CDsnDialog::getDriverBuildKey()
{
	return MAJOR_VERSION * 1000000 + MINOR_VERSION * 10000 + BUILDNUM_VERSION;
}

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

void CDsnDialog::OnTestConnection(HWND hDlg)
{
	Connection	* connection = NULL;
	Properties *properties = NULL;
	HINSTANCE libraryHandle;
    char strHeadDlg[256];

	GetWindowText(hDlg,strHeadDlg,sizeof(strHeadDlg));

	try
	{
		UpdateData(hDlg);

		libraryHandle = LoadLibrary ((const char *)m_driver);
		if ( !libraryHandle )
		{
			JString text;
			text.Format ( _TR( IDS_ERROR_MESSAGE_01, "Unable to connect to data source: library '%s' failed to load" ), (const char *)m_driver);
			MessageBox(hDlg, text, TEXT(strHeadDlg), MB_ICONERROR|MB_OK);
			return;
		}
#ifdef __BORLANDC__
		ConnectFn fn = (ConnectFn) GetProcAddress (libraryHandle, "_createConnection");
#else
		ConnectFn fn = (ConnectFn) GetProcAddress (libraryHandle, "createConnection");
#endif
		if (!fn)
		{
			JString text;
			text.Format ( _TR( IDS_ERROR_MESSAGE_02, "Unable to connect to data source %s: can't find entrypoint 'createConnection'" ) );
			MessageBox(hDlg, text, TEXT(strHeadDlg), MB_ICONERROR|MB_OK);
			return;
		}

		connection = (fn)();

		if ( getDriverBuildKey() != connection->getDriverBuildKey() )
		{
			connection->close();
			connection = NULL;

			FreeLibrary(libraryHandle);
			libraryHandle = NULL;

			JString text;
			text.Format ( _TR( IDS_ERROR_MESSAGE_03, " Unable to load %s Library : can't find ver. %s " ), (const char *)m_driver, DRIVER_VERSION);
			MessageBox(hDlg, (const char*)text, _TR( IDS_MESSAGE_02, "Connection failed!" ), MB_ICONINFORMATION|MB_OK);
			return;
		}

		properties = connection->allocProperties();
		if ( !m_name.IsEmpty() )
			properties->putValue ("user", (const char*)m_user);
		if ( !m_password.IsEmpty() )
			properties->putValue ("password", (const char*)m_password);
		if ( !m_role.IsEmpty() )
			properties->putValue ("role", (const char*)m_role);
		if ( !m_charset.IsEmpty() )
			properties->putValue ("charset", (const char*)m_charset);
		if ( !m_client.IsEmpty() )
			properties->putValue ("client", (const char*)m_client);

		if ( m_database.IsEmpty() )
			m_database = "<empty>";

		connection->openDatabase ( (const char*)m_database, properties );
		properties->release();
		connection->close();
		connection = NULL;
		MessageBox(hDlg, _TR( IDS_MESSAGE_01, "Connection successful!" ), TEXT(strHeadDlg), MB_ICONINFORMATION|MB_OK);
	}
	catch ( std::exception &ex )
	{
		SQLException &exception = (SQLException&)ex;
		char buffer[2048];
		JString text = exception.getText();
		if (properties)
			properties->release();
		if ( connection )
			connection->close();

		sprintf( buffer, "%s\n%s", _TR( IDS_MESSAGE_02, "Connection failed!" ), (const char*)text );
		removeNameFileDBfromMessage ( buffer );

		MessageBox(hDlg, TEXT(buffer), TEXT(strHeadDlg), MB_ICONERROR|MB_OK);
	}

	FreeLibrary ( libraryHandle );
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

	MessageBox( hWnd, stringID, TEXT( _TR( IDS_DLG_TITLE_SETUP, "FireBird ODBC Setup" ) ), MB_OK );
}

int nCopyAnsiToWideChar (LPWORD lpWCStr, LPCSTR lpAnsiIn)
{
  int cchAnsi = lstrlen(lpAnsiIn);
  return MultiByteToWideChar(GetACP(), MB_PRECOMPOSED, lpAnsiIn, cchAnsi,(LPWSTR) lpWCStr, cchAnsi) + 1;
}

LPWORD lpwAlign ( LPWORD lpIn)
{
  ULONG ul;

  ul = (ULONG) lpIn;
  ul +=3;
  ul >>=2;
  ul <<=2;
  return (LPWORD) ul;
}


#define TMP_COMTROL(CONTROL,STRTEXT,CTRL_ID,X,Y,CX,CY,STYLE)	\
	p = lpwAlign (p);											\
																\
	lStyle = STYLE;												\
	*p++ = LOWORD (lStyle);										\
	*p++ = HIWORD (lStyle);										\
	*p++ = 0;			/* LOWORD (lExtendedStyle) */			\
	*p++ = 0;			/* HIWORD (lExtendedStyle) */			\
	*p++ = X;			/* x  */								\
	*p++ = Y;			/* y  */								\
	*p++ = CX;			/* cx */								\
	*p++ = CY;			/* cy */								\
	*p++ = CTRL_ID;		/* ID */								\
																\
	*p++ = (WORD)0xffff;										\
	*p++ = (WORD)CONTROL;										\
																\
	/* copy the text of the item */								\
	nchar = nCopyAnsiToWideChar (p, TEXT(STRTEXT));				\
	p += nchar;													\
																\
	*p++ = 0;  /* advance pointer over nExtraStuff WORD	*/		\


//    PUSHBUTTON      "Browse",IDC_FIND_FILE,189,42,36,14
#define TMP_PUSHBUTTON(STRTEXT,ID,X,Y,CX,CY) \
	TMP_COMTROL(0x0080,STRTEXT,ID,X,Y,CX,CY, (BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | WS_TABSTOP) )

#define TMP_DEFPUSHBUTTON(STRTEXT,ID,X,Y,CX,CY)	\
	TMP_COMTROL(0x0080,STRTEXT,ID,X,Y,CX,CY, (BS_DEFPUSHBUTTON | WS_VISIBLE | WS_CHILD | WS_TABSTOP) )

//    EDITTEXT        IDC_NAME,7,17,102,12,ES_AUTOHSCROLL
#define TMP_EDITTEXT(ID,X,Y,CX,CY,STYLE) \
	TMP_COMTROL(0x0081,"",ID,X,Y,CX,CY, 0x50810080|STYLE )

//    COMBOBOX        IDC_DRIVER,123,17,102,47,CBS_DROPDOWN | WS_VSCROLL | WS_TABSTOP
#define TMP_COMBOBOX(ID,X,Y,CX,CY,STYLE) \
	TMP_COMTROL(0x0085,"",ID,X,Y,CX,CY, (STYLE | WS_VISIBLE | WS_CHILD ) )

//    LTEXT           "Data Source Name (DSN)",IDC_STATIC,7,7,83,8
#define TMP_LTEXT(STRTEXT,ID,X,Y,CX,CY)	\
	TMP_COMTROL(0x0082,STRTEXT,(short)ID,X,Y,CX,CY, ( WS_VISIBLE | WS_CHILD ) )

//    GROUPBOX        "Options",IDC_STATIC,7,111,223,49
#define TMP_GROUPBOX(STRTEXT,ID,X,Y,CX,CY) \
	TMP_COMTROL(0x0080,STRTEXT,(short)ID,X,Y,CX,CY, (BS_GROUPBOX | WS_VISIBLE | WS_CHILD | WS_TABSTOP) )

//    CONTROL         "read (default write)",IDC_CHECK_READ,"Button", BS_AUTOCHECKBOX | WS_TABSTOP,18,129,69,10
#define TMP_BUTTONCONTROL(STRTEXT,ID,NAME_CTRL,STYLE,X,Y,CX,CY) \
	TMP_COMTROL(0x0080,STRTEXT,ID,X,Y,CX,CY, 0x50010003 )

//    CONTROL         "3",IDC_DIALECT3,"Button",BS_AUTORADIOBUTTON,104,154,16,10
#define TMP_RADIOCONTROL(STRTEXT,ID,NAME_CTRL,STYLE,X,Y,CX,CY) \
	TMP_COMTROL(0x0080,STRTEXT,ID,X,Y,CX,CY, 0x50000009 )

//IDD_DSN_PROPERTIES DIALOG DISCARDABLE  0, 0, 237, 186
//STYLE DS_MODALFRAME | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU
//CAPTION "FireBird ODBC Setup"
//FONT 8, "MS Sans Serif"

int DialogBoxDynamic()
{
	HWND hwnd = NULL;
	WORD  *p, *pdlgtemplate;
	int   nchar;
	DWORD lStyle;

	pdlgtemplate = p = (PWORD) LocalAlloc (LPTR, 4096);
	lStyle = DS_SETFONT | DS_MODALFRAME | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE;

	*p++ = LOWORD (lStyle);
	*p++ = HIWORD (lStyle);
	*p++ = 0;          // LOWORD (lExtendedStyle)
	*p++ = 0;          // HIWORD (lExtendedStyle)

	*p++ = 34;         // NumberOfItems

	*p++ = 0;          // x
	*p++ = 0;          // y
	*p++ = 310;        // cx
	*p++ = 221;        // cy
	*p++ = 0;          // Menu
	*p++ = 0;          // Class

	/* copy the title of the dialog */
	nchar = nCopyAnsiToWideChar (p, TEXT( _TR( IDS_DLG_TITLE_SETUP, "FireBird ODBC Setup" ) ) );
	p += nchar;

	*p++ = 8;          // FontSize
	nchar = nCopyAnsiToWideChar (p, TEXT("MS Sans Serif"));
	p += nchar;

    TMP_EDITTEXT      ( IDC_NAME,7,12,184,12,ES_AUTOHSCROLL )
    TMP_COMBOBOX      ( IDC_DRIVER,196,12,107,47,CBS_DROPDOWN | WS_VSCROLL | WS_TABSTOP )
    TMP_EDITTEXT      ( IDC_DATABASE,7,37,231,12,ES_AUTOHSCROLL )
    TMP_PUSHBUTTON    ( _TR( IDS_BUTTON_FIND_FILE, "Browse" ), IDC_FIND_FILE,243,36,60,14 )
    TMP_EDITTEXT      ( IDC_CLIENT,7,62,231,12,ES_AUTOHSCROLL )
    TMP_PUSHBUTTON    ( _TR( IDS_BUTTON_FIND_FILE, "Browse" ), IDC_FIND_FILE_CLIENT,243,61,60,14 )
    TMP_EDITTEXT      ( IDC_USER,7,87,107,12,ES_UPPERCASE | ES_AUTOHSCROLL )
    TMP_EDITTEXT      ( IDC_PASSWORD,118,87,74,12,ES_PASSWORD | ES_AUTOHSCROLL )
    TMP_EDITTEXT      ( IDC_ROLE,196,87,107,12,ES_AUTOHSCROLL )
    TMP_COMBOBOX      ( IDC_CHARSET,7,112,100,120,CBS_DROPDOWN | WS_VSCROLL | WS_TABSTOP )
    TMP_BUTTONCONTROL ( _TR( IDS_CHECK_READ, "read (default write)" ), IDC_CHECK_READ,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,14,144,136,10 )
    TMP_BUTTONCONTROL ( _TR( IDS_CHECK_NOWAIT, "nowait (default wait)" ), IDC_CHECK_NOWAIT,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,14,154,136,10 )
    TMP_DEFPUSHBUTTON ( _TR( IDS_BUTTON_OK, "OK" ), IDOK,86,200,60,14 )
    TMP_PUSHBUTTON    ( _TR( IDS_BUTTON_CANCEL, "Cancel" ), IDCANCEL,154,200,60,14 )
    TMP_LTEXT         ( _TR( IDS_STATIC_DSN, "Data Source Name (DSN)" ), IDC_STATIC,7,2,167,8 )
    TMP_LTEXT         ( _TR( IDS_STATIC_DATABASE, "Database" ), IDC_STATIC,7,27,218,8 )
    TMP_LTEXT         ( _TR( IDS_STATIC_ACCOUNT, "Database Account" ), IDC_STATIC,7,77,107,8 )
    TMP_LTEXT         ( _TR( IDS_STATIC_PASSWORD, "Password" ), IDC_STATIC,119,77,72,8 )
    TMP_LTEXT         ( _TR( IDS_STATIC_DRIVER, "Driver" ), IDC_STATIC,197,2,103,8 )
    TMP_LTEXT         ( _TR( IDS_STATIC_ROLE, "Role" ), IDC_STATIC,197,77,105,8 )
    TMP_LTEXT         ( _TR( IDS_STATIC_CHARSET, "Character Set" ), IDC_STATIC,7,102,98,8 )
    TMP_GROUPBOX      ( _TR( IDS_GROUPBOX_OPTIONS, "Options" ), IDC_STATIC,7,127,296,68 )
    TMP_GROUPBOX      ( _TR( IDS_GROUPBOX_INIT_TRANSACTION, "Transaction" ), IDC_STATIC,10,135,142,33 )
    TMP_LTEXT         ( _TR( IDS_STATIC_CLIENT, "Client" ), IDC_STATIC,7,52,218,8 )
    TMP_GROUPBOX      ( _TR( IDS_GROUPBOX_DIALECT, "Dialect" ), IDC_STATIC,10,169,142,23 )
    TMP_RADIOCONTROL  ( "3",IDC_DIALECT3,"Button",BS_AUTORADIOBUTTON,61,179,16,10 )
    TMP_RADIOCONTROL  ( "1",IDC_DIALECT1,"Button",BS_AUTORADIOBUTTON,91,179,16,10 )
    TMP_GROUPBOX      ( _TR( IDS_GROUPBOX_EXT_PROPERTY, "Extended identifier properties" ), IDC_STATIC,154,135,146,57 )
    TMP_BUTTONCONTROL ( _TR( IDS_CHECK_QUOTED, "quoted identifiers" ), IDC_CHECK_QUOTED,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,159,145,139,9 )
    TMP_BUTTONCONTROL ( _TR( IDS_CHECK_SENSITIVE, "sensitive identifier" ), IDC_CHECK_SENSITIVE,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,159,155,139,9 )
    TMP_BUTTONCONTROL ( _TR( IDS_CHECK_AUTOQUOTED, "autoquoted identifier" ), IDC_CHECK_AUTOQUOTED,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,159,165,139,9 )
    TMP_COMBOBOX      ( IDC_COMBOBOX_USE_SCHEMA,159,176,136,60,CBS_DROPDOWN | WS_VSCROLL | WS_TABSTOP )
    TMP_PUSHBUTTON    ( _TR( IDS_BUTTON_TEST_CONNECTION, "Test connection" ), IDC_TEST_CONNECTION,206,106,97,18 )
    TMP_PUSHBUTTON    ( _TR( IDS_BUTTON_HELP_ODBC, "Help" ), IDC_HELP_ODBC,243,200,60,14 )

	int nRet = DialogBoxIndirect(m_hInstance, (LPDLGTEMPLATE) pdlgtemplate, hwnd, (DLGPROC)wndprocDsnDialog);
	LocalFree (LocalHandle (pdlgtemplate));

	return nRet;
}

}; // end namespace OdbcJdbcSetupLibrary
