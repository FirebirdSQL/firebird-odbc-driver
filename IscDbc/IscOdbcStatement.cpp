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
 *  The Original Code was created by Vladimir Tsvigun for IBPhoenix.
 *
 *  Copyright (c) 2003 Vladimir Tsvigun
 *  All Rights Reserved.
 */

//  
// IscOdbcStatement.cpp: interface for the IscOdbcStatement class.
//
//////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include "IscDbc.h"
#include "IscOdbcStatement.h"
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

IscOdbcStatement::IscOdbcStatement(IscConnection *connection) : IscStatement (connection)
{
	statementMetaDataIPD = NULL;
	statementMetaDataIRD = NULL;
}

IscOdbcStatement::~IscOdbcStatement()
{
	delete statementMetaDataIPD;
	delete statementMetaDataIRD;
}

ResultSet* IscOdbcStatement::executeQuery()
{
	if (outputSqlda.sqlda->sqld < 1)
		throw SQLEXCEPTION (RUNTIME_ERROR, "statement is not a Select");

	IscStatement::execute();
	getMoreResults();

	return getResultSet();
}

void IscOdbcStatement::executeMetaDataQuery()
{
	if (outputSqlda.sqlda->sqld < 1)
		throw SQLEXCEPTION (RUNTIME_ERROR, "statement is not a Select");

	IscStatement::execute();
	getMoreResults();
}

void IscOdbcStatement::drop()
{
	IscStatement::close ();
	resultsCount = 0;
	resultsSequence	= 0;
	freeStatementHandle();
	inputSqlda.clearSqlda();
	outputSqlda.clearSqlda();
	numberColumns = 0;
}

void IscOdbcStatement::prepareStatement(const char * sqlString)
{
	IscStatement::prepareStatement (sqlString);
	getInputParameters();
}

void IscOdbcStatement::getInputParameters()
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

	inputSqlda.allocBuffer(connection);
}

int IscOdbcStatement::getNumParams()
{
	if ( isActiveProcedure() )
		return inputSqlda.getColumnCount() + outputSqlda.getColumnCount();

	return inputSqlda.getColumnCount();
}

StatementMetaData* IscOdbcStatement::getStatementMetaDataIPD()
{
	if (statementMetaDataIPD)
		return statementMetaDataIPD;

	statementMetaDataIPD = new IscStatementMetaData (connection, &inputSqlda);

	return statementMetaDataIPD;
}

StatementMetaData* IscOdbcStatement::getStatementMetaDataIRD()
{
	if (statementMetaDataIRD)
		return statementMetaDataIRD;

	statementMetaDataIRD = new IscStatementMetaData (connection, &outputSqlda);

	return statementMetaDataIRD;
}

int IscOdbcStatement::objectVersion()
{
	return PREPAREDSTATEMENT_VERSION;
}

}; // end namespace IscDbcLibrary
