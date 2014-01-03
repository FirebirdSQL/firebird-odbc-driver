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
 *
 *
 *  2002-06-25	Connection.h
 *				Contributed by C. G. Alvarez
 *				declare getDatabaseServerName() in DatabaseMetaData
 *	
 *
 *	2002-06-04	Connection.h
 *				Contributed by Robert Milharcic
 *				o Added declarations for beginDataTransfer()
 *				  putSegmentData() and endDataTransfer()
 *
 *
 */


// Connection.h: interface for the Connection class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_CONNECTION_H_)
#define _CONNECTION_H_

#include "BinaryBlob.h"
#include "Properties.h"
#include "SQLException.h"

#ifndef QUAD

#ifndef _WINDOWS
#define __int64			long long
#define _stdcall
#endif

typedef unsigned char	UCHAR;
typedef unsigned long	ULONG;
typedef __int64			QUAD;
typedef unsigned __int64			UQUAD;
#endif

#ifdef __BORLANDC__
#define ENTRY_DLL_CREATE_CONNECTION         "_createConnection"
#define ENTRY_DLL_CREATE_SERVICES           "_createServices"
#else
#define ENTRY_DLL_CREATE_CONNECTION         "createConnection"
#define ENTRY_DLL_CREATE_SERVICES           "createServices"
#endif

#if defined ( _WINDOWS)

#define NAME_CLIENT_SHARE_LIBRARY					"gds32.dll"
#define NAME_DEFAULT_CLIENT_SHARE_LIBRARY			"fbclient.dll"

#define OPEN_SHARE_LIBLARY(sharedLibrary)           LoadLibrary( sharedLibrary )
#define GET_ENTRY_POINT(libraryHandle,nameProc)     GetProcAddress( libraryHandle, nameProc )
#define CLOSE_SHARE_LIBLARY(libraryHandle)          FreeLibrary( libraryHandle )

size_t _MbsToWcs( wchar_t *wcstr, const char *mbstr, size_t count );
size_t _WcsToMbs( char *mbstr,  const wchar_t *wcstr, size_t count );

#elif defined (__APPLE__)

#define NAME_CLIENT_SHARE_LIBRARY					"libgds.dylib"
#define NAME_DEFAULT_CLIENT_SHARE_LIBRARY			"libfbclient.dylib"

#define OPEN_SHARE_LIBLARY(sharedLibrary)		    dlopen( sharedLibrary, RTLD_NOW )
#define GET_ENTRY_POINT(libraryHandle,nameProc)     dlsym( libraryHandle, nameProc )
#define CLOSE_SHARE_LIBLARY(libraryHandle)          dlclose( libraryHandle )

#else

#define NAME_CLIENT_SHARE_LIBRARY					"libgds.so"
#define NAME_DEFAULT_CLIENT_SHARE_LIBRARY			"libfbclient.so"

#define OPEN_SHARE_LIBLARY(sharedLibrary)		    dlopen( sharedLibrary, RTLD_NOW )
#define GET_ENTRY_POINT(libraryHandle,nameProc)     dlsym( libraryHandle, nameProc )
#define CLOSE_SHARE_LIBLARY(libraryHandle)          dlclose( libraryHandle )

#endif

#ifndef ISC_TIME_SECONDS_PRECISION

#define ISC_TIME_SECONDS_PRECISION          10000L
#define ISC_TIME_SECONDS_PRECISION_SCALE    (-4)

#endif // ISC_TIME_SECONDS_PRECISION

// Default standart size of fraction it's 9 number  
// It's 9 = ISC_TIME_SECONDS_PRECISION * STD_TIME_SECONDS_PRECISION
#define STD_TIME_SECONDS_PRECISION          100000L

// values for databaseAccess
enum enDatabaseAccess {
	OPEN_DB				= 0,
	CREATE_DB			= 1,
	DROP_DB				= 2
};

/* values for tra_flags */
enum tra_flags_vals {
	TRA_ro				= 1,
	TRA_nw				= 2,
	TRA_con				= 4,
	TRA_rrl 			= 8,
	TRA_inc 			= 16,
	TRA_read_committed	= 32,
	TRA_autocommit		= 64,
	TRA_no_rec_version	= 128,
	TRA_no_auto_undo	= 256
};

typedef void (*callbackEvent)( void *interfaseUserEvents, short length, char *updated );
typedef size_t (*WCSTOMBS)( char *mbstr,  const wchar_t *wcstr, size_t count );
typedef size_t (*MBSTOWCS)( wchar_t *wcstr, const char *mbstr, size_t count );

namespace IscDbcLibrary {

class ServiceManager;
class Connection;
class Statement;
class InternalStatement;
class PreparedStatement;
class CallableStatement;
class ResultSet;
class ResultList;
class DatabaseMetaData;
class DateTime;
class TimeStamp;
class SqlTime;
class StatementMetaData;
class PropertiesEvents;
class UserEvents;

#define CONNECTION_VERSION	1

typedef ServiceManager* (*ServiceManagerFn)();
typedef Connection* (*ConnectFn)();

struct UserInfo
{
	char        *userName;
	char        *firstName;
	char        *middleName;
	char        *lastName;
	char        *password;
	char        *roleName;
	int	        groupId;
	int	        userId;
	UserInfo    *next;
};

class ServiceManager
{
public:
	virtual Properties  *allocProperties() = 0;
	virtual void        startBackupDatabase( Properties *prop, ULONG options ) = 0;
	virtual void        startRestoreDatabase( Properties *prop, ULONG options ) = 0;
	virtual void        exitRestoreDatabase() = 0;
	virtual void        startStaticticsDatabase( Properties *prop, ULONG options ) = 0;
	virtual void        startShowDatabaseLog( Properties *prop ) = 0;
	virtual void        startRepairDatabase( Properties *prop, ULONG options, ULONG optionsValidate ) = 0;
	virtual void        startUsersQuery( Properties *prop ) = 0;
	virtual bool        nextQuery( char *outBuffer, int length, int &lengthOut, int &countError ) = 0;
	virtual bool        nextQueryLimboTransactionInfo( char *outBuffer, int length, int &lengthOut ) = 0;
	virtual bool        nextQueryUserInfo( char *outBuffer, int length, int &lengthOut ) = 0;
	virtual void        closeService() = 0;
	virtual int         getDriverBuildKey() = 0;

	virtual void        addRef() = 0;
	virtual int         release() = 0;
};

class EnvironmentShare
{
public:
	virtual int			getCountConnection () = 0;
	virtual void		sqlEndTran(int operation) = 0;
};

class Connection  
{
public:
//{{{ specification jdbc
	virtual void		clearWarnings() = 0;
	virtual void		close() = 0;
	virtual void		commit() = 0;
	virtual Statement*	createStatement() = 0;
	virtual bool		getAutoCommit() = 0;
	virtual const char*	getCatalog() = 0;
	virtual DatabaseMetaData* getMetaData() = 0;
	virtual int			getTransactionIsolation() = 0;
//	virtual void		getWarnings() = 0;
	virtual bool		isClosed() = 0;
	virtual bool		isReadOnly() = 0;
	virtual const char*	nativeSQL(const char* sqlString) = 0;
	virtual CallableStatement* prepareCall (const char *sql) = 0;
	virtual PreparedStatement* prepareStatement (const char *sqlString) = 0;
	virtual void		rollback() = 0;
	virtual void		setAutoCommit (bool setting) = 0;
	virtual void		setCatalog(const char* catalog) = 0;
	virtual void		setReadOnly(bool readOnly) = 0;
	virtual void		setTransactionIsolation (int level) = 0;
//}}} specification jdbc

	virtual bool		isConnected() = 0;
	virtual void		prepareTransaction() = 0;
	virtual void		commitAuto() = 0;
	virtual void		rollbackAuto() = 0;

	virtual Blob*		genHTML (Properties *context, int genHeaders) = 0;
	virtual int			getNativeSql (const char * inStatementText, int textLength1,
										char * outStatementText, int bufferLength,
										int * textLength2Ptr) = 0;

	virtual PropertiesEvents* allocPropertiesEvents() = 0;
	virtual UserEvents* prepareUserEvents( PropertiesEvents *context, callbackEvent astRoutine, void *userAppData = 0 ) = 0;
	virtual InternalStatement* createInternalStatement() = 0;
	virtual void		ping() = 0;
	virtual int			hasRole (const char *schemaName, const char *roleName) = 0;
	virtual void		sqlExecuteCreateDatabase(const char * sqlString) = 0;
	virtual void		openDatabase (const char *database, Properties *context) = 0;
	virtual void		createDatabase (const char *dbName, Properties *context) = 0;
	virtual Properties*	allocProperties() = 0;
	virtual int			objectVersion() = 0;
	virtual Connection*	clone() = 0;
	virtual bool		getTransactionPending() = 0;
	virtual void		setExtInitTransaction (int optTpb) = 0;
	virtual int			getDriverBuildKey() = 0;
	virtual EnvironmentShare* getEnvironmentShare() = 0;
	virtual void		connectionToEnvShare() = 0;
	virtual void		connectionFromEnvShare() = 0;
	virtual int			getUseAppOdbcVersion () = 0;
	virtual void		setUseAppOdbcVersion ( int appOdbcVersion ) = 0;
	virtual int			getConnectionCharsetCode() = 0;
	virtual WCSTOMBS	getConnectionWcsToMbs() = 0;
	virtual MBSTOWCS	getConnectionMbsToWcs() = 0;

	virtual void		addRef() = 0;
	virtual int			release() = 0;
};

#define DATABASEMETADATA_VERSION	1

class DatabaseMetaData 
{
public:
//{{{ specification jdbc
	virtual bool allProceduresAreCallable() = 0;
	virtual bool allTablesAreSelectable() = 0;
	virtual bool dataDefinitionCausesTransactionCommit() = 0;
	virtual bool dataDefinitionIgnoredInTransactions() = 0;
	virtual bool doesMaxRowSizeIncludeBlobs() = 0;
	virtual ResultSet* getBestRowIdentifier(const char* catalog, const char* schema, const char* table, int scope, bool nullable) = 0;
	virtual ResultSet* getCatalogs() = 0;
	virtual const char* getCatalogSeparator() = 0;
	virtual const char* getCatalogTerm() = 0;
	virtual ResultSet* getColumnPrivileges(const char* catalog, const char* schema,	const char* table, const char* columnNamePattern) = 0;
	virtual ResultSet* getColumns (const char *catalog, const char *schema, const char *table, const char *fieldNamePattern) = 0;
	virtual ResultSet* getCrossReference(const char* primaryCatalog, const char* primarySchema, const char* primaryTable, const char* foreignCatalog, const char* foreignSchema, const char* foreignTable ) = 0;
	virtual const char* getDatabaseProductName() = 0;
	virtual const char* getDatabaseProductVersion() = 0;
	virtual int getDefaultTransactionIsolation() = 0;
	virtual int getDriverMajorVersion() = 0;
	virtual int getDriverMinorVersion() = 0;
	virtual const char* getDriverName() = 0;
	virtual const char* getDriverVersion() = 0;
	virtual ResultSet* getExportedKeys(const char* catalog, const char* schema,	const char* table) = 0;
	virtual const char* getExtraNameCharacters() = 0;
	virtual const char* getIdentifierQuoteString() = 0;
	virtual ResultSet* getImportedKeys (const char * catalog, const char * schemaPattern, const char * tableNamePattern) = 0;
	virtual ResultSet* getIndexInfo (const char * catalog, const char * schemaPattern, const char * tableNamePattern, bool unique, bool approximate) = 0;
	virtual int getMaxBinaryLiteralLength() = 0;
	virtual int getMaxCatalogNameLength() = 0;
	virtual int getMaxCharLiteralLength() = 0;
	virtual int getMaxColumnNameLength() = 0;
	virtual int getMaxColumnsInGroupBy() = 0;
	virtual int getMaxColumnsInIndex() = 0;
	virtual int getMaxColumnsInOrderBy() = 0;
	virtual int getMaxColumnsInSelect() = 0;
	virtual int getMaxColumnsInTable() = 0;
	virtual int getMaxConnections() = 0;
	virtual int getMaxCursorNameLength() = 0;
	virtual int getMaxIndexLength() = 0;
	virtual int getMaxProcedureNameLength() = 0;
	virtual int getMaxRowSize() = 0;
	virtual int getMaxSchemaNameLength() = 0;
	virtual int getMaxStatementLength() = 0;
	virtual int getMaxStatements() = 0;
	virtual int getMaxTableNameLength() = 0;
	virtual int getMaxTablesInSelect() = 0;
	virtual int getMaxUserNameLength() = 0;
	virtual const char* getNumericFunctions() = 0;
	virtual ResultSet* getPrimaryKeys (const char * catalog, const char * schemaPattern, const char * tableNamePattern) = 0;
	virtual ResultSet* getProcedureColumns(const char* catalog,	const char* schemaPattern, const char* procedureNamePattern, const char* columnNamePattern) = 0;
	virtual ResultSet* getProcedures(const char* catalog, const char* schemaPattern, const char* procedureNamePattern) = 0;
	virtual const char* getProcedureTerm() = 0;
	virtual ResultSet* getSchemas() = 0;
	virtual const char* getSchemaTerm() = 0;
	virtual const char* getSearchStringEscape() = 0;
	virtual const char* getSQLKeywords() = 0;
	virtual const char* getStringFunctions() = 0;
	virtual const char* getSystemFunctions() = 0;
	virtual ResultSet* getTablePrivileges(const char* catalog, const char* schemaPattern, const char* tableNamePattern) = 0;
	virtual ResultSet* getTables (const char *catalog, const char *schemaPattern, const char *tableNamePattern, int typeCount, const char **types) = 0;
	virtual ResultSet* getTableTypes() = 0;
	virtual const char* getTimeDateFunctions() = 0;
	virtual ResultSet* getTypeInfo(int dataType) = 0;
	virtual const char* getURL() = 0;
	virtual const char* getUserName() = 0;
	virtual ResultSet* getVersionColumns(const char* catalog, const char* schema, const char* table) = 0;
	virtual bool isCatalogAtStart() = 0;
	virtual bool isReadOnly() = 0;
	virtual bool nullPlusNonNullIsNull() = 0;
	virtual bool nullsAreSortedAtEnd() = 0;
	virtual bool nullsAreSortedAtStart() = 0;
	virtual bool nullsAreSortedHigh() = 0;
	virtual bool nullsAreSortedLow() = 0;
	virtual bool storesLowerCaseIdentifiers() = 0;
	virtual bool storesLowerCaseQuotedIdentifiers() = 0;
	virtual bool storesMixedCaseIdentifiers() = 0;
	virtual bool storesMixedCaseQuotedIdentifiers() = 0;
	virtual bool storesUpperCaseIdentifiers() = 0;
	virtual bool storesUpperCaseQuotedIdentifiers() = 0;
	virtual bool supportsAlterTableWithAddColumn() = 0;
	virtual bool supportsAlterTableWithDropColumn() = 0;
	virtual bool supportsANSI92EntryLevelSQL() = 0;
	virtual bool supportsANSI92FullSQL() = 0;
	virtual bool supportsANSI92IntermediateSQL() = 0;
	virtual bool supportsCatalogsInDataManipulation() = 0;
	virtual bool supportsCatalogsInIndexDefinitions() = 0;
	virtual bool supportsCatalogsInPrivilegeDefinitions() = 0;
	virtual bool supportsCatalogsInProcedureCalls() = 0;
	virtual bool supportsCatalogsInTableDefinitions() = 0;
	virtual bool supportsColumnAliasing() = 0;
	virtual bool supportsConvert() = 0;
	virtual bool supportsConvert(int fromType, int toType) = 0;
	virtual bool supportsCoreSQLGrammar() = 0;
	virtual bool supportsCorrelatedSubqueries() = 0;
	virtual bool supportsDataDefinitionAndDataManipulationTransactions() = 0;
	virtual bool supportsDataManipulationTransactionsOnly() = 0;
	virtual bool supportsDifferentTableCorrelationNames() = 0;
	virtual bool supportsExpressionsInOrderBy() = 0;
	virtual bool supportsExtendedSQLGrammar() = 0;
	virtual bool supportsFullOuterJoins() = 0;
	virtual bool supportsGroupBy() = 0;
	virtual bool supportsGroupByBeyondSelect() = 0;
	virtual bool supportsGroupByUnrelated() = 0;
	virtual bool supportsIntegrityEnhancementFacility() = 0;
	virtual bool supportsLikeEscapeClause() = 0;
	virtual bool supportsLimitedOuterJoins() = 0;
	virtual bool supportsMinimumSQLGrammar() = 0;
	virtual bool supportsMixedCaseIdentifiers() = 0;
	virtual bool supportsMixedCaseQuotedIdentifiers() = 0;
	virtual bool supportsMultipleResultSets() = 0;
	virtual bool supportsMultipleTransactions() = 0;
	virtual bool supportsNonNullableColumns() = 0;
	virtual bool supportsOpenCursorsAcrossCommit() = 0;
	virtual bool supportsOpenCursorsAcrossRollback() = 0;
	virtual bool supportsOpenStatementsAcrossCommit() = 0;
	virtual bool supportsOpenStatementsAcrossRollback() = 0;
	virtual bool supportsOrderByUnrelated() = 0;
	virtual bool supportsOuterJoins() = 0;
	virtual bool supportsPositionedDelete() = 0;
	virtual bool supportsPositionedUpdate() = 0;
	virtual bool supportsSchemasInDataManipulation() = 0;
	virtual bool supportsSchemasInIndexDefinitions() = 0;
	virtual bool supportsSchemasInPrivilegeDefinitions() = 0;
	virtual bool supportsSchemasInProcedureCalls() = 0;
	virtual bool supportsSchemasInTableDefinitions() = 0;
	virtual bool supportsSelectForUpdate() = 0;
	virtual bool supportsStoredProcedures() = 0;
	virtual bool supportsSubqueriesInComparisons() = 0;
	virtual bool supportsSubqueriesInExists() = 0;
	virtual bool supportsSubqueriesInIns() = 0;
	virtual bool supportsSubqueriesInQuantifieds() = 0;
	virtual bool supportsTableCorrelationNames() = 0;
	virtual bool supportsTransactionIsolationLevel(int level) = 0;
	virtual bool supportsTransactions() = 0;
	virtual bool supportsUnion() = 0;
	virtual bool supportsUnionAll() = 0;
	virtual bool usesLocalFilePerTable() = 0;
	virtual bool usesLocalFiles() = 0;
//}}} end specification jdbc

public:
	virtual short getSqlStrPageSizeBd(const void * info_buffer, int bufferLength,short *lengthPtr) = 0;
	virtual short getSqlStrWalInfoBd(const void * info_buffer, int bufferLength,short *lengthPtr) = 0;
	virtual short getStrStatInfoBd(const void * info_buffer, int bufferLength,short *lengthPtr) = 0;
	virtual ResultSet* getObjectPrivileges (const char *catalog, const char *schemaPattern, const char *namePattern, int objectType) = 0;
	virtual ResultSet* getUserRoles (const char *user) = 0;
	virtual ResultSet* getRoles (const char * catalog, const char * schema, const char *rolePattern) = 0;
	virtual ResultSet* getUsers (const char * catalog, const char *userPattern) = 0;
	virtual ResultSet* specialColumns (const char * catalog, const char *schema, const char * table, int scope, int nullable) = 0;
    virtual const char* getDatabaseServerName() = 0;
	virtual const char* getUserAccess() = 0;
	virtual int getDatabasePageSize() = 0;
	virtual StatementMetaData* getMetaDataTypeInfo(ResultSet* setTypeInfo) = 0;
	virtual bool supportsResultSetConcurrency(int type, int concurrency) = 0;
	virtual bool ownUpdatesAreVisible(int type) = 0;
	virtual bool ownDeletesAreVisible(int type) = 0;
	virtual bool ownInsertsAreVisible(int type) = 0;
	virtual bool othersUpdatesAreVisible(int type) = 0;
	virtual bool othersDeletesAreVisible(int type) = 0;
	virtual bool othersInsertsAreVisible(int type) = 0;
	virtual bool updatesAreDetected(int type) = 0;
	virtual bool deletesAreDetected(int type) = 0;
	virtual bool insertsAreDetected(int type) = 0;
	virtual bool supportsBatchUpdates() = 0;
	virtual ResultSet* getUDTs(const char* catalog, const char* schemaPattern, const char* typeNamePattern, int* types) = 0;
	virtual bool supportsStatementMetaData() = 0;
	virtual void LockThread() = 0;
	virtual void UnLockThread() = 0;

	virtual int		objectVersion() = 0;
};

#define JDRIVER_VERSION	1

class Driver
{
public:
//{{{ specification jdbc
	virtual bool		acceptsURL( const char *url ) = 0;
//	virtual Connection* connect( const char *url, Properties info ) = 0;
	virtual int			getMajorVersion() = 0;
	virtual int			getMinorVersion() = 0;
//	virtual DriverPropertyInfo[] getPropertyInfo( const char *url, Properties info ) = 0;
 	virtual bool		jdbcCompliant() = 0;
//}}} end specification jdbc
public:
	virtual int			release() = 0;
	virtual void		addRef() = 0;
	virtual int			objectVersion() = 0;
};

#define STATEMENT_VERSION	1

class Statement  
{
public:
//{{{ specification jdbc
	virtual bool		execute (const char *sqlString) = 0;
	virtual ResultSet*	executeQuery (const char *sqlString) = 0;
	virtual int			getUpdateCount() = 0;
	virtual bool		getMoreResults() = 0;
	virtual void		setCursorName (const char *name) = 0;
	virtual void		setEscapeProcessing(bool enable) = 0;
	virtual ResultSet*	getResultSet() = 0;
	virtual int			executeUpdate (const char *sqlString) = 0;
	virtual int			getMaxFieldSize() = 0;
	virtual int			getMaxRows() = 0;
	virtual int			getQueryTimeout() = 0;
	virtual void		cancel() = 0;
//	virtual void		clearWarnings() = 0;
//	virtual void		getWarnings() = 0;
	virtual void		close() = 0;
	virtual void		setMaxFieldSize(int max) = 0;
	virtual void		setMaxRows(int max) = 0;
	virtual void		setQueryTimeout(int seconds) = 0;
//}}} end specification jdbc

public:
	virtual void		clearResults() = 0;
	virtual ResultList* search (const char *searchString) = 0;
	virtual int			release() = 0;
	virtual void		addRef() = 0;
	virtual bool		isActiveSelect() = 0;
	virtual bool		isActiveSelectForUpdate() = 0;
	virtual bool		isActiveProcedure() = 0;
	virtual bool		isActiveModify() = 0;
	virtual bool		isActiveNone() = 0;
	virtual int			objectVersion() = 0;
	virtual int			getStmtPlan(const void * value, int bufferLength,int *lengthPtr) = 0;
	virtual int			getStmtType(const void * value, int bufferLength,int *lengthPtr) = 0;
	virtual int			getStmtInfoCountRecords(const void * value, int bufferLength,int *lengthPtr) = 0;
	virtual bool		isActiveLocalTransaction() = 0;
	virtual void		setActiveLocalParamTransaction() = 0;
	virtual void		delActiveLocalParamTransaction() = 0;
	virtual void		declareLocalParamTransaction() = 0;
	virtual void		switchTransaction(bool local) = 0;
};

class HeadSqlVar
{
public:
	virtual void		setTypeText() = 0;
	virtual void		setTypeVarying() = 0;
	virtual void		setTypeBoolean() = 0;
	virtual void		setTypeShort() = 0;
	virtual void		setTypeLong() = 0;
	virtual void		setTypeFloat() = 0;
	virtual void		setTypeDouble() = 0;
	virtual void		setType_D_Float() = 0;
	virtual void		setTypeTimestamp() = 0;
	virtual void		setTypeBlob() = 0;
	virtual void		setTypeArray() = 0;
	virtual void		setTypeQuad() = 0;
	virtual void		setTypeTime() = 0;
	virtual void		setTypeDate() = 0;
	virtual void		setTypeInt64() = 0;

	virtual void		setSqlType ( short type ) = 0;
	virtual void		setSqlScale ( short scale ) = 0;
	virtual void		setSqlSubType ( short subtype ) = 0;
	virtual void		setSqlLen ( short len ) = 0;
	virtual short		getSqlMultiple () = 0;

	virtual char *		getSqlData() = 0;
	virtual short *		getSqlInd() = 0;
	virtual void		setSqlData( char *data ) = 0;
	virtual void		setSqlInd( short *ind ) = 0;

	virtual bool		isReplaceForParamArray () = 0;

	virtual void		release() = 0;
	virtual void		restoreOrgPtrSqlData() = 0;
};

#define STATEMENTMETADATA_VERSION	1

class StatementMetaData  
{
public:
// specification jdbc
	virtual int			getColumnCount() = 0;
	virtual int			getColumnType (int index, int &realSqlType) = 0;
	virtual int			getPrecision(int index) = 0;
	virtual int			getNumPrecRadix(int index) = 0;
	virtual int			getScale(int index) = 0;
	virtual bool		isNullable (int index) = 0;
	virtual int			getColumnDisplaySize(int index) = 0;
	virtual const char* getColumnLabel(int index) = 0;
	virtual const char* getSqlTypeName(int index) = 0;
	virtual const char* getColumnName(int index) = 0;
	virtual const char* getTableName(int index) = 0;
	virtual const char* getColumnTypeName(int index) = 0;
	virtual bool		isSigned (int index) = 0;
	virtual bool		isReadOnly (int index) = 0;
	virtual bool		isWritable (int index) = 0;
	virtual bool		isDefinitelyWritable (int index) = 0;
	virtual bool		isCurrency (int index) = 0;
	virtual bool		isCaseSensitive (int index) = 0;
	virtual bool		isAutoIncrement (int index) = 0;
	virtual bool		isSearchable (int index) = 0;
	virtual int			isBlobOrArray(int index) = 0;
	virtual const char*	getSchemaName (int index) = 0;
	virtual const char*	getCatalogName (int index) = 0;
// end specification jdbc
	virtual bool		isColumnPrimaryKey(int index) = 0;

public:
	virtual void		getSqlData(int index, Blob *& ptDataBlob, HeadSqlVar *& ptHeadSqlVar) = 0;
	virtual void		createBlobDataTransfer(int index, Blob *& ptDataBlob) = 0;
	virtual WCSTOMBS	getAdressWcsToMbs( int index ) = 0;
	virtual MBSTOWCS	getAdressMbsToWcs( int index ) = 0;

	virtual int			objectVersion() = 0;
};

#define PREPAREDSTATEMENT_VERSION	1

class PreparedStatement : public Statement  
{
public: 
//{{{ specification jdbc
	virtual void		clearParameters() = 0;
	virtual bool		execute() = 0;
	virtual ResultSet*	executeQuery() = 0;
	virtual int			executeUpdate() = 0;
//	virtual void		setAsciiStream( int parameterIndex, InputStream x, int length ) = 0;
//	virtual void		setBigDecimal( int parameterIndex, BigDecimal x ) = 0;
//	virtual void		setBinaryStream( int parameterIndex, InputStream x, int length ) = 0;
	virtual void		setBoolean( int parameterIndex, bool x ) = 0;
	virtual void		setByte (int index, char value) = 0;
	virtual void		setBytes (int index, const void *bytes) = 0;
	virtual void		setDate (int index, DateTime value) = 0;
	virtual void		setDouble (int index, double value) = 0;
	virtual void		setFloat (int index, float value) = 0;
	virtual void		setInt (int index, int value) = 0;
	virtual void		setLong (int index, QUAD value) = 0;
	virtual void		setNull (int index, int type) = 0;
//	virtual void		setObject( int parameterIndex, Object x ) = 0;
//	virtual void		setObject( int parameterIndex, Object x, int targetSqlType ) = 0;
//	virtual void		setObject( int parameterIndex, Object x, int targetSqlType, int scale ) = 0;
	virtual void		setShort (int index, short value) = 0;
	virtual void		setString(int index, const char * string) = 0;
	virtual void		setTime (int index, SqlTime value) = 0;
	virtual void		setTimestamp (int index, TimeStamp value) = 0;
//	virtual void		setUnicodeStream( int parameterIndex, InputStream x, int length ) = 0;
//}}} end specification jdbc
public:
	virtual void		executeMetaDataQuery() = 0;

	virtual void		setBytes (int index, int length, const void *bytes) = 0;
    virtual void        setString(int index, const char * string, int length) = 0;
	virtual void        convStringData(int index) = 0;
	virtual void		setBlob (int index, Blob *value) = 0;
	virtual void		setArray (int index, Blob *value) = 0;
    virtual void        beginBlobDataTransfer(int index) = 0;
    virtual void        putBlobSegmentData (int length, const void *bytes) = 0;
    virtual void        endBlobDataTransfer() = 0;
	virtual StatementMetaData*	getStatementMetaDataIPD() = 0;
	virtual StatementMetaData*	getStatementMetaDataIRD() = 0;
	virtual	int			getNumParams() = 0;
	virtual int			objectVersion() = 0;
};

#define RESULTSET_VERSION	1

class ResultSet  
{
public:
//{{{ specification jdbc
//	virtual void		clearWarnings() = 0;
	virtual void		close() = 0;
	virtual int			findColumn (const char *columName) = 0;
//	virtual	InputStream* getAsciiStream( int columnIndex ) = 0;
//	virtual	InputStream* getAsciiStream( const char *columnName ) = 0;
//	virtual	BigDecimal	getBigDecimal( int columnIndex, int scale ) = 0;
//	virtual	BigDecimal	getBigDecimal( const char *columnName, int scale ) = 0;
//	virtual	InputStream getBinaryStream( int columnIndex ) = 0;
//	virtual	InputStream getBinaryStream( const char *columnName ) = 0;
	virtual	bool		getBoolean( int columnIndex ) = 0;
	virtual	bool		getBoolean( const char *columnName ) = 0;
	virtual char		getByte (int columnIndex) = 0;
	virtual char		getByte (const char *columnName) = 0;
//	virtual byte[]		getBytes( int columnIndex ) = 0;
//	virtual byte[]		getBytes( int columnIndex ) = 0;
//	virtual byte[]		getBytes( const char *columnName ) = 0;
	virtual const char* getCursorName() = 0;
	virtual DateTime	getDate (int columnIndex) = 0;
	virtual DateTime	getDate (const char *columnName) = 0;
	virtual double		getDouble (int columnIndex) = 0;
	virtual double		getDouble (const char *columnName) = 0;
	virtual float		getFloat (int columnIndex) = 0;
	virtual float		getFloat (const char *columnName) = 0;
	virtual int			getInt (int columnIndex) = 0;
	virtual int			getInt (const char *columnName) = 0;
	virtual QUAD		getLong (int columnIndex) = 0;
	virtual QUAD		getLong (const char *columnName) = 0;
	virtual StatementMetaData* getMetaData() = 0;
//	virtual Object		getObject( int columnIndex ) = 0;
	virtual short		getShort (int columnIndex) = 0;
	virtual short		getShort (const char *columnName) = 0;
	virtual const char* getString (int columnIndex) = 0;
	virtual const char* getString (const char *columnName) = 0;
	virtual SqlTime		getTime (int columnIndex) = 0;
	virtual SqlTime		getTime (const char *columnName) = 0;
	virtual TimeStamp	getTimestamp (int columnIndex) = 0;
	virtual TimeStamp	getTimestamp (const char *columnName) = 0;
//	virtual InputStream getUnicodeStream( int columnIndex ) = 0;
//	virtual InputStream getUnicodeStream( const char *columnName ) = 0;
//	virtual void		getWarnings() = 0;
	virtual bool		next() = 0;
	virtual bool		wasNull() = 0;
//}}} end specification jdbc

public:
	virtual Blob*		getBlob (int index) = 0;
	virtual Blob*		getBlob (const char *columnName) = 0;

	virtual void		setPosRowInSet(int posRow) = 0;
	virtual int			getPosRowInSet() = 0;
	virtual size_t*		getSqlDataOffsetPtr() = 0;
	virtual bool		readStaticCursor() = 0;
	virtual bool		nextFetch() = 0;
	virtual bool		setCurrentRowInBufferStaticCursor(int nRow) = 0;
	virtual void		copyNextSqldaInBufferStaticCursor() = 0;
	virtual void		copyNextSqldaFromBufferStaticCursor() = 0;
	virtual int			getCountRowsStaticCursor() = 0;
	virtual bool		getDataFromStaticCursor (int column/*, Blob * pointerBlobData*/) = 0;
	virtual bool		nextFromProcedure() = 0;
	virtual int			release() = 0;
	virtual void		addRef() = 0;
	virtual int			getColumnCount() = 0;
	virtual bool		isBeforeFirst() = 0;
	virtual bool		isAfterLast() = 0;
	virtual bool		isCurrRowsetStart() = 0;
	virtual bool		isFirst() = 0;
	virtual bool		isLast() = 0;
	virtual void		beforeFirst() = 0;
	virtual void		afterLast() = 0;
	virtual void		currRowsetStart() = 0;
	virtual bool		first() = 0;
	virtual bool		last() = 0;
	virtual int			getRow() = 0;
	virtual bool		absolute (int row) = 0;
	virtual bool		relative (int rows) = 0;
	virtual bool		previous() = 0;
	virtual void		setFetchDirection (int direction) = 0;
	virtual int			getFetchDirection () = 0;
	virtual int			getFetchSize() = 0;
	virtual int			getType() = 0;
	virtual bool		rowUpdated() = 0;
	virtual bool		rowInserted() = 0;
	virtual bool		rowDeleted() = 0;
	virtual void		updateNull (int columnIndex) = 0;
	virtual void		updateBoolean (int columnIndex, bool value) = 0;
	virtual void		updateByte (int columnIndex, char value) = 0;
	virtual void		updateShort (int columnIndex, short value) = 0;
	virtual void		updateInt (int columnIndex, int value) = 0;
	virtual void		updateLong (int columnIndex, QUAD value) = 0;
	virtual void		updateFloat (int columnIndex, float value) = 0;
	virtual void		updateDouble (int columnIndex, double value) = 0;
	virtual void		updateString (int columnIndex, const char* value) = 0;
	virtual void		updateBytes (int columnIndex, int length, const void *bytes) = 0;
	virtual void		updateDate (int columnIndex, DateTime value) = 0;
	virtual void		updateTime (int columnIndex, SqlTime value) = 0;
	virtual void		updateTimeStamp (int columnIndex, TimeStamp value) = 0;
	virtual void		updateBlob (int columnIndex, Blob* value) = 0;
	virtual void		updateNull (const char *columnName) = 0;
	virtual void		updateBoolean (const char *columnName, bool value) = 0;
	virtual void		updateByte (const char *columnName, char value) = 0;
	virtual void		updateShort (const char *columnName, short value) = 0;
	virtual void		updateInt (const char *columnName, int value) = 0;
	virtual void		updateLong (const char *columnName, QUAD value) = 0;
	virtual void		updateFloat (const char *columnName, float value) = 0;
	virtual void		updateDouble (const char *columnName, double value) = 0;
	virtual void		updateString (const char *columnName, const char* value) = 0;
	virtual void		updateBytes (const char *columnName, int length, const void *bytes) = 0;
	virtual void		updateDate (const char *columnName, DateTime value) = 0;
	virtual void		updateTime (const char *columnName, SqlTime value) = 0;
	virtual void		updateTimeStamp (const char *columnName, TimeStamp value) = 0;
	virtual void		updateBlob (const char *columnName, Blob* value) = 0;
	virtual void		insertRow() = 0;
	virtual void		updateRow() = 0;
	virtual void		deleteRow() = 0;
	virtual void		refreshRow() = 0;
	virtual void		cancelRowUpdates() = 0;
	virtual void		moveToInsertRow() = 0;
	virtual void		moveToCurrentRow() = 0;
	virtual Statement	*getStatement() = 0;

	virtual int			objectVersion() = 0;
};

#define RESULTSETMETADATA_VERSION	1

class ResultSetMetaData  
{
public:
	virtual const char*	getTableName (int index) = 0;
	virtual const char*	getColumnName (int index) = 0;
	virtual int			getColumnDisplaySize (int index) = 0;
	virtual int			getColumnType (int index) = 0;
	virtual const char*	getColumnTypeName (int index) = 0;
	virtual int			getColumnCount() = 0;
	virtual int			getPrecision(int index) = 0;
	virtual int			getNumPrecRadix(int index) = 0;
	virtual int			getScale(int index) = 0;
	virtual bool		isNullable (int index) = 0;
	virtual int			objectVersion() = 0;
	virtual const char*	getColumnLabel (int index) = 0;
	virtual bool		isSigned (int index) = 0;
	virtual bool		isReadOnly (int index) = 0;
	virtual bool		isWritable (int index) = 0;
	virtual bool		isDefinitelyWritable (int index) = 0;
	virtual bool		isCurrency (int index) = 0;
	virtual bool		isCaseSensitive (int index) = 0;
	virtual bool		isAutoIncrement (int index) = 0;
	virtual bool		isSearchable (int index) = 0;
	virtual const char*	getSchemaName (int index) = 0;
	virtual const char*	getCatalogName (int index) = 0;
};

#define RESULTLIST_VERSION		1

class ResultList  
{
public:
	virtual const char*	getTableName() = 0;
	virtual double		getScore() = 0;
	virtual int			getCount() = 0;
	virtual ResultSet*	fetchRecord() = 0;
	virtual bool		next() = 0;
	virtual void		close() = 0;
	virtual void		addRef() = 0;
	virtual int			release() = 0;
	virtual int			objectVersion() = 0;
};

#define CALLABLESTATEMENT_VERSION	1

class CallableStatement : public PreparedStatement
{
public:
//{{{ specification jdbc
//	virtual BigDecimal	getBigDecimal( int parameterIndex, int scale ) = 0;
	virtual bool		getBoolean(int parameterIndex) = 0;
	virtual char		getByte(int parameterIndex) = 0;
//	virtual byte[]		getBytes(int parameterIndex) = 0;
	virtual DateTime	getDate(int parameterIndex) = 0;
	virtual double		getDouble(int parameterIndex) = 0;
	virtual float		getFloat(int parameterIndex) = 0;
	virtual int			getInt(int parameterIndex) = 0;
	virtual QUAD		getLong(int parameterIndex) = 0;
//	virtual Object		getObject( int parameterIndex ) = 0;
	virtual short		getShort(int parameterIndex) = 0;
	virtual const char* getString(int parameterIndex) = 0;
	virtual SqlTime		getTime(int parameterIndex) = 0;
	virtual TimeStamp	getTimestamp(int parameterIndex) = 0;
	virtual void		registerOutParameter(int parameterIndex, int sqlType) = 0;
	virtual void		registerOutParameter(int parameterIndex, int sqlType, int scale) = 0;
	virtual bool		wasNull() = 0;
//}}} specification jdbc

public:
	virtual Blob*		getBlob (int parameterIndex) = 0;
};

#define INTERNALSTATEMENT_VERSION	1

class InternalStatement : public Statement
{
public:
	virtual bool		isActive() = 0;
	virtual bool		isActiveDDL() = 0;
	virtual void		prepareStatement(const char * sqlString) = 0;
	virtual bool		executeStatement() = 0;
	virtual bool		executeProcedure() = 0;
	virtual void		rollbackLocal() = 0;
	virtual void		commitLocal() = 0;
	virtual StatementMetaData*	
						getStatementMetaDataIPD() = 0;
	virtual StatementMetaData*	
						getStatementMetaDataIRD() = 0;
	virtual int			getNumParams() = 0;
	virtual void		drop() = 0;
	virtual Statement*	getStatement() = 0;
	virtual int			objectVersion() = 0;
};

class PropertiesEvents
{
public:
	virtual void		putNameEvent( const char *name ) = 0;
	virtual int			getCount() = 0;
	virtual const char	*getNameEvent( int index ) = 0;
	virtual int			findIndex( const char *name ) = 0;
	virtual unsigned long getCountExecutedEvents( int index ) = 0;
	virtual bool		isChanged( int index ) = 0;
	virtual void		addRef() = 0;
	virtual int			release() = 0;
};

#define USEREVENTS_VERSION		1

class UserEvents
{
public:
	virtual void		queEvents( void *interfase = 0 ) = 0;
	virtual bool		isChanged( int numEvent = 0 ) = 0;
	virtual unsigned long getCountEvents( int numEvent = 0 ) = 0;
	virtual int			getCountRegisteredNameEvents() = 0;
	virtual void		updateResultEvents( char *result ) = 0;
	virtual void		*getUserData() = 0;
	virtual void		addRef() = 0;
	virtual int			release() = 0;
	virtual int			objectVersion() = 0;
};

#ifdef __BORLANDC__
extern "C" __declspec( dllexport ) Connection*	createConnection();
extern "C" __declspec( dllexport ) ServiceManager* createServices();
#else
extern "C" Connection*	createConnection();
extern "C" ServiceManager* createServices();
#endif

}; // end namespace IscDbcLibrary

#endif // !defined(_CONNECTION_H_)
