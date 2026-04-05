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
 *  The Original Code was created by James A. Starkey
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 *
 *	2002-10-11	IscPreparedStatement.cpp
 *	            Contributed by C. G. Alvarez
 *              Added/modified Blob/Clob support
 *
 *	2002-06-17	Submitted by C. G. Alvarez
 *				Overloaded SetString with a length parameter.
 *
 *	2002-06-04	IscPreparedStatement.cpp
 *				Contributed by Robert Milharcic
 *				o Added beginDataTransfer(), putSegmentData()
 *				  and endDataTransfer().
 *
 *
 */

// IscPreparedStatement.cpp: implementation of the IscPreparedStatement class.
//
//////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>
#include "IscDbc.h"
#include "IscPreparedStatement.h"
#include "SQLError.h"
#include "IscResultSet.h"
#include "IscConnection.h"
#include "BinaryBlob.h"
#include "Value.h"
#include "IscStatementMetaData.h"

using namespace Firebird;

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscPreparedStatement::IscPreparedStatement(IscConnection *connection) : IscStatement (connection)
{
	statementMetaDataIPD = NULL;
	statementMetaDataIRD = NULL;
    segmentBlob = NULL;
	segmentClob = NULL;
}

IscPreparedStatement::~IscPreparedStatement()
{
	if (statementMetaDataIPD)
		delete statementMetaDataIPD;
	if (statementMetaDataIRD)
		delete statementMetaDataIRD;
}

ResultSet* IscPreparedStatement::executeQuery()
{
	if (outputSqlda.columnsCount < 1)
		throw SQLEXCEPTION (RUNTIME_ERROR, "statement is not a Select");

	execute();
	getMoreResults();

	return getResultSet();
}

int IscPreparedStatement::executeUpdate()
{
	NOT_YET_IMPLEMENTED;

	return 0;
}

void IscPreparedStatement::executeMetaDataQuery()
{
	if (outputSqlda.columnsCount < 1)
		throw SQLEXCEPTION (RUNTIME_ERROR, "statement is not a Select");

	execute();
	getMoreResults();
}

Value* IscPreparedStatement::getParameter(int index)
{
	if (index < 0 || index >= parameters.count)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid parameter index %d", index);

	return parameters.values + index;
}

void IscPreparedStatement::convStringData(int index)
{
	getParameter (--index)->convertStringData ();
}

void IscPreparedStatement::clearParameters()
{
	NOT_YET_IMPLEMENTED;
}

bool IscPreparedStatement::execute()
{
	int numberParameters = inputSqlda.getColumnCount();

	for (int n = 0; n < numberParameters; ++n)
		inputSqlda.setValue (n, parameters.values + n, this);

	return IscStatement::execute();
}

void IscPreparedStatement::beginBlobDataTransfer(int index)
{
    if (segmentBlob)
        endBlobDataTransfer();

    segmentBlob = new BinaryBlob();
	getParameter (index - 1)->setValue (segmentBlob);
}

void IscPreparedStatement::putBlobSegmentData(int length, const void* bytes)
{
	if (segmentBlob)
		segmentBlob->putSegment (length, (char*) bytes, true);
}

void IscPreparedStatement::endBlobDataTransfer()
{
	if (segmentBlob)
	{
		segmentBlob->release();
		segmentBlob = NULL;
	}
}

void IscPreparedStatement::prepare(const char * sqlString)
{
	prepareStatement (sqlString);
	getInputParameters();
}

void IscPreparedStatement::getInputParameters()
{
	// Phase 9.11: Removed commented-out legacy ISC _dsql_describe_bind code.
	// Input parameters are now obtained via IStatement::getInputMetadata() in Sqlda.
	parameters.alloc (inputSqlda.getColumnCount());
}

int IscPreparedStatement::getNumParams()
{
	if ( isActiveProcedure() )
		return parameters.count + outputSqlda.getColumnCount();
	return parameters.count;
}


StatementMetaData* IscPreparedStatement::getStatementMetaDataIPD()
{
	if (statementMetaDataIPD)
		return statementMetaDataIPD;

	statementMetaDataIPD = new IscStatementMetaData (this, &inputSqlda);

	return statementMetaDataIPD;
}

StatementMetaData* IscPreparedStatement::getStatementMetaDataIRD()
{
	if (statementMetaDataIRD)
		return statementMetaDataIRD;

	statementMetaDataIRD = new IscStatementMetaData (this, &outputSqlda);

	return statementMetaDataIRD;
}
/*
void IscPreparedStatement::setAsciiStream( int parameterIndex, InputStream x, int length )
{
	NOT_YET_IMPLEMENTED;
}

void IscPreparedStatement::setBigDecimal( int parameterIndex, BigDecimal x );
{
	NOT_YET_IMPLEMENTED;
}

void IscPreparedStatement::setBinaryStream( int parameterIndex, InputStream x, int length );
{
	NOT_YET_IMPLEMENTED;
}
*/
void IscPreparedStatement::setBoolean(int index, bool value)
{
	getParameter (index - 1)->setValue ((short) value);
}

void IscPreparedStatement::setByte(int index, char value)
{
	getParameter (index - 1)->setValue ((short) value);
}

void IscPreparedStatement::setBytes(int index, const void* bytes)
{
	NOT_YET_IMPLEMENTED;
}

void IscPreparedStatement::setBytes(int index, int length, const void* bytes)
{
	char *idx = (char*)bytes;
	BinaryBlob *blob = new BinaryBlob();
	getParameter (index - 1)->setValue (blob);
	blob->release();
	while (length >= DEFAULT_BLOB_BUFFER_LENGTH) 
	{
		blob->putSegment(DEFAULT_BLOB_BUFFER_LENGTH, idx, true);
		idx += DEFAULT_BLOB_BUFFER_LENGTH;
		length -= DEFAULT_BLOB_BUFFER_LENGTH;
	}
	if (length)	blob->putSegment(length, idx, true);
}

void IscPreparedStatement::setDate(int index, DateTime value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setDouble(int index, double value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setFloat(int index, float value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setInt(int index, int value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setLong(int index, QUAD value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setNull(int index, int type)
{
	getParameter (index - 1)->clear();
}
/*
void IscPreparedStatement::setObject( int parameterIndex, Object x )
{
	NOT_YET_IMPLEMENTED;
}
void IscPreparedStatement::setObject( int parameterIndex, Object x, int targetSqlType )
{
	NOT_YET_IMPLEMENTED;
}
void IscPreparedStatement::setObject( int parameterIndex, Object x, int targetSqlType, int scale )
{
	NOT_YET_IMPLEMENTED;
}
*/
void IscPreparedStatement::setShort(int index, short value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setString(int index, const char * string)
{
	getParameter (index - 1)->setString (string, true);
}

void IscPreparedStatement::setString(int index, const char * string, int length)
{
    getParameter (index - 1)->setString (length, string, true);
}

void IscPreparedStatement::setTime(int index, SqlTime value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setTimestamp(int index, TimeStamp value)
{
	getParameter (index - 1)->setValue (value);
}
/*
void IscPreparedStatement::setUnicodeStream( int parameterIndex, InputStream x, int length )
{
	NOT_YET_IMPLEMENTED;
}
*/
void IscPreparedStatement::setBlob(int index, Blob * value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setArray(int index, Blob * value)
{
	getParameter (index - 1)->setValue (value);
}

int IscPreparedStatement::objectVersion()
{
	return PREPAREDSTATEMENT_VERSION;
}

//////////////////////////////////////////////////////////////////////
// Phase 14.4.6a: CallableStatement implementation (merged from IscCallableStatement).
//////////////////////////////////////////////////////////////////////

// Character classification table for SQL rewriting (escape syntax).
// Global — also used by IscConnection.cpp, IscArray.cpp via extern.
char charTable[256] = {0};
static int initCharTable()
{
	const char* p;
	for (p = " ;\t\r\n"; *p; ++p) charTable[(unsigned char)*p] = WHITE;
	for (p = "?=(),{}"; *p; ++p) charTable[(unsigned char)*p] = PUNCT;
	for (int n = 'a'; n <= 'z'; ++n) charTable[n] = LETTER | IDENT;
	for (int n = 'A'; n <= 'Z'; ++n) charTable[n] = LETTER | IDENT;
	for (int n = '0'; n <= '9'; ++n) charTable[n] = DIGIT | IDENT;
	charTable[(unsigned char)'\''] = QUOTE;
	charTable[(unsigned char)'"'] = QUOTE;
	charTable[(unsigned char)'_'] = IDENT;
	charTable[(unsigned char)'$'] = IDENT;
	return 0;
}
static int charTableInit_ = initCharTable();

bool IscPreparedStatement::executeCallable()
{
	callableValues_.alloc(numberColumns);
	int numberParameters = inputSqlda.getColumnCount();
	ITransaction* transHandle = startTransaction();

	for (int n = 0; n < numberParameters; ++n)
		inputSqlda.setValue(n, parameters.values + n, this);

	ThrowStatusWrapper status(connection->GDS->_status);
	try
	{
		sqlda_check_and_rebuild(inputSqlda);
		auto* _imeta = inputSqlda.useExecBufferMeta ? inputSqlda.execMeta : inputSqlda.meta;
		auto* _ibufPtr = inputSqlda.useExecBufferMeta ? inputSqlda.execBuffer.data()
		                                              : inputSqlda.activeBufferData();

		statementHandle->execute(&status, transHandle, _imeta, _ibufPtr,
		                         outputSqlda.meta, outputSqlda.activeBufferData());
	}
	catch (const FbException& error)
	{
		THROW_ISC_EXCEPTION(connection, error.getStatus());
	}

	resultsCount = 1;
	resultsSequence = 0;
	getUpdateCounts();

	Value* value = callableValues_.values;
	for (int n = 0; n < numberColumns; ++n, ++value)
		setValue(value, n + 1, outputSqlda);

	return numberColumns > 0;
}

void IscPreparedStatement::prepareCall(const char* originalSql)
{
	char buffer[1024];
	const char* sql = rewriteSql(originalSql, buffer, sizeof(buffer));
	prepare(sql);
}

const char* IscPreparedStatement::rewriteSql(const char* originalSql, char* buffer, int length)
{
	const char* p = originalSql;
	char token[256];
	getToken(&p, token);

	if (token[0] != '{')
		return originalSql;

	getToken(&p, token);

	if (strcasecmp(token, "call") != 0)
		throw SQLEXCEPTION(SYNTAX_ERROR, "unsupported form of procedure call");

	char* q = buffer;
	strcpy(q, "execute procedure ");
	while (*q) ++q;

	while (*p)
	{
		getToken(&p, q);
		if (*q == '}')
			break;
		while (*q) ++q;
	}

	*q = 0;
	return buffer;
}

void IscPreparedStatement::getToken(const char** ptr, char* token)
{
	const char* p = *ptr;
	SKIP_WHITE(p);
	char* q = token;

	if (*p)
	{
		char c = charTable[(unsigned char)*p];
		*q++ = *p++;
		if (c & IDENT)
			while (charTable[(unsigned char)*p] & IDENT)
				*q++ = *p++;
		else if (c & QUOTE)
		{
			char quote = p[-1];
			while (*p && (*p != quote || q[-1] == '\\'))
				*q++ = *p++;
			if (*p)
				*q++ = *p++;
		}
	}

	*q = 0;
	*ptr = p;
}

bool IscPreparedStatement::getBoolean(int id)  { return getValue(id)->getBoolean(); }
short IscPreparedStatement::getShort(int id)    { return getValue(id)->getShort(); }
char IscPreparedStatement::getByte(int id)      { return getValue(id)->getByte(); }
int IscPreparedStatement::getInt(int id)        { return getValue(id)->getLong(); }
QUAD IscPreparedStatement::getLong(int id)      { return getValue(id)->getQuad(); }
float IscPreparedStatement::getFloat(int id)    { return getValue(id)->getFloat(); }
double IscPreparedStatement::getDouble(int id)  { return getValue(id)->getDouble(); }
Blob* IscPreparedStatement::getBlob(int id)     { return getValue(id)->getBlob(); }
SqlTime IscPreparedStatement::getTime(int id)   { return getValue(id)->getTime(); }
DateTime IscPreparedStatement::getDate(int id)  { return getValue(id)->getDate(); }
TimeStamp IscPreparedStatement::getTimestamp(int id) { return getValue(id)->getTimestamp(); }
const char* IscPreparedStatement::getString(int id)  { return getValue(id)->getString(); }

bool IscPreparedStatement::wasNull()
{
	return valueWasNull_;
}

Value* IscPreparedStatement::getValue(int index)
{
	if (index < minOutputVariable_ || index >= minOutputVariable_ + numberColumns)
		throw SQLEXCEPTION(RUNTIME_ERROR, "invalid column index for procedure call");

	Value* value = callableValues_.values + index - minOutputVariable_;
	valueWasNull_ = value->type == Null;
	return value;
}

void IscPreparedStatement::registerOutParameter(int parameterIndex, int sqlType)
{
	minOutputVariable_ = (minOutputVariable_ == 0) ? parameterIndex : MIN(minOutputVariable_, parameterIndex);
}

void IscPreparedStatement::registerOutParameter(int parameterIndex, int sqlType, int scale)
{
	minOutputVariable_ = (minOutputVariable_ == 0) ? parameterIndex : MIN(minOutputVariable_, parameterIndex);
}

}; // end namespace IscDbcLibrary
