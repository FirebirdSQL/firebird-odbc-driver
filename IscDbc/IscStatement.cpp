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
 */

// IscStatement.cpp: implementation of the IscStatement class.
//
//////////////////////////////////////////////////////////////////////

#include <malloc.h>
#include <time.h>
#include "IscDbc.h"
#include "IscStatement.h"
#include "IscResultSet.h"
#include "IscConnection.h"
#include "Attachment.h"
#include "IscBlob.h"
#include "SQLError.h"
#include "Value.h"

static char requestInfo [] = { isc_info_sql_records,
							   isc_info_sql_stmt_type,
							   isc_info_end };

static int init();
static struct tm baseTm = { 0, 0, 0, 1, 0, 70 };
static long baseDate = init();

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

int init()
{
	ISC_QUAD baseDate;
	isc_encode_date (&baseTm, &baseDate);

	return baseDate.gds_quad_high;
}

IscStatement::IscStatement(IscConnection *connect)
{
	connection = connect;
	useCount = 1;
	numberColumns = 0;
	statementHandle = NULL;
	//transactionHandle = NULL;
	updateCount = insertCount = deleteCount = 0;
	selectActive = false;
}

IscStatement::~IscStatement()
{
	FOR_OBJECTS (IscResultSet*, resultSet, &resultSets)
		resultSet->close();
	END_FOR;

	if (connection)
		{
		connection->deleteStatement (this);
		connection = NULL;
		}

	try
		{
		if (statementHandle)
			freeStatementHandle();
		/***
		if (transactionHandle)
			commitAuto();
		***/
		}
	catch (...)
		{
		}
}

IscResultSet* IscStatement::createResultSet()
{
	IscResultSet *resultSet = new IscResultSet (this);
	resultSets.append (resultSet);

	return resultSet;
}

void IscStatement::close()
{
	FOR_OBJECTS (IscResultSet*, resultSet, &resultSets)
		resultSet->close();
	END_FOR;

	if (selectActive)
		{
		selectActive = false;
		if (connection->autoCommit)
			connection->commitAuto();
		}
}

bool IscStatement::execute(const char * sqlString)
{
	NOT_YET_IMPLEMENTED;

	return false;
}

int IscStatement::executeUpdate(const char * sqlString)
{
	NOT_YET_IMPLEMENTED;

	return 0;
}

ResultList* IscStatement::search(const char * searchString)
{
	NOT_YET_IMPLEMENTED;

	return NULL;
}

ResultSet* IscStatement::getResultSet()
{
	if (!statementHandle)
		throw SQLEXCEPTION (RUNTIME_ERROR, "no active statement");

	if (outputSqlda.sqlda->sqld < 1)
		throw SQLEXCEPTION (RUNTIME_ERROR, "current statement doesn't return results");
	
	return createResultSet();
}

ResultSet* IscStatement::executeQuery(const char * sqlString)
{
	NOT_YET_IMPLEMENTED;

	return NULL;
}

void IscStatement::setCursorName(const char * name)
{
	ISC_STATUS statusVector [20];
	isc_dsql_set_cursor_name (statusVector, &statementHandle, (char*) name, 0);

	if (statusVector [1])
		THROW_ISC_EXCEPTION (statusVector);
}

void IscStatement::addRef()
{
	++useCount;
}

int IscStatement::release()
{
	if (--useCount == 0)
		{
		delete this;
		return 0;
		}

	return useCount;
}

bool IscStatement::getMoreResults()
{
	if (resultsSequence >= resultsCount)
		return false;

	++resultsSequence;

	if (outputSqlda.sqlda->sqld > 0)
		return true;

	return false;
}

int IscStatement::getUpdateCount()
{
	if (outputSqlda.sqlda->sqld > 0)
		return -1;

	return summaryUpdateCount;
}

void IscStatement::deleteResultSet(IscResultSet * resultSet)
{
	resultSets.deleteItem (resultSet);

	if (resultSets.isEmpty())
		{
		selectActive = false;
		connection->commitAuto();
		}
}

void IscStatement::prepareStatement(const char * sqlString)
{
	clearResults();
	sql = sqlString;

	// Make sure we have a transaction started.  Allocate a statement.

	void *transHandle = connection->startTransaction();
	ISC_STATUS statusVector [20];
	isc_dsql_allocate_statement (statusVector, &connection->databaseHandle, &statementHandle);

	if (statusVector [1])
		THROW_ISC_EXCEPTION (statusVector);

	// Prepare dynamic SQL statement.  Make first attempt to get parameters

	int dialect = connection->getDatabaseDialect();
	isc_dsql_prepare (statusVector, &transHandle, &statementHandle,
					  0, (char*) sqlString, dialect, outputSqlda);

	if (statusVector [1])
		THROW_ISC_EXCEPTION (statusVector);

	// If we didn't allocate a large enough SQLDA, try again.

	if (outputSqlda.checkOverflow())
		{
		isc_dsql_describe (statusVector, &statementHandle, dialect, outputSqlda);
		if (statusVector [1])
			THROW_ISC_EXCEPTION (statusVector);
		}

	numberColumns = outputSqlda.getColumnCount();
	XSQLVAR *var = outputSqlda.sqlda->sqlvar;
	insertCount = deleteCount = updateCount = 0;
}

bool IscStatement::execute()
{
	if (selectActive && connection->autoCommit && resultSets.isEmpty())
		clearSelect();

	// Make sure there is a transaction

	ISC_STATUS statusVector [20];
	void *transHandle = connection->startTransaction();

	int dialect = connection->getDatabaseDialect ();
	if (isc_dsql_execute (statusVector, &transHandle, &statementHandle, 
			dialect, inputSqlda))
		{
		clearSelect();
		if (connection->autoCommit)
			connection->rollbackAuto();
		THROW_ISC_EXCEPTION (statusVector);
		}

	resultsCount = 1;
	resultsSequence = 0;
	int statementType = getUpdateCounts();

	switch (statementType)
		{
		case isc_info_sql_stmt_ddl:
			clearSelect();
			connection->commit();
			freeStatementHandle();
			break;

		case isc_info_sql_stmt_select:
		case isc_info_sql_stmt_select_for_upd:
			selectActive = true;
			break;

		case isc_info_sql_stmt_insert:
		case isc_info_sql_stmt_update:
		case isc_info_sql_stmt_delete:
			if (connection->autoCommit)
				connection->commitAuto();
			break;
		}

	return outputSqlda.sqlda->sqld > 0;
}

void IscStatement::clearResults()
{

}

int IscStatement::objectVersion()
{
	return STATEMENT_VERSION;
}

int IscStatement::getUpdateCounts()
{
	char buffer [128];
	ISC_STATUS	statusVector [20];
	isc_dsql_sql_info (statusVector, &statementHandle, 
					   sizeof (requestInfo), requestInfo,
					   sizeof (buffer), buffer);

	int statementType = 0;

	for (char *p = buffer; *p != isc_info_end;)
		{
		char item = *p++;
		int length = isc_vax_integer (p, 2);
		p += 2;
		switch (item)
			{
			case isc_info_sql_records:
				{
				int n;
				for (char *q = p; *q != isc_info_end;)
					{
					char item = *q++;
					int l = isc_vax_integer (q, 2);
					q += 2;
					switch (item)
						{
						case isc_info_req_insert_count:
							n = isc_vax_integer (q, l);
							insertDelta = n - insertCount;
							insertCount = n;
							break;

						case isc_info_req_delete_count:
							n = isc_vax_integer (q, l);
							deleteDelta = n - deleteCount;
							deleteCount = n;
							break;

						case isc_info_req_update_count:
							n = isc_vax_integer (q, l);
							updateDelta = n - updateCount;
							updateCount = n;
							break;
						}
					q += l;
					}
				}
				break;

			case isc_info_sql_stmt_type:
				statementType = isc_vax_integer (p, length);
				break;
			}
		p += length;
		}

	summaryUpdateCount = MAX (insertDelta, deleteDelta);
	summaryUpdateCount = MAX (summaryUpdateCount, updateDelta);

	return statementType;
}

void IscStatement::setValue(Value *value, XSQLVAR *var)
{
	if ((var->sqltype & 1) && *var->sqlind == -1)
		value->setNull();
	else
		switch (var->sqltype & ~1)
			{
			case SQL_TEXT:
				{
				char *data = (char*) var->sqldata;
				//printf ("%d '%s'\n", n, data);
				data [var->sqllen - 1] = 0;
				value->setString (data, false);
				}
				break;

			case SQL_VARYING:
				{
				int length = *((short*) var->sqldata);
				char *data = var->sqldata + 2;
				if (length < var->sqllen)
					{
					data [length] = 0;
					value->setString (data, false);
					}
				else
					value->setString (length, data, false);
				//printf ("%d '%*s'\n", n, length, data);
				}
				break;

			case SQL_SHORT:
				value->setValue (*(short*) var->sqldata, var->sqlscale);
				break;

			case SQL_LONG:
				value->setValue (*(long*) var->sqldata, var->sqlscale);
				break;

			case SQL_FLOAT:
				value->setValue (*(float*) var->sqldata);
				break;

			case SQL_DOUBLE:
				value->setValue (*(double*) var->sqldata);
				break;

			case SQL_QUAD:
			case SQL_INT64:
				value->setValue (*(QUAD*) var->sqldata, var->sqlscale);
				break;

			case SQL_BLOB:
				value->setValue (new IscBlob (this, (ISC_QUAD*) var->sqldata));
				break;

			case SQL_TIMESTAMP:
				{
				ISC_TIMESTAMP *date = (ISC_TIMESTAMP*) var->sqldata;
				long days = date->timestamp_date - baseDate;

                                if ((days > 24855) || (days < -24885))
                                    throw SQLEXCEPTION (CONVERSION_ERROR, "date out of range");

				TimeStamp timestamp;
				timestamp = (long) ((days * 24 * 60 * 60) + date->timestamp_time / 10000);
				timestamp.nanos = (date->timestamp_time / 10000) * 100;
				value->setValue (timestamp);
				}
				break;

			case SQL_TYPE_DATE:
				{
				ISC_DATE date = *(ISC_DATE*) var->sqldata;
				long days = date - baseDate;
				DateTime dateTime;
				dateTime = (long) (days * 24 * 60 * 60);
				value->setValue (dateTime);
				}
				break;

			case SQL_TYPE_TIME:
				{
				ISC_TIME data = *(ISC_TIME*) var->sqldata;
				Time time;
				time = data / ISC_TIME_SECONDS_PRECISION;
				value->setValue (time);
				}
				break;

			case SQL_D_FLOAT:
				NOT_SUPPORTED("d_float", var->relname_length, var->relname, var->aliasname_length, var->aliasname);
				break;

			case SQL_ARRAY:
				NOT_SUPPORTED("array", var->relname_length, var->relname, var->aliasname_length, var->aliasname);
				break;
			}
}

ISC_DATE IscStatement::getIscDate(DateTime value)
{
	return value.date / (24 * 60 * 60) + baseDate;
}

ISC_TIMESTAMP IscStatement::getIscTimeStamp(TimeStamp value)
{
	ISC_TIMESTAMP date;
	date.timestamp_date = value.date / (24 * 60 * 60) + baseDate;
	date.timestamp_time = value.date % (24 * 60 * 60) + value.nanos / 100;

	return date;
}

ISC_TIME IscStatement::getIscTime(Time value)
{
	return value.timeValue * ISC_TIME_SECONDS_PRECISION;
}

/***
void IscStatement::commitAuto()
{
	if (transactionHandle)
		{
		ISC_STATUS statusVector [20];
		isc_commit_transaction (statusVector, &transactionHandle);
		if (statusVector [1])
			THROW_ISC_EXCEPTION (statusVector);
		}
}

void* IscStatement::getTransaction()
{
	ISC_STATUS statusVector [20];

	if (!connection->autoCommit)
		return connection->startTransaction();

	if (transactionHandle)
		return transactionHandle;

	isc_start_transaction (statusVector, &transactionHandle, 1, &connection->attachment->databaseHandle, 0, NULL);

	if (statusVector [1])
		THROW_ISC_EXCEPTION (statusVector);

	return transactionHandle;
}

void IscStatement::rollbackAuto()
{
	if (transactionHandle)
		{
		ISC_STATUS statusVector [20];
		isc_rollback_transaction (statusVector, &transactionHandle);
		if (statusVector [1])
			THROW_ISC_EXCEPTION (statusVector);
		}
}
***/

void IscStatement::clearSelect()
{
	if (selectActive)
		{
		selectActive = false;
		connection->commitAuto();
		freeStatementHandle();
		}
}

void IscStatement::freeStatementHandle()
{
	ISC_STATUS statusVector [20];
	isc_dsql_free_statement (statusVector, &statementHandle, DSQL_drop);
}
