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

// Setup.h: interface for the Setup class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SETUP_H__ED260D92_1BC4_11D4_98DF_0000C01D2301__INCLUDED_)
#define AFX_SETUP_H__ED260D92_1BC4_11D4_98DF_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class Setup  
{
public:
	CString readAttribute (const char *attribute);
	void readAttributes();
	bool configureDialog();
	void writeAttributes();
	void writeAttribute (const char *attribute, const char *value);
	CString getAttribute (const char *attribute);
	void removeDsn();
	void addDsn();
	void configDsn();
	Setup (HWND windowHandle, const char *drvr, const char *attr);
	virtual ~Setup();

	HWND		hWnd;
	CString		driver;
	CString		attributes;
	CString		dsn;
	CString		dbName;
	CString		user;
	CString		password;
	CString		jdbcDriver;
	CString		role;
	CString		readonlyTpb;
	CString		nowaitTpb;
};

#endif // !defined(AFX_SETUP_H__ED260D92_1BC4_11D4_98DF_0000C01D2301__INCLUDED_)
