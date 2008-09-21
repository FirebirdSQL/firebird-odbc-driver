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

#if !defined(_DSNDIALOG_H_INCLUDED_)
#define _DSNDIALOG_H_INCLUDED_

// DsnDialog.h : header file
//

namespace OdbcJdbcSetupLibrary {

using namespace classJString;

struct TranslateString 
{
	int		userLCID;
	struct 
	{
		int		id;
		char	*string;
	} table[64];
};

#define _TR( id, msg ) ( currentCP == -1 ? msg : translate[currentCP].table[id].string )

/////////////////////////////////////////////////////////////////////////////
// CDsnDialog dialog
class CServiceTabCtrl;

class CDsnDialog
{
	HWND hwndHtmlHelp;

	const char** drivers;
	const char** charsets;
	const char** useshemas;

public:
	CDsnDialog( HWND hDlgParent, const char **jdbcDrivers, const char **jdbcCharsets,
				const char **useShemasIdentifier );
	~CDsnDialog();

// Dialog Data
	enum { IDD = IDD_DSN_PROPERTIES };
	HWND    m_hWndParent;
	HWND    m_hWndDlg;
	JString	m_database;
	JString	m_client;
	JString	m_name;
	JString	m_description;
	JString	m_password;
	JString	m_user;
	JString	m_driver;
	JString	m_role;
	JString	m_charset;
	JString	m_useschema;
	JString	m_locktimeout;
	BOOL	m_readonly;
	BOOL	m_nowait;
	BOOL	m_dialect3;
	BOOL	m_quoted;
	BOOL	m_sensitive;
	BOOL	m_autoQuoted;
	BOOL	m_safeThread;

public:
	intptr_t DoModal();

	BOOL IsLocalhost( char * fullPathFileName, int &nSme );
	void CheckRemotehost( char * fullPathFileName );
	BOOL OnFindFile();
	BOOL OnFindFileClient();
	void SetDisabledDlgItem( HWND hDlg, int ID, BOOL bDisabled = TRUE );
	void UpdateData( HWND hDlg, BOOL bSaveAndValidate = TRUE );
	BOOL OnInitDialog( HWND hDlg );
#ifdef _WINDOWS
	void OnTestConnection( HWND hDlg );
	void WinHtmlHelp( HWND hDlg );
#endif
	void removeNameFileDBfromMessage(char * message);
};

}; // end namespace OdbcJdbcSetupLibrary

#endif // !defined(_DSNDIALOG_H_INCLUDED_)
