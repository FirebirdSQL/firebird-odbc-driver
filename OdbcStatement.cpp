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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "IscDbc/Connection.h"
#include "IscDbc/SQLException.h"
#include "OdbcEnv.h"
#include "OdbcConnection.h"
#include "OdbcStatement.h"
#include "OdbcError.h"
#include "DescRecord.h"

#ifdef DEBUG                               
#define TRACE(msg)		OutputDebugString(#msg"\n");
#define TRACE02(msg,val)  TraceOutput(#msg,val)
#else
#define TRACE(msg)		
#define TRACE02(msg,val)
#endif

namespace OdbcJdbcLibrary {

using namespace IscDbcLibrary;

void TraceOutput(char * msg,long val)
{
	char buf[80];
	sprintf(buf,"\t%s = %d : %x\n",msg,val,val);
	OutputDebugString(buf);
}

#ifdef __GNUWIN32__
extern double listScale[]; // from OdbcConvert.cpp
#else
extern unsigned __int64 listScale[]; // from OdbcConvert.cpp
#endif

//	Bound Address + Binding Offset + ((Row Number – 1) x Element Size)
//	*ptr = binding->pointer + bindOffsetPtr + ((1 – 1) * rowBindType); // <-- for single row
#define GETBOUNDADDRESS(binding)	(unsigned long)binding->dataPtr + (unsigned long)bindOffsetPtr; 

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

OdbcStatement::OdbcStatement(OdbcConnection *connect, int statementNumber)
{
	connection = connect;
	resultSet = NULL;
	statement = connection->connection->createInternalStatement();
	execute = &OdbcStatement::executeStatement;
	fetchNext = &ResultSet::next;
	schemaFetchData = true;
	metaData = NULL;
	cancel = false;
	countFetched = 0l;
	enFetch = NoneFetch;
    parameterNeedData = 0;	
	maxRows = 0;
	maxLength = 0;
	applicationRowDescriptor = connection->allocDescriptor (odtApplicationRow);
	saveApplicationRowDescriptor = applicationRowDescriptor;
	applicationParamDescriptor = connection->allocDescriptor (odtApplicationParameter);
	saveApplicationParamDescriptor = applicationParamDescriptor;
	implementationRowDescriptor = connection->allocDescriptor (odtImplementationRow);
	implementationParamDescriptor = connection->allocDescriptor (odtImplementationParameter);
	implementationGetDataDescriptor = NULL;
	fetchRetData = SQL_RD_ON;
	rowBindType = SQL_BIND_BY_COLUMN;
	paramBindType = SQL_BIND_BY_COLUMN;
	sqldataOutOffsetPtr = NULL;
	bindOffsetPtr = NULL;
	paramsetSize = 0;
	numberColumns = 0;
	registrationOutParameter = false;
	isRegistrationOutParameter = false;
	isResultSetFromSystemCatalog = false;
	isFetchStaticCursor = false;
	paramsProcessedPtr = NULL;
	currency = SQL_CONCUR_READ_ONLY;
	cursorType = SQL_CURSOR_FORWARD_ONLY;
	cursorName.Format ("cursor%d", statementNumber);
	setPreCursorName = false;
	cursorScrollable = SQL_NONSCROLLABLE;
	asyncEnable = false;
    rowArraySize  = applicationRowDescriptor->headArraySize;
	enableAutoIPD = SQL_TRUE;
	useBookmarks = SQL_UB_OFF;
	cursorSensitivity = SQL_INSENSITIVE;
	fetchBookmarkPtr = NULL;
	noscanSQL = SQL_NOSCAN_OFF;

	listBindIn = new ListBindColumn;
	convert = new OdbcConvert(this);
	listBindOut = new ListBindColumn;
	listBindGetData = NULL;
}

OdbcStatement::~OdbcStatement()
{
	releaseBindings();
	releaseParameters();
	releaseStatement();
	statement->release();
	delete applicationRowDescriptor;
	delete applicationParamDescriptor;
	delete implementationRowDescriptor;
	delete implementationParamDescriptor;
	delete implementationGetDataDescriptor;
	delete convert;
	delete listBindIn;
	delete listBindOut;
	delete listBindGetData;
	connection->statementDeleted (this);
}

OdbcObjectType OdbcStatement::getType()
{
	return odbcTypeStatement;
}

inline StatementMetaData* OdbcStatement::getStatementMetaDataIRD()
{ 
	return resultSet ? resultSet->getMetaData() : statement->getStatementMetaDataIRD(); 
}

inline void OdbcStatement::clearErrors()
{
	if ( infoPosted )
		OdbcObject::clearErrors();
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

RETCODE OdbcStatement::sqlPrepare(SQLCHAR * sql, int sqlLength)
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
		implementationParamDescriptor->releasePrepared();

		statement->prepareStatement (string);
	
		if ( statement->isActiveProcedure() )
			execute = &OdbcStatement::executeProcedure;
		else
			execute = &OdbcStatement::executeStatement;

		listBindIn->removeAll();
		listBindOut->removeAll();
		implementationRowDescriptor->setDefaultImplDesc (statement->getStatementMetaDataIRD());
		implementationParamDescriptor->setDefaultImplDesc (statement->getStatementMetaDataIRD(), statement->getStatementMetaDataIPD());
		applicationRowDescriptor->clearPrepared();
		rebindColumn();
		numberColumns = statement->getStatementMetaDataIRD()->getColumnCount();
		implementationParamDescriptor->updateDefinedIn();
		applicationParamDescriptor->clearPrepared();
		
		if ( enableAutoIPD == SQL_TRUE )
			rebindParam();

		if ( setPreCursorName )
		{
			statement->setCursorName (cursorName);
			setPreCursorName = false;
		}
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
	eof = false;
	cancel = false;
	numberColumns = 0;
	releaseResultSet();
	statement->drop();
}

void OdbcStatement::releaseResultSet()
{
	if (resultSet)
	{
		resultSet->release();
		resultSet = NULL;
		metaData  = NULL;
		sqldataOutOffsetPtr = NULL;
		implementationRowDescriptor->clearDefined();
		implementationParamDescriptor->clearDefined();
	}
	
	countFetched = 0;
	isResultSetFromSystemCatalog = false;

	if ( implementationGetDataDescriptor )
	{
		delete implementationGetDataDescriptor;
		implementationGetDataDescriptor = NULL;
		delete listBindGetData;
		listBindGetData = NULL;
	}
}

void OdbcStatement::setResultSet(ResultSet * results, bool fromSystemCatalog)
{
	execute = &OdbcStatement::executeStatement;
	fetchNext = &ResultSet::next;
	resultSet = results;
	metaData = resultSet->getMetaData();
	sqldataOutOffsetPtr = resultSet->getSqlDataOffsetPtr();

	if ( !statement->isActive() )
	{
		listBindOut->removeAll();
		implementationRowDescriptor->setDefaultImplDesc (metaData);
		applicationRowDescriptor->clearPrepared();
		rebindColumn();
	}
	else
		implementationRowDescriptor->updateDefinedOut();

	convert->setBindOffsetPtrFrom(sqldataOutOffsetPtr, NULL);
	numberColumns = resultSet->getColumnCount();
	enFetch = NoneFetch;
	eof = false;
	cancel = false;
	countFetched = 0;
	rowNumber = 0;
	indicatorRowNumber = 0;

	if ( fromSystemCatalog )
	{
		isResultSetFromSystemCatalog = true;
		setCursorRowCount(resultSet->getCountRowsStaticCursor());
	}
}

void OdbcStatement::rebindColumn()
{
	if ( !implementationRowDescriptor->headCount )
		return;

	int nCount = implementationRowDescriptor->headCount;
	int nCountApp = applicationRowDescriptor->headCount;
	DescRecord * record = applicationRowDescriptor->getDescRecord (0);

	if ( !record->isPrepared && record->isDefined )
	{	// set column 0
		DescRecord *imprec = implementationRowDescriptor->getDescRecord (0);
		imprec->dataPtr = &rowNumber;
		imprec->indicatorPtr = &indicatorRowNumber;
		record->octetLengthPtr = &indicatorRowNumber;
		record->initZeroColumn();
	}

	for (int column = 1, columnApp = 1; column <= nCount && columnApp <= nCountApp; ++column,  ++columnApp)
	{
		record = applicationRowDescriptor->getDescRecord ( columnApp );

		if ( !record->isPrepared && record->isDefined )
		{
			SQLINTEGER bufferLength = record->length;
			bindOutputColumn ( columnApp, record);
			record->length = bufferLength;
		}
	}
}

void OdbcStatement::addBindColumn(int column, DescRecord * recordFrom, DescRecord * recordTo)
{
	CBindColumn bindCol(column, recordFrom, recordTo);

	int j = listBindOut->SearchAndInsert( &bindCol );
	if( j < 0 )
		(*listBindOut)[-j-1] = bindCol;
	else
		(*listBindOut)[j] = bindCol;
}

void OdbcStatement::delBindColumn(int column)
{
}

RETCODE OdbcStatement::sqlBindCol(int column, int targetType, SQLPOINTER targetValuePtr, SQLINTEGER bufferLength, SQLINTEGER * indPtr)
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
			}
		}

		DescRecord *record = applicationRowDescriptor->getDescRecord (column);

		record->parameterType = SQL_PARAM_OUTPUT;
		record->type = targetType;
		record->conciseType = targetType;
		record->dataPtr = targetValuePtr;
		record->indicatorPtr = indPtr;
		record->length = bufferLength;
		record->isDefined = true;
		record->isPrepared = false;

		if ( implementationRowDescriptor->isDefined() )
		{
			if ( !column )
			{
				DescRecord *imprec = implementationRowDescriptor->getDescRecord (column);
				imprec->dataPtr = &rowNumber;
				imprec->indicatorPtr = &indicatorRowNumber;
				record->octetLengthPtr = &indicatorRowNumber;
				record->initZeroColumn();
			}

			bindOutputColumn ( column, record);
			record->length = bufferLength;
		}
	}
	catch (SQLException& exception)
	{
		postError ("HY000", exception);
		return SQL_ERROR;
	}

	return sqlSuccess();
}

inline
void OdbcStatement::setZeroColumn(int column)
{
	CBindColumn * bindCol = listBindOut->GetHeadPosition();
	if ( bindCol && !bindCol->column )
		convert->setZeroColumn(bindCol->appRecord, column);
}

inline
RETCODE OdbcStatement::fetchData()
{
	SQLUINTEGER rowCount = 0;
	SQLUINTEGER *rowCountPt = implementationRowDescriptor->headRowsProcessedPtr ? implementationRowDescriptor->headRowsProcessedPtr
								: &rowCount;
	SQLUSMALLINT *statusPtr = implementationRowDescriptor->headArrayStatusPtr ? implementationRowDescriptor->headArrayStatusPtr
								: NULL;
	int nCountRow = applicationRowDescriptor->headArraySize;
	SQLINTEGER	*bindOffsetPtrSave = bindOffsetPtr;

	try
	{
		int nRow = 0;

		if ( !eof )
		{
			SQLINTEGER	bindOffsetPtrTmp = bindOffsetPtr ? *bindOffsetPtr : 0;
			bindOffsetPtr = &bindOffsetPtrTmp;

			if ( schemaFetchData )
			{
				convert->setBindOffsetPtrTo(bindOffsetPtr, bindOffsetPtr);
				while ( nRow < nCountRow && (resultSet->*fetchNext)() )
				{
					++countFetched;
					++rowNumber; // Should stand only here!!!

					returnData();
					
					bindOffsetPtrTmp += rowBindType;
					++nRow;
				}
				if ( statusPtr )
					memset(statusPtr, SQL_ROW_SUCCESS, sizeof(*statusPtr) * nRow);
			}
			else // if ( schemaExtendedFetchData )
			{
				SQLINTEGER	bindOffsetPtrData = 0;
				SQLINTEGER	bindOffsetPtrInd = 0;
				convert->setBindOffsetPtrTo(&bindOffsetPtrData, &bindOffsetPtrInd);
				while ( nRow < nCountRow && (resultSet->*fetchNext)() )
				{
					++countFetched;
					++rowNumber; // Should stand only here!!!

					returnDataFromExtendedFetch();
					
					bindOffsetPtrInd += sizeof(SQLINTEGER);
					++bindOffsetPtrTmp;
					++nRow;
				}
				if ( statusPtr )
					memset(statusPtr, SQL_ROW_SUCCESS, sizeof(*statusPtr) * nRow);
			}

			*rowCountPt = nRow;
			setZeroColumn(rowNumber);
			bindOffsetPtr = bindOffsetPtrSave;

			if( !nRow || nRow < nCountRow )
			{
				eof = true;
				if( nRow && statusPtr )
				{
					SQLUSMALLINT * pt = statusPtr + nRow;
					SQLUSMALLINT * ptEnd = statusPtr + nCountRow;
					while ( pt < ptEnd )
						*pt++ = SQL_ROW_NOROW;
				}
				else if ( !nRow )
					return SQL_NO_DATA;
			}
		}
		else
		{
			*rowCountPt = 0;
			return SQL_NO_DATA;
		}
	}
	catch (SQLException& exception)
	{
		bindOffsetPtr = bindOffsetPtrSave;
		OdbcError *error = postError ("HY000", exception);
		error->setRowNumber (rowNumber);
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
	{
		enFetch = Fetch;
		schemaFetchData = getSchemaFetchData();
		rebindColumn();
		convert->setBindOffsetPtrFrom(sqldataOutOffsetPtr, NULL);
		isFetchStaticCursor = isStaticCursor();
	}

	if ( isFetchStaticCursor )
		return sqlFetchScrollCursorStatic ( SQL_FETCH_NEXT, 1);

	return fetchData();
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
	SQLUINTEGER rowCount = 0;
	SQLUINTEGER *rowCountPt =	implementationRowDescriptor->headRowsProcessedPtr ? implementationRowDescriptor->headRowsProcessedPtr
								: &rowCount;

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
		if ( eof && rowNumber == sqlDiagCursorRowCount - 1 )
			return SQL_NO_DATA;

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

		if( rowNumber >= sqlDiagCursorRowCount )
		{
			resultSet->afterLast();
			resultSet->setPosRowInSet(sqlDiagCursorRowCount ? sqlDiagCursorRowCount - 1 : 0);
			return SQL_NO_DATA;
		}
		break;

	default:
		return sqlReturn (SQL_ERROR, "HY106", "Fetch type out of range");
	}

	if( rowNumber >= 0 && rowNumber < sqlDiagCursorRowCount )
	{
		int nRow = 0;
		resultSet->setPosRowInSet(rowNumber);
		eof = false;

		if ( fetchRetData == SQL_RD_OFF )
		{
			convert->setBindOffsetPtrTo(bindOffsetPtr, bindOffsetPtr);
			setZeroColumn(rowNumber);
			*rowCountPt = 0;
		}
		else // if ( !eof )
		{
			SQLINTEGER	*bindOffsetPtrSave = bindOffsetPtr;
			SQLUSMALLINT *statusPtr = implementationRowDescriptor->headArrayStatusPtr ? implementationRowDescriptor->headArrayStatusPtr
										: NULL;
			SQLINTEGER	bindOffsetPtrTmp = bindOffsetPtr ? *bindOffsetPtr : 0;

			bindOffsetPtr = &bindOffsetPtrTmp;
			resultSet->setCurrentRowInBufferStaticCursor(rowNumber);

			if( rowBindType )
			{
				convert->setBindOffsetPtrTo(bindOffsetPtr, bindOffsetPtr);
				while ( nRow < rowsetSize && rowNumber < sqlDiagCursorRowCount )
				{
					resultSet->copyNextSqldaFromBufferStaticCursor();
					++countFetched;
					++rowNumber; // Should stand only here!!!

					returnData();

					bindOffsetPtrTmp += rowBindType;
					++nRow;
				}
				if ( statusPtr )
					memset(statusPtr, SQL_ROW_SUCCESS, sizeof(*statusPtr) * nRow);
			}
			else
			{
				SQLINTEGER	bindOffsetPtrData = 0;
				SQLINTEGER	bindOffsetPtrInd = 0;
				convert->setBindOffsetPtrTo(&bindOffsetPtrData, &bindOffsetPtrInd);
				while ( nRow < rowsetSize && rowNumber < sqlDiagCursorRowCount )
				{
					resultSet->copyNextSqldaFromBufferStaticCursor();
					++countFetched;
					++rowNumber; // Should stand only here!!!

					returnDataFromExtendedFetch();

					bindOffsetPtrInd += sizeof(SQLINTEGER);
					++bindOffsetPtrTmp;
					++nRow;
				}
				if ( statusPtr )
					memset(statusPtr, SQL_ROW_SUCCESS, sizeof(*statusPtr) * nRow);
			}
			
			*rowCountPt = nRow;
			bindOffsetPtr = bindOffsetPtrSave;

			if( !nRow || nRow < rowsetSize )
			{
				eof = !nRow || rowNumber == sqlDiagCursorRowCount;

				if( nRow && statusPtr )
				{
					SQLUSMALLINT * pt = statusPtr + nRow;
					SQLUSMALLINT * ptEnd = statusPtr + rowsetSize;
					while ( pt < ptEnd )
						*pt++ = SQL_ROW_NOROW;
				}
				else if ( !nRow && eof )
					return SQL_NO_DATA;
			}
		}
	}
	else if ( rowNumber < 0 )
	{
		rowNumber = 0;
		convert->setBindOffsetPtrTo(bindOffsetPtr, bindOffsetPtr);
		setZeroColumn(rowNumber);
		*rowCountPt = 0;
		return SQL_NO_DATA;
	}
	else 
	{
		rowNumber = sqlDiagCursorRowCount - 1;
		convert->setBindOffsetPtrTo(bindOffsetPtr, bindOffsetPtr);
		setZeroColumn(rowNumber);
		*rowCountPt = 0;
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
		resultSet->setPosRowInSet(rowNumber);
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

	if (!resultSet)
		return sqlReturn (SQL_ERROR, "24000", "Invalid cursor state");

	if( enFetch == NoneFetch )
	{
		enFetch = FetchScroll;
		schemaFetchData = getSchemaFetchData();
		convert->setBindOffsetPtrFrom(sqldataOutOffsetPtr, NULL);
		isFetchStaticCursor = isStaticCursor();
	}

	if( cursorType == SQL_CURSOR_FORWARD_ONLY && orientation != SQL_FETCH_NEXT )
		return sqlReturn (SQL_ERROR, "HY106", "Fetch type out of range");

	if (cancel)
	{
		releaseResultSet();
		return sqlReturn (SQL_ERROR, "S1008", "Operation canceled");
	}

	if ( isFetchStaticCursor )
		return sqlFetchScrollCursorStatic(orientation,offset);

	return fetchData();
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
	{
		enFetch = ExtendedFetch;
		schemaFetchData = getSchemaFetchData();
		convert->setBindOffsetPtrFrom(sqldataOutOffsetPtr, NULL);
		isFetchStaticCursor = isStaticCursor();
	}

	implementationRowDescriptor->headRowsProcessedPtr = rowCountPointer;
	implementationRowDescriptor->headArrayStatusPtr = rowStatusArray;
	applicationRowDescriptor->headArraySize = rowArraySize ? rowArraySize : 1;

	if ( isFetchStaticCursor )
		return sqlFetchScrollCursorStatic (orientation, offset);

	return fetchData();
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
		++countFetched;
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
	convert->setBindOffsetPtrFrom(NULL, NULL);
	convert->setBindOffsetPtrTo(NULL, NULL);

	try
	{
		switch (option)
		{
		case SQL_CLOSE:
			setPreCursorName = false;
			releaseResultSet();
			statement->close();
			implementationParamDescriptor->setDefined( false );
			implementationParamDescriptor->clearPrepared();
			applicationParamDescriptor->clearPrepared();
			break;

		case SQL_UNBIND:
			releaseBindings();
			break;

		case SQL_RESET_PARAMS:
			releaseParameters();
			break;
		
		case SQL_DROP:
			statement->release();
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
	listBindOut->removeAll();
	enFetch = NoneFetch;

	if ( implementationGetDataDescriptor )
	{
		delete implementationGetDataDescriptor;
		implementationGetDataDescriptor = NULL;
		delete listBindGetData;
		listBindGetData = NULL;
	}
}

void OdbcStatement::releaseParameters()
{
	listBindIn->removeAll();

	if ( statement->isActiveProcedure() )
		listBindOut->removeAll();

	implementationParamDescriptor->setDefined( false );
	implementationParamDescriptor->clearPrepared();
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

	if ( columns )
		*columns = numberColumns;

	return SQL_SUCCESS;
}

RETCODE OdbcStatement::sqlNumParams(SWORD * params)
{
	clearErrors();
	
	if ( statement->isActive() )
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
		StatementMetaData *metaData = getStatementMetaDataIRD();
		const char *name = metaData->getColumnName (col);
		setString (name, colName, nameSize, nameLength);
		if (sqlType)
			*sqlType = metaData->getColumnType (col, realSqlType);
		if (precision)
			*precision = metaData->getPrecision (col);
		if (scale)
			*scale = metaData->getScale (col);
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

RETCODE OdbcStatement::prepareGetData(int column, DescRecord *recordARD)
{
	DescRecord *recordIRD = implementationRowDescriptor->getDescRecord(column);

	if ( !recordIRD->isDefined )
		implementationRowDescriptor->defFromMetaDataOut(column,recordIRD);

	if( recordARD->conciseType == SQL_C_DEFAULT )
	{
		int length = recordARD->length;
		recordIRD->setDefault(recordARD);
		recordARD->length = length;
		recordARD->conciseType = implementationRowDescriptor->getDefaultFromSQLToConciseType(recordIRD->type, recordARD->length);
	}

	recordARD->fnConv = convert->getAdresFunction(recordIRD,recordARD);

	if ( !recordARD->fnConv )
	{
		postError ("07006", "Restricted data type attribute violation");
		return SQL_ERROR;
	}

	recordARD->isPrepared = true;
	(*listBindGetData)(column) = CBindColumn(column,recordIRD,recordARD);

	return SQL_SUCCESS;
}

RETCODE OdbcStatement::sqlGetData(int column, int cType, PTR pointer, int bufferLength, SDWORD * indicatorPointer)
{
	clearErrors();

	if( !implementationGetDataDescriptor )
	{
		if ( !listBindGetData )
			listBindGetData = new ListBindColumn;
		else
			listBindGetData->removeAll();

		implementationGetDataDescriptor = connection->allocDescriptor (odtImplementationGetData);
		convert->setBindOffsetPtrFrom(sqldataOutOffsetPtr, NULL);
		implementationGetDataDescriptor->getDescRecord (implementationRowDescriptor->headCount, false);
	}

	DescRecord *record = implementationGetDataDescriptor->getDescRecord (column);
	
	if ( record->callType != cType )
	{
		record->parameterType = SQL_PARAM_OUTPUT;

		if ( cType == SQL_ARD_TYPE )
		{
			DescRecord *recordArd = applicationRowDescriptor->getDescRecord (column);
			*record = recordArd;
		}
//		else if ( cType == SQL_APD_TYPE )
//		{
//			DescRecord *recordApd = applicationParamDescriptor->getDescRecord (column);
//			*record = recordApd;
//		}
		else
		{
			record->type = cType;
			record->length = bufferLength;
			record->conciseType = cType;
		}

		record->callType = cType;

		if ( prepareGetData(column, record) )
			return SQL_ERROR;
	}
	else if ( !record->isPrepared && prepareGetData(column, record) )
		return SQL_ERROR;
	
	record->dataPtr = pointer;
	record->length = bufferLength;
	record->indicatorPtr = indicatorPointer;

	if ( fetchRetData == SQL_RD_ON )
	{
		if ( isStaticCursor() )
			resultSet->getDataFromStaticCursor (column);

		try
		{
			CBindColumn &bindCol = (*listBindGetData)[column];
			convert->setBindOffsetPtrTo(NULL, NULL);

			int retcode = (convert->*bindCol.appRecord->fnConv)(bindCol.impRecord,bindCol.appRecord);
			if ( retcode )
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
	}

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlExecute()
{
	clearErrors();
	int retcode;

	try
	{
		enFetch = NoneFetch;
		releaseResultSet();
		parameterNeedData = 0;
		retcode = (this->*execute)();
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
	int retcode = sqlPrepare (sql, sqlLength);
	if (retcode && retcode != SQL_SUCCESS_WITH_INFO)
		return retcode;
	try
	{
		enFetch = NoneFetch;
		parameterNeedData = 0;
		retcode = (this->*execute)();
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

void OdbcStatement::rebindParam ( bool initAttrDataAtExec )
{
	int nCount = implementationParamDescriptor->metaDataIn->getColumnCount();
	int nCountApp = applicationParamDescriptor->headCount;

	for (int paramApp = 1, param = 1; param <= nCount && paramApp <= nCountApp; ++param, ++paramApp)
	{
		DescRecord * recordApp = applicationParamDescriptor->getDescRecord ( paramApp );
		if ( !recordApp->isPrepared && recordApp->isDefined )
			bindInputOutputParam ( param, recordApp );

		if ( initAttrDataAtExec )
		{
			long * length;

			if ( !applicationParamDescriptor->headBindOffsetPtr )
				length = recordApp->indicatorPtr;
			else
				length = (long*)((char*)recordApp->indicatorPtr + *applicationParamDescriptor->headBindOffsetPtr);
	
			recordApp->data_at_exec = length && 
				(*length == SQL_DATA_AT_EXEC || *length <= SQL_LEN_DATA_AT_EXEC_OFFSET);
		}
	}
}

void OdbcStatement::addBindParam(int param, DescRecord * recordFrom, DescRecord * recordTo)
{
	CBindColumn bindCol(param, recordFrom, recordTo);

	int j = listBindIn->SearchAndInsert( &bindCol );
	if( j < 0 )
		(*listBindIn)[-j-1] = bindCol;
	else
		(*listBindIn)[j] = bindCol;
}

void OdbcStatement::delBindParam(int param)
{

}

RETCODE OdbcStatement::sqlDescribeParam(int parameter, SWORD * sqlType, UDWORD * precision, SWORD * scale, SWORD * nullable)
{
	clearErrors();

	if ( !statement->isActive() )
		return sqlReturn (SQL_ERROR, "HY010", "Function sequence error");

	if ( parameter < 1 )
		return sqlReturn (SQL_ERROR, "07009", "Invalid descriptor index");

	OdbcDesc *& ipd = implementationParamDescriptor;
	StatementMetaData *metaData = ipd->metaDataIn;
	int paramCount = ipd->metaDataIn->getColumnCount();
	int realSqlType;

	if ( statement->isActiveProcedure() && parameter > paramCount )
	{
		metaData = implementationParamDescriptor->metaDataOut;
		parameter -= paramCount;

		if ( parameter > ipd->metaDataOut->getColumnCount() )
			return sqlReturn (SQL_ERROR, "07009", "Invalid descriptor index");
	}

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
			 case SQL_TINYINT:
				cType = SQL_C_STINYINT;
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
			 case SQL_REAL:
				cType = SQL_C_FLOAT;
				break;
			 case SQL_FLOAT:
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
		record->data_at_exec = false; // Is defined in the moment SQLExecute or SqlExecDirect
		record->startedTransfer = false;
		record->isDefined = true;
		record->isPrepared = false;

		DescRecord *imprec = implementationParamDescriptor->getDescRecord (parameter);

		imprec->parameterType = type; // SQL_PARAM_INPUT, SQL_PARAM_OUTPUT, SQL_PARAM_INPUT_OUTPUT
		imprec->type = sqlType;
		imprec->conciseType = sqlType;
		imprec->isDefined = false;
		imprec->isPrepared = false;

		switch (sqlType)
		{
		 case SQL_TIME:
		 case SQL_TYPE_TIME:
		 case SQL_TIMESTAMP:
		 case SQL_TYPE_TIMESTAMP:
			 imprec->precision = scale;
		 case SQL_CHAR:
		 case SQL_VARCHAR:
		 case SQL_LONGVARCHAR:
		 case SQL_BINARY:
		 case SQL_VARBINARY:
		 case SQL_LONGVARBINARY:
		 case SQL_DATE:
		 case SQL_TYPE_DATE:
			 imprec->length = precision;
			break;
		 case SQL_DECIMAL:
		 case SQL_NUMERIC:
			 imprec->scale = scale;
		 case SQL_REAL:
		 case SQL_FLOAT:
		 case SQL_DOUBLE:
			 imprec->precision = precision;
			 break;
		}

		if ( implementationParamDescriptor->isDefined() )
			implementationParamDescriptor->setDefined ( false );

		registrationOutParameter = false;
	}
	catch (SQLException& exception)
	{
		postError ("HY000", exception);
		return SQL_ERROR;
	}

	return sqlSuccess();
}

RETCODE OdbcStatement::sqlCancel()
{
	try
	{
		cancel = true;
	}
	catch (SQLException& exception)
	{
		postError ("HY000", exception);
		return SQL_ERROR;
	}

	return SQL_SUCCESS;
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
		if( !statement->isActive() )
			setPreCursorName = true;
		else
		{
			statement->setCursorName (cursorName);
			setPreCursorName = false;
		}
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
		setPreCursorName = false;
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
				value = (long) implementationRowDescriptor->headArrayStatusPtr;
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

			case SQL_ATTR_ENABLE_AUTO_IPD:		// 15 
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

inline
RETCODE OdbcStatement::transferDataToBlobParam ( DescRecord *recordApp )
{
	RETCODE ret = SQL_SUCCESS;
	recordApp->endBlobDataTransfer();
	CBindColumn &bindCol = (*listBindIn)[ parameterNeedData - 1 ];
	switch (recordApp->conciseType)
	{
	case SQL_C_CHAR:
		ret = convert->convStreamHexStringToBlob(bindCol.appRecord,bindCol.impRecord);
		break;
	case SQL_C_BINARY:
		ret = convert->convStreamToBlob(bindCol.appRecord,bindCol.impRecord);
		break;
	}
	return ret;
}

void OdbcStatement::bindInputOutputParam(int param, DescRecord * recordApp)
{
	OdbcDesc * ipd = implementationParamDescriptor;
	StatementMetaData * metaDataIn = ipd->metaDataIn;
	StatementMetaData * metaDataOut = ipd->metaDataOut;

	if ( !metaDataOut && !metaDataIn )
		return;

	DescRecord *record = ipd->getDescRecord ( param );

	if ( record->parameterType != SQL_PARAM_OUTPUT && param <= metaDataIn->getColumnCount() )
	{
		if ( !record->isDefined )
			ipd->defFromMetaDataIn( param, record );
		
		if( recordApp->conciseType == SQL_C_DEFAULT )
		{
			record->setDefault ( recordApp );
			recordApp->conciseType = ipd->getDefaultFromSQLToConciseType ( record->type );
		}

		recordApp->fnConv = convert->getAdresFunction ( recordApp, record );

//		if ( convert->isIdentity() )
			addBindParam ( param, record, recordApp );
	}
	else if ( param -= metaDataIn->getColumnCount(), param <= metaDataOut->getColumnCount() )
	{
		if ( !record->isDefined )
			ipd->defFromMetaDataOut( param, record );

		if( recordApp->conciseType == SQL_C_DEFAULT )
		{
			record->setDefault ( recordApp );
			recordApp->conciseType = ipd->getDefaultFromSQLToConciseType(record->type);
		}

		record->fnConv = convert->getAdresFunction(record, recordApp);

//		if ( convert->isIdentity() )
			addBindColumn ( param, record, recordApp );
	}
	else
		return;

	record->isPrepared = true;
	recordApp->isPrepared = true;
}

void OdbcStatement::bindOutputColumn(int column, DescRecord * recordApp)
{
	if ( !implementationRowDescriptor->metaDataOut )
		return;
	
	OdbcDesc * ird = implementationRowDescriptor;
	DescRecord *record = ird->getDescRecord ( column );

	if( !column )
		recordApp->setDefault ( record );
	else
	{
		if ( !record->isDefined )
			ird->defFromMetaDataOut ( column, record );
		
		if( recordApp->conciseType == SQL_C_DEFAULT )
		{
			record->setDefault ( recordApp );
			recordApp->conciseType = ird->getDefaultFromSQLToConciseType(record->type);
		}
	}

	record->fnConv = convert->getAdresFunction ( record, recordApp );
	recordApp->sizeColumnExtendedFetch = ird->getConciseSize ( recordApp->conciseType, recordApp->length );

//	if ( convert->isIdentity() )
		addBindColumn ( column, record, recordApp );

	record->isPrepared = true;
	recordApp->isPrepared = true;
}

bool OdbcStatement::registerOutParameter()
{
	registrationOutParameter = true;

	int nCountApp = applicationParamDescriptor->headCount;
	int param = implementationParamDescriptor->metaDataIn->getColumnCount() + 1;

	if ( nCountApp >= param + numberColumns )
	{
		postError ("07002", "COUNT field incorrect");
		return false;
	}

	isRegistrationOutParameter = param <= nCountApp;

	for ( ; param <= nCountApp; ++param)
	{
		DescRecord * recordApp = applicationParamDescriptor->getDescRecord ( param );
		if ( !recordApp->isPrepared && recordApp->isDefined )
			bindInputOutputParam ( param, recordApp );
	}

	if ( !implementationParamDescriptor->headCount ) // count input param
		convert->setBindOffsetPtrFrom ( applicationParamDescriptor->headBindOffsetPtr, applicationParamDescriptor->headBindOffsetPtr );
	return true;
}

RETCODE OdbcStatement::inputParam()
{
	StatementMetaData *metaData = statement->getStatementMetaDataIPD();
	int nInputParam = metaData->getColumnCount();

	if( nInputParam )
	{
		if(parameterNeedData == 0)
		{
			if ( !implementationParamDescriptor->isDefined() )
			{
				implementationParamDescriptor->setDefined(true);
				rebindParam( true );
			}

			if ( listBindIn->GetCount() < nInputParam )
			{
				postError ("07002", "COUNT field incorrect");
				return SQL_ERROR;
			}

			parameterNeedData = 1;
			convert->setBindOffsetPtrFrom ( applicationParamDescriptor->headBindOffsetPtr, applicationParamDescriptor->headBindOffsetPtr );
			convert->setBindOffsetPtrTo(NULL, NULL);
		}

		for (int n = parameterNeedData; n <= nInputParam; ++n)
		{
			DescRecord *record = applicationParamDescriptor->getDescRecord (n);

			if ( record->data_at_exec )
			{
				parameterNeedData = n;
				
				if ( record->startedTransfer )
				{
					if ( record->isBlobOrArray )
						transferDataToBlobParam ( record );
					else
					{
						record->startedTransfer = false;
						record->dataOffset = 0;
					}
					continue;
				}

				record->isBlobOrArray = metaData->isBlobOrArray ( parameterNeedData );

				if ( record->isBlobOrArray )
				{
					switch (record->conciseType)
					{
					case SQL_C_CHAR:
					case SQL_C_BINARY:
						if ( !record->dataBlobPtr )
							metaData->createBlobDataTransfer ( parameterNeedData, record->dataBlobPtr );
					}
				}

				return SQL_NEED_DATA;
			}
			else if( record->dataPtr || 
					 (record->indicatorPtr && *record->indicatorPtr == SQL_NULL_DATA) )
			{
				CBindColumn &bindCol = (*listBindIn)[n-1];
				RETCODE ret = (convert->*bindCol.appRecord->fnConv)(bindCol.appRecord,bindCol.impRecord);
			}
		}
	}

	return SQL_SUCCESS;
}

RETCODE OdbcStatement::executeStatement()
{
	RETCODE ret;

	if ( (ret = inputParam()) )
		return ret;

	statement->executeStatement();

	if ( statement->getMoreResults() )
		setResultSet (statement->getResultSet(), false);

	if ( statement->isActiveSelect() && isStaticCursor() )
	{
		resultSet->readStaticCursor(); 
		setCursorRowCount(resultSet->getCountRowsStaticCursor());
	}

	return SQL_SUCCESS;
}

RETCODE OdbcStatement::executeProcedure()
{
	RETCODE ret;

	if ( (ret = inputParam()) )
		return ret;

	if ( !registrationOutParameter )
		if ( !registerOutParameter() )
			return SQL_ERROR;

	if ( statement->executeProcedure() )
	{
		if ( isRegistrationOutParameter )
		{
			RETCODE retCode;

			++countFetched;
			convert->statusReturnData = true;

			CBindColumn * bindParam = listBindOut->GetHeadPosition();
			while( bindParam )
			{
				retCode = (convert->*bindParam->impRecord->fnConv)(bindParam->impRecord,bindParam->appRecord);
				if ( retCode != SQL_SUCCESS )
				{
					ret = retCode;
					if ( ret != SQL_SUCCESS_WITH_INFO )
						break;
				}
				bindParam = listBindOut->GetNext();
			}
			convert->statusReturnData = false;
		}
		else
		{
			releaseResultSet();

			if ( statement->getMoreResults() )
			{
				setResultSet (statement->getResultSet(), false);
				execute = &OdbcStatement::executeProcedure;
				fetchNext = &ResultSet::nextFromProcedure;
			}
		}
	}

	return ret;
}

RETCODE OdbcStatement::sqlGetTypeInfo(int dataType)
{
	clearErrors();
	releaseStatement();

	try
	{
		DatabaseMetaData *metaData = connection->getMetaData();
		setResultSet (metaData->getTypeInfo (dataType), false);
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

	*(unsigned long*)ptr = GETBOUNDADDRESS(binding);

	if( binding->indicatorPtr && binding->data_at_exec )
	{
		if ( binding->startedTransfer )
		{
			; // continue into (this->*execute)();
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
						if ( !binding->dataBlobPtr )
						{
							binding->startedTransfer = false;
							metaData->createBlobDataTransfer ( parameterNeedData, binding->dataBlobPtr );
						}
						if ( !binding->startedTransfer )
							binding->beginBlobDataTransfer();
					}
				}
			}

			return SQL_NEED_DATA;
		}
	}
	
	try
	{
		int saveParameter = parameterNeedData;

		retcode = (this->*execute)();
		
		if ( retcode == SQL_NEED_DATA && saveParameter != parameterNeedData )
		{
			binding = applicationParamDescriptor->getDescRecord ( parameterNeedData );
			*(unsigned long*)ptr = GETBOUNDADDRESS(binding);
		}
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

    if (parameterNeedData > implementationParamDescriptor->headCount)
		return sqlReturn (SQL_ERROR, "HY000", "General error :: OdbcStatement::sqlPutData");

	DescRecord *binding = applicationParamDescriptor->getDescRecord (parameterNeedData);

	if (valueSize == SQL_NULL_DATA)
	{
		binding->setNull();
		*binding->indicatorPtr = SQL_NULL_DATA;
	}
	else if ( binding->isBlobOrArray )
	{
		if ( !binding->startedTransfer )
			binding->beginBlobDataTransfer();

		switch (binding->conciseType)
		{
		case SQL_C_CHAR:
			if(valueSize == SQL_NTS)
				valueSize = strlen( (char*)value );

		case SQL_C_BINARY:
			if( valueSize )
				binding->putBlobSegmentData (valueSize, value);
			break;
		}
	}
	else
	{
		if ( !binding->startedTransfer )
			binding->startedTransfer = true;

		switch (binding->conciseType)
		{
		case SQL_C_CHAR:
			if(valueSize == SQL_NTS)
				valueSize = strlen( (char*)value );
		default:
			{
				CBindColumn &bindParam = (*listBindIn)[ parameterNeedData - 1 ];
				SQLPOINTER valueSave = binding->dataPtr;
				binding->dataPtr = value;
				*binding->indicatorPtr = valueSize;
				(convert->*bindParam.appRecord->fnConv)(bindParam.appRecord,bindParam.impRecord);
				binding->dataPtr = valueSave;
			}
			break;
		}
	}
	return sqlSuccess();
}

inline
RETCODE OdbcStatement::returnData()
{
	RETCODE retCode, ret = SQL_SUCCESS;
	convert->statusReturnData = true;
	CBindColumn * bindCol = listBindOut->GetHeadPosition();
	while( bindCol )
	{
		retCode = (convert->*bindCol->impRecord->fnConv)(bindCol->impRecord,bindCol->appRecord);

		if ( retCode != SQL_SUCCESS )
		{
			ret = retCode;
			if ( ret != SQL_SUCCESS_WITH_INFO )
				break;
		}

		bindCol = listBindOut->GetNext();
	}
	convert->statusReturnData = false;
	return ret;
}

inline
RETCODE OdbcStatement::returnDataFromExtendedFetch()
{
	RETCODE retCode, ret = SQL_SUCCESS;
	SQLINTEGER	&bindOffsetPtrTo = convert->getBindOffsetPtrTo();
	SQLINTEGER	&currentRow = *bindOffsetPtr;
	convert->statusReturnData = true;

	CBindColumn * bindCol = listBindOut->GetHeadPosition();
	while( bindCol )
	{
		DescRecord *& appRecord = bindCol->appRecord;
		bindOffsetPtrTo = appRecord->sizeColumnExtendedFetch * currentRow;

		retCode = (convert->*bindCol->impRecord->fnConv)(bindCol->impRecord, appRecord);

		if ( retCode != SQL_SUCCESS )
		{
			ret = retCode;
			if ( ret != SQL_SUCCESS_WITH_INFO )
				break;
		}

		bindCol = listBindOut->GetNext();
	}
	convert->statusReturnData = false;
	return ret;
}

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
				cursorScrollable = (int) ptr;

				if( cursorScrollable == SQL_NONSCROLLABLE )
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

			case SQL_ATTR_ENABLE_AUTO_IPD:			// 15 
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

			case SQL_ATTR_APP_ROW_DESC:
				applicationRowDescriptor = (OdbcDesc *)ptr;
				if ( !applicationRowDescriptor )
					applicationRowDescriptor = saveApplicationRowDescriptor;
				if ( applicationRowDescriptor->headAllocType == SQL_DESC_ALLOC_AUTO )
				{
					applicationRowDescriptor = saveApplicationRowDescriptor;
					return sqlReturn (SQL_ERROR, "HY017", "Invalid use of an automatically allocated descriptor handle");
				}
				TRACE02(SQL_ATTR_APP_ROW_DESC,(int) applicationRowDescriptor);
				break;

			case SQL_ATTR_APP_PARAM_DESC:
				applicationParamDescriptor = (OdbcDesc *)ptr;
				if ( !applicationParamDescriptor )
					applicationParamDescriptor = saveApplicationParamDescriptor;
				if ( applicationParamDescriptor->headAllocType == SQL_DESC_ALLOC_AUTO )
				{
					applicationParamDescriptor = saveApplicationParamDescriptor;
					return sqlReturn (SQL_ERROR, "HY017", "Invalid use of an automatically allocated descriptor handle");
				}
				TRACE02(SQL_ATTR_APP_PARAM_DESC,(int) applicationParamDescriptor);
				break;

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
		if ( statement->isActiveDDL() )
			*rowCount = statement->getUpdateCount();
		else if (!statement->isActive() && !resultSet)
			return sqlReturn (SQL_ERROR, "HY010", "Function sequence error");
		else if ( isStaticCursor() )
			*rowCount = sqlDiagCursorRowCount;
		else
		{
			if ( enFetch != NoneFetch )
				*rowCount = rowNumber;
			else if ( statement->isActive() )
				*rowCount = statement->getUpdateCount();
			else 
				*rowCount = -1;
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
	return sqlColAttribute(column, descType, buffer, bufferSize, length, valuePtr);
}

RETCODE OdbcStatement::sqlColAttribute(int column, int fieldId, SQLPOINTER attributePtr, int bufferLength, SQLSMALLINT *strLengthPtr, SQLPOINTER numericAttributePtr)
{
	clearErrors();
	int value;
	const char *string = NULL;
	int realSqlType;

	try
	{
		StatementMetaData *metaData = getStatementMetaDataIRD();
		switch (fieldId)
		{
		case SQL_DESC_LABEL:
			string = metaData->getColumnLabel (column);
			break;

		case SQL_DESC_BASE_COLUMN_NAME:
		case SQL_COLUMN_NAME:
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

		case SQL_COLUMN_COUNT:
		case SQL_DESC_COUNT:
			if ( statement && statement->isActiveProcedure() )
				value = 0;
			else
				value = metaData->getColumnCount();
			break;

		case SQL_DESC_TYPE:
		case SQL_DESC_CONCISE_TYPE:
			value = metaData->getColumnType (column, realSqlType);
			break;

		case SQL_COLUMN_LENGTH:
		case SQL_DESC_LENGTH:
			value = metaData->getColumnDisplaySize (column);
			break;

		case SQL_COLUMN_PRECISION:
		case SQL_DESC_PRECISION:
			value = metaData->getPrecision (column);
			break;

		case SQL_COLUMN_SCALE:
		case SQL_DESC_SCALE:
			value = metaData->getScale (column);
			break;

		case SQL_DESC_DISPLAY_SIZE:
			value = metaData->getColumnDisplaySize (column);
			break;

		case SQL_COLUMN_NULLABLE:
		case SQL_DESC_NULLABLE:
			value = (metaData->isNullable (column)) ? SQL_NULLABLE : SQL_NO_NULLS;
			break;

		case SQL_DESC_FIXED_PREC_SCALE:
			value = (metaData->isCurrency (column)) ? 1 : 0;
			break;

		case SQL_DESC_AUTO_UNIQUE_VALUE: // REVISAR
		    value = (metaData->isAutoIncrement (column)) ? 1 : 0;
		    break;

		case SQL_DESC_CASE_SENSITIVE:
			value = (metaData->isCaseSensitive (column)) ? SQL_TRUE : SQL_FALSE;
			break;

		case SQL_DESC_SEARCHABLE:
			value = (metaData->isSearchable (column)) ? SQL_PRED_SEARCHABLE : SQL_PRED_NONE;
			break;

		case SQL_DESC_TYPE_NAME:
			string = metaData->getColumnTypeName (column);               
			break; 

		case SQL_DESC_BASE_TABLE_NAME:
		case SQL_DESC_TABLE_NAME:
			string = metaData->getTableName (column);
			break;

		case SQL_DESC_SCHEMA_NAME:
			string = metaData->getSchemaName (column);
			break;

		case SQL_DESC_CATALOG_NAME:
			string = metaData->getCatalogName (column);
			break;

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

	if (!statement->isActive() || !statement->getMoreResults() || statement->isActiveProcedure() )
		return SQL_NO_DATA;

	return SQL_SUCCESS;
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
		{
			resultSet->setPosRowInSet(sqlDiagCursorRowCount ? sqlDiagCursorRowCount - 1 : 0);
			eof = true;
		}
	}
	catch (SQLException& exception)
	{
		postError ("HY000", exception);
		return SQL_ERROR;
	}

	return sqlSuccess();
}

}; // end namespace OdbcJdbcLibrary
