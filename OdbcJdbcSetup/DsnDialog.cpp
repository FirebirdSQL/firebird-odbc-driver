/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
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
#include "OdbcJdbcSetup.h"
#include "../IscDbc/Connection.h"
#include "DsnDialog.h"
#include "../SetupAttributes.h"

extern HINSTANCE m_hInstance;

namespace OdbcJdbcSetupLibrary {

using namespace classJString;
using namespace IscDbcLibrary;

CDsnDialog * m_ptDsnDialog = NULL;

BOOL CALLBACK wndprocDsnDialog(HWND hDlg, UINT message, WORD wParam, LONG lParam);
void ProcessCDError(DWORD dwErrorCode, HWND hWnd);

CDsnDialog::CDsnDialog(const char **jdbcDrivers, const char **jdbcCharsets)
{
	m_database = "";
	m_client = "";
	m_name = "";
	m_password = "";
	m_user = "";
	m_driver = "";
	m_role = "";
	m_charset = "";
	m_readonly = FALSE;
	m_nowait = FALSE;
	m_dialect3 = TRUE;
	m_quoted = TRUE;

	drivers = jdbcDrivers;
	charsets = jdbcCharsets;
	m_ptDsnDialog = this;
}

CDsnDialog::~CDsnDialog()
{
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

        m_readonly = SendDlgItemMessage(hDlg, IDC_CHECK_READ, BM_GETCHECK, 0, 0);
        m_nowait = SendDlgItemMessage(hDlg, IDC_CHECK_NOWAIT, BM_GETCHECK, 0, 0);

        m_dialect3 = IsDlgButtonChecked(hDlg, IDC_DIALECT3);

		m_quoted = SendDlgItemMessage(hDlg, IDC_CHECK_QUOTED, BM_GETCHECK, 0, 0);
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

        CheckDlgButton(hDlg, IDC_CHECK_READ, m_readonly);
        CheckDlgButton(hDlg, IDC_CHECK_NOWAIT, m_nowait);

		CheckRadioButton(hDlg, IDC_DIALECT3, IDC_DIALECT1, m_dialect3 ? IDC_DIALECT3 : IDC_DIALECT1);
		if ( m_dialect3 )
			SetDisabledDlgItem(hDlg, IDC_CHECK_QUOTED, FALSE);
		else
			SetDisabledDlgItem(hDlg, IDC_CHECK_QUOTED);

        CheckDlgButton(hDlg, IDC_CHECK_QUOTED, m_quoted);

		SetDisabledDlgItem(hDlg, IDC_HELP_ODBC);
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
    ofn.lpstrTitle = "Select Firebird database";
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
    ofn.lpstrTitle = "Select Firebird client";
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

#ifdef __MINGW32__
int DialogBoxDynamic();
#endif

int CDsnDialog::DoModal()
{
#ifdef __MINGW32__
	return DialogBoxDynamic();
#else
	return DialogBox(m_hInstance, MAKEINTRESOURCE(IDD), NULL, (DLGPROC)wndprocDsnDialog);
#endif
}

BOOL CDsnDialog::OnInitDialog(HWND hDlg) 
{
	HWND hWndBox = GetDlgItem(hDlg, IDC_DRIVER);

	for (const char **driver = drivers; *driver; ++driver)
		SendMessage(hWndBox, CB_ADDSTRING, 0, (LPARAM)*driver);

	hWndBox = GetDlgItem(hDlg, IDC_CHARSET);

	for (const char **charset = charsets; *charset; ++charset)
		SendMessage(hWndBox, CB_ADDSTRING, 0, (LPARAM)*charset);

	return TRUE;
}

BOOL CALLBACK wndprocDsnDialog(HWND hDlg, UINT message, WORD wParam, LONG lParam)
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
			m_ptDsnDialog->SetDisabledDlgItem(hDlg, IDC_CHECK_QUOTED);
			break;

		case IDC_DIALECT3:
			m_ptDsnDialog->SetDisabledDlgItem(hDlg, IDC_CHECK_QUOTED, FALSE);
			break;

        case IDC_HELP_ODBC:
			WinHelp(hDlg, DRIVER_NAME, HELP_CONTEXT, 0x60000);
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
			text.Format ("Unable to connect to data source: library '%s' failed to load", (const char *)m_driver);
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
			text.Format ("Unable to connect to data source %s: can't find entrypoint 'createConnection'");
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
			text.Format (" Unable to load %s Library : can't find ver. %s ", (const char *)m_driver, DRIVER_VERSION);
			MessageBox(hDlg, "Connection failed!", (const char*)text, MB_ICONINFORMATION|MB_OK);
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

		connection->openDatabase ( (const char*)m_database, properties );
		properties->release();
		connection->close();
		connection = NULL;
		MessageBox(hDlg, "Connection successful!", TEXT(strHeadDlg), MB_ICONINFORMATION|MB_OK);
	}
	catch (SQLException& exception)
	{
		char buffer[2048];
		JString text = exception.getText();
		if (properties)
			properties->release();
		if ( connection )
			connection->close();

		strcpy ( buffer, (const char*)text );
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

	MessageBox(hWnd, stringID, TEXT("FireBird ODBC Setup"), MB_OK);
}

// Temporarily!
// After assembly from MinGW LoadString and LoadResurse does not work!!!
// To me these magic switchs for dllwrap.exe are unknown ;-(
// 
#ifdef __MINGW32__
int nCopyAnsiToWideChar (LPWORD lpWCStr, LPSTR lpAnsiIn)
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

	*p++ = 30;         // NumberOfItems

	*p++ = 0;          // x
	*p++ = 0;          // y
	*p++ = 237;        // cx
	*p++ = 202;        // cy
	*p++ = 0;          // Menu
	*p++ = 0;          // Class

	/* copy the title of the dialog */
	nchar = nCopyAnsiToWideChar (p, TEXT("FireBird ODBC Setup"));
	p += nchar;

	*p++ = 8;          // FontSize
	nchar = nCopyAnsiToWideChar (p, TEXT("MS Sans Serif"));
	p += nchar;

    TMP_EDITTEXT      ( IDC_NAME,7,12,112,12,ES_AUTOHSCROLL )
    TMP_COMBOBOX      ( IDC_DRIVER,123,12,107,47,CBS_DROPDOWN | WS_VSCROLL | WS_TABSTOP )
    TMP_EDITTEXT      ( IDC_DATABASE,7,37,181,12,ES_AUTOHSCROLL )
    TMP_PUSHBUTTON    ( "Browse",IDC_FIND_FILE,194,36,36,14 )
    TMP_EDITTEXT      ( IDC_CLIENT,7,62,182,12,ES_AUTOHSCROLL )
    TMP_PUSHBUTTON    ( "Browse",IDC_FIND_FILE_CLIENT,194,61,36,14 )
    TMP_EDITTEXT      ( IDC_USER,7,89,66,12,ES_UPPERCASE | ES_AUTOHSCROLL )
    TMP_EDITTEXT      ( IDC_PASSWORD,77,89,73,12,ES_PASSWORD | ES_AUTOHSCROLL )
    TMP_EDITTEXT      ( IDC_ROLE,154,89,76,12,ES_AUTOHSCROLL )
    TMP_COMBOBOX      ( IDC_CHARSET,53,109,114,120,CBS_DROPDOWN | WS_VSCROLL | WS_TABSTOP )
    TMP_BUTTONCONTROL ( "read (default write)",IDC_CHECK_READ,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,18,148,69,10 )
    TMP_BUTTONCONTROL ( "nowait (default wait)",IDC_CHECK_NOWAIT,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,18,158,72,10 )
    TMP_DEFPUSHBUTTON ( "OK",IDOK,85,183,50,14 )
    TMP_PUSHBUTTON    ( "Cancel",IDCANCEL,143,183,50,14 )
    TMP_LTEXT         ( "Data Source Name (DSN)",IDC_STATIC,7,2,83,8 )
    TMP_LTEXT         ( "Database",IDC_STATIC,7,27,32,8 )
    TMP_LTEXT         ( "Database Account",IDC_STATIC,7,78,60,8 )
    TMP_LTEXT         ( "Password",IDC_STATIC,80,78,32,8 )
    TMP_LTEXT         ( "Driver",IDC_STATIC,123,2,20,8 )
    TMP_LTEXT         ( "Role",IDC_STATIC,155,78,16,8 )
    TMP_LTEXT         ( "Character Set",IDC_STATIC,7,112,44,8 )
    TMP_GROUPBOX      ( "Options",IDC_STATIC,7,130,223,49 )
    TMP_GROUPBOX      ( "Initializing transaction",IDC_STATIC,14,139,89,33 )
    TMP_LTEXT         ( "Client",IDC_STATIC,7,52,32,8 )
    TMP_GROUPBOX      ( "Dialect",IDC_STATIC,106,139,31,33 )
    TMP_RADIOCONTROL  ( "3",IDC_DIALECT3,"Button",BS_AUTORADIOBUTTON,114,148,16,10 )
    TMP_RADIOCONTROL  ( "1",IDC_DIALECT1,"Button",BS_AUTORADIOBUTTON,114,158,16,10 )
    TMP_BUTTONCONTROL ( "quoted identifiers",IDC_CHECK_QUOTED,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,146,148,66,10 )
    TMP_PUSHBUTTON    ( "Test connection",IDC_TEST_CONNECTION,172,108,58,14 )
    TMP_PUSHBUTTON    ( "Help",IDC_HELP_ODBC,27,183,50,14 )

	int nRet = DialogBoxIndirect(m_hInstance, (LPDLGTEMPLATE) pdlgtemplate, hwnd, (DLGPROC)wndprocDsnDialog);
	LocalFree (LocalHandle (pdlgtemplate));

	return nRet;
}
#endif // __MINGW32__

}; // end namespace OdbcJdbcSetupLibrary
