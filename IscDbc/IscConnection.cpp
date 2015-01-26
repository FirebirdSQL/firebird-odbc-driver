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
#include "IscDbc.h"
#include "EnvShare.h"
#include "IscConnection.h"
#include "IscProceduresResultSet.h"
#include "IscTablePrivilegesResultSet.h"
#include "SQLError.h"
#include "IscOdbcStatement.h"
#include "IscUserEvents.h"
#include "IscCallableStatement.h"
#include "IscDatabaseMetaData.h"
#include "Parameters.h"
#include "ParametersEvents.h"
#include "Attachment.h"
#include "Mlist.h"
#include "SupportFunctions.h"
#include "../SetupAttributes.h"
#include "MultibyteConvert.h"

namespace IscDbcLibrary {

extern SupportFunctions supportFn;

extern char charTable [];

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern EnvShare environmentShare;

extern "C" Connection* createConnection()
{
	setlocale( LC_ALL, ".ACP" );
	return new IscConnection;
}

InfoTransaction::InfoTransaction()
{
	transactionHandle = NULL;
	transactionIsolation = 0;
	transactionPending = false;
	autoCommit = true;
	transactionExtInit = 0;
	nodeParamTransaction = NULL;
}

InfoTransaction::~InfoTransaction()
{
	delete nodeParamTransaction;
}

IscConnection::IscConnection()
{
	init();
}

IscConnection::IscConnection(IscConnection * source)
{
	init();
	attachment = source->attachment;
	attachment->addRef();
}

void IscConnection::init()
{
	useCount = 1;
	metaData = NULL;
	shareConnected = false;
	attachment = NULL;
	userEvents = NULL;
	useAppOdbcVersion = 3; // SQL_OV_ODBC3
	tmpParamTransaction = NULL;
}

IscConnection::~IscConnection()
{
	if (metaData)
		delete metaData;

	if (attachment)
		attachment->release();

	delete tmpParamTransaction;

	if ( userEvents )
		userEvents->release();
}

bool IscConnection::isConnected()
{
	return attachment != NULL;
}

void IscConnection::close()
{
	FOR_OBJECTS (IscStatement*, statement, &statements)
		statement->close();
		statement->freeStatementHandle();
		statement->connection = NULL; // NOMEY
	END_FOR;

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

	statements.append (statement);

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
	InfoTransaction	&tr = transactionInfo;

	if ( tr.transactionHandle )
	{
		ISC_STATUS statusVector[20];
		GDS->_commit_transaction( statusVector, &tr.transactionHandle );

		if ( statusVector[1] )
		{
			rollback();
			THROW_ISC_EXCEPTION ( this, statusVector );
		}
	}
	tr.transactionPending = false;
}

void IscConnection::rollback()
{
	InfoTransaction	&tr = transactionInfo;

	if ( tr.transactionHandle )
	{
		ISC_STATUS statusVector[20];
		GDS->_rollback_transaction( statusVector, &tr.transactionHandle );

		if ( statusVector[1] )
			THROW_ISC_EXCEPTION ( this, statusVector );
	}
	tr.transactionPending = false;
}

void IscConnection::prepareTransaction()
{
	InfoTransaction	&tr = transactionInfo;

	if ( tr.transactionHandle )
	{
		ISC_STATUS statusVector[20];
		GDS->_prepare_transaction2( statusVector, &tr.transactionHandle, 0, 0 );

		if ( statusVector[1] )
			THROW_ISC_EXCEPTION ( this, statusVector );
	}
}

bool IscConnection::getTransactionPending()
{
	return 	transactionInfo.transactionPending;
}

isc_db_handle IscConnection::getHandleDb()
{	
	return attachment->databaseHandle;
}

isc_tr_handle IscConnection::startTransaction()
{
	InfoTransaction	&tr = transactionInfo;

	if ( shareConnected )
	{
		if ( !attachment->transactionHandle )
		{
			environmentShare.startTransaction();
			// ASSERT (!autoCommit)
			tr.transactionPending = true;
		}

		return attachment->transactionHandle;
	}

    if ( tr.transactionHandle )
        return tr.transactionHandle;

    ISC_STATUS statusVector[20];

    char    iscTpb[9];
	int		count = sizeof( iscTpb ) - 4; // 4 it's size block for Lock Timeout Wait Transactions

    iscTpb[0] = isc_tpb_version3;
    iscTpb[1] = tr.transactionExtInit & TRA_ro ? isc_tpb_read : isc_tpb_write;
    iscTpb[2] = tr.transactionExtInit & TRA_nw ? isc_tpb_nowait : isc_tpb_wait;
    /* Isolation level */
    switch( tr.transactionIsolation )
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

	if ( !(tr.transactionExtInit & TRA_nw) 
		&& attachment->isFirebirdVer2_0()
		&& attachment->getUseLockTimeoutWaitTransactions() )
	{
		char *pt = &iscTpb[count];

		*pt++ = isc_tpb_lock_timeout;
		*pt++ = sizeof ( short );
		*pt++ = (char)attachment->getUseLockTimeoutWaitTransactions();
		*pt++ = (char)(attachment->getUseLockTimeoutWaitTransactions() >> 8);

		count += 4;
	}

    GDS->_start_transaction( statusVector, &tr.transactionHandle, 1, &attachment->databaseHandle,
            count, iscTpb );

    if ( statusVector [1] )
		THROW_ISC_EXCEPTION ( this, statusVector );

	if ( !tr.autoCommit )
		tr.transactionPending = true;

    return tr.transactionHandle;
}

Statement* IscConnection::createStatement()
{
	IscStatement *statement = new IscStatement (this);
	statements.append (statement);

	return statement;
}

InternalStatement* IscConnection::createInternalStatement()
{
	IscOdbcStatement *statement = new IscOdbcStatement (this);
	statements.append (statement);

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
	delimiter = delimiter == ' ' || attachment->databaseDialect < 3 ? 0 : delimiter;
	bool autoQuoted = delimiter && attachment->autoQuotedIdentifier;

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
	char tpbBuffer[4096]; // Note: Fb(gpre) use 4000
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
				if ( environmentShare.findParamTransactionFromList( node ) )
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
						if ( !transactionInfo.nodeParamTransaction )
							transactionInfo.nodeParamTransaction = new CNodeParamTransaction;

						*transactionInfo.nodeParamTransaction = node;
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

	char* text = tpbBuffer;
	*text++ = isc_tpb_version3;
	*text++ = (transFlags & TRA_ro) ? isc_tpb_read : isc_tpb_write;
	if (transFlags & TRA_con)
		*text++ = isc_tpb_consistency;
	else if (transFlags & TRA_read_committed)
		*text++ = isc_tpb_read_committed;
	else
		*text++ = isc_tpb_concurrency;

	if ( transFlags & TRA_nw )
		*text++ = isc_tpb_nowait;
	else
	{
		*text++ = isc_tpb_wait;

		if ( node.lockTimeout && attachment->isFirebirdVer2_0() )
		{
			*text++ = isc_tpb_lock_timeout;
			*text++ = sizeof ( short );
			*text++ = (char)node.lockTimeout;
			*text++ = (char)(node.lockTimeout >> 8);
		}
	}

	if ( transFlags & TRA_read_committed )
	{
		*text++ = (transFlags & TRA_no_rec_version) ?
			isc_tpb_no_rec_version : isc_tpb_rec_version;
	}

	if (transFlags & TRA_no_auto_undo)
		*text++ = isc_tpb_no_auto_undo;

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
				environmentShare.addParamTransactionToList( node );
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

			if ( !transactionInfo.nodeParamTransaction )
				transactionInfo.nodeParamTransaction = new CNodeParamTransaction;

			*transactionInfo.nodeParamTransaction = node;

			if ( node.lengthNameTransaction )
			{
				environmentShare.addParamTransactionToList( node );
				ret = -6; // for all connections  = -6 it's named param Transaction
			}
		}
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
	char * ptEndBracket = NULL;
	int ignoreBracket = 0;
	int statusQuote = 0;
	int statusBracket = 0;
	char quote;
	char delimiter = *metaData->getIdentifierQuoteString();
	delimiter = delimiter == ' ' || attachment->databaseDialect < 3 ? 0 : delimiter;

	bool autoRemoveSchemaFromIdentifier = attachment->useSchemaIdentifier == 1;
	bool autoQuoted = delimiter && attachment->autoQuotedIdentifier;

#pragma FB_COMPILER_MESSAGE("IscConnection::getNativeSql - The temporary decision; FIXME!")

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
			else if ( *ptIn == '{' )
				ptEndBracket = ptOut;
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

	if ( !ptEndBracket )
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
	else
	{
		while ( ptEndBracket )
		{
			ptIn = ptEndBracket;

			ptIn++; // '{'
			SKIP_WHITE ( ptIn );

//	On a note		++ignoreBracket; // ignored { }
			if ( *ptIn == '?' || IS_MATCH( ptIn, "CALL" ) )
			{	
				if ( *ptIn == '?' )
				{
					ptIn++;
					SKIP_WHITE ( ptIn );

					if(*ptIn != '=')
						return statysModify;

					ptIn++; // '='
					SKIP_WHITE ( ptIn );
				}

				if ( !IS_MATCH( ptIn, "CALL" ) )
					return statysModify;

				ptIn += 4; // 'call'
				SKIP_WHITE ( ptIn );

				ptOut = ptEndBracket;
				int ignoreBr = ignoreBracket;
				char * savePtOut;

				const int lenSpase = 18;
				int offset = lenSpase - ( ptIn - ptOut );

				memmove(ptOut + offset, ptOut, strlen(ptOut) + 1 );
				memset(ptOut, ' ', lenSpase);
				savePtOut = ptOut;
				ptIn += offset; 
				ptOut += lenSpase;

				char procedureName[256];
				char * end = procedureName;
				bool repeatWhile;
				char quote = 0;

				do
				{
					repeatWhile = false;

					SKIP_WHITE ( ptIn );

					end = procedureName;
					char *ptTmp = ptIn;

					if ( autoRemoveSchemaFromIdentifier )
					{
						if ( IS_QUOTE( *ptIn ) )
							quote = *ptIn++;

						while ( !(IS_END_TOKEN(*ptIn)) )
						{
							if ( IS_POINT( *ptIn ) )
								break;

							if ( quote )
							{
								if ( quote == *ptIn )
									break;
	
								*end++ = *ptIn++;
							}
							else
								*end++ = UPPER(*ptIn), ++ptIn;
						}

						if ( quote && quote == *ptIn )
						{
							++ptIn;
							if ( IS_POINT( *ptIn ) )
								quote = 0;
						}

						SKIP_WHITE ( ptIn );

						if ( IS_POINT( *ptIn ) )
						{
							++ptIn;
							memmove( quote ? ptTmp + 1 : ptTmp, ptIn, strlen( ptIn ) + 1 );
							ptIn = ptTmp;
							repeatWhile = true;
						}
					}
					else if ( IS_QUOTE( *ptIn ) )
					{
						quote = *ptIn++;

						while ( !(IS_END_TOKEN(*ptIn)) )
							*end++ = *ptIn++;

						end--;

						if ( !IS_QUOTE(*end) || quote != *end )
							return statysModify;

						quote = 0;
					}
					else
						while ( !(IS_END_TOKEN(*ptIn)) )
							*end++ = UPPER(*ptIn), ++ptIn;

				} while ( repeatWhile );
				
				*end = '\0';

				int numIn, numOut;
				bool canSelect;

				if ( !getCountInputParamFromProcedure ( procedureName, numIn, numOut, canSelect ) )
				{
					JString text;
					text.Format( "Unknown procedure '%s'", procedureName );
					throw SQLEXCEPTION( SYNTAX_ERROR, text );
				}

				int ret = buildParamProcedure ( ptIn, numIn );

				if ( ret == -1 )
					return statysModify;
				else if ( canSelect )
					memcpy(savePtOut, "select * from ", 14);
				else
					memcpy(savePtOut, "execute procedure ", 18);

				ptOut = ptIn;

				statusQuote = 0;
				quote = 0;

				do
				{
					while( *ptIn )
					{
						if ( !statusQuote )
						{
							if ( IS_QUOTE( *ptIn ) )
							{
								quote = *ptIn;
								statusQuote ^= 1;
							}
							else if ( *ptIn == '}' )
								break;
						}
						else if ( quote == *ptIn )
						{
							quote = 0;
							statusQuote ^= 1;
						}

						*ptOut++ = *ptIn++;
					}

					if( ignoreBr )
						*ptOut++ = *ptIn++;

				}while ( ignoreBr-- );

				if(*ptIn != '}')
					return statysModify;

				ptIn++; // '}'

				while( *ptIn )
					*ptOut++ = *ptIn++;

				*ptOut = '\0';
				statysModify = ret == 1 ? 2 : 1;
			}
			else
			{
				ptOut = ptEndBracket;

				// Check 'oj' or 'OJ'
				if ( *(short*)ptIn == 0x6a6f || *(short*)ptIn == 0x4a4f )
					ptIn += 2; // 'oj'
				else if ( IS_MATCH( ptOut, "INTERVAL" ) )
					ptIn += 8; // 'INTERVAL'
				else if ( !strncasecmp ( ptOut, "{TS", 3 ) )
					ptIn += 2; // 'ts'
				else if ( !strncasecmp ( ptOut, "{D", 2 ) || !strncasecmp ( ptOut, "{T", 2 ) )
					ptIn += 1; // 'd', 't'
				else
				{
					ptIn += 2; // 'fn'
//
// select "FIRST_NAME" from "EMPLOYEE" where { fn UCASE("FIRST_NAME") } = { fn UCASE('robert') }
// to
// select "FIRST_NAME" from "EMPLOYEE" where UPPER("FIRST_NAME") = UPPER('robert')
//
// ATTENTION! ptIn and ptOut pointer of outStatementText
					supportFn.translateNativeFunction ( ptIn, ptOut );
				}

				int ignoreBr = ignoreBracket;
				statusQuote = 0;
				quote = 0;

				do
				{
					while( *ptIn )
					{
						if ( !statusQuote )
						{
							if ( IS_QUOTE( *ptIn ) )
							{
								quote = *ptIn;
								statusQuote ^= 1;
							}
							else if ( *ptIn == '}' )
								break;
						}
						else if ( quote == *ptIn )
						{
							quote = 0;
							statusQuote ^= 1;
						}

						*ptOut++ = *ptIn++;
					}

					if( ignoreBr )
						*ptOut++ = *ptIn++;

				}while ( ignoreBr-- );

				if(*ptIn != '}')
					return statysModify;

				ptIn++; // '}'

				while( *ptIn )
					*ptOut++ = *ptIn++;

				*ptOut = '\0';
				statysModify = 1;
			}

			--ptEndBracket; // '{'

			statusQuote = 0;
			quote = 0;

			while ( ptEndBracket > outStatementText )
			{
				if ( !statusQuote )
				{
					if ( IS_QUOTE( *ptEndBracket ) )
					{
						quote = *ptEndBracket;
						statusQuote ^= 1;
					}
					else if ( *ptEndBracket == '{' )
						break;
				}
				else if ( quote == *ptEndBracket )
				{
					quote = 0;
					statusQuote ^= 1;
				}

				--ptEndBracket;
			}

			if(*ptEndBracket != '{')
				ptEndBracket = NULL;
		}

		if ( textLength2Ptr )
			*textLength2Ptr = ptOut - outStatementText;
	}

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
	return attachment->charsetCode;
}

WCSTOMBS IscConnection::getConnectionWcsToMbs()
{
	return adressWcsToMbs( attachment->charsetCode );
}

MBSTOWCS IscConnection::getConnectionMbsToWcs()
{
	return adressMbsToWcs( attachment->charsetCode );
}

int IscConnection::hasRole(const char * schemaName, const char * roleName)
{
	NOT_YET_IMPLEMENTED;

	return false;
}

void IscConnection::ping()
{
}

void IscConnection::sqlExecuteCreateDatabase(const char * sqlString)
{
	isc_db_handle   newdb = NULL;
	isc_tr_handle   trans = NULL;
	ISC_STATUS      statusVector[20];

	if ( GDS->_dsql_execute_immediate( statusVector, &newdb, &trans, 0, (char*)sqlString, 3, NULL ) )
    {
		if ( statusVector[1] )
			THROW_ISC_EXCEPTION ( this, statusVector );
	}
	
	GDS->_commit_transaction( statusVector, &trans );
	GDS->_detach_database( statusVector, &newdb );
}

void IscConnection::createDatabase(const char * dbName, Properties * properties)
{
	try
	{
		attachment = new Attachment;
		attachment->createDatabase( dbName, properties );
		databaseHandle = attachment->databaseHandle;
		GDS = attachment->GDS;
	}
	catch ( SQLException& exception )
	{
		delete attachment;
		attachment = NULL;
		GDS = NULL;
		throw SQLEXCEPTION ( (SqlCode)exception.getSqlcode() , exception.getText() );
	}
	catch (...)
	{
		delete attachment;
		attachment = NULL;
		GDS = NULL;
		throw;
	}
}

void IscConnection::openDatabase(const char * dbName, Properties * properties)
{
	try
	{
		attachment = new Attachment;
		attachment->openDatabase (dbName, properties);
		databaseHandle = attachment->databaseHandle;
		GDS = attachment->GDS;

		if ( databaseHandle && !attachment->isRoles && !attachment->admin )
		{
			IscTablePrivilegesResultSet resultSet ( (IscDatabaseMetaData *)getMetaData() );
			resultSet.allTablesAreSelectable = true;
			resultSet.getTablePrivileges( NULL, NULL, "RDB$ROLES" );

			if ( resultSet.getCountRowsStaticCursor() )
			{
				int len1 = (int)strlen( attachment->userName );
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

				if( len1 == len2 && !strncmp( attachment->userName, beg, len1 ) )
					attachment->admin = true;
			}
		}
	}
	catch ( SQLException& exception )
	{
		delete attachment;
		attachment = NULL;
		GDS = NULL;
		throw SQLEXCEPTION ( (SqlCode)exception.getSqlcode(), exception.getFbcode(), exception.getText() );
	}
	catch (...)
	{
		delete attachment;
		attachment = NULL;
		GDS = NULL;
		throw;
	}
}


void IscConnection::deleteStatement(IscStatement * statement)
{
//From R. Milharcic
 	statements.deleteItem (statement);
}


JString IscConnection::getIscStatusText(ISC_STATUS * statusVector)
{
	return attachment->getIscStatusText(statusVector);
}


int IscConnection::getInfoItem(char * buffer, int infoItem, int defaultValue)
{
	for (char *p = buffer; *p != isc_info_end;)
		{
		char item = *p++;
		int length = GDS->_vax_integer (p, 2);
		p += 2;
		if (item == infoItem)
			return GDS->_vax_integer (p, length);
		p += length;
		}

	return defaultValue;			
}

JString IscConnection::getInfoString(char * buffer, int infoItem, const char * defaultString)
{
	for (char *p = buffer; *p != isc_info_end;)
		{
		char item = *p++;
		int length = GDS->_vax_integer (p, 2);
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
	InfoTransaction	&tr = transactionInfo;

	if( !tr.autoCommit && setting && tr.transactionPending )
		commitAuto();

	tr.autoCommit = setting;
}

bool IscConnection::getAutoCommit()
{
	return transactionInfo.autoCommit;
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
	transactionInfo.transactionIsolation = level;
}

int IscConnection::getTransactionIsolation()
{
	return transactionInfo.transactionIsolation;
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
	transactionInfo.transactionExtInit = optTpb;
}

EnvironmentShare* IscConnection::getEnvironmentShare()
{
	return (EnvironmentShare*)&environmentShare;
}

void IscConnection::connectionToEnvShare()
{
	shareConnected = environmentShare.addConnection (this);
}

void IscConnection::connectionFromEnvShare()
{
	environmentShare.removeConnection (this);
	shareConnected = false;
}

JString IscConnection::getDatabaseServerName()
{
	if ( attachment->databaseServerName.IsEmpty() )
		return environmentShare.getDatabaseServerName();

	return attachment->databaseServerName;
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
	IscCallableStatement *statement = NULL;

	try
		{
		statement = new IscCallableStatement (this);
		statement->prepare (sqlString);
		}
	catch (...)
		{
		if (statement)
			delete statement;
		throw;
		}

	statements.append (statement);
	return statement;
}

void IscConnection::commitAuto()
{
	bool callRetaining = false;

	FOR_OBJECTS (IscStatement*, statement, &statements)
		if ( statement->isActiveCursor() )
			callRetaining = true;
		else if ( statement->isActiveLocalTransaction() )
			statement->commitLocal();
	END_FOR;

	if ( callRetaining )
		commitRetaining();
	else
		commit();
}

void IscConnection::rollbackAuto()
{
	bool callRetaining = false;

	FOR_OBJECTS (IscStatement*, statement, &statements)
		if ( statement->isActiveCursor() )
			callRetaining = true;
		else if ( statement->isActiveLocalTransaction() )
			statement->rollbackLocal();
	END_FOR;

	if ( callRetaining )
		rollbackRetaining();
	else
		rollback();
}

int IscConnection::getDatabaseDialect()
{
	return attachment->getDatabaseDialect();
}

void IscConnection::commitRetaining()
{
	InfoTransaction	&tr = transactionInfo;

	if ( tr.transactionHandle )
	{
		ISC_STATUS statusVector[20];
		GDS->_commit_retaining( statusVector, &tr.transactionHandle );

		if ( statusVector[1] )
		{
			rollbackRetaining();
			THROW_ISC_EXCEPTION ( this, statusVector );
		}
	}
	tr.transactionPending = false;
}

void IscConnection::rollbackRetaining()
{
	InfoTransaction	&tr = transactionInfo;

	if ( tr.transactionHandle )
	{
		ISC_STATUS statusVector[20];
		try
		{
			GDS->_rollback_retaining( statusVector, &tr.transactionHandle );
		}
		catch (...)
		{
			if ( statusVector[1] )
			{
				rollback();
				THROW_ISC_EXCEPTION ( this, statusVector );
			}
		}
	}
	tr.transactionPending = false;
}

}; // end namespace IscDbcLibrary
