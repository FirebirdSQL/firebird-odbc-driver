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

#if !defined(AFX_DSNDIALOG_H__B70DB912_1BB7_11D4_98DE_0000C01D2301__INCLUDED_)
#define AFX_DSNDIALOG_H__B70DB912_1BB7_11D4_98DE_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DsnDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDsnDialog dialog

class CDsnDialog : public CDialog
{
// Construction
public:
	const char** drivers;
	CDsnDialog(const char **drivers, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDsnDialog)
	enum { IDD = IDD_DSN_PROPERTIES };
	CString	m_database;
	CString	m_name;
	CString	m_password;
	CString	m_user;
	CString	m_driver;
	CString	m_role;
	BOOL	m_readonly;
	BOOL	m_nowait;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDsnDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDsnDialog)
	afx_msg void OnFindFile();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DSNDIALOG_H__B70DB912_1BB7_11D4_98DE_0000C01D2301__INCLUDED_)
