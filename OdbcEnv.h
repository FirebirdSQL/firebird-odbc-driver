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

// OdbcEnv.h: interface for the OdbcEnv class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ODBCENV_H__ED260D95_1BC4_11D4_98DF_0000C01D2301__INCLUDED_)
#define AFX_ODBCENV_H__ED260D95_1BC4_11D4_98DF_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "OdbcObject.h"

class OdbcConnection;

class OdbcEnv : public OdbcObject  
{
public:
	RETCODE sqlSetEnvAttr (int attribute, SQLPOINTER value, int length);
	void connectionClosed (OdbcConnection *connection);
	RETCODE sqlEndTran(int operation);
	virtual RETCODE allocHandle (int handleType, SQLHANDLE *outputHandle);
	virtual OdbcObjectType getType();
	OdbcEnv();
	virtual ~OdbcEnv();

	OdbcConnection	*connections;
	const char		*odbcIniFileName;
};

#endif // !defined(AFX_ODBCENV_H__ED260D95_1BC4_11D4_98DF_0000C01D2301__INCLUDED_)
