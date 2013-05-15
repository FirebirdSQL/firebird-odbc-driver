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

// IscConnection.h: interface for the IscConnection class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ISCCONNECTION_H_)
#define _ISCCONNECTION_H_

#include "Connection.h"
#include "LinkedList.h"
#include "JString.h"	// Added by ClassView

namespace IscDbcLibrary {

using namespace classJString;

class CNodeParamTransaction;

class InfoTransaction
{
public:
	InfoTransaction();
	~InfoTransaction();

	void setParam( const InfoTransaction &src )
	{ 
		transactionIsolation = src.transactionIsolation;
		transactionExtInit = src.transactionExtInit;
		autoCommit = src.autoCommit;
	}

public:
	isc_tr_handle	transactionHandle;
	int				transactionIsolation;
	int				transactionExtInit;
	bool			autoCommit;
	bool			transactionPending;

	CNodeParamTransaction *nodeParamTransaction;
};

class IscStatement;
class IscDatabaseMetaData;
class Attachment;
class IscUserEvents;

class IscConnection : public Connection  
{
public:
	enum TypeTransaction { TRANSACTION_NONE, TRANSACTION_READ_COMMITTED, TRANSACTION_READ_UNCOMMITTED ,
							TRANSACTION_REPEATABLE_READ, TRANSACTION_SERIALIZABLE };
//{{{ specification jdbc
	virtual void		clearWarnings();
	virtual void		close();
	virtual void		commit();
	virtual Statement*	createStatement();
	virtual bool		getAutoCommit();
	virtual const char*	getCatalog();
	virtual DatabaseMetaData* getMetaData();
	virtual int			getTransactionIsolation();
//	virtual void		getWarnings();
	virtual bool		isClosed();
	virtual bool		isReadOnly();
	virtual const char*	nativeSQL(const char* sqlString);
	virtual CallableStatement* prepareCall (const char *sql);
	virtual PreparedStatement* prepareStatement (const char *sqlString);
	virtual void		rollback();
	virtual void		setAutoCommit (bool setting);
	virtual void		setCatalog(const char* catalog);
	virtual void		setReadOnly(bool readOnly);
	virtual void		setTransactionIsolation (int level);
//}}} specification jdbc

public:
	void commitRetaining();
	void rollbackRetaining();
	int getDatabaseDialect();
	void rollbackAuto();
	virtual void commitAuto();
	virtual int release();
	virtual void addRef();
	virtual void setExtInitTransaction (int optTpb);
	EnvironmentShare* getEnvironmentShare();
	virtual void connectionToEnvShare();
	virtual void connectionFromEnvShare();
	int	getUseAppOdbcVersion () { return useAppOdbcVersion; }
	void setUseAppOdbcVersion ( int appOdbcVersion ) { useAppOdbcVersion = appOdbcVersion; }
	JString getDatabaseServerName();
	virtual int	getDriverBuildKey();
	void init();
	IscConnection (IscConnection *source);
	virtual Connection* clone();
	virtual int objectVersion();
	virtual Properties* allocProperties();
	JString getInfoString (char *buffer, int item, const char *defaultString);
	int getInfoItem (char *buffer, int item, int defaultValue);
	JString getIscStatusText (ISC_STATUS *statusVector);
	bool removeSchemaFromSQL( char *strSql, int lenSql, char *strSqlOut, int &lenSqlOut );
	virtual int getNativeSql (const char * inStatementText, int textLength1,
								char * outStatementText, int bufferLength,
								int * textLength2Ptr);
	isc_tr_handle startTransaction();
	void deleteStatement (IscStatement *statement);
	IscConnection();
	~IscConnection();
	virtual void openDatabase (const char *dbName, Properties *properties);
	virtual void createDatabase (const char * dbName, Properties *properties);
	virtual void sqlExecuteCreateDatabase(const char * sqlString);
	virtual void ping();
	virtual int getConnectionCharsetCode();
	virtual WCSTOMBS getConnectionWcsToMbs();
	virtual MBSTOWCS getConnectionMbsToWcs();
	virtual int hasRole (const char *schemaName, const char *roleName);
	virtual PropertiesEvents* allocPropertiesEvents();
	virtual UserEvents* prepareUserEvents( PropertiesEvents *context, callbackEvent astRoutine, void *userAppData = 0 );
	//virtual void freeHTML (const char *html);
	virtual Blob* genHTML (Properties *context, int genHeaders);
	virtual bool isConnected();
	InternalStatement* createInternalStatement();
	bool getCountInputParamFromProcedure ( const char* procedureName, int &numIn, int &numOut, bool &canSelect );
	int buildParamProcedure ( char *& string, int numInputParam );
	bool paramTransactionModes( char *& string, short &transFlags, bool expectIsolation );
	void parseReservingTable( char *& string, char *& tpbBuffer, short transFlags );
	int buildParamTransaction( char *& string, char boolDeclare = false );
	inline bool isMatchExt( char *& string, const char *keyWord, const int length );
	virtual void prepareTransaction();
	virtual bool getTransactionPending();
	isc_db_handle getHandleDb();

public:
	CNodeParamTransaction *tmpParamTransaction;
	Attachment		*attachment;
	CFbDll			*GDS;
	isc_db_handle   databaseHandle;
	InfoTransaction	transactionInfo;
	LinkedList		statements;
	IscDatabaseMetaData	*metaData;
	IscUserEvents	*userEvents;
	bool			shareConnected;
	int				useAppOdbcVersion;
	int				useCount;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_ISCCONNECTION_H_)
