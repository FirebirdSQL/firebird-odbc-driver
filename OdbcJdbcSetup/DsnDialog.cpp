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

// DsnDialog.cpp : implementation file
//

#include "stdafx.h"
#include "OdbcJdbcSetup.h"
#include "DsnDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDsnDialog dialog


CDsnDialog::CDsnDialog(const char **jdbcDrivers, CWnd* pParent /*=NULL*/)
	: CDialog(CDsnDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDsnDialog)
	m_database = _T("");
	m_name = _T("");
	m_password = _T("");
	m_user = _T("");
	m_driver = _T("");
	m_role = _T("");
	//}}AFX_DATA_INIT

	drivers = jdbcDrivers;
}


void CDsnDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDsnDialog)
	DDX_Text(pDX, IDC_DATABASE, m_database);
	DDX_Text(pDX, IDC_NAME, m_name);
	DDX_Text(pDX, IDC_PASSWORD, m_password);
	DDX_Text(pDX, IDC_USER, m_user);
	DDX_CBString(pDX, IDC_DRIVER, m_driver);
	DDX_Text(pDX, IDC_ROLE, m_role);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDsnDialog, CDialog)
	//{{AFX_MSG_MAP(CDsnDialog)
	ON_BN_CLICKED(IDC_FIND_FILE, OnFindFile)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDsnDialog message handlers

void CDsnDialog::OnFindFile() 
{
	UpdateData (true);
	CFileDialog dialog (true, "gdb", m_database, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
						"Database Files (*.gdb)|*.gdb||");

	if (dialog.DoModal() == IDOK)
		{
		m_database = dialog.GetPathName();
		UpdateData (false);
		}	
}

BOOL CDsnDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CComboBox *box = (CComboBox*) GetDlgItem (IDC_DRIVER);

	for (const char **driver = drivers; *driver; ++driver)
		box->AddString (*driver);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
