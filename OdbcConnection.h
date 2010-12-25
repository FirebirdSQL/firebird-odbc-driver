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

// OdbcConnection.h: interface for the OdbcConnection class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ODBCCONNECTION_H_)
#define _ODBCCONNECTION_H_

#if defined(__GNUC__)
#include <inttypes.h>
#endif

#include "OdbcDesc.h"
#include "IscDbc/JString.h"
#include "Headers/OdbcUserEvents.h"

namespace OdbcJdbcLibrary {

class OdbcEnv;
class OdbcStatement;

class OdbcConnection : public OdbcObject  
{
	enum 
	{	
		DEF_READONLY_TPB = 1,
		DEF_NOWAIT_TPB = 2,
		DEF_DIALECT = 4,
		DEF_QUOTED = 8,
		DEF_SENSITIVE = 16,
		DEF_AUTOQUOTED = 32,
		DEF_SAFETHREAD = 64
	};

public:
	SQLRETURN sqlGetConnectAttr (int attribute, SQLPOINTER ptr, int bufferLength, SQLINTEGER * lengthPtr);
	void descriptorDeleted (OdbcDesc* descriptor);
	OdbcDesc* allocDescriptor(OdbcDescType type);
	void expandConnectParameters();
	void saveConnectParameters();
	void statementDeleted (OdbcStatement *statement);
	SQLRETURN sqlEndTran (int operation);
	SQLRETURN sqlExecuteCreateDatabase(const char * sqlString);
	SQLRETURN connect (const char *sharedLibrary, const char *databaseName, const char *account, const char *password, const char *role, const char *charset);
	SQLRETURN sqlConnect (const SQLCHAR *dsn, int dsnLength, SQLCHAR*UID,int uidLength,SQLCHAR*password,int passwordLength);
	DatabaseMetaData* getMetaData();
	virtual SQLRETURN allocHandle (int handleType, SQLHANDLE *outputHandle);
	char* appendString (char *ptr, const char *string);
	SQLRETURN sqlGetInfo( SQLUSMALLINT type, SQLPOINTER ptr, SQLSMALLINT maxLength, SQLSMALLINT * actualLength );
	SQLRETURN sqlDisconnect();
	SQLRETURN sqlGetFunctions (SQLUSMALLINT functionId, SQLUSMALLINT *supportedPtr);
	JString readAttribute (const char *attribute);
	JString readAttributeFileDSN (const char * attribute);
	void writeAttributeFileDSN (const char * attribute, const char * value);
	SQLRETURN sqlDriverConnect (SQLHWND hWnd, 
						   const SQLCHAR *connectString, int connectStringLength, 
						   SQLCHAR *outConnectBuffer, int connectBufferLength, SQLSMALLINT *outStringLength, 
						   int driverCompletion);
	SQLRETURN sqlBrowseConnect(SQLCHAR * inConnectionString, SQLSMALLINT stringLength1, SQLCHAR * outConnectionString, SQLSMALLINT bufferLength, SQLSMALLINT * stringLength2Ptr);
	SQLRETURN sqlNativeSql(SQLCHAR * inStatementText, SQLINTEGER textLength1,	SQLCHAR * outStatementText, SQLINTEGER bufferLength, SQLINTEGER * textLength2Ptr);
	SQLRETURN sqlSetConnectAttr( SQLINTEGER attribute, SQLPOINTER value, SQLINTEGER stringLength );
	virtual OdbcConnection* getConnection();
	virtual OdbcObjectType getType();

	void initUserEvents( PODBC_EVENTS_BLOCK_INFO infoEvents );
	void updateResultEvents( char *updated );
	void requeueEvents();
	void releaseObjects();

	OdbcConnection(OdbcEnv *parent);
	~OdbcConnection();

	void Lock();
	void UnLock();

#ifdef _WINDOWS
#if _MSC_VER > 1000

public:
	// realised into TransactionResourceAsync.cpp
	bool IsInstalledMsTdsInterface();
	bool enlistTransaction( SQLPOINTER globalTransaction );

#endif // _MSC_VER > 1000
#endif // _WINDOWS

public:

	OdbcEnv		*env;
	Connection	*connection;
	OdbcStatement*	statements;
	OdbcDesc*	descriptors;
	UserEvents	*userEvents;
	bool		connected;
	bool		safeThread;
	int			connectionTimeout;
	JString		dsn;
	JString		description;
	JString		filedsn;
	JString		savedsn;
	JString		databaseName;
	JString		databaseServerName;
	int			databaseAccess;
	JString		client;
	JString		account;
	JString		password;
	JString		role;
	JString		charset;
	JString		jdbcDriver;
	JString		pageSize;
	int			optTpb;
	int			defOptions;
	JString		useSchemaIdentifier;
	JString		useLockTimeoutWaitTransactions;
	bool		quotedIdentifier;
	bool		sensitiveIdentifier;
	bool		autoQuotedIdentifier;
	bool		dialect3;
	SQLUINTEGER	asyncEnabled;
	bool		autoCommit;
	int			accessMode;
	int			transactionIsolation;
	int			cursors;			// default is SQL_CUR_USE_DRIVER
	int			statementNumber;
	int			levelBrowseConnect;
	int			charsetCode;
	WCSTOMBS	WcsToMbs;
	MBSTOWCS	MbsToWcs;

	PODBC_USER_EVENTS_INTERFASE userEventsInterfase;

#ifdef _WINDOWS
#if _MSC_VER > 1000

	bool		enlistConnect;

#endif // _MSC_VER > 1000
#endif

};

class SafeConnectThread
{
	OdbcConnection * connection;
public:
	SafeConnectThread(OdbcConnection * connect)
	{
		if(connect && connect->safeThread && connect->connected)
		{
			connection=connect;
			connection->Lock();
		}
		else
			connection = NULL;
	}
	~SafeConnectThread()
	{
		if(connection && connection->connected)
			connection->UnLock();
	}
};

}; // end namespace OdbcJdbcLibrary

#endif // !defined(_ODBCCONNECTION_H_)
