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
 *	Changes
 *
 *	See OdbcStatement.cpp for details of changes
 *
 */

// OdbcStatement.h: interface for the OdbcStatement class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ODBCSTATEMENT_H_)
#define _ODBCSTATEMENT_H_

#include "OdbcObject.h"

namespace OdbcJdbcLibrary {

using namespace classJString;
using namespace IscDbcLibrary;

class OdbcConnection;
class OdbcDesc;
class DescRecord;
class OdbcStatement;
class OdbcConvert;

//
//	from odbcss.h
//
#define MSSQL_CA_SS_COLUMN_HIDDEN		1211 //	Column is hidden (FOR BROWSE)
#define MSSQL_CA_SS_COLUMN_KEY			1212 //	Column is key column (FOR BROWSE)

enum enFetchType { NoneFetch, Fetch, ExtendedFetch, FetchScroll };

typedef SQLRETURN (OdbcStatement::*EXECUTE_FUNCTION)();
typedef bool (ResultSet::*FETCH_FUNCTION)();

class OdbcStatement : public OdbcObject  
{
public:
	SQLRETURN sqlMoreResults();
	inline SQLRETURN fetchData();
	inline SQLRETURN returnData();
	inline SQLRETURN returnDataFromExtendedFetch();
#ifdef _WIN64
	SQLRETURN sqlColAttribute( int column, int fieldId, SQLPOINTER attributePtr, int bufferLength, SQLSMALLINT *strLengthPtr, SQLLEN *numericAttributePtr );
#else
	SQLRETURN sqlColAttribute( int column, int fieldId, SQLPOINTER attributePtr, int bufferLength, SQLSMALLINT *strLengthPtr, SQLPOINTER numericAttributePtr );
#endif
	SQLRETURN sqlFetchScroll (int orientation, int offset);
	SQLRETURN sqlFetchScrollCursorStatic(int orientation, int offset);
	SQLRETURN sqlSetPos (SQLUSMALLINT rowNumber, SQLUSMALLINT operation, SQLUSMALLINT lockType);
	SQLRETURN sqlBulkOperations( int operation );
	SQLRETURN sqlSetScrollOptions (SQLUSMALLINT fConcurrency, SQLLEN crowKeyset, SQLUSMALLINT crowRowset);
	SQLRETURN sqlExtendedFetch (int orientation, int offset, SQLULEN *rowCountPointer, SQLUSMALLINT *rowStatusArray);
	SQLRETURN sqlRowCount (SQLLEN *rowCount);
	SQLRETURN sqlSetStmtAttr (int attribute, SQLPOINTER ptr, int length);
	SQLRETURN sqlParamData(SQLPOINTER *ptr);	// Carlos Guzmán Álvarez
	SQLRETURN	sqlPutData (SQLPOINTER value, SQLLEN valueSize);
	SQLRETURN sqlGetTypeInfo (int dataType);
	bool 	registerOutParameter();
	SQLRETURN inputParam( bool arrayColumnWiseBinding = false );
	SQLRETURN executeStatement();
	SQLRETURN executeStatementParamArray();
	SQLRETURN executeProcedure();
	SQLRETURN executeCommit();
	SQLRETURN executeRollback();
	SQLRETURN executeNone();
	SQLRETURN executeCreateDatabase();
	SQLRETURN sqlGetCursorName (SQLCHAR *name, int bufferLength, SQLSMALLINT *nameLength);
	SQLRETURN sqlGetStmtAttr (int attribute, SQLPOINTER value, int bufferLength, SQLINTEGER *lengthPtr);
	SQLRETURN sqlCloseCursor();
	SQLRETURN sqlSetCursorName (SQLCHAR*name, int nameLength);
	SQLRETURN sqlProcedureColumns(SQLCHAR * catalog, int catLength, SQLCHAR * schema, int schemaLength, SQLCHAR * proc, int procLength, SQLCHAR*col,int colLength);
	SQLRETURN sqlProcedures(SQLCHAR * catalog, int catLength, SQLCHAR * schema, int schemaLength, SQLCHAR * proc, int procLength);
	SQLRETURN sqlCancel();
	SQLRETURN sqlBindParameter (int parameter, int type, int cType, int sqlType, int precision, int scale, PTR ptr, int bufferLength, SQLLEN *length);
	SQLRETURN sqlDescribeParam (int parameter, SWORD* sqlType, SQLULEN*precision, SWORD*scale,SWORD*nullable);
	SQLRETURN formatParameter( int parameter );
	SQLRETURN sqlExecDirect (SQLCHAR * sql, int sqlLength);
	SQLRETURN sqlExecute();
	SQLRETURN sqlGetData (int column, int cType, PTR value, SQLLEN bufferLength, SQLLEN *length);
	SQLRETURN sqlDescribeCol (int col, SQLCHAR *colName, int nameSize, SWORD *nameLength,SWORD*sqlType,SQLULEN*precision,SWORD*scale,SWORD *nullable);
	SQLRETURN sqlNumResultCols (SWORD *columns);
	SQLRETURN sqlNumParams (SWORD *params);
	SQLRETURN sqlForeignKeys (SQLCHAR *pkCatalog, int pkCatLength, SQLCHAR*pkSchema, int pkSchemaLength,SQLCHAR*pkTable,int pkTableLength, SQLCHAR* fkCatalog,int fkCatalogLength, SQLCHAR*fkSchema, int fkSchemaLength,SQLCHAR*fkTable,int fkTableLength);
	SQLRETURN sqlPrimaryKeys (SQLCHAR * catalog, int catLength, SQLCHAR * schema, int schemaLength, SQLCHAR * table, int tableLength);
	SQLRETURN sqlStatistics (SQLCHAR * catalog, int catLength, SQLCHAR * schema, int schemaLength, SQLCHAR * table, int tableLength, int unique, int reservedSic);
	void releaseParameters();
	void releaseBindings();
	SQLRETURN sqlFreeStmt (int option);
	SQLRETURN sqlFetch();
	SQLRETURN sqlBindCol (int columnNumber, int targetType, SQLPOINTER targetValuePtr, SQLLEN bufferLength, SQLLEN *indPtr);
	void rebindColumn();
	void rebindParam(bool initAttrDataAtExec = false);
	void setResultSet (ResultSet *results, bool fromSystemCatalog = true);
	void releaseResultSet();
	void releaseStatement();
	SQLRETURN sqlPrepare (SQLCHAR *sql, int sqlLength);

	SQLRETURN sqlColumns (SQLCHAR * catalog, int catLength, SQLCHAR * schema, int schemaLength, SQLCHAR * table, int tableLength, SQLCHAR *column, int columnLength);
	SQLRETURN sqlTables (SQLCHAR* catalog, int catLength, SQLCHAR* schema, int schemaLength, SQLCHAR*table, int tableLength, SQLCHAR *type, int typeLength);
	SQLRETURN sqlTablePrivileges (SQLCHAR* catalog, int catLength, SQLCHAR* schema, int schemaLength, SQLCHAR*table, int tableLength);
	SQLRETURN sqlColumnPrivileges (SQLCHAR* catalog, int catLength, SQLCHAR* schema, int schemaLength, SQLCHAR*table, int tableLength, SQLCHAR * column, int columnLength);
	SQLRETURN sqlSpecialColumns(unsigned short rowId, SQLCHAR * catalog, int catLength, SQLCHAR * schema, int schemaLength, SQLCHAR * table, int tableLength, unsigned short scope, unsigned short nullable);
	SQLRETURN sqlSetParam (int parameter, int cType, int sqlType, int precision, int scale, PTR ptr, SQLLEN * length);
	void addBindColumn(int column, DescRecord * recordFrom, DescRecord * recordTo);
	void delBindColumn(int column);
	void addBindParam(int param, DescRecord * recordFrom, DescRecord * recordTo);
	void delBindParam(int param);
	virtual OdbcConnection* getConnection();
	virtual OdbcObjectType getType();
	OdbcStatement(OdbcConnection *connect, int statementNumber);
	~OdbcStatement();
	bool isStaticCursor(){ return cursorType != SQL_CURSOR_FORWARD_ONLY && cursorScrollable == SQL_SCROLLABLE || isResultSetFromSystemCatalog; }
	int getCurrentFetched(){ return countFetched; }
	bool getSchemaFetchData(){ return applicationRowDescriptor->headBindType || applicationRowDescriptor->headBindOffsetPtr; }
	inline StatementMetaData	*getStatementMetaDataIRD();
	inline void clearErrors();
	SQLRETURN prepareGetData(int column, DescRecord *recordARD);
	inline void setZeroColumn(int column);
	inline SQLRETURN transferDataToBlobParam ( DescRecord *record );
	void bindInputOutputParam(int param, DescRecord * recordApp);
	void bindOutputColumn(int column, DescRecord * recordApp);
	operator Statement* () { return statement->getStatement(); }

	OdbcConnection		*connection;
	OdbcDesc			*applicationRowDescriptor;
	OdbcDesc			*saveApplicationRowDescriptor;
	OdbcDesc			*applicationParamDescriptor;
	OdbcDesc			*saveApplicationParamDescriptor;
	OdbcDesc			*implementationRowDescriptor;
	OdbcDesc			*implementationParamDescriptor;
	OdbcDesc			*implementationGetDataDescriptor;
	OdbcConvert			*convert;
	ListBindColumn		*listBindIn;
	ListBindColumn		*listBindOut;
	ListBindColumn		*listBindGetData;
	ResultSet			*resultSet;
	EXECUTE_FUNCTION	execute;
	FETCH_FUNCTION		fetchNext;
	InternalStatement	*statement;
	StatementMetaData	*metaData;
	OdbcStatement		*bulkInsert;
	int					numberColumns;
	bool				registrationOutParameter;
	bool				isRegistrationOutParameter;
    int                 parameterNeedData;
	bool				eof;
	bool				cancel;
	int					countFetched;
	enFetchType			enFetch;
	JString				cursorName;
	JString				sqlPrepareString;
	bool				setPreCursorName;
	bool				isResultSetFromSystemCatalog;
	bool				isFetchStaticCursor;
	bool				schemaFetchData;

	SQLLEN				fetchRetData;
	SQLLEN				*sqldataOutOffsetPtr;
	SQLUINTEGER			enableAutoIPD;
	SQLINTEGER			useBookmarks;
	SQLINTEGER			cursorSensitivity;
	SQLPOINTER			fetchBookmarkPtr;
	SQLUINTEGER			noscanSQL;
	SQLLEN				bindOffsetColumnWiseBinding;
	SQLLEN				bindOffsetIndColumnWiseBinding;
	int					currency;
	int					cursorType;
	int					cursorScrollable;
	bool				asyncEnable;
	int					rowNumber;
	int					rowNumberParamArray;
	int					lastRowsetSize;
	SQLLEN				indicatorRowNumber;
	int					maxRows;
	int					maxLength;
};

}; // end namespace OdbcJdbcLibrary

#endif // !defined(_ODBCSTATEMENT_H_)
