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

// OdbcConnection.h: interface for the OdbcConnection class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ODBCCONNECTION_H__ED260D96_1BC4_11D4_98DF_0000C01D2301__INCLUDED_)
#define AFX_ODBCCONNECTION_H__ED260D96_1BC4_11D4_98DF_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "OdbcDesc.h"
#include "IscDbc/JString.h"	// Added by ClassView

class OdbcEnv;
class Connection;
class DatabaseMetaData;
class OdbcStatement;


class OdbcConnection : public OdbcObject  
{
public:
	void transactionStarted();
	RETCODE sqlGetConnectAttr (int attribute, SQLPOINTER ptr, int bufferLength, SQLINTEGER * lengthPtr);
	void descriptorDeleted (OdbcDesc* descriptor);
	OdbcDesc* allocDescriptor(OdbcDescType type);
	void expandConnectParameters();
	void statementDeleted (OdbcStatement *statement);
	RETCODE sqlEndTran (int operation);
	RETCODE connect (const char *sharedLibrary, const char *databaseName, const char *account, const char *password, const char *role, const char *charset);
	RETCODE sqlConnect (const SQLCHAR *dsn, int dsnLength, SQLCHAR*UID,int uidLength,SQLCHAR*password,int passwordLength);
	DatabaseMetaData* getMetaData();
	virtual RETCODE allocHandle (int handleType, SQLHANDLE *outputHandle);
	char* appendString (char *ptr, const char *string);
	RETCODE sqlGetInfo (UWORD type, PTR ptr, int maxLength, SWORD *actualLength);
	RETCODE sqlDisconnect();
	RETCODE sqlGetFunctions (SQLUSMALLINT functionId, SQLUSMALLINT *supportedPtr);
	JString readAttribute (const char *attribute);
	RETCODE sqlDriverConnect (SQLHWND hWnd, 
						   const SQLCHAR *connectString, int connectStringLength, 
						   SQLCHAR *outConnectBuffer, int connectBufferLength, SQLSMALLINT *outStringLength, 
						   int driverCompletion);
	RETCODE sqlBrowseConnect(SQLCHAR * inConnectionString, SQLSMALLINT stringLength1, SQLCHAR * outConnectionString, SQLSMALLINT bufferLength, SQLSMALLINT * stringLength2Ptr);
	RETCODE sqlNativeSql(SQLCHAR * inStatementText, SQLINTEGER textLength1,	SQLCHAR * outStatementText, SQLINTEGER bufferLength, SQLINTEGER * textLength2Ptr);
	RETCODE sqlSetConnectAttr (SQLINTEGER arg1, SQLPOINTER arg2, SQLINTEGER stringLength);
	virtual OdbcObjectType getType();
	OdbcConnection(OdbcEnv *parent);
	virtual ~OdbcConnection();
	void Lock();
	void UnLock();

	OdbcEnv		*env;
	Connection	*connection;
	OdbcStatement*	statements;
	OdbcDesc*	descriptors;
	bool		connected;
	int			connectionTimeout;
	JString		dsn;
	JString		databaseName;
	JString		client;
	JString		account;
	JString		password;
	JString		role;
	JString		charset;
	JString		jdbcDriver;
	int			optTpb;
	bool		quotedIdentifiers;
	bool		dialect3;
	SQLUINTEGER	asyncEnabled;
	bool		autoCommit;
	int			accessMode;
	int			transactionIsolation;
	int			cursors;			// default is SQL_CUR_USE_DRIVER
	int			statementNumber;
	int			levelBrowseConnect;
};

class SafeConnectThread
{
	OdbcConnection * connection;
public:
	SafeConnectThread(OdbcConnection * connect)
	{
		if(connect->connected)
		{
			connection=connect;
			connection->Lock();
		}
		else
			connection = NULL;
	}
	virtual ~SafeConnectThread()
	{
		if(connection && connection->connected)
			connection->UnLock();
	}
};


#endif // !defined(AFX_ODBCCONNECTION_H__ED260D96_1BC4_11D4_98DF_0000C01D2301__INCLUDED_)
