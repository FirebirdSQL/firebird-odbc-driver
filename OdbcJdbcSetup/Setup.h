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

#if !defined(_SETUP_H_INCLUDED_)
#define _SETUP_H_INCLUDED_

class Setup  
{
public:
	JString readAttribute (const char *attribute);
	void readAttributes();
	bool configureDialog();
	void writeAttributes();
	void writeAttribute (const char *attribute, const char *value);
	JString getAttribute (const char *attribute);
	void removeDsn();
	void addDsn();
	void configDsn();
	Setup (HWND windowHandle, const char *drvr, const char *attr);
	virtual ~Setup();

	HWND		hWnd;
	JString		driver;
	JString		attributes;
	JString		dsn;
	JString		dbName;
	JString		client;
	JString		user;
	JString		password;
	JString		jdbcDriver;
	JString		role;
	JString		charset;
	JString		readonlyTpb;
	JString		nowaitTpb;
	JString		dialect;
	JString		quoted;
};

#endif // !defined(_SETUP_H_INCLUDED_)
