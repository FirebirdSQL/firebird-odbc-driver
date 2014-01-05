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
 *
 *	2003-03-24	IscStatement.cpp
 *				Contributed by Norbert Meyer
 *				use value->setString (length, data, true); if not, 
 *				the String is not nullterminated, but used as
 *				nullterminated String in ODBCStatement::setValue()
 *				(case SQL_C_CHAR: ...). You can also check the 
 *				length in ODBCStatement::setValue, but there is no
 *				function getStringLength...
 *
 *	2003-03-24	IscStatement.cpp
 *				Contributed by Carlos Guzman Alvarez
 *				Remove updatePreparedResultSet from OdbStatement 
 *				and achieve the same goal in another way.
 *
 *	2003-03-24	IscStatement.cpp
 *				Contributed by Vladimir Tcvigyn
 *				Fix for timestamp bug (line 497)
 *
 *	2003-03-24	IscStatement.cpp
 *				Contributed by Vladimir Tcvigyn
 *				Fix for timestamp bug (line 497)
 *
 *	2002-08-12	IscStatement.cpp
 *				Contributed by Roger Gammans
 *				Close the cursor when releasing a result set.
 *				
 *	2002-08-12	IscStatement.cpp
 *				Contributed by C. G. Alvarez
 *				Added more graceful detection of statements that do
 *				not return a result set.	
 *	
 *	2002-06-04	IscStatement.cpp
 *				Amended setValue() again. (RM)
 *				Hopefully this means that we finally 
 *				have got SQL_CHAR and SQL_VARYING right.
 *
 *	2002-05-20	Added suggestion from Bernhard Schulte
 *				o	IscStatement::setValue() amended to 
 *					fix problem with trailing blanks.
 *
 *				o	Update setValue() to support changes to the 
 *					OdbcStatement datetime routines.
 *				
 *				o	Update getIscTimeStamp() to support changes to the 
 *					OdbcStatement datetime routines.
 *
 *	2002-04-30	Added suggestions from LiWeimin
 *				o	IscStatement::setValue
 *					When writing a varchar decrement the sqllen by 2 
 *					before the test.
 *
 *				o	IscStatement::getIscDate
 *					Don't modify the date returned with this expression
 *						/ (24 * 60 * 60) + baseDate
 *					just return the date.
 */

// IscStatement.cpp: implementation of the IscStatement class.
//
//////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <time.h>
#include "IscDbc.h"
#include "ListParamTransaction.h"
#include "IscStatement.h"
#include "IscResultSet.h"
#include "IscConnection.h"
#include "Attachment.h"
#include "IscBlob.h"
#include "IscArray.h"

#include "SQLError.h"
#include "Value.h"

static char requestInfo [] = { isc_info_sql_records,
							   isc_info_sql_stmt_type,
							   isc_info_end };

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscStatement::IscStatement(IscConnection *connect)
{
	connection = connect;
	useCount = 1;
	numberColumns = 0;
	statementHandle = NULL;
	transactionLocal = false;
	transactionStatusChange = false;
	transactionStatusChangingToLocal = false;

	openCursor = false;
	typeStmt = stmtNone;
	resultsCount = 0;
	resultsSequence	= 0;
}

IscStatement::~IscStatement()
{
	FOR_OBJECTS (IscResultSet*, resultSet, &resultSets)
		resultSet->close();
	END_FOR;

	try
	{
		if (statementHandle)
			freeStatementHandle();

		if (connection)
		{
			connection->deleteStatement (this);
			connection = NULL;
		}
	}
	catch (...)
	{
	}
}

void IscStatement::rollbackLocal()
{
	InfoTransaction	&tr = transactionInfo;

	if ( tr.transactionHandle )
	{
		ISC_STATUS statusVector[20];
		connection->GDS->_rollback_transaction( statusVector, &tr.transactionHandle );

		if ( statusVector[1] )
			THROW_ISC_EXCEPTION( connection, statusVector );
	}
	tr.transactionPending = false;
}

void IscStatement::commitLocal()
{
	InfoTransaction	&tr = transactionInfo;

	if ( tr.transactionHandle )
	{
		ISC_STATUS statusVector[20];
		connection->GDS->_commit_transaction( statusVector, &tr.transactionHandle );

		if ( !tr.nodeParamTransaction && transactionLocal )
			transactionLocal = false;

		if ( statusVector[1] )
		{
			rollbackLocal();
			THROW_ISC_EXCEPTION( connection, statusVector );
		}
	}
	tr.transactionPending = false;
}

void IscStatement::setReadOnlyTransaction()
{
	transactionLocal = true;

	transactionInfo.transactionHandle = NULL;
	transactionInfo.transactionIsolation = 0x00000002L; // SQL_TXN_READ_COMMITTED
	transactionInfo.transactionPending = false;
	transactionInfo.autoCommit = true;
	transactionInfo.transactionExtInit = TRA_ro | TRA_nw;
}

void IscStatement::setActiveLocalParamTransaction()
{
	transactionStatusChange = true;
	transactionStatusChangingToLocal = true;

	if ( connection->tmpParamTransaction )
	{
		CNodeParamTransaction &tmp = *connection->tmpParamTransaction;

		if ( tmp.tpbBuffer && tmp.lengthTpbBuffer )
		{
			if ( !transactionInfo.nodeParamTransaction )
				transactionInfo.nodeParamTransaction = new CNodeParamTransaction;

			*transactionInfo.nodeParamTransaction = tmp;
		}

		delete connection->tmpParamTransaction;
		connection->tmpParamTransaction = NULL;
	}
	else
	{
		transactionInfo.setParam( connection->transactionInfo );
	}
}

void IscStatement::delActiveLocalParamTransaction()
{
	transactionStatusChange = true;
	transactionStatusChangingToLocal = false;

	if ( transactionInfo.nodeParamTransaction )
	{
		delete transactionInfo.nodeParamTransaction;
		transactionInfo.nodeParamTransaction = NULL;
	}

	transactionInfo.setParam( connection->transactionInfo );
}

void IscStatement::declareLocalParamTransaction()
{
	transactionStatusChange = false;
	transactionStatusChangingToLocal = false;

	if ( connection->tmpParamTransaction )
	{
		CNodeParamTransaction &tmp = *connection->tmpParamTransaction;

		if ( tmp.tpbBuffer && tmp.lengthTpbBuffer )
		{
			if ( !transactionInfo.nodeParamTransaction )
				transactionInfo.nodeParamTransaction = new CNodeParamTransaction;

			*transactionInfo.nodeParamTransaction = tmp;
		}

		delete connection->tmpParamTransaction;
		connection->tmpParamTransaction = NULL;
	}
}

void IscStatement::switchTransaction( bool local )
{
	transactionStatusChange = true;

	if ( local && transactionInfo.nodeParamTransaction )
		transactionStatusChangingToLocal = true;
	else
		transactionStatusChangingToLocal = false;
}

isc_tr_handle IscStatement::startTransaction()
{
	if ( connection->shareConnected )
		return connection->startTransaction();

	InfoTransaction	*tr = transactionLocal ? &transactionInfo : &connection->transactionInfo;

	if ( !statementHandle && transactionStatusChange )
	{
		if ( transactionStatusChangingToLocal )
		{
			tr = &transactionInfo;
			transactionLocal = true;
		}
		else
		{
			tr = &connection->transactionInfo;
			transactionLocal = false;
		}

		transactionStatusChange = false;
	}

    if ( tr->transactionHandle )
        return tr->transactionHandle;

	if ( transactionStatusChange )
	{
		if ( transactionStatusChangingToLocal )
		{
			tr = &transactionInfo;
			transactionLocal = true;
		}
		else
		{
			tr = &connection->transactionInfo;
			transactionLocal = false;
		}

		transactionStatusChange = false;

		if ( tr->transactionHandle )
			return tr->transactionHandle;
	}

    ISC_STATUS statusVector[20];
    char    iscTpb[9], *tpbBuffer;
	int		count;

	if ( !tr->nodeParamTransaction )
	{
		tpbBuffer = iscTpb;
		count = sizeof( iscTpb ) - 4; // 4 it's size block for Lock Timeout Wait Transactions

		iscTpb[0] = isc_tpb_version3;
		iscTpb[1] = tr->transactionExtInit & TRA_ro ? isc_tpb_read : isc_tpb_write;
		iscTpb[2] = tr->transactionExtInit & TRA_nw ? isc_tpb_nowait : isc_tpb_wait;

		/* Isolation level */
		switch( tr->transactionIsolation )
		{
		case 0x00000008L:
			// SQL_TXN_SERIALIZABLE:
			iscTpb[3] = isc_tpb_consistency;
			count = 4;
			break;

		case 0x00000004L:
			// SQL_TXN_REPEATABLE_READ:
			iscTpb[3] = isc_tpb_concurrency;
			count = 4;
			break;

		case 0x00000001L:
			// SQL_TXN_READ_UNCOMMITTED:
			iscTpb[3] = isc_tpb_read_committed;
			iscTpb[4] = isc_tpb_no_rec_version;
			break;

		case 0x00000002L:
		default:
			// SQL_TXN_READ_COMMITTED:
			iscTpb[3] = isc_tpb_read_committed;
			iscTpb[4] = isc_tpb_rec_version;
			break;
		}

		if ( !(tr->transactionExtInit & TRA_nw) 
			&& connection->attachment->isFirebirdVer2_0()
			&& connection->attachment->getUseLockTimeoutWaitTransactions() )
		{
			char *pt = &iscTpb[count];

			*pt++ = isc_tpb_lock_timeout;
			*pt++ = sizeof ( short );
			*pt++ = (char)connection->attachment->getUseLockTimeoutWaitTransactions();
			*pt++ = (char)(connection->attachment->getUseLockTimeoutWaitTransactions() >> 8);

			count += 4;
		}
	}
	else
	{
		tpbBuffer = tr->nodeParamTransaction->tpbBuffer;
		count = tr->nodeParamTransaction->lengthTpbBuffer;
		tr->autoCommit = tr->nodeParamTransaction->autoCommit;
	}

    connection->GDS->_start_transaction( statusVector, &tr->transactionHandle, 1, &connection->attachment->databaseHandle,
            count, tpbBuffer );

    if ( statusVector[1] )
		THROW_ISC_EXCEPTION( connection, statusVector );

	if ( !tr->autoCommit )
		tr->transactionPending = true;

    return tr->transactionHandle;
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

	if ( isActiveSelect() )
	{
		openCursor = false;
		if ( transactionLocal )
		{
			if ( transactionInfo.autoCommit )
				commitLocal();
		}
		else if ( connection->transactionInfo.autoCommit )
			connection->commitAuto();
	}
}

void IscStatement::setMaxFieldSize(int max)
{
	NOT_YET_IMPLEMENTED;
}

void IscStatement::setMaxRows(int max)
{
	NOT_YET_IMPLEMENTED;
}

void IscStatement::setQueryTimeout(int seconds)
{
	NOT_YET_IMPLEMENTED;
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

int	IscStatement::getMaxFieldSize()
{
	NOT_YET_IMPLEMENTED;

	return 0;
}

int	IscStatement::getMaxRows()
{
	NOT_YET_IMPLEMENTED;

	return 0;
}

int	IscStatement::getQueryTimeout()
{
	NOT_YET_IMPLEMENTED;

	return 0;
}

void IscStatement::cancel()
{
	NOT_YET_IMPLEMENTED;
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

    if ( !isActiveSelect() && outputSqlda.sqlda->sqld < 1)
		throw SQLEXCEPTION (NO_RECORDS_FOR_FETCH, "current statement doesn't return results");
	
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
	connection->GDS->_dsql_set_cursor_name (statusVector, &statementHandle, (char*) name, 0);

	if (statusVector [1])
		THROW_ISC_EXCEPTION (connection, statusVector);
}

void IscStatement::setEscapeProcessing(bool enable)
{
	NOT_YET_IMPLEMENTED;
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
		bool isActiveCursor = this->isActiveCursor();
		openCursor = false;
		typeStmt = stmtNone;

		if ( connection )
		{
			if ( isActiveCursor )
			{
				// Close cursors too.
				ISC_STATUS statusVector[20];
				connection->GDS->_dsql_free_statement( statusVector, &statementHandle, DSQL_close );
				// Cursor already closed or not assigned
				if ( statusVector[1] && statusVector[1] != 335544569)
					THROW_ISC_EXCEPTION (connection, statusVector);
			}			
			
			if ( transactionLocal )
			{
				if ( transactionInfo.autoCommit )
					commitLocal();
			}
			else if ( connection->transactionInfo.autoCommit )
				connection->commitAuto();
		}
	}
}

void IscStatement::prepareStatement(const char * sqlString)
{
	clearResults();
	sql = sqlString;
	CFbDll * GDS = connection->GDS;

	// Make sure we have a transaction started.  Allocate a statement.

	isc_tr_handle transHandle = startTransaction();
	ISC_STATUS statusVector [20];
	GDS->_dsql_allocate_statement (statusVector, &connection->databaseHandle, &statementHandle);

	if (statusVector [1])
		THROW_ISC_EXCEPTION (connection, statusVector);

	// Prepare dynamic SQL statement.  Make first attempt to get parameters

	int dialect = connection->getDatabaseDialect();
	GDS->_dsql_prepare (statusVector, &transHandle, &statementHandle,
					  0, (char*) sqlString, dialect, outputSqlda);

	if (statusVector [1])
		THROW_ISC_EXCEPTION (connection, statusVector);

	// If we didn't allocate a large enough SQLDA, try again.

	if (outputSqlda.checkOverflow())
	{
		GDS->_dsql_describe (statusVector, &statementHandle, dialect, outputSqlda);
		if (statusVector [1])
			THROW_ISC_EXCEPTION (connection, statusVector);
	}
	
	outputSqlda.allocBuffer ( this );

	openCursor			= false;
	typeStmt			= stmtPrepare;
	resultsCount		= 1;
	resultsSequence		= 0;
	int statementType	= getUpdateCounts();
	
	switch ( statementType )
	{
	case isc_info_sql_stmt_ddl:
		typeStmt = stmtDDL;
		break;
	case isc_info_sql_stmt_insert:
		typeStmt = stmtInsert | stmtModify;
		break;
	case isc_info_sql_stmt_update:
		typeStmt = stmtUpdate | stmtModify;
		break;
	case isc_info_sql_stmt_delete:
		typeStmt = stmtDelete | stmtModify;
		break;
	case isc_info_sql_stmt_exec_procedure:
		typeStmt = stmtProcedure;
		break;
	}

	numberColumns		= outputSqlda.getColumnCount();
}

bool IscStatement::execute()
{
	if ( isActiveSelect() && connection->transactionInfo.autoCommit && resultSets.isEmpty() )
		clearSelect();

	// Make sure there is a transaction
	ISC_STATUS statusVector [20];
	isc_tr_handle transHandle = startTransaction();

	int dialect = connection->getDatabaseDialect ();
	if (connection->GDS->_dsql_execute2 (statusVector, &transHandle, &statementHandle, 
			dialect, inputSqlda, NULL))
	{
		if ( connection->transactionInfo.autoCommit )
			connection->rollbackAuto();
		clearSelect();
		THROW_ISC_EXCEPTION (connection, statusVector);
	}

	resultsCount		= 1;
	resultsSequence		= 0;
	int statementType	= getUpdateCounts();

	if ( isActiveSelect() )
		typeStmt = stmtNone;

	openCursor = false;

	switch (statementType)
	{
	case isc_info_sql_stmt_ddl:
		{
			clearSelect();
			if ( transactionLocal )
			{
				if ( transactionInfo.autoCommit )
					commitLocal();
			}
			else if ( connection->transactionInfo.autoCommit )
				connection->commitAuto();
			freeStatementHandle();
		}
		break;

	case isc_info_sql_stmt_select:
		typeStmt = stmtSelect;
		openCursor = true;
		break;

	case isc_info_sql_stmt_select_for_upd:
		typeStmt = stmtSelectForUpdate;
		openCursor = true;
		break;

	case isc_info_sql_stmt_insert:
	case isc_info_sql_stmt_update:
	case isc_info_sql_stmt_delete:
		if ( transactionLocal )
		{
			if ( transactionInfo.autoCommit )
				commitLocal();
		}
		else if ( connection->transactionInfo.autoCommit )
			connection->commitAuto();
		break;
	}

	return outputSqlda.sqlda->sqld > 0;
}

bool IscStatement::executeProcedure()
{
	ISC_STATUS statusVector [20];
	isc_tr_handle transHandle = startTransaction();

	int dialect = connection->getDatabaseDialect ();
	if (connection->GDS->_dsql_execute2 (statusVector, &transHandle, &statementHandle,
			dialect, inputSqlda, outputSqlda))
	{
		if ( connection->transactionInfo.autoCommit )
			connection->rollbackAuto();
		THROW_ISC_EXCEPTION (connection, statusVector);
	}

	resultsCount		= 1;
	resultsSequence		= 0;
	getUpdateCounts();

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
	CFbDll * GDS = connection->GDS;

	GDS->_dsql_sql_info (statusVector, &statementHandle, 
						sizeof (requestInfo), requestInfo,
						sizeof (buffer), buffer);

	if (statusVector [1])
		THROW_ISC_EXCEPTION (connection, statusVector);

	int statementType = 0;
	int insertCount = 0, updateCount = 0, deleteCount = 0;

	for (char *p = buffer; *p != isc_info_end;)
	{
		char item = *p++;
		int length = GDS->_vax_integer (p, 2);
		p += 2;
		switch (item)
		{
		case isc_info_sql_records:
			{
				for (char *q = p; *q != isc_info_end;)
				{
					char item = *q++;
					int l = GDS->_vax_integer (q, 2);
					q += 2;
					switch (item)
					{
					case isc_info_req_insert_count:
						insertCount = GDS->_vax_integer (q, l);
						break;

					case isc_info_req_delete_count:
						deleteCount = GDS->_vax_integer (q, l);
						break;

					case isc_info_req_update_count:
						updateCount = GDS->_vax_integer (q, l);
						break;
					}
					q += l;
				}
			}
			break;

		case isc_info_sql_stmt_type:
			statementType = GDS->_vax_integer (p, length);
			break;
		}
		p += length;
	}

	summaryUpdateCount = MAX (insertCount, deleteCount);
	summaryUpdateCount = MAX (summaryUpdateCount, updateCount);

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
				data [ var->sqllen ] = 0;    
				value->setString (data, false);
				}
				break;

			case SQL_VARYING:
				{
				int length = *((short*) var->sqldata);
				char *data = var->sqldata + 2;
				if ( length < var->sqllen )
				{
					data [length] = 0;
					value->setString (data, false);
				}
				else
					value->setString (length, data, true);

				}
				break;

			case SQL_BOOLEAN:
				value->setValue (*(TYPE_BOOLEAN*) var->sqldata, var->sqlscale);
				break;

			case SQL_SHORT:
				value->setValue (*(short*) var->sqldata, var->sqlscale);
				break;

			case SQL_LONG:
				value->setValue (*(int*) var->sqldata, var->sqlscale);
				break;

			case SQL_FLOAT:
				value->setValue (*(float*) var->sqldata);
				break;

			case SQL_D_FLOAT:
			case SQL_DOUBLE:
				value->setValue (*(double*) var->sqldata);
				break;

			case SQL_QUAD:
			case SQL_INT64:
				value->setValue (*(QUAD*) var->sqldata, var->sqlscale);
				break;

			case SQL_BLOB:
				{
				IscBlob* blob = new IscBlob (this, var);
				value->setValue (blob);
				blob->release();
				}
				break;

			case SQL_TIMESTAMP:
				{
				ISC_TIMESTAMP *date = (ISC_TIMESTAMP*) var->sqldata;
				TimeStamp timestamp;
				timestamp.date = date->timestamp_date;
				timestamp.nanos = date->timestamp_time;
				value->setValue (timestamp);
				}
				break;

			case SQL_TYPE_DATE:
				{
				ISC_DATE date = *(ISC_DATE*) var->sqldata;
				int days = date;
				DateTime dateTime;
				dateTime.date = days; //NOMEY +
				value->setValue (dateTime);
				}
				break;

			case SQL_TYPE_TIME:
				{
				ISC_TIME data = *(ISC_TIME*) var->sqldata;
				SqlTime time;
				time.timeValue = data;
				value->setValue (time);
				}
				break;

			case SQL_ARRAY:
				{
				IscArray* blob = new IscArray (this, var);
				value->setValue (blob);
				blob->release();
				}
				break;
			}
}

ISC_DATE IscStatement::getIscDate(DateTime value)
{
	return value.date;
}

ISC_TIMESTAMP IscStatement::getIscTimeStamp(TimeStamp value)
{
	ISC_TIMESTAMP date;

	date.timestamp_date = value.date ;
	date.timestamp_time =  value.nanos;

	return date;
}

ISC_TIME IscStatement::getIscTime(SqlTime value)
{
	return value.timeValue * ISC_TIME_SECONDS_PRECISION;
}

void IscStatement::clearSelect()
{
	if ( isActiveSelect() )
	{
		resultsCount = 0;
		resultsSequence	= 0;
		openCursor = false;
		typeStmt = stmtNone;
		if ( transactionLocal )
		{
			if ( transactionInfo.autoCommit )
				commitLocal();
		}
		else if ( connection->transactionInfo.autoCommit )
			connection->commitAuto();
		freeStatementHandle();
	}
}

void IscStatement::freeStatementHandle()
{
	if ( connection && statementHandle )
	{
		ISC_STATUS statusVector [20];
		connection->GDS->_dsql_free_statement (statusVector, &statementHandle, DSQL_drop);
		statementHandle = NULL;
	}
}

}; // end namespace IscDbcLibrary
