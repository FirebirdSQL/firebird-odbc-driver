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

// Attachment.h: interface for the Attachment class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ATTACHMENT_H__F3F1D3A9_4083_11D4_98E8_0000C01D2301__INCLUDED_)
#define AFX_ATTACHMENT_H__F3F1D3A9_4083_11D4_98E8_0000C01D2301__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Mutex.h"

namespace IscDbcLibrary 
{

using namespace classMutex;

class Properties;

class Attachment  
{
public:
	bool isAdmin();
	void checkAdmin();
	JString& getUserAccess();
	int getUserType();
	JString existsAccess(const char *prefix, const char * relobject, int typeobject, const char *suffix);
	int getDatabaseDialect();
	JString getIscStatusText (ISC_STATUS *statusVector);
	int release();
	void addRef();
	void openDatabase(const char * dbName, Properties * properties);
	Attachment();
	~Attachment();

	CFbDll		*GDS;
	void		*databaseHandle;
	void		*transactionHandle;
	JString		databaseName;
	JString		userName;
	JString		userAccess;
	int			userType;
	JString		serverVersion;
	int			pageSize;
	int			serverBaseLevel;	
	int			databaseDialect;
	bool		quotedIdentifiers;
	int			transactionIsolation;
	int			useCount;
	bool		admin;
	bool		isRoles;
	Mutex		mutex;
};

}; // end namespace IscDbcLibrary

#endif // !defined(AFX_ATTACHMENT_H__F3F1D3A9_4083_11D4_98E8_0000C01D2301__INCLUDED_)
