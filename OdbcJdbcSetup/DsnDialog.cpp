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
#include <windows.h>
#include "OdbcJdbcSetup.h"
#include "DsnDialog.h"

extern HINSTANCE m_hInstance;
CDsnDialog * m_ptDsnDialog = NULL;

BOOL CALLBACK wndprocDsnDialog(HWND hDlg, UINT message, WORD wParam, LONG lParam);

CDsnDialog::CDsnDialog(const char **jdbcDrivers)
{
	m_database = "";
	m_name = "";
	m_password = "";
	m_user = "";
	m_driver = "";
	m_role = "";
	m_readonly = FALSE;
	m_nowait = FALSE;

	drivers = jdbcDrivers;
	m_ptDsnDialog = this;
}

CDsnDialog::~CDsnDialog()
{
	m_ptDsnDialog = NULL;
}

void CDsnDialog::UpdateData(HWND hDlg, BOOL bSaveAndValidate)
{
	HWND hWnd;

	if ( bSaveAndValidate )
	{
		GetDlgItemText(hDlg, IDC_DATABASE, m_database.getBuffer(256), 256);
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
        m_readonly = SendDlgItemMessage(hDlg, IDC_CHECK_READ, BM_GETCHECK, 0, 0);
        m_nowait = SendDlgItemMessage(hDlg, IDC_CHECK_NOWAIT, BM_GETCHECK, 0, 0);
	}
	else
	{
		SetDlgItemText(hDlg, IDC_DATABASE, (const char *)m_database);
		SetDlgItemText(hDlg, IDC_NAME, (const char *)m_name);
		SetDlgItemText(hDlg, IDC_PASSWORD, (const char *)m_password);
		SetDlgItemText(hDlg, IDC_USER, (const char *)m_user);

		hWnd = GetDlgItem(hDlg, IDC_DRIVER);

		if (SendMessage(hWnd, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)(const char *)m_driver) == CB_ERR)
		{
			SetWindowText(hWnd, (const char *)m_driver);
		}

		SetDlgItemText(hDlg, IDC_ROLE, (const char *)m_role);
        CheckDlgButton(hDlg, IDC_CHECK_READ, m_readonly);
        CheckDlgButton(hDlg, IDC_CHECK_NOWAIT, m_nowait);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDsnDialog message handlers

BOOL CDsnDialog::OnFindFile()
{
    OPENFILENAME ofn;
    char strFullPathFileName[256];
    char achPathFileName[256];
	char * szOpenFilter =	"Firebird Database Files (*.fdb;*.gdb)\0*.fdb;*.gdb\0"
							"All files (*.*)\0*.*\0"
							"\0";

	strcpy(strFullPathFileName,(const char*)m_database);
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
        return FALSE;

	m_database = strFullPathFileName;

	return TRUE;
}

int CDsnDialog::DoModal()
{
	return DialogBox(m_hInstance, MAKEINTRESOURCE(IDD), NULL, (DLGPROC)wndprocDsnDialog);
}

BOOL CDsnDialog::OnInitDialog(HWND hDlg) 
{
	HWND hWndBox = GetDlgItem(hDlg, IDC_DRIVER);

	for (const char **driver = drivers; *driver; ++driver)
		SendMessage(hWndBox, CB_ADDSTRING, 0, (LPARAM)*driver);

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

        case IDOK:
			m_ptDsnDialog->UpdateData(hDlg);
            EndDialog(hDlg, TRUE);
            return TRUE;
        }
        break;
	}
    return FALSE ;
}
