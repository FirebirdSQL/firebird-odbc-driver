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

enum enFetchType { NoneFetch, Fetch, ExtendedFetch, FetchScroll };

typedef RETCODE (OdbcStatement::*EXECUTE_FUNCTION)();
typedef bool (ResultSet::*FETCH_FUNCTION)();

class OdbcStatement : public OdbcObject  
{
public:
	RETCODE sqlMoreResults();
	inline RETCODE fetchData();
	inline RETCODE returnData();
	inline RETCODE returnDataFromExtendedFetch();
	RETCODE sqlColAttribute (int column, int fieldId, SQLPOINTER attributePtr, int bufferLength, SQLSMALLINT* strLengthPtr, SQLPOINTER numericAttributePtr);
	RETCODE sqlFetchScroll (int orientation, int offset);
	RETCODE sqlFetchScrollCursorStatic(int orientation, int offset);
	RETCODE sqlSetPos (SQLUSMALLINT rowNumber, SQLUSMALLINT operation, SQLUSMALLINT lockType);
	RETCODE sqlSetScrollOptions (SQLUSMALLINT fConcurrency, SQLINTEGER crowKeyset, SQLUSMALLINT crowRowset);
	RETCODE sqlExtendedFetch (int orientation, int offset, SQLUINTEGER *rowCountPointer, SQLUSMALLINT *rowStatusArray);
	RETCODE sqlColAttributes (int column, int descType, SQLPOINTER buffer, int bufferSize, SWORD *length, SDWORD *value);
	RETCODE sqlRowCount (SQLINTEGER *rowCount);
	RETCODE sqlSetStmtAttr (int attribute, SQLPOINTER ptr, int length);
	RETCODE sqlParamData(SQLPOINTER *ptr);	// Carlos Guzmán Álvarez
	RETCODE	sqlPutData (SQLPOINTER value, SQLINTEGER valueSize);
	RETCODE sqlGetTypeInfo (int dataType);
	bool 	registerOutParameter();
	RETCODE inputParam();
	RETCODE executeStatement();
	RETCODE executeProcedure();
	RETCODE sqlGetCursorName (SQLCHAR *name, int bufferLength, SQLSMALLINT *nameLength);
	RETCODE sqlGetStmtAttr (int attribute, SQLPOINTER value, int bufferLength, SQLINTEGER *lengthPtr);
	RETCODE sqlCloseCursor();
	RETCODE sqlSetCursorName (SQLCHAR*name, int nameLength);
	RETCODE sqlProcedureColumns(SQLCHAR * catalog, int catLength, SQLCHAR * schema, int schemaLength, SQLCHAR * proc, int procLength, SQLCHAR*col,int colLength);
	RETCODE sqlProcedures(SQLCHAR * catalog, int catLength, SQLCHAR * schema, int schemaLength, SQLCHAR * proc, int procLength);
	RETCODE sqlCancel();
	RETCODE sqlBindParameter (int parameter, int type, int cType, int sqlType, int precision, int scale, PTR ptr, int bufferLength, SDWORD *length);
	RETCODE sqlDescribeParam (int parameter, SWORD* sqlType, UDWORD*precision, SWORD*scale,SWORD*nullable);
	RETCODE formatParameter( int parameter );
	RETCODE sqlExecuteDirect (SQLCHAR * sql, int sqlLength);
	RETCODE sqlExecute();
	RETCODE sqlGetData (int column, int cType, PTR value, int bufferLength, SDWORD *length);
	RETCODE sqlDescribeCol (int col, SQLCHAR *colName, int nameSize, SWORD *nameLength,SWORD*sqlType,UDWORD*precision,SWORD*scale,SWORD *nullable);
	RETCODE sqlNumResultCols (SWORD *columns);
	RETCODE sqlNumParams (SWORD *params);
	RETCODE sqlForeignKeys (SQLCHAR *pkCatalog, int pkCatLength, SQLCHAR*pkSchema, int pkSchemaLength,SQLCHAR*pkTable,int pkTableLength, SQLCHAR* fkCatalog,int fkCatalogLength, SQLCHAR*fkSchema, int fkSchemaLength,SQLCHAR*fkTable,int fkTableLength);
	RETCODE sqlPrimaryKeys (SQLCHAR * catalog, int catLength, SQLCHAR * schema, int schemaLength, SQLCHAR * table, int tableLength);
	RETCODE sqlStatistics (SQLCHAR * catalog, int catLength, SQLCHAR * schema, int schemaLength, SQLCHAR * table, int tableLength, int unique, int reservedSic);
	void releaseParameters();
	void releaseBindings();
	RETCODE sqlFreeStmt (int option);
	RETCODE sqlFetch();
	RETCODE sqlBindCol (int columnNumber, int targetType, SQLPOINTER targetValuePtr, SQLINTEGER bufferLength, SQLINTEGER *indPtr);
	void rebindColumn();
	void rebindParam(bool initAttrDataAtExec = false);
	void setResultSet (ResultSet *results, bool fromSystemCatalog = true);
	void releaseResultSet();
	void releaseStatement();
	RETCODE sqlPrepare (SQLCHAR *sql, int sqlLength);

	RETCODE sqlColumns (SQLCHAR * catalog, int catLength, SQLCHAR * schema, int schemaLength, SQLCHAR * table, int tableLength, SQLCHAR *column, int columnLength);
	RETCODE sqlTables (SQLCHAR* catalog, int catLength, SQLCHAR* schema, int schemaLength, SQLCHAR*table, int tableLength, SQLCHAR *type, int typeLength);
	RETCODE sqlTablePrivileges (SQLCHAR* catalog, int catLength, SQLCHAR* schema, int schemaLength, SQLCHAR*table, int tableLength);
	RETCODE sqlColumnPrivileges (SQLCHAR* catalog, int catLength, SQLCHAR* schema, int schemaLength, SQLCHAR*table, int tableLength, SQLCHAR * column, int columnLength);
	RETCODE sqlSpecialColumns(unsigned short rowId, SQLCHAR * catalog, int catLength, SQLCHAR * schema, int schemaLength, SQLCHAR * table, int tableLength, unsigned short scope, unsigned short nullable);
	RETCODE sqlSetParam (int parameter, int cType, int sqlType, int precision, int scale, PTR ptr, SDWORD * length);
	void addBindColumn(int column, DescRecord * recordFrom, DescRecord * recordTo);
	void delBindColumn(int column);
	void addBindParam(int param, DescRecord * recordFrom, DescRecord * recordTo);
	void delBindParam(int param);
	virtual OdbcObjectType getType();
	OdbcStatement(OdbcConnection *connect, int statementNumber);
	~OdbcStatement();
	bool isStaticCursor(){ return cursorType == SQL_CURSOR_STATIC && cursorScrollable == SQL_SCROLLABLE || isResultSetFromSystemCatalog; }
	long getCurrentFetched(){ return countFetched; }
	bool getSchemaFetchData(){ return rowBindType || bindOffsetPtr; }
	inline StatementMetaData	*getStatementMetaDataIRD();
	inline void clearErrors();
	RETCODE prepareGetData(int column, DescRecord *recordARD);
	inline void setZeroColumn(int column);
	inline RETCODE transferDataToBlobParam ( DescRecord *record );
	void bindInputOutputParam(int param, DescRecord * recordApp);
	void bindOutputColumn(int column, DescRecord * recordApp);

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
	int					numberColumns;
	bool				registrationOutParameter;
	bool				isRegistrationOutParameter;
    int                 parameterNeedData;
	bool				eof;
	bool				cancel;
	long				countFetched;
	enFetchType			enFetch;
	JString				cursorName;
	bool				setPreCursorName;
	bool				isResultSetFromSystemCatalog;
	bool				isFetchStaticCursor;
	bool				schemaFetchData;
	int					rowBindType;
	int					paramBindType;
	int					rowArraySize;

	int					fetchRetData;
	void				*paramBindOffset;
	void				*paramsProcessedPtr;
	int					paramsetSize;
	SQLINTEGER			*sqldataOutOffsetPtr;
	SQLINTEGER			*bindOffsetPtr;
	SQLUINTEGER			enableAutoIPD;
	SQLINTEGER			useBookmarks;
	SQLINTEGER			cursorSensitivity;
	SQLPOINTER			fetchBookmarkPtr;
	SQLUINTEGER			noscanSQL;
	int					currency;
	int					cursorType;
	int					cursorScrollable;
	bool				asyncEnable;
	int					rowNumber;
	long				indicatorRowNumber;
	int					maxRows;
	int					maxLength;
};

}; // end namespace OdbcJdbcLibrary

#endif // !defined(_ODBCSTATEMENT_H_)
