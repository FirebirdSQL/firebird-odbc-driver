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
#include "Attachment.h"
#include "BinaryBlob.h"
#include "Value.h"
#include "IscStatementMetaData.h"

using namespace Firebird;

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
	batchCancel();  // Release any open IBatch handle
	delete statementMetaDataIPD;
	delete statementMetaDataIRD;
}

ResultSet* IscOdbcStatement::executeQuery()
{
	if (outputSqlda.columnsCount < 1)
		throw SQLEXCEPTION (RUNTIME_ERROR, "statement is not a Select");

	IscStatement::execute();
	getMoreResults();

	return getResultSet();
}

void IscOdbcStatement::executeMetaDataQuery()
{
	if (outputSqlda.columnsCount < 1)
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

	char * tempSql = NULL;
	int * labelParamArray = NULL;
	int replaceParamArray;

	if ( (replaceParamArray = replacementArrayParamForStmtUpdate( tempSql, labelParamArray )) )
	{
		freeStatementHandle();
		IscStatement::prepareStatement ( (const char*)tempSql );
		getInputParameters();
	}

	if ( replaceParamArray )
	{
		int * label = labelParamArray;
		while ( replaceParamArray-- )
		{
			CAttrSqlVar *var = inputSqlda.Var ( *label++ );
			var->replaceForParamArray = true;
		}

		free ( tempSql );
		free ( labelParamArray );
	}
}

void IscOdbcStatement::getInputParameters()
{
	// Phase 9.11: Removed commented-out legacy ISC _dsql_describe_bind code.
	// Input parameters are now obtained via IStatement::getInputMetadata() in Sqlda.
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

	statementMetaDataIPD = new IscStatementMetaData (this, &inputSqlda);

	return statementMetaDataIPD;
}

StatementMetaData* IscOdbcStatement::getStatementMetaDataIRD()
{
	if (statementMetaDataIRD)
		return statementMetaDataIRD;

	statementMetaDataIRD = new IscStatementMetaData (this, &outputSqlda);

	return statementMetaDataIRD;
}

//  
//  UPDATE "TESTTBL" SET "KOD"=?,"ARRAYFLD1"=?,"ARRAYFLD2"=? WHERE "KOD" = ? AND "ARRAYFLD1" = ? AND "ARRAYFLD2" = ?
//  "ARRAYFLD1","ARRAYFLD2" - it's array
//	Our purpose:
//		set attributes(relname,sqlname) of param 2 to param 5
//			and param 3 to param 6
// 
int IscOdbcStatement::replacementArrayParamForStmtUpdate( char *& tempSql, int *& labelParamArray )
{
	const char *strSql = sql, *ch;
	int numberColumns = inputSqlda.columnsCount;
	int *offsetParam = NULL;
	int *offsetNameParam = NULL;
	int countDefined = 0;

	for (int n = 0; n < numberColumns; ++n)
	{
		auto* var = &inputSqlda.sqlvar.at( n );
		switch ( var->sqltype )
		{
		case SQL_ARRAY:
			if ( var->sqlname && *var->sqlname == 0 )
			{
				if ( !offsetParam )
				{
					offsetParam = new int[numberColumns];
					offsetNameParam = new int[numberColumns];
			        memset ( offsetNameParam, 0, sizeof(int) * numberColumns );

					int *param = offsetParam;

					ch = strSql;

					while ( *ch )
					{
						if ( *ch == '?' )
							*param++ = ch - strSql;
						ch++;
					}
				}

				const char *end = strSql + offsetParam[n];

				while ( end > strSql && *end != '=' ) --end;
				--end; // '='
				while ( end > strSql && *end == ' ' ) --end;

				char delimiter = '"';

				if ( *end != '"' )
				{
					delimiter = ' ';
					end++;
				}

				const char *start = end;

				while ( start-- > strSql )
					if ( *start == delimiter )
					{
						start++;
						break;
					}

				int len = end - start;

				for ( int m = 0; m < n; ++m )
				{
					auto * varIn = &inputSqlda.sqlvar.at( m );
					if ( varIn->sqlname && strlen(varIn->sqlname) == len && !strncasecmp ( varIn->sqlname, start, len ) )
					{
						var->sqlname = varIn->sqlname;
						var->relname = varIn->relname;

						offsetNameParam[n] = end - strSql;

						if ( delimiter == '"' )
							offsetNameParam[n]++;

						countDefined++;
						break;
					}
				}
				
			}
			break;
		}
	}

	if ( countDefined )
	{
		int lenOldSql = (int)strlen(strSql);
		tempSql = (char *)malloc ( countDefined * 3 + lenOldSql + 1);
		labelParamArray = (int *)malloc ( countDefined * sizeof(int) );
		int n, offset = 0;
		int * label = labelParamArray;

		ch = strSql;
	
		for ( n = 0; n < numberColumns; ++n )
		{
			int &offsetEndName = offsetNameParam[n];

			if ( offsetEndName )
			{
				memcpy ( &tempSql[offset], ch, offsetEndName - offset );
				ch += offsetEndName;
				offset += offsetEndName;
				memcpy ( &tempSql[offset], "[1]", 3 );
				offset += 3;
				*label++ = n + 1;
			}
		}
		
		n = lenOldSql - ( ch - strSql );

		if ( n != 0 )
		{
			memcpy ( &tempSql[offset], ch, n );
			offset += n;
		}

		tempSql[offset] = '\0';
		delete [] offsetParam;
		delete [] offsetNameParam;

		return countDefined;
	}
	return 0;
}

int IscOdbcStatement::objectVersion()
{
	return INTERNALSTATEMENT_VERSION;
}

// ============================================================
// Batch execution (IBatch API, FB4+). Phase 9.1.
// ============================================================

// Batch row status constants (matching ODBC SQL_PARAM_* values from SQLEXT.H)
static constexpr unsigned short kBatchRowSuccess = 0;  // SQL_PARAM_SUCCESS
static constexpr unsigned short kBatchRowError   = 5;  // SQL_PARAM_ERROR

bool IscOdbcStatement::isBatchSupported()
{
	// IBatch requires Firebird 4.0+ and a prepared statement
	if (!statementHandle || !connection || !connection->attachment)
		return false;

	return connection->attachment->isVersionAtLeast(4, 0);
}

void IscOdbcStatement::batchBegin()
{
	if (batch_)
	{
		batchCancel();
	}

	// Don't create the batch yet — we need to wait until after the first
	// inputParam() call, which may override metadata via setTypeVarying(),
	// setSqlLen(), etc. The batch will be lazily created in batchAdd().
	ThrowStatusWrapper status(connection->GDS->_status);
	try
	{
		startTransaction();
		batchRowCount_ = 0;

		// Phase 9.2: Detect if any input parameter is a BLOB.
		// If so, we'll enable BLOB_ID_ENGINE policy in the batch BPB
		// and use registerBlob() for each BLOB value.
		batchHasBlobs_ = false;
		for (const auto& var : inputSqlda.sqlvar)
		{
			if ((var.orgSqlProperties.sqltype & ~1) == SQL_BLOB)
			{
				batchHasBlobs_ = true;
				break;
			}
		}
	}
	catch (const FbException& error)
	{
		THROW_ISC_EXCEPTION(connection, error.getStatus());
	}
}

void IscOdbcStatement::batchAdd()
{
	ThrowStatusWrapper status(connection->GDS->_status);
	try
	{
		// Lazily create the batch on first add().
		// Use the original metadata (meta) which has the maximum column sizes.
		if (!batch_)
		{
			IUtil* utl = connection->GDS->_master->getUtilInterface();
			IXpbBuilder* bpb = utl->getXpbBuilder(&status, IXpbBuilder::BATCH, NULL, 0);

			bpb->insertTag(&status, IBatch::TAG_RECORD_COUNTS);
			bpb->insertTag(&status, IBatch::TAG_MULTIERROR);
			bpb->insertTag(&status, IBatch::TAG_DETAILED_ERRORS);

			// Phase 9.2: Enable inline BLOB support when metadata has BLOB columns.
			// BLOB_ID_ENGINE lets the batch engine assign internal blob IDs;
			// we use registerBlob() to map existing server-side blob IDs.
			if (batchHasBlobs_)
				bpb->insertInt(&status, IBatch::TAG_BLOB_POLICY, IBatch::BLOB_ID_ENGINE);

			batch_ = statementHandle->createBatch(&status, inputSqlda.meta,
				bpb->getBufferLength(&status), bpb->getBuffer(&status));

			bpb->dispose();
		}

		// Assemble a contiguous message buffer using the original meta layout.
		// The ODBC conversion functions may:
		// 1. Redirect sqldata to point at app buffers (via setSqlData)
		// 2. Change sqltype (e.g., SQL_VARYING → SQL_TEXT for array params)
		// 3. Change sqllen to the actual data length
		//
		// We must put data back into buffer in the ORIGINAL meta format,
		// since that's what the batch was created with.
		for (const auto& var : inputSqlda.sqlvar)
		{
			char* bufDest = &inputSqlda.buffer.at(var.offsetData);
			short* indDest = (short*)&inputSqlda.buffer.at(var.offsetNull);

			// Copy null indicator
			*indDest = *var.sqlind;

			if (*var.sqlind == -1) // null — no data to copy
				continue;

			unsigned origType = var.orgSqlProperties.sqltype;
			unsigned curType = var.sqltype;

			if (curType == SQL_TEXT && origType == SQL_VARYING)
			{
				// Conversion changed type from SQL_VARYING to SQL_TEXT.
				// sqldata points to raw string data (no length prefix).
				// sqllen has the actual string length.
				// Write it back as SQL_VARYING: 2-byte length + data.
				unsigned short actualLen = (unsigned short)var.sqllen;
				*(unsigned short*)bufDest = actualLen;
				if (actualLen > 0)
					memcpy(bufDest + 2, var.sqldata, actualLen);
			}
			else if (var.sqldata != bufDest)
			{
				// sqldata was redirected — copy data back into buffer.
				if (origType == SQL_VARYING)
				{
					// Copy 2-byte length prefix + actual data
					unsigned short actualLen = *(unsigned short*)var.sqldata;
					memcpy(bufDest, var.sqldata, 2 + actualLen);
				}
				else
				{
					// Fixed-length: copy original length worth of data
					memcpy(bufDest, var.sqldata, var.orgSqlProperties.sqllen);
				}
			}
			// else: sqldata points into buffer already — data is in place
		}

		// Phase 9.2: Register existing server-side BLOBs with the batch.
		// For each non-null BLOB column, the conversion functions have already
		// created a server-side blob and placed its ISC_QUAD blob ID in the buffer.
		// We call registerBlob() to map that existing blob ID to a batch-internal ID.
		if (batchHasBlobs_)
		{
			for (const auto& var : inputSqlda.sqlvar)
			{
				unsigned origType = var.orgSqlProperties.sqltype & ~1;
				if (origType != SQL_BLOB)
					continue;

				short* indDest = (short*)&inputSqlda.buffer.at(var.offsetNull);
				if (*indDest == -1) // null — skip
					continue;

				ISC_QUAD* blobIdInBuf = (ISC_QUAD*)&inputSqlda.buffer.at(var.offsetData);
				ISC_QUAD existingId = *blobIdInBuf;
				ISC_QUAD batchId = {0, 0};

				// registerBlob maps the existing server-side blob to a batch-internal ID
				batch_->registerBlob(&status, &existingId, &batchId);

				// Replace the blob ID in the message buffer with the batch-internal one
				*blobIdInBuf = batchId;
			}
		}

		batch_->add(&status, 1, inputSqlda.buffer.data());
		++batchRowCount_;
	}
	catch (const FbException& error)
	{
		THROW_ISC_EXCEPTION(connection, error.getStatus());
	}
}

int IscOdbcStatement::batchExecute(unsigned short* statusOut, int nRows)
{
	if (!batch_)
		throw SQLEXCEPTION(RUNTIME_ERROR, "IscOdbcStatement::batchExecute(): batch not started");

	ThrowStatusWrapper status(connection->GDS->_status);
	IBatchCompletionState* cs = nullptr;
	int totalAffected = 0;

	try
	{
		ITransaction* transHandle = startTransaction();
		cs = batch_->execute(&status, transHandle);

		unsigned batchSize = cs->getSize(&status);

		for (unsigned i = 0; i < batchSize && static_cast<int>(i) < nRows; ++i)
		{
			int state = cs->getState(&status, i);

			if (state == IBatchCompletionState::EXECUTE_FAILED)
			{
				if (statusOut)
					statusOut[i] = kBatchRowError;
			}
			else
			{
				if (statusOut)
					statusOut[i] = kBatchRowSuccess;

				// state >= 0 means row count for that statement
				if (state > 0)
					totalAffected += state;
				else if (state == IBatchCompletionState::SUCCESS_NO_INFO)
					totalAffected += 1; // assume 1 row affected
			}
		}

		cs->dispose();
		cs = nullptr;

		// Clean up the batch object
		batch_->close(&status);
		batch_ = nullptr;
		batchRowCount_ = 0;

		// Handle auto-commit (same as IscStatement::execute() does for DML)
		if (transactionLocal)
		{
			if (transactionInfo.autoCommit)
				commitLocal();
		}
		else if (connection->transactionInfo.autoCommit)
			connection->commitAuto();
	}
	catch (const FbException& error)
	{
		if (cs) cs->dispose();
		if (batch_)
		{
			try
			{
				ThrowStatusWrapper cancelStatus(connection->GDS->_status);
				batch_->cancel(&cancelStatus);
			}
			catch (...) {}
		}
		batch_ = nullptr;
		batchRowCount_ = 0;
		THROW_ISC_EXCEPTION(connection, error.getStatus());
	}

	return totalAffected;
}

void IscOdbcStatement::batchCancel()
{
	if (batch_)
	{
		try
		{
			ThrowStatusWrapper status(connection->GDS->_status);
			batch_->cancel(&status);
		}
		catch (...)
		{
			// Ignore errors during cancel
		}
		batch_ = nullptr;
		batchRowCount_ = 0;
	}
}

}; // end namespace IscDbcLibrary
