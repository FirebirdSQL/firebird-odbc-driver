/*
 *  The contents of this file are subject to the J Public License 
 *  Version 1.0 (the "License"); you may not use this file except 
 *  in compliance with the License. You may obtain a copy of the 
 *  License at http://www.IBPhoenix.com/JPL.html
 *  
 *  Software distributed under the License is distributed on an 
 *  "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express 
 *  or implied.  See the License for the specific language governing 
 *  rights and limitations under the License. 
 *
 *  The Original Code was created by James A. Starkey
 *
 *  Copyright (c) 1999, 2000 James A. Starkey
 *  All Rights Reserved.
 */
// IscCallableStatement.cpp: implementation of the IscCallableStatement class.
//
//////////////////////////////////////////////////////////////////////

#include <string.h>
#include "IscDbc.h"
#include "IscCallableStatement.h"
#include "IscConnection.h"
#include "DateTime.h"
#include "SqlTime.h"
#include "TimeStamp.h"
#include "Value.h"
#include "SQLError.h"

#define SKIP_WHITE(p)	while (charTable [*p] == WHITE) ++p

#define PUNCT			1
#define WHITE			2
#define DIGIT			4
#define LETTER			8
#define QUOTE			16
#define IDENT			32

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
		charTable [n] = LETTER | IDENT;

	for (n = 'A'; n <= 'A'; ++n)
		charTable [n] = LETTER | IDENT;

	for (n = '0'; n <= '9'; ++n)
		charTable [n] = DIGIT | IDENT;

	charTable ['\''] = QUOTE;
	charTable ['"'] = QUOTE;
	charTable ['_'] = IDENT;

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscCallableStatement::IscCallableStatement(IscConnection *connection)
		: IscPreparedStatement (connection)
{
	minOutputVariable = 0;
}

IscCallableStatement::~IscCallableStatement()
{

}

ResultSet* IscCallableStatement::executeQuery()
{
	return Parent::executeQuery();
}

void IscCallableStatement::setInt(int index, long value)
{
	Parent::setInt(index, value);
}

void IscCallableStatement::setNull(int index, int type)
{
	Parent::setNull(index, type);
}

void IscCallableStatement::setDate(int index, DateTime value)
{
	Parent::setDate(index, value);
}

void IscCallableStatement::setDouble(int index, double value)
{
	Parent::setDouble(index, value);
}

void IscCallableStatement::setString(int index, const char * string)
{
	Parent::setString(index, string);
}

bool IscCallableStatement::execute()
{
	connection->startTransaction();
	ISC_STATUS statusVector [20];
	outputSqlda.allocBuffer();
	values.alloc (numberColumns);
	int numberParameters = inputSqlda.getColumnCount();
	void *transHandle = connection->startTransaction();
	int n;

	for (n = 0; n < numberParameters; ++n)
		inputSqlda.setValue (n, parameters.values + n, connection);

	int dialog = connection->getDatabaseDialect();
	if (isc_dsql_execute2 (statusVector, &transHandle, &statementHandle,
						  dialog, inputSqlda, outputSqlda))
		THROW_ISC_EXCEPTION (statusVector);

	resultsCount = 1;
	resultsSequence = 0;
	getUpdateCounts();

	XSQLVAR *var = outputSqlda.sqlda->sqlvar;
    Value *value = values.values;

	for (n = 0; n < numberColumns; ++n, ++var, ++value)
		setValue (value, var);

	return outputSqlda.sqlda->sqld > 0;
}

int IscCallableStatement::executeUpdate()
{
	return Parent::executeUpdate();
}

void IscCallableStatement::setBytes(int index, int length, const void* bytes)
{
	Parent::setBytes(index, length, bytes);
}


bool IscCallableStatement::execute (const char *sqlString)
{
	return Parent::execute(sqlString);
}

ResultSet*	 IscCallableStatement::executeQuery (const char *sqlString)
{
	return Parent::executeQuery(sqlString);
}

int	IscCallableStatement::getUpdateCount()
{
	return Parent::getUpdateCount();
}

bool IscCallableStatement::getMoreResults()
{
	return Parent::getMoreResults();
}

void IscCallableStatement::setCursorName (const char *name)
{
	Parent::setCursorName(name);
}

ResultSet* IscCallableStatement::getResultSet()
{
	return Parent::getResultSet();
}

ResultList* IscCallableStatement::search (const char *searchString)
{
	return Parent::search(searchString);
}

int	IscCallableStatement::executeUpdate (const char *sqlString)
{
	return Parent::executeUpdate(sqlString);
}

void IscCallableStatement::close()
{
	Parent::close();
}

int IscCallableStatement::release()
{
	return Parent::release();
}

void IscCallableStatement::addRef()
{
	Parent::addRef();
}

StatementMetaData* IscCallableStatement::getStatementMetaData()
{
	return Parent::getStatementMetaData();
}

void IscCallableStatement::setByte(int index, char value)
{
	Parent::setByte(index, value);
}

void IscCallableStatement::setLong(int index, QUAD value)
{
	Parent::setLong(index, value);
}

void IscCallableStatement::setFloat(int index, float value)
{
	Parent::setFloat(index, value);
}

void IscCallableStatement::setTime(int index, SqlTime value)
{
	Parent::setTime(index, value);
}

void IscCallableStatement::setTimestamp(int index, TimeStamp value)
{
	Parent::setTimestamp(index, value);
}

void IscCallableStatement::setShort(int index, short value)
{
	Parent::setShort(index, value);
}

void IscCallableStatement::setBlob(int index, Blob * value)
{
	Parent::setBlob(index, value);
}

void IscCallableStatement::setClob(int index, Clob * value)
{
	Parent::setClob(index, value);
}

int IscCallableStatement::objectVersion()
{
	return CALLABLESTATEMENT_VERSION;
}

short IscCallableStatement::getShort(int id)
{
	return getValue (id)->getShort();
}

char IscCallableStatement::getByte(int id)
{
	return getValue (id)->getByte();
}

long IscCallableStatement::getInt(int id)
{
	return getValue (id)->getLong();
}

QUAD IscCallableStatement::getLong(int id)
{
	return getValue (id)->getQuad();
}

float IscCallableStatement::getFloat(int id)
{
	return (float) getValue (id)->getDouble();
}

double IscCallableStatement::getDouble(int id)
{
	return getValue (id)->getDouble();
}

Clob* IscCallableStatement::getClob(int id)
{
	return getValue (id)->getClob();
}

Blob* IscCallableStatement::getBlob(int id)
{
	return getValue (id)->getBlob();
}

SqlTime IscCallableStatement::getTime(int id)
{
	return getValue (id)->getTime();
}

DateTime IscCallableStatement::getDate(int id)
{
	return getValue (id)->getDate();
}

TimeStamp IscCallableStatement::getTimestamp(int id)
{
	return getValue (id)->getTimestamp();
}

bool IscCallableStatement::wasNull()
{
	return valueWasNull;
}

const char* IscCallableStatement::getString(int id)
{
	return getValue (id)->getString();
}

Value* IscCallableStatement::getValue(int index)
{
	if (index < minOutputVariable || index >= minOutputVariable + numberColumns)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column index for procedure call");

	Value *value = values.values + index - minOutputVariable;
	valueWasNull = value->type == Null;

	return value;
}

void IscCallableStatement::registerOutParameter(int parameterIndex, int sqlType)
{
	minOutputVariable = (minOutputVariable == 0) ? parameterIndex : MIN (minOutputVariable, parameterIndex);
}

void IscCallableStatement::registerOutParameter(int parameterIndex, int sqlType, int scale)
{
	minOutputVariable = (minOutputVariable == 0) ? parameterIndex : MIN (minOutputVariable, parameterIndex);
}

void IscCallableStatement::prepare(const char * originalSql)
{
	char	buffer [1024];
	const char *sql = rewriteSql (originalSql, buffer, sizeof (buffer));

	Parent::prepare (sql);
}

const char* IscCallableStatement::rewriteSql(const char *originalSql, char *buffer, int length)
{
	const char *p = originalSql;
	char token [256];
	getToken (&p, token);

	if (token [0] != '{')
		return originalSql;

	getToken (&p, token);

	if (strcasecmp (token, "call") != 0)
		throw SQLEXCEPTION (SYNTAX_ERROR, "unsupported form of procedure call");

	char *q = buffer;
	strcpy (q, "execute procedure ");
	while (*q) ++q;

	while (*p)
		{
		getToken (&p, q);
		if (*q == '}')
			break;
		while (*q) ++q;
		}

	*q = 0;

	return buffer;
}

void IscCallableStatement::getToken(const char **ptr, char *token)
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
		else if (c & QUOTE)
			{
			char quote = p [-1];
			while (*p && (*p != quote || q [-1] == '\\'))
				*q++ = *p++;
			if (*p)
				*q++ = *p++;
			}
		}

	*q = 0;
	*ptr = p;
}
