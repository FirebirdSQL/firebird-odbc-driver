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

#include <string.h>
#include "IscDbc.h"
#include "IscConnection.h"
#include "SQLError.h"
#include "IscCallableStatement.h"
#include "IscDatabaseMetaData.h"
#include "Parameters.h"
#include "Attachment.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern "C" Connection* createConnection()
{
	return new IscConnection;
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
	transactionHandle = NULL;
	transactionIsolation = 0;
	transactionPending = false;
	autoCommit = true;
	attachment = NULL;
	transactionExtInit = 0;
}

IscConnection::~IscConnection()
{
	if (metaData)
		delete metaData;

	if (attachment)
		attachment->release();
}

bool IscConnection::isConnected()
{
	return attachment != NULL;
}

void IscConnection::close()
{
	FOR_OBJECTS (IscStatement*, statement, &statements)
		statement->close();
		statement->connection = NULL; // NOMEY
	END_FOR;

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

//From R. Milharcic
	statements.append (statement);

	return statement;
}

void IscConnection::commit()
{
	if (transactionHandle)
	{
		ISC_STATUS statusVector [20];
		GDS->_commit_transaction (statusVector, &transactionHandle);

		if (statusVector [1])
		{
			rollback();
			throw SQLEXCEPTION (statusVector [1], getIscStatusText (statusVector));
		}
	}
	transactionPending = false;
}

void IscConnection::rollback()
{
	if (transactionHandle)
	{
		ISC_STATUS statusVector [20];
		GDS->_rollback_transaction (statusVector, &transactionHandle);

		if (statusVector [1])
			throw SQLEXCEPTION (statusVector [1], getIscStatusText (statusVector));
	}
	transactionPending = false;
}

void IscConnection::prepareTransaction()
{
}

bool IscConnection::getTransactionPending()
{
	return transactionPending;
}

void* IscConnection::getHandleDb()
{	
	return attachment->databaseHandle;
}

void* IscConnection::startTransaction()
{
    if (transactionHandle)
        return transactionHandle;

    ISC_STATUS statusVector [20];

    char    iscTpb[5];
	int		count = sizeof( iscTpb );

    iscTpb[0] = isc_tpb_version3;
    iscTpb[1] = transactionExtInit & TRA_ro ? isc_tpb_read : isc_tpb_write;
    iscTpb[2] = transactionExtInit & TRA_nw ? isc_tpb_nowait : isc_tpb_wait;
    /* Isolation level */
    switch( transactionIsolation )
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
            iscTpb[4] = isc_tpb_rec_version;
            break;

        case 0x00000002L:
        default:
            // SQL_TXN_READ_COMMITTED:
            iscTpb[3] = isc_tpb_read_committed;
            iscTpb[4] = isc_tpb_no_rec_version;
            break;
    }

    GDS->_start_transaction( statusVector, &transactionHandle, 1, &attachment->databaseHandle,
            count, &iscTpb);

    if (statusVector [1])
        throw SQLEXCEPTION (statusVector [1], getIscStatusText (statusVector));

	if (!autoCommit)
		transactionPending = true;

    return transactionHandle;
}


Statement* IscConnection::createStatement()
{
	IscStatement *statement = new IscStatement (this);
	statements.append (statement);

	return statement;
}


Blob* IscConnection::genHTML(Properties * parameters, long genHeaders)
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

bool IscConnection::getNativeSql (const char * inStatementText, long textLength1,
								char * outStatementText, long bufferLength,
								long * textLength2Ptr)
{
	bool bModify = false;
	char * ptIn = (char*)inStatementText;
	char * ptInEnd = ptIn + textLength1;
	char * ptBeg, * ptOut = outStatementText;
	char * ptEndBracket = NULL;

#pragma FB_COMPILER_MESSAGE("IscConnection::getNativeSql - The temporary decision; FIXME!")

	while ( ptIn < ptInEnd )
	{
		if ( *ptIn == '\"' )
		{
			// replace "123" to '123'
			bool bConst = false;
			ptBeg = ptOut;
			*ptOut++ = *ptIn++;
			
			while( *ptIn == ' ')
				*ptOut++ = *ptIn++;

			if ( *ptIn >= '0' && *ptIn <= '9' )
				bConst = true;

			while( *ptIn && *ptIn != '\"' )
				*ptOut++ = *ptIn++;

			if ( *ptIn != '\"' )
				return false;
			
			if ( bConst )
				*ptBeg = *ptOut = '\'';
			else
				*ptOut = *ptIn;

			++ptIn;	++ptOut;
			bModify = true;
			continue;
		}
		else
		{
			if ( *ptIn == '{' )
				ptEndBracket = ptOut;

			*ptOut++ = *ptIn++;
		}
	}

	*ptOut = '\0';
	int ignoreBracket = 0;

	while ( ptEndBracket )
	{
		ptIn = ptEndBracket;

		ptIn++; // '{'
		
		while( *ptIn == ' ' )ptIn++;

		if ( *(long*)ptIn == 0x6c6c6163 || *(long*)ptIn == 0x4c4c4143 )
			++ignoreBracket; // { call }
		else
		{
			// Check 'oj' or 'OJ'
			if ( *(short*)ptIn == 0x6a6f || *(short*)ptIn == 0x4a4f )
				ptIn += 2; // 'oj'
			else
				ptIn += 2; // temp 'fn'

			ptOut = ptEndBracket;
			int ignoreBr = ignoreBracket;

			do
			{
				while( *ptIn && *ptIn != '}' )
					*ptOut++ = *ptIn++;

				if( ignoreBr )
					*ptOut++ = *ptIn++;

			}while ( ignoreBr-- );

			if(*ptIn != '}')
				return false;

			ptIn++; // '}'

			while( *ptIn )
				*ptOut++ = *ptIn++;

			*ptOut = '\0';
			bModify = true;
		}

		--ptEndBracket; // '{'

		while ( ptEndBracket > outStatementText && *ptEndBracket != '{')
			--ptEndBracket;

		if(*ptEndBracket != '{')
			ptEndBracket = NULL;
	}

	if ( textLength2Ptr )
		*textLength2Ptr = ptOut - outStatementText;

	return bModify;
}

DatabaseMetaData* IscConnection::getMetaData()
{
	if (metaData)
		return metaData;

	metaData = new IscDatabaseMetaData (this);

	return metaData;
}

int IscConnection::hasRole(const char * schemaName, const char * roleName)
{
	NOT_YET_IMPLEMENTED;

	return false;
}

void IscConnection::ping()
{
}

void IscConnection::createDatabase(const char * host, const char * dbName, Properties * properties)
{
}

void IscConnection::openDatabase(const char * dbName, Properties * properties)
{
	try
	{
		attachment = new Attachment;
		attachment->openDatabase (dbName, properties);
		databaseHandle = attachment->databaseHandle;
		GDS = attachment->GDS;
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

int IscConnection::objectVersion()
{
	return CONNECTION_VERSION;
}

Connection* IscConnection::clone()
{
	return new IscConnection (this);
}

void IscConnection::setAutoCommit(bool setting)
{
	if(!autoCommit && setting && transactionPending)
		commitAuto();

	autoCommit = setting;
}

bool IscConnection::getAutoCommit()
{
	return autoCommit;
}

void IscConnection::setTransactionIsolation(int level)
{
	transactionIsolation = level;
}

int IscConnection::getTransactionIsolation()
{
	return transactionIsolation;
}

void IscConnection::setExtInitTransaction(int optTpb)
{
	transactionExtInit = optTpb;
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
	FOR_OBJECTS (IscStatement*, statement, &statements)
		if (statement->selectActive)
		{
			commitRetaining();
			return;
		}
	END_FOR;

	commit();
}

void IscConnection::rollbackAuto()
{
	rollback();
}

int IscConnection::getDatabaseDialect()
{
	return attachment->getDatabaseDialect();
}

void IscConnection::commitRetaining()
{
	if (transactionHandle)
	{
		ISC_STATUS statusVector [20];
		GDS->_commit_retaining (statusVector, &transactionHandle);

		if (statusVector [1])
		{
			rollback();
			throw SQLEXCEPTION (statusVector [1], getIscStatusText (statusVector));
		}
	}
	transactionPending = false;
}
