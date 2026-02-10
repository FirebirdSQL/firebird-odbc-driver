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
#include <algorithm>
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

using namespace Firebird;

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscStatement::IscStatement(IscConnection *connect) :
	inputSqlda{connect, Sqlda::SQLDA_INPUT},
	outputSqlda{connect, Sqlda::SQLDA_OUTPUT}
{
	connection = connect;
	useCount = 1;
	numberColumns = 0;
	statementHandle = NULL;
	fbResultSet = nullptr;
	transactionLocal = false;
	transactionStatusChange = false;
	transactionStatusChangingToLocal = false;

	openCursor = false;
	typeStmt = stmtNone;
	resultsCount = 0;
	resultsSequence	= 0;
	summaryUpdateCount = 0;
}

IscStatement::~IscStatement()
{
	for (auto* resultSet : resultSets)
		resultSet->close();
	resultSets.clear();

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
		ThrowStatusWrapper status( connection->GDS->_status );
		try
		{
			tr.transactionHandle->rollback( &status );
			tr.transactionHandle = nullptr;
		} 
		catch( const FbException& error )
		{
			if( tr.transactionHandle ) {
				tr.transactionHandle->release();
				tr.transactionHandle = nullptr;
			}
			THROW_ISC_EXCEPTION( connection, error.getStatus() );
		}
	}
	tr.transactionPending = false;
}

void IscStatement::commitLocal()
{
	InfoTransaction	&tr = transactionInfo;

	if ( tr.transactionHandle )
	{
		if ( !tr.nodeParamTransaction && transactionLocal )
			transactionLocal = false;

		ThrowStatusWrapper status( connection->GDS->_status );
		try
		{
			tr.transactionHandle->commit( &status );
			tr.transactionHandle = nullptr;
		} 
		catch( const FbException& error )
		{
			rollbackLocal();
			THROW_ISC_EXCEPTION( connection, error.getStatus() );
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

ITransaction* IscStatement::startTransaction()
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

	IUtil* utl    = connection->GDS->_master->getUtilInterface();
	IXpbBuilder* tpb = nullptr;
	ThrowStatusWrapper status( connection->GDS->_status );
	try
	{
		char *tpbBuffer;
		int count;

		if ( !tr->nodeParamTransaction )
		{
			tpb = utl->getXpbBuilder(&status, IXpbBuilder::TPB, NULL, 0);

			tpb->insertTag( &status, (tr->transactionExtInit & TRA_ro) ? isc_tpb_read   : isc_tpb_write );
			tpb->insertTag( &status, (tr->transactionExtInit & TRA_nw) ? isc_tpb_nowait : isc_tpb_wait  );

			/* Isolation level */
			switch( tr->transactionIsolation )
			{
				case 0x00000008L:
					// SQL_TXN_SERIALIZABLE:
					tpb->insertTag( &status, isc_tpb_consistency );
					break;

				case 0x00000004L:
					// SQL_TXN_REPEATABLE_READ:
					tpb->insertTag( &status, isc_tpb_concurrency );
					break;

				case 0x00000001L:
					// SQL_TXN_READ_UNCOMMITTED:
					tpb->insertTag( &status, isc_tpb_read_committed );
					tpb->insertTag( &status, isc_tpb_no_rec_version );
					break;

				case 0x00000002L:
				default:
					// SQL_TXN_READ_COMMITTED:
					tpb->insertTag( &status, isc_tpb_read_committed );
					tpb->insertTag( &status, isc_tpb_rec_version );
					break;
			}

			if ( !(tr->transactionExtInit & TRA_nw) 
				&& connection->attachment->isFirebirdVer2_0()
				&& connection->attachment->getUseLockTimeoutWaitTransactions() )
			{
				tpb->insertInt(&status, isc_tpb_lock_timeout, connection->attachment->getUseLockTimeoutWaitTransactions() );
			}
			
			count = tpb->getBufferLength(&status);
			tpbBuffer = (char*)tpb->getBuffer(&status);
		}
		else
		{
			tpbBuffer = tr->nodeParamTransaction->tpbBuffer;
			count = tr->nodeParamTransaction->lengthTpbBuffer;
			tr->autoCommit = tr->nodeParamTransaction->autoCommit;
		}

		tr->transactionHandle =
			connection->attachment->databaseHandle->startTransaction( &status, count, (const unsigned char*)tpbBuffer );

		if( tpb ) {
			tpb->dispose();
			tpb = nullptr;
		}
	}
	catch( const FbException& error )
	{
		if( tpb ) tpb->dispose();
		THROW_ISC_EXCEPTION ( connection, error.getStatus() );
	}

	if ( !tr->autoCommit )
		tr->transactionPending = true;

    return tr->transactionHandle;
}

IscResultSet* IscStatement::createResultSet()
{
	IscResultSet *resultSet = new IscResultSet (this);
	resultSets.push_back (resultSet);

	return resultSet;
}

void IscStatement::close()
{
	for (auto* resultSet : resultSets)
		resultSet->close();
	resultSets.clear();

	if ( isActiveSelect() )
	{
		//openCursor = false;
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
	if (connection)
		connection->cancelOperation();
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

    if ( !isActiveSelect() && outputSqlda.getColumnCount() < 1)
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
	ThrowStatusWrapper status( connection->GDS->_status );
	try
	{
		statementHandle->setCursorName( &status, name );
	}
	catch( const FbException& error )
	{
		THROW_ISC_EXCEPTION ( connection, error.getStatus() );
	}
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

	if (outputSqlda.getColumnCount() > 0)
		return true;

	return false;
}

int IscStatement::getUpdateCount()
{
	if (outputSqlda.getColumnCount() > 0)
		return -1;

	return summaryUpdateCount;
}

void IscStatement::deleteResultSet(IscResultSet * resultSet)
{
	resultSets.erase(std::remove(resultSets.begin(), resultSets.end(), resultSet), resultSets.end());
	if (resultSets.empty())
	{
		bool isActiveCursor = this->isActiveCursor();
		typeStmt = stmtNone;

		if ( connection )
		{
			if ( isActiveCursor )
			{
				closeFbResultSet();
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

	ITransaction* transHandle = startTransaction();
	ThrowStatusWrapper status( connection->GDS->_status );
	try
	{
		int dialect = connection->getDatabaseDialect();

		statementHandle =
			connection->databaseHandle->prepare( &status, transHandle, 0, sqlString, dialect, IStatement::PREPARE_PREFETCH_METADATA );

		inputSqlda.allocBuffer( this, statementHandle->getInputMetadata( &status ) );
		outputSqlda.allocBuffer( this, statementHandle->getOutputMetadata( &status ) );

		//OOAPI gives a 100% way to check whether stmt is selectable or not.
		openCursor = ( statementHandle->getFlags(&status) & IStatement::FLAG_HAS_CURSOR );
	}
	catch( const FbException& error )
	{
		THROW_ISC_EXCEPTION ( connection, error.getStatus() );
	}

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
	if ( isActiveSelect() && connection->transactionInfo.autoCommit && resultSets.empty() )
		clearSelect();

	// Use savepoints for statement-level error isolation when auto-commit is OFF
	const bool useSavepoint = !connection->transactionInfo.autoCommit
	                          && connection->transactionInfo.transactionHandle;
	const char* svpName = "FBODBC_SVP";

	ThrowStatusWrapper status( connection->GDS->_status );
	try
	{
		// Make sure there is a transaction
		ITransaction* transHandle = startTransaction();

		if ( useSavepoint )
			connection->setSavepoint(svpName);

		inputSqlda.checkAndRebuild();
		auto* _imeta = inputSqlda.useExecBufferMeta ? inputSqlda.execMeta   : inputSqlda.meta;
		auto& _ibuf  = inputSqlda.useExecBufferMeta ? inputSqlda.execBuffer : inputSqlda.buffer;

		if( openCursor == false )
		{
			statementHandle->execute( &status, transHandle, _imeta, _ibuf.data(), NULL, NULL);
		}
		else
		{
			fbResultSet = statementHandle->openCursor( &status, transHandle,
			                                           _imeta, _ibuf.data(),
			                                           outputSqlda.meta, 0 );
		}

		if ( useSavepoint )
			connection->releaseSavepoint(svpName);
	}
	catch( const FbException& error )
	{
		if ( useSavepoint )
			connection->rollbackSavepoint(svpName);
		else if ( connection->transactionInfo.autoCommit )
			connection->rollbackAuto();
		clearSelect();
		THROW_ISC_EXCEPTION ( connection, error.getStatus() );
	}

	resultsCount		= 1;
	resultsSequence		= 0;
	int statementType	= getUpdateCounts();

	if ( isActiveSelect() )
		typeStmt = stmtNone;

	//* already set it on prepare()
	//openCursor = false;

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
		//openCursor = true;
		break;

	case isc_info_sql_stmt_select_for_upd:
		typeStmt = stmtSelectForUpdate;
		//openCursor = true;
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

	return outputSqlda.getColumnCount() > 0;
}

bool IscStatement::executeProcedure()
{
	// Use savepoints for statement-level error isolation when auto-commit is OFF
	const bool useSavepoint = !connection->transactionInfo.autoCommit
	                          && connection->transactionInfo.transactionHandle;
	const char* svpName = "FBODBC_SVP";

	ThrowStatusWrapper status( connection->GDS->_status );
	try
	{
		// Make sure there is a transaction
		ITransaction* transHandle = startTransaction();

		if ( useSavepoint )
			connection->setSavepoint(svpName);

		inputSqlda.checkAndRebuild();
		auto* _imeta = inputSqlda.useExecBufferMeta ? inputSqlda.execMeta   : inputSqlda.meta;
		auto& _ibuf  = inputSqlda.useExecBufferMeta ? inputSqlda.execBuffer : inputSqlda.buffer;

		statementHandle->execute( &status, transHandle,
		                          _imeta, _ibuf.data(),
		                          outputSqlda.meta, outputSqlda.buffer.data() );

		if ( useSavepoint )
			connection->releaseSavepoint(svpName);
	}
	catch( const FbException& error )
	{
		if ( useSavepoint )
			connection->rollbackSavepoint(svpName);
		else if ( connection->transactionInfo.autoCommit )
			connection->rollbackAuto();
		THROW_ISC_EXCEPTION ( connection, error.getStatus() );
	}

	resultsCount		= 1;
	resultsSequence		= 0;
	getUpdateCounts();

	return outputSqlda.getColumnCount() > 0;
}

void IscStatement::clearResults()
{
	// Phase 9.11: Intentionally empty.
	// Statement cleanup is handled by close()/freeStatementHandle() instead.
}

int IscStatement::objectVersion()
{
	return STATEMENT_VERSION;
}

int IscStatement::getUpdateCounts()
{
	char buffer [128];
	CFbDll * GDS = connection->GDS;

	ThrowStatusWrapper status( GDS->_status );
	try
	{
		statementHandle->getInfo( &status, 
						sizeof (requestInfo), (const unsigned char*)requestInfo,
						sizeof (buffer), (unsigned char*)buffer );
	}
	catch( const FbException& error )
	{
		THROW_ISC_EXCEPTION ( connection, error.getStatus() );
	}

	int statementType = 0;
	int insertCount = 0, updateCount = 0, deleteCount = 0;

	for (char *p = buffer; *p != isc_info_end;)
	{
		char item = *p++;
		int length = fb_vax_integer(p, 2);
		p += 2;
		switch (item)
		{
		case isc_info_sql_records:
			{
				for (char *q = p; *q != isc_info_end;)
				{
					char item = *q++;
					int l = fb_vax_integer(q, 2);
					q += 2;
					switch (item)
					{
					case isc_info_req_insert_count:
						insertCount = fb_vax_integer(q, l);
						break;

					case isc_info_req_delete_count:
						deleteCount = fb_vax_integer(q, l);
						break;

					case isc_info_req_update_count:
						updateCount = fb_vax_integer(q, l);
						break;
					}
					q += l;
				}
			}
			break;

		case isc_info_sql_stmt_type:
			statementType = fb_vax_integer(p, length);
			break;
		}
		p += length;
	}

	summaryUpdateCount = MAX (insertCount, deleteCount);
	summaryUpdateCount = MAX (summaryUpdateCount, updateCount);

	return statementType;
}

void IscStatement::setValue( Value *value, unsigned index, Sqlda& sqlData )
{
	auto * var = sqlData.Var( index );
	auto * buf = var->sqldata;
	
	if( sqlData.isNull( index ) )
		value->setNull();
	else
		switch ( var->sqltype )
			{
			case SQL_TEXT:
				{
				buf [ var->sqllen ] = 0;    
				value->setString (buf, false);
				}
				break;

			case SQL_VARYING:
				{
				int length = *((short*)buf);
				char *data = buf + 2;
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
				value->setValue (*(TYPE_BOOLEAN*)buf, var->sqlscale);
				break;

			case SQL_SHORT:
				value->setValue (*(short*)buf, var->sqlscale);
				break;

			case SQL_LONG:
				value->setValue (*(int*)buf, var->sqlscale);
				break;

			case SQL_FLOAT:
				value->setValue (*(float*)buf);
				break;

			case SQL_D_FLOAT:
			case SQL_DOUBLE:
				value->setValue (*(double*)buf);
				break;

			case SQL_QUAD:
			case SQL_INT64:
				value->setValue (*(QUAD*)buf, var->sqlscale);
				break;

			case SQL_BLOB:
				{
				IscBlob* blob = new IscBlob (this, buf, var->sqlsubtype);
				value->setValue (blob);
				blob->release();
				}
				break;

			case SQL_TIMESTAMP:
				{
				ISC_TIMESTAMP *date = (ISC_TIMESTAMP*)buf;
				TimeStamp timestamp;
				timestamp.date = date->timestamp_date;
				timestamp.nanos = date->timestamp_time;
				value->setValue (timestamp);
				}
				break;

			case SQL_TYPE_DATE:
				{
				ISC_DATE date = *(ISC_DATE*)buf;
				int days = date;
				DateTime dateTime;
				dateTime.date = days; //NOMEY +
				value->setValue (dateTime);
				}
				break;

			case SQL_TYPE_TIME:
				{
				ISC_TIME data = *(ISC_TIME*)buf;
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
		closeFbResultSet();
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
		ThrowStatusWrapper status( connection->GDS->_status );
		try {
			statementHandle->free( &status );
			statementHandle = nullptr;
		}
		catch( const FbException& ) {}

		if( statementHandle ) {
			statementHandle->release();
			statementHandle = nullptr;
		}
	}
}

void IscStatement::closeFbResultSet()
{
	if( !fbResultSet ) return;

	ThrowStatusWrapper status( connection->GDS->_status );
	try {
		fbResultSet->close( &status );
		fbResultSet = nullptr;
	} catch( const FbException& error ) {
		if( fbResultSet ) {
			fbResultSet->release();
			fbResultSet = nullptr;
		}
		THROW_ISC_EXCEPTION ( connection, error.getStatus() );
	}
}

}; // end namespace IscDbcLibrary
