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

#ifndef _WIN32
#define __int64			long long
#define _stdcall
#endif

typedef unsigned char	UCHAR;
typedef unsigned long	ULONG;
typedef __int64			QUAD;
typedef unsigned __int64			UQUAD;
#endif

#define ISC_TIME_SECONDS_PRECISION          10000L
#define ISC_TIME_SECONDS_PRECISION_SCALE    (-4)
// Default standart size of fraction it's 9 number  
// It's 9 = ISC_TIME_SECONDS_PRECISION * STD_TIME_SECONDS_PRECISION
#define STD_TIME_SECONDS_PRECISION          100000L

/* values for tra_flags */
#define TRA_ro			1
#define TRA_nw			2

class Statement;
class PreparedStatement;
class CallableStatement;
class ResultSet;
class ResultList;
class DatabaseMetaData;
class DateTime;
class TimeStamp;
class SqlTime;
class StatementMetaData;

#define CONNECTION_VERSION	1

class Connection  
{
public:
	virtual void		close() = 0;
	virtual void		addRef() = 0;
	virtual int			release() = 0;
	virtual bool		isConnected() = 0;

	virtual void		prepareTransaction() = 0;
	virtual void		rollback() = 0;
	virtual void		commit() = 0;
	virtual void		commitAuto() = 0;
	virtual void		rollbackAuto() = 0;

	virtual Blob*		genHTML (Properties *context, long genHeaders) = 0;
	virtual bool		getNativeSql (const char * inStatementText, long textLength1,
										char * outStatementText, long bufferLength,
										long * textLength2Ptr) = 0;

	virtual Statement*	createStatement() = 0;
	virtual PreparedStatement* prepareStatement (const char *sqlString) = 0;
	virtual DatabaseMetaData* getMetaData() = 0;
	virtual void		ping() = 0;
	virtual int			hasRole (const char *schemaName, const char *roleName) = 0;
	virtual void		openDatabase (const char *database, Properties *context) = 0;
	virtual void		createDatabase (const char *host, const char *dbName, Properties *context) = 0;
	virtual Properties*	allocProperties() = 0;
	virtual int			objectVersion() = 0;
	virtual Connection*	clone() = 0;
	virtual void		setAutoCommit (bool setting) = 0;
	virtual bool		getAutoCommit() = 0;
	virtual void		setTransactionIsolation (int level) = 0;
	virtual int			getTransactionIsolation() = 0;
	virtual bool		getTransactionPending() = 0;
	virtual void		setExtInitTransaction (int optTpb) = 0;
	virtual CallableStatement* prepareCall (const char *sql) = 0;
};

#define DATABASEMETADATA_VERSION	1

class DatabaseMetaData 
{
public:
	virtual short getSqlStrPageSizeBd(const void * info_buffer, int bufferLength,short *lengthPtr) = 0;
	virtual short getSqlStrWalInfoBd(const void * info_buffer, int bufferLength,short *lengthPtr) = 0;
	virtual short getStrStatInfoBd(const void * info_buffer, int bufferLength,short *lengthPtr) = 0;
	virtual ResultSet* getIndexInfo (const char * catalog, const char * schemaPattern, const char * tableNamePattern, bool unique, bool approximate) = 0;
	virtual ResultSet* getImportedKeys (const char * catalog, const char * schemaPattern, const char * tableNamePattern) = 0;
	virtual ResultSet* getPrimaryKeys (const char * catalog, const char * schemaPattern, const char * tableNamePattern) = 0;
	virtual ResultSet* getColumns (const char *catalog, const char *schema, const char *table, const char *fieldNamePattern) = 0;
	virtual ResultSet* getTables (const char *catalog, const char *schemaPattern, const char *tableNamePattern, int typeCount, const char **types) = 0;
	virtual ResultSet* getObjectPrivileges (const char *catalog, const char *schemaPattern, const char *namePattern, int objectType) = 0;
	virtual ResultSet* getUserRoles (const char *user) = 0;
	virtual ResultSet* getRoles (const char * catalog, const char * schema, const char *rolePattern) = 0;
	virtual ResultSet* getUsers (const char * catalog, const char *userPattern) = 0;
	virtual ResultSet* specialColumns (const char * catalog, const char *schema, const char * table, int scope, int nullable) = 0;
    virtual const char* getDatabaseServerName() = 0;
	virtual bool allProceduresAreCallable() = 0;
	virtual bool allTablesAreSelectable() = 0;
	virtual const char* getURL() = 0;
	virtual const char* getUserName() = 0;
	virtual const char* getUserAccess() = 0;
	virtual bool isReadOnly() = 0;
	virtual bool nullsAreSortedHigh() = 0;
	virtual bool nullsAreSortedLow() = 0;
	virtual bool nullsAreSortedAtStart() = 0;
	virtual bool nullsAreSortedAtEnd() = 0;
	virtual const char* getDatabaseProductName() = 0;
	virtual const char* getDatabaseProductVersion() = 0;
	virtual int getDatabasePageSize() = 0;
	virtual const char* getDriverName() = 0;
	virtual const char* getDriverVersion() = 0;
	virtual int getDriverMajorVersion() = 0;
	virtual int getDriverMinorVersion() = 0;
	virtual bool usesLocalFiles() = 0;
	virtual bool usesLocalFilePerTable() = 0;
	virtual bool supportsMixedCaseIdentifiers() = 0;
	virtual bool storesUpperCaseIdentifiers() = 0;
	virtual bool storesLowerCaseIdentifiers() = 0;
	virtual bool storesMixedCaseIdentifiers() = 0;
	virtual bool supportsMixedCaseQuotedIdentifiers() = 0;
	virtual bool storesUpperCaseQuotedIdentifiers() = 0;
	virtual bool storesLowerCaseQuotedIdentifiers() = 0;
	virtual bool storesMixedCaseQuotedIdentifiers() = 0;
	virtual const char* getIdentifierQuoteString() = 0;
	virtual const char* getSQLKeywords() = 0;
	virtual const char* getNumericFunctions() = 0;
	virtual const char* getStringFunctions() = 0;
	virtual const char* getSystemFunctions() = 0;
	virtual const char* getTimeDateFunctions() = 0;
	virtual const char* getSearchStringEscape() = 0;
	virtual const char* getExtraNameCharacters() = 0;
	virtual bool supportsAlterTableWithAddColumn() = 0;
	virtual bool supportsAlterTableWithDropColumn() = 0;
	virtual bool supportsColumnAliasing() = 0;
	virtual bool nullPlusNonNullIsNull() = 0;
	virtual bool supportsConvert() = 0;
	virtual bool supportsConvert(int fromType, int toType) = 0;
	virtual bool supportsTableCorrelationNames() = 0;
	virtual bool supportsDifferentTableCorrelationNames() = 0;
	virtual bool supportsExpressionsInOrderBy() = 0;
	virtual bool supportsOrderByUnrelated() = 0;
	virtual bool supportsGroupBy() = 0;
	virtual bool supportsGroupByUnrelated() = 0;
	virtual bool supportsGroupByBeyondSelect() = 0;
	virtual bool supportsLikeEscapeClause() = 0;
	virtual bool supportsMultipleResultSets() = 0;
	virtual bool supportsMultipleTransactions() = 0;
	virtual bool supportsNonNullableColumns() = 0;
	virtual bool supportsMinimumSQLGrammar() = 0;
	virtual bool supportsCoreSQLGrammar() = 0;
	virtual bool supportsExtendedSQLGrammar() = 0;
	virtual bool supportsANSI92EntryLevelSQL() = 0;
	virtual bool supportsANSI92IntermediateSQL() = 0;
	virtual bool supportsANSI92FullSQL() = 0;
	virtual bool supportsIntegrityEnhancementFacility() = 0;
	virtual bool supportsOuterJoins() = 0;
	virtual bool supportsFullOuterJoins() = 0;
	virtual bool supportsLimitedOuterJoins() = 0;
	virtual const char* getSchemaTerm() = 0;
	virtual const char* getProcedureTerm() = 0;
	virtual const char* getCatalogTerm() = 0;
	virtual bool isCatalogAtStart() = 0;
	virtual const char* getCatalogSeparator() = 0;
	virtual bool supportsSchemasInDataManipulation() = 0;
	virtual bool supportsSchemasInProcedureCalls() = 0;
	virtual bool supportsSchemasInTableDefinitions() = 0;
	virtual bool supportsSchemasInIndexDefinitions() = 0;
	virtual bool supportsSchemasInPrivilegeDefinitions() = 0;
	virtual bool supportsCatalogsInDataManipulation() = 0;
	virtual bool supportsCatalogsInProcedureCalls() = 0;
	virtual bool supportsCatalogsInTableDefinitions() = 0;
	virtual bool supportsCatalogsInIndexDefinitions() = 0;
	virtual bool supportsCatalogsInPrivilegeDefinitions() = 0;
	virtual bool supportsPositionedDelete() = 0;
	virtual bool supportsPositionedUpdate() = 0;
	virtual bool supportsSelectForUpdate() = 0;
	virtual bool supportsStoredProcedures() = 0;
	virtual bool supportsSubqueriesInComparisons() = 0;
	virtual bool supportsSubqueriesInExists() = 0;
	virtual bool supportsSubqueriesInIns() = 0;
	virtual bool supportsSubqueriesInQuantifieds() = 0;
	virtual bool supportsCorrelatedSubqueries() = 0;
	virtual bool supportsUnion() = 0;
	virtual bool supportsUnionAll() = 0;
	virtual bool supportsOpenCursorsAcrossCommit() = 0;
	virtual bool supportsOpenCursorsAcrossRollback() = 0;
	virtual bool supportsOpenStatementsAcrossCommit() = 0;
	virtual bool supportsOpenStatementsAcrossRollback() = 0;
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
	virtual int getMaxSchemaNameLength() = 0;
	virtual int getMaxProcedureNameLength() = 0;
	virtual int getMaxCatalogNameLength() = 0;
	virtual int getMaxRowSize() = 0;
	virtual bool doesMaxRowSizeIncludeBlobs() = 0;
	virtual int getMaxStatementLength() = 0;
	virtual int getMaxStatements() = 0;
	virtual int getMaxTableNameLength() = 0;
	virtual int getMaxTablesInSelect() = 0;
	virtual int getMaxUserNameLength() = 0;
	virtual int getDefaultTransactionIsolation() = 0;
	virtual bool supportsTransactions() = 0;
	virtual bool supportsTransactionIsolationLevel(int level) = 0;
	virtual bool supportsDataDefinitionAndDataManipulationTransactions() = 0;
	virtual bool supportsDataManipulationTransactionsOnly() = 0;
	virtual bool dataDefinitionCausesTransactionCommit() = 0;
	virtual bool dataDefinitionIgnoredInTransactions() = 0;
	virtual ResultSet* getProcedures(const char* catalog, const char* schemaPattern,
			const char* procedureNamePattern) = 0;

	virtual ResultSet* getProcedureColumns(const char* catalog,
			const char* schemaPattern,
			const char* procedureNamePattern, 
			const char* columnNamePattern) = 0;

	virtual ResultSet* getSchemas() = 0;
	virtual ResultSet* getCatalogs() = 0;
	virtual ResultSet* getTableTypes() = 0;
	virtual ResultSet* getColumnPrivileges(const char* catalog, const char* schema,
		const char* table, const char* columnNamePattern) = 0;

	virtual ResultSet* getTablePrivileges(const char* catalog, const char* schemaPattern,
				const char* tableNamePattern) = 0;

	virtual ResultSet* getBestRowIdentifier(const char* catalog, const char* schema,
		const char* table, int scope, bool nullable) = 0;

	virtual ResultSet* getVersionColumns(const char* catalog, const char* schema,
				const char* table) = 0;

	virtual ResultSet* getExportedKeys(const char* catalog, const char* schema,
				const char* table) = 0;

	virtual ResultSet* getCrossReference(
		const char* primaryCatalog, const char* primarySchema, const char* primaryTable,
		const char* foreignCatalog, const char* foreignSchema, const char* foreignTable
		) = 0;

	virtual ResultSet* getTypeInfo(int dataType) = 0;
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
	virtual ResultSet* getUDTs(const char* catalog, const char* schemaPattern, 
			  const char* typeNamePattern, int* types) = 0;
	virtual int		objectVersion() = 0;
	virtual bool supportsStatementMetaData() = 0;
	virtual void LockThread() = 0;
	virtual void UnLockThread() = 0;
};

#define STATEMENT_VERSION	1

class Statement  
{
public:
	virtual bool		execute (const char *sqlString) = 0;
	virtual ResultSet*	executeQuery (const char *sqlString) = 0;
	virtual int			getUpdateCount() = 0;
	virtual void		clearResults() = 0;
	virtual bool		getMoreResults() = 0;
	virtual void		setCursorName (const char *name) = 0;
	virtual ResultSet*	getResultSet() = 0;
	virtual ResultList* search (const char *searchString) = 0;
	virtual int			executeUpdate (const char *sqlString) = 0;
	virtual void		close() = 0;
	virtual int			release() = 0;
	virtual void		addRef() = 0;
	virtual int			objectVersion() = 0;
	virtual int			getStmtPlan(const void * value, int bufferLength,long *lengthPtr) = 0;
	virtual int			getStmtType(const void * value, int bufferLength,long *lengthPtr) = 0;
	virtual int			getStmtInfoCountRecords(const void * value, int bufferLength,long *lengthPtr) = 0;
	
};

#define STATEMENTMETADATA_VERSION	1

class StatementMetaData  
{
public:
	virtual int			getColumnCount() = 0;
	virtual int			getColumnType (int index, int &realSqlType) = 0;
	virtual int			getPrecision(int index) = 0;
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

	virtual void		getSqlData(int index, char *& ptData, short *& ptIndData, Blob *& ptDataBlob) = 0;
	virtual void		setSqlData(int index, long ptData, long ptIndData) = 0;
	virtual void		saveSqlData(int index) = 0;
	virtual void		restoreSqlData(int index) = 0;

	virtual int			objectVersion() = 0;
};

#define PREPAREDSTATEMENT_VERSION	1

class PreparedStatement : public Statement  
{
public:
	virtual bool		execute() = 0;
	virtual ResultSet*	executeQuery() = 0;
	virtual void		executeMetaDataQuery() = 0;
	virtual int			executeUpdate() = 0;
	virtual void		setString(int index, const char * string) = 0;
    virtual void        setString(int index, const char * string, int length) = 0;
	virtual void        convStringData(int index) = 0;
	virtual void		setByte (int index, char value) = 0;
	virtual void		setShort (int index, short value) = 0;
	virtual void		setInt (int index, long value) = 0;
	virtual void		setQuad (int index, QUAD value) = 0;
	virtual void		setBytes (int index, int length, const void *bytes) = 0;
//Next three lines added by RM 2002-06-4
    virtual void        beginBlobDataTransfer(int index) = 0;
    virtual void        putBlobSegmentData (int length, const void *bytes) = 0;
    virtual void        endBlobDataTransfer() = 0;

	virtual void		setFloat (int index, float value) = 0;
	virtual void		setDouble (int index, double value) = 0;
	virtual void		setNull (int index, int type) = 0;
	virtual void		setDate (int index, DateTime value) = 0;
	virtual void		setTime (int index, SqlTime value) = 0;
	virtual void		setTimestamp (int index, TimeStamp value) = 0;
	virtual void		setBlob (int index, Blob *value) = 0;
	virtual void		setArray (int index, Blob *value) = 0;
	virtual StatementMetaData*
						getStatementMetaDataIPD() = 0;
	virtual StatementMetaData*
						getStatementMetaDataIRD() = 0;
	virtual	int			getNumParams() = 0;
	virtual int			objectVersion() = 0;
};

#define RESULTSET_VERSION	1

class ResultSet  
{
public:
	virtual const char* getString (int id) = 0;
	virtual const char* getString (const char *columnName) = 0;
	virtual char		getByte (int id) = 0;
	virtual char		getByte (const char *columnName) = 0;
	virtual short		getShort (int id) = 0;
	virtual short		getShort (const char *columnName) = 0;
	virtual long		getInt (int id) = 0;
	virtual long		getInt (const char *columnName) = 0;
	virtual float		getFloat (int id) = 0;
	virtual float		getFloat (const char *columnName) = 0;
	virtual double		getDouble (int id) = 0;
	virtual double		getDouble (const char *columnName) = 0;
	virtual DateTime	getDate (int id) = 0;
	virtual DateTime	getDate (const char *columnName) = 0;
	virtual SqlTime		getTime (int id) = 0;
	virtual SqlTime		getTime (const char *columnName) = 0;
	virtual TimeStamp	getTimestamp (int id) = 0;
	virtual TimeStamp	getTimestamp (const char *columnName) = 0;
	virtual Blob*		getBlob (int index) = 0;
	virtual Blob*		getBlob (const char *columnName) = 0;
	virtual QUAD		getQuad (int id) = 0;
	virtual QUAD		getQuad (const char *columnName) = 0;

	virtual int			findColumn (const char *columName) = 0;
	virtual StatementMetaData* getMetaData() = 0;
	virtual void		close() = 0;
	virtual void		setPosRowInSet(int posRow) = 0;
	virtual int			getPosRowInSet() = 0;
	virtual long*		getSqlDataOffsetPtr() = 0;
	virtual bool		readStaticCursor() = 0;
	virtual bool		readForwardCursor() = 0;
	virtual bool		setCurrentRowInBufferStaticCursor(int nRow) = 0;
	virtual void		copyNextSqldaInBufferStaticCursor() = 0;
	virtual void		copyNextSqldaFromBufferStaticCursor() = 0;
	virtual int			getCountRowsStaticCursor() = 0;
	virtual bool		getDataFromStaticCursor (int column/*, Blob * pointerBlobData*/) = 0;
	virtual bool		next() = 0;
	virtual int			release() = 0;
	virtual void		addRef() = 0;
	virtual bool		wasNull() = 0;
	virtual int			objectVersion() = 0;
	virtual bool		isBeforeFirst() = 0;
	virtual bool		isAfterLast() = 0;
	virtual bool		isFirst() = 0;
	virtual bool		isLast() = 0;
	virtual void		beforeFirst() = 0;
	virtual void		afterLast() = 0;
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
	virtual void		registerOutParameter(int parameterIndex, int sqlType) = 0;
	virtual void		registerOutParameter(int parameterIndex, int sqlType, int scale) = 0;
	virtual bool		wasNull() = 0;
	virtual const char* getString(int parameterIndex) = 0;
	//virtual bool		getBoolean(int parameterIndex) = 0;
	virtual char		getByte(int parameterIndex) = 0;
	virtual short		getShort(int parameterIndex) = 0;
	virtual long		getInt(int parameterIndex) = 0;
	virtual float		getFloat(int parameterIndex) = 0;
	virtual double		getDouble(int parameterIndex) = 0;
	//virtual byte[]	getBytes(int parameterIndex) = 0;
	virtual DateTime	getDate(int parameterIndex) = 0;
	virtual SqlTime		getTime(int parameterIndex) = 0;
	virtual TimeStamp	getTimestamp(int parameterIndex) = 0;
	virtual QUAD		getQuad(int parameterIndex) = 0;
	virtual Blob*		getBlob (int i) = 0;
};

#ifdef __BORLANDC__
extern "C" __declspec( dllexport ) Connection*	createConnection();
#else
extern "C" Connection*	createConnection();
#endif

#endif // !defined(_CONNECTION_H_)
