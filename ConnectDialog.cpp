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
 *  Copyright (c) 2003 Vladimir Tsvigun
 *  All Rights Reserved.
 */
//
// ConnectDialog.cpp : implementation file
//
#ifdef _WINDOWS

#include <windows.h>
#include "OdbcEnv.h"
#include "OdbcConnection.h"
#include "ConnectDialog.h"

extern HINSTANCE m_hInstance;

namespace OdbcJdbcLibrary {

int currentCP;

#define _TR( id, msg ) ( currentCP == -1 ? msg : translate[currentCP].table[id].string )

struct TranslateString 
{
	int		userLCID;
	struct 
	{
		int		id;
		char	*string;
	} table[6];
};

TranslateString translate[] = 
{
	#include "Res/resource.en"
,	
	#include "Res/resource.ru"
,	
	#include "Res/resource.uk"
,	
	#include "Res/resource.es"
,	
	#include "Res/resource.it"
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

CConnectDialog * m_ptConnectDialog = NULL;

intptr_t DialogBoxDynamicConnect();
BOOL CALLBACK wndprocConnectDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

CConnectDialog::CConnectDialog()
{
	m_user = "";
	m_password = "";
	m_role = "";

	m_ptConnectDialog = this;
}

CConnectDialog::~CConnectDialog()
{
	m_ptConnectDialog = NULL;
}

void CConnectDialog::UpdateData(HWND hDlg, BOOL bSaveAndValidate)
{
	if ( bSaveAndValidate )
	{
		GetDlgItemText(hDlg, IDC_USER, m_user.getBuffer(256), 256);
		GetDlgItemText(hDlg, IDC_PASSWORD, m_password.getBuffer(256), 256);
		GetDlgItemText(hDlg, IDC_ROLE, m_role.getBuffer(256), 256);
	}
	else
	{
		SetDlgItemText(hDlg, IDC_USER, (const char *)m_user);
		SetDlgItemText(hDlg, IDC_PASSWORD, (const char *)m_password);
		SetDlgItemText(hDlg, IDC_ROLE, (const char *)m_role);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CConnectDialog message handlers

int DialogBoxDynamic();

intptr_t CConnectDialog::DoModal()
{
	return DialogBoxDynamicConnect();
}

BOOL CConnectDialog::OnInitDialog(HWND hDlg) 
{
	return TRUE;
}

BOOL CALLBACK wndprocConnectDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
    case WM_INITDIALOG:

		if ( !m_ptConnectDialog->OnInitDialog(hDlg) )
			return FALSE;

		m_ptConnectDialog->UpdateData(hDlg, FALSE);

		return TRUE;

	case WM_COMMAND:
        switch (LOWORD(wParam)) 
		{
        case IDCANCEL:
			EndDialog(hDlg, FALSE);
			return(TRUE);

        case IDOK:
			m_ptConnectDialog->UpdateData(hDlg);
            EndDialog(hDlg, TRUE);
            return TRUE;
        }
        break;
	}
    return FALSE ;
}

int nCopyAnsiToWideChar (LPWORD lpWCStr, LPCSTR lpAnsiIn)
{
  int cchAnsi = lstrlen(lpAnsiIn);
  return MultiByteToWideChar(GetACP(), MB_PRECOMPOSED, lpAnsiIn, cchAnsi,(LPWSTR) lpWCStr, cchAnsi) + 1;
}

LPWORD lpwAlign ( LPWORD lpIn)
{
  uintptr_t ul;

  ul = (uintptr_t) lpIn;
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

//IDD_DSN_PROPERTIES DIALOG DISCARDABLE  0, 0, 237, 186
//STYLE DS_MODALFRAME | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU
//CAPTION "Firebird ODBC Setup"
//FONT 8, "MS Sans Serif"

intptr_t DialogBoxDynamicConnect()
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

	*p++ = 8;          // NumberOfItems

	*p++ = 0;          // x
	*p++ = 0;          // y
	*p++ = 142;        // cx
	*p++ = 84;         // cy
	*p++ = 0;          // Menu
	*p++ = 0;          // Class

	/* copy the title of the dialog */
	nchar = nCopyAnsiToWideChar ( p, TEXT( _TR( IDS_DLG_TITLE_CONNECT, "Firebird ODBC Connect" ) ) );
	p += nchar;

	*p++ = 8;          // FontSize
	nchar = nCopyAnsiToWideChar (p, TEXT("MS Sans Serif"));
	p += nchar;

    TMP_EDITTEXT      ( IDC_USER,60,10,70,12,ES_UPPERCASE | ES_AUTOHSCROLL )
    TMP_EDITTEXT      ( IDC_PASSWORD,60,25,70,12,ES_PASSWORD | ES_AUTOHSCROLL )
    TMP_EDITTEXT      ( IDC_ROLE,60,40,70,12,ES_AUTOHSCROLL )
    TMP_DEFPUSHBUTTON ( _TR( IDS_BUTTON_OK, "OK" ), IDOK,16,64,50,14 )
    TMP_PUSHBUTTON    ( _TR( IDS_BUTTON_CANCEL, "Cancel" ), IDCANCEL,77,64,50,14 )
    TMP_LTEXT         ( _TR( IDS_STATIC_ACCOUNT, "Account" ), IDC_STATIC,5,12,54,8 )
    TMP_LTEXT         ( _TR( IDS_STATIC_PASSWORD, "Password" ), IDC_STATIC,5,26,54,8 )
    TMP_LTEXT         ( _TR( IDS_STATIC_ROLE, "Role" ), IDC_STATIC,5,41,54,8 )

	intptr_t nRet = DialogBoxIndirect(m_hInstance, (LPDLGTEMPLATE) pdlgtemplate, hwnd, (DLGPROC)wndprocConnectDialog);
	LocalFree (LocalHandle (pdlgtemplate));

	return nRet;
}

}; // end namespace OdbcJdbcLibrary

#endif // _WINDOWS
