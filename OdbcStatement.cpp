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
#include "OdbcStatement.h"
#include "OdbcConnection.h"
#include "OdbcError.h"
#include "Connection.h"
#include "SQLException.h"
#include "OdbcDateTime.h"
#include <time.h>
#include "IscDbc/SqlTime.h"
#include "DateTime.h"
#include "TimeStamp.h"

#define RESULTS(fn)		(resultSet) ? resultSet->fn : callableStatement->fn
#define SKIP_WHITE(p)	while (charTable [*p] == WHITE) ++p

#define PUNCT			1
#define WHITE			2
#define DIGIT			4
#define LETTER			8
#define IDENT			(LETTER | DIGIT)

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
	bindings = NULL;
	getDataBindings = NULL;	//added by RM
	parameters = NULL;
	metaData = NULL;
	cancel = false;
	numberParameters = 0;
    parameterNeedData = -1;	//Added 2002-06-04 RM
    numberBindings = 0;
	numberGetDataBindings = 0;	//added by RM
	maxRows = 0;
	applicationRowDescriptor = connection->allocDescriptor (odtApplicationRow);
	applicationParamDescriptor = connection->allocDescriptor (odtApplicationParameter);
	implementationRowDescriptor = connection->allocDescriptor (odtImplementationRow);
	implementationParamDescriptor = connection->allocDescriptor (odtImplementationParameter);
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
	updatePreparedResultSet = true;	//added by CGA
}

OdbcStatement::~OdbcStatement()
{
	connection->statementDeleted (this);
	delete applicationRowDescriptor;
	delete applicationParamDescriptor;
	delete implementationRowDescriptor;
	delete implementationParamDescriptor;
	releaseStatement();
	releaseBindings();
	releaseParameters();
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

RETCODE OdbcStatement::sqlPrepare(SQLCHAR * sql, int sqlLength)
{
	clearErrors();
	releaseStatement();
	JString temp;
	const char *string = (const char*) sql;

	if (sqlLength != SQL_NTS)
		{
		temp = JString ((const char*) sql, sqlLength);
		string = temp;
		}

	try
		{
		if (isStoredProcedureEscape (string))
			statement = callableStatement = connection->connection->prepareCall (string);
		else
			statement = connection->connection->prepareStatement (string);
		}
	catch (SQLException& exception)
		{
		postError ("HY000", exception);
		return SQL_ERROR;
		}

	//Added by CGA
	if( updatePreparedResultSet )
		setResultSet(statement->getResultSet());

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
		}
}

void OdbcStatement::setResultSet(ResultSet * results)
{
	resultSet = results;
	metaData = resultSet->getMetaData();
	numberColumns = metaData->getColumnCount();
	eof = false;
	cancel = false;
	rowNumber = 0;
}

//RETCODE OdbcStatement::sqlBindCol(int column, int targetType, SQLPOINTER targetValuePtr, SQLINTEGER bufferLength, SQLINTEGER * indPtr)
RETCODE OdbcStatement::sqlBindCol(int column, int targetType, SQLPOINTER targetValuePtr, SQLINTEGER bufferLength, SQLINTEGER * indPtr, Binding** _bindings, int* _numberBindings)
{
	clearErrors();

//Added by RM
	if (!_bindings)
		_bindings = &bindings;

	if (!_numberBindings)
		_numberBindings = &numberBindings;

	int count = MAX (column, numberColumns);

	if (column < 0)
		return sqlReturn (SQL_ERROR, "07009", "Invalid descriptor index");

//Orig
//	if (count >= numberBindings)
//		{
//		bindings = allocBindings (count + 1, numberBindings, bindings);
//		numberBindings = count + 1;
//		}
//Added by RM
	if (count >= *_numberBindings)
		{
		*_bindings = allocBindings (count + 1, *_numberBindings, *_bindings);
		*_numberBindings = count + 1;
		}

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

//Orig
//		Binding *binding = bindings + column;
//From RM
		Binding *binding = *_bindings + column;	
		binding->type = SQL_PARAM_OUTPUT;
		binding->cType = targetType;
		binding->pointer = targetValuePtr;
		binding->bufferLength = bufferLength;
		binding->indicatorPointer = indPtr;
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
	if (!resultSet)
		return sqlReturn (SQL_ERROR, "24000", "Invalid cursor state");

	if (cancel)
		{
		releaseResultSet();
		return sqlReturn (SQL_ERROR, "S1008", "Operation canceled");
		}

		if (eof || !resultSet->next())
			{
			eof = true;
			if (implementationRowDescriptor->rowsProcessedPtr)
				*implementationRowDescriptor->rowsProcessedPtr = 0;
			return SQL_NO_DATA;
			}

	return returnData();
}

RETCODE OdbcStatement::sqlFetchScroll(int orientation, int offset)
{
	if (!resultSet)
		return sqlReturn (SQL_ERROR, "24000", "Invalid cursor state");

	if (cancel)
		{
		releaseResultSet();
		return sqlReturn (SQL_ERROR, "S1008", "Operation canceled");
		}

	try
		{
		if (eof || !resultSet->next())
			{
			eof = true;
			if (implementationRowDescriptor->rowsProcessedPtr)
				*implementationRowDescriptor->rowsProcessedPtr = 0;
			return SQL_NO_DATA;
			}
		}
	catch (SQLException& exception)
		{
		OdbcError *error = postError ("HY000", exception);
		error->setRowNumber (rowNumber);
		return SQL_ERROR;
		}

	return returnData();;
}

RETCODE OdbcStatement::sqlExtendedFetch(int orientation, int offset, SQLUINTEGER *rowCountPointer, SQLUSMALLINT *rowStatusArray)
{
	if (!resultSet)
		return sqlReturn (SQL_ERROR, "24000", "Invalid cursor state");

	if (cancel)
		{
		releaseResultSet();
		return sqlReturn (SQL_ERROR, "S1008", "Operation canceled");
		}

	try
		{
		if (eof || !resultSet->next())
			{
			eof = true;
			return SQL_NO_DATA;
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

bool OdbcStatement::setValue(Binding * binding, int column)
{
	bool info = false;
	int length = binding->bufferLength;
	int type = binding->cType;
	OdbcError *error = NULL;

	if (type == SQL_C_DEFAULT)
		{
		ResultSetMetaData *metaData = resultSet->getMetaData();
		type = getCType (metaData->getColumnType (column), 
						 metaData->isSigned (column));
		}

	switch (type)
		{
		case SQL_C_CHAR:
//Orig
/*			{
			int len = length - 1;
			const char *string = RESULTS (getString (column));
			length = strlen (string);
			if (length < len)
				strcpy ((char*) binding->pointer, string);
			else
				{
				memcpy (binding->pointer, string, len);
				((char*) (binding->pointer)) [len] = 0;
				error = postError (new OdbcError (0, "01004", "Data truncated"));
				info = true;
				}
			}
*/
//Added by PG
			{

			const char *string = RESULTS (getString (column));

			int dataRemaining = strlen(string) - binding->dataOffset;
			int len = MIN(dataRemaining, binding->bufferLength);
			 
			//Added by PR. If len is negative we get an AV so
			if ( len > 0 ) 
			{
				memcpy (binding->pointer, string+binding->dataOffset, len);						
				((char*) (binding->pointer)) [len] = 0;
			}

			if (len < dataRemaining)
			{
				error = postError (new OdbcError (0, "01004", "Data truncated"));
				info = true;
			}
				
			length = dataRemaining;
			binding->dataOffset += len;

			if (!info)
				binding->dataOffset = 0;
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
			*((double*) binding->pointer) = RESULTS (getDouble (column));
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
			*((QUAD*) binding->pointer) = RESULTS (getLong (column));
			length = sizeof(QUAD);
			break;

		case SQL_TYPE_DATE:
		case SQL_C_DATE:
			{
			OdbcDateTime converter;
			DateTime dateTime = RESULTS (getDate (column));
			tagDATE_STRUCT *var = (tagDATE_STRUCT*) binding->pointer;
			converter.convert (&dateTime, var);
			length = sizeof(tagDATE_STRUCT);
			}
			break;

		case SQL_TYPE_TIMESTAMP:
		case SQL_C_TIMESTAMP:
			{
			TimeStamp timestamp = RESULTS (getTimestamp (column));
			tagTIMESTAMP_STRUCT *var = (tagTIMESTAMP_STRUCT*) binding->pointer;
			OdbcDateTime converter;
			converter.convert (&timestamp, var);
			length = sizeof(tagTIMESTAMP_STRUCT);
			}
			break;

		case SQL_C_TIME:
			{
			SqlTime sqlTime = RESULTS (getTime (column));
			tagTIME_STRUCT *var = (tagTIME_STRUCT*) binding->pointer;
			var->hour = (unsigned short) (sqlTime.timeValue / 60 * 60) % 24;
			var->minute = (unsigned short) (sqlTime.timeValue / 60) % 60;
			var->second = (unsigned short) (sqlTime.timeValue % 60);
			length = sizeof (tagTIME_STRUCT);
			}
			break;

		case SQL_C_BINARY:
			{
			//for now, just get value so the wasNull check will work
			Blob* blob = RESULTS (getBlob(column));
//Orig
/*			length = blob->length();

			blob->getBytes (0, length, binding->pointer);
*/
//From RM
			int dataRemaining = blob->length() - binding->dataOffset;
			int len = MIN(dataRemaining, binding->bufferLength);
			 
			blob->getBytes (binding->dataOffset, len, binding->pointer);

			if (len < dataRemaining)
			{
				error = postError (new OdbcError (0, "01004", "Data truncated"));
				info = true;
			}
				
			length = dataRemaining;
			binding->dataOffset += len;

			if (!info)
				binding->dataOffset = 0;

			break;	
			}

		case SQL_C_BIT:
		//case SQL_C_BOOKMARK:
		//case SQL_C_VARBOOKMARK:
		case SQL_C_NUMERIC:
		//case SQL_C_GUID:
			//break;

		default:
			error = postError (new OdbcError (0, "HYC00", "Optional feature not implemented"));
			info = true;
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

	return info;
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
	numberBindings = 0;

	if (bindings)
		{
		delete [] bindings;
		bindings = NULL;
		}
//Added by RM
	numberGetDataBindings = 0;

	if (getDataBindings)
		{
		delete [] getDataBindings;
		getDataBindings = NULL;
		}

}

void OdbcStatement::releaseParameters()
{
	numberParameters = 0;
	paramsetSize = 0;
	paramsProcessedPtr = NULL;

	if (parameters)
		{
		delete [] parameters;
		parameters = NULL;
		}
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
			ResultSetMetaData *metaData = resultSet->getMetaData();
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
		ResultSetMetaData *metaData = resultSet->getMetaData();
		const char *name = metaData->getColumnName (col);
		setString (name, colName, nameSize, nameLength);
		if (sqlType)
			*sqlType = metaData->getColumnType (col);
		if (scale)
			*scale = metaData->getScale (col);
		if (precision)
			*precision = metaData->getPrecision (col);
		if (nullable)
			*nullable = (metaData->isNullable (col)) ? SQL_NULLABLE : SQL_NO_NULLS;
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

//Orig.
/*	Binding binding;
	binding.cType = cType;
	binding.pointer = pointer;
	binding.bufferLength = bufferLength;
	binding.indicatorPointer = indicatorPointer;
*/
//From RM
	int retcode = sqlBindCol(column, cType, pointer, bufferLength, indicatorPointer, &getDataBindings, &numberGetDataBindings);

	if (retcode && retcode != SQL_SUCCESS_WITH_INFO)
		return retcode;

	Binding* binding = getDataBindings + column;

	try
		{
//Orig.
//		if (setValue (&binding, column))
//From RM
		if (setValue (binding, column))
			return SQL_SUCCESS_WITH_INFO;
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
		if ( resultSet )
			releaseResultSet();
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
	updatePreparedResultSet = false;
	int retcode = sqlPrepare (sql, sqlLength);
	updatePreparedResultSet = true;


	if (retcode && retcode != SQL_SUCCESS_WITH_INFO)
		return retcode;

	try
		{
//		executeStatement();
//Amended to return RETCODE 2002-06-04 RM
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

	if (resultSet)
		releaseResultSet();

	if (!statement->getMoreResults())
		return NULL;

	setResultSet (statement->getResultSet());

	return resultSet;
}

RETCODE OdbcStatement::sqlDescribeParam(int parameter, SWORD * sqlType, UDWORD * precision, SWORD * scale, SWORD * nullable)
{
	clearErrors();
	StatementMetaData *metaData = statement->getStatementMetaData();

	try
		{
		if (sqlType)
			*sqlType = metaData->getParameterType (parameter);

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
{	clearErrors();
	return sqlBindParameter (parameter, SQL_PARAM_INPUT_OUTPUT, cType, sqlType, precision, scale, ptr, SQL_SETPARAM_VALUE_MAX, length);
}


RETCODE OdbcStatement::sqlBindParameter(int parameter, int type, int cType, int sqlType, int precision, int scale, PTR ptr, int bufferLength, SDWORD * length)
{
	clearErrors();

	if (parameter <= 0)
		return sqlReturn (SQL_ERROR, "S1093", "Invalid parameter number");

	int parametersNeeded = parameter;

	try
		{
		if (parameter > numberParameters)
			{
			if (statement)
				{
				StatementMetaData *metaData = statement->getStatementMetaData();
				int n = metaData->getParameterCount();
				parametersNeeded = MAX (parametersNeeded, n);
				}
			parameters = allocBindings (parametersNeeded, numberParameters, parameters);
			numberParameters = parametersNeeded;
			}

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
			//case SQL_C_BOOKMARK:
			//case SQL_C_VARBOOKMARK:
			case SQL_C_TYPE_DATE:
			case SQL_C_TYPE_TIME:
			case SQL_C_TYPE_TIMESTAMP:
			case SQL_C_NUMERIC:
			//case SQL_C_GUID:
				break;

			default:
				return sqlReturn (SQL_ERROR, "S1C00", "Driver not capable");
			}

		Binding *binding = parameters + parameter - 1;
		binding->type = type;
		binding->cType = cType;
		binding->sqlType = sqlType;
		binding->pointer = ptr;
		binding->bufferLength = bufferLength;
		binding->indicatorPointer = length;
		}
	catch (SQLException& exception)
		{
		postError ("HY000", exception);
		return SQL_ERROR;
		}

	return sqlSuccess();
}

//void OdbcStatement::setParameter(Binding * binding, int parameter)
//Amended 2002-06-04 RM
RETCODE OdbcStatement::setParameter(Binding * binding, int parameter)
{
	//Added 2002-06-04 RM
    RETCODE retcode = SQL_SUCCESS;
    clearErrors();

	if (binding && binding->indicatorPointer && *(binding->indicatorPointer) == SQL_NULL_DATA)
		{
		statement->setNull(parameter, 0);
		//Amended to return status 2002-06-04 RM
		return SQL_SUCCESS;
        ;
		}

	try
		{
		switch (binding->cType)
			{
			case SQL_C_CHAR:
//Orig
//				statement->setString (parameter, (char*) binding->pointer);
//				break;
//Suggested by CGA to handle situation where strings are NOT null-terminated.
                switch( *binding->indicatorPointer )
                {
                    case SQL_NTS:
                        statement->setString (parameter, (char*) binding->pointer );
                        break;

                    default:                       
                        statement->setString (parameter, (char*)binding->pointer, *binding->indicatorPointer );
                        break;
                }


			case SQL_C_SHORT:
			case SQL_C_SSHORT:
			case SQL_C_USHORT:
				statement->setShort (parameter, *(short*) binding->pointer);
				break;

			case SQL_C_LONG:
			case SQL_C_SLONG:
			case SQL_C_ULONG:
				statement->setInt (parameter, *(long*) binding->pointer);
				break;

			case SQL_C_FLOAT:
				statement->setFloat (parameter, *(float*) binding->pointer);
				break;

			case SQL_C_DOUBLE:
				statement->setDouble (parameter, *(double*) binding->pointer);
				break;

			case SQL_C_TINYINT:
			case SQL_C_STINYINT:
			case SQL_C_UTINYINT:
				statement->setByte (parameter, *(char*) binding->pointer);
				break;

			case SQL_C_SBIGINT:
			case SQL_C_UBIGINT:
				statement->setLong (parameter, *(QUAD*) binding->pointer);
				break;

			case SQL_C_TYPE_TIMESTAMP:
				{
				OdbcDateTime converter;
				tagTIMESTAMP_STRUCT *var = (tagTIMESTAMP_STRUCT*) binding->pointer;
/*	Orig.
// this is obsolete ... I want more days ;-)

				DateTime dateTime;
				converter.convert (var, &dateTime);
				statement->setDate (parameter, dateTime);
*/
//From B. Schulte
				TimeStamp timestamp;
		        converter.convert (var, &timestamp);
				statement->setTimestamp ( parameter, timestamp);
				}
				break;

			case SQL_C_TYPE_DATE:
				{
				OdbcDateTime converter;
				tagDATE_STRUCT *var = (tagDATE_STRUCT*) binding->pointer;
				DateTime dateTime;
				converter.convert (var, &dateTime);
				statement->setDate (parameter, dateTime);
				}
				break;

			case SQL_C_TYPE_TIME:
				{
				tagTIME_STRUCT *var = (tagTIME_STRUCT*) binding->pointer;
				SqlTime dateTime;
				dateTime.timeValue = var->hour * 60 * 60 + var->minute * 60 + var->second;
				statement->setTime (parameter, dateTime);
				}
				break;

			case SQL_C_BINARY:
				//Added if block 2002-06-04 RM
                if (*binding->indicatorPointer < SQL_LEN_DATA_AT_EXEC_OFFSET)
                    {
                    binding->dataOffset = -(*binding->indicatorPointer) + SQL_LEN_DATA_AT_EXEC_OFFSET;
                    statement->beginDataTransfer(parameter);
                    retcode = SQL_NEED_DATA;
                    }
                else
                	statement->setBytes(parameter, *(binding->indicatorPointer), binding->pointer);
				break;

			case SQL_C_BIT:
			//case SQL_C_BOOKMARK:
			//case SQL_C_VARBOOKMARK:
			case SQL_C_NUMERIC:
			//case SQL_C_GUID:
				//break;

			default:
				postError (new OdbcError (0, "HYC00", "Optional feature not implemented"));
				//Amended to return errorcode 2002--6-04 RM
				return SQL_ERROR;
			}
		}
	catch (SQLException& exception)
		{
		postError ("HY000", exception);
		//Amended to return errorcode 2002-6-04 RM
		return SQL_ERROR;
		}

	//Amended to return errorcode 2002-6-04 RM
    return retcode;
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
			case SQL_ATTR_APP_ROW_DESC:
				value = (long) applicationRowDescriptor;
				break;

			case SQL_ATTR_APP_PARAM_DESC:
				value = (long) applicationParamDescriptor;
				break;

			case SQL_ATTR_IMP_ROW_DESC:
				value = (long) implementationRowDescriptor;
				break;

			case SQL_ATTR_IMP_PARAM_DESC:
				value = (long) implementationParamDescriptor;
				break;

			case SQL_ATTR_CURSOR_TYPE:
				value = SQL_CURSOR_FORWARD_ONLY;
				break;

			case SQL_ATTR_CONCURRENCY:
				value = SQL_CONCUR_LOCK;
				break;

			case SQL_ATTR_ROW_ARRAY_SIZE:
				value = rowArraySize;
				break;

			case SQL_ATTR_MAX_ROWS:					// SQL_MAX_ROWS 1
				value = maxRows;
				break;
				
			case SQL_ATTR_MAX_LENGTH:
				value = 0;					//driver does not truncat
				break;

			case SQL_ATTR_QUERY_TIMEOUT:
				value = 0;							// driver doesn't timeout
				break;

			case SQL_ATTR_ASYNC_ENABLE:
				value = 0;							// driver doesn do async				
				break;

			case	SQL_ATTR_PARAM_BIND_TYPE:
				value = SQL_PARAM_BIND_BY_COLUMN;	// no row binding
				break;

			case SQL_ATTR_RETRIEVE_DATA:
				value = SQL_RD_ON;					// fetch returns data
				break;

			case SQL_ATTR_ROW_NUMBER:
				value = (long) rowNumber;
				break;

			case SQL_ATTR_ROW_BIND_TYPE:
				value = (long) rowBindType;
				break;

			case SQL_ATTR_ROW_STATUS_PTR:
				value = (long) rowStatusPtr;
				break;


			/***
			case SQL_ATTR_ENABLE_AUTO_IPD			15
			case SQL_ATTR_FETCH_BOOKMARK_PTR			16
			case SQL_ATTR_KEYSET_SIZE				SQL_KEYSET_SIZE
			case SQL_ATTR_NOSCAN						SQL_NOSCAN
			case SQL_ATTR_PARAM_BIND_OFFSET_PTR		17
			case SQL_ATTR_PARAM_OPERATION_PTR		19
			case SQL_ATTR_PARAM_STATUS_PTR			20
			case	SQL_ATTR_PARAMS_PROCESSED_PTR		21
			case	SQL_ATTR_PARAMSET_SIZE				22
			case SQL_ATTR_RETRIEVE_DATA				SQL_RETRIEVE_DATA
			case SQL_ATTR_ROW_BIND_OFFSET_PTR		23
			case SQL_ATTR_ROW_OPERATION_PTR			24
			case SQL_ATTR_ROW_STATUS_PTR				25
			caseSQL_ATTR_ROWS_FETCHED_PTR			26
			case SQL_ATTR_SIMULATE_CURSOR			SQL_SIMULATE_CURSOR
			case SQL_ATTR_USE_BOOKMARKS				SQL_USE_BOOKMARKS
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

//Added 2002-06-04 RM
RETCODE OdbcStatement::setParameters()
{
    RETCODE retcode = SQL_SUCCESS;
    for (int n = 0; n < numberParameters; ++n)
        {
        Binding *binding = parameters + n;
        if (binding->pointer && binding->type != SQL_PARAM_OUTPUT)
            {
            RETCODE rc = setParameter(binding, n + 1);
            if (rc == SQL_NEED_DATA)
                retcode = rc;
            else if (rc && rc != SQL_SUCCESS_WITH_INFO)
                return rc;              
            }
        }
    return retcode;
}

//Renamed executeStatement to executeSQL and extensively modified it
/*
void OdbcStatement::executeStatement()
{
	for (int n = 0; n < numberParameters; ++n)
		{
		Binding *binding = parameters + n;
		if (binding->pointer&& binding->type != SQL_PARAM_OUTPUT)
			setParameter (binding, n + 1);
		}

	if (callableStatement)
		for (int n = 0; n < numberParameters; ++n)
			{
			Binding *binding = parameters + n;
			if (binding->pointer && binding->type != SQL_PARAM_INPUT)
				callableStatement->registerOutParameter (n + 1, binding->sqlType);
			}
		
	statement->execute();
	connection->transactionStarted();

	if (callableStatement)
		for (int n = 0; n < numberParameters; ++n)
			{
			Binding *binding = parameters + n;
			if (binding->pointer && binding->type != SQL_PARAM_INPUT)
				setValue (binding, n + 1);
			}

	getResultSet();
}
*/

void OdbcStatement::executeSQL()
{
    if (callableStatement)
    for (int n = 0; n < numberParameters; ++n)
        {
        Binding *binding = parameters + n;
        if (binding->pointer&& binding->type != SQL_PARAM_OUTPUT)
            setParameter (binding, n + 1);
        }

    if (callableStatement)
        for (int n = 0; n < numberParameters; ++n)
            {
            Binding *binding = parameters + n;
            if (binding->pointer && binding->type != SQL_PARAM_INPUT)
                callableStatement->registerOutParameter (n + 1, binding->sqlType);
            }
        
    statement->execute();
    connection->transactionStarted();

    if (callableStatement)
        for (int n = 0; n < numberParameters; ++n)
            {
            Binding *binding = parameters + n;
            if (binding->pointer && binding->type != SQL_PARAM_INPUT)
                setValue (binding, n + 1);
            }

    getResultSet();
}


//Added/amended 2002-06-04 RM
RETCODE OdbcStatement::executeStatement()
{
    RETCODE retcode;

    retcode = setParameters();
    
    if (retcode == SQL_NEED_DATA)
        return retcode;

    executeSQL();

    return retcode;
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
		setResultSet (metaData->getTypeInfo ());
		}
	catch (SQLException& exception)
		{
		postError ("HY000", exception);
		return SQL_ERROR;
		}

	return sqlSuccess();
}

//Amended to iterate through parameter list instead of 
//releasing statement. 2002-06-04 RM
RETCODE OdbcStatement::sqlParamData(SQLPOINTER ptr)
{
	clearErrors();
//	releaseStatement();
    for (int n = 0; n < numberParameters; ++n)
        {
        Binding *binding = parameters + n;
        if (binding->pointer && binding->type != SQL_PARAM_OUTPUT)
            {
            if (ptr && binding->pointer && binding->dataOffset)
                {
                *(DWORD*)ptr = *(DWORD*)binding->pointer;
                parameterNeedData = n;
                return SQL_NEED_DATA;
                }
            }
        }
	return sqlSuccess();
}

RETCODE OdbcStatement::sqlSetStmtAttr(int attribute, SQLPOINTER ptr, int length)
{
	clearErrors();

	try
		{
		switch (attribute)
			{
			case SQL_QUERY_TIMEOUT:				// 0
				break;

			case SQL_ATTR_PARAM_BIND_TYPE:		// 18
				paramBindType = (int) ptr;
				break;

			case SQL_ATTR_PARAM_BIND_OFFSET_PTR:// 17
				paramBindOffset = ptr;
				break;

			case SQL_ATTR_PARAMS_PROCESSED_PTR:	// 21
				paramsProcessedPtr = ptr;
				break;

			case SQL_ATTR_PARAMSET_SIZE:		// 22
				paramsetSize = (int) ptr;
				break;

			case SQL_ATTR_ROW_BIND_TYPE:		// SQL_BIND_TYPE 5
				rowBindType = (int) ptr;
				break;

			case SQL_ATTR_ROW_ARRAY_SIZE:		// 27
				rowArraySize = (int) ptr;
				applicationRowDescriptor->descArraySize = rowArraySize;
				break;
					
			case SQL_ATTR_ROWS_FETCHED_PTR:		// 26
				implementationRowDescriptor->rowsProcessedPtr = (SQLUINTEGER*) ptr;
				break;

			case SQL_ATTR_ROW_BIND_OFFSET_PTR:	// 23
				bindOffsetPtr = (SQLINTEGER*) ptr;
				break;

			case SQL_ATTR_ROW_STATUS_PTR:		// 25
				rowStatusPtr = (SQLUSMALLINT*) ptr;
				break;
 
 			case SQL_ATTR_CONCURRENCY:			// SQL_CONCURRENCY	7
				currency = (int) ptr;
				break;

			case SQL_ATTR_CURSOR_TYPE:			// SQL_CURSOR_TYPE 6
				cursorType = (int) ptr;
				break;

			case SQL_ATTR_CURSOR_SCROLLABLE:
				cursorScrollable = (int) ptr == SQL_SCROLLABLE;
				break;

			case SQL_ATTR_ASYNC_ENABLE:			// 4
				asyncEnable = (int) ptr == SQL_ASYNC_ENABLE_ON;
				break;

			case SQL_ATTR_MAX_ROWS:					// SQL_MAX_ROWS 1
				maxRows = (int) ptr;
				break;
			
			case SQL_ATTR_MAX_LENGTH:
				if (!(long)ptr)
				    break;
				else 
				    return sqlReturn (SQL_SUCCESS_WITH_INFO, "01SO2", 
					    "Driver does not truncate");

			/***
			case SQL_ATTR_ASYNC_ENABLE				4
			case SQL_ATTR_CONCURRENCY				SQL_CONCURRENCY	7
			case SQL_ATTR_CURSOR_TYPE				SQL_CURSOR_TYPE 6
			case SQL_ATTR_ENABLE_AUTO_IPD			15
			case SQL_ATTR_FETCH_BOOKMARK_PTR			16
			case SQL_ATTR_KEYSET_SIZE				SQL_KEYSET_SIZE 8
			case SQL_ATTR_NOSCAN						SQL_NOSCAN 2
			case SQL_ATTR_PARAM_BIND_OFFSET_PTR		17
			case SQL_ATTR_PARAM_BIND_TYPE			18
			case SQL_ATTR_PARAM_OPERATION_PTR		19
			case SQL_ATTR_PARAM_STATUS_PTR			20
			case SQL_ATTR_PARAMS_PROCESSED_PTR		21
			case SQL_ATTR_PARAMSET_SIZE				22
			case SQL_ATTR_QUERY_TIMEOUT				SQL_QUERY_TIMEOUT
			case SQL_ATTR_RETRIEVE_DATA				SQL_RETRIEVE_DATA 11
			case SQL_ATTR_ROW_BIND_TYPE				SQL_BIND_TYPE 5
			case SQL_ATTR_ROW_NUMBER					SQL_ROW_NUMBER 14
			case SQL_ATTR_ROW_OPERATION_PTR			24
			case SQL_ATTR_ROW_STATUS_PTR				25
			case SQL_ATTR_ROW_ARRAY_SIZE				27	
			case SQL_ATTR_SIMULATE_CURSOR			SQL_SIMULATE_CURSOR 10
			case SQL_ATTR_USE_BOOKMARKS				SQL_USE_BOOKMARKS 13	
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
		*rowCount = statement->getUpdateCount();
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

	try
		{
		ResultSetMetaData *metaData = resultSet->getMetaData();
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
				value = (metaData->isWritable (column)) ? SQL_ATTR_WRITE : SQL_ATTR_READONLY;
				break;

			case SQL_COLUMN_COUNT:
				value = metaData->getColumnCount();
				break;

			case SQL_COLUMN_TYPE:
				value = metaData->getColumnType (column);
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
				value = metaData->getColumnType (column);
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
		if (bindings)
			for (columnNumber = 1; columnNumber <= numberColumns; ++columnNumber)
				{
				Binding *binding = bindings + columnNumber;
				if (binding->pointer && binding->type != SQL_PARAM_INPUT)
					setValue (binding, columnNumber);
				}

		if (implementationRowDescriptor->rowsProcessedPtr)
			*implementationRowDescriptor->rowsProcessedPtr = 1;
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

RETCODE OdbcStatement::sqlColAttribute(int column, int fieldId, SQLPOINTER attributePtr, int bufferLength, SQLSMALLINT *strLengthPtr, SQLPOINTER numericAttributePtr)
{
	clearErrors();
	int value;
	const char *string = NULL;

	try
		{
		ResultSetMetaData *metaData = resultSet->getMetaData();
		switch (fieldId)
			{
			case SQL_DESC_LABEL:
				string = metaData->getColumnLabel (column);
				break;

			case SQL_DESC_NAME:
				string = metaData->getColumnName (column);
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
				value = metaData->getColumnType (column);
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

			case SQL_DESC_AUTO_INCREMENT:
				value = (metaData->isAutoIncrement (column)) ? 1 : 0;
				break;
			***/

			case SQL_DESC_CASE_SENSITIVE:
				value = (metaData->isCaseSensitive (column)) ? SQL_TRUE : SQL_FALSE;
				break;

			case SQL_DESC_SEARCHABLE:
				value = (metaData->isSearchable (column)) ? SQL_PRED_SEARCHABLE : SQL_PRED_NONE;
				break;

			case SQL_DESC_TYPE_NAME:
				value = metaData->getColumnType (column);
				break;

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
		}
	catch (SQLException& exception)
		{
		postError ("HY000", exception);
		return SQL_ERROR;
		}

	return sqlSuccess();
}

//Filled in this method. 2002-06-04 RM
RETCODE OdbcStatement::sqlPutData (SQLPOINTER value, int valueSize)
{
    
    bool info = false;

    if (parameterNeedData == -1)
        return sqlReturn (SQL_ERROR, "HY010", "Function sequence error");

    if (parameterNeedData >= numberParameters)
        return sqlReturn (SQL_ERROR, "HY000", "General error");

    Binding *binding = parameters + parameterNeedData;

    if (!binding)
        return sqlReturn (SQL_ERROR, "HY000", "General error");

    if (binding->dataOffset > 0)
        {
        binding->dataOffset -= valueSize;
        if (binding->dataOffset < 0)
            {
            info = true;
            valueSize += binding->dataOffset;
            binding->dataOffset = 0;
            }
        
        statement->putSegmentData (valueSize, value);
        if (binding->dataOffset == 0)
            {
            parameterNeedData = -1;
            statement->endDataTransfer();
            executeSQL();
            }
        if (info)
            return sqlReturn (SQL_SUCCESS_WITH_INFO, "22001", "Data truncated");
        }

    	return sqlSuccess();
}
