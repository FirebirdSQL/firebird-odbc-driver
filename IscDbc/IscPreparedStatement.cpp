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

#include <malloc.h>
#include "IscDbc.h"
#include "IscPreparedStatement.h"
#include "SQLError.h"
#include "IscResultSet.h"
#include "IscConnection.h"
#include "BinaryBlob.h"
#include "AsciiBlob.h"
#include "Value.h"
#include "IscStatementMetaData.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscPreparedStatement::IscPreparedStatement(IscConnection *connection) : IscStatement (connection)
{
	statementMetaDataIPD = NULL;
	statementMetaDataIRD = NULL;
//Added by RM 2002-06-04
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

Value* IscPreparedStatement::getParameter(int index)
{
	if (index < 0 || index >= parameters.count)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid parameter index %d", index);

	return parameters.values + index;
}

void IscPreparedStatement::setInt(int index, long value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setNull(int index, int type)
{
	getParameter (index - 1)->clear();
}

void IscPreparedStatement::setDate(int index, DateTime value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setDouble(int index, double value)
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

void IscPreparedStatement::convStringData(int index)
{
	getParameter (--index)->convertStringData ();
}

bool IscPreparedStatement::execute()
{
	int numberParameters = inputSqlda.getColumnCount();

	for (int n = 0; n < numberParameters; ++n)
		inputSqlda.setValue (n, parameters.values + n, connection);

	return IscStatement::execute();
}

int IscPreparedStatement::executeUpdate()
{
	connection->startTransaction();
	NOT_YET_IMPLEMENTED;

	return 0;
}

void IscPreparedStatement::setBytes(int index, int length, const void* bytes)
{
	BinaryBlob *blob = new BinaryBlob();
	getParameter (index - 1)->setValue (blob);
	blob->putSegment (length, (char*) bytes, true);
}

//Added by RM 2002-06-04
void IscPreparedStatement::beginBlobDataTransfer(int index)
{
    if (segmentBlob)
        endBlobDataTransfer();

    segmentBlob = new BinaryBlob();
	getParameter (index - 1)->setValue (segmentBlob);
}

//Added by RM 2002-06-04
void IscPreparedStatement::putBlobSegmentData(int length, const void* bytes)
{
	if (segmentBlob)
		segmentBlob->putSegment (length, (char*) bytes, true);
}

//Added by RM 2002-06-04
void IscPreparedStatement::endBlobDataTransfer()
{
	if (segmentBlob)
{
    segmentBlob->release();
    segmentBlob = NULL;
}
}

// Carlos G.A.
void IscPreparedStatement::beginClobDataTransfer(int index)
{
	if (segmentClob)
        endClobDataTransfer();

    segmentClob = new AsciiBlob();
	getParameter (index - 1)->setValue (segmentClob);
}

// Carlos G.A.
void IscPreparedStatement::putClobSegmentData(int length, const void* bytes)
{
	if( segmentClob )
		segmentClob->putSegment (length, (char*) bytes, true);
}

// Carlos G.A.
void IscPreparedStatement::endClobDataTransfer()
{
	if( segmentClob )
	{
		segmentClob->release();
		segmentClob = NULL;
	}
}

bool IscPreparedStatement::execute (const char *sqlString)
{
	return IscStatement::execute (sqlString);
}

ResultSet*	 IscPreparedStatement::executeQuery (const char *sqlString)
{
	return IscStatement::executeQuery (sqlString);
}

void IscPreparedStatement::clearResults()
{
	IscStatement::clearResults ();
}

int	IscPreparedStatement::getUpdateCount()
{
	return IscStatement::getUpdateCount ();
}

bool IscPreparedStatement::getMoreResults()
{
	return IscStatement::getMoreResults();
}

void IscPreparedStatement::setCursorName (const char *name)
{
	IscStatement::setCursorName (name);
}

ResultSet* IscPreparedStatement::getResultSet()
{
	return IscStatement::getResultSet ();
}

ResultList* IscPreparedStatement::search (const char *searchString)
{
	return IscStatement::search (searchString);
}

int	IscPreparedStatement::executeUpdate (const char *sqlString)
{
	return IscStatement::executeUpdate (sqlString);
}

void IscPreparedStatement::close()
{
	IscStatement::close ();
}

int IscPreparedStatement::release()
{
	return IscStatement::release ();
}

void IscPreparedStatement::addRef()
{
	IscStatement::addRef ();
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
	GDS->_dsql_describe_bind (statusVector, &statementHandle, dialect, inputSqlda);

	if (statusVector [1])
		THROW_ISC_EXCEPTION (statusVector);

	if (inputSqlda.checkOverflow())
		{
		GDS->_dsql_describe_bind (statusVector, &statementHandle, dialect, inputSqlda);
		if (statusVector [1])
			THROW_ISC_EXCEPTION (statusVector);
		}

	parameters.alloc (inputSqlda.getColumnCount());
	inputSqlda.allocBuffer();
	
}

int IscPreparedStatement::getNumParams()
{
	return parameters.count;
}


StatementMetaData* IscPreparedStatement::getStatementMetaDataIPD()
{
	if (statementMetaDataIPD)
		return statementMetaDataIPD;

	statementMetaDataIPD = new IscStatementMetaData (&inputSqlda);

	return statementMetaDataIPD;
}

StatementMetaData* IscPreparedStatement::getStatementMetaDataIRD()
{
	if (statementMetaDataIRD)
		return statementMetaDataIRD;

	statementMetaDataIRD = new IscStatementMetaData (&outputSqlda);

	return statementMetaDataIRD;
}

void IscPreparedStatement::setByte(int index, char value)
{
	getParameter (index - 1)->setValue ((short) value);
}

void IscPreparedStatement::setLong(int index, QUAD value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setFloat(int index, float value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setTime(int index, SqlTime value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setTimestamp(int index, TimeStamp value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setShort(int index, short value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setBlob(int index, Blob * value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setArray(int index, Blob * value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setClob(int index, Clob * value)
{
	getParameter (index - 1)->setValue (value);
}

int IscPreparedStatement::objectVersion()
{
	return PREPAREDSTATEMENT_VERSION;
}
