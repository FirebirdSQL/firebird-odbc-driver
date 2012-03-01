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
 *  2002-10-11	IscCallableStatement.cpp
 *				Contributed by C. G. Alvarez
 *				Implemented new Blob and Clob support
 *
 *	2002-06-17	Submitted by C. G. Alvarez
 *				Overloaded SetString with a length parameter.
 *
 *	2002-06-04	IscCallableStatement.cpp
 *				Contributed by Robert Milharcic
 *				o Added beginDataTransfer(), putSegmentData()
 *				  and endDataTransfer().
 *
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

namespace IscDbcLibrary {

char charTable [256] = {0};
static int init();
static int foo = init();

int init ()
{
	int n;
	const char *p;

	for (p = " ;\t\r\n"; *p; ++p)
		charTable [*p] = WHITE;

	for (p = "?=(),{}"; *p; ++p)
		charTable [*p] = PUNCT;

	for (n = 'a'; n <= 'z'; ++n)
		charTable [n] = LETTER | IDENT;

	for (n = 'A'; n <= 'Z'; ++n)
		charTable [n] = LETTER | IDENT;

	for (n = '0'; n <= '9'; ++n)
		charTable [n] = DIGIT | IDENT;

	charTable ['\''] = QUOTE;
	charTable ['"'] = QUOTE;
	charTable ['_'] = IDENT;
	charTable ['$'] = IDENT;

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

bool IscCallableStatement::execute()
{
	ISC_STATUS statusVector [20];
	values.alloc (numberColumns);
	int numberParameters = inputSqlda.getColumnCount();
	isc_tr_handle transHandle = startTransaction();
	int n;

	for (n = 0; n < numberParameters; ++n)
		inputSqlda.setValue (n, parameters.values + n, this);

	int dialect = connection->getDatabaseDialect();

	if (connection->GDS->_dsql_execute2 (statusVector, &transHandle, &statementHandle,
						  dialect, inputSqlda, outputSqlda))
		THROW_ISC_EXCEPTION (connection, statusVector);

	resultsCount	= 1;
	resultsSequence = 0;
	getUpdateCounts();

	XSQLVAR *var = outputSqlda.sqlda->sqlvar;
    Value *value = values.values;

	for (n = 0; n < numberColumns; ++n, ++var, ++value)
		setValue (value, var);

	return outputSqlda.sqlda->sqld > 0;
}

int IscCallableStatement::objectVersion()
{
	return CALLABLESTATEMENT_VERSION;
}

bool IscCallableStatement::getBoolean(int id)
{
	return getValue (id)->getBoolean();
}

short IscCallableStatement::getShort(int id)
{
	return getValue (id)->getShort();
}

char IscCallableStatement::getByte(int id)
{
	return getValue (id)->getByte();
}

int IscCallableStatement::getInt(int id)
{
	return getValue (id)->getLong();
}

QUAD IscCallableStatement::getLong(int id)
{
	return getValue (id)->getQuad();
}

float IscCallableStatement::getFloat(int id)
{
	return getValue (id)->getFloat();
}

double IscCallableStatement::getDouble(int id)
{
	return getValue (id)->getDouble();
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

	IscPreparedStatement::prepare (sql);
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

}; // end namespace IscDbcLibrary
