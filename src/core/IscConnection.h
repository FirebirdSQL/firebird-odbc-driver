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

#include <vector>
#include <memory>
#include <string>
#include "Connection.h"
#include "Mutex.h"
#include <fb-cpp/Attachment.h>
#include <fb-cpp/Transaction.h>

namespace IscDbcLibrary {

using namespace classMutex;

class CNodeParamTransaction;

class IscStatement;
class IscDatabaseMetaData;
class IscUserEvents;

class IscConnection final : public Connection  
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
	// Savepoint support
	virtual void setSavepoint(const char* name);
	virtual void releaseSavepoint(const char* name);
	virtual void rollbackSavepoint(const char* name);

	// Server version detection
	virtual int getServerMajorVersion();
	virtual int getServerMinorVersion();	EnvironmentShare* getEnvironmentShare();
	virtual void connectionToEnvShare();
	virtual void connectionFromEnvShare();
	int	getUseAppOdbcVersion () { return useAppOdbcVersion; }
	void setUseAppOdbcVersion ( int appOdbcVersion ) { useAppOdbcVersion = appOdbcVersion; }
	const char* getDatabaseServerName();
	virtual int	getDriverBuildKey();
	void init();
	IscConnection (IscConnection *source);
	virtual Connection* clone();
	virtual int objectVersion();
	virtual Properties* allocProperties();
	std::string getInfoString (char *buffer, int item, const char *defaultString);
	int getInfoItem (char *buffer, int item, int defaultValue);
	std::string getIscStatusText (Firebird::IStatus *status);
	bool removeSchemaFromSQL( char *strSql, int lenSql, char *strSqlOut, int &lenSqlOut );
	virtual int getNativeSql (const char * inStatementText, int textLength1,
								char * outStatementText, int bufferLength,
								int * textLength2Ptr);
	Firebird::ITransaction* startTransaction();
	void deleteStatement (IscStatement *statement);
	IscConnection();
	~IscConnection();
	virtual void openDatabase (const char *dbName, Properties *properties);
	virtual void createDatabase (const char * dbName, Properties *properties);
	virtual void sqlExecuteCreateDatabase(const char * sqlString);
	virtual bool ping();
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
	Firebird::IAttachment* getHandleDb();
	virtual bool isMsAccess() override { return GDS ? GDS->isMsAccess() : false; }
	virtual void cancelOperation() override;

	// Phase 14.2: Accessors for connection metadata (moved from Attachment)
	bool isAdmin() const { return admin_; }
	std::string& getUserAccess() { return userAccess_; }
	int getUserType() const { return userType_; }
	int getUseSchemaIdentifier() const { return useSchemaIdentifier_; }
	int getUseLockTimeoutWaitTransactions() const { return useLockTimeoutWaitTransactions_; }
	bool isVersionAtLeast(int major, int minor = 0) const { return (majorFb_ > major) || (majorFb_ == major && minorFb_ >= minor); }
	const std::string& getDsn() const { return dsn_; }
	const std::string& getUserName() const { return userName_; }
	const std::string& getDatabaseProductName() const { return databaseProductName_; }
	const std::string& getServerVersion() const { return serverVersion_; }
	int getPageSize() const { return pageSize_; }
	bool getQuotedIdentifier() const { return quotedIdentifier_; }
	bool getSensitiveIdentifier() const { return sensitiveIdentifier_; }
	bool getAutoQuotedIdentifier() const { return autoQuotedIdentifier_; }
	const std::string& getDatabaseServerNameStr() const { return databaseServerName_; }

	/// Access the FbClient wrapper (Phase 14.2.1)
	CFbDll* getFbClient() { return GDS; }

	/// Phase 14.3: Check if connection has an active transaction handle
	Firebird::ITransaction* getTransactionHandle()
	{
		return (transaction_ && transaction_->isValid()) ? transaction_->getHandle().get() : nullptr;
	}

public:
	CNodeParamTransaction *tmpParamTransaction;
	CFbDll			*GDS;
	Firebird::IAttachment* databaseHandle;
	std::vector<IscStatement*>	statements;
	IscDatabaseMetaData	*metaData;
	IscUserEvents	*userEvents;
	bool			shareConnected;
	int				useAppOdbcVersion;
	int				useCount;

	// Phase 14.2: Two-phase transaction handle (was Attachment::transactionHandle)
	Firebird::ITransaction* twoPhaseTransactionHandle;

	// Phase 14.2: Attachment mutex (for thread-safe metadata access)
	Mutex			attachmentMutex;

	// Phase 14.2.2: fb-cpp Attachment for RAII connection lifecycle
	std::unique_ptr<fbcpp::Attachment> attachment_;

	// Phase 14.3: fb-cpp Transaction for RAII transaction lifecycle
	std::unique_ptr<fbcpp::Transaction> transaction_;
	int				transactionIsolation_ = 0x00000002L; // SQL_TXN_READ_COMMITTED
	int				transactionExtInit_ = 0;
	bool			autoCommit_ = true;
	bool			transactionPending_ = false;
	CNodeParamTransaction *nodeParamTransaction_ = nullptr;

private:
	// Phase 14.2: Connection management helpers (inlined from Attachment)
	void loadClientLibrary(Properties *properties);
	void checkAdmin();

	// Phase 14.2: Connection metadata (moved from Attachment class)
	std::string	dsn_;
	std::string	databaseName_;
	std::string	databaseServerName_;
	std::string	databaseNameFromServer_;
	std::string	userName_;
	std::string	userAccess_;
	int			userType_ = 8;
	std::string	serverVersion_;
	std::string	databaseProductName_;
	int			majorFb_ = 1;
	int			minorFb_ = 0;
	int			versionFb_ = 0;
	int			charsetCode_ = 0;
	int			pageSize_ = 0;
	int			connectionTimeout_ = 0;
	int			serverBaseLevel_ = 0;
	int			databaseDialect_ = 1; // SQL_DIALECT_V5
	int			useSchemaIdentifier_ = 0;
	int			useLockTimeoutWaitTransactions_ = 0;
	bool		quotedIdentifier_ = false;
	bool		sensitiveIdentifier_ = false;
	bool		autoQuotedIdentifier_ = false;
	int			databaseAccess_ = 0;
	bool		admin_ = true;
	bool		isRoles_ = false;
	bool		ownsConnection_ = false; // true if this IscConnection owns GDS/databaseHandle
};

}; // end namespace IscDbcLibrary

#endif // !defined(_ISCCONNECTION_H_)
