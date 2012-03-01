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
#include "IscDbc.h"
#include "IscPreparedStatement.h"
#include "SQLError.h"
#include "IscResultSet.h"
#include "IscConnection.h"
#include "BinaryBlob.h"
#include "Value.h"
#include "IscStatementMetaData.h"

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
	if (outputSqlda.sqlda->sqld < 1)
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
	if (outputSqlda.sqlda->sqld < 1)
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
	ISC_STATUS statusVector [20];

	int dialect = connection->getDatabaseDialect ();
	connection->GDS->_dsql_describe_bind (statusVector, &statementHandle, dialect, inputSqlda);

	if (statusVector [1])
		THROW_ISC_EXCEPTION (connection, statusVector);

	if (inputSqlda.checkOverflow())
	{
		connection->GDS->_dsql_describe_bind (statusVector, &statementHandle, dialect, inputSqlda);
		if (statusVector [1])
			THROW_ISC_EXCEPTION (connection, statusVector);
	}

	parameters.alloc (inputSqlda.getColumnCount());
	inputSqlda.allocBuffer ( this );
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

}; // end namespace IscDbcLibrary
