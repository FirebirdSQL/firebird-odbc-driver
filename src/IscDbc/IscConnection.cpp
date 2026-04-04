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
 *	Changes
 *
 *	2003-03-24	IscConnection.cpp
 *				Contributed by Norbert Meyer
 *				o If ++attachment means attachment->addRef() 
 *				  then let's say so.
 *				o In close() set statement->connection to NULL.
 *				  This prevents connection->deleteStatement(this)
 *				  in destructor of IscStatement.
 *
 *
 *	2002-06-08  IscConnection::startTransaction()
 *				Contributed by Carlos Alvarez. New implementation
 *				to better support different transaction options.
 *
 *	2002-05-20	IscConnection.cpp
 *		
 *				Contributed by Robert Milharcic
 *				o better management of statements variable
 *
 */

// IscConnection.cpp: implementation of the IscConnection class.
//
//////////////////////////////////////////////////////////////////////

#include <time.h>
#include <locale.h>
#include <string.h>
#include <sstream>
#include "IscDbc.h"
#include "EnvShare.h"
#include "IscConnection.h"
#include "IscProceduresResultSet.h"
#include "IscTablePrivilegesResultSet.h"
#include "SQLError.h"
#include <algorithm>
#include "IscOdbcStatement.h"
#include "IscUserEvents.h"
#include "IscPreparedStatement.h"
#include "IscDatabaseMetaData.h"
#include "Parameters.h"
#include "ParametersEvents.h"
#include "FbClient.h"
#include "Mlist.h"
#include "../SetupAttributes.h"
#include "MultibyteConvert.h"
#include <fb-cpp/Transaction.h>
#include <fb-cpp/Attachment.h>
#include <fb-cpp/Exception.h>

using namespace Firebird;

namespace IscDbcLibrary {

extern char charTable [];

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern "C" Connection* createConnection()
{
	setlocale( LC_ALL, ".ACP" );
	return new IscConnection;
}

IscConnection::IscConnection()
{
	init();
}

IscConnection::IscConnection(IscConnection * source)
{
	init();
	// Phase 14.2: Copy connection handle and metadata from source
	databaseHandle = source->databaseHandle;
	GDS = source->GDS;
	// Copy all metadata from source
	dsn_ = source->dsn_;
	databaseName_ = source->databaseName_;
	databaseServerName_ = source->databaseServerName_;
	databaseNameFromServer_ = source->databaseNameFromServer_;
	userName_ = source->userName_;
	userAccess_ = source->userAccess_;
	userType_ = source->userType_;
	serverVersion_ = source->serverVersion_;
	databaseProductName_ = source->databaseProductName_;
	majorFb_ = source->majorFb_;
	minorFb_ = source->minorFb_;
	versionFb_ = source->versionFb_;
	charsetCode_ = source->charsetCode_;
	pageSize_ = source->pageSize_;
	connectionTimeout_ = source->connectionTimeout_;
	serverBaseLevel_ = source->serverBaseLevel_;
	databaseDialect_ = source->databaseDialect_;
	useSchemaIdentifier_ = source->useSchemaIdentifier_;
	useLockTimeoutWaitTransactions_ = source->useLockTimeoutWaitTransactions_;
	quotedIdentifier_ = source->quotedIdentifier_;
	sensitiveIdentifier_ = source->sensitiveIdentifier_;
	autoQuotedIdentifier_ = source->autoQuotedIdentifier_;
	databaseAccess_ = source->databaseAccess_;
	// Phase 14.3: Copy transaction settings from source
	transactionIsolation_ = source->transactionIsolation_;
	transactionExtInit_ = source->transactionExtInit_;
	autoCommit_ = source->autoCommit_;
	admin_ = source->admin_;
	isRoles_ = source->isRoles_;
	twoPhaseTransactionHandle = source->twoPhaseTransactionHandle;
}

void IscConnection::init()
{
	useCount = 1;
	metaData = NULL;
	shareConnected = false;
	userEvents = NULL;
	useAppOdbcVersion = 3; // SQL_OV_ODBC3
	tmpParamTransaction = NULL;
	twoPhaseTransactionHandle = nullptr;
	GDS = NULL;
	databaseHandle = NULL;
}

IscConnection::~IscConnection()
{
	if (metaData)
		delete metaData;

	// Phase 14.2/14.3: Only detach if this connection owns the handle
	// (cloned connections share GDS/databaseHandle but don't own them)
	if ( ownsConnection_ )
	{
		// Phase 14.3: Destroy transaction first (RAII auto-rollback)
		transaction_.reset();
		delete nodeParamTransaction_;

		// Phase 14.2.2: Destroy fb-cpp Attachment (RAII auto-disconnect)
		// This replaces manual databaseHandle->detach()
		if ( attachment_ )
		{
			try { attachment_->disconnect(); } catch (...) {}
			attachment_.reset();
			databaseHandle = nullptr;
		}
		else if ( GDS && databaseHandle )
		{
			// Fallback: manual detach for connections not using fb-cpp Attachment
			ThrowStatusWrapper status( GDS->_status );
			try
			{
				databaseHandle->detach( &status );
				databaseHandle = nullptr;
			}
			catch( ... )
			{
				if( databaseHandle ) databaseHandle->release();
				databaseHandle = nullptr;
			}
		}

		if( GDS )
		{
			delete GDS;
			GDS = NULL;
		}
	}

	delete tmpParamTransaction;

	if ( userEvents )
		userEvents->release();
}

bool IscConnection::isConnected()
{
	return databaseHandle != nullptr;
}

void IscConnection::close()
{
	for (auto* statement : statements)
	{
		statement->close();
		statement->freeStatementHandle();
		statement->connection = NULL; // NOMEY
	}
	statements.clear();

	if ( shareConnected )
		connectionFromEnvShare();

	delete this;
}

PreparedStatement* IscConnection::prepareStatement(const char * sqlString)
{
	IscPreparedStatement *statement = NULL;

	try
	{
		statement = new IscPreparedStatement (this);
		statement->prepare (sqlString);
	}
	catch (...)
	{
		if (statement)
			delete statement;
		throw;
	}

	statements.push_back (statement);

	return statement;
}

UserEvents* IscConnection::prepareUserEvents( PropertiesEvents *context, callbackEvent astRoutine, void *userAppData )
{
	if ( userEvents )
		throw SQLEXCEPTION( APPLICATION_ERROR, "this is executed" );

	try
	{
		userEvents = new IscUserEvents( this, context, astRoutine, userAppData );
		// it's used App user and IscConnection
		// We have useCount == 2
		userEvents->addRef();
	}
	catch (...)
	{
		delete userEvents;
		throw;
	}

	return userEvents;
}

void IscConnection::commit()
{
	if ( !transaction_ || !transaction_->isValid() )
	{
		transactionPending_ = false;
		return;
	}

	try
	{
		transaction_->commit();
	}
	catch( const fbcpp::DatabaseException& e )
	{
		// On commit failure, rollback
		try { transaction_->rollback(); } catch (...) {}
		transaction_.reset();
		transactionPending_ = false;
		throw SQLEXCEPTION( RUNTIME_ERROR, e.what() );
	}

	transaction_.reset();
	transactionPending_ = false;
}

void IscConnection::rollback()
{
	if ( !transaction_ || !transaction_->isValid() )
	{
		transactionPending_ = false;
		return;
	}

	try
	{
		transaction_->rollback();
	}
	catch( const fbcpp::DatabaseException& e )
	{
		transaction_.reset();
		transactionPending_ = false;
		throw SQLEXCEPTION( RUNTIME_ERROR, e.what() );
	}

	transaction_.reset();
	transactionPending_ = false;
}

void IscConnection::prepareTransaction()
{
	if ( !transaction_ || !transaction_->isValid() )
		return;

	try
	{
		transaction_->prepare();
	}
	catch( const fbcpp::DatabaseException& e )
	{
		throw SQLEXCEPTION( RUNTIME_ERROR, e.what() );
	}
}

bool IscConnection::getTransactionPending()
{
	return transactionPending_;
}

void IscConnection::cancelOperation()
{
	if (databaseHandle)
	{
		ThrowStatusWrapper status(GDS->_status);
		try
		{
			databaseHandle->cancelOperation(&status, fb_cancel_raise);
		}
		catch (const FbException& /*ignored*/)
		{
			// cancelOperation may fail if the connection is idle or
			// if the operation already completed. Ignore errors.
		}
	}
}

Firebird::IAttachment* IscConnection::getHandleDb()
{	
	return databaseHandle;
}

Firebird::ITransaction* IscConnection::startTransaction()
{
	if ( shareConnected )
	{
		if ( !twoPhaseTransactionHandle )
		{
			getEnvironmentShareInstance().startTransaction();
			transactionPending_ = true;
		}

		return twoPhaseTransactionHandle;
	}

	// Return existing transaction if still active
	if ( transaction_ && transaction_->isValid() )
		return transaction_->getHandle().get();

	// Phase 14.3: Build TransactionOptions using fb-cpp's typed API
	fbcpp::TransactionOptions txnOpts;

	// Access mode
	if ( transactionExtInit_ & TRA_ro )
		txnOpts.setAccessMode( fbcpp::TransactionAccessMode::READ_ONLY );
	else
		txnOpts.setAccessMode( fbcpp::TransactionAccessMode::READ_WRITE );

	// Wait mode
	if ( transactionExtInit_ & TRA_nw )
		txnOpts.setWaitMode( fbcpp::TransactionWaitMode::NO_WAIT );
	else
		txnOpts.setWaitMode( fbcpp::TransactionWaitMode::WAIT );

	// Isolation level (SQL_TXN_ISOLATION_OPTION values)
	switch ( transactionIsolation_ )
	{
		case 0x00000008L:
			// SQL_TXN_SERIALIZABLE → Firebird CONSISTENCY
			txnOpts.setIsolationLevel( fbcpp::TransactionIsolationLevel::CONSISTENCY );
			break;

		case 0x00000004L:
			// SQL_TXN_REPEATABLE_READ → Firebird SNAPSHOT (concurrency)
			txnOpts.setIsolationLevel( fbcpp::TransactionIsolationLevel::SNAPSHOT );
			break;

		case 0x00000001L:
			// SQL_TXN_READ_UNCOMMITTED → Firebird READ_COMMITTED + NO_RECORD_VERSION
			txnOpts.setIsolationLevel( fbcpp::TransactionIsolationLevel::READ_COMMITTED );
			txnOpts.setReadCommittedMode( fbcpp::TransactionReadCommittedMode::NO_RECORD_VERSION );
			break;

		case 0x00000002L:
		default:
			// SQL_TXN_READ_COMMITTED → Firebird READ_COMMITTED + RECORD_VERSION
			txnOpts.setIsolationLevel( fbcpp::TransactionIsolationLevel::READ_COMMITTED );
			txnOpts.setReadCommittedMode( fbcpp::TransactionReadCommittedMode::RECORD_VERSION );
			break;
	}

	// Lock timeout (Firebird 2.0+ only, and only in WAIT mode)
	// fb-cpp TransactionOptions doesn't expose lock_timeout, so we add it via raw TPB
	if ( !(transactionExtInit_ & TRA_nw)
		&& isVersionAtLeast(2, 0)
		&& useLockTimeoutWaitTransactions_ )
	{
		// Build a raw TPB with lock_timeout using IXpbBuilder, then merge
		IUtil* utl = GDS->_master->getUtilInterface();
		ThrowStatusWrapper status( GDS->_status );
		IXpbBuilder* tpb = nullptr;
		try
		{
			tpb = utl->getXpbBuilder(&status, IXpbBuilder::TPB, NULL, 0);
			tpb->insertInt(&status, isc_tpb_lock_timeout, useLockTimeoutWaitTransactions_);

			auto bufLen = tpb->getBufferLength(&status);
			auto* buf = tpb->getBuffer(&status);
			txnOpts.setTpb( std::vector<std::uint8_t>( buf, buf + bufLen ) );

			tpb->dispose();
			tpb = nullptr;
		}
		catch( const FbException& error )
		{
			if( tpb ) tpb->dispose();
			THROW_ISC_EXCEPTION( this, error.getStatus() );
		}
	}

	// Create fb-cpp Transaction (starts the transaction via Firebird OO API)
	try
	{
		transaction_ = std::make_unique<fbcpp::Transaction>( *attachment_, txnOpts );
	}
	catch( const fbcpp::DatabaseException& e )
	{
		throw SQLEXCEPTION( -1, 0, e.what() );
	}

	if ( !autoCommit_ )
		transactionPending_ = true;

	return transaction_->getHandle().get();
}

Statement* IscConnection::createStatement()
{
	IscStatement *statement = new IscStatement (this);
	statements.push_back (statement);

	return statement;
}

InternalStatement* IscConnection::createInternalStatement()
{
	IscOdbcStatement *statement = new IscOdbcStatement (this);
	statements.push_back (statement);

	return statement;
}

Blob* IscConnection::genHTML(Properties * parameters, int genHeaders)
{
	NOT_YET_IMPLEMENTED;

	return NULL;
}

/***
void IscConnection::freeHTML(const char * html)
{
	delete [] (char*) html;
}
***/

int IscConnection::buildParamProcedure ( char *& string, int numInputParam )
{
	char * ptSrc = string;

	SKIP_WHITE ( ptSrc );

	if ( *ptSrc == '}' )
	{
		return 0;
	}

	if ( *ptSrc != '(' )
		return -1;

	if ( !numInputParam )
	{
		char * ptCh = ptSrc++; // '('

		while ( *ptSrc && *ptSrc != ')' )
			ptSrc++;

		if ( *ptSrc != ')' )
			return -1;

		ptSrc++; // ')'
		memmove(ptCh, ptSrc, strlen(ptSrc) + 1 );
		return 0;
	}

	ptSrc++; // '('

	int i = 0;
	bool nextParam = false;
	char * ptCh;

	while( *ptSrc && i < numInputParam )
	{
		SKIP_WHITE ( ptSrc );

		if ( *ptSrc == ')' )
		{
			return 0;
		}

		if ( *ptSrc == ',' )
		{
			if ( nextParam == true )
			{
				nextParam = false;
				ptSrc++;
			}
			else
			{
				i++;
				memmove(ptSrc + 1, ptSrc, strlen(ptSrc) + 1 );
				*ptSrc = '?';
				ptSrc += 2;
			}
			continue;
		}

		char delimiter;

		ptCh = ptSrc;

		if ( *ptCh == '\'' )
		{
			delimiter = *ptCh;
			++ptCh; // '\''
			ptSrc = ptCh;
			while ( *ptCh )
			{
				if ( *ptCh == delimiter )
				{
					if ( *(ptCh+1) == delimiter )
					{
						ptCh += 2;
						continue;
					}
					break;
				}
				++ptCh;
			}

			if ( *ptCh == delimiter )
				++ptCh;

			if ( *ptCh && *ptCh != ',' )
			{
				ptSrc = ptCh;
				i++;
				break;
			}

			if ( !*ptCh )
				break;

			nextParam = true;
		}
		else
		{
			delimiter = ',';
			ptSrc = ptCh;
			while ( *ptCh && *ptCh != delimiter && *ptCh != ')' )
				++ptCh;

			if ( *ptCh && *ptCh != delimiter )
			{
				ptSrc = ptCh;
				i++;
				break;
			}

			if ( !*ptCh )
				break;

			nextParam = false;
		}

		if( ptCh == ptSrc )
			ptSrc++;
		else
		{
			ptSrc = ptCh + 1;
			i++;
		}
	}

	if ( *(ptSrc-1) == ',' )
	{
		ptCh = --ptSrc;
		//  ok, it's output param

		while ( *ptCh && *ptCh != ')' )
			ptCh++;

		memmove(ptSrc, ptCh, strlen(ptCh) + 1 );
		string = ptSrc + 1;

		return 1; 
	}

	SKIP_WHITE(ptSrc);

	if ( *ptSrc == ')' )
	{
		return 0;
	}

// error query
	return -1;
}

#define IS_MATCH_EXT(token)	isMatchExt( ptOut, token, sizeof ( token ) - 1 )

inline
bool IscConnection::isMatchExt( char *& string, const char *keyWord, const int length )
{
	if ( ( length == 1 && *string == *keyWord )
		|| ( !strncasecmp( string, keyWord, length ) && IS_END_TOKEN( *(string + length) ) ) )
	{
		string += length;
		SKIP_WHITE ( string );
		return true;
	}
	return false;
}

bool IscConnection::paramTransactionModes( char *& string, short &transFlags, bool expectIsolation )
{
	char *& ptOut = string;

	if ( IS_MATCH_EXT( "READ" ) ) 
	{
		if ( IS_MATCH_EXT( "ONLY" ) ) 
		{
			if ( expectIsolation )
				throw SQLEXCEPTION( SYNTAX_ERROR, "after SNAPSHOT not ONLY" );

			transFlags |= TRA_ro;
			return true;
		}
		else if ( IS_MATCH_EXT( "WRITE" ) ) 
		{
			if ( expectIsolation )
				throw SQLEXCEPTION( SYNTAX_ERROR, "after SNAPSHOT not WRITE" );

			return true;
		}

		if ( !( IS_MATCH_EXT( "COMMITTED" ) || IS_MATCH_EXT( "UNCOMMITTED" ) ) )
			throw SQLEXCEPTION( SYNTAX_ERROR, "should be keyword COMMITTED or UNCOMMITTED" );
	
		transFlags |= TRA_read_committed;

		if ( IS_MATCH_EXT( "NO" ) ) 
		{
			if ( IS_MATCH_EXT( "RECORD_VERSION" ) )
			{
				transFlags |= TRA_no_rec_version;
				return true;
			}
			else if ( IS_MATCH_EXT( "WAIT" ) ) 
			{
				transFlags |= TRA_nw;
				return true;
			}

			throw SQLEXCEPTION( SYNTAX_ERROR, "should be keyword WAIT or VERSION" );
		}

		IS_MATCH_EXT( "RECORD_VERSION" );
		return true;
	}
	else if ( IS_MATCH_EXT( "SNAPSHOT" ) ) 
	{
		if ( IS_MATCH_EXT( "TABLE" ) ) 
		{
			transFlags |= TRA_con;

			IS_MATCH_EXT( "STABILITY" );
		}
		return true;
	}
	else if ( IS_MATCH_EXT( "REPEATABLE" ) )
	{
		if ( IS_MATCH_EXT( "READ" ) ) 
		{
			transFlags |= TRA_con;
			return true;
		}
		throw SQLEXCEPTION( SYNTAX_ERROR, "should be keyword READ after REPEATABLE" );
	}
	else if ( IS_MATCH_EXT( "SERIALIZABLE" ) )
	{
		return true;
	}

	return false;
}

void IscConnection::parseReservingTable( char *& string, char *& tpbBuffer, short transFlags )
{
	char *saveLockMode[256];
	char *saveLockLevel[256];
	int countTable = 0;
	char lockMode = 0;
	char lockLevel = 0;
	char *& ptOut = string;
	char *beg = tpbBuffer + 2;
	char *end;
	char quote;
	char delimiter = *metaData->getIdentifierQuoteString();
	delimiter = delimiter == ' ' || databaseDialect_ < 3 ? 0 : delimiter;
	bool autoQuoted = delimiter && autoQuotedIdentifier_;

	while ( true )
	{
		saveLockMode[countTable] = beg - 2;
		char &lengthTableName = *(beg - 1);

		end = beg;

		if ( IS_QUOTE( *ptOut ) )
		{
			quote = *ptOut++;
			while ( *ptOut != '\0' && *ptOut != quote )
				*end++ = *ptOut++;

			if ( *ptOut == '\0' )
				throw SQLEXCEPTION( SYNTAX_ERROR, "missing closing quote for identifier" );
			else
				ptOut++;
		}
		else
		{
			bool mixed = false;

			if (autoQuoted)
			{
				const char* pt = ptOut;
				bool hasUpper = false;
				bool hasLower = false;

				while ( IS_IDENT( *pt ) && !mixed )
				{
					hasUpper |= ISUPPER( *pt );
					hasLower |= ISLOWER( *pt );
					mixed = hasUpper && hasLower;
					pt++;
				}
			}

			if (mixed)
			{
				while ( IS_IDENT( *ptOut ) )
					*end++ = *ptOut++;
			}
			else
			{
				while ( IS_IDENT( *ptOut ) )
				{
					// UPPER uses the argument two times - therefore the pointer inc is in a separate line
					*end++ = UPPER( *ptOut );
					ptOut++;
				}
			}
		}

		lengthTableName = end - beg;

		SKIP_WHITE ( ptOut );
    	//		SYNTAX_ERROR ("relation name");

		saveLockLevel[countTable++] = end++;


		if ( !( IS_MATCH_EXT( "," ) ) )
		{
			IS_MATCH_EXT( "FOR" );
			lockLevel = (transFlags & TRA_con) ? isc_tpb_protected : isc_tpb_shared;
			lockMode = isc_tpb_lock_read;

			if ( IS_MATCH_EXT( "PROTECTED" ) )
				lockLevel = isc_tpb_protected;
			else if ( IS_MATCH_EXT( "EXCLUSIVE" ) )
				lockLevel = isc_tpb_exclusive;
			else if ( IS_MATCH_EXT( "SHARED" ) )
				lockLevel = isc_tpb_shared;

			if ( IS_MATCH_EXT( "WRITE" ) )
			{
				if ( transFlags & TRA_ro )
					throw SQLEXCEPTION( SYNTAX_ERROR, "write lock requested for a read_only transaction" );

				lockMode = isc_tpb_lock_write;
			}
			else 
				IS_MATCH_EXT( "READ" );

			if ( countTable )
			{
		    	//
		    	// get the lock level and mode and apply them to all the
		    	// relations in the list
		    	//
				do {
			    	*saveLockLevel[--countTable] = lockLevel;
			    	*saveLockMode[countTable] = lockMode;
        		} while ( countTable );
			}

			if ( !( IS_MATCH_EXT( "," ) ) )
				break;
    	}


		beg = end + 2;
  	}

	tpbBuffer = end;

}

int IscConnection::buildParamTransaction( char *& string, char boolDeclare )
{
	CNodeParamTransaction node;
	bool localParamTransaction = false;
	char *& ptOut = string;
	short transFlags = 0;
	int ret = 0;

	SKIP_WHITE ( ptOut );

	if ( IS_MATCH_EXT( "LOCAL" ) )
		localParamTransaction = true;

	if ( IS_MATCH_EXT( "NAME" ) )
	{
		int &len = node.lengthNameTransaction = 0;
		char *end = node.nameTransaction;

		while ( !IS_END_TOKEN( *ptOut ) && len < node.getMaxLengthName() )
			*end++ = *ptOut++, len++;

		SKIP_WHITE ( ptOut );

		if ( IS_MATCH_EXT( "USING" ) )
		{
			transFlags |= TRA_inc;
			int &len = node.lengthNameUnique = 0;
			char *end = node.nameUnique;

			while ( !IS_END_TOKEN( *ptOut ) && len < node.getMaxLengthName() )
				*end++ = *ptOut++, len++;
		}

		if ( !*ptOut || IS_MATCH_EXT( "GO" ) )
		{
			if ( boolDeclare )
				throw SQLEXCEPTION( SYNTAX_ERROR, "bad declare param transaction" );

			// find ParamTransaction from Environment
			if ( node.lengthNameTransaction )
			{
				if ( getEnvironmentShareInstance().findParamTransactionFromList( node ) )
				{
					if ( localParamTransaction )
					{
						if ( !tmpParamTransaction )
							tmpParamTransaction = new CNodeParamTransaction;

						*tmpParamTransaction = node;
						ret = -4; // for local stmt = -4
					}
					else
					{
						if ( !nodeParamTransaction_ )
							nodeParamTransaction_ = new CNodeParamTransaction;

						*nodeParamTransaction_ = node;
						ret = -5; // for connection  = -5
					}

					return ret;
				}
			}
			throw SQLEXCEPTION( SYNTAX_ERROR, "transaction name not found" );
		}
	}

	while ( true )
	{
		if ( IS_MATCH_EXT( "ISOLATION" ) )
		{
			IS_MATCH_EXT( "LEVEL" );

			if ( !paramTransactionModes( ptOut, transFlags, true ) )
				throw SQLEXCEPTION( SYNTAX_ERROR, "should be keyword SNAPSHOT" );

			continue;
		}

		if ( paramTransactionModes( ptOut, transFlags, false ))
			continue;

		if ( IS_MATCH_EXT( "NO" ) )
		{
			if ( IS_MATCH_EXT( "WAIT" ) )
			{
				transFlags |= TRA_nw;
				continue;
			}
			
			if ( IS_MATCH_EXT( "RECORD_VERSION" ) )
				throw SQLEXCEPTION( SYNTAX_ERROR, "NO RECORD_VERSION use only with READ COMMITTED" );

			throw SQLEXCEPTION( SYNTAX_ERROR, "should be keyword WAIT" );
		}

		if ( IS_MATCH_EXT( "RECORD_VERSION" ) )
			throw SQLEXCEPTION( SYNTAX_ERROR, "RECORD_VERSION use only with READ COMMITTED" );

		if ( IS_MATCH_EXT( "WAIT" ) )
		{
			if ( IS_MATCH_EXT( "LOCK" ) )
			{
				if ( !IS_MATCH_EXT( "TIMEOUT" ) )
					throw SQLEXCEPTION( SYNTAX_ERROR, "should be keyword LOCK TIMEOUT" );

				char buffer[MAX_SMALLINT_LENGTH + 1];
				int lenBuffer = 0;
				char *end = buffer;

				while ( !IS_END_TOKEN( *ptOut )
						&& ISDIGIT( *ptOut )
						&& lenBuffer < MAX_SMALLINT_LENGTH )
				{
					*end++ = *ptOut++;
					lenBuffer++;
				}

				if ( !IS_END_TOKEN( *ptOut ) )
					throw SQLEXCEPTION( SYNTAX_ERROR, "should be keyword LOCK TIMEOUT <value>" );

				*end = '\0';
				node.lockTimeout = atoi( buffer );
			}
			continue;
		}
                                          
		if ( IS_MATCH_EXT( "AUTOCOMMIT" ) )
		{
			transFlags |= TRA_autocommit;
			continue;
		}

		if ( IS_MATCH_EXT( "NO_AUTO_UNDO" ) )
		{
			transFlags |= TRA_no_auto_undo;
			continue;
		}
		break;
	}

	// Phase 9.5: Build TPB using IXpbBuilder instead of manual byte-stuffing.
	// IXpbBuilder handles version prefix, endianness for lock_timeout, etc.
	IUtil* utl = GDS->_master->getUtilInterface();
	IXpbBuilder* tpb = nullptr;
	ThrowStatusWrapper status( GDS->_status );
	try
	{
		tpb = utl->getXpbBuilder(&status, IXpbBuilder::TPB, NULL, 0);

		tpb->insertTag( &status, (transFlags & TRA_ro) ? isc_tpb_read : isc_tpb_write );

		if (transFlags & TRA_con)
			tpb->insertTag( &status, isc_tpb_consistency );
		else if (transFlags & TRA_read_committed)
			tpb->insertTag( &status, isc_tpb_read_committed );
		else
			tpb->insertTag( &status, isc_tpb_concurrency );

		if ( transFlags & TRA_nw )
			tpb->insertTag( &status, isc_tpb_nowait );
		else
		{
			tpb->insertTag( &status, isc_tpb_wait );

			if ( node.lockTimeout && isVersionAtLeast(2, 0) )
			{
				// IXpbBuilder handles endianness for the lock_timeout integer
				tpb->insertInt( &status, isc_tpb_lock_timeout, node.lockTimeout );
			}
		}

		if ( transFlags & TRA_read_committed )
		{
			tpb->insertTag( &status, (transFlags & TRA_no_rec_version) ?
				isc_tpb_no_rec_version : isc_tpb_rec_version );
		}

		if (transFlags & TRA_no_auto_undo)
			tpb->insertTag( &status, isc_tpb_no_auto_undo );

		// For RESERVING, extract the IXpbBuilder buffer into tpbBuffer
		// and let parseReservingTable append raw table-lock entries.
		char tpbBuffer[4096];
		unsigned builderLen = tpb->getBufferLength( &status );
		const unsigned char* builderBuf = tpb->getBuffer( &status );
		if (builderLen > sizeof(tpbBuffer))
			throw SQLEXCEPTION( RUNTIME_ERROR, "TPB buffer overflow" );
		memcpy( tpbBuffer, builderBuf, builderLen );
		char* text = tpbBuffer + builderLen;

		tpb->dispose();
		tpb = nullptr;

		if ( IS_MATCH_EXT( "RESERVING" ) )
		{
			transFlags |= TRA_rrl;
			parseReservingTable( ptOut, text, transFlags );
		}
	
		if ( IS_MATCH_EXT( "USING" ) )
		{
			transFlags |= TRA_inc;
			int &len = node.lengthNameUnique = 0;
			char *end = node.nameUnique;

			while ( !IS_END_TOKEN( *ptOut ) && len < node.getMaxLengthName() )
				*end++ = *ptOut++, len++;
		}

		int tpb_len = text - tpbBuffer;

		if ( tpb_len > 0 )
		{
			if ( transFlags & TRA_autocommit )
				node.autoCommit = true;

			node.setTpbBuffer( tpbBuffer, tpb_len );

			if ( boolDeclare )
			{
				if ( !node.lengthNameTransaction )
				{
					if ( !localParamTransaction )
						throw SQLEXCEPTION( SYNTAX_ERROR, "bad declare param transaction" );

					if ( !tmpParamTransaction )
						tmpParamTransaction = new CNodeParamTransaction;

					*tmpParamTransaction = node;
					ret = -7; // declare for local stmt = -7
				}

				if ( node.lengthNameTransaction )
				{
					getEnvironmentShareInstance().addParamTransactionToList( node );
					ret = -6; // for all connections  = -6 it's named param Transaction
				}
			}
			else if ( localParamTransaction )
			{
				if ( !tmpParamTransaction )
					tmpParamTransaction = new CNodeParamTransaction;

				*tmpParamTransaction = node;
				ret = -4; // for local stmt = -4
			}
			else
			{
				ret = -5; // for connection  = -5

				if ( !nodeParamTransaction_ )
					nodeParamTransaction_ = new CNodeParamTransaction;

				*nodeParamTransaction_ = node;

				if ( node.lengthNameTransaction )
				{
					getEnvironmentShareInstance().addParamTransactionToList( node );
					ret = -6; // for all connections  = -6 it's named param Transaction
				}
			}
		}
	}
	catch (const FbException& error)
	{
		if (tpb) tpb->dispose();
		THROW_ISC_EXCEPTION(this, error.getStatus());
	}

	return ret;
}

class CSchemaIdentifier
{
public:
	char	*stringSql;
	bool	deleteNode;
	bool	quotedNode;
	int		begNameNode;
	short	lengthNameNode;

	CSchemaIdentifier()
	{
		remove();
	}
	void remove()
	{ 
		stringSql = NULL;
		deleteNode = false;
		quotedNode = false;
		begNameNode = 0;
		lengthNameNode = 0;
	}
	CSchemaIdentifier & operator =(const CSchemaIdentifier & src)
	{ 
		stringSql = src.stringSql;
		deleteNode = src.deleteNode;
		quotedNode = src.quotedNode;
		begNameNode = src.begNameNode;
		lengthNameNode = src.lengthNameNode;
		return  *this;
	}
};

typedef MList<CSchemaIdentifier> ListSchemaIdentifier;

bool IscConnection::removeSchemaFromSQL( char *strSql, int lenSql, char *strSqlOut, int &lenSqlOut )
{
	ListSchemaIdentifier listSchemaIdentifierAll;
	ListSchemaIdentifier listSchemaIdentifierTbl;
	int countNodesShema = 0;
	int countTblNodesShema = 0;
	int statysModify = 0;
	int statusQuote = 0;
	char *beg = strSql;
	char *ptIn = strSql;
	char *ptInEnd = strSql + lenSql;
	char *ptOut = strSqlOut;
	char quote;
	char chUpper;
	bool success = true;
	bool defTable = false;

	lenSqlOut = lenSql;

	SKIP_WHITE ( ptIn );

	chUpper = UPPER( *ptIn );

	if ( ( chUpper == 'S' && !IS_MATCH( ptIn, "SELECT" ) )
		|| ( chUpper == 'U' && !IS_MATCH( ptIn, "UPDATE" ) )
		|| ( chUpper == 'I' && !IS_MATCH( ptIn, "INSERT" ) )
		|| ( chUpper == 'D' && !IS_MATCH( ptIn, "DELETE" ) ) )
		return false;

	while ( ptIn < ptInEnd )
	{
		if ( !statusQuote )
		{
			chUpper = UPPER( *ptIn );

			switch ( chUpper )
			{
			case 'S':
				if ( IS_MATCH( ptIn, "SELECT" ) )
				{
					ptIn += 6;
					defTable = false;
				}
				else if ( IS_MATCH( ptIn, "SET" ) )
				{
					ptIn += 3;
					defTable = false;
				}
				break;

			case 'I':
				if ( IS_MATCH( ptIn, "INSERT" ) )
				{
					ptIn += 6;
					defTable = true;
				}
				break;

			case 'U':
				if ( IS_MATCH( ptIn, "UPDATE" ) )
				{
					ptIn += 6;
					defTable = true;
				}
				break;

			case 'F':
				if ( IS_MATCH( ptIn, "FROM" ) )
				{
					ptIn += 4;
					defTable = true;
				}
				break;

			case 'V':
				if ( IS_MATCH( ptIn, "VALUES" ) )
				{
					ptIn += 6;
					defTable = false;
				}
				break;

			case 'W':
				if ( IS_MATCH( ptIn, "WHERE" ) )
				{
					ptIn += 5;
					defTable = false;
				}
				break;

			case 'O':
				if ( IS_MATCH( ptIn, "ON" ) )
				{
					ptIn += 2;
					defTable = false;
				}
				break;

			case '.':
				if ( (*ptIn + 1) != '*' )
				{
					do
					{
						bool digit = true;
						char quoteTmp = 0;
						char *pt = ptIn - 1;

						if ( IS_QUOTE( *pt ) )
						{
							quoteTmp = *pt--;
							digit = false;
							while ( pt >= beg && IS_IDENT( *pt ) )
							{
								--pt;
							}
							if ( *pt != quoteTmp )
							{
								success = false;
								break;
							}
						}
						else
						{
							while ( pt >= beg && IS_IDENT( *pt ) )
							{
								if ( digit && !ISDIGIT( *pt ) )
									digit = false;
								--pt;
							}

							++pt;
						}

						if ( !digit )
						{
							bool deleteNode = false;
							char *ptEnd = pt;

							pt = ptIn + 1;

							while ( !(IS_END_TOKEN( *pt )) )
							{
								if ( IS_POINT( *pt ) && !deleteNode )
								{
									deleteNode = true;

									if ( defTable )
										ptIn = pt;
								}
								++pt;
							}

							CSchemaIdentifier &node = listSchemaIdentifierAll( countNodesShema++ );

							node.stringSql = strSql;
							node.deleteNode = deleteNode;
							node.quotedNode = !!quoteTmp;
							node.begNameNode = ptEnd - beg;
							node.lengthNameNode = ptIn - ptEnd;

							if ( defTable )
							{
								CSchemaIdentifier &nodeDef = listSchemaIdentifierTbl( countTblNodesShema++ );
								nodeDef = node;
								node.deleteNode = true;
							}

							ptIn = pt;
						}

					} while ( false );
				}
				break;

			case '\"':
			case '\'':
				quote = *ptIn;
				statusQuote ^= 1;
				break;
			}
		}
		else if ( quote == *ptIn )
		{
			quote = 0;
			statusQuote ^= 1;
		}

		++ptIn;
	}

	if ( countTblNodesShema )
	{
		int offset = 0;
		int offsetNode = 0;
		int length;

		CSchemaIdentifier *node = listSchemaIdentifierAll.GetRoot();

		while( countNodesShema-- )
		{
			bool itsDdelete = false;

			length = node->begNameNode - offsetNode;
			memcpy( &ptOut[offset], &beg[offsetNode], length );
			offset += length;
			offsetNode += length;
			
			if ( node->deleteNode )
				itsDdelete = true;
			else 
			{
				int countTbl = countTblNodesShema;

				CSchemaIdentifier *nodeTbl = listSchemaIdentifierTbl.GetRoot();

				while( countTbl-- )
				{
					if ( node->lengthNameNode == nodeTbl->lengthNameNode 
						&& !strncasecmp( &beg[node->begNameNode],
										 &beg[nodeTbl->begNameNode],
										 node->lengthNameNode ) )
					{
						itsDdelete = true;
						break;
					}

					nodeTbl++;
				}
			}

			if ( itsDdelete )
				offsetNode += node->lengthNameNode + 1;

			node++;
		}

		if ( lenSql > offsetNode )
		{
			length = lenSql - offsetNode;
			memcpy( &ptOut[offset], &beg[offsetNode], lenSql - offsetNode );
			offset += length;
		}

		ptOut[offset] = '\0';
		lenSqlOut = offset;
	}
	else
	{
		lenSqlOut = lenSql;
		memcpy( ptOut, beg, lenSqlOut );
		ptOut[lenSqlOut] = '\0';
	}

	return success;
}

int IscConnection::getNativeSql (const char * inStatementText, int textLength1,
								char * outStatementText, int bufferLength,
								int * textLength2Ptr)
{
	int statysModify = 0;
	char * ptIn = (char*)inStatementText;
	char * ptInEnd = ptIn + textLength1;
	char * ptOut = outStatementText;
	int statusQuote = 0;
	int statusBracket = 0;
	char quote;
	char delimiter = *metaData->getIdentifierQuoteString();
	delimiter = delimiter == ' ' || databaseDialect_ < 3 ? 0 : delimiter;

	bool autoRemoveSchemaFromIdentifier = useSchemaIdentifier_ == 1;
	bool autoQuoted = delimiter && autoQuotedIdentifier_;

	// Schema removal + auto-quoting of mixed-case identifiers.
	// Bracket-to-space conversion for "(SELECT ...) UNION (SELECT ...)" is intentional.
	if ( autoRemoveSchemaFromIdentifier )
	{
		statysModify = removeSchemaFromSQL( ptIn, textLength1, ptOut, *textLength2Ptr );

		if ( statysModify )
		{
			ptIn = ptOut;
			textLength1= *textLength2Ptr;
			ptInEnd = ptIn + textLength1;
		}
	}

	SKIP_WHITE ( ptIn );

	bool externalBracket = *ptIn == '(';

	while ( ptIn < ptInEnd )
	{
		if ( !statusQuote )
		{
			if ( IS_QUOTE( *ptIn ) )
			{
				quote = *ptIn;
				statusQuote ^= 1;
			}
			else if ( autoQuoted && IS_IDENT ( *ptIn ) )
			{
				bool mixed = false;
				char * pt = ptIn;
				
				if ( ISUPPER ( *ptIn ) )
				{
					while ( IS_IDENT ( *pt ) )
					{
						if ( ISLOWER ( *pt ) )
						{
							mixed = true;
							break;
						}
						pt++;
					}
				}
				else
				{
					while ( IS_IDENT ( *pt ) )
					{
						if ( ISUPPER ( *pt ) )
						{
							mixed = true;
							break;
						}
						pt++;
					}
				}

				if ( mixed )
				{
					*ptOut++ = delimiter;
					 
					while ( IS_IDENT ( *ptIn ) )
						*ptOut++ = *ptIn++;

					*ptOut++ = delimiter;
					statysModify++;
				}
				else
					while ( IS_IDENT ( *ptIn ) )
						*ptOut++ = *ptIn++;
				continue;
			}
			else if ( externalBracket )	// Probably it "(select ...) union (select ...)"
			{							// convert to  " select ...  union  select ... "
				if ( !statusBracket )
				{
					while ( IS_WHITE ( *ptIn ) )
						*ptOut++ = *ptIn++;

					if ( *ptIn == '(' )
					{
						*ptIn = ' ';
						++statusBracket;
						++statysModify;
					}
					else
						externalBracket = false;
				}
				else
				{
					if ( *ptIn == ')' )
					{
						if ( !--statusBracket )
						{
							*ptIn = ' ';
							++statysModify;
							externalBracket = false;

							while ( IS_WHITE ( *ptIn ) )
								*ptOut++ = *ptIn++;

							if ( IS_MATCH( ptIn, "UNION" ) )
							{
								externalBracket = true;

								while ( IS_IDENT ( *ptIn ) )
									*ptOut++ = *ptIn++;
							}
							continue;
						}
					}
				}
			}
		}
		else if ( quote == *ptIn )
		{
			quote = 0;
			statusQuote ^= 1;
		}

		*ptOut++ = *ptIn++;
	}

	if ( statusQuote ) // There is no '"' or '\'' a syntactic mistake
	{
		if ( textLength2Ptr )
			*textLength2Ptr = textLength1;
		return statysModify;
	}

	*ptOut = '\0';

	{
		if ( textLength2Ptr )
			*textLength2Ptr = ptOut - outStatementText;

		ptOut = outStatementText;

		SKIP_WHITE ( ptOut );

		if ( IS_MATCH( ptOut, "COMMIT" ) )
			statysModify = -1;
		else if ( IS_MATCH( ptOut, "ROLLBACK" ) )
			statysModify = -2;
		else if ( IS_MATCH_EXT( "SET" ) )
		{
			if ( IS_MATCH_EXT( "TRANSACTION" ) )
			{
				// for local stmt = -4
				// for connection  = -5
				// for all connections  = -6 it's named param Transaction
				statysModify = buildParamTransaction( ptOut );
			}
		}
		else if ( IS_MATCH_EXT( "DECLARE" ) )
		{
			char boolDeclare = true;

			if ( IS_MATCH_EXT( "TRANSACTION" ) )
			{
				// for local stmt = -4
				// for connection  = -5
				// for all connections  = -6 it's named param Transaction
				statysModify = buildParamTransaction( ptOut, boolDeclare );
			}
		}
		else if ( IS_MATCH( ptOut, "CREATE" ) || IS_MATCH( ptOut, "ALTER" ) )
		{
			bool bContinue = true;

			if ( UPPER(*ptOut) == 'A' )
				ptOut += 5;
			else
			{
				ptOut += 6;

				SKIP_WHITE ( ptOut );

				if ( IS_MATCH( ptOut, "DATABASE" ) )
				{
					statysModify = -3;
					bContinue = false;
				}
			}

			if ( bContinue )
			{
				while ( *ptOut )
				{
					SKIP_WHITE ( ptOut );

					if ( *ptOut )
					{
						if ( IS_MATCH( ptOut, "TINYINT" ) )
						{
							const char * nameTinyint = "CHAR CHARACTER SET OCTETS";
							int lenNameTinyint = 25;
							int offset = lenNameTinyint - TOKEN_LENGTH( "TINYINT" );

							memmove(ptOut + offset, ptOut, strlen(ptOut) + 1 );
							memcpy(ptOut, nameTinyint, lenNameTinyint);
							ptOut += lenNameTinyint;
						}
						else 
							SKIP_NO_WHITE ( ptOut );
					}
				}

				if ( textLength2Ptr )
					*textLength2Ptr = ptOut - outStatementText;
			}
		}
	}
	// ODBC escape sequences ({fn ...}, {d ...}, {ts ...}, {oj ...}, {CALL ...})
	// are NOT processed. SQL is sent to Firebird as-is for maximum performance
	// and transparency. Applications should use native Firebird SQL syntax.

	return statysModify;
}

bool IscConnection::getCountInputParamFromProcedure ( const char* procedureName, int &numIn, int &numOut, bool &canSelect )
{
	bool ret = false; // not found
	numIn = numOut = 0;
	canSelect = false;

	IscProceduresResultSet resultSet ( (IscDatabaseMetaData *)getMetaData() );
	resultSet.addBlr = true;
	resultSet.getProcedures ( NULL, NULL, procedureName );

	if ( resultSet.getCountRowsStaticCursor() )
	{
		numIn = resultSet.sqlda->getShort(4); // NUM_INPUT_PARAM
		numOut = resultSet.sqlda->getShort(5); // NUM_OUTPUT_PARAM
		if ( numOut )
			canSelect = resultSet.canSelectFromProcedure();
		ret = true;
	}

	return ret;
}

DatabaseMetaData* IscConnection::getMetaData()
{
	if (metaData)
		return metaData;

	metaData = new IscDatabaseMetaData (this);

	return metaData;
}

int IscConnection::getConnectionCharsetCode()
{
	return charsetCode_;
}

WCSTOMBS IscConnection::getConnectionWcsToMbs()
{
	return adressWcsToMbs( charsetCode_ );
}

MBSTOWCS IscConnection::getConnectionMbsToWcs()
{
	return adressMbsToWcs( charsetCode_ );
}

int IscConnection::hasRole(const char * schemaName, const char * roleName)
{
	NOT_YET_IMPLEMENTED;

	return false;
}

bool IscConnection::ping()
{
	if (databaseHandle)
	{
		CheckStatusWrapper status(GDS->_status);
		databaseHandle->ping(&status);
		return (status.getState() & IStatus::STATE_ERRORS) == 0;
	}

	return false;
}

void IscConnection::sqlExecuteCreateDatabase(const char * sqlString)
{
	ThrowStatusWrapper status( GDS->_status );
	IAttachment* newdb = nullptr;
	try
	{
		newdb =
			GDS->_master->getUtilInterface()->executeCreateDatabase( &status, strlen(sqlString), sqlString, SQL_DIALECT_CURRENT, nullptr );
		newdb->detach( &status );
		newdb = nullptr;
	}
	catch( const FbException& error )
	{
		if( newdb ) newdb->release();
		const ISC_STATUS * statusVector = error.getStatus()->getErrors();
		throw SQLEXCEPTION ( GDS->getSqlCode( statusVector ), statusVector [1], getIscStatusText( error.getStatus() ) );
	}
}

// Helper: load the Firebird client library if not already loaded.
void IscConnection::loadClientLibrary(Properties *properties)
{
	const char *clientDefault = NULL;
	const char *client = properties->findValue ("client", NULL);

	if ( !client || !*client )
#if defined (_WINDOWS)
		client = "gds32.dll",
		clientDefault = "fbclient.dll";
#elif defined (__APPLE__)
		client = "libgds.dylib",
		clientDefault = "libfbclient.dylib";
#else
		client = "libgds.so",
		clientDefault = "libfbclient.so";
#endif

	GDS = new CFbDll();
	if ( !GDS->LoadDll (client, clientDefault) )
	{
		JString text;
		text.Format ("Unable to connect to data source: library '%s' failed to load", client);
		throw SQLEXCEPTION( -904, 335544375l, text );
	}
}

// Helper: check if user is admin.
void IscConnection::checkAdmin()
{
	QUAD adm1 = (QUAD)71752869960019.0;
	QUAD adm2 = (QUAD)107075219978611.0;
	QUAD user = (QUAD)0;
	memcpy((void *)&user,(const char *)userName_,6);

	admin_ = user == adm1 || user == adm2;

	if ( admin_ )
	{
		userAccess_ = "";
		userType_ = 0;
	}
}

static char databaseInfoItems [] = { 
	isc_info_db_id,
	isc_info_db_sql_dialect,
	isc_info_base_level,
	isc_info_ods_version,
	isc_info_firebird_version,
	isc_info_version, 
	isc_info_page_size,
	isc_info_user_names,
	isc_info_end 
};

void IscConnection::createDatabase(const char * dbName, Properties * properties)
{
	try
	{
		if ( !GDS )
			loadClientLibrary( properties );

		char sql[1024];
		char *p = sql;

		p += sprintf( p, "CREATE DATABASE \'%s\' ", dbName );

		const char *dialect = properties->findValue ("dialect", NULL);

		if ( dialect && *dialect == '1')
			databaseDialect_ = SQL_DIALECT_V5;
		else
			databaseDialect_ = SQL_DIALECT_V6;

		const char *user = properties->findValue ("user", NULL);

		if (user && *user)
			p += sprintf( p, "USER \'%s\' ", user );

		const char *password = properties->findValue ("password", NULL);

		if (password && *password)
			p += sprintf( p, "PASSWORD \'%s\' ", password );

		const char *pagesize = properties->findValue ("pagesize", NULL);

		if (pagesize && *pagesize)
			p += sprintf( p, "PAGE_SIZE %s ", pagesize );

		const char *charset = properties->findValue ("charset", NULL);
		if ( !charset )
			charset = properties->findValue ("characterset", NULL);

		if (charset && *charset)
			p += sprintf( p, "DEFAULT CHARACTER SET %s ", charset );

		*p = '\0';
	
		ThrowStatusWrapper status( GDS->_status );
		databaseHandle =
			GDS->_master->getUtilInterface()->executeCreateDatabase( &status, strlen(sql), sql, databaseDialect_, nullptr );
		ownsConnection_ = true;
	}
	catch ( SQLException& exception )
	{
		GDS = NULL;
		throw SQLEXCEPTION ( (SqlCode)exception.getSqlcode() , exception.getText() );
	}
	catch ( const FbException& error )
	{
		const ISC_STATUS * statusVector = error.getStatus()->getErrors();
		throw SQLEXCEPTION ( GDS->getSqlCode( statusVector ), statusVector [1], getIscStatusText( error.getStatus() ) );
	}
	catch (...)
	{
		GDS = NULL;
		throw;
	}
}

void IscConnection::openDatabase(const char * dbName, Properties * properties)
{
	try
	{
		if ( !GDS )
			loadClientLibrary( properties );

		const char *dialect = properties->findValue ("dialect", NULL);

		isRoles_ = false;
		databaseName_ = dbName;
		const char emptyStr[] = "";
		std::stringstream dpb_config;

		IUtil* utl = GDS->_master->getUtilInterface();
		ThrowStatusWrapper throw_status( GDS->_status );
		IXpbBuilder* dpb = nullptr;

		try
		{
			dpb = utl->getXpbBuilder(&throw_status, IXpbBuilder::DPB, NULL, 0);

			const char *user = properties->findValue ("user", NULL);

			if (user && *user)
			{
				userName_ = user;
				userAccess_ = user;
				userType_ = 8;
				dpb->insertString( &throw_status, isc_dpb_user_name, user );
			}
			else
			{
				dpb->insertString( &throw_status, isc_dpb_user_name, emptyStr );
			}

			const char *password = properties->findValue ("password", NULL);

			if (password && *password)
			{
				dpb->insertString( &throw_status, isc_dpb_password, password );
			}
			else
			{
				dpb->insertString( &throw_status, isc_dpb_password, emptyStr );
			}

			const char *timeout = properties->findValue ("timeout", NULL);

			if (timeout && *timeout)
			{
				connectionTimeout_ = atoi(timeout);
				dpb->insertInt(&throw_status, isc_dpb_connect_timeout, connectionTimeout_);
			}

			const char *role = properties->findValue ("role", NULL);

			if (role && *role)
			{
				userAccess_ = role;

				char *ch = (char *)(const char *)userAccess_;
				while ( (*ch = UPPER ( *ch )) )
					++ch;

				userType_ = 13;
				isRoles_ = true;

				dpb->insertString(&throw_status, isc_dpb_sql_role_name, role);
			}

			const char *charset = properties->findValue ("charset", NULL);
			if ( !charset )
				charset = properties->findValue ("characterset", NULL);

			// Phase 12 (12.4.1): Default to UTF8 when no charset specified.
			if (!charset || !*charset)
				charset = "UTF8";

			dpb->insertString(&throw_status, isc_dpb_lc_ctype, charset);
			charsetCode_ = findCharsetsCode( charset );

			const char *property = properties->findValue ("databaseAccess", NULL);

			if ( property )
				databaseAccess_ = (int)(*property - '0');
			else
				databaseAccess_ = 0;

			const char* enable_compat_bind = properties->findValue("EnableCompatBind", "Y");
			if( *enable_compat_bind == 'Y' )
			{
				const char* bind_cmd = properties->findValue("SetCompatBind", NULL);
				dpb->insertString(&throw_status, isc_dpb_set_bind,
					(bind_cmd && *bind_cmd) ? bind_cmd : "int128 to varchar;decfloat to legacy;time zone to legacy");
			}

			const char* enable_wire_compression = properties->findValue("EnableWireCompression", "N");
			if (*enable_wire_compression == 'Y')
			{
				dpb_config << "WireCompression=true\n";
			}

			dpb->insertString(&throw_status, isc_dpb_config, dpb_config.str().c_str());
		}
		catch( const FbException& error )
		{
			if( dpb ) dpb->dispose();
			const ISC_STATUS * statusVector = error.getStatus()->getErrors();
			throw SQLEXCEPTION ( GDS->getSqlCode( statusVector ), statusVector [1], getIscStatusText( error.getStatus() ) );
		}

		CheckStatusWrapper check_status( GDS->_status );

		// Phase 14.2.2: Get raw DPB bytes and connect via fb-cpp Attachment (RAII)
		unsigned dpbLen = 0;
		const unsigned char* dpbBuf = nullptr;
		try
		{
			dpbLen = dpb->getBufferLength(&throw_status);
			dpbBuf = dpb->getBuffer(&throw_status);
		}
		catch( const FbException& error )
		{
			dpb->dispose();
			const ISC_STATUS * statusVector = error.getStatus()->getErrors();
			throw SQLEXCEPTION ( GDS->getSqlCode( statusVector ), statusVector [1], getIscStatusText( error.getStatus() ) );
		}

		fbcpp::AttachmentOptions attachOpts;
		attachOpts.setDpb( std::vector<std::uint8_t>( dpbBuf, dpbBuf + dpbLen ) );

		dpb->dispose();
		dpb = nullptr;

		try
		{
			if ( databaseAccess_ == CREATE_DB )
				attachOpts.setCreateDatabase( true );

			attachment_ = std::make_unique<fbcpp::Attachment>( GDS->getClient(), dbName, attachOpts );
			databaseHandle = attachment_->getHandle().get();
		}
		catch( const fbcpp::DatabaseException& e )
		{
			if ( databaseAccess_ != CREATE_DB )
			{
				// Try the CREATE_DB fallback on any connection error
				// (before Phase 14.7, we can't inspect specific ISC codes from DatabaseException)
				throw SQLEXCEPTION( -1, 0, e.what() );
			}
			else
			{
				// CREATE_DB was explicitly requested but failed
				throw SQLEXCEPTION( -1, 0, e.what() );
			}
		}

		if ( databaseAccess_ == DROP_DB )
		{
			try
			{
				attachment_->dropDatabase();
				databaseHandle = nullptr;
				attachment_.reset();
			}
			catch( const fbcpp::DatabaseException& e )
			{
				throw SQLEXCEPTION( -1, 0, e.what() );
			}
			return;
		}

		// Parse database info
		char result [2048];
		databaseDialect_ = SQL_DIALECT_V5;

		CheckStatusWrapper info_status( GDS->_status );
		databaseHandle->getInfo( &info_status, sizeof (databaseInfoItems),
		                         (const unsigned char*)databaseInfoItems, sizeof (result), (unsigned char*)result );

		if( ( info_status.getState() & IStatus::STATE_ERRORS ) == 0 )
		{
			for (auto p = result; p < result + sizeof (result) && *p != isc_info_end && *p != isc_info_truncated;)
			{
				char item = *p++;
				int length = fb_vax_integer(p, 2);
				p += 2;
				switch (item)
				{
				case isc_info_db_id:
					{
						char * next = p + 2 + *( p + 1 );

						databaseNameFromServer_.Format( "%.*s", *( p + 1 ), p + 2 );
						databaseServerName_.Format( "%.*s", *next, next + 1 );
					}
					break;

				case isc_info_db_sql_dialect:
					databaseDialect_ = fb_vax_integer(p, length);
					break;
				
				case isc_info_base_level:
					serverBaseLevel_ = fb_vax_integer(p, length);
					break;

				case isc_info_user_names:
					if ( userAccess_.IsEmpty() )
					{
						userName_ = JString ( p + 1, (int)*p );
						userAccess_ = userName_;
						userType_ = 8;
					}
					break;

				case isc_info_firebird_version:
					{
						int level = 0;
						char * start = p + 2;
						char * beg = start;
						char * end = beg + p [1];
						
						while ( beg < end )
						{
							if ( *beg >= '0' && *beg <= '9' )
							{
								switch ( ++level )
								{
								case 1:
									majorFb_ = atoi(beg);
									while( *++beg != '.' );
									break;
								case 2:
									minorFb_ = atoi(beg);
									while( *++beg != '.' );
									break;
								default:
									versionFb_ = atoi(beg);
									while( *beg >= '0' && *beg <= '9' || *beg == ' ')
										beg++;
									if ( *beg == '.' )
										break;
									beg = end;
									break;
								}
							}
							else
								beg++;
						}
						databaseProductName_ = "Firebird";
					}
					break;

				case isc_info_version:
					{
						JString productName;
						int level = 0;
						int major = 0, minor = 0, version = 0;
						char * start = p + 2;
						char * beg = start;
						char * end = beg + p [1];
						char * tmp = NULL;
						
						while ( beg < end )
						{
							if ( *beg >= '0' && *beg <= '9' )
							{
								switch ( ++level )
								{
								case 1:
									tmp = beg;
									major = atoi(beg);
									while( *++beg != '.' );
									break;
								case 2:
									minor = atoi(beg);
									while( *++beg != '.' );
									break;
								default: // Firebird (##.##.##.####) and Yaffil(##.##.####)
									version = atoi(beg);
									while( *beg >= '0' && *beg <= '9' || *beg == ' ')
										beg++;
									if ( *beg == '.' )
										break;
									if ( beg < end )
									{
										productName = JString( beg, end - beg );

										char *endBeg = beg;

										while( *endBeg != ' ' )
											endBeg++;

										databaseProductName_ = JString( beg, endBeg - beg );
									}
									beg = end;
									break;
								}
							}
							else
								beg++;
						}
						serverVersion_.Format( "%02d.%02d.%04d %.*s %s",major,minor,version, tmp ? tmp - start : 0, start, (const char*)productName );
					}
					break;

				case isc_info_page_size:
					pageSize_ = fb_vax_integer(p, length);
					break;
				}
				p += length;
			}
		}
		
		if ( dialect && *dialect == '1')
			databaseDialect_ = SQL_DIALECT_V5;
		else
			databaseDialect_ = SQL_DIALECT_V6;

		const char* property = properties->findValue ("quoted", NULL);

		if ( property && *property == 'Y')
			quotedIdentifier_ = true;
		else
			quotedIdentifier_ = false;

		property = properties->findValue ("sensitive", NULL);

		if ( property && *property == 'Y')
			sensitiveIdentifier_ = true;
		else
			sensitiveIdentifier_ = false;

		property = properties->findValue ("autoQuoted", NULL);

		if ( property && *property == 'Y')
			autoQuotedIdentifier_ = true;
		else
			autoQuotedIdentifier_ = false;

		property = properties->findValue ("useSchema", NULL);

		if ( property )
		{
			switch ( *property )
			{
			case '1': // remove SCHEMA from SQL query
				useSchemaIdentifier_ = 1;
				break;

			case '2': // use full SCHEMA
				useSchemaIdentifier_ = 2;
				break;

			default:
			case '0': // set null field SCHEMA
				useSchemaIdentifier_ = 0;
				break;
			}
		}
		else
			useSchemaIdentifier_ = 0;

		property = properties->findValue ("useLockTimeout", NULL);

		if (property && *property)
			useLockTimeoutWaitTransactions_ = atoi(property);

		property = properties->findValue ("dsn", NULL);

		if (property && *property)
			dsn_ = property;

		checkAdmin();
		ownsConnection_ = true;

		if ( databaseHandle && !isRoles_ && !admin_ )
		{
			IscTablePrivilegesResultSet resultSet ( (IscDatabaseMetaData *)getMetaData() );
			resultSet.allTablesAreSelectable = true;
			resultSet.getTablePrivileges( NULL, NULL, "RDB$ROLES" );

			if ( resultSet.getCountRowsStaticCursor() )
			{
				int len1 = (int)strlen( userName_ );
				int len2;
				char *beg = resultSet.sqlda->getVarying( 5, len2 );
				char *end = beg + len2;
				char *save = end;

				while ( end > beg && *(--end) == ' ');

				if ( save != end )
				{
					len2 = end - beg + 1;
					*(end+1) = '\0';
				}

				if( len1 == len2 && !strncmp( userName_, beg, len1 ) )
					admin_ = true;
			}
		}
	}
	catch ( SQLException& exception )
	{
		attachment_.reset();
		databaseHandle = NULL;
		GDS = NULL;
		throw SQLEXCEPTION ( (SqlCode)exception.getSqlcode(), exception.getFbcode(), exception.getText() );
	}
	catch (...)
	{
		attachment_.reset();
		databaseHandle = NULL;
		GDS = NULL;
		throw;
	}
}


void IscConnection::deleteStatement(IscStatement * statement)
{
//From R. Milharcic
	statements.erase(std::remove(statements.begin(), statements.end(), statement), statements.end());
}


JString IscConnection::getIscStatusText(Firebird::IStatus *status)
{
	return GDS->getIscStatusText(status);
}

int IscConnection::getInfoItem(char * buffer, int infoItem, int defaultValue)
{
	for (char *p = buffer; *p != isc_info_end;)
		{
		char item = *p++;
		int length = fb_vax_integer(p, 2);
		p += 2;
		if (item == infoItem)
			return fb_vax_integer(p, length);
		p += length;
		}

	return defaultValue;			
}

JString IscConnection::getInfoString(char * buffer, int infoItem, const char * defaultString)
{
	for (char *p = buffer; *p != isc_info_end;)
		{
		char item = *p++;
		int length = fb_vax_integer(p, 2);
		p += 2;
		if (item == infoItem)
			return JString (p, length);
		p += length;
		}

	return defaultString;			
}

Properties* IscConnection::allocProperties()
{
	return new Parameters;
}

PropertiesEvents* IscConnection::allocPropertiesEvents()
{
	return new ParametersEvents;
}

int IscConnection::objectVersion()
{
	return CONNECTION_VERSION;
}

void IscConnection::clearWarnings()
{
	NOT_YET_IMPLEMENTED;
}

Connection* IscConnection::clone()
{
	return new IscConnection (this);
}

void IscConnection::setAutoCommit(bool setting)
{
	if( !autoCommit_ && setting && transactionPending_ )
		commitAuto();

	autoCommit_ = setting;
}

bool IscConnection::getAutoCommit()
{
	return autoCommit_;
}

const char*	IscConnection::getCatalog()
{
	NOT_YET_IMPLEMENTED;
	return "";
}

void IscConnection::setCatalog(const char* catalog)
{
	NOT_YET_IMPLEMENTED;
}

void IscConnection::setTransactionIsolation(int level)
{
	transactionIsolation_ = level;
}

int IscConnection::getTransactionIsolation()
{
	return transactionIsolation_;
}

bool IscConnection::isClosed()
{
	NOT_YET_IMPLEMENTED;
	return false;
}

bool IscConnection::isReadOnly()
{
	NOT_YET_IMPLEMENTED;
	return false;
}

void IscConnection::setReadOnly(bool readOnly)
{
	NOT_YET_IMPLEMENTED;
}

const char*	IscConnection::nativeSQL(const char *sqlString)
{
	NOT_YET_IMPLEMENTED;
	return sqlString;
}

void IscConnection::setExtInitTransaction(int optTpb)
{
	transactionExtInit_ = optTpb;
}

EnvironmentShare* IscConnection::getEnvironmentShare()
{
	return (EnvironmentShare*)&getEnvironmentShareInstance();
}

void IscConnection::connectionToEnvShare()
{
	shareConnected = getEnvironmentShareInstance().addConnection (this);
}

void IscConnection::connectionFromEnvShare()
{
	getEnvironmentShareInstance().removeConnection (this);
	shareConnected = false;
}

JString IscConnection::getDatabaseServerName()
{
	if ( databaseServerName_.IsEmpty() )
		return getEnvironmentShareInstance().getDatabaseServerName();

	return databaseServerName_;
}

int	IscConnection::getDriverBuildKey()
{
	return DRIVER_BUILD_KEY;
}

void IscConnection::addRef()
{
	++useCount;
}

int IscConnection::release()
{
	if (--useCount == 0)
		{
		close();
		return 0;
		}

	return useCount;
}

CallableStatement* IscConnection::prepareCall(const char * sqlString)
{
	// Phase 14.4.6a: IscCallableStatement merged into IscPreparedStatement.
	IscPreparedStatement *statement = NULL;

	try
		{
		statement = new IscPreparedStatement (this);
		statement->prepareCall (sqlString);
		}
	catch (...)
		{
		if (statement)
			delete statement;
		throw;
		}

	statements.push_back (statement);
	return statement;
}

void IscConnection::commitAuto()
{
	bool callRetaining = false;

	for (auto* statement : statements)
	{
		if ( statement->isActiveCursor() )
			callRetaining = true;
		else if ( statement->isActiveLocalTransaction() )
			statement->commitLocal();
	}

	if ( callRetaining )
		commitRetaining();
	else
		commit();
}

void IscConnection::rollbackAuto()
{
	bool callRetaining = false;

	for (auto* statement : statements)
	{
		if ( statement->isActiveCursor() )
			callRetaining = true;
		else if ( statement->isActiveLocalTransaction() )
			statement->rollbackLocal();
	}

	if ( callRetaining )
		rollbackRetaining();
	else
		rollback();
}

void IscConnection::setSavepoint(const char* name)
{
	if ( !transaction_ || !transaction_->isValid() )
		return;

	char sql[256];
	snprintf(sql, sizeof(sql), "SAVEPOINT %s", name);

	ThrowStatusWrapper status( GDS->_status );
	try
	{
		databaseHandle->execute(
			&status, transaction_->getHandle().get(),
			0, sql, databaseDialect_,
			NULL, NULL, NULL, NULL );
	}
	catch( const FbException& error )
	{
		THROW_ISC_EXCEPTION( this, error.getStatus() );
	}
}

void IscConnection::releaseSavepoint(const char* name)
{
	if ( !transaction_ || !transaction_->isValid() )
		return;

	char sql[256];
	snprintf(sql, sizeof(sql), "RELEASE SAVEPOINT %s", name);

	ThrowStatusWrapper status( GDS->_status );
	try
	{
		databaseHandle->execute(
			&status, transaction_->getHandle().get(),
			0, sql, databaseDialect_,
			NULL, NULL, NULL, NULL );
	}
	catch( const FbException& )
	{
		// RELEASE SAVEPOINT may fail if the savepoint was already
		// released or rolled back — this is not an error condition
	}
}

void IscConnection::rollbackSavepoint(const char* name)
{
	if ( !transaction_ || !transaction_->isValid() )
		return;

	char sql[256];
	snprintf(sql, sizeof(sql), "ROLLBACK TO SAVEPOINT %s", name);

	ThrowStatusWrapper status( GDS->_status );
	try
	{
		databaseHandle->execute(
			&status, transaction_->getHandle().get(),
			0, sql, databaseDialect_,
			NULL, NULL, NULL, NULL );
	}
	catch( const FbException& error )
	{
		THROW_ISC_EXCEPTION( this, error.getStatus() );
	}
}

int IscConnection::getServerMajorVersion()
{
	return databaseHandle ? majorFb_ : 0;
}

int IscConnection::getServerMinorVersion()
{
	return databaseHandle ? minorFb_ : 0;
}

int IscConnection::getDatabaseDialect()
{
	return databaseDialect_;
}

void IscConnection::commitRetaining()
{
	if ( !transaction_ || !transaction_->isValid() )
	{
		transactionPending_ = false;
		return;
	}

	try
	{
		transaction_->commitRetaining();
	}
	catch( const fbcpp::DatabaseException& e )
	{
		try { rollbackRetaining(); } catch (...) {}
		throw SQLEXCEPTION( RUNTIME_ERROR, e.what() );
	}
	transactionPending_ = false;
}

void IscConnection::rollbackRetaining()
{
	if ( !transaction_ || !transaction_->isValid() )
	{
		transactionPending_ = false;
		return;
	}

	try
	{
		transaction_->rollbackRetaining();
	}
	catch( const fbcpp::DatabaseException& e )
	{
		// If rollbackRetaining fails, do a full rollback
		try { rollback(); } catch (...) {}
		throw SQLEXCEPTION( RUNTIME_ERROR, e.what() );
	}
	transactionPending_ = false;
}

}; // end namespace IscDbcLibrary
