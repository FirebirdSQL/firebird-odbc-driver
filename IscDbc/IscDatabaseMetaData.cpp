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
 *	2002-10-11	IscDatabaseMetaData.cpp
 *				Contributed by C. G. Alvarez
 *				Implemented full list of keywords
 *
 *	2002-08-12	IscDatabaseMetaData.cpp
 *				Contributed by C. G. Alvarez
 *				Implemented getTablePrivileges()
 *
 *
 *	2002-07-02	IscDatabaseMetaData.cpp
 *				C. G.Alvarez
 *				Return TRANSACTION_READ_COMMITTED from 
 *				IscDatabaseMetaData::getDefaultTransactionIsolation()
 *
 *
 *	2002-06-25	IscDatabaseMetaData.cpp
 *				C. G.Alvarez
 *				Implement getDatabaseServerName()
 *
 */

// IscDatabaseMetaData.cpp: implementation of the IscDatabaseMetaData class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "IscDbc.h"
#include "../SetupAttributes.h"
#include "IscDatabaseMetaData.h"
#include "IscConnection.h"
#include "Attachment.h"
#include "SQLError.h"
#include "IscResultSet.h"
#include "IscTablesResultSet.h"
#include "IscColumnPrivilegesResultSet.h"
#include "IscColumnsResultSet.h"
#include "IscIndexInfoResultSet.h"
#include "IscPrimaryKeysResultSet.h"
#include "IscCrossReferenceResultSet.h"
#include "IscProceduresResultSet.h"
#include "IscProcedureColumnsResultSet.h"
#include "TypesResultSet.h"
#include "IscSpecialColumnsResultSet.h"
#include "IscTablePrivilegesResultSet.h" 

#define TRANSACTION_READ_UNCOMMITTED  1

/**
 * Dirty reads are prevented; non-repeatable reads and phantom
 * reads can occur.
 */
#define TRANSACTION_READ_COMMITTED    2

/**
 * Dirty reads and non-repeatable reads are prevented; phantom
 * reads can occur.     
 */
#define TRANSACTION_REPEATABLE_READ   4

/**
 * Dirty reads, non-repeatable reads and phantom reads are prevented.
 */
#define TRANSACTION_SERIALIZABLE      8


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscDatabaseMetaData::IscDatabaseMetaData(IscConnection *connect)
{
	connection = connect;
}

IscDatabaseMetaData::~IscDatabaseMetaData()
{

}

void IscDatabaseMetaData::LockThread()
{
	connection->attachment->mutex.lock();
}

void IscDatabaseMetaData::UnLockThread()
{
	connection->attachment->mutex.release();
}

short IscDatabaseMetaData::getSqlStrPageSizeBd(const void * info_buffer, int bufferLength,short *lengthPtr)
{
	return getPageDatabase(connection, info_buffer,bufferLength,lengthPtr);
}
short IscDatabaseMetaData::getSqlStrWalInfoBd(const void * info_buffer, int bufferLength,short *lengthPtr)
{
	return getWalDatabase(connection, info_buffer,bufferLength,lengthPtr);
}
short IscDatabaseMetaData::getStrStatInfoBd(const void * info_buffer, int bufferLength,short *lengthPtr)
{
	return getStatInformations(connection, info_buffer,bufferLength,lengthPtr);
}


ResultSet* IscDatabaseMetaData::getTables(const char * catalog, const char * schemaPattern, const char * tableNamePattern, int typeCount, const char **types)
{
	IscTablesResultSet *resultSet = new IscTablesResultSet (this);

	try
		{
		resultSet->getTables (catalog, schemaPattern, tableNamePattern, typeCount, types);
		}
	catch (...)
		{
		delete resultSet;
		throw;
		}

	return resultSet;
}

ResultSet* IscDatabaseMetaData::getColumns(const char * catalog, const char * schemaPattern, const char * tableNamePattern, const char * fieldNamePattern)
{
	IscColumnsResultSet *resultSet = new IscColumnsResultSet (this);

	try
		{
		resultSet->getColumns (catalog, schemaPattern, tableNamePattern, fieldNamePattern);
		}
	catch (...)
		{
		delete resultSet;
		throw;
		}

	return resultSet;
}

ResultSet* IscDatabaseMetaData::specialColumns(const char * catalog, const char * schema, const char * table, int scope, int nullable)
{
	IscSpecialColumnsResultSet *resultSet = new IscSpecialColumnsResultSet (this);

	try
		{
		resultSet->specialColumns (catalog, schema, table, scope, nullable);
		}
	catch (...)
		{
		delete resultSet;
		throw;
		}

	return resultSet;
}

ResultSet* IscDatabaseMetaData::getPrimaryKeys(const char * catalog, const char * schemaPattern, const char * tableNamePattern)
{
	IscPrimaryKeysResultSet *resultSet = new IscPrimaryKeysResultSet (this);

	try
		{
		resultSet->getPrimaryKeys (catalog, schemaPattern, tableNamePattern);
		}
	catch (...)
		{
		delete resultSet;
		throw;
		}

	return resultSet;
}

ResultSet* IscDatabaseMetaData::getImportedKeys(const char * catalog, const char * schemaPattern, const char * tableNamePattern)
{
	NOT_YET_IMPLEMENTED;
	IscResultSet *resultSet = new IscResultSet (NULL);

	return (ResultSet*) resultSet;
}

ResultSet* IscDatabaseMetaData::getIndexInfo(const char * catalog, 
											 const char * schemaPattern, 
											 const char * tableNamePattern, 
											 bool unique, bool approximate)
{
	IscIndexInfoResultSet *resultSet = new IscIndexInfoResultSet (this);

	try
		{
		resultSet->getIndexInfo (catalog, schemaPattern, tableNamePattern, unique, approximate);
		}
	catch (...)
		{
		delete resultSet;
		throw;
		}

	return resultSet;
}

ResultSet* IscDatabaseMetaData::getObjectPrivileges(const char * catalog, const char * schemaPattern, const char * namePattern, int objectType)
{
	NOT_YET_IMPLEMENTED;
	IscResultSet *resultSet = new IscResultSet (NULL);

	return (ResultSet*) resultSet;
}

ResultSet* IscDatabaseMetaData::getUserRoles(const char * user)
{
	NOT_YET_IMPLEMENTED;
	IscResultSet *resultSet = new IscResultSet (NULL);

	return (ResultSet*) resultSet;
}

ResultSet* IscDatabaseMetaData::getRoles(const char * catalog, const char * schemaPattern, const char *rolePattern)
{
	NOT_YET_IMPLEMENTED;
	IscResultSet *resultSet = new IscResultSet (NULL);

	return (ResultSet*) resultSet;
}

ResultSet* IscDatabaseMetaData::getUsers(const char * catalog, const char *userPattern)
{
	NOT_YET_IMPLEMENTED;
	IscResultSet *resultSet = new IscResultSet (NULL);

	return (ResultSet*) resultSet;
}

bool IscDatabaseMetaData::allProceduresAreCallable()
	{
	return connection->attachment->isAdmin();
	}

bool IscDatabaseMetaData::allTablesAreSelectable()
	{
	return connection->attachment->isAdmin();
	}

const char* IscDatabaseMetaData::getURL()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

const char* IscDatabaseMetaData::getUserName()
	{
	return connection->attachment->userName;
	}

const char* IscDatabaseMetaData::getUserAccess()
	{
	return connection->attachment->getUserAccess();
	}

const int IscDatabaseMetaData::getUserType()
	{
	return connection->attachment->getUserType();
	}

JString IscDatabaseMetaData::existsAccess(const char *prefix, const char * relobject, int typeobject, const char *suffix)
	{
	return connection->attachment->existsAccess(prefix, relobject, typeobject, suffix);
	}

bool IscDatabaseMetaData::isReadOnly()
	{
	return false;
	}

bool IscDatabaseMetaData::nullsAreSortedHigh()
	{
	return false;
	}

bool IscDatabaseMetaData::nullsAreSortedLow()
	{
	return false;
	}

bool IscDatabaseMetaData::nullsAreSortedAtStart()
	{
	return false;
	}

bool IscDatabaseMetaData::nullsAreSortedAtEnd()
	{
	return true;
	}

const char* IscDatabaseMetaData::getDatabaseServerName()
	{
	return "Firebird / InterBase(r) Server";
	}

const char* IscDatabaseMetaData::getDatabaseProductName()
	{
	return "Firebird / InterBase(r)";
	}

const char* IscDatabaseMetaData::getDatabaseProductVersion()
	{
	return connection->attachment->serverVersion;
	}

int IscDatabaseMetaData::getDatabasePageSize()
	{
	return connection->attachment->pageSize;
	}

const char* IscDatabaseMetaData::getDriverName()
	{
	return "IscDbc";
	}

const char* IscDatabaseMetaData::getDriverVersion()
	{
	return DRIVER_VERSION;
	}

int IscDatabaseMetaData::getDriverMajorVersion()
	{
	return MAJOR_VERSION;
	}

int IscDatabaseMetaData::getDriverMinorVersion()
	{
	return MINOR_VERSION;
	}

bool IscDatabaseMetaData::usesLocalFiles()
	{
	return false;
	}

bool IscDatabaseMetaData::usesLocalFilePerTable()
	{
	return false;
	}

bool IscDatabaseMetaData::supportsMixedCaseIdentifiers()
	{
	return true;
	}

bool IscDatabaseMetaData::storesUpperCaseIdentifiers()
	{
	return connection->attachment->databaseDialect < 3;
	}

bool IscDatabaseMetaData::storesLowerCaseIdentifiers()
	{
	return false;
	}

bool IscDatabaseMetaData::storesMixedCaseIdentifiers()
	{
	return false;
	}

bool IscDatabaseMetaData::supportsMixedCaseQuotedIdentifiers()
	{
	return true;
	}

bool IscDatabaseMetaData::storesUpperCaseQuotedIdentifiers()
	{
	return false;
	}

bool IscDatabaseMetaData::storesLowerCaseQuotedIdentifiers()
	{
	return false;
	}

bool IscDatabaseMetaData::storesMixedCaseQuotedIdentifiers()
	{
	return connection->attachment->databaseDialect < 3;
	}

const char* IscDatabaseMetaData::getIdentifierQuoteString()
	{
	return (connection->attachment->quotedIdentifiers) ? "\"" : " ";
	}

const char* IscDatabaseMetaData::getSQLKeywords()
	{
	return  "ACTIVE,ADMIN,AFTER,ASCENDING,AUTO,AUTODDL,BASED,BASENAME,BASE_NAME,"
			"BEFORE,BIGINT,BLOB,BLOBEDIT,BUFFER,CACHE,CHECK_POINT_LEN,CHECK_POINT_LENGTH,"
			"COMMITTED,COMPILETIME,COMPUTED,CONDITIONAL,CONTAINING,CSTRING,CURRENT_CONNECTION,"
			"CURRENT_TRANSACTION,DATABASE,DB_KEY,DEBUG,DESCENDING,DISPLAY,ECHO,EDIT,ENTRY_POINT,"
			"EVENT,EXIT,FILE,FILTER,FIRST,FREE_IT,FUNCTION,GDSCODE,GENERATOR,GEN_ID,"
			"GROUP_COMMIT_WAIT,GROUP_COMMIT_,WAIT_TIME,HELP,IF,INACTIVE,INPUT_TYPE,ISQL,"
			"LC_MESSAGES,LC_TYPE,LENGTH,LOGFILE,LOG_BUFFER_SIZE,LOG_BUF_SIZE,LONG,MANUAL,MAXIMUM,"
			"MAXIMUM_SEGMENT,MAX_SEGMENT,MERGE,MESSAGE,MINIMUM,MODULE_NAME,NEW,NOAUTO,"
			"NUM_LOG_BUFS,NUM_LOG_BUFFERS,OLD,OUTPUT_TYPE,OVERFLOW,PAGE,PAGELENGTH,PAGES,"
			"PAGE_SIZE,PARAMETER,PASSWORD,PLAN,POST_EVENT,PROTECTED,QUIT,RAW_PARTITIONS,"
			"RDB$DB_KEY,RECORD_VERSION,RECREATE,RELEASE,RESERV,RESERVING,RETAIN,RETURN,"
			"RETURNING_VALUES,RETURNS,ROLE,RUNTIME,SCHEMA,SEGMENT,SHADOW,SHARED,SHELL,"
			"SHOW,SINGULAR,SNAPSHOT,SORT,STABILITY,STARTING,STARTS,STATEMENT,STATIC,"
			"STATISTICS,SUB_TYPE,SUSPEND,TERM,TERMINATOR,TRIGGER,TYPE,UNCOMMITTED,VARIABLE,"
			"VERSION,WAIT,WEEKDAY,WHILE,YEARDAY";
	}

const char* IscDatabaseMetaData::getNumericFunctions()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

const char* IscDatabaseMetaData::getStringFunctions()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

const char* IscDatabaseMetaData::getSystemFunctions()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

const char* IscDatabaseMetaData::getTimeDateFunctions()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

const char* IscDatabaseMetaData::getSearchStringEscape()
	{
	return "\\";
	}

const char* IscDatabaseMetaData::getExtraNameCharacters()
	{
	return "$";	
	}

bool IscDatabaseMetaData::supportsAlterTableWithAddColumn()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsAlterTableWithDropColumn()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsColumnAliasing()
	{
	return true;
	}

bool IscDatabaseMetaData::nullPlusNonNullIsNull()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsConvert()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsConvert(int fromType, int toType)
	{
	if ( fromType == JDBC_NUMERIC )
		if ( toType == JDBC_CHAR || toType == JDBC_VARCHAR || toType == JDBC_DATE ||
			 toType == JDBC_TIME || toType == JDBC_TIMESTAMP )
			return true;
		else
			return false;

	if ( fromType == JDBC_CHAR || fromType == JDBC_VARCHAR )
		if ( toType == JDBC_NUMERIC || toType == JDBC_DATE ||
			toType == JDBC_TIME || toType == JDBC_TIMESTAMP )
			return true;
		else
			return false;

	if ( fromType == JDBC_DATE )
		if ( toType == JDBC_CHAR || toType == JDBC_VARCHAR || toType == JDBC_TIMESTAMP )
			return true;
		else
			return false;

	if ( fromType == JDBC_TIME )
		if ( toType == JDBC_CHAR || toType == JDBC_VARCHAR || toType == JDBC_TIMESTAMP )
			return true;
		else
			return false;

	if ( fromType == JDBC_TIMESTAMP )
		if ( toType == JDBC_CHAR || toType == JDBC_VARCHAR || toType == JDBC_DATE || 
			 toType == JDBC_TIME )
			return true;
		else
			return false;

	return false;
	}

bool IscDatabaseMetaData::supportsTableCorrelationNames()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsDifferentTableCorrelationNames()
	{
	return false;
	}

bool IscDatabaseMetaData::supportsExpressionsInOrderBy()
	{
	return false;
	}

bool IscDatabaseMetaData::supportsOrderByUnrelated()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsGroupBy()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsGroupByUnrelated()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsGroupByBeyondSelect()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsLikeEscapeClause()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsMultipleResultSets()
	{
	return false;
	}

bool IscDatabaseMetaData::supportsMultipleTransactions()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsNonNullableColumns()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsMinimumSQLGrammar()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsCoreSQLGrammar()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsExtendedSQLGrammar()
	{
	return false;
	}

bool IscDatabaseMetaData::supportsANSI92EntryLevelSQL()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsANSI92IntermediateSQL()
	{
	return false;
	}

bool IscDatabaseMetaData::supportsANSI92FullSQL()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsIntegrityEnhancementFacility()
	{
	return false;
	}

bool IscDatabaseMetaData::supportsOuterJoins()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsFullOuterJoins()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsLimitedOuterJoins()
	{
	return true;
	}

const char* IscDatabaseMetaData::getSchemaTerm()
	{
	return "";
//	return "schema";
	}

const char* IscDatabaseMetaData::getProcedureTerm()
	{
	return "procedure";
	}

const char* IscDatabaseMetaData::getCatalogTerm()
	{
	return "system tables";
	}

bool IscDatabaseMetaData::isCatalogAtStart()
	{
	return true;
	}

const char* IscDatabaseMetaData::getCatalogSeparator()
	{
	return ".";
	}

bool IscDatabaseMetaData::supportsSchemasInDataManipulation()
	{
	return false;
	}

bool IscDatabaseMetaData::supportsSchemasInProcedureCalls()
	{
	return false;
	}

bool IscDatabaseMetaData::supportsSchemasInTableDefinitions()
	{
	return false;
	}

bool IscDatabaseMetaData::supportsSchemasInIndexDefinitions()
	{
	return false;
	}

bool IscDatabaseMetaData::supportsSchemasInPrivilegeDefinitions()
	{
	return false;
	}

bool IscDatabaseMetaData::supportsCatalogsInDataManipulation()
	{
	return false;
	}

bool IscDatabaseMetaData::supportsCatalogsInProcedureCalls()
	{
	return false;
	}

bool IscDatabaseMetaData::supportsCatalogsInTableDefinitions()
	{
	return false;
	}

bool IscDatabaseMetaData::supportsCatalogsInIndexDefinitions()
	{
	return false;
	}

bool IscDatabaseMetaData::supportsCatalogsInPrivilegeDefinitions()
	{
	return false;
	}

bool IscDatabaseMetaData::supportsPositionedDelete()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsPositionedUpdate()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsSelectForUpdate()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsStoredProcedures()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsSubqueriesInComparisons()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsSubqueriesInExists()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsSubqueriesInIns()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsSubqueriesInQuantifieds()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsCorrelatedSubqueries()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsUnion()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsUnionAll()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsOpenCursorsAcrossCommit()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsOpenCursorsAcrossRollback()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsOpenStatementsAcrossCommit()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsOpenStatementsAcrossRollback()
	{
	return true;
	}

int IscDatabaseMetaData::getMaxCharLiteralLength()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

int IscDatabaseMetaData::getMaxColumnNameLength()
	{
	return 31;
	}

int IscDatabaseMetaData::getMaxColumnsInGroupBy()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

int IscDatabaseMetaData::getMaxColumnsInIndex()
	{
	return 16;
	}

int IscDatabaseMetaData::getMaxColumnsInOrderBy()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

int IscDatabaseMetaData::getMaxColumnsInSelect()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

int IscDatabaseMetaData::getMaxColumnsInTable()
	{
	return 32767;
	}

int IscDatabaseMetaData::getMaxConnections()
	{
	return 32767;
	}

int IscDatabaseMetaData::getMaxCursorNameLength()
	{
	return 31;
	}

int IscDatabaseMetaData::getMaxIndexLength()
	{
	return 250;
	}

int IscDatabaseMetaData::getMaxSchemaNameLength()
	{
	return 0;
	}

int IscDatabaseMetaData::getMaxProcedureNameLength()
	{
	return 31;
	}

int IscDatabaseMetaData::getMaxCatalogNameLength()
	{
	return 0;
	}

int IscDatabaseMetaData::getMaxRowSize()
	{
	return 65535;
	}

bool IscDatabaseMetaData::doesMaxRowSizeIncludeBlobs()
	{
	return false;
	}

int IscDatabaseMetaData::getMaxStatementLength()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

int IscDatabaseMetaData::getMaxStatements()
	{
	return 65535;
	}

int IscDatabaseMetaData::getMaxTableNameLength()
	{
	return 31;
	}

int IscDatabaseMetaData::getMaxTablesInSelect()
	{
	return 128;
	}

int IscDatabaseMetaData::getMaxUserNameLength()
	{
	return 31;
	}

int IscDatabaseMetaData::getDefaultTransactionIsolation()
	{
    return TRANSACTION_READ_COMMITTED; 
	}

bool IscDatabaseMetaData::supportsTransactions()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsTransactionIsolationLevel(int level)
	{
	switch (level)
		{
		case TRANSACTION_READ_UNCOMMITTED:
			return false;

		/**
		 * Dirty reads are prevented; non-repeatable reads and phantom
		 * reads can occur.
		 */
		case TRANSACTION_READ_COMMITTED:
			return true;

		/**
		 * Dirty reads and non-repeatable reads are prevented; phantom
		 * reads can occur.     
		 */
		case TRANSACTION_REPEATABLE_READ:
			return true;

		/**
		 * Dirty reads, non-repeatable reads and phantom reads are prevented.
		 */
		case TRANSACTION_SERIALIZABLE:
			return true;
		}

	return false;
	}

bool IscDatabaseMetaData::supportsDataDefinitionAndDataManipulationTransactions()
	{
	return true;
	}

bool IscDatabaseMetaData::supportsDataManipulationTransactionsOnly()
	{
	return false;
	}

bool IscDatabaseMetaData::dataDefinitionCausesTransactionCommit()
	{
	return false;
	}

bool IscDatabaseMetaData::dataDefinitionIgnoredInTransactions()
	{
	return false;
	}

ResultSet* IscDatabaseMetaData::getProcedures(const char* catalog, const char* schemaPattern,
		const char* procedureNamePattern)
	{
	IscProceduresResultSet *resultSet = new IscProceduresResultSet (this);

	try
		{
		resultSet->getProcedures (catalog, schemaPattern, procedureNamePattern);
		}
	catch (...)
		{
		delete resultSet;
		throw;
		}

	return resultSet;
	}


ResultSet* IscDatabaseMetaData::getProcedureColumns(const char* catalog,
		const char* schemaPattern,
		const char* procedureNamePattern, 
		const char* columnNamePattern)
	{
	IscProcedureColumnsResultSet *resultSet = new IscProcedureColumnsResultSet (this);

	try
		{
		resultSet->getProcedureColumns (catalog, schemaPattern, procedureNamePattern, columnNamePattern);
		}
	catch (...)
		{
		delete resultSet;
		throw;
		}

	return resultSet;
	}


ResultSet* IscDatabaseMetaData::getSchemas()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

ResultSet* IscDatabaseMetaData::getCatalogs()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

ResultSet* IscDatabaseMetaData::getTableTypes()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

ResultSet* IscDatabaseMetaData::getColumnPrivileges(const char* catalog, const char* schema,
	const char* table, const char* columnNamePattern)
	{
	IscColumnPrivilegesResultSet *resultSet = new IscColumnPrivilegesResultSet (this);

	try
		{
		resultSet->getColumnPrivileges (catalog, schema, table, columnNamePattern);
		}
	catch (...)
		{
		delete resultSet;
		throw;
		}

	return resultSet;
	}


ResultSet* IscDatabaseMetaData::getTablePrivileges(const char* catalog, const char* schemaPattern,
            const char* tableNamePattern)
    {
    IscTablePrivilegesResultSet *resultSet = new IscTablePrivilegesResultSet (this);

    try
        {
        resultSet->getTablePrivileges (catalog, schemaPattern, tableNamePattern);
        }
    catch (...)
        {
        delete resultSet;
        throw;
        }

    return resultSet;
    }


ResultSet* IscDatabaseMetaData::getBestRowIdentifier(const char* catalog, const char* schema,
	const char* table, int scope, bool nullable)
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}


ResultSet* IscDatabaseMetaData::getVersionColumns(const char* catalog, const char* schema,
			const char* table)
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}


ResultSet* IscDatabaseMetaData::getExportedKeys(const char* catalog, const char* schema,
			const char* table)
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}


ResultSet* IscDatabaseMetaData::getCrossReference(
		const char* primaryCatalog, const char* primarySchema, const char* primaryTable,
		const char* foreignCatalog, const char* foreignSchema, const char* foreignTable
	)

	{
	IscCrossReferenceResultSet *resultSet = new IscCrossReferenceResultSet (this);

	try
		{
		resultSet->getCrossReference (primaryCatalog, primarySchema,primaryTable,foreignCatalog,foreignSchema, foreignTable);
		}
	catch (...)
		{
		delete resultSet;
		throw;
		}

	return resultSet;
	}

ResultSet* IscDatabaseMetaData::getTypeInfo(int dataType)
	{
	return new TypesResultSet(dataType);
	}

bool IscDatabaseMetaData::supportsResultSetConcurrency(int type, int concurrency)
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

bool IscDatabaseMetaData::ownUpdatesAreVisible(int type)
	{
	return true;
	}

bool IscDatabaseMetaData::ownDeletesAreVisible(int type)
	{
	return true;
	}

bool IscDatabaseMetaData::ownInsertsAreVisible(int type)
	{
	return true;
	}

bool IscDatabaseMetaData::othersUpdatesAreVisible(int type)
	{
	return false;
	}

bool IscDatabaseMetaData::othersDeletesAreVisible(int type)
	{
	return false;
	}

bool IscDatabaseMetaData::othersInsertsAreVisible(int type)
	{
	return false;
	}

bool IscDatabaseMetaData::updatesAreDetected(int type)
	{
	return false;
	}

bool IscDatabaseMetaData::deletesAreDetected(int type)
	{
	return false;
	}

bool IscDatabaseMetaData::insertsAreDetected(int type)
	{
	return false;
	}

bool IscDatabaseMetaData::supportsBatchUpdates()
	{
	return false;
	}

ResultSet* IscDatabaseMetaData::getUDTs(const char* catalog, const char* schemaPattern, 
		  const char* typeNamePattern, int* types)
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}


int IscDatabaseMetaData::objectVersion()
{
	return DATABASEMETADATA_VERSION;
}

bool IscDatabaseMetaData::supportsStatementMetaData()
{
	return true;
}
