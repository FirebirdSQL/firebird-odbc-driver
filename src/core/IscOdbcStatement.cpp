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
// Phase 14.4.4: Include fb-cpp BEFORE IscDbc.h to avoid MAX/MIN macro conflicts.
#include <fb-cpp/Batch.h>
#include <fb-cpp/Blob.h>
#include <fb-cpp/Exception.h>

#include "IscDbc.h"
#include "IscOdbcStatement.h"
#include "SQLError.h"
#include "IscResultSet.h"
#include "IscConnection.h"
#include "FbClient.h"
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
	const char *strSql = sql.c_str(), *ch;
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
// Batch execution (fbcpp::Batch, FB4+). Phase 9.1 / 14.4.4.
// ============================================================

// Batch row status constants (matching ODBC SQL_PARAM_* values from SQLEXT.H)
static constexpr unsigned short kBatchRowSuccess = 0;  // SQL_PARAM_SUCCESS
static constexpr unsigned short kBatchRowError   = 5;  // SQL_PARAM_ERROR

bool IscOdbcStatement::isBatchSupported()
{
	// IBatch requires Firebird 4.0+ and a prepared statement
	if (!statementHandle || !connection || !connection->isConnected())
		return false;

	return connection->isVersionAtLeast(4, 0);
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
	try
	{
		startTransaction();
		batchRowCount_ = 0;

		// Phase 9.2: Detect if any input parameter is a BLOB.
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
	try
	{
		// Lazily create the batch on first add().
		if (!batch_)
		{
			if (!fbStatement_ || !connection->transaction_ || !connection->transaction_->isValid())
				throw SQLEXCEPTION(RUNTIME_ERROR, "Batch requires fb-cpp Statement and Transaction");

			fbcpp::BatchOptions opts;
			opts.setRecordCounts(true);
			opts.setMultiError(true);
			if (batchHasBlobs_)
				opts.setBlobPolicy(fbcpp::BlobPolicy::ID_ENGINE);

			batch_ = std::make_unique<fbcpp::Batch>(
				*fbStatement_, *connection->transaction_, opts);
		}

		// Assemble a contiguous message buffer using the original meta layout.
		// Phase 14.4.7.1: Use active buffer (external fbcpp::inMessage or internal).
		char* activeBuf = inputSqlda.activeBufferData();
		for (const auto& var : inputSqlda.sqlvar)
		{
			char* bufDest = activeBuf + var.offsetData;
			short* indDest = (short*)(activeBuf + var.offsetNull);

			*indDest = *var.sqlind;

			if (*var.sqlind == -1)
				continue;

			unsigned origType = var.orgSqlProperties.sqltype;
			unsigned curType = var.sqltype;

			if (curType == SQL_TEXT && origType == SQL_VARYING)
			{
				unsigned short actualLen = (unsigned short)var.sqllen;
				*(unsigned short*)bufDest = actualLen;
				if (actualLen > 0)
					memcpy(bufDest + 2, var.sqldata, actualLen);
			}
			else if (var.sqldata != bufDest)
			{
				if (origType == SQL_VARYING)
				{
					unsigned short actualLen = *(unsigned short*)var.sqldata;
					memcpy(bufDest, var.sqldata, 2 + actualLen);
				}
				else
				{
					memcpy(bufDest, var.sqldata, var.orgSqlProperties.sqllen);
				}
			}
		}

		// Phase 9.2: Register existing server-side BLOBs with the batch.
		if (batchHasBlobs_)
		{
			for (const auto& var : inputSqlda.sqlvar)
			{
				unsigned origType = var.orgSqlProperties.sqltype & ~1;
				if (origType != SQL_BLOB)
					continue;

				short* indDest = (short*)(activeBuf + var.offsetNull);
				if (*indDest == -1)
					continue;

				ISC_QUAD* blobIdInBuf = (ISC_QUAD*)(activeBuf + var.offsetData);
				fbcpp::BlobId existingId;
				existingId.id = *blobIdInBuf;

				fbcpp::BlobId batchId = batch_->registerBlob(existingId);
				*blobIdInBuf = batchId.id;
			}
		}

		batch_->add(1, activeBuf);
		++batchRowCount_;
	}
	catch (const fbcpp::DatabaseException& e)
	{
		throw SQLError::fromDatabaseException(e);
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

	int totalAffected = 0;

	try
	{
		auto cs = batch_->execute();
		unsigned batchSize = cs.getSize();

		for (unsigned i = 0; i < batchSize && static_cast<int>(i) < nRows; ++i)
		{
			int state = cs.getState(i);

			if (state == fbcpp::BatchCompletionState::EXECUTE_FAILED)
			{
				if (statusOut)
					statusOut[i] = kBatchRowError;
			}
			else
			{
				if (statusOut)
					statusOut[i] = kBatchRowSuccess;

				if (state > 0)
					totalAffected += state;
				else if (state == fbcpp::BatchCompletionState::SUCCESS_NO_INFO)
					totalAffected += 1;
			}
		}

		// cs goes out of scope here — RAII cleanup of BatchCompletionState

		// Clean up the batch object — RAII via unique_ptr
		batch_.reset();
		batchRowCount_ = 0;

		// Handle auto-commit
		if (transactionLocal)
		{
			if (transactionInfo.autoCommit)
				commitLocal();
		}
		else if (connection->autoCommit_)
			connection->commitAuto();
	}
	catch (const fbcpp::DatabaseException& e)
	{
		batch_.reset();
		batchRowCount_ = 0;
		throw SQLError::fromDatabaseException(e);
	}
	catch (const FbException& error)
	{
		batch_.reset();
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
			batch_->cancel();
		}
		catch (...)
		{
			// Ignore errors during cancel
		}
		batch_.reset();
		batchRowCount_ = 0;
	}
}

}; // end namespace IscDbcLibrary
