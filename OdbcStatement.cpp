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
 *  The Original Code was created by James A. Starkey
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 *
 *
 *	Changes
 *	2003-03-24	OdbcStatement.cpp
 *				Contributed by Norbert Meyer
 *				o In sqlExtendedFetch() add support for
 *				  applications which only check rowCountPointer
 *				o In setValue()
 *				  Empty strings have len = 0, so test for that
 *				o In setParameter() and executeStatement()
 *				  test for binding->indicatorPointer
 *
 *	2003-03-24	OdbcStatement.cpp
 *				Contributed by Carlos Guzman Alvarez
 *				Remove updatePreparedResultSet from OdbStatement 
 *				and achieve the same goal in another way.
 *
 *	2003-03-24	OdbcStatement.cpp
 *				Contributed by Roger Gammans
 *				Fix a segv in SQLBindCol()
 *
 *	2002-11-21	OdbcStatement.cpp
 *				Contributed by C. G. Alvarez
 *				Improved handling of TIME datatype
 *
 *	2002-11-21	OdbcStatement.cpp
 *				Contributed by C. G. Alvarez
 *				Modification to OdbcStatement::sqlExtendedFetch
 *				to support SQL_API_SQLEXTENDEDFETCH
 *
 *  2002-10-11	OdbcStatement.cpp
 *				Contributed by C. G. Alvarez
 *				Extensive modifications to blob reading and writing
 *
 *  2002-10-11	OdbcStatement.cpp
 *				Contributed by C. G. Alvarez
 *				Added sqlNumParams()
 *
 *  2002-08-14	OdbcStatement.cpp
 *				Contributed by C. G. Alvarez
 *				Minor enhancements to sqlGetSmtAttr and sqlSetStmtAttr.
 *
 *
 *	2002-08-12	OdbcStatement.cpp
 *				Added changes from C. G. Alvarez to so that
 *				sqlColAttributes() called with SQL_COLUMN_TYPE_NAME returns 
 *				the name of the type instead of the number of the type.
 *				Similarly, sqlColAttribute() will return string for 
 *				SQL_DESC_TYPE_NAME.
 *				
 *				Added sqlTablePrivileges()
 *
 *
 *	2002-07-08	OdbcStatement.cpp
 *				Added changes from C. G. Alvarez to return
 *				SQL_DESC_UNNAMED and SQL_DESC_BASE_TABLE_NAME
 *				from sqlColAtrributes()
 *
 *	2002-06-26	OdbcStatement.cpp
 *				Added changes from C. G. Alvarez to provide 
 *				better support for remote views.
 *
 *	2002-06-26	OdbcStatement::OdbcStatement
 *				Initialised numberColumns in constructor (Roger Gammans)
 *
 *	2002-06-17	OdbcStatement::setParameter()
 *				Submitted by C. G. Alvarez
 *				Added code to handle returning strings that are not
 *				null terminated.
 *
 *	2002-06-08	OdbcStatement.cpp
 *				Submitted by B. Schulte
 *				sqlNumResultCols().
 *				This fixes the bug : ' I can't edit my remote-views 
 *				in Visual FoxPro'. If the resultSet does not exist, 
 *				execute it, to get a valid resultSet. Foxpro calls 
 *				this function to get all column-descriptions for 
 *				its remote-views. 
 *
 *	2002-06-04	OdbcdStatement.cpp
 *				submitted by Robert Milharcic
 *				Extensive changes to improve writing and 
 *				retrieval of binary blobs
 *
 *	2002-05-20	Updated OdbcStatement.cpp 
 *
 *				Contributed by Pier Alberto GUIDOTTI
 *				o Use RM's changes to support reading 
 *				  text blobs too.
 *
 *	2002-05-20	Updated OdbcStatement.cpp 
 *
 *				Contributed by Robert Milharcic
 *				o Several changes to allow reading of binary blobs
 *				  See code commented with //Added by RM or //From RM
 *
 *
 *	2002-05-20	Updated OdbcStatement.cpp 
 *
 *				Contributed by Bernhard Schulte 
 *				o Use TimeStamp instead of DateTime in setParameter().
 *
 *
 */

// OdbcStatement.cpp: implementation of the OdbcStatement class.
//
//////////////////////////////////////////////////////////////////////
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include "OdbcStatement.h"
#include "OdbcConnection.h"
#include "OdbcError.h"
#include "IscDbc/Connection.h"
#include "DescRecord.h"
#include "IscDbc/SQLException.h"
#include "OdbcDateTime.h"

#ifdef DEBUG                               
#define TRACE(msg)		OutputDebugString(#msg"\n");
#define TRACE02(msg,val)  TraceOutput(#msg,val)
#else
#define TRACE(msg)		
#define TRACE02(msg,val)
#endif
#define RESULTS(fn)		(resultSet) ? resultSet->fn : callableStatement->fn
#define SKIP_WHITE(p)	while (charTable [*p] == WHITE) ++p

void TraceOutput(char * msg,long val)
{
	char buf[80];
	sprintf(buf,"\t%s = %d : %x\n",msg,val,val);
	OutputDebugString(buf);
}

#define PUNCT			1
#define WHITE			2
#define DIGIT			4
#define LETTER			8
#define IDENT			(LETTER | DIGIT)

#ifdef __GNUWIN32__
extern double listScale[]; // from OdbcConvert.cpp
#else
extern unsigned __int64 listScale[]; // from OdbcConvert.cpp
#endif
static char charTable [256];
static int init();
static int foo = init();

int init ()
{
	int n;
	const char *p;

	for (p = " \t\n"; *p; ++p)
		charTable [*p] = WHITE;

	for (p = "?=(),{}"; *p; ++p)
		charTable [*p] = PUNCT;

	for (n = 'a'; n <= 'z'; ++n)
		charTable [n] |= LETTER;

	for (n = 'A'; n <= 'A'; ++n)
		charTable [n] |= LETTER;

	for (n = '0'; n <= '9'; ++n)
		charTable [n] |= DIGIT;

	return 0;
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

OdbcStatement::OdbcStatement(OdbcConnection *connect, int statementNumber)
{
	connection = connect;
	resultSet = NULL;
	statement = NULL;
	callableStatement = NULL;
	getDataBindings = NULL;	//added by RM
	metaData = NULL;
	cancel = false;
	fetched = false;
	enFetch = NoneFetch;
    parameterNeedData = 0;	
	numberGetDataBindings = 0;	//added by RM
	columnPrevGetDataBinding = -1;
	maxRows = 0;
	maxLength = 0;
	applicationRowDescriptor = connection->allocDescriptor (odtApplicationRow);
	applicationParamDescriptor = connection->allocDescriptor (odtApplicationParameter);
	implementationRowDescriptor = connection->allocDescriptor (odtImplementationRow);
	implementationParamDescriptor = connection->allocDescriptor (odtImplementationParameter);

	fetchRetData = SQL_RD_ON;			
	rowBindType = SQL_BIND_BY_COLUMN;
	paramBindType = SQL_BIND_BY_COLUMN;
	bindOffsetPtr = NULL;
	rowStatusPtr = NULL;
	paramsetSize = 0;
	numberColumns = 0;	//added by RG
	paramsProcessedPtr = NULL;
	currency = SQL_CONCUR_READ_ONLY;
	cursorType = SQL_CURSOR_FORWARD_ONLY;
	cursorName.Format ("cursor%d", statementNumber);
	cursorScrollable = false;
	asyncEnable = false;
    rowArraySize  = applicationRowDescriptor->headArraySize; //added by CGA
	enableAutoIPD = SQL_FALSE;
	useBookmarks = SQL_UB_OFF;
	cursorSensitivity = SQL_INSENSITIVE;
	fetchBookmarkPtr = NULL;
	noscanSQL = SQL_NOSCAN_OFF;
}

OdbcStatement::~OdbcStatement()
{
	connection->statementDeleted (this);
	releaseStatement();
	releaseBindings();
	releaseParameters();
	delete applicationRowDescriptor;
	delete applicationParamDescriptor;
	delete implementationRowDescriptor;
	delete implementationParamDescriptor;
}

OdbcObjectType OdbcStatement::getType()
{
	return odbcTypeStatement;
}

RETCODE OdbcStatement::sqlTables(SQLCHAR * catalog, int catLength, 
								 SQLCHAR * schema, int schemaLength, 
								 SQLCHAR * table, int tableLength, 
								 SQLCHAR * type, int typeLength)
{
	clearErrors();
	releaseStatement();
	char temp [1024], *p = temp;

	const char *cat = getString (&p, catalog, catLength, NULL);
	const char *scheme = getString (&p, schema, schemaLength, NULL);
	const char *tbl = getString (&p, table, tableLength, NULL);
	const char *typeString = getString (&p, type, typeLength, "");

	const char *typeVector [16];
	int numberTypes = 0;

	for (const char *q = typeString; *q && numberTypes < 16;)
		if (*q == ' ')
			++q;
		else
			{
			typeVector [numberTypes++] = p;
			if (*q == '\'')
				{
				for (++q; *q && *q++ != '\'';)
					*p++ = q [-1];
				while (*q && *q++ != ',')
					;
				}
			else
				for (char c; *q && (c = *q++) != ',';)
					*p++ = c;
			*p++ = 0;
			}

	try
		{
		DatabaseMetaData *metaData = connection->getMetaData();
		setResultSet (metaData->getTables (cat, scheme, tbl, numberTypes, typeVector));
		}
	catch (SQLException &exception)
		{
		postError ("HY000", exception);
		return SQL_ERROR;
		}

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlTablePrivileges(SQLCHAR * catalog, int catLength, 
								 SQLCHAR * schema, int schemaLength, 
								 SQLCHAR * table, int tableLength)
{
	clearErrors();
	releaseStatement();
	char temp [1024], *p = temp;

	const char *cat			= getString (&p, catalog, catLength, NULL);
	const char *scheme		= getString (&p, schema, schemaLength, NULL);
	const char *tbl			= getString (&p, table, tableLength, NULL);

	try
		{
		DatabaseMetaData *metaData = connection->getMetaData();
		setResultSet (metaData->getTablePrivileges (cat, scheme, tbl));
		}
	catch (SQLException &exception)
		{
		postError ("HY000", exception);
		return SQL_ERROR;
		}

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlColumnPrivileges(SQLCHAR * catalog, int catLength, 
								 SQLCHAR * schema, int schemaLength, 
								 SQLCHAR * table, int tableLength,
								 SQLCHAR * column, int columnLength)
{
	clearErrors();
	releaseStatement();
	char temp [1024], *p = temp;

	const char *cat		= getString (&p, catalog, catLength, NULL);
	const char *scheme	= getString (&p, schema, schemaLength, NULL);
	const char *tbl		= getString (&p, table, tableLength, NULL);
	const char *col		= getString (&p, column, columnLength, NULL);

	try
	{
		DatabaseMetaData *metaData = connection->getMetaData();
		setResultSet (metaData->getColumnPrivileges (cat, scheme, tbl, col));
	}
	catch (SQLException &exception)
	{
		postError ("HY000", exception);
		return SQL_ERROR;
	}

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlPrepare(SQLCHAR * sql, int sqlLength, bool isExecDirect)
{
	clearErrors();
	releaseStatement();
	JString temp, tempNative;
	const char *string = (const char*) sql;

	if (sqlLength != SQL_NTS)
	{
		temp = JString ((const char*) sql, sqlLength);
		string = temp;
	}

#ifdef DEBUG
	{
		char tempDebugStr [8196];
		sprintf (tempDebugStr, "Preparing statement:\n\t%.8170s\n", string);
		OutputDebugString (tempDebugStr);
	}
#endif

	if ( noscanSQL == SQL_NOSCAN_OFF )
	{
		long lenstrSQL = strlen(string);
		long lennewstrSQL = lenstrSQL + 4096;
		if ( connection->connection->getNativeSql(string,lenstrSQL,tempNative.getBuffer(lennewstrSQL),lennewstrSQL,&lenstrSQL))
			string = tempNative;
	}

#ifdef DEBUG
	{
		char tempDebugStr [8196];
		sprintf (tempDebugStr, "Preparing statement:\n\t%.8170s\n", string);
		OutputDebugString (tempDebugStr);
	}
#endif

	try
		{
		if (isStoredProcedureEscape (string))
			statement = callableStatement = connection->connection->prepareCall (string);
		else
			statement = connection->connection->prepareStatement (string);

		implementationRowDescriptor->setDefaultImplDesc (statement->getStatementMetaDataIRD());
		implementationParamDescriptor->setDefaultImplDesc (statement->getStatementMetaDataIPD());

//		//Added by CGA
//		if( updatePreparedResultSet )
//			setResultSet(statement->getResultSet());
		if (!isExecDirect)
			getResultSet();

		}
	catch (SQLException& exception)
		{
		postError ("HY000", exception);
		return SQL_ERROR;
		}

	return sqlSuccess();
}

void OdbcStatement::releaseStatement()
{
	releaseResultSet();
	callableStatement = NULL;

	if (statement)
		{
		statement->release();
		statement = NULL;
		}
}

void OdbcStatement::releaseResultSet()
{
	if (resultSet)
	{
		resultSet->release();
		resultSet = NULL;
		metaData  = NULL;
		implementationRowDescriptor->setDefaultImplDesc (metaData);
	}
}

void OdbcStatement::setResultSet(ResultSet * results)
{
	resultSet = results;
	metaData = resultSet->getMetaData();
	
	if ( !statement )
		implementationRowDescriptor->metaData = metaData;

	numberColumns = metaData->getColumnCount();
	eof = false;
	cancel = false;
	rowNumber = 0;
	indicatorRowNumber = 0;
}

//RETCODE OdbcStatement::sqlBindCol(int column, int targetType, SQLPOINTER targetValuePtr, SQLINTEGER bufferLength, SQLINTEGER * indPtr)
RETCODE OdbcStatement::sqlBindCol(int column, int targetType, SQLPOINTER targetValuePtr, SQLINTEGER bufferLength, SQLINTEGER * indPtr, Binding** _bindings, int* _numberBindings)
{
	clearErrors();

	if (column < 0)
		return sqlReturn (SQL_ERROR, "07009", "Invalid descriptor index");

	try
		{

		switch (targetType)
			{
			case SQL_C_CHAR:
			case SQL_C_SHORT:
			case SQL_C_SSHORT:
			case SQL_C_USHORT:
			case SQL_C_LONG:
			case SQL_C_SLONG:
			case SQL_C_ULONG:
			case SQL_C_FLOAT:
			case SQL_C_DOUBLE:
			case SQL_C_BIT:
			case SQL_C_TINYINT:
			case SQL_C_STINYINT:
			case SQL_C_UTINYINT:
			case SQL_C_SBIGINT:
			case SQL_C_UBIGINT:
			case SQL_C_BINARY:
			//case SQL_C_BOOKMARK:
			//case SQL_C_VARBOOKMARK:
			case SQL_C_DATE:
			case SQL_C_TIME:
			case SQL_C_TIMESTAMP:
			case SQL_C_NUMERIC:
			case SQL_DECIMAL:
			case SQL_TYPE_DATE:
			case SQL_TYPE_TIME:
			case SQL_TYPE_TIMESTAMP:
			case SQL_C_DEFAULT:
			//case SQL_C_GUID:
				break;
				
			default:
				{
				JString msg;
				msg.Format ("Invalid application datatype (%d)", targetType);
				LOG_MSG ((const char*) msg);
				LOG_MSG ("\n");
				return sqlReturn (SQL_ERROR, "HY03", (const char*) msg);
				//return sqlReturn (SQL_ERROR, "HY03", "Invalid application buffer type");
				}
			}

		if (!_bindings)
		{
			DescRecord *record = applicationRowDescriptor->getDescRecord (column);

			record->parameterType = SQL_PARAM_OUTPUT;
			record->type = targetType;
			record->conciseType = targetType;
			record->dataPtr = targetValuePtr;
			record->length = bufferLength;
			record->indicatorPtr = indPtr;

			if ( !column )
			{
				DescRecord *imprec = implementationRowDescriptor->getDescRecord (column);
				imprec->dataPtr = &rowNumber;
				imprec->indicatorPtr = &indicatorRowNumber;
				record->octetLengthPtr = &indicatorRowNumber;

				record->autoUniqueValue = SQL_FALSE;
				record->caseSensitive = SQL_FALSE;
				record->catalogName = "";
				record->datetimeIntervalCode = 0;
				record->displaySize = 8;
				record->fixedPrecScale = SQL_FALSE;
				record->label = "";
				record->literalPrefix = "";
				record->literalSuffix = "";
				record->localTypeName = "";
				record->name = "";
				record->nullable = SQL_NO_NULLS;
				record->octetLength = 4;
				record->precision = 4;
				record->scale = 0;
				record->schemaName = "";
				record->searchable = SQL_PRED_NONE;
				record->tableName = "";
				record->typeName = "";
				record->unNamed = SQL_UNNAMED;
				record->unSigned = SQL_FALSE;
				record->updaTable = SQL_ATTR_READONLY;
			}

			int ret = implementationRowDescriptor->setConvFn(column, record);
			if( ret == 1 )// isIdentity
			{ 
				if ( isStaticCursor() )
					implementationRowDescriptor->addBindColumn(column, record);
			}
			else if( ret == 0 )
				implementationRowDescriptor->addBindColumn(column, record);
		}
		else
		{
			int realSqlType;
			int count = MAX (column, numberColumns);
			if (count >= *_numberBindings)
			{
				*_bindings = allocBindings (count + 1, *_numberBindings, *_bindings);
				*_numberBindings = count + 1;
			}

			Binding *binding = *_bindings + column;	
			binding->type = SQL_PARAM_OUTPUT;
			binding->cType = targetType;
			binding->pointer = targetValuePtr;
			binding->bufferLength = bufferLength;
			binding->indicatorPointer = indPtr;

			if(	columnPrevGetDataBinding != column )
			{
				binding->dataOffset	= 0;
				columnPrevGetDataBinding = column;
			}
			else if ( metaData &&
				 ( metaData->getColumnType (column, realSqlType) == SQL_CHAR || 
				   metaData->getColumnType (column, realSqlType) == SQL_VARCHAR ))
				binding->dataOffset			= 0;
#ifdef DEBUG
			char tempDebugStr [128];
			sprintf (tempDebugStr, "Column %d %31s has SQL Type %3.3d, CType %3.3d, Type %3.3d \n", 
						column,
						"",
						binding->sqlType,
						binding->cType,
						binding->type
					);
			OutputDebugString (tempDebugStr);
#endif

		}
	}

	catch (SQLException& exception)
	{
		postError ("HY000", exception);
		return SQL_ERROR;
	}
	return sqlSuccess();
}

RETCODE OdbcStatement::sqlFetch()
{
	clearErrors();
	if (!resultSet)
		return sqlReturn (SQL_ERROR, "24000", "Invalid cursor state");

	if (cancel)
	{
		releaseResultSet();
		return sqlReturn (SQL_ERROR, "S1008", "Operation canceled");
	}

	if( enFetch == NoneFetch )
		enFetch = Fetch;

	fetched = true;

	try
	{
		if (eof || !resultSet->next())
		{
			eof = true;
			if (implementationRowDescriptor->headRowsProcessedPtr)
				*(SQLINTEGER*)implementationRowDescriptor->headRowsProcessedPtr = 0;
			return SQL_NO_DATA;
		}
	}
	catch (SQLException& exception)
	{
		postError ("HY000", exception);
		return SQL_ERROR;
	}

	return returnData();
}

#ifdef DEBUG
char *strDebOrientFetch[]=
{
	"",
	"SQL_FETCH_NEXT",
	"SQL_FETCH_FIRST",
	"SQL_FETCH_LAST",
	"SQL_FETCH_PRIOR",
	"SQL_FETCH_ABSOLUTE",
	"SQL_FETCH_RELATIVE",
	"",
	"SQL_FETCH_BOOKMARK",
	""
};
#endif

RETCODE OdbcStatement::sqlFetchScrollCursorStatic(int orientation, int offset)
{
	int rowsetSize = applicationRowDescriptor->headArraySize;
	rowNumber = resultSet->getPosRowInSet();

	switch(orientation) 
	{
	case SQL_FETCH_RELATIVE: 
		break;

	case SQL_FETCH_ABSOLUTE:
		if( offset == -1 )
		{
			rowNumber = sqlDiagCursorRowCount - applicationRowDescriptor->headArraySize;
			if ( rowNumber < 0 )
				rowNumber = 0;
		}
		else if ( offset < 0 )
		{
			if( abs(offset) > sqlDiagCursorRowCount )
			{
				if ( abs(offset) > rowsetSize )
				{
					resultSet->beforeFirst();
					resultSet->setPosRowInSet(0);
					return SQL_NO_DATA;
				}

				rowNumber = 0;
			}
			else
				rowNumber = sqlDiagCursorRowCount + offset + 1;
//				rowNumber += offset + 1;
		}
		else if( !offset )
		{
			resultSet->beforeFirst();
			resultSet->setPosRowInSet(0);
			return SQL_NO_DATA;
		}
		else if( offset > sqlDiagCursorRowCount )
		{
			resultSet->afterLast();
			resultSet->setPosRowInSet(sqlDiagCursorRowCount ? sqlDiagCursorRowCount - 1 : 0);
			return SQL_NO_DATA;
		}
		else
			rowNumber = offset - 1;
		break;

	case SQL_FETCH_NEXT:
	case SQL_FETCH_LAST:
		break;

	case SQL_FETCH_FIRST:
	case SQL_FETCH_PRIOR:
		break;
	
	case SQL_FETCH_BOOKMARK:

		if ( fetchBookmarkPtr )
		{
			if ( *(long*)fetchBookmarkPtr + offset < 1 )
			{
				resultSet->beforeFirst();
				resultSet->setPosRowInSet(0);
				return SQL_NO_DATA;
			}
			rowNumber = *(long*)fetchBookmarkPtr + offset - 1;
		}

		if( rowNumber >= sqlDiagCursorRowCount && resultSet->isAfterLast())
			return SQL_NO_DATA;

		break;

	default:
		return sqlReturn (SQL_ERROR, "HY106", "Fetch type out of range");
	}

	if( rowNumber >= 0 && rowNumber < sqlDiagCursorRowCount )
	{
		int nRow = 0;
		resultSet->setPosRowInSet(rowNumber);

		if ( !eof )
		{
			SQLINTEGER	*bindOffsetPtrSave = bindOffsetPtr;
			SQLUSMALLINT *statusPtr = implementationRowDescriptor->headArrayStatusPtr;
			SQLINTEGER	bindOffsetPtrTmp = bindOffsetPtr ? *bindOffsetPtr : 0;
			bindOffsetPtr = &bindOffsetPtrTmp;
			resultSet->setCurrentRowInBufferStaticCursor(rowNumber);
			implementationRowDescriptor->setBindOffsetPtr(&bindOffsetPtr);
			while ( nRow < rowsetSize && rowNumber < sqlDiagCursorRowCount )
			{
				resultSet->copyNextSqldaFromBufferStaticCursor();
				++rowNumber; // Should stand only here!!!
				implementationRowDescriptor->returnData(); 

				if ( statusPtr )
					statusPtr[nRow] = SQL_ROW_SUCCESS;
				bindOffsetPtrTmp += rowBindType;
				++nRow;
			}
			
			if (implementationRowDescriptor->headRowsProcessedPtr)
				*(SQLUINTEGER*)implementationRowDescriptor->headRowsProcessedPtr = nRow;

			if( !nRow )
				eof = true;
			else if( nRow < rowsetSize)
			{
				if( nRow && statusPtr)
					while(nRow < rowsetSize)
						statusPtr[nRow++] = SQL_ROW_NOROW;
			}

			bindOffsetPtr = bindOffsetPtrSave;
		}

		if ( eof )
		{
			if (implementationRowDescriptor->headRowsProcessedPtr)
				*(SQLINTEGER*)implementationRowDescriptor->headRowsProcessedPtr = 0;
			return SQL_NO_DATA;
		}
	}
	else if ( rowNumber < 0 )
	{
		rowNumber = 0;
		setValue (applicationRowDescriptor->getDescRecord (0), 0);
		if (implementationRowDescriptor->headRowsProcessedPtr)
			*(SQLINTEGER*)implementationRowDescriptor->headRowsProcessedPtr = 0;
		return SQL_NO_DATA;
	}
	else 
	{
		rowNumber = sqlDiagCursorRowCount - 1;
		setValue (applicationRowDescriptor->getDescRecord (0), 0);
		if (implementationRowDescriptor->headRowsProcessedPtr)
			*(SQLINTEGER*)implementationRowDescriptor->headRowsProcessedPtr = 0;
		return SQL_NO_DATA;
	}

	switch(orientation) 
	{
	case SQL_FETCH_RELATIVE: 
		if(offset<0) 
			resultSet->beforeFirst();
		else if(offset>0) 
			resultSet->afterLast();
		break;

	case SQL_FETCH_ABSOLUTE:
		if(offset==0)
			resultSet->beforeFirst();
		else
			resultSet->afterLast();
		break;

	case SQL_FETCH_NEXT:
	case SQL_FETCH_LAST:
		resultSet->afterLast();
		break;

	case SQL_FETCH_FIRST:
	case SQL_FETCH_PRIOR:
		resultSet->beforeFirst();
		break;

	case SQL_FETCH_BOOKMARK:
		resultSet->afterLast();
		break;
	}

	return sqlSuccess();
}


RETCODE OdbcStatement::sqlFetchScroll(int orientation, int offset)
{
#ifdef DEBUG
	char strTmp[128];
	sprintf(strTmp,"\t%s : offset %i : bookmark %i\n",strDebOrientFetch[orientation],offset,
								fetchBookmarkPtr ? *(long*)fetchBookmarkPtr : 0);
	OutputDebugString(strTmp); 
#endif
	clearErrors();

	if( enFetch == NoneFetch )
		enFetch = FetchScroll;

	fetched = true;

	if (!resultSet)
		return sqlReturn (SQL_ERROR, "24000", "Invalid cursor state");

	if( cursorType == SQL_CURSOR_FORWARD_ONLY && orientation != SQL_FETCH_NEXT )
		return sqlReturn (SQL_ERROR, "HY106", "Fetch type out of range");

	if (cancel)
	{
		releaseResultSet();
		return sqlReturn (SQL_ERROR, "S1008", "Operation canceled");
	}

	if ( isStaticCursor() )
		return sqlFetchScrollCursorStatic(orientation,offset);

	SQLINTEGER	*bindOffsetPtrSave = bindOffsetPtr;

	try
	{ 
		if(applicationRowDescriptor->headArraySize > 1)
		{
			SQLUSMALLINT *statusPtr = implementationRowDescriptor->headArrayStatusPtr;
			int nCountRow = applicationRowDescriptor->headArraySize;

			if ( !eof )
			{
				SQLINTEGER	bindOffsetPtrTmp = bindOffsetPtr ? *bindOffsetPtr : 0;
				bindOffsetPtr = &bindOffsetPtrTmp;
				int nRow = 0;
				while ( nRow < nCountRow && resultSet->next() )
				{
					returnData();
					
					if ( statusPtr )
						statusPtr[nRow] = SQL_SUCCESS;
					bindOffsetPtrTmp += rowBindType;

					++nRow;
				}
				
				if( !nRow || nRow < nCountRow)
				{
					eof = true;
					if( nRow && statusPtr)
						while(nRow < nCountRow)
							statusPtr[nRow++] = SQL_ROW_NOROW;
				}

				if (implementationRowDescriptor->headRowsProcessedPtr)
					*(SQLUINTEGER*)implementationRowDescriptor->headRowsProcessedPtr = nRow;

				setValue (applicationRowDescriptor->getDescRecord (0), 0);

				bindOffsetPtr = bindOffsetPtrSave;
			}

			if ( eof )
			{
				if (implementationRowDescriptor->headRowsProcessedPtr)
					*(SQLINTEGER*)implementationRowDescriptor->headRowsProcessedPtr = 0;
				return SQL_NO_DATA;
			}

			return sqlSuccess();
		}
		else
		{
			if (eof || !resultSet->next())
			{
				eof = true;
				if (implementationRowDescriptor->headRowsProcessedPtr)
					*(SQLINTEGER*)implementationRowDescriptor->headRowsProcessedPtr = 0;
				
				if(implementationRowDescriptor->headArrayStatusPtr)
					implementationRowDescriptor->headArrayStatusPtr[0] = SQL_ROW_NOROW;

				return SQL_NO_DATA;
			}
			if( implementationRowDescriptor->headArrayStatusPtr )
				implementationRowDescriptor->headArrayStatusPtr[0] = SQL_ROW_SUCCESS;

			setValue (applicationRowDescriptor->getDescRecord (0), 0);
		}
	}
	catch (SQLException& exception)
	{
		bindOffsetPtr = bindOffsetPtrSave;
		OdbcError *error = postError ("HY000", exception);
		error->setRowNumber (rowNumber);
		return SQL_ERROR;
	}

	return returnData();
}


RETCODE OdbcStatement::sqlExtendedFetch(int orientation, int offset, SQLUINTEGER *rowCountPointer, SQLUSMALLINT *rowStatusArray)
{
	clearErrors();
	if (!resultSet)
		return sqlReturn (SQL_ERROR, "24000", "Invalid cursor state");

	if (cancel)
	{
		releaseResultSet();
		return sqlReturn (SQL_ERROR, "S1008", "Operation canceled");
	}

	if( cursorType == SQL_CURSOR_FORWARD_ONLY && orientation != SQL_FETCH_NEXT )
		return sqlReturn (SQL_ERROR, "HY106", "Fetch type out of range");

	if( enFetch == NoneFetch )
		enFetch = ExtendedFetch;

	fetched = true;

	try
	{
		if(rowCountPointer)
			*rowCountPointer = 1;
		
		if (eof || !resultSet->next())
		{
			eof = true;
			if(rowCountPointer)
				*rowCountPointer = 0;
 			if(rowStatusArray)
 				rowStatusArray[0] = SQL_ROW_NOROW;
			return SQL_NO_DATA;
		}
		else
		{
			if(rowStatusArray)
				rowStatusArray[0] = SQL_ROW_SUCCESS;
		}		
	}
	catch (SQLException& exception)
	{
		OdbcError *error = postError ("HY000", exception);
		error->setRowNumber (rowNumber);
		return SQL_ERROR;
	}

	return returnData();

}

#ifdef DEBUG
char *strDebOrientSetPos[]=
{
	"SQL_POSITION",
	"SQL_REFRESH",
	"SQL_UPDATE",
	"SQL_DELETE"
};
#endif

RETCODE OdbcStatement::sqlSetPos (SQLUSMALLINT row, SQLUSMALLINT operation, SQLUSMALLINT lockType)
{
#ifdef DEBUG
	char strTmp[128];
	sprintf(strTmp,"\t%s : row %i : current bookmark %i\n",strDebOrientSetPos[operation],row,
								fetchBookmarkPtr ? *(long*)fetchBookmarkPtr : 0);
	OutputDebugString(strTmp); 
#endif

	switch ( operation )
	{
	case SQL_POSITION:
		if( fetchBookmarkPtr )
			rowNumber = (*(long*)fetchBookmarkPtr - 1) + row - 1;
		else
			rowNumber = row - 1;
		if( resultSet )
			resultSet->setPosRowInSet(rowNumber);
		fetched = true;
		break;
	case SQL_REFRESH:
		break;
	case SQL_UPDATE:
		break;
	case SQL_DELETE:
		break;
	}

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlSetScrollOptions (SQLUSMALLINT fConcurrency, SQLINTEGER crowKeyset, SQLUSMALLINT crowRowset)
{
	bool bOk;
    UWORD InfoType, InfoValuePtr;

    switch( crowKeyset )
    {
      case SQL_SCROLL_FORWARD_ONLY:
        InfoType = SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2;
        break;

      case SQL_SCROLL_STATIC:
        InfoType = SQL_STATIC_CURSOR_ATTRIBUTES2;
        break;

      case SQL_SCROLL_KEYSET_DRIVEN:
        InfoType = SQL_KEYSET_CURSOR_ATTRIBUTES2;
        break;

      case SQL_SCROLL_DYNAMIC:
        InfoType = SQL_DYNAMIC_CURSOR_ATTRIBUTES2;
        break;

      default:
        if ( crowKeyset > crowRowset )
            InfoType = SQL_KEYSET_CURSOR_ATTRIBUTES2;
        else
			return sqlReturn (SQL_ERROR, "S1107", "Row value out of range");
        break;
    }

    connection->sqlGetInfo (InfoType, &InfoValuePtr, sizeof(InfoValuePtr), 0);

	bOk = false;

	switch( fConcurrency )
	{
	case SQL_CONCUR_READ_ONLY:
		if ( InfoValuePtr & SQL_CA2_READ_ONLY_CONCURRENCY )
			bOk = true;
		break;
	case SQL_CONCUR_LOCK:
		if ( InfoValuePtr & SQL_CA2_LOCK_CONCURRENCY )
			bOk = true;
		break;
	case SQL_CONCUR_ROWVER:
		if ( InfoValuePtr & SQL_CA2_OPT_ROWVER_CONCURRENCY )
			bOk = true;
		break;
	case SQL_CONCUR_VALUES:
		if ( InfoValuePtr & SQL_CA2_OPT_VALUES_CONCURRENCY )
			bOk = true;
		break;
	default:
		return sqlReturn (SQL_ERROR, "S1108", "Concurrency option out of range");
	}

	if ( bOk == false )
		return sqlReturn (SQL_ERROR, "S1C00", "Driver not capable");

	sqlSetStmtAttr(SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)(int)InfoType, 0);
	sqlSetStmtAttr(SQL_ATTR_CONCURRENCY, (SQLPOINTER)(int)fConcurrency, 0);

	if ( crowKeyset > 0 )
		sqlSetStmtAttr(SQL_ATTR_KEYSET_SIZE, (SQLPOINTER)crowKeyset, 0);

	sqlSetStmtAttr(SQL_ROWSET_SIZE, (SQLPOINTER)crowKeyset, 0);

	return sqlSuccess();
}

bool OdbcStatement::setValue(DescRecord *record, int column)
{
	bool info = false;

	SQLSMALLINT &type = record->conciseType;
	SQLPOINTER pointer = record->dataPtr;
	SQLUINTEGER &bufferLength = record->length;
	SQLINTEGER * indicatorPointer = record->indicatorPtr;

	if( !pointer )
		return info;

	if ( bindOffsetPtr )
	{
		pointer = (SQLPOINTER)((char*)pointer + *bindOffsetPtr);
		indicatorPointer = (SQLINTEGER *)((char*)indicatorPointer + *bindOffsetPtr);
	}

	int realSqlType;
	SQLINTEGER	dataOffset = 0;
	int length = bufferLength;

	OdbcError *error = NULL;

	if( !pointer )
		return info;

	if (type == SQL_C_DEFAULT)
	{
		StatementMetaData *metaData = resultSet->getMetaData();
		type = getCType (metaData->getColumnType (column, realSqlType), 
						 metaData->isSigned (column));
	}

	switch (type)
		{
		case SQL_C_CHAR:
			{

			const char *string = RESULTS (getString (column));

			int dataRemaining = strlen(string) - dataOffset;
			int len = MIN(dataRemaining, (long)MAX(0, (long)bufferLength-1));
			 
			//Added by PR. If len is negative we get an AV
			//Added by NOMEY. and empty strings have len = 0
			if ( len >= 0 ) 
			{
				memcpy (pointer, string+dataOffset, len);
				((char*) (pointer)) [len] = 0;
			}

			if (len && len < dataRemaining)
			{
				error = postError (new OdbcError (0, "01004", "Data truncated"));
				info = true;
			}

			length = len;
			dataOffset += len;

			if (!info)
				dataOffset = 0;
			}
			break;

		case SQL_C_SHORT:
		case SQL_C_SSHORT:
		case SQL_C_USHORT:
			*((short*) pointer) = RESULTS (getShort (column));
			length = sizeof(short);
			break;

		case SQL_C_LONG:
		case SQL_C_SLONG:
		case SQL_C_ULONG:
			if( column == 0 )
				*((long*) pointer) = rowNumber+1;
			else
				*((long*) pointer) = RESULTS (getInt (column));
			length = sizeof(long);
			break;

		case SQL_C_FLOAT:
			*((float*) pointer) = RESULTS (getFloat (column));
			length = sizeof(float);
			break;

		case SQL_C_DOUBLE:
			{
				int scale = metaData->getScale (column);
				double &val = *((double*) pointer);
				val = RESULTS (getDouble (column));
			
				if(scale)
					val /= (QUAD)listScale[scale];
			}	
			length = sizeof(double);
			break;

		case SQL_C_TINYINT:
		case SQL_C_STINYINT:
		case SQL_C_UTINYINT:
			*((char*) pointer) = RESULTS (getByte (column));
			length = sizeof(char);
			break;

		case SQL_C_SBIGINT:
		case SQL_C_UBIGINT:
			*((QUAD*) pointer) = RESULTS (getQuad (column));
			length = sizeof(QUAD);
			break;

//		case SQL_TYPE_DATE:
		case SQL_C_TYPE_DATE:
		case SQL_C_DATE:
			{
			OdbcDateTime converter;
			DateTime dateTime = RESULTS (getDate (column));
			tagDATE_STRUCT *var = (tagDATE_STRUCT*) pointer;
			converter.convert (&dateTime, var);
			length = sizeof(tagDATE_STRUCT);
			}
			break;

//		case SQL_TYPE_TIMESTAMP:
		case SQL_C_TYPE_TIMESTAMP:
		case SQL_C_TIMESTAMP:
			{
			TimeStamp timestamp = RESULTS (getTimestamp (column));
			tagTIMESTAMP_STRUCT *var = (tagTIMESTAMP_STRUCT*) pointer;
			OdbcDateTime converter;
			converter.convert (&timestamp, var);
			length = sizeof(tagTIMESTAMP_STRUCT);
			}
			break;

		case SQL_C_TYPE_TIME:
		case SQL_C_TIME:
			{
			SqlTime sqlTime = RESULTS (getTime (column));
			tagTIME_STRUCT *var = (tagTIME_STRUCT*) pointer;
			long minutes = sqlTime.timeValue / (ISC_TIME_SECONDS_PRECISION * 60);
			var->hour = (SQLUSMALLINT)(minutes / 60);
			var->minute = (SQLUSMALLINT)(minutes % 60);
			var->second = (short)((sqlTime.timeValue / ISC_TIME_SECONDS_PRECISION) % 60);
			length = sizeof (tagTIME_STRUCT);
			}
			break;

		case SQL_C_BINARY:
			{
			//for now, just get value so the wasNull check will work
			Blob* blob = RESULTS (getBlob(column));
			int dataRemaining = blob->length() - dataOffset;
			int len = MIN(dataRemaining, (long)bufferLength);
			 
			blob->getBytes (dataOffset, len, pointer);

			if (len < dataRemaining)
			{
				error = postError (new OdbcError (0, "01004", "Data truncated"));
				info = true;
			}
				
			length = len;
			dataOffset += len;

			if (!info)
				dataOffset = 0;
			}
			break;	

		case SQL_DECIMAL:
		case SQL_C_NUMERIC:
			{
				char *var = (char*) pointer;
				QUAD &number = *(QUAD*)(var+3) = RESULTS (getQuad(column));
				*var++=(char)metaData->getPrecision (column);
				*var=(char)metaData->getScale (column);

				if( number && *var )
				{
					metaData->getColumnType (column, realSqlType);
					switch ( realSqlType )
					{
					case SQL_C_SHORT:
					case SQL_C_USHORT:
					case SQL_C_SSHORT:
					case SQL_C_LONG:
					case SQL_C_ULONG:
					case SQL_C_SLONG:
					case SQL_C_DOUBLE:
//						number *= (QUAD)listScale[*var];
						break;
					}
				}

				++var;

				if ( number < 0 )
					number = -number,
					*var++=0;
				else
					*var++=1;
				
				length = 0;
			}
			break;	

		case SQL_C_BIT:
		//case SQL_C_BOOKMARK:
		//case SQL_C_VARBOOKMARK:
		//case SQL_C_GUID:
			//break;

		default:
			error = postError (new OdbcError (0, "HYC00", "Optional feature not implemented"));
			info = true;
		}

	if (error)
		error->setColumnNumber (column, rowNumber);

	if (indicatorPointer && column)
		{
		if (RESULTS (wasNull()))
			*indicatorPointer = SQL_NULL_DATA;
		else
			*indicatorPointer = length;
		}

	return info;
}


RETCODE OdbcStatement::setValue(Binding * binding, int column)
{
	RETCODE retinfo = SQL_SUCCESS;
	int length = binding->bufferLength;
	int type = binding->cType;
	int realSqlType;
	OdbcError *error = NULL;

	if (type == SQL_C_DEFAULT)
	{
		StatementMetaData *metaData = resultSet->getMetaData();
		type = getCType (metaData->getColumnType (column, realSqlType), 
						 metaData->isSigned (column));
	}

	switch (type)
		{
		case SQL_C_CHAR:
			{
			const char *string = RESULTS (getString (column));
			
			if ( fetched == true )
			{
				fetched = false;
				binding->dataOffset = 0;
			}

			int dataRemaining = strlen(string) - binding->dataOffset;

			if (!dataRemaining && binding->dataOffset)
			{
				binding->dataOffset = 0;
				if( binding->bufferLength )
					retinfo = SQL_NO_DATA;
			}
			else
			{
				if ( !binding->bufferLength )
					length = dataRemaining;
				else
				{
					int len = MIN(dataRemaining, MAX(0, (long)binding->bufferLength-1));
					 
					if ( len > 0 ) 
					{
						memcpy (binding->pointer, string+binding->dataOffset, len);
						((char*) (binding->pointer)) [len] = 0;
					}

					if ( len < dataRemaining)
					{
						error = postError (new OdbcError (0, "01004", "Data truncated"));
						retinfo = SQL_SUCCESS_WITH_INFO;
					}

					binding->dataOffset += len;
					length = len;
				}
			}
			}
			break;

		case SQL_C_SHORT:
		case SQL_C_SSHORT:
		case SQL_C_USHORT:
			*((short*) binding->pointer) = RESULTS (getShort (column));
			length = sizeof(short);
			break;

		case SQL_C_LONG:
		case SQL_C_SLONG:
		case SQL_C_ULONG:
			*((long*) binding->pointer) = RESULTS (getInt (column));
			length = sizeof(long);
			break;

		case SQL_C_FLOAT:
			*((float*) binding->pointer) = RESULTS (getFloat (column));
			length = sizeof(float);
			break;

		case SQL_C_DOUBLE:
			{
				int scale = metaData->getScale (column);
				double &val = *((double*) binding->pointer);
				val = RESULTS (getDouble (column));
			
				if(scale)
					val /= (QUAD)listScale[scale];
			}	
			length = sizeof(double);
			break;

		case SQL_C_TINYINT:
		case SQL_C_STINYINT:
		case SQL_C_UTINYINT:
			*((char*) binding->pointer) = RESULTS (getByte (column));
			length = sizeof(char);
			break;

		case SQL_C_SBIGINT:
		case SQL_C_UBIGINT:
			*((QUAD*) binding->pointer) = RESULTS (getQuad (column));
			length = sizeof(QUAD);
			break;

		case SQL_C_TYPE_DATE:
		case SQL_C_DATE:
			{
			OdbcDateTime converter;
			DateTime dateTime = RESULTS (getDate (column));
			tagDATE_STRUCT *var = (tagDATE_STRUCT*) binding->pointer;
			converter.convert (&dateTime, var);
			length = sizeof(tagDATE_STRUCT);
			}
			break;

		case SQL_C_TYPE_TIMESTAMP:
		case SQL_C_TIMESTAMP:
			{
			TimeStamp timestamp = RESULTS (getTimestamp (column));
			tagTIMESTAMP_STRUCT *var = (tagTIMESTAMP_STRUCT*) binding->pointer;
			OdbcDateTime converter;
			converter.convert (&timestamp, var);
			length = sizeof(tagTIMESTAMP_STRUCT);
			}
			break;

		case SQL_C_TYPE_TIME:
		case SQL_C_TIME:
			{
			SqlTime sqlTime = RESULTS (getTime (column));
			tagTIME_STRUCT *var = (tagTIME_STRUCT*) binding->pointer;
			long minutes = sqlTime.timeValue / (ISC_TIME_SECONDS_PRECISION * 60);
			var->hour = (SQLUSMALLINT)(minutes / 60);
			var->minute = (SQLUSMALLINT)(minutes % 60);
			var->second = (short)((sqlTime.timeValue / ISC_TIME_SECONDS_PRECISION) % 60);
			length = sizeof (tagTIME_STRUCT);
			}
			break;

		case SQL_C_BINARY:
			{
			Blob* blob = RESULTS (getBlob(column));
			
			if ( fetched == true )
			{
				fetched = false;
				binding->dataOffset = 0;
			}

			int dataRemaining = blob->length() - binding->dataOffset;

			if (!dataRemaining && binding->dataOffset)
			{
				binding->dataOffset = 0;
				if( binding->bufferLength )
					retinfo = SQL_NO_DATA;
			}
			else
			{
				if ( !binding->bufferLength )
					length = dataRemaining;
				else
				{
					int len = MIN(dataRemaining, binding->bufferLength);
					 
					if ( len > 0 ) 
						blob->getBytes (binding->dataOffset, len, binding->pointer);

					if ( len < dataRemaining)
					{
						error = postError (new OdbcError (0, "01004", "Data truncated"));
						retinfo = SQL_SUCCESS_WITH_INFO;
					}

					binding->dataOffset += len;
					length = len;
				}
			}
			}
			break;	

		case SQL_DECIMAL:
		case SQL_C_NUMERIC:
			{
				char *var = (char*) binding->pointer;
				QUAD &number = *(QUAD*)(var+3) = RESULTS (getQuad(column));
				*var++=(char)metaData->getPrecision (column);
				*var=(char)metaData->getScale (column);

				if( number && *var )
				{
					metaData->getColumnType (column, realSqlType);
					switch ( realSqlType )
					{
					case SQL_C_SHORT:
					case SQL_C_USHORT:
					case SQL_C_SSHORT:
					case SQL_C_LONG:
					case SQL_C_ULONG:
					case SQL_C_SLONG:
					case SQL_C_DOUBLE:
//						number *= (QUAD)listScale[*var];
						break;
					}
				}

				++var;

				if ( number < 0 )
					number = -number,
					*var++=0;
				else
					*var++=1;
				
				length = 0;
			}
			break;	

		case SQL_C_BIT:
		//case SQL_C_BOOKMARK:
		//case SQL_C_VARBOOKMARK:
		//case SQL_C_GUID:
			//break;

		default:
			error = postError (new OdbcError (0, "HYC00", "Optional feature not implemented"));
			retinfo = SQL_SUCCESS_WITH_INFO;
		}

	if (error)
		error->setColumnNumber (column, rowNumber);

	if (binding->indicatorPointer)
	{
		if (RESULTS (wasNull()))
			*binding->indicatorPointer = SQL_NULL_DATA;
		else
			*binding->indicatorPointer = length;
	}

	return retinfo;
}

RETCODE OdbcStatement::sqlColumns(SQLCHAR * catalog, int catLength, SQLCHAR * schema, int schemaLength, SQLCHAR * table, int tableLength, SQLCHAR * column, int columnLength)
{
	clearErrors();
	releaseStatement();
	char temp [1024], *p = temp;

	const char *cat = getString (&p, catalog, catLength, NULL);
	const char *scheme = getString (&p, schema, schemaLength, NULL);
	const char *tbl = getString (&p, table, tableLength, NULL);
	const char *col = getString (&p, column, columnLength, NULL);

	try
		{
		DatabaseMetaData *metaData = connection->getMetaData();
		setResultSet (metaData->getColumns (cat, scheme, tbl, col));
		}
	catch (SQLException& exception)
		{
		postError ("HY000", exception);
		return SQL_ERROR;
		}

	return sqlSuccess();
}













































RETCODE OdbcStatement::sqlFreeStmt(int option)
{
	clearErrors();
	try
		{
		switch (option)
			{
			case SQL_CLOSE:
				releaseResultSet();
				if(statement)
					statement->clearResults();
				break;

			case SQL_UNBIND:
				releaseBindings();
				break;

			case SQL_RESET_PARAMS:
				releaseParameters();
				break;
			}
		}
	catch (SQLException& exception)
		{
		postError ("HY000", exception);
		return SQL_ERROR;
		}

	return sqlSuccess();
}

void OdbcStatement::releaseBindings()
{
	applicationRowDescriptor->removeRecords();
	implementationRowDescriptor->delAllBindColumn();
//Added by RM
	numberGetDataBindings = 0;
	columnPrevGetDataBinding = -1;

	if (getDataBindings)
		{
		delete [] getDataBindings;
		getDataBindings = NULL;
		}

}

void OdbcStatement::releaseParameters()
{
	applicationParamDescriptor->removeRecords();
	paramsetSize = 0;
	paramsProcessedPtr = NULL;
}

RETCODE OdbcStatement::sqlStatistics(SQLCHAR * catalog, int catLength, 
									 SQLCHAR * schema, int schemaLength, 
									 SQLCHAR * table, int tableLength, 
									 int unique, int reservedSic)
{
	clearErrors();
	releaseStatement();
	char temp [1024], *p = temp;

	const char *cat = getString (&p, catalog, catLength, NULL);
	const char *scheme = getString (&p, schema, schemaLength, NULL);
	const char *tbl = getString (&p, table, tableLength, NULL);

	try
	{
		DatabaseMetaData *metaData = connection->getMetaData();
		setResultSet (metaData->getIndexInfo (cat, scheme, tbl, 
										unique == SQL_INDEX_UNIQUE, 
										reservedSic == SQL_QUICK));
	}
	catch (SQLException& exception)
	{
		postError ("HY000", exception);
		return SQL_ERROR;
	}

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlPrimaryKeys(SQLCHAR * catalog, int catLength, SQLCHAR * schema, int schemaLength, SQLCHAR * table, int tableLength)
{
	clearErrors();
	releaseStatement();
	char temp [1024], *p = temp;

	const char *cat = getString (&p, catalog, catLength, NULL);
	const char *scheme = getString (&p, schema, schemaLength, NULL);
	const char *tbl = getString (&p, table, tableLength, NULL);

	try
		{
		DatabaseMetaData *metaData = connection->getMetaData();
		setResultSet (metaData->getPrimaryKeys (cat, scheme, tbl));
		}
	catch (SQLException& exception)
		{
		postError ("HY000", exception);
		return SQL_ERROR;
		}

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlForeignKeys (SQLCHAR * pkCatalog, int pkCatLength, 
								       SQLCHAR * pkSchema, int pkSchemaLength, 
									   SQLCHAR * pkTable, int pkTableLength, 
									   SQLCHAR * fkCatalog, int fkCatalogLength, 
									   SQLCHAR * fkSchema, int fkSchemaLength, 
									   SQLCHAR * fkTable, int fkTableLength)
{
	clearErrors();
	releaseStatement();
	char temp [1024], *p = temp;

	const char *pkCat = getString (&p, pkCatalog, pkCatLength, NULL);
	const char *pkScheme = getString (&p, pkSchema, pkSchemaLength, NULL);
	const char *pkTbl = getString (&p, pkTable, pkTableLength, NULL);
	const char *fkCat = getString (&p, fkCatalog, fkCatalogLength, NULL);
	const char *fkScheme = getString (&p, fkSchema, fkSchemaLength, NULL);
	const char *fkTbl = getString (&p, fkTable, fkTableLength, NULL);

	try
		{
		DatabaseMetaData *metaData = connection->getMetaData();
		setResultSet (metaData->getCrossReference (pkCat, pkScheme,pkTbl,fkCat,fkScheme,fkTbl));
		}
	catch (SQLException& exception)
		{
		postError ("HY000", exception);
		return SQL_ERROR;
		}

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlNumResultCols(SWORD * columns)
{
	clearErrors();

	if (resultSet)
		try
			{
			StatementMetaData *metaData = resultSet->getMetaData();
			*columns = metaData->getColumnCount();
			}
		catch (SQLException& exception)
			{
			postError ("HY000", exception);
			return SQL_ERROR;
			}
	else
		*columns = 0;
	return sqlSuccess();
}

RETCODE OdbcStatement::sqlNumParams(SWORD * params)
{
	clearErrors();
	
	if (statement)
		try
		{
			if( params )
				*params = statement->getNumParams();
		}
		catch (SQLException& exception)
		{
			postError ("HY000", exception);
			return SQL_ERROR;
		}
	else if( params )
		*params = 0;

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlDescribeCol(int col, 
									  SQLCHAR * colName, int nameSize, SWORD * nameLength, 
									  SWORD * sqlType, 
									  UDWORD * precision, 
									  SWORD * scale, 
									  SWORD * nullable)
{
	clearErrors();

	try
	{
		int realSqlType;
		StatementMetaData *metaData = resultSet->getMetaData();
		const char *name = metaData->getColumnName (col);
		setString (name, colName, nameSize, nameLength);
		if (sqlType)
			*sqlType = metaData->getColumnType (col, realSqlType);
		if (scale)
			*scale = metaData->getScale (col);
		if (precision)
			*precision = metaData->getPrecision (col);
		if (nullable)
			*nullable = (metaData->isNullable (col)) ? SQL_NULLABLE : SQL_NO_NULLS;
#ifdef DEBUG
		char tempDebugStr [128];
		sprintf (tempDebugStr, "Column %.2d %31s has type %.3d, scale %.3d, precision %.3d \n", 
				col,
				metaData->getColumnName(col),
				metaData->getColumnType (col, realSqlType),
				metaData->getScale (col),
				metaData->getPrecision (col)
				);
		OutputDebugString (tempDebugStr);
#endif
	}
	catch (SQLException& exception)
	{
		postError ("HY000", exception);
		return SQL_ERROR;
	}

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlGetData(int column, int cType, PTR pointer, int bufferLength, SDWORD * indicatorPointer)
{
	clearErrors();

	if ( isStaticCursor() )
	{
//		if ( cType == SQL_ARD_TYPE )
		resultSet->getDataFromStaticCursor (column,cType,pointer,bufferLength,indicatorPointer);
	}
//From RM
	int retcode = sqlBindCol(column, cType, pointer, bufferLength, indicatorPointer, &getDataBindings, &numberGetDataBindings);

	if (retcode && retcode != SQL_SUCCESS_WITH_INFO)
		return retcode;

#pragma FB_COMPILER_MESSAGE("sqlBindCol - to use when this column was not connected; FIXME!")
// Problem with length of a line StaticCursor
// When blob a line is output to string
// If cType is SQL_ARD_TYPE, the driver uses the type identifier 
// specified in the SQL_DESC_CONCISE_TYPE field of the ARD. 
// If it is SQL_C_DEFAULT, the driver selects the default C data 
// type based upon the SQL data type of the source.

	Binding* binding = getDataBindings + column;

	try
	{
		if ((retcode = setValue (binding, column)))
		{
			if ( retcode == SQL_NO_DATA )
				return SQL_NO_DATA;
			return SQL_SUCCESS_WITH_INFO;
		}
	}
	catch (SQLException& exception)
	{
		postError ("HY000", exception);
		return SQL_ERROR;
	}

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlExecute()
{
	clearErrors();
	int retcode = SQL_SUCCESS;

	try
	{
		enFetch = NoneFetch;
		releaseResultSet();
		parameterNeedData = 0;
		retcode = executeStatement();
	}
	catch (SQLException& exception)
	{
		postError ("HY000", exception);
		retcode = SQL_ERROR;
	}

	if (retcode && retcode != SQL_SUCCESS_WITH_INFO)
		return retcode;

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlExecuteDirect(SQLCHAR * sql, int sqlLength)
{
	int retcode = sqlPrepare (sql, sqlLength, true);
	if (retcode && retcode != SQL_SUCCESS_WITH_INFO)
		return retcode;
	try
	{
		enFetch = NoneFetch;
		parameterNeedData = 0;
		retcode = executeStatement();
	}
	catch (SQLException& exception)
	{
		postError ("HY000", exception);
		return SQL_ERROR;
	}

	if (retcode && retcode != SQL_SUCCESS_WITH_INFO)
		return retcode;

	return sqlSuccess();		
}

ResultSet* OdbcStatement::getResultSet()
{
	eof = false;
	cancel = false;
	numberColumns = 0;

	releaseResultSet();

	if (!statement->getMoreResults())
		return NULL;

	setResultSet (statement->getResultSet());

	return resultSet;
}

RETCODE OdbcStatement::sqlDescribeParam(int parameter, SWORD * sqlType, UDWORD * precision, SWORD * scale, SWORD * nullable)
{
	clearErrors();
	StatementMetaData *metaData = statement->getStatementMetaDataIPD();
	int realSqlType;

	try
	{
		if (sqlType)
			*sqlType = metaData->getColumnType (parameter, realSqlType);

		if (precision)
			*precision = metaData->getPrecision (parameter);

		if (scale)
			*scale = metaData->getScale (parameter);

		if (nullable)
			*nullable = (metaData->isNullable (parameter)) ? SQL_NULLABLE : SQL_NO_NULLS;
	}
	catch (SQLException& exception)
	{
		postError ("HY000", exception);
		return SQL_ERROR;
	}

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlSetParam (int parameter, int cType, int sqlType, int precision, int scale, PTR ptr, SDWORD * length)
{	
	clearErrors();
	return sqlBindParameter (parameter, SQL_PARAM_INPUT_OUTPUT, cType, sqlType, precision, scale, ptr, SQL_SETPARAM_VALUE_MAX, length);
}

RETCODE OdbcStatement::sqlBindParameter(int parameter, int type, int cType, 
										int sqlType, int precision, int scale, 
										PTR ptr, int bufferLength, SDWORD * length)
{
	clearErrors();

	if (parameter <= 0)
		return sqlReturn (SQL_ERROR, "S1093", "Invalid parameter number");

	int parametersNeeded = parameter;

	try
	{
		if (cType == SQL_C_DEFAULT)
			switch (sqlType)
			{
			 case SQL_CHAR:
			 case SQL_VARCHAR:
			 case SQL_LONGVARCHAR:
			 case SQL_DECIMAL:
			 case SQL_NUMERIC:
				cType = SQL_C_CHAR;
				break;
			 case SQL_BIT:
				cType = SQL_C_BIT;
				break;
			 case SQL_SMALLINT:
				cType = SQL_C_SSHORT;
				break;
			 case SQL_INTEGER:
				cType = SQL_C_SLONG;
				break;
			 case SQL_BIGINT:
				cType = SQL_C_CHAR;
				break;
			 case SQL_FLOAT:
			 case SQL_REAL:
				cType = SQL_C_FLOAT;
				break;
			 case SQL_DOUBLE:
				cType = SQL_C_DOUBLE;
				break;
			 case SQL_BINARY:
			 case SQL_VARBINARY:
			 case SQL_LONGVARBINARY:
				cType = SQL_C_BINARY;
				break;
			 case SQL_DATE:
				cType = SQL_C_DATE;
				break;
			 case SQL_TIME:
				cType = SQL_C_TIME;
				break;
			 case SQL_TIMESTAMP:
				cType = SQL_C_TIMESTAMP;
				break;
			 case SQL_TYPE_DATE:
				cType = SQL_C_TYPE_DATE;
				break;
			 case SQL_TYPE_TIME:
				cType = SQL_C_TYPE_TIME;
				break;
			 case SQL_TYPE_TIMESTAMP:
				cType = SQL_C_TYPE_TIMESTAMP;
				break;
			}

		switch (cType)
		{
		case SQL_C_CHAR:
		case SQL_C_SHORT:
		case SQL_C_SSHORT:
		case SQL_C_USHORT:
		case SQL_C_LONG:
		case SQL_C_SLONG:
		case SQL_C_ULONG:
		case SQL_C_FLOAT:
		case SQL_C_DOUBLE:
		case SQL_C_BIT:
		case SQL_C_TINYINT:
		case SQL_C_STINYINT:
		case SQL_C_UTINYINT:
		case SQL_C_SBIGINT:
		case SQL_C_UBIGINT:
		case SQL_C_BINARY:
		case SQL_C_DATE:
		case SQL_C_TIME:
		case SQL_C_TIMESTAMP:
		case SQL_C_TYPE_DATE:
		case SQL_C_TYPE_TIME:
		case SQL_C_TYPE_TIMESTAMP:
		case SQL_C_NUMERIC:
		case SQL_DECIMAL:
		//case SQL_C_BOOKMARK:
		//case SQL_C_VARBOOKMARK:
		//case SQL_C_GUID:
			break;

		default:
			return sqlReturn (SQL_ERROR, "S1C00", "Driver not capable");
		}

		DescRecord *record = applicationParamDescriptor->getDescRecord (parameter);

		switch (sqlType)
		{
		case SQL_DATE:
		case SQL_TYPE_DATE:
		case SQL_TIME:
		case SQL_TYPE_TIME:
		case SQL_TIMESTAMP:
		case SQL_TYPE_TIMESTAMP:
			record->type = sqlType;
			break;
		default:
			record->type = cType;
		}

		record->conciseType = cType;
		record->dataPtr = ptr;
		record->octetLength = bufferLength;
		record->length = bufferLength;
		record->octetLengthPtr = length;
		record->indicatorPtr = length;
#pragma FB_COMPILER_MESSAGE("Definition data_at_exec!!! FIXME!")
		record->data_at_exec = length && type != SQL_PARAM_OUTPUT 
					&& (*length == SQL_DATA_AT_EXEC || *length <= SQL_LEN_DATA_AT_EXEC_OFFSET );
		record->startedTransfer = false;

		record = implementationParamDescriptor->getDescRecord (parameter);

		record->parameterType = type; // SQL_PARAM_INPUT, SQL_PARAM_OUTPUT, SQL_PARAM_INPUT_OUTPUT
		record->type = sqlType;
		record->conciseType = sqlType;
		record->indicatorPtr = length;

		switch (sqlType)
		{
		 case SQL_TIME:
		 case SQL_TYPE_TIME:
		 case SQL_TIMESTAMP:
		 case SQL_TYPE_TIMESTAMP:
			 record->precision = scale;
		 case SQL_CHAR:
		 case SQL_VARCHAR:
		 case SQL_LONGVARCHAR:
		 case SQL_BINARY:
		 case SQL_VARBINARY:
		 case SQL_LONGVARBINARY:
		 case SQL_DATE:
		 case SQL_TYPE_DATE:
			 record->length = precision;
			break;
		 case SQL_DECIMAL:
		 case SQL_NUMERIC:
			 record->scale = scale;
		 case SQL_REAL:
		 case SQL_FLOAT:
		 case SQL_DOUBLE:
			 record->precision = precision;
			 break;
		}
	}
	catch (SQLException& exception)
	{
		postError ("HY000", exception);
		return SQL_ERROR;
	}

	return sqlSuccess();
}

void OdbcStatement::setParameter(DescRecord *record,int parameter)
{
    clearErrors();

	DescRecord *recordIPD = implementationParamDescriptor->getDescRecord (parameter);
	SQLINTEGER *indicatorPointer = record->indicatorPtr;
	char * pointer = (char *)record->dataPtr;


	if (indicatorPointer && *indicatorPointer == SQL_NULL_DATA)
	{
		statement->setNull(parameter, 0);
		return;
	}

	try
	{
#pragma FB_COMPILER_MESSAGE("Modify SQL 92 FIXME!")
		int type = record->conciseType == SQL_C_DEFAULT ? record->type : record->conciseType;

		switch ( type )
		{
		case SQL_C_CHAR:
			{
				if (indicatorPointer)
				{
					switch( *indicatorPointer )
					{
					case SQL_NTS:
					case SQL_DEFAULT_PARAM:
						statement->setString (parameter, pointer );
						break;
						
					default:                       
						statement->setString (parameter, pointer, *indicatorPointer < 0 ? strlen((char*)pointer) : *indicatorPointer );
						break;
					}
				}
				else
				{
					statement->setString (parameter, pointer );
				}
				switch(recordIPD->conciseType)
				{
				case SQL_DATE:
				case SQL_TIME:
				case SQL_TIMESTAMP:
				case SQL_TYPE_DATE:
				case SQL_TYPE_TIME:
				case SQL_TYPE_TIMESTAMP:
					statement->convStringData (parameter);
					break;
				}
			}
			break;

		case SQL_C_SHORT:
		case SQL_C_SSHORT:
		case SQL_C_USHORT:
			statement->setShort (parameter, *(short*) pointer);
			break;

		case SQL_C_LONG:
		case SQL_C_SLONG:
		case SQL_C_ULONG:
			statement->setInt (parameter, *(long*) pointer);
			break;

		case SQL_C_FLOAT:
			statement->setFloat (parameter, *(float*) pointer);
			break;

		case SQL_C_DOUBLE:
			statement->setDouble (parameter, *(double*) pointer);
			break;

		case SQL_C_TINYINT:
		case SQL_C_STINYINT:
		case SQL_C_UTINYINT:
			statement->setByte (parameter, *(char*) pointer);
			break;

		case SQL_C_SBIGINT:
		case SQL_C_UBIGINT:
			statement->setQuad (parameter, *(QUAD*) pointer);
			break;

		case SQL_C_TIMESTAMP:
		case SQL_C_TYPE_TIMESTAMP:
			{
			OdbcDateTime converter;
			tagTIMESTAMP_STRUCT *var = (tagTIMESTAMP_STRUCT*) pointer;
			TimeStamp timestamp;
		    converter.convert (var, &timestamp);
			statement->setTimestamp ( parameter, timestamp);
			}
			break;

		case SQL_C_DATE:
		case SQL_C_TYPE_DATE:
			{
			OdbcDateTime converter;
			tagDATE_STRUCT *var = (tagDATE_STRUCT*) pointer;
			DateTime dateTime;
			converter.convert (var, &dateTime);
			statement->setDate (parameter, dateTime);
			}
			break;

		case SQL_C_TIME:
		case SQL_C_TYPE_TIME:
			{
			tagTIME_STRUCT *var = (tagTIME_STRUCT*) pointer;
			SqlTime dateTime;
			dateTime.timeValue = var->hour * 60 * 60 + var->minute * 60 + var->second;
			statement->setTime (parameter, dateTime);
			}
			break;

		case SQL_C_BINARY:
			if (!record->data_at_exec)
               	statement->setBytes(parameter, record->length, pointer);
			break;

		case SQL_C_NUMERIC:
		case SQL_DECIMAL:
			{
				QUAD &number = *(QUAD*)(pointer+3);
				char scale = *(pointer+1);
				if(scale)number /= (QUAD)listScale[scale];
				statement->setQuad(parameter, number);
			}
			break;

		case SQL_C_BIT:
		//case SQL_C_BOOKMARK:
		//case SQL_C_VARBOOKMARK:
		//case SQL_C_GUID:
			//break;

		default:
			postError (new OdbcError (0, "HYC00", "Optional feature not implemented"));
			return;
		}
	}
	catch (SQLException& exception)
	{
		postError ("HY000", exception);
		return;
	}

}


RETCODE OdbcStatement::sqlCancel()
{
	try
		{
		clearErrors();
		cancel = true;
		}
	catch (SQLException& exception)
		{
		postError ("HY000", exception);
		return SQL_ERROR;
		}

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlProcedures(SQLCHAR * catalog, int catLength, SQLCHAR * schema, int schemaLength, SQLCHAR * proc, int procLength)
{
	try
		{
		clearErrors();
		releaseStatement();
		char temp [1024], *p = temp;

		const char *cat = getString (&p, catalog, catLength, NULL);
		const char *scheme = getString (&p, schema, schemaLength, NULL);
		const char *procedures = getString (&p, proc, procLength, NULL);

		DatabaseMetaData *metaData = connection->getMetaData();
		setResultSet (metaData->getProcedures (cat, scheme, procedures));
		}
	catch (SQLException& exception)
		{
		postError ("HY000", exception);
		return SQL_ERROR;
		}

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlProcedureColumns(SQLCHAR * catalog, int catLength, SQLCHAR * schema, int schemaLength, SQLCHAR * proc, int procLength, SQLCHAR * col, int colLength)
{
	clearErrors();
	releaseStatement();
	char temp [1024], *p = temp;

	const char *cat = getString (&p, catalog, catLength, NULL);
	const char *scheme = getString (&p, schema, schemaLength, NULL);
	const char *procedures = getString (&p, proc, procLength, NULL);
	const char *columns = getString (&p, col, colLength, NULL);

	try
		{
		DatabaseMetaData *metaData = connection->getMetaData();
		setResultSet (metaData->getProcedureColumns (cat, scheme, procedures, columns));
		}
	catch (SQLException& exception)
		{
		postError ("HY000", exception);
		return SQL_ERROR;
		}

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlSetCursorName(SQLCHAR * name, int nameLength)
{
	clearErrors();
	char temp [1024], *p = temp;

	cursorName = getString (&p, name, nameLength, NULL);

	try
		{
		statement->setCursorName (cursorName);
		}
	catch (SQLException& exception)
		{
		postError ("HY000", exception);
		return SQL_ERROR;
		}

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlCloseCursor()
{
	clearErrors();

	try
		{
		releaseResultSet();
		}
	catch (SQLException& exception)
		{
		postError ("HY000", exception);
		return SQL_ERROR;
		}

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlGetStmtAttr(int attribute, SQLPOINTER ptr, int bufferLength, SQLINTEGER * lengthPtr)
{
	clearErrors();
	long value;
	char *string = NULL;

	try
		{
		switch (attribute)
			{
			case 11999:
				return statement->getStmtPlan(ptr,bufferLength,lengthPtr);

			case 11998:
				return statement->getStmtType(ptr,bufferLength,lengthPtr);

			case 11997:
				return statement->getStmtInfoCountRecords(ptr,bufferLength,lengthPtr);

			case SQL_ATTR_APP_ROW_DESC:
				value = (long) applicationRowDescriptor;
				TRACE02(SQL_ATTR_APP_ROW_DESC,value);
				break;

			case SQL_ATTR_APP_PARAM_DESC:
				value = (long) applicationParamDescriptor;
				TRACE02(SQL_ATTR_APP_PARAM_DESC,value);
				break;

			case SQL_ATTR_IMP_ROW_DESC:
				value = (long) implementationRowDescriptor;
				TRACE02(SQL_ATTR_IMP_ROW_DESC,value);
				break;

			case SQL_ATTR_IMP_PARAM_DESC:
				value = (long) implementationParamDescriptor;
				TRACE02(SQL_ATTR_IMP_PARAM_DESC,value);
				break;

			case SQL_ATTR_CURSOR_TYPE:
				value = cursorType;
				TRACE02(SQL_ATTR_CURSOR_TYPE,value);
				break;

			case SQL_ATTR_CONCURRENCY:
				value = currency;
				TRACE02(SQL_ATTR_CONCURRENCY,value);
				break;

		    case SQL_ATTR_ROW_ARRAY_SIZE:
				value = applicationRowDescriptor->headArraySize;
				TRACE02(SQL_ATTR_ROW_ARRAY_SIZE,value);
				break;

			case SQL_ROWSET_SIZE:
		        value = rowArraySize;
				TRACE02(SQL_ROWSET_SIZE,value);
				break;

			case SQL_ATTR_MAX_ROWS:					// SQL_MAX_ROWS 1
				value = maxRows;
				TRACE02(SQL_ATTR_MAX_ROWS,value);
				break;
				
			case SQL_ATTR_MAX_LENGTH:
				value = maxLength;
				TRACE02(SQL_ATTR_MAX_LENGTH,value);
				break;

			case SQL_ATTR_QUERY_TIMEOUT:
				value = 0;							// driver doesn't timeout
				TRACE02(SQL_ATTR_QUERY_TIMEOUT,value);
				break;

			case SQL_ATTR_ASYNC_ENABLE:
				value = 0;							// driver doesn't do async
				TRACE02(SQL_ATTR_ASYNC_ENABLE,value);
				break;

			case SQL_ATTR_PARAM_BIND_TYPE:
				value = SQL_PARAM_BIND_BY_COLUMN;	// no row binding
				TRACE02(SQL_ATTR_PARAM_BIND_TYPE,value);
				break;

			case SQL_ATTR_RETRIEVE_DATA:
				value = fetchRetData;
				TRACE02(SQL_ATTR_RETRIEVE_DATA,value);
				break;

			case SQL_ATTR_ROW_NUMBER:
				value = (long) rowNumber;
				TRACE02(SQL_ATTR_ROW_NUMBER,value);
				break;

			case SQL_ATTR_ROW_BIND_TYPE:
				value = (long) rowBindType;
				TRACE02(SQL_ATTR_ROW_BIND_TYPE,value);
				break;

			case SQL_ATTR_ROW_STATUS_PTR:
				value = (long) rowStatusPtr;
				TRACE02(SQL_ATTR_ROW_STATUS_PTR,value);
				break;

		    case SQL_ATTR_USE_BOOKMARKS:
				value = useBookmarks;
				TRACE02(SQL_ATTR_USE_BOOKMARKS,value);
		        break;

		    case SQL_ATTR_CURSOR_SENSITIVITY:
				value = cursorSensitivity;
				TRACE02(SQL_ATTR_CURSOR_SENSITIVITY,value);
		        break;

			case SQL_ATTR_ENABLE_AUTO_IPD:
				value = enableAutoIPD;
				TRACE02(SQL_ATTR_ENABLE_AUTO_IPD,value);
		        break;

			case SQL_ATTR_PARAMSET_SIZE:		// 22
				value = (long)paramsetSize;
				TRACE02(SQL_ATTR_PARAMSET_SIZE,value);
				break;

			case SQL_ATTR_FETCH_BOOKMARK_PTR:		//	16
				value = (long)fetchBookmarkPtr;
				TRACE02(SQL_ATTR_FETCH_BOOKMARK_PTR,value);
		        break;

			case SQL_ATTR_NOSCAN:					// 2
				value = noscanSQL;
				TRACE02(SQL_ATTR_NOSCAN,value);
		        break;

			/***
			case SQL_ATTR_ENABLE_AUTO_IPD			15
			case SQL_ATTR_FETCH_BOOKMARK_PTR			16
			case SQL_ATTR_KEYSET_SIZE				SQL_KEYSET_SIZE
			case SQL_ATTR_PARAM_BIND_OFFSET_PTR		17
			case SQL_ATTR_PARAM_OPERATION_PTR		19
			case SQL_ATTR_PARAM_STATUS_PTR			20
			case	SQL_ATTR_PARAMS_PROCESSED_PTR		21
			case SQL_ATTR_RETRIEVE_DATA				SQL_RETRIEVE_DATA
			case SQL_ATTR_ROW_BIND_OFFSET_PTR		23
			case SQL_ATTR_ROW_OPERATION_PTR			24
			case SQL_ATTR_ROW_STATUS_PTR				25
			caseSQL_ATTR_ROWS_FETCHED_PTR			26
			case SQL_ATTR_SIMULATE_CURSOR			SQL_SIMULATE_CURSOR
			***/
			default:
				return sqlReturn (SQL_ERROR, "HYC00", "Optional feature not implemented");
			}

		if (string)
			return returnStringInfo (ptr, bufferLength, lengthPtr, string);

		if (ptr)
			*(long*) ptr = value;

		if (lengthPtr)
			*lengthPtr = sizeof (long);
		}
	catch (SQLException& exception)
		{
		postError ("HY000", exception);
		return SQL_ERROR;
		}

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlGetCursorName(SQLCHAR *name, int bufferLength, SQLSMALLINT *nameLength)
{
	clearErrors();
	try
		{
		returnStringInfo (name, bufferLength, nameLength, cursorName);
		}
	catch (SQLException& exception)
		{
		postError ("HY000", exception);
		return SQL_ERROR;
		}
	return sqlSuccess();
}

bool OdbcStatement::isStoredProcedureEscape(const char *sqlString)
{
	const char *p = sqlString;
	char token [128];
	getToken (&p, token);

	// If we begin with "execute" assume a stored procedure

	if (strcasecmp (token, "execute") == 0)
		return true;

	// if we're not an escape at all, bail out

	if (token [0] != '{')
		return false;

	getToken (&p, token);

	if (token [0] == '?')
		{
		if (*getToken (&p, token) != '=')
			return false;
		getToken (&p, token);
		}

	return strcasecmp (token, "call") == 0;
}

char* OdbcStatement::getToken(const char **ptr, char *token)
{
	const char *p = *ptr;
	SKIP_WHITE (p);
	char *q = token;

	if (*p)
		{
		char c = charTable [*p];
		*q++ = *p++;
		if (c & IDENT)
			while (charTable [*p] & IDENT)
				*q++ = *p++;
		}

	*q = 0;
	*ptr = p;

	return token;
}

RETCODE OdbcStatement::executeStatement()
{
	StatementMetaData *metaData = statement->getStatementMetaDataIPD();
	int nInputParam = metaData->getColumnCount();
	if( nInputParam )
	{
		if(parameterNeedData == 0)
			parameterNeedData = 1;

		for (int n = parameterNeedData; n <= nInputParam; ++n)
		{
			DescRecord *record = applicationParamDescriptor->getDescRecord (n);

			if ( record->parameterType != SQL_PARAM_OUTPUT )
			{
				if ( record->data_at_exec )
				{
					parameterNeedData = n;
					
					if ( record->startedTransfer )
					{
						record->startedTransfer = false;
						statement->endBlobDataTransfer();
						continue;
					}

					record->isBlobOrArray = metaData->isBlobOrArray(parameterNeedData);

					if ( record->isBlobOrArray )
					{
						switch (record->conciseType)
						{
						case SQL_C_CHAR:
						case SQL_C_BINARY:
							record->startedTransfer = true;
							*record->indicatorPtr = 0;
							statement->beginBlobDataTransfer( parameterNeedData );
						}
					}

					return SQL_NEED_DATA;
				}
				else if( record->dataPtr || 
						 (record->indicatorPtr && *record->indicatorPtr == SQL_NULL_DATA) )
					setParameter (record, n);
			}
		}
	}
	if (callableStatement)
		for (int n = 1; n <= implementationParamDescriptor->headCount; ++n)
		{
			DescRecord *record = implementationParamDescriptor->getDescRecord (n);
			if (record->parameterType != SQL_PARAM_INPUT)
				callableStatement->registerOutParameter (n, record->conciseType);
		}

	statement->execute();

	if (callableStatement)
		for (int n = 1; n <= implementationParamDescriptor->headCount; ++n)
		{
			DescRecord *record = implementationParamDescriptor->getDescRecord (n);
			if (record->parameterType != SQL_PARAM_INPUT)
				setValue (applicationParamDescriptor->getDescRecord (n), n );
		}

	getResultSet();

	if ( isStaticCursor() )
	{
		resultSet->readStaticCursor(); 
		setCursorRowCount(resultSet->getCountRowsStaticCursor());
	}

	return SQL_SUCCESS;
}

Binding* OdbcStatement::allocBindings(int count, int oldCount, Binding *oldBindings)
{
	Binding *bindings = new Binding [count];
	memset (bindings, 0, sizeof (Binding) * count);

	if (oldCount)
		{
		memcpy (bindings, oldBindings, sizeof (Binding) * oldCount);
		delete [] oldBindings;
		}

	return bindings;
}

RETCODE OdbcStatement::sqlGetTypeInfo(int dataType)
{
	clearErrors();
	releaseStatement();

	try
		{
		DatabaseMetaData *metaData = connection->getMetaData();
		setResultSet (metaData->getTypeInfo (dataType));
		}
	catch (SQLException& exception)
		{
		postError ("HY000", exception);
		return SQL_ERROR;
		}

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlParamData(SQLPOINTER *ptr)
{	
	RETCODE retcode = sqlSuccess();

	clearErrors();

	if (parameterNeedData == 0)
		return sqlReturn (SQL_ERROR, "HY010", "Function sequence error :: OdbcStatement::sqlParamData");

    if (parameterNeedData-1 > implementationParamDescriptor->headCount)
		return sqlReturn (SQL_ERROR, "HY000", "General error :: OdbcStatement::sqlParamData");

	DescRecord *binding = applicationParamDescriptor->getDescRecord ( parameterNeedData );

//	Bound Address + Binding Offset + ((Row Number – 1) x Element Size)
//	*ptr = binding->pointer + bindOffsetPtr + ((1 – 1) * rowBindType);
	*(unsigned long*)ptr = (unsigned long)binding->dataPtr + (unsigned long)bindOffsetPtr; // for single row

	if( binding->indicatorPtr && binding->data_at_exec )
	{
		if (*binding->indicatorPtr && binding->startedTransfer)
		{
			; // continue into executeStatement();
		}
		else
		{
			StatementMetaData *metaData = statement->getStatementMetaDataIPD();

			if( metaData )
			{
				binding->isBlobOrArray = metaData->isBlobOrArray(parameterNeedData);

				if ( binding->isBlobOrArray )
				{
					switch (binding->conciseType)
					{
					case SQL_C_CHAR:
					case SQL_C_BINARY:
						binding->startedTransfer = true;
						*binding->indicatorPtr = 0;
						statement->beginBlobDataTransfer( parameterNeedData );
					}
				}
			}

			return SQL_NEED_DATA;
		}
	}
	
	try
	{
		retcode = executeStatement();
	}
	catch (SQLException& exception)
	{
		postError ("HY000", exception);
		retcode = SQL_ERROR;
	}

	if (retcode && retcode != SQL_SUCCESS_WITH_INFO)
		return retcode;

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlPutData (SQLPOINTER value, SQLINTEGER valueSize)
{
	bool endPutData = false;

	if (parameterNeedData == 0)
		return sqlReturn (SQL_ERROR, "HY010", "Function sequence error :: OdbcStatement::sqlPutData");

    if (parameterNeedData > implementationParamDescriptor->headCount)
		return sqlReturn (SQL_ERROR, "HY000", "General error :: OdbcStatement::sqlPutData");

	DescRecord *binding = applicationParamDescriptor->getDescRecord (parameterNeedData);

    if (!binding)
		return sqlReturn (SQL_ERROR, "HY000", "General error :: OdbcStatement::sqlPutData");

	if (valueSize == SQL_NULL_DATA)
	{
		if (binding->startedTransfer)
		{
			binding->startedTransfer = false;
			switch (binding->conciseType)
			{
			case SQL_C_CHAR:
			case SQL_C_BINARY:
				statement->endBlobDataTransfer();
				break;
			}
		}
		statement->setNull(parameterNeedData, 0);
		*binding->indicatorPtr = SQL_NULL_DATA;
		endPutData = true;
	}
	else if ( binding->isBlobOrArray )
	{
		switch (binding->conciseType)
		{
		case SQL_C_CHAR:
			if(valueSize == SQL_NTS)
				valueSize = strlen( (char*)value );

		case SQL_C_BINARY:
			if( valueSize )
			{
				*binding->indicatorPtr += valueSize;
				statement->putBlobSegmentData (valueSize, value);
			}
			else
				endPutData = true;
			break;
		}
	}
	else
	{
		switch (binding->conciseType)
		{
		case SQL_C_CHAR:
			if(valueSize == SQL_NTS)
				valueSize = strlen( (char*)value );
		default:
			binding->dataPtr = value;
			*binding->indicatorPtr = valueSize;
			setParameter (binding, parameterNeedData );
			endPutData = true;
			break;
		}
	}

	if ( endPutData )
		++parameterNeedData;

	return sqlSuccess();
}

/*
RETCODE OdbcStatement::sqlParamData(SQLPOINTER *ptr)
{	
	RETCODE retcode = sqlSuccess();

	clearErrors();

	if (parameterNeedData == 0)
		return sqlReturn (SQL_ERROR, "HY010", "Function sequence error :: OdbcStatement::sqlParamData");

    if (parameterNeedData > numberParameters)
		return sqlReturn (SQL_ERROR, "HY000", "General error :: OdbcStatement::sqlParamData");

	int n = parameterNeedData;

	Binding *binding = parameters + n;

//	Bound Address + Binding Offset + ((Row Number – 1) x Element Size)
//	*ptr = binding->pointer + bindOffsetPtr + ((1 – 1) * rowBindType);
	*(unsigned long*)ptr = (unsigned long)binding->pointerOrg + (unsigned long)bindOffsetPtr; // for single row
 
	if( binding->indicatorPointer && !binding->startedTransfer && *binding->indicatorPointer < SQL_LEN_DATA_AT_EXEC_OFFSET ) 
	{
		switch (binding->cType)
		{
			case SQL_C_CHAR:
			{
				if ( !binding->startedTransfer )
				{
					binding->startedTransfer = true;
					*binding->indicatorPointer = 0;
					statement->beginClobDataTransfer(n+1);
				}

				return SQL_NEED_DATA;
			}
			break;

			case SQL_C_BINARY:
			{
				if ( !binding->startedTransfer )
				{
					binding->startedTransfer = true;
					*binding->indicatorPointer = 0;
					statement->beginBlobDataTransfer(n+1);
				}

				return SQL_NEED_DATA;
			}
			break;				
		}
	}
	else
	{			
		if (binding->startedTransfer)
		{
			binding->startedTransfer = false;
			if(binding->data_at_exec)
				switch (binding->cType)
				{
				case SQL_C_CHAR:
					statement->endClobDataTransfer();
					break;
						
				case SQL_C_BINARY:
					statement->endBlobDataTransfer();
					break;
				}
		}
	}
	
	try
	{
		retcode = executeStatement();
	}
	catch (SQLException& exception)
	{
		postError ("HY000", exception);
		retcode = SQL_ERROR;
	}

	if (retcode && retcode != SQL_SUCCESS_WITH_INFO)
		return retcode;

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlPutData (SQLPOINTER value, SQLINTEGER valueSize)
{
	if (parameterNeedData == 0)
		return sqlReturn (SQL_ERROR, "HY010", "Function sequence error :: OdbcStatement::sqlPutData");

    if (parameterNeedData > numberParameters)
		return sqlReturn (SQL_ERROR, "HY000", "General error :: OdbcStatement::sqlPutData");

    Binding *binding = parameters + parameterNeedData++;

    if (!binding)
		return sqlReturn (SQL_ERROR, "HY000", "General error :: OdbcStatement::sqlPutData");

	if (valueSize == SQL_NULL_DATA)
	{
		*binding->indicatorPointer = SQL_NULL_DATA;
		return SQL_SUCCESS;
	}

	switch (binding->cType)
	{
		case SQL_C_CHAR:
		{
			if(valueSize == SQL_NTS)
				valueSize = strlen( (char*)value );

			if( binding->data_at_exec )
			{
				binding->pointer = value;
				*binding->indicatorPointer = valueSize;
				setParameter (binding, parameterNeedData );
			}
			else
			{
				*binding->indicatorPointer += valueSize;
				statement->putClobSegmentData (valueSize, value);
			}
		}
		break;

		case SQL_C_BINARY:
		{			
			*binding->indicatorPointer += valueSize;

			statement->putBlobSegmentData (valueSize, value);
		}
		break;
	}

	return sqlSuccess();
}
*/

RETCODE OdbcStatement::sqlSetStmtAttr(int attribute, SQLPOINTER ptr, int length)
{
	clearErrors();

	try
		{
		switch (attribute)
			{
			case SQL_QUERY_TIMEOUT:				// 0
				TRACE(SQL_QUERY_TIMEOUT);
				break;

			case SQL_ATTR_RETRIEVE_DATA:
				fetchRetData = (int) ptr;
				TRACE02(SQL_ATTR_RETRIEVE_DATA,(int) ptr);
				break;

			case SQL_ATTR_PARAM_BIND_TYPE:		// 18
				paramBindType = (int) ptr;
				applicationParamDescriptor->headBindType = (SQLINTEGER) ptr;
				TRACE02(SQL_ATTR_PARAM_BIND_TYPE,(int) ptr);
				break;

			case SQL_ATTR_PARAM_BIND_OFFSET_PTR:// 17
				paramBindOffset = ptr;
				applicationParamDescriptor->headBindOffsetPtr = (SQLINTEGER*)ptr;
				TRACE02(SQL_ATTR_PARAM_BIND_OFFSET_PTR,(int) ptr);
				break;

			case SQL_ATTR_PARAMS_PROCESSED_PTR:	// 21
				paramsProcessedPtr = ptr;
				implementationParamDescriptor->headRowsProcessedPtr = (SQLUINTEGER*) ptr;
				TRACE02(SQL_ATTR_PARAMS_PROCESSED_PTR,(int) ptr);
				break;

			case SQL_ATTR_PARAMSET_SIZE:		// 22
				paramsetSize = (int) ptr;
				applicationParamDescriptor->headArraySize = (SQLUINTEGER)ptr;
				TRACE02(SQL_ATTR_PARAMSET_SIZE,(int) ptr);
				break;

			case SQL_ATTR_ROW_BIND_TYPE:		// SQL_BIND_TYPE 5
				rowBindType = (int) ptr;
				applicationRowDescriptor->headBindType = (SQLINTEGER)ptr;
				TRACE02(SQL_ATTR_ROW_BIND_TYPE,(int) ptr);
				break;

			case SQL_ATTR_ROW_ARRAY_SIZE:		// 27
				applicationRowDescriptor->headArraySize = (int) ptr;
				TRACE02(SQL_ATTR_ROW_ARRAY_SIZE,(int) ptr);
				break;
		    case SQL_ROWSET_SIZE:                // 9
				rowArraySize = (int) ptr;
				TRACE02(SQL_ROWSET_SIZE,(int) ptr);
				break;
					
			case SQL_ATTR_ROWS_FETCHED_PTR:		// 26
				implementationRowDescriptor->headRowsProcessedPtr = (SQLUINTEGER*) ptr;
				TRACE02(SQL_ATTR_ROWS_FETCHED_PTR,(int) ptr);
				break;

			case SQL_ATTR_ROW_BIND_OFFSET_PTR:	// 23
				bindOffsetPtr = (SQLINTEGER*) ptr;
				applicationRowDescriptor->headBindOffsetPtr = (SQLINTEGER*)ptr;
				TRACE02(SQL_ATTR_ROW_BIND_OFFSET_PTR,(int) ptr);
				break;

			case SQL_ATTR_ROW_STATUS_PTR:		// 25
				rowStatusPtr = (SQLUSMALLINT*) ptr;
				implementationRowDescriptor->headArrayStatusPtr = (SQLUSMALLINT*)ptr;
				TRACE02(SQL_ATTR_ROW_STATUS_PTR,(int) ptr);
				break;
 
 			case SQL_ATTR_CONCURRENCY:			// SQL_CONCURRENCY	7
				currency = (int) ptr;

				if(currency == SQL_CONCUR_READ_ONLY)
					cursorSensitivity = SQL_INSENSITIVE;
				else
					cursorSensitivity = SQL_UNSPECIFIED;

				TRACE02(SQL_ATTR_CONCURRENCY,(int) ptr);
				break;

			case SQL_ATTR_CURSOR_TYPE:			// SQL_CURSOR_TYPE 6
				cursorType = (int) ptr;
				if ( cursorType == SQL_CURSOR_DYNAMIC )
				{
					cursorScrollable = SQL_SCROLLABLE;
					if(currency != SQL_CONCUR_READ_ONLY)
						cursorSensitivity = SQL_SENSITIVE;
				}
				else if ( cursorType == SQL_CURSOR_FORWARD_ONLY )
				{
					cursorScrollable = SQL_NONSCROLLABLE;
				}
				else if ( cursorType == SQL_CURSOR_KEYSET_DRIVEN )
				{
					cursorScrollable = SQL_SCROLLABLE;
					if(currency != SQL_CONCUR_READ_ONLY)
						cursorSensitivity = SQL_UNSPECIFIED;
				}
				else if ( cursorType == SQL_CURSOR_STATIC )
				{
					cursorScrollable = SQL_SCROLLABLE;
					if(currency != SQL_CONCUR_READ_ONLY)
						cursorSensitivity = SQL_UNSPECIFIED;
					else
						cursorSensitivity = SQL_INSENSITIVE;
				}
				TRACE02(SQL_ATTR_CURSOR_TYPE,(int) ptr);
				break;

			case SQL_ATTR_CURSOR_SCROLLABLE:
				cursorScrollable = SQL_SCROLLABLE;

				if( !cursorScrollable )
					cursorType = SQL_CURSOR_FORWARD_ONLY;
				else
					cursorType = SQL_CURSOR_STATIC;

				TRACE02(SQL_ATTR_CURSOR_SCROLLABLE,(int) ptr);
				break;

			case SQL_ATTR_ASYNC_ENABLE:			// 4
				asyncEnable = (int) ptr == SQL_ASYNC_ENABLE_ON;
				TRACE02(SQL_ATTR_ASYNC_ENABLE,(int) ptr);
				break;

			case SQL_ATTR_MAX_ROWS:					// SQL_MAX_ROWS 1
				maxRows = (int) ptr;
				TRACE02(SQL_ATTR_MAX_ROWS,(int) ptr);
				break;
			
			case SQL_ATTR_MAX_LENGTH:
				if ( length == SQL_IS_POINTER )
					maxLength = *(int*) ptr;
				else
					maxLength = (int) ptr;
				TRACE02(SQL_ATTR_MAX_LENGTH, maxLength);
				break;

		    case SQL_ATTR_USE_BOOKMARKS:        //    SQL_USE_BOOKMARKS 12
				applicationRowDescriptor->allocBookmarkField();
				useBookmarks = (SQLINTEGER)ptr;
				TRACE02(SQL_ATTR_USE_BOOKMARKS,(int) ptr);
				break;

			case SQL_ATTR_CURSOR_SENSITIVITY:    // (-2)
				cursorSensitivity = (SQLINTEGER)ptr;
				if ( cursorSensitivity == SQL_INSENSITIVE )
				{
					currency = SQL_CONCUR_READ_ONLY;
					cursorType = SQL_CURSOR_STATIC;
				}
				else if ( cursorSensitivity == SQL_SENSITIVE )
				{
					currency = SQL_CONCUR_ROWVER;
					cursorType = SQL_CURSOR_FORWARD_ONLY;
				}
				else // if ( cursorSensitivity == SQL_UNSPECIFIED )
				{
					currency = SQL_CONCUR_READ_ONLY;
					cursorType = SQL_CURSOR_FORWARD_ONLY;
				}

				TRACE02(SQL_ATTR_CURSOR_SENSITIVITY,(int) ptr);
		        break;

			case SQL_ATTR_PARAM_OPERATION_PTR:		// 19
				applicationParamDescriptor->headArrayStatusPtr = (SQLUSMALLINT*)ptr;
				TRACE02(SQL_ATTR_PARAM_OPERATION_PTR,(int) ptr);
		        break;

			case SQL_ATTR_PARAM_STATUS_PTR:			// 20
				implementationParamDescriptor->headArrayStatusPtr = (SQLUSMALLINT*)ptr;
				TRACE02(SQL_ATTR_PARAM_STATUS_PTR,(int) ptr);
		        break;

			case SQL_ATTR_ROW_OPERATION_PTR:		//	24
				applicationRowDescriptor->headArrayStatusPtr = (SQLUSMALLINT*)ptr;
				TRACE02(SQL_ATTR_ROW_OPERATION_PTR,(int) ptr);
				break;

			case SQL_ATTR_ENABLE_AUTO_IPD:
				enableAutoIPD = (int) ptr;
				TRACE02(SQL_ATTR_ENABLE_AUTO_IPD,(int) ptr);
		        break;

			case SQL_ATTR_FETCH_BOOKMARK_PTR:		//	16
				fetchBookmarkPtr = ptr;
				TRACE02(SQL_ATTR_FETCH_BOOKMARK_PTR,(int) ptr);
		        break;

			case SQL_ATTR_NOSCAN:					// 2
				noscanSQL = (int) ptr;
				TRACE02(SQL_ATTR_NOSCAN,(int) ptr);
		        break;

			/***
			case SQL_ATTR_ASYNC_ENABLE				4
			case SQL_ATTR_CONCURRENCY				SQL_CONCURRENCY	7
			case SQL_ATTR_CURSOR_TYPE				SQL_CURSOR_TYPE 6
			case SQL_ATTR_ENABLE_AUTO_IPD			15
			case SQL_ATTR_KEYSET_SIZE				SQL_KEYSET_SIZE 8
			case SQL_ATTR_PARAM_BIND_OFFSET_PTR		17
			case SQL_ATTR_PARAM_BIND_TYPE			18
			case SQL_ATTR_PARAM_OPERATION_PTR		19
			case SQL_ATTR_PARAM_STATUS_PTR			20
			case SQL_ATTR_PARAMS_PROCESSED_PTR		21
			case SQL_ATTR_QUERY_TIMEOUT				SQL_QUERY_TIMEOUT
			case SQL_ATTR_RETRIEVE_DATA				SQL_RETRIEVE_DATA 11
			case SQL_ATTR_ROW_BIND_TYPE				SQL_BIND_TYPE 5
			case SQL_ATTR_ROW_OPERATION_PTR			24
			case SQL_ATTR_ROW_STATUS_PTR				25
			case SQL_ATTR_SIMULATE_CURSOR			SQL_SIMULATE_CURSOR 10
			***/

			default:
				return sqlReturn (SQL_ERROR, "HYC00", "Optional feature not implemented");
			}
		}
	catch (SQLException& exception)
		{
		postError ("HY000", exception);
		return SQL_ERROR;
		}

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlRowCount(SQLINTEGER *rowCount)
{
	clearErrors();

	try
	{
		if (!statement)
			return sqlReturn (SQL_ERROR, "HY010", "Function sequence error");
		if ( isStaticCursor() )
			*rowCount = sqlDiagCursorRowCount;
		else
		{
			if ( enFetch != NoneFetch )
				*rowCount = rowNumber;
			else
				*rowCount = statement->getUpdateCount();
		}
	}
	catch (SQLException& exception)
	{
		postError ("HY000", exception);
		return SQL_ERROR;
	}

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlColAttributes(int column, int descType, SQLPOINTER buffer, int bufferSize, SWORD *length, SDWORD *valuePtr)
{
	clearErrors();
	int value = 0;
	const char *string = NULL;
	int realSqlType;

	try
	{
		StatementMetaData *metaData = resultSet->getMetaData();
		switch (descType)
		{
		case SQL_COLUMN_LABEL:
			string = metaData->getColumnLabel (column);
			break;

		case SQL_DESC_BASE_COLUMN_NAME:
		case SQL_COLUMN_NAME:
			string = metaData->getColumnName (column);
			break;

		case SQL_COLUMN_UNSIGNED:
			value = (metaData->isSigned (column)) ? SQL_FALSE : SQL_TRUE;
			break;

		case SQL_COLUMN_UPDATABLE:
			value = SQL_ATTR_READWRITE_UNKNOWN;
//				value = (metaData->isWritable (column)) ? SQL_ATTR_WRITE : SQL_ATTR_READONLY;
			*length = sizeof(long);
			break;

		case SQL_COLUMN_COUNT:
			value = metaData->getColumnCount();
			break;

		case SQL_COLUMN_TYPE:
			value = metaData->getColumnType (column, realSqlType);
			break;

		case SQL_COLUMN_LENGTH:
			value = metaData->getColumnDisplaySize (column);
			break;

		case SQL_COLUMN_PRECISION:
			value = metaData->getPrecision (column);
			break;

		case SQL_COLUMN_SCALE:
			value = metaData->getScale (column);
			break;

		case SQL_COLUMN_DISPLAY_SIZE:
			value = metaData->getColumnDisplaySize (column);
			break;

		case SQL_COLUMN_NULLABLE:
			value = (metaData->isNullable (column)) ? SQL_NULLABLE : SQL_NO_NULLS ;
			break;

		case SQL_COLUMN_MONEY:
			value = (metaData->isCurrency (column)) ? 1 : 0;
			break;

		case SQL_COLUMN_AUTO_INCREMENT:
			value = (metaData->isAutoIncrement (column)) ? 1 : 0;
			break;

		case SQL_COLUMN_CASE_SENSITIVE:
			value = (metaData->isCaseSensitive (column)) ? SQL_TRUE : SQL_FALSE;
			break;

		case SQL_COLUMN_SEARCHABLE:
			value = (metaData->isSearchable (column)) ? SQL_PRED_SEARCHABLE : SQL_PRED_NONE;
			break;

		case SQL_COLUMN_TYPE_NAME:
            string = metaData->getColumnTypeName (column);
            break;

		case SQL_COLUMN_TABLE_NAME:
			string = metaData->getTableName (column);
			break;

		case SQL_COLUMN_OWNER_NAME:
			string = metaData->getSchemaName (column);
			break;

		case SQL_COLUMN_QUALIFIER_NAME:
			string = metaData->getCatalogName (column);
			break;


		/***
		case SQL_COLUMN_COUNT                0
		case SQL_COLUMN_NAME                 1
		case SQL_COLUMN_TYPE                 2
		case SQL_COLUMN_LENGTH               3
		case SQL_COLUMN_PRECISION            4
		case SQL_COLUMN_SCALE                5
		case SQL_COLUMN_DISPLAY_SIZE         6
		case SQL_COLUMN_NULLABLE             7
		case SQL_COLUMN_UNSIGNED             8
		case SQL_COLUMN_MONEY                9
		case SQL_COLUMN_UPDATABLE            10
		case SQL_COLUMN_AUTO_INCREMENT       11
		case SQL_COLUMN_CASE_SENSITIVE       12
		case SQL_COLUMN_SEARCHABLE           13
		case SQL_COLUMN_TYPE_NAME            14
		case SQL_COLUMN_TABLE_NAME           15
		case SQL_COLUMN_OWNER_NAME           16
		case SQL_COLUMN_QUALIFIER_NAME       17
		case SQL_COLUMN_LABEL                18
		case SQL_COLATT_OPT_MAX              SQL_COLUMN_LABEL
		***/
		default:
			{
			JString msg;
			msg.Format ("Descriptor type (%d) out of range", descType);
			return sqlReturn (SQL_ERROR, "S1091", (const char*) msg);
			//return sqlReturn (SQL_ERROR, "S1091", "Descriptor type out of range");
			}
		}
	}
	catch (SQLException& exception)
	{
		postError ("HY000", exception);
		return SQL_ERROR;
	}

	if (string)
		setString (string, (SQLCHAR*) buffer, bufferSize, length);
	else if (valuePtr)
		*valuePtr = value;

	return sqlSuccess();
}

RETCODE OdbcStatement::returnData()
{
	int columnNumber;
	++rowNumber;

	try
	{
		int nCount = applicationRowDescriptor->headCount;
		for (columnNumber = 1; columnNumber <= nCount; ++columnNumber)
			setValue (applicationRowDescriptor->getDescRecord (columnNumber), columnNumber);

		if (implementationRowDescriptor->headRowsProcessedPtr)
			*(SQLUINTEGER*)implementationRowDescriptor->headRowsProcessedPtr = 1;
		}
	catch (SQLException& exception)
		{
		OdbcError *error = postError ("HY000", exception);
		error->setRowNumber (rowNumber);
		error->setColumnNumber (columnNumber, rowNumber);
		return SQL_ERROR;
		}
	return sqlSuccess();
}

/*
RETCODE OdbcStatement::returnData()
{
	int columnNumber;
	++rowNumber;

	try
		{
		if (bindings && *bindings)
//			for (columnNumber = 1; columnNumber <= numberColumns; ++columnNumber)
			for (columnNumber = 1; columnNumber < numberBindings; ++columnNumber)
				{
				Binding *binding = *bindings + columnNumber;
				if (binding->pointer && binding->type != SQL_PARAM_INPUT)
					setValue (binding, columnNumber);
				}

		if (implementationRowDescriptor->headRowsProcessedPtr)
			*(SQLINTEGER*)implementationRowDescriptor->headRowsProcessedPtr = 1;
		}
	catch (SQLException& exception)
		{
		OdbcError *error = postError ("HY000", exception);
		error->setRowNumber (rowNumber);
		error->setColumnNumber (columnNumber, rowNumber);
		return SQL_ERROR;
		}
	return sqlSuccess();
}
*/
RETCODE OdbcStatement::sqlColAttribute(int column, int fieldId, SQLPOINTER attributePtr, int bufferLength, SQLSMALLINT *strLengthPtr, SQLPOINTER numericAttributePtr)
{
	clearErrors();
	int value;
	const char *string = NULL;
	int realSqlType;

	try
	{
		StatementMetaData *metaData = resultSet->getMetaData();
		switch (fieldId)
		{
		case SQL_DESC_LABEL:
			string = metaData->getColumnLabel (column);
			break;

		case SQL_DESC_BASE_COLUMN_NAME:
		case SQL_DESC_NAME:
			string = metaData->getColumnName (column);
			break;
		case SQL_DESC_UNNAMED:
			value = (metaData->getColumnName (column)) ? SQL_NAMED : SQL_UNNAMED;
			break;

		case SQL_DESC_UNSIGNED:
			value = (metaData->isSigned (column)) ? SQL_FALSE : SQL_TRUE;
			break;

		case SQL_DESC_UPDATABLE:
			value = (metaData->isWritable (column)) ? SQL_ATTR_WRITE : SQL_ATTR_READONLY;
			break;

		case SQL_DESC_COUNT:
			value = metaData->getColumnCount();
			break;

		case SQL_DESC_TYPE:
		case SQL_DESC_CONCISE_TYPE:
			value = metaData->getColumnType (column, realSqlType);
			break;

		case SQL_DESC_LENGTH:
			value = metaData->getColumnDisplaySize (column);
			break;

		case SQL_DESC_PRECISION:
			value = metaData->getPrecision (column);
			break;

		case SQL_DESC_SCALE:
			value = metaData->getScale (column);
			break;

		case SQL_DESC_DISPLAY_SIZE:
			value = metaData->getColumnDisplaySize (column);
			break;

		case SQL_DESC_NULLABLE:
			value = (metaData->isNullable (column)) ? SQL_NULLABLE : SQL_NO_NULLS;
			break;

		/***
		case SQL_DESC_MONEY:
			value = (metaData->isCurrency (column)) ? 1 : 0;
			break;

		***/

		case SQL_DESC_AUTO_UNIQUE_VALUE: // REVISAR
		    value = (metaData->isAutoIncrement (column)) ? 1 : 0;
		    break;

		case SQL_DESC_CASE_SENSITIVE:
			value = (metaData->isCaseSensitive (column)) ? SQL_TRUE : SQL_FALSE;
			break;

		case SQL_DESC_SEARCHABLE:
			value = (metaData->isSearchable (column)) ? SQL_PRED_SEARCHABLE : SQL_PRED_NONE;
			break;

		//case SQL_DESC_TYPE_NAME:
		//	value = metaData->getColumnType (column, realSqlType);
		//	break;

		case SQL_DESC_TYPE_NAME:
			string = metaData->getColumnTypeName (column);               
			break; 

		case SQL_DESC_BASE_TABLE_NAME:
		case SQL_DESC_TABLE_NAME:
			string = metaData->getTableName (column);
			break;



	/***
		case SQL_DESC_OWNER_NAME:
			string = metaData->getSchemaName (column);
			break;

		case SQL_DESC_QUALIFIER_NAME:
			string = metaData->getCatalogName (column);
			break;

		case SQL_DESC_COUNT                0
		case SQL_DESC_NAME                 1
		case SQL_DESC_TYPE                 2
		case SQL_DESC_CONCISE_TYPE         2
		case SQL_DESC_LENGTH               3
		case SQL_DESC_PRECISION            4
		case SQL_DESC_SCALE                5
		case SQL_DESC_DISPLAY_SIZE         6
		case SQL_DESC_NULLABLE             7
		case SQL_DESC_UNSIGNED             8
		case SQL_DESC_MONEY                9
		case SQL_DESC_UPDATABLE            10
		case SQL_DESC_AUTO_INCREMENT       11
		case SQL_DESC_CASE_SENSITIVE       12
		case SQL_DESC_SEARCHABLE           13
		case SQL_DESC_TYPE_NAME            14
		case SQL_DESC_TABLE_NAME           15
		case SQL_DESC_OWNER_NAME           16
		case SQL_DESC_QUALIFIER_NAME       17
		case SQL_DESC_LABEL                18
		case SQL_COLATT_OPT_MAX              SQL_DESC_LABEL
		***/
		default:
			{
			JString msg;
			msg.Format ("field id (%d) out of range", fieldId);
			return sqlReturn (SQL_ERROR, "HY091", (const char*) msg);
			}
		}
	}
	catch (SQLException& exception)
	{
		postError ("HY000", exception);
		return SQL_ERROR;
	}

	if (string)
		setString (string, (SQLCHAR*) attributePtr, bufferLength, strLengthPtr);
	else if (numericAttributePtr)
		*(SQLINTEGER*) numericAttributePtr = value;

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlMoreResults()
{
	clearErrors();

	try
		{
		if (!statement || !statement->getMoreResults())
			return SQL_NO_DATA;
		}
	catch (SQLException& exception)
		{
		postError ("HY000", exception);
		return SQL_ERROR;
		}

	return sqlSuccess();
}


RETCODE OdbcStatement::sqlSpecialColumns(unsigned short rowId, SQLCHAR * catalog, int catLength, SQLCHAR * schema, int schemaLength, SQLCHAR * table, int tableLength, unsigned short scope, unsigned short nullable)
{
	clearErrors();
	releaseStatement();
	char temp [1024], *p = temp;

	const char *cat = getString (&p, catalog, catLength, NULL);
	const char *scheme = getString (&p, schema, schemaLength, NULL);
	const char *tbl = getString (&p, table, tableLength, NULL);

	try
	{
		DatabaseMetaData *metaData = connection->getMetaData();
		setResultSet (metaData->specialColumns (cat, scheme, tbl, scope, nullable));
		if ( rowId == SQL_ROWVER )
			eof = true;
	}
	catch (SQLException& exception)
	{
		postError ("HY000", exception);
		return SQL_ERROR;
	}

	return sqlSuccess();
}

