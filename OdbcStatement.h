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

#if !defined(AFX_ODBCSTATEMENT_H__ED260D97_1BC4_11D4_98DF_0000C01D2301__INCLUDED_)
#define AFX_ODBCSTATEMENT_H__ED260D97_1BC4_11D4_98DF_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "OdbcObject.h"

struct Binding {
	int			type;
	int			cType;
	int			sqlType;
	void		*pointer;
	void		*pointerOrg;
	SQLINTEGER	bufferLength;
//Suggested by R. Milharcic
	SQLINTEGER	dataOffset;
	bool		startedTransfer;		// Carlos G.A.
	bool		data_at_exec;
	SQLINTEGER	*indicatorPointer;
	};
    
class OdbcConnection;
class OdbcDesc;
class DescRecord;
class ResultSet;
class ResultSetMetaData;
class PreparedStatement;
class CallableStatement;

enum enFetchType { NoneFetch, Fetch, ExtendedFetch, FetchScroll };

class OdbcStatement : public OdbcObject  
{
public:
	RETCODE sqlMoreResults();
	RETCODE sqlColAttribute (int column, int fieldId, SQLPOINTER attributePtr, int bufferLength, SQLSMALLINT* strLengthPtr, SQLPOINTER numericAttributePtr);
	RETCODE returnData();
	RETCODE sqlFetchScroll (int orientation, int offset);
	RETCODE sqlFetchScrollCursorStatic(int orientation, int offset);
	RETCODE sqlSetPos (SQLUSMALLINT rowNumber, SQLUSMALLINT operation, SQLUSMALLINT lockType);
	RETCODE sqlExtendedFetch (int orientation, int offset, SQLUINTEGER *rowCountPointer, SQLUSMALLINT *rowStatusArray);
	RETCODE sqlColAttributes (int column, int descType, SQLPOINTER buffer, int bufferSize, SWORD *length, SDWORD *value);
	RETCODE sqlRowCount (SQLINTEGER *rowCount);
	RETCODE sqlSetStmtAttr (int attribute, SQLPOINTER ptr, int length);
	RETCODE sqlParamData(SQLPOINTER *ptr);	// Carlos Guzmán Álvarez
	RETCODE	sqlPutData (SQLPOINTER value, SQLINTEGER valueSize);
	RETCODE sqlGetTypeInfo (int dataType);
	Binding* allocBindings (int count, int oldCount, Binding *oldBindings);
//	RETCODE setParameters();	//Added 2002-06-04 RM
//	void executeSQL();			//Added 2002-06-04 RM
	RETCODE executeStatement();	//Changed return type 2002-06-04 RM 
	char* getToken (const char** ptr, char *token);
	bool isStoredProcedureEscape (const char *sqlString);
	RETCODE sqlGetCursorName (SQLCHAR *name, int bufferLength, SQLSMALLINT *nameLength);
	RETCODE sqlGetStmtAttr (int attribute, SQLPOINTER value, int bufferLength, SQLINTEGER *lengthPtr);
	RETCODE sqlCloseCursor();
	RETCODE sqlSetCursorName (SQLCHAR*name, int nameLength);
	RETCODE sqlProcedureColumns(SQLCHAR * catalog, int catLength, SQLCHAR * schema, int schemaLength, SQLCHAR * proc, int procLength, SQLCHAR*col,int colLength);
	RETCODE sqlProcedures(SQLCHAR * catalog, int catLength, SQLCHAR * schema, int schemaLength, SQLCHAR * proc, int procLength);
	RETCODE sqlCancel();
	void setParameter (Binding *binding, int parameter);
	void setParameter(DescRecord *record,int parameter);
	RETCODE sqlBindParameter (int parameter, int type, int cType, int sqlType, int precision, int scale, PTR ptr, int bufferLength, SDWORD *length);
	RETCODE sqlDescribeParam (int parameter, SWORD* sqlType, UDWORD*precision, SWORD*scale,SWORD*nullable);
	RETCODE OdbcStatement::formatParameter( int parameter );
	ResultSet* getResultSet();
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
	RETCODE setValue (Binding *binding, int column);
	bool setValue (DescRecord *record, int column);

	RETCODE sqlFetch();
//	RETCODE sqlBindCol (int columnNumber, int targetType, SQLPOINTER targetValuePtr, SQLINTEGER bufferLength, SQLINTEGER *indPtr);
	RETCODE sqlBindCol (int columnNumber, int targetType, SQLPOINTER targetValuePtr, SQLINTEGER bufferLength, SQLINTEGER *indPtr, Binding** _bindings = NULL, int* _numberBindings = NULL); //From RM
	void setResultSet (ResultSet *results);
	void releaseResultSet();
	void releaseStatement();
//	RETCODE sqlPrepare (SQLCHAR *sql, int sqlLength);
	RETCODE sqlPrepare (SQLCHAR *sql, int sqlLength, bool isExecDirect);

	RETCODE sqlColumns (SQLCHAR * catalog, int catLength, SQLCHAR * schema, int schemaLength, SQLCHAR * table, int tableLength, SQLCHAR *column, int columnLength);
	RETCODE sqlTables (SQLCHAR* catalog, int catLength, SQLCHAR* schema, int schemaLength, SQLCHAR*table, int tableLength, SQLCHAR *type, int typeLength);
	RETCODE sqlTablePrivileges (SQLCHAR* catalog, int catLength, SQLCHAR* schema, int schemaLength, SQLCHAR*table, int tableLength);
	RETCODE sqlColumnPrivileges (SQLCHAR* catalog, int catLength, SQLCHAR* schema, int schemaLength, SQLCHAR*table, int tableLength, SQLCHAR * column, int columnLength);
	RETCODE sqlSpecialColumns(unsigned short rowId, SQLCHAR * catalog, int catLength, SQLCHAR * schema, int schemaLength, SQLCHAR * table, int tableLength, unsigned short scope, unsigned short nullable);
	RETCODE sqlSetParam (int parameter, int cType, int sqlType, int precision, int scale, PTR ptr, SDWORD * length);
	virtual OdbcObjectType getType();
	OdbcStatement(OdbcConnection *connect, int statementNumber);
	virtual ~OdbcStatement();
	bool isStaticCursor(){ return cursorType == SQL_CURSOR_STATIC && cursorScrollable == SQL_SCROLLABLE; }

	OdbcConnection		*connection;
	OdbcDesc			*applicationRowDescriptor;
	OdbcDesc			*applicationParamDescriptor;
	OdbcDesc			*implementationRowDescriptor;
	OdbcDesc			*implementationParamDescriptor;
	ResultSet			*resultSet;
	PreparedStatement	*statement;
	CallableStatement	*callableStatement;
	ResultSetMetaData	*metaData;
	int					numberColumns;
	int					numberParameters;
//Added 2002-06-04	RM
    int                 parameterNeedData;
 	int					numberGetDataBindings;
 	int					columnPrevGetDataBinding;
	Binding				*getDataBindings;
	bool				eof;
	bool				cancel;
	bool				fetched;
	enFetchType			enFetch;
	JString				cursorName;
	int					rowBindType;
	int					paramBindType;
	int					rowArraySize;

	int					fetchRetData;
	void				*paramBindOffset;
	void				*paramsProcessedPtr;
	int					paramsetSize;
	SQLINTEGER			*bindOffsetPtr;
	SQLUSMALLINT		*rowStatusPtr;
	SQLUINTEGER			enableAutoIPD;
	SQLINTEGER			useBookmarks;
	SQLINTEGER			cursorSensitivity;
	SQLPOINTER			fetchBookmarkPtr;
	int					currency;
	int					cursorType;
	bool				cursorScrollable;
	bool				asyncEnable;
	int					rowNumber;
	long				indicatorRowNumber;
	int					maxRows;
	int					maxLength;
};

#endif // !defined(AFX_ODBCSTATEMENT_H__ED260D97_1BC4_11D4_98DF_0000C01D2301__INCLUDED_)
