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
#include "IscDbc/Mutex.h"

class OdbcConnection;

class OdbcEnv : public OdbcObject  
{
public:
	RETCODE sqlGetEnvAttr(int attribute, SQLPOINTER ptr, int bufferLength, SQLINTEGER *lengthPtr);
	RETCODE sqlSetEnvAttr (int attribute, SQLPOINTER value, int length);
	void connectionClosed (OdbcConnection *connection);
	RETCODE sqlEndTran(int operation);
	RETCODE sqlDrivers( SQLUSMALLINT direction,	SQLCHAR * serverName, SQLSMALLINT	bufferLength1, SQLSMALLINT * nameLength1Ptr, SQLCHAR * description, SQLSMALLINT bufferLength2, SQLSMALLINT * nameLength2Ptr);
	RETCODE sqlDataSources( SQLUSMALLINT direction,	SQLCHAR * serverName, SQLSMALLINT	bufferLength1, SQLSMALLINT * nameLength1Ptr, SQLCHAR * description, SQLSMALLINT bufferLength2, SQLSMALLINT * nameLength2Ptr);
#ifdef _WIN32
	BOOL getDrivers();
	BOOL getDataSources(UWORD wConfigMode);
#endif
	virtual RETCODE allocHandle (int handleType, SQLHANDLE *outputHandle);
	void LockEnv();
	void UnLockEnv();
	virtual OdbcObjectType getType();
	OdbcEnv();
	~OdbcEnv();

#ifdef _WIN32
	HINSTANCE		libraryHandle;
#else
	void			*libraryHandle;
#endif

	Mutex			mutex;
	OdbcConnection	*connections;
	const char		*odbcIniFileName;
	const char		*odbcInctFileName;

#ifdef _WIN32
	char			listDSN[1024];
	char			*activeDSN;
	char			*endDSN;

	char			listDrv[2048];
	char			*activeDrv;
	char			*endDrv;
#endif
};

class SafeEnvThread
{
	OdbcEnv * env;
public:
	SafeEnvThread(OdbcEnv * ptEnv)
	{
		if( ptEnv )
		{
			env = ptEnv;
			env->LockEnv();
		}
		else 
			env = NULL;
	}
	~SafeEnvThread()
	{
		if(env)
			env->UnLockEnv();
	}
};

#endif // !defined(AFX_ODBCENV_H__ED260D95_1BC4_11D4_98DF_0000C01D2301__INCLUDED_)
