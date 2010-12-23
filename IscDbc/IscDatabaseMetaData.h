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

// IscDatabaseMetaData.h: interface for the IscDatabaseMetaData class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ISCDATABASEMETADATA_H_)
#define _ISCDATABASEMETADATA_H_

#include "Connection.h"
#include "LinkedList.h"

namespace IscDbcLibrary {

class IscConnection;


class IscDatabaseMetaData : public DatabaseMetaData  
{
public:
//{{{ specification jdbc
	virtual ResultSet* getIndexInfo (const char * catalog, const char * schemaPattern, const char * tableNamePattern, bool unique, bool approximate);
	virtual ResultSet* getImportedKeys (const char * catalog, const char * schemaPattern, const char * tableNamePattern);
	virtual ResultSet* getPrimaryKeys (const char * catalog, const char * schemaPattern, const char * tableNamePattern);
	virtual ResultSet* getColumns (const char *catalog, const char *schema, const char *table, const char *fieldNamePattern);
	virtual ResultSet* getTables (const char *catalog, const char *schemaPattern, const char *tableNamePattern, int typeCount, const char **types);
	virtual bool allProceduresAreCallable();
	virtual bool allTablesAreSelectable();
	virtual const char* getURL();
	virtual const char* getDSN();
	virtual const char* getUserName();
	virtual const int getUserType();
	virtual bool isReadOnly();
	virtual bool nullsAreSortedHigh();
	virtual bool nullsAreSortedLow();
	virtual bool nullsAreSortedAtStart();
	virtual bool nullsAreSortedAtEnd();
	virtual const char* getDatabaseProductName();
	virtual const char* getDatabaseProductVersion();
	virtual const char* getDriverName();
	virtual const char* getDriverVersion();
	virtual int getDriverMajorVersion();
	virtual int getDriverMinorVersion();
	virtual bool usesLocalFiles();
	virtual bool usesLocalFilePerTable();
	virtual bool supportsMixedCaseIdentifiers();
	virtual bool storesUpperCaseIdentifiers();
	virtual bool storesLowerCaseIdentifiers();
	virtual bool storesMixedCaseIdentifiers();
	virtual bool supportsMixedCaseQuotedIdentifiers();
	virtual bool storesUpperCaseQuotedIdentifiers();
	virtual bool storesLowerCaseQuotedIdentifiers();
	virtual bool storesMixedCaseQuotedIdentifiers();
	virtual const char* getIdentifierQuoteString();
	virtual const char* getSQLKeywords();
	virtual const char* getNumericFunctions();
	virtual const char* getStringFunctions();
	virtual const char* getSystemFunctions();
	virtual const char* getTimeDateFunctions();
	virtual const char* getSearchStringEscape();
	virtual const char* getExtraNameCharacters();
	virtual bool supportsAlterTableWithAddColumn();
	virtual bool supportsAlterTableWithDropColumn();
	virtual bool supportsColumnAliasing();
	virtual bool nullPlusNonNullIsNull();
	virtual bool supportsConvert();
	virtual bool supportsConvert(int fromType, int toType);
	virtual bool supportsTableCorrelationNames();
	virtual bool supportsDifferentTableCorrelationNames();
	virtual bool supportsExpressionsInOrderBy();
	virtual bool supportsOrderByUnrelated();
	virtual bool supportsGroupBy();
	virtual bool supportsGroupByUnrelated();
	virtual bool supportsGroupByBeyondSelect();
	virtual bool supportsLikeEscapeClause();
	virtual bool supportsMultipleResultSets();
	virtual bool supportsMultipleTransactions();
	virtual bool supportsNonNullableColumns();
	virtual bool supportsMinimumSQLGrammar();
	virtual bool supportsCoreSQLGrammar();
	virtual bool supportsExtendedSQLGrammar();
	virtual bool supportsANSI92EntryLevelSQL();
	virtual bool supportsANSI92IntermediateSQL();
	virtual bool supportsANSI92FullSQL();
	virtual bool supportsIntegrityEnhancementFacility();
	virtual bool supportsOuterJoins();
	virtual bool supportsFullOuterJoins();
	virtual bool supportsLimitedOuterJoins();
	virtual const char* getSchemaTerm();
	virtual const char* getProcedureTerm();
	virtual const char* getCatalogTerm();
	virtual bool isCatalogAtStart();
	virtual const char* getCatalogSeparator();
	virtual bool supportsSchemasInDataManipulation();
	virtual bool supportsSchemasInProcedureCalls();
	virtual bool supportsSchemasInTableDefinitions();
	virtual bool supportsSchemasInIndexDefinitions();
	virtual bool supportsSchemasInPrivilegeDefinitions();
	virtual bool supportsCatalogsInDataManipulation();
	virtual bool supportsCatalogsInProcedureCalls();
	virtual bool supportsCatalogsInTableDefinitions();
	virtual bool supportsCatalogsInIndexDefinitions();
	virtual bool supportsCatalogsInPrivilegeDefinitions();
	virtual bool supportsPositionedDelete();
	virtual bool supportsPositionedUpdate();
	virtual bool supportsSelectForUpdate();
	virtual bool supportsStoredProcedures();
	virtual bool supportsSubqueriesInComparisons();
	virtual bool supportsSubqueriesInExists();
	virtual bool supportsSubqueriesInIns();
	virtual bool supportsSubqueriesInQuantifieds();
	virtual bool supportsCorrelatedSubqueries();
	virtual bool supportsUnion();
	virtual bool supportsUnionAll();
	virtual bool supportsOpenCursorsAcrossCommit();
	virtual bool supportsOpenCursorsAcrossRollback();
	virtual bool supportsOpenStatementsAcrossCommit();
	virtual bool supportsOpenStatementsAcrossRollback();
	virtual int getMaxBinaryLiteralLength();
	virtual int getMaxCharLiteralLength();
	virtual int getMaxColumnNameLength();
	virtual int getMaxColumnsInGroupBy();
	virtual int getMaxColumnsInIndex();
	virtual int getMaxColumnsInOrderBy();
	virtual int getMaxColumnsInSelect();
	virtual int getMaxColumnsInTable();
	virtual int getMaxConnections();
	virtual int getMaxCursorNameLength();
	virtual int getMaxIndexLength();
	virtual int getMaxSchemaNameLength();
	virtual int getMaxProcedureNameLength();
	virtual int getMaxCatalogNameLength();
	virtual int getMaxRowSize();
	virtual bool doesMaxRowSizeIncludeBlobs();
	virtual int getMaxStatementLength();
	virtual int getMaxStatements();
	virtual int getMaxTableNameLength();
	virtual int getMaxTablesInSelect();
	virtual int getMaxUserNameLength();
	virtual int getDefaultTransactionIsolation();
	virtual bool supportsTransactions();
	virtual bool supportsTransactionIsolationLevel(int level);
	virtual bool supportsDataDefinitionAndDataManipulationTransactions();
	virtual bool supportsDataManipulationTransactionsOnly();
	virtual bool dataDefinitionCausesTransactionCommit();
	virtual bool dataDefinitionIgnoredInTransactions();
	virtual ResultSet* getProcedures(const char* catalog, const char* schemaPattern,
			const char* procedureNamePattern);

	virtual ResultSet* getProcedureColumns(const char* catalog,
			const char* schemaPattern,
			const char* procedureNamePattern, 
			const char* columnNamePattern);

	virtual ResultSet* getSchemas();
	virtual ResultSet* getCatalogs();
	virtual ResultSet* getTableTypes();
	virtual ResultSet* getColumnPrivileges(const char* catalog, const char* schema,
		const char* table, const char* columnNamePattern);

	virtual ResultSet* getTablePrivileges(const char* catalog, const char* schemaPattern,
				const char* tableNamePattern);

	virtual ResultSet* getBestRowIdentifier(const char* catalog, const char* schema,
		const char* table, int scope, bool nullable);

	virtual ResultSet* getVersionColumns(const char* catalog, const char* schema,
				const char* table);

	virtual ResultSet* getExportedKeys(const char* catalog, const char* schema,
				const char* table);

	virtual ResultSet* getCrossReference(
		const char* primaryCatalog, const char* primarySchema, const char* primaryTable,
		const char* foreignCatalog, const char* foreignSchema, const char* foreignTable);
	virtual ResultSet* getTypeInfo(int dataType);
//}}} end specification jdbc

public:
	IscDatabaseMetaData(IscConnection *connect);
	~IscDatabaseMetaData();

	virtual ResultSet* getUsers (const char * catalog, const char *userPattern);
	virtual ResultSet* getRoles (const char * catalog, const char * schema, const char *rolePattern);
	virtual ResultSet* getUserRoles (const char *user);
	virtual ResultSet* specialColumns(const char * catalog, const char * schema, const char * table, int scope, int nullable);
	virtual ResultSet* getObjectPrivileges (const char *catalog, const char *schemaPattern, const char *namePattern, int objectType);
	virtual short getSqlStrPageSizeBd(const void * info_buffer, int bufferLength,short *lengthPtr);
	virtual short getSqlStrWalInfoBd(const void * info_buffer, int bufferLength,short *lengthPtr);
	virtual short getStrStatInfoBd(const void * info_buffer, int bufferLength,short *lengthPtr);
    virtual const char* getDatabaseServerName();    
	virtual const char* getUserAccess();
	virtual int getDatabasePageSize();
	virtual const int getUseSchemaIdentifier();
	virtual const int getUseLockTimeoutWaitTransactions();
	virtual bool supportsStatementMetaData();
	virtual ResultSet* getUDTs(const char* catalog, const char* schemaPattern, const char* typeNamePattern, int* types);
	virtual StatementMetaData* getMetaDataTypeInfo(ResultSet* setTypeInfo);
	virtual bool supportsResultSetConcurrency(int type, int concurrency);
	virtual bool ownUpdatesAreVisible(int type);
	virtual bool ownDeletesAreVisible(int type);
	virtual bool ownInsertsAreVisible(int type);
	virtual bool othersUpdatesAreVisible(int type);
	virtual bool othersDeletesAreVisible(int type);
	virtual bool othersInsertsAreVisible(int type);
	virtual bool updatesAreDetected(int type);
	virtual bool deletesAreDetected(int type);
	virtual bool insertsAreDetected(int type);
	virtual bool supportsBatchUpdates();

	void LockThread();
	void UnLockThread();
	virtual int objectVersion();

public:
	IscConnection	*connection;
	LinkedList		resultSets;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_ISCDATABASEMETADATA_H_)
