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

// IscConnection.h: interface for the IscConnection class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISCCONNECTION_H__C19738B7_1C87_11D4_98DF_0000C01D2301__INCLUDED_)
#define AFX_ISCCONNECTION_H__C19738B7_1C87_11D4_98DF_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Connection.h"
#include "LinkedList.h"
#include "JString.h"	// Added by ClassView

class IscStatement;
class IscDatabaseMetaData;
class Attachment;

class IscConnection : public Connection  
{
public:
	void commitRetaining();
	int getDatabaseDialect();
	void rollbackAuto();
	void commitAuto();
	virtual CallableStatement* prepareCall (const char *sql);
	virtual int release();
	virtual void addRef();
	virtual int getTransactionIsolation();
	virtual void setTransactionIsolation (int level);
	virtual void setExtInitTransaction (int optTpb);
	virtual bool getAutoCommit();
	virtual void setAutoCommit (bool setting);
	void init();
	IscConnection (IscConnection *source);
	virtual Connection* clone();
	virtual int objectVersion();
	virtual Properties* allocProperties();
	JString getInfoString (char *buffer, int item, const char *defaultString);
	int getInfoItem (char *buffer, int item, int defaultValue);
	static JString getIscStatusText (ISC_STATUS *statusVector);
	void* startTransaction();
	void deleteStatement (IscStatement *statement);
	IscConnection();
	virtual ~IscConnection();
	virtual void openDatabase (const char *dbName, Properties *properties);
	virtual void createDatabase (const char *host, const char * dbName, Properties *properties);
	virtual void ping();
	virtual int hasRole (const char *schemaName, const char *roleName);
	//virtual void freeHTML (const char *html);
	virtual Blob* genHTML (Properties *context, long genHeaders);
	virtual bool isConnected();
	virtual Statement* createStatement();
	virtual void prepareTransaction();
	virtual bool getTransactionPending();
	virtual void rollback();
	virtual void commit();
	virtual PreparedStatement* prepareStatement (const char *sqlString);
	virtual void close();
	virtual DatabaseMetaData* getMetaData();

	Attachment		*attachment;
	void			*databaseHandle;
	void			*transactionHandle;
	LinkedList		statements;
	IscDatabaseMetaData	*metaData;
	int				transactionIsolation;
	int				transactionExtInit;
	bool			autoCommit;
	bool			transactionPending;
	int				useCount;
};

#endif // !defined(AFX_ISCCONNECTION_H__C19738B7_1C87_11D4_98DF_0000C01D2301__INCLUDED_)
