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
 *	2003-03-24	OdbcConnection.cpp
 *				Contributed by Norbert Meyer
 *				delete Statements before close connection 
 *				(statement-destructor needs connection-pointer 
 *				for call connection->deleteStatement.  
 *				If connection->deleteStatement not called, 
 *				you get an AV if you use Statements and call 
 *				  SQLDisconnect(...); 
 *				  SQLFreeHandle(..., connection);
 *
 *	2002-12-05	OdbcConnection.cpp 
 *				Contributed by C. Guzman Alvarez
 *				SQLGetInfo returns more info.
 *				Solve error in SQL_ORDER_BY_COLUMNS_IN_SELECT.
 *
 *	2002-08-12	OdbcConnection.cpp 
 *				Contributed by C. G. Alvarez
 *				Added SQL_API_SQLGETCONNECTOPTION to list of 
 *				supported functions
 *
 *				Added more items to sqlGetInfo()
 *
 *  2002-07-02  OdbcConnection.cpp 
 *				Added better management of txn isolation
 *				Added fix to enable setting the asyncEnable property
 *				contributed by C. G. Alvarez
 *
 *  2002-07-01  OdbcConnection.cpp 
 *				Added SQL_API_SQLSETCONNECTOPTION to 
 *				supportedFunctions	C. G. Alvarez
 *
 *	2002-06-26	OdbcConnection::sqlGetInfo
 *				Added call to clearErrors() at start of 
 *				the method(Roger Gammans).
 *
 *	2002-06-25  OdbcConnection.cpp  
 *				Contributed by C. G. Alvarez
 *				Return Database Server Name from sqlGetInfo
 *
 *
 *	2002-06-08  OdbcConnection.cpp 
 *				Contributed by C. G. Alvarez
 *				sqlSetConnectAttr() and connect()
 *				now supports SQL_ATTR_TXN_ISOLATION
 *
 *
 *	2002-06-08	OdbcConnection.cpp
 *				Contributed by C. G. Alvarez
 *				Changed sqlDriverConnect() to better support
 *				Crystal Reports.
 */

// OdbcConnection.cpp: implementation of the OdbcConnection class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include "OdbcJdbc.h"
#include "OdbcConnection.h"
#include "OdbcEnv.h"
#include <odbcinst.h>
#include "SetupAttributes.h"
#include "Connection.h"
#include "SQLException.h"
#include "OdbcStatement.h"
#include "OdbcDesc.h"

#ifndef _WIN32
#define ELF
#endif

#ifdef ELF
#include <dlfcn.h>
#endif

#define DEFAULT_DRIVER		"IscDbc"

#define SQL_FBGETPAGEDB			180
#define SQL_FBGETWALDB			181
#define SQL_FBGETSTATINFODB		182

typedef Connection* (*ConnectFn)();

#define ODBC_DRIVER_VERSION	"03.00"
//#define ODBC_DRIVER_VERSION	SQL_SPEC_STRING
#define ODBC_VERSION_NUMBER	"03.00.0000"

static const int supportedFunctions [] = {
            // Deprecated but important stuff

            SQL_API_SQLALLOCCONNECT,
            SQL_API_SQLALLOCENV,
            SQL_API_SQLALLOCSTMT,
            SQL_API_SQLFREECONNECT,
            SQL_API_SQLFREEENV,
            SQL_API_SQLFREESTMT,
            SQL_API_SQLCOLATTRIBUTES,
            SQL_API_SQLERROR,
            SQL_API_SQLSETPARAM,
            SQL_API_SQLTRANSACT,
            SQL_API_SQLSETCONNECTOPTION,
            SQL_API_SQLGETCONNECTOPTION,

            SQL_API_SQLENDTRAN,

            SQL_API_SQLALLOCHANDLE,
            SQL_API_SQLGETDESCFIELD,
            SQL_API_SQLBINDCOL,
            SQL_API_SQLGETDESCREC,
            SQL_API_SQLCANCEL,
            SQL_API_SQLGETDIAGFIELD,
            SQL_API_SQLCLOSECURSOR,
            SQL_API_SQLGETDIAGREC,
            SQL_API_SQLCOLATTRIBUTE,
            SQL_API_SQLGETENVATTR,
            SQL_API_SQLCONNECT,
            SQL_API_SQLGETFUNCTIONS,
            SQL_API_SQLCOPYDESC,
            SQL_API_SQLGETINFO,
            SQL_API_SQLDATASOURCES,
            SQL_API_SQLGETSTMTATTR,
            SQL_API_SQLDESCRIBECOL,
            SQL_API_SQLGETTYPEINFO,
            SQL_API_SQLDISCONNECT,
            SQL_API_SQLNUMRESULTCOLS,
            SQL_API_SQLDRIVERS,
            SQL_API_SQLPARAMDATA,
            SQL_API_SQLPREPARE,
            SQL_API_SQLEXECDIRECT,
            SQL_API_SQLPUTDATA,
            SQL_API_SQLEXECUTE,
            SQL_API_SQLROWCOUNT,
            SQL_API_SQLFETCH,
            SQL_API_SQLSETCONNECTATTR,
            SQL_API_SQLFETCHSCROLL,
            SQL_API_SQLSETCURSORNAME,
            SQL_API_SQLFREEHANDLE,
            SQL_API_SQLSETDESCFIELD,
            SQL_API_SQLFREESTMT,
            SQL_API_SQLSETDESCREC,
            SQL_API_SQLGETCONNECTATTR,
            SQL_API_SQLSETENVATTR,
            SQL_API_SQLGETCURSORNAME,
            SQL_API_SQLSETSTMTATTR,
            SQL_API_SQLGETDATA,

			SQL_API_SQLEXTENDEDFETCH,	//Deprecated ODBC 1.0 call. Replaced by SQL_API_SQLFETCHSCROLL in ODBC 3.0

            // The following is a list of valid values for FunctionId for functions conforming to the X/Open standards - compliance level,,

            SQL_API_SQLCOLUMNS,
            SQL_API_SQLSTATISTICS,
            SQL_API_SQLSPECIALCOLUMNS,
            SQL_API_SQLTABLES,

            //The following is a list of valid values for FunctionId for functions conforming to the ODBC standards - compliance level,,
            SQL_API_SQLBINDPARAMETER,
            SQL_API_SQLNATIVESQL,
            SQL_API_SQLBROWSECONNECT,
            SQL_API_SQLNUMPARAMS,
            SQL_API_SQLBULKOPERATIONS,
            SQL_API_SQLPRIMARYKEYS,
            SQL_API_SQLCOLUMNPRIVILEGES,
            SQL_API_SQLPROCEDURECOLUMNS,
            SQL_API_SQLDESCRIBEPARAM,
            SQL_API_SQLPROCEDURES,
            SQL_API_SQLDRIVERCONNECT,
            SQL_API_SQLSETPOS,
            SQL_API_SQLFOREIGNKEYS,
            SQL_API_SQLTABLEPRIVILEGES,
            SQL_API_SQLMORERESULTS,
        };

enum InfoType { infoString, infoShort, infoLong, infoUnsupported };
struct TblInfoItem
{
	int			item;
	const char	*name;
	InfoType	type;
	const char	*value;
};

#define CITEM(item,value)	{item, #item, infoString, value},
#define SITEM(item,value)	{item, #item, infoShort, (char*) value},
#define NITEM(item,value)	{item, #item, infoLong, (char*) value},
#define UITEM(item,value)	{item, #item, infoUnsupported, (char*) value},

static const TblInfoItem tblInfoItems [] = {
#include "InfoItems.h"
            {0, 0, infoShort, 0}
        };

struct InfoItem
{
	const char	*name;
	InfoType	type;
	const char	*value;
};

#define INFO_SLOT(n)		((n < 10000) ? n : n - 10000 + 200)
#define INFO_SLOTS			400

static SQLUSMALLINT functionsArray [100];
static SQLSMALLINT  functionsBitmap [SQL_API_ODBC3_ALL_FUNCTIONS_SIZE];
static bool moduleInit();
static InfoItem infoItems [INFO_SLOTS];
static bool foo = moduleInit();

/***
#define SQL_FUNC_EXISTS(pfExists, uwAPI) \
				((*(((UWORD*) (pfExists)) + ((uwAPI) >> 4)) \
					& (1 << ((uwAPI) & 0x000F)) \
 				 ) ? SQL_TRUE : SQL_FALSE \
***/

bool moduleInit()
{
	for (unsigned int n = 0; n < sizeof (supportedFunctions) / sizeof (supportedFunctions [0]); ++n)
	{
		int fn = supportedFunctions [n];
		functionsArray [fn] = SQL_TRUE;
		ASSERT ((fn >> 4) < SQL_API_ODBC3_ALL_FUNCTIONS_SIZE);
		functionsBitmap [fn >> 4] |= 1 << (fn & 0xf);
	}

	for (const TblInfoItem *t = tblInfoItems; t->name; ++t)
	{
		int slot = INFO_SLOT (t->item);
		ASSERT (slot >= 0 && slot < INFO_SLOTS);
		InfoItem *item = infoItems + slot;
		ASSERT (item->name == NULL);
		item->name = t->name;
		item->type = t->type;
		item->value = t->value;
	}

	bool test = SQL_FUNC_EXISTS (functionsBitmap, SQL_API_SQLALLOCHANDLE);

	return test;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

OdbcConnection::OdbcConnection(OdbcEnv *parent)
{
	env					= parent;
	connected			= false;
	connectionTimeout	= 0;
	connection			= NULL;
	statements			= NULL;
	descriptors			= NULL;
	libraryHandle		= NULL;
	asyncEnabled		= SQL_ASYNC_ENABLE_OFF;
	autoCommit			= true;
	cursors				= SQL_CUR_USE_DRIVER; //Org
	statementNumber		= 0;
	accessMode			= SQL_MODE_READ_WRITE;
	transactionIsolation = SQL_TXN_READ_COMMITTED; //suggested by CGA.
	optTpb				= 0;
}

OdbcConnection::~OdbcConnection()
{
	if (connection)
		connection->close();

	if (env)
		env->connectionClosed (this);

// NOMEY begin
//	while (statements)
//		delete statements;

//	while (descriptors)
//		delete descriptors;

	while (statements)
	{
		OdbcStatement* statement = statements;
		statements = (OdbcStatement*)statement->next;
		delete statement;
	}
	
	while (descriptors)
	{
		OdbcDesc* descriptor = descriptors;
		descriptors = (OdbcDesc*)descriptor->next;
		delete descriptor;
	}
// NOMEY end



#ifdef ELF
	if (libraryHandle)
		dlclose (libraryHandle);
#endif
}

OdbcObjectType OdbcConnection::getType()
{
	return odbcTypeConnection;
}

RETCODE OdbcConnection::sqlSetConnectAttr (SQLINTEGER attribute, SQLPOINTER value, SQLINTEGER stringLength)
{
	clearErrors();

	switch (attribute)
	{
	case SQL_ATTR_LOGIN_TIMEOUT:
		connectionTimeout = (int) value;
		break;

	case SQL_ATTR_AUTOCOMMIT:
		autoCommit = (int) value == SQL_AUTOCOMMIT_ON;
		if (connection)
			connection->setAutoCommit (autoCommit);
		break;

	case SQL_ATTR_ODBC_CURSORS:
		cursors = (int) value;
		break;

		//Added by CA
	case SQL_ATTR_TXN_ISOLATION:
		transactionIsolation = (int)value;
		if( connection )
			connection->setTransactionIsolation( (int) value );
		break;

		//Added by CA
	case SQL_ATTR_ASYNC_ENABLE:
		asyncEnabled = (int) value;
		break;

	case SQL_ATTR_ACCESS_MODE:
		accessMode = (int)value;
		break;
	}

	return sqlSuccess();
}

RETCODE OdbcConnection::sqlDriverConnect(SQLHWND hWnd, const SQLCHAR * connectString, int connectStringLength, SQLCHAR * outConnectBuffer, int connectBufferLength, SQLSMALLINT * outStringLength, int driverCompletion)
{
	clearErrors();

	if (connected)
		return sqlReturn (SQL_ERROR, "08002", "Connection name is use");

	if (connectStringLength < 0 && connectStringLength != SQL_NTS)
		return sqlReturn (SQL_ERROR, "HY090", "Invalid string or buffer length");

	switch (driverCompletion)
	{
	case SQL_DRIVER_COMPLETE:
	case SQL_DRIVER_COMPLETE_REQUIRED:
		break;

	case SQL_DRIVER_PROMPT:
		if (hWnd == NULL)
			return sqlReturn (SQL_ERROR, "HY092", "Invalid attribute/option identifier");
		break;
	}

	int length = stringLength (connectString, connectStringLength);
	const char *end = (const char*) connectString + length;
	JString driver = DRIVER_NAME;

	for (const char *p = (const char*) connectString; p < end;)
	{
		char name [256];
		char value [256];
		char *q = name;
		char c;
		while (p < end && (c = *p++) != '=' && c != ';')
			*q++ = c;
		*q = 0;
		q = value;
		if (c == '=')
			while (p < end && (c = *p++) != ';')
				*q++ = c;
		*q = 0;
		if (!strcmp (name, "DSN"))
			dsn = value;
		else if (!strcmp (name, "DBNAME"))
			databaseName = value;
		else if (!strcmp (name, "UID"))
			account = value;
		else if (!strcmp (name, "PWD"))
			password = value;
		else if (!strcmp (name, "ROLE"))
			role = value;
		else if (!strcmp (name, "DRIVER"))
			driver = value;
		else if (!strcmp (name, "ODBC"))
			;
		else
			postError ("01S00", "Invalid connection string attribute");
	}

	expandConnectParameters();

	char returnString [1024], *r = returnString;

	/* Removed at suggestion of CGA
		r = appendString (r, "DRIVER=");
		r = appendString (r, driver);

		if (!dsn.IsEmpty())
			{
			r = appendString (r, ";DSN=");
			r = appendString (r, dsn);
			}

		if (!account.IsEmpty())
			{
			r = appendString (r, ";UID=");
			r = appendString (r, account);
			}

		if (!password.IsEmpty())
			{
			r = appendString (r, ";PWD=");
			r = appendString (r, password);
			}

		if (!role.IsEmpty())
			{
			r = appendString (r, ";ROLE=");
			r = appendString (r, role);
			}
	*/
	//Block suggested by CGA
	r = appendString (r, "DSN=");
	r = appendString (r, dsn);

	if (!driver.IsEmpty())
	{
		r = appendString (r, ";DRIVER=");
		r = appendString (r, driver);
	}

	if (!databaseName.IsEmpty())
	{
		r = appendString (r, ";DBNAME=");
		r = appendString (r, databaseName);
	}

	if (!account.IsEmpty())
	{
		r = appendString (r, ";UID=");
		r = appendString (r, account);
	}

	if (!password.IsEmpty())
	{
		r = appendString (r, ";PWD=");
		r = appendString (r, password);
	}

	if (!role.IsEmpty())
	{
		r = appendString (r, ";ROLE=");
		r = appendString (r, role);
	}

	if (setString ((UCHAR*) returnString, r - returnString, outConnectBuffer, connectBufferLength, outStringLength))
		postError ("01004", "String data, right truncated");

	RETCODE ret = connect (jdbcDriver, databaseName, account, password, role);

	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
		return ret;

	return sqlSuccess();
}

JString OdbcConnection::readAttribute(const char * attribute)
{
	char buffer [256];

	int ret = SQLGetPrivateProfileString (dsn, attribute, "", buffer, sizeof (buffer), env->odbcIniFileName);

	return JString (buffer, ret);
}

RETCODE OdbcConnection::sqlGetFunctions(SQLUSMALLINT functionId, SQLUSMALLINT * supportedPtr)
{
	clearErrors();

	switch (functionId)
	{
	case SQL_API_ODBC3_ALL_FUNCTIONS:
		memcpy (supportedPtr, functionsBitmap, sizeof (functionsBitmap));
		//memset (supportedPtr, -1, sizeof (functionsBitmap));
		return sqlSuccess();

	case SQL_API_ALL_FUNCTIONS:
		memcpy (supportedPtr, functionsArray, sizeof (functionsArray));
		return sqlSuccess();
	}

	/***
	if (functionId >= 0 && functionId < SQL_API_ODBC3_ALL_FUNCTIONS_SIZE  * 16)
		return sqlReturn (SQL_ERROR, "HY095", "Function type out of range");
	***/

	*supportedPtr = (SQL_FUNC_EXISTS (functionsBitmap, functionId)) ? SQL_TRUE : SQL_FALSE;

	return sqlSuccess();
}

RETCODE OdbcConnection::sqlDisconnect()
{
	if (!connected)
		sqlReturn (SQL_ERROR, "08003", "Connection does not exist");

	if (connection->getTransactionPending())
		sqlReturn (SQL_ERROR, "25000", "Invalid transaction state");

	try
	{
		connection->commit();
		connection->close();
		connection = NULL;
		connected = false;
	}
	catch (SQLException& exception)
	{
		postError ("01002", exception);
		connection = NULL;
		connected = false;
		return SQL_SUCCESS_WITH_INFO;
	}

	return sqlSuccess();
}

RETCODE OdbcConnection::sqlGetInfo(UWORD type, PTR ptr, int maxLength, SWORD * actualLength)
{

	clearErrors();

#ifdef DEBUG
	char temp [256];
#endif
	int slot = INFO_SLOT (type);
	InfoItem *item = infoItems + slot;
	int n;

	if (slot < 0 || slot >= INFO_SLOTS || item->name == NULL)
		return sqlReturn (SQL_ERROR, "HY096", "Information type out of range");

	const char *string = item->value;
	SQLUINTEGER value = (SQLUINTEGER) item->value;
	DatabaseMetaData *metaData = NULL;

	if (connection)
		metaData = connection->getMetaData();
	else
		switch (type)
		{
		case SQL_ODBC_VER:
		case SQL_DRIVER_ODBC_VER:
		case SQL_ODBC_API_CONFORMANCE:
			break;

		default:
			return sqlReturn (SQL_ERROR, "08003", "Connection does not exist");
		}

	switch (type)
	{
//////////////////////////////////???
//////////////////////////////////???
//////////////////////////////////???
	case SQL_FBGETPAGEDB:
		metaData->getSqlStrPageSizeBd(ptr,maxLength,actualLength);
		return SQL_SUCCESS;
	case SQL_FBGETWALDB:
		metaData->getSqlStrWalInfoBd(ptr,maxLength,actualLength);
		return SQL_SUCCESS;
	case SQL_FBGETSTATINFODB:
		metaData->getStrStatInfoBd(ptr,maxLength,actualLength);
		return SQL_SUCCESS;
//////////////////////////////////???
//////////////////////////////////???
//////////////////////////////////???
	case SQL_CURSOR_COMMIT_BEHAVIOR:
		if (metaData->supportsOpenCursorsAcrossCommit())
			value = SQL_CB_PRESERVE;
		else if (metaData->supportsOpenStatementsAcrossCommit())
			value = SQL_CB_CLOSE;
		else
			value = SQL_CB_DELETE;
		break;

	case SQL_CURSOR_ROLLBACK_BEHAVIOR:
		if (metaData->supportsOpenCursorsAcrossRollback())
			value = SQL_CB_PRESERVE;
		else if (metaData->supportsOpenStatementsAcrossRollback())
			value = SQL_CB_CLOSE;
		else
			value = SQL_CB_DELETE;
		break;

	case SQL_IDENTIFIER_QUOTE_CHAR:
		string = metaData->getIdentifierQuoteString();
		break;

	case SQL_DATABASE_NAME:
		string = databaseName;
		break;

	case SQL_CATALOG_LOCATION:
		value = (metaData->isCatalogAtStart()) ? SQL_CL_START : SQL_CL_END;
		break;

	case SQL_CORRELATION_NAME:
		value = (metaData->supportsTableCorrelationNames()) ? SQL_CL_START : SQL_CL_END;
		break;

	case SQL_COLUMN_ALIAS:
		string = (metaData->supportsColumnAliasing()) ? "Y" : "N";
		break;

	case SQL_CATALOG_NAME:
		string = metaData->getCatalogTerm();
		string = (string [0]) ? "Y" : "N";
		break;

	case SQL_CATALOG_TERM:
		string = metaData->getCatalogTerm();
		break;

	case SQL_CATALOG_NAME_SEPARATOR:
		string = metaData->getCatalogSeparator();
		break;

	case SQL_CATALOG_USAGE:
		if (metaData->supportsCatalogsInDataManipulation())
			value |= SQL_CU_DML_STATEMENTS;
		if (metaData->supportsCatalogsInTableDefinitions())
			value |= SQL_CU_TABLE_DEFINITION;
		if (metaData->supportsCatalogsInIndexDefinitions())
			value |= SQL_CU_INDEX_DEFINITION;
		if (metaData->supportsCatalogsInPrivilegeDefinitions())
			value |= SQL_CU_PRIVILEGE_DEFINITION;
		if (metaData->supportsCatalogsInProcedureCalls())
			value |= SQL_CU_PROCEDURE_INVOCATION;
		break;

	case SQL_SCHEMA_USAGE:
		if (metaData->supportsSchemasInDataManipulation())
			value |= SQL_SU_DML_STATEMENTS;
		if (metaData->supportsSchemasInProcedureCalls())
			value |= SQL_SU_PROCEDURE_INVOCATION;
		if (metaData->supportsSchemasInTableDefinitions())
			value |= SQL_SU_TABLE_DEFINITION;
		if (metaData->supportsCatalogsInIndexDefinitions())
			value |= SQL_SU_INDEX_DEFINITION;
		if (metaData->supportsSchemasInPrivilegeDefinitions())
			value |= SQL_SU_PRIVILEGE_DEFINITION;
		break;

		//Added by CGA. 2002-06-25
	case SQL_SERVER_NAME:
		string = metaData->getDatabaseServerName();
		break;

	case SQL_DATA_SOURCE_NAME:
		string = dsn;
		break;

	case SQL_DATA_SOURCE_READ_ONLY:
		string = (metaData->isReadOnly()) ? "Y" : "N";
		break;

	case SQL_CURSOR_SENSITIVITY:
		{
			if ( metaData->supportsANSI92EntryLevelSQL() )
				value = SQL_UNSPECIFIED;	// Entry level SQL-92 compliant.
			if ( metaData->supportsANSI92FullSQL() )
				value = SQL_INSENSITIVE;	// Full level SQL-92 compliant.
		}
		break;

	case SQL_DBMS_NAME:
		string = metaData->getDatabaseProductName();
		break;

	case SQL_DBMS_VER:
		string = metaData->getDatabaseProductVersion();
		break;

	case SQL_DEFAULT_TXN_ISOLATION:
		value = metaData->getDefaultTransactionIsolation();
		break;

	case SQL_DESCRIBE_PARAMETER:
		string = (metaData->supportsStatementMetaData()) ? "Y" : "N";
		break;

	case SQL_DRIVER_HDBC:
		value = (SQLUINTEGER) this;
		break;

	case SQL_DRIVER_HENV:
		value = (SQLUINTEGER) env;
		break;

	case SQL_USER_NAME:
		string = account;
		break;

	case SQL_SCHEMA_TERM:
		string = metaData->getSchemaTerm();
		break;

	case SQL_SQL_CONFORMANCE:
		{
			if ( metaData->supportsANSI92EntryLevelSQL() )
				value = SQL_SC_SQL92_ENTRY;	// Entry level SQL-92 compliant.
			if ( metaData->supportsANSI92IntermediateSQL() )
				value = SQL_SC_SQL92_INTERMEDIATE; // Intermediate level SQL-92 compliant.
			if ( metaData->supportsANSI92FullSQL() )
				value = SQL_SC_SQL92_FULL; // Full level SQL-92 compliant.
		}
		break;

	case SQL_ODBC_SQL_CONFORMANCE:
		{
			if ( metaData->supportsMinimumSQLGrammar() )
				value = SQL_OSC_MINIMUM;
			if ( metaData->supportsCoreSQLGrammar() )
				value = SQL_OSC_CORE;
			if ( metaData->supportsExtendedSQLGrammar() )
				value = SQL_OSC_EXTENDED;
		}
		break;

	case SQL_KEYWORDS:
		string = metaData->getSQLKeywords();
		break;

	case SQL_SPECIAL_CHARACTERS:
		string = metaData->getExtraNameCharacters();
		break;

	case SQL_LIKE_ESCAPE_CLAUSE:
		string = metaData->supportsLikeEscapeClause() ? "Y" : "N";
		break;

	case SQL_DATETIME_LITERALS:
		value = SQL_DL_SQL92_DATE |
		        SQL_DL_SQL92_TIME |
		        SQL_DL_SQL92_TIMESTAMP;
		break;

	case SQL_CREATE_DOMAIN:
		{
			value = 0;
			if ( metaData->supportsMinimumSQLGrammar() )
				value |= 0;
			if ( metaData->supportsCoreSQLGrammar() )
				value = SQL_CDO_CREATE_DOMAIN	|
				        SQL_CDO_DEFAULT			|
				        SQL_CDO_CONSTRAINT		|
				        SQL_CDO_COLLATION;
			if ( metaData->supportsANSI92FullSQL() )
				value |= 0;
		}
		break;

	case SQL_CREATE_TABLE:
		{
			value = 0;
			if ( metaData->supportsMinimumSQLGrammar() )
				value |= SQL_CT_CREATE_TABLE		|
				         SQL_CT_COLUMN_CONSTRAINT	|
				         SQL_CT_COLUMN_DEFAULT		|
				         SQL_CT_TABLE_CONSTRAINT;
			if ( metaData->supportsCoreSQLGrammar() )
				value |= SQL_CT_CONSTRAINT_NAME_DEFINITION;
			if ( metaData->supportsExtendedSQLGrammar() )
				value |= SQL_CT_COLUMN_COLLATION;
		}
		break;

	case SQL_FILE_USAGE:
		{
			value =  SQL_FILE_NOT_SUPPORTED;
			if ( metaData->usesLocalFiles() )
				value = SQL_FILE_TABLE;
			if ( metaData->usesLocalFilePerTable() )
				value = SQL_FILE_CATALOG;
		}
		break;

	case SQL_TXN_ISOLATION_OPTION:
		for (n = SQL_TXN_READ_UNCOMMITTED; n <= SQL_TXN_SERIALIZABLE; n *= 2)
			if (metaData->supportsTransactionIsolationLevel (n))
				value |= n;
		break;

	case SQL_MAX_DRIVER_CONNECTIONS:
		value = metaData->getMaxConnections();
		break;

	case SQL_MAX_COLUMN_NAME_LEN:
		value = metaData->getMaxColumnNameLength();
		break;

	case SQL_MAX_TABLE_NAME_LEN:
		value = metaData->getMaxTableNameLength();
		break;

	case SQL_MAX_CURSOR_NAME_LEN:
		value = metaData->getMaxCursorNameLength();
		break;

	case SQL_MAX_USER_NAME_LEN:
		value = metaData->getMaxUserNameLength();
		break;

	case SQL_MAX_COLUMNS_IN_INDEX:
		value = metaData->getMaxColumnsInIndex();
		break;

	case SQL_MAX_COLUMNS_IN_TABLE:
		value = metaData->getMaxColumnsInTable();
		break;

	case SQL_MAX_INDEX_SIZE:
		value = metaData->getMaxIndexLength();
		break;

	case SQL_MAX_SCHEMA_NAME_LEN:
		value = metaData->getMaxSchemaNameLength();
		break;

	case SQL_MAX_CATALOG_NAME_LEN:
		value = metaData->getMaxCatalogNameLength();
		break;

	case SQL_MAX_ROW_SIZE:
		value = metaData->getMaxRowSize();
		break;

	case SQL_MAX_ROW_SIZE_INCLUDES_LONG:
		string = metaData->doesMaxRowSizeIncludeBlobs() ? "Y" : "N";
		break;

	case SQL_MAX_TABLES_IN_SELECT:
		value = metaData->getMaxTablesInSelect();
		break;

	case SQL_GROUP_BY:
		if ( metaData->supportsGroupBy() )
			value = SQL_GB_GROUP_BY_CONTAINS_SELECT;
		else
			value = SQL_GB_NOT_SUPPORTED;
		break;

	case SQL_SEARCH_PATTERN_ESCAPE:
		if ( metaData->supportsLikeEscapeClause() )
			string = metaData->getSearchStringEscape();
		else
			string = "";
			break;

	case SQL_PROCEDURES:
		string = metaData->supportsStoredProcedures() ? "Y" : "N";
		break;

	case SQL_MAX_PROCEDURE_NAME_LEN:
		value = metaData->getMaxProcedureNameLength();
		break;

	case SQL_SUBQUERIES:
		{
			if ( metaData->supportsSubqueriesInComparisons() )
				value |= SQL_SQ_COMPARISON;
			if ( metaData->supportsSubqueriesInExists() )
				value |= SQL_SQ_EXISTS;
			if ( metaData->supportsSubqueriesInIns() )
				value |= SQL_SQ_IN;
			if ( metaData->supportsSubqueriesInQuantifieds() )
				value |= SQL_SQ_QUANTIFIED;
			if ( metaData->supportsCorrelatedSubqueries() )
				value |= SQL_SQ_CORRELATED_SUBQUERIES;
		}
		break;

	case SQL_UNION:
		{
			if ( metaData->supportsUnion() )
				value |= SQL_U_UNION;
			if ( metaData->supportsUnionAll() )
				value |= SQL_U_UNION_ALL;
		}
		break;

	case SQL_STRING_FUNCTIONS:
		value = SQL_FN_STR_UCASE |
		        SQL_FN_STR_SUBSTRING;
		break;

	case SQL_NUMERIC_FUNCTIONS:
		value = 0;
		break;

	case SQL_TIMEDATE_FUNCTIONS:
		value |= SQL_FN_TD_EXTRACT;
		break;

	case SQL_SYSTEM_FUNCTIONS:
		value = SQL_FN_SYS_USERNAME |
		        SQL_FN_SYS_DBNAME;
		break;

	case SQL_EXPRESSIONS_IN_ORDERBY:
		string = metaData->supportsExpressionsInOrderBy() ? "Y" : "N";
		break;

	case SQL_ORDER_BY_COLUMNS_IN_SELECT:
		string = metaData->supportsOrderByUnrelated() ? "Y" : "N";
		break;

	case SQL_CONVERT_FUNCTIONS:
		value |= SQL_FN_CVT_CAST;
		break;

	case SQL_CONVERT_NUMERIC:
		if ( metaData->supportsConvert() )
		{
			if ( metaData->supportsConvert(SQL_NUMERIC, SQL_CHAR) )
				value |= SQL_CVT_CHAR;
			if ( metaData->supportsConvert(SQL_NUMERIC, SQL_VARCHAR) )
				value |= SQL_CVT_VARCHAR;
			if ( metaData->supportsConvert(SQL_NUMERIC, SQL_TYPE_DATE) )
				value |= SQL_CVT_DATE;
			if ( metaData->supportsConvert(SQL_NUMERIC, SQL_TYPE_TIME) )
				value |= SQL_CVT_TIME;
			if ( metaData->supportsConvert(SQL_NUMERIC, SQL_TYPE_TIMESTAMP) )
				value |= SQL_CVT_TIMESTAMP;
		}
		break;

	case SQL_CONVERT_DATE:
		if ( metaData->supportsConvert() )
		{
			if ( metaData->supportsConvert(SQL_TYPE_DATE, SQL_CHAR) )
				value |= SQL_CVT_CHAR;
			if ( metaData->supportsConvert(SQL_TYPE_DATE, SQL_VARCHAR) )
				value |= SQL_CVT_VARCHAR;
			if ( metaData->supportsConvert(SQL_TYPE_DATE, SQL_TYPE_TIMESTAMP) )
				value |= SQL_CVT_TIMESTAMP;
		}
		break;

	case SQL_CONVERT_TIME:
		if ( metaData->supportsConvert() )
		{
			if ( metaData->supportsConvert(SQL_TYPE_TIME, SQL_CHAR) )
				value |= SQL_CVT_CHAR;
			if ( metaData->supportsConvert(SQL_TYPE_TIME, SQL_VARCHAR) )
				value |= SQL_CVT_VARCHAR;
			if ( metaData->supportsConvert(SQL_TYPE_TIME, SQL_TYPE_TIMESTAMP) )
				value |= SQL_CVT_TIMESTAMP;
		}
		break;

	case SQL_CONVERT_CHAR:
	case SQL_CONVERT_VARCHAR:
		if ( metaData->supportsConvert() )
		{
			if ( metaData->supportsConvert(SQL_CHAR, SQL_NUMERIC) )
				value |= SQL_CVT_NUMERIC;
			if ( metaData->supportsConvert(SQL_CHAR, SQL_TYPE_DATE) )
				value |= SQL_CVT_DATE;
			if ( metaData->supportsConvert(SQL_CHAR, SQL_TYPE_TIME) )
				value |= SQL_CVT_TIME;
			if ( metaData->supportsConvert(SQL_CHAR, SQL_TYPE_TIMESTAMP) )
				value |= SQL_CVT_TIMESTAMP;
		}
		break;

	case SQL_CONVERT_TIMESTAMP:
		if ( metaData->supportsConvert() )
		{
			if ( metaData->supportsConvert(SQL_TYPE_TIMESTAMP, SQL_CHAR) )
				value |= SQL_CVT_CHAR;
			if ( metaData->supportsConvert(SQL_TYPE_TIMESTAMP, SQL_VARCHAR) )
				value |= SQL_CVT_VARCHAR;
			if ( metaData->supportsConvert(SQL_TYPE_TIMESTAMP, SQL_TYPE_DATE) )
				value |= SQL_CVT_DATE;
			if ( metaData->supportsConvert(SQL_TYPE_TIMESTAMP, SQL_TYPE_TIME) )
				value |= SQL_CVT_TIME;
		}
		break;

	case SQL_IDENTIFIER_CASE:
		if (metaData->storesUpperCaseIdentifiers())
			value = SQL_IC_UPPER;
		else if (metaData->storesLowerCaseIdentifiers())
			value = SQL_IC_LOWER;
		else if (metaData->storesMixedCaseIdentifiers())
			value = SQL_IC_MIXED;
		else
			value = SQL_IC_SENSITIVE;
		break;

	case SQL_QUOTED_IDENTIFIER_CASE:
		if (metaData->storesUpperCaseQuotedIdentifiers())
			value = SQL_IC_UPPER;
		else if (metaData->storesLowerCaseQuotedIdentifiers())
			value = SQL_IC_LOWER;
		else if (metaData->storesMixedCaseQuotedIdentifiers())
			value = SQL_IC_MIXED;
		else
			value = SQL_IC_SENSITIVE;
		break;

	case SQL_NON_NULLABLE_COLUMNS:
		if(metaData->supportsNonNullableColumns())
			value = SQL_NNC_NON_NULL;
		break;

	case SQL_NULL_COLLATION:
		value = 0;
		if(metaData->nullsAreSortedHigh())
			value |= SQL_NC_HIGH;
		if(metaData->nullsAreSortedLow())
			value |= SQL_NC_LOW;
		if(metaData->nullsAreSortedAtStart())
			value |= SQL_NC_START;
		if(metaData->nullsAreSortedAtEnd())
			value |= SQL_NC_END;
		break;

	case SQL_DYNAMIC_CURSOR_ATTRIBUTES1:
		break;

	case SQL_DYNAMIC_CURSOR_ATTRIBUTES2:
		break;

	case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES1:
		break;

	case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2:
		break;

	case SQL_STATIC_CURSOR_ATTRIBUTES1:
		break;

	case SQL_STATIC_CURSOR_ATTRIBUTES2:
		break;

	case SQL_PROCEDURE_TERM:
		string = metaData->getProcedureTerm();
		break;

	case SQL_OUTER_JOINS:
		if (metaData->supportsFullOuterJoins())
			string = "F";
		else if (metaData->supportsLimitedOuterJoins())
			string = "P";
		else if (metaData->supportsOuterJoins())
			string = "Y";
		else
			string = "N";
		break;

	case SQL_OJ_CAPABILITIES:
		if (metaData->supportsFullOuterJoins())
			value = SQL_OJ_LEFT | SQL_OJ_RIGHT | SQL_OJ_FULL | SQL_OJ_NESTED |
			        SQL_OJ_NOT_ORDERED | SQL_OJ_INNER | SQL_OJ_ALL_COMPARISON_OPS;
		else if (metaData->supportsLimitedOuterJoins())
			value = 0;
		else if (metaData->supportsOuterJoins())
			value = 0;
		else
			value = 0;
		break;

	case SQL_STANDARD_CLI_CONFORMANCE:
		value = SQL_SCC_ISO92_CLI;
		break;

	case SQL_SQL92_PREDICATES:
		{
			value = 0;
			if ( metaData->supportsANSI92EntryLevelSQL() )
			{
				value |= SQL_SP_BETWEEN    |
				         SQL_SP_EXISTS    |
				         SQL_SP_IN        |
				         SQL_SP_ISNOTNULL|
				         SQL_SP_ISNULL    |
				         SQL_SP_LIKE;
				// SQL_SP_QUANTIFIED_COMPARISON
				// SQL_SP_UNIQUE
			}
			if ( metaData->supportsANSI92IntermediateSQL() )
				value |= SQL_SP_OVERLAPS;
			if ( metaData->supportsANSI92FullSQL() )
				value |= SQL_SP_MATCH_FULL            |
				         SQL_SP_MATCH_PARTIAL        |
				         SQL_SP_MATCH_UNIQUE_FULL    |
				         SQL_SP_MATCH_UNIQUE_PARTIAL    |
				         SQL_SP_ISNULL    |
				         SQL_SP_LIKE;
		}
		break;


	case SQL_SQL92_GRANT:
		{
			value = 0;
			if ( metaData->supportsANSI92EntryLevelSQL() )
			{
				value |= SQL_SG_DELETE_TABLE    |
				         SQL_SG_INSERT_TABLE    |
				         SQL_SG_REFERENCES_COLUMN |
				         SQL_SG_REFERENCES_TABLE    |
				         SQL_SG_SELECT_TABLE    |
				         SQL_SG_UPDATE_COLUMN    |
				         SQL_SG_UPDATE_TABLE    |
				         SQL_SG_WITH_GRANT_OPTION ;
			}
			if ( metaData->supportsANSI92IntermediateSQL() )
				value |= SQL_SG_USAGE_ON_DOMAIN    |
				         SQL_SG_USAGE_ON_CHARACTER_SET |
				         SQL_SG_USAGE_ON_COLLATION |
				         SQL_SG_USAGE_ON_TRANSLATION;
			// if ( metaData->supportsANSI92FullSQL() )
		}
		break;

	case SQL_SQL92_REVOKE:
		{
			value = 0;
			if ( metaData->supportsANSI92EntryLevelSQL() )
				value |= SQL_SR_DELETE_TABLE    |
				         SQL_SR_INSERT_TABLE    |
				         SQL_SR_REFERENCES_COLUMN |
				         SQL_SR_REFERENCES_TABLE    |
				         SQL_SR_SELECT_TABLE    |
				         SQL_SR_UPDATE_COLUMN    |
				         SQL_SR_UPDATE_TABLE;
			if ( metaData->supportsANSI92IntermediateSQL() )
				value |= SQL_SR_CASCADE            |
				         SQL_SR_GRANT_OPTION_FOR |
				         SQL_SR_INSERT_COLUMN    |
				         SQL_SR_RESTRICT        |
				         SQL_SR_USAGE_ON_DOMAIN    |
				         SQL_SR_USAGE_ON_CHARACTER_SET |
				         SQL_SR_USAGE_ON_COLLATION |
				         SQL_SR_USAGE_ON_TRANSLATION;
			// if ( metaData->supportsANSI92FullSQL() )
		}
		break;

	case SQL_SQL92_DATETIME_FUNCTIONS:
		value |= SQL_SDF_CURRENT_DATE |
		         SQL_SDF_CURRENT_TIME |
		         SQL_SDF_CURRENT_TIMESTAMP;
		break;


	case SQL_SQL92_FOREIGN_KEY_DELETE_RULE:
		value |= SQL_SFKD_CASCADE    |
		         SQL_SFKD_NO_ACTION |
		         SQL_SFKD_SET_DEFAULT |
		         SQL_SFKD_SET_NULL;
		break;

	case SQL_SQL92_FOREIGN_KEY_UPDATE_RULE:
		value |= SQL_SFKU_CASCADE    |
		         SQL_SFKU_NO_ACTION |
		         SQL_SFKU_SET_DEFAULT |
		         SQL_SFKU_SET_NULL;
		break;

	case SQL_SQL92_RELATIONAL_JOIN_OPERATORS:
		{
			value = 0;
			if ( metaData->supportsANSI92EntryLevelSQL() )
				value |= SQL_SRJO_LEFT_OUTER_JOIN |    // (FIPS Transitional level)
				         SQL_SRJO_NATURAL_JOIN    |    // (FIPS Transitional level)
				         SQL_SRJO_INNER_JOIN    |    // (FIPS Transitional level)
				         SQL_SRJO_RIGHT_OUTER_JOIN; // (FIPS Transitional level)

			if ( metaData->supportsANSI92IntermediateSQL() )
				value |= SQL_SRJO_CORRESPONDING_CLAUSE |
				         SQL_SRJO_EXCEPT_JOIN |
				         SQL_SRJO_FULL_OUTER_JOIN |
				         SQL_SRJO_INTERSECT_JOIN;
						 
			if ( metaData->supportsANSI92FullSQL() )
				value |= SQL_SRJO_CROSS_JOIN |
				         SQL_SRJO_UNION_JOIN;
		}
		break;

	case SQL_SQL92_VALUE_EXPRESSIONS:
		{
			value = 0;
			if ( metaData->supportsANSI92EntryLevelSQL() )
				value |= SQL_SVE_CAST; // (FIPS Transitional level)

			if ( metaData->supportsANSI92IntermediateSQL() )
				value |= SQL_SVE_CASE |
				         SQL_SVE_COALESCE |
				         SQL_SVE_NULLIF;

			// if ( metaData->supportsANSI92FullSQL() )
		}
		break;
	}

	//char temp [256];

	switch (item->type)
	{
	case infoString:
#ifdef DEBUG
		sprintf (temp, "  %s (string) %.128s\n", item->name, string);
		OutputDebugString (temp);
#endif
		return (setString (string, (SQLCHAR*) ptr, maxLength, actualLength)) ?
		       SQL_SUCCESS_WITH_INFO : SQL_SUCCESS;

	case infoShort:
#ifdef DEBUG
		sprintf (temp, "  %s (short) %d\n", item->name, value);
		OutputDebugString (temp);
#endif
		*((SQLUSMALLINT*) ptr) = (SQLUSMALLINT) value;
		if (actualLength)
			*actualLength = sizeof (SQLUSMALLINT);
		break;

	case infoLong:
#ifdef DEBUG
		sprintf (temp, "  %s (long) %d\n", item->name, value);
		OutputDebugString (temp);
#endif
		*((SQLUINTEGER*) ptr) = value;
		if (actualLength)
			*actualLength = sizeof (SQLUINTEGER);
		break;

	case infoUnsupported:
#ifdef DEBUG
		sprintf (temp, "  %s (string) %s\n", item->name, "*unsupported*");
		OutputDebugString (temp);
#endif
		*((SQLUINTEGER*) ptr) = value;
		break;
	}

	return sqlSuccess();
}

char* OdbcConnection::appendString(char * ptr, const char * string)
{
	while (*string)
		*ptr++ = *string++;

	return ptr;
}

RETCODE OdbcConnection::allocHandle(int handleType, SQLHANDLE * outputHandle)
{
	clearErrors();
	if ( handleType == SQL_HANDLE_DESC )
	{
		OdbcDesc * desc = allocDescriptor(odtApplication);
		desc->headAllocType = SQL_DESC_ALLOC_USER;
		*outputHandle = desc;
		return sqlSuccess();
	}
	else if (handleType == SQL_HANDLE_STMT)
	{
		*outputHandle = SQL_NULL_HDBC;

		OdbcStatement *statement = new OdbcStatement (this, statementNumber++);
		statement->next = statements;
		statements = statement;
		*outputHandle = (SQLHANDLE)statement;

		return sqlSuccess();
	}

	return sqlReturn (SQL_ERROR, "HY000", "General Error");
}

DatabaseMetaData* OdbcConnection::getMetaData()
{
	return connection->getMetaData();
}

void OdbcConnection::Lock()
{
	connection->getMetaData()->LockThread();
}

void OdbcConnection::UnLock()
{
	connection->getMetaData()->UnLockThread();
}
RETCODE OdbcConnection::sqlConnect(const SQLCHAR *dataSetName, int dsnLength, SQLCHAR *uid, int uidLength, SQLCHAR * passwd, int passwdLength, SQLCHAR * roleSQL, int roleLength)
{
	clearErrors();

	if (connected)
		return sqlReturn (SQL_ERROR, "08002", "Connection name is use");

	char temp [1024], *p = temp;

	dsn = getString (&p, dataSetName, dsnLength, "");
	account = getString (&p, uid, uidLength, "");
	password = getString (&p, passwd, passwdLength, "");
	role = getString (&p, roleSQL, roleLength, "");
	expandConnectParameters();
	RETCODE ret = connect (jdbcDriver, databaseName, account, password, role);

	if (ret != SQL_SUCCESS)
		return ret;

	return sqlSuccess();
}

RETCODE OdbcConnection::connect(const char *sharedLibrary, const char * databaseName, const char * account, const char * password, const char * role)
{
	Properties *properties = NULL;

	try
	{
#ifdef _WIN32
		HINSTANCE handle = LoadLibrary (sharedLibrary);
		if (!handle)
		{
			JString text;
			text.Format ("Unable to connect to data source: library '%s' failed to load", sharedLibrary);
			return sqlReturn (SQL_ERROR, "08001", text);
		}
		ConnectFn fn = (ConnectFn) GetProcAddress (handle, "createConnection");
		if (!fn)
		{
			JString text;
			text.Format ("Unable to connect to data source %s: can't find entrypoint 'createConnection'");
			return sqlReturn (SQL_ERROR, "08001", text);
		}
#endif
#ifdef ELF
		libraryHandle = dlopen (sharedLibrary, RTLD_NOW);
		if (!libraryHandle)
		{
			JString text;
			const char *msg = dlerror();
			text.Format ("Unable to connect to data source: library '%s' failed to load: %s",
			             sharedLibrary, msg);
			return sqlReturn (SQL_ERROR, "08001", text);
		}
		ConnectFn fn = (ConnectFn) dlsym (libraryHandle, "createConnection");
		if (!fn)
		{
			JString text;
			const char *msg = dlerror();
			if (!msg)
			{
				text.Format ("Unable to connect to data source %s: can't find entrypoint 'createConnection'", sharedLibrary);
				msg = text;
			}
			return sqlReturn (SQL_ERROR, "08001", msg);
		}
#endif
		connection = (fn)();
		//connection = createConnection();
		properties = connection->allocProperties();
		if (account)
			properties->putValue ("user", account);
		if (password)
			properties->putValue ("password", password);
		if (role)
			properties->putValue ("role", role);
		connection->openDatabase (databaseName, properties);
		delete properties;
		DatabaseMetaData *metaData = connection->getMetaData();
		const char *quoteString = metaData->getIdentifierQuoteString();
		quotedIdentifiers = quoteString [0] == '"';

		// Next two lines added by CA
		connection->setAutoCommit( autoCommit );
		connection->setTransactionIsolation( transactionIsolation );
		connection->setExtInitTransaction( optTpb );

	}
	catch (SQLException& exception)
	{
		if (properties)
			delete properties;
		JString text = exception.getText();
		connection->close();
		connection = NULL;
		return sqlReturn (SQL_ERROR, "08004", text);
	}

	connected = true;

	return SQL_SUCCESS;
}

RETCODE OdbcConnection::sqlEndTran(int operation)
{
	clearErrors();

	if (connection)
		try
		{
			switch (operation)
			{
			case SQL_COMMIT:
				connection->commit();
				break;

			case SQL_ROLLBACK:
				connection->rollback();
			}
		}
		catch (SQLException& exception)
		{
			postError ("S1000", exception);
			return SQL_ERROR;
		}

	return sqlSuccess();
}

void OdbcConnection::statementDeleted(OdbcStatement * statement)
{
	for (OdbcObject **ptr = (OdbcObject**) &statements; *ptr; ptr =&((*ptr)->next))
		if (*ptr == statement)
		{
			*ptr = statement->next;
			break;
		}
}

void OdbcConnection::expandConnectParameters()
{
	if (!dsn.IsEmpty())
	{
		if (databaseName.IsEmpty())
			databaseName = readAttribute (SETUP_DBNAME);

		if (account.IsEmpty())
			account = readAttribute (SETUP_USER);

		if (password.IsEmpty())
			password = readAttribute (SETUP_PASSWORD);

		if (jdbcDriver.IsEmpty())
			jdbcDriver = readAttribute (SETUP_JDBC_DRIVER);

		if (role.IsEmpty())
			role = readAttribute(SETUP_ROLE);

		optTpb = 0;

		JString	optionsTpb = readAttribute(SETUP_READONLY_TPB);
		if(optionsTpb[0]=='Y')optTpb |=TRA_ro;
		optionsTpb = readAttribute(SETUP_NOWAIT_TPB);
		if(optionsTpb[0]=='Y')optTpb |=TRA_nw;
	}

	if (jdbcDriver.IsEmpty())
		jdbcDriver = DEFAULT_DRIVER;
}

OdbcDesc* OdbcConnection::allocDescriptor(OdbcDescType type)
{
	OdbcDesc *descriptor = new OdbcDesc (type, this);
	descriptor->next = descriptors;
	descriptors = descriptor;

	return descriptor;
}

void OdbcConnection::descriptorDeleted(OdbcDesc * descriptor)
{
	for (OdbcDesc **ptr = &descriptors; *ptr; ptr = (OdbcDesc**)&(*ptr)->next)
		if (*ptr == descriptor)
		{
			*ptr = (OdbcDesc*) descriptor->next;
			break;
		}
}

RETCODE OdbcConnection::sqlGetConnectAttr(int attribute, SQLPOINTER ptr, int bufferLength, SQLINTEGER *lengthPtr)
{
	clearErrors();
	long value;
	char *string = NULL;

	switch (attribute)
	{
	case SQL_ATTR_ASYNC_ENABLE:
		value = asyncEnabled;
		break;

	case SQL_ATTR_ACCESS_MODE:			//   101		
		value = accessMode;
		break;



	case SQL_TXN_ISOLATION:			//   108
		value = connection->getTransactionIsolation();
		break;

	case SQL_AUTOCOMMIT:			//   102
		value = (autoCommit) ? SQL_AUTOCOMMIT_ON : SQL_AUTOCOMMIT_OFF;
		break;

	case SQL_ATTR_ODBC_CURSORS: // SQL_ODBC_CURSORS   110
		value = cursors;
		break;

	case SQL_ATTR_CONNECTION_DEAD:
		value = SQL_CD_FALSE;
		break;

	case SQL_ATTR_AUTO_IPD:			// 10001
		value = SQL_TRUE;
		break;

	case SQL_LOGIN_TIMEOUT:			//   103
	case SQL_OPT_TRACE:				//   104
	case SQL_OPT_TRACEFILE:			//   105
	case SQL_TRANSLATE_DLL:			//   106
	case SQL_TRANSLATE_OPTION:		//   107
	case SQL_QUIET_MODE:			//   111
	case SQL_PACKET_SIZE:			//   112

	default:
		return sqlReturn (SQL_ERROR, "HYC00", "Optional feature not implemented");
	}

	SQLINTEGER len = bufferLength;
	lengthPtr = &len;
	if (string)
		return returnStringInfo (ptr, bufferLength, lengthPtr, string);

	if (ptr)
		*(long*) ptr = value;

	if (lengthPtr)
		*lengthPtr = sizeof (long);

	return sqlSuccess();
}
