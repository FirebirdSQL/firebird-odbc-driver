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

extern "C"
{
Connection* createConnection()
{
	return new IscConnection;
}
}

IscConnection::IscConnection()
{
	init();
}


IscConnection::IscConnection(IscConnection * source)
{
	init();
	attachment = source->attachment;
	++attachment;
}

void IscConnection::init()
{
	useCount = 1;
	metaData = NULL;
	transactionHandle = NULL;
	transactionIsolation = 0;
	autoCommit = true;
	attachment = NULL;
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
		isc_commit_transaction (statusVector, &transactionHandle);

		if (statusVector [1])
			throw SQLEXCEPTION (statusVector [1], getIscStatusText (statusVector));
		}
}

void IscConnection::rollback()
{
	if (transactionHandle)
		{
		ISC_STATUS statusVector [20];
		isc_rollback_transaction (statusVector, &transactionHandle);

		if (statusVector [1])
			throw SQLEXCEPTION (statusVector [1], getIscStatusText (statusVector));
		}
}

void IscConnection::prepareTransaction()
{
}

/* Original
void* IscConnection::startTransaction()
{
	if (transactionHandle)
		return transactionHandle;

	ISC_STATUS statusVector [20];
	isc_start_transaction (statusVector, &transactionHandle, 1, &attachment->databaseHandle, 0, NULL);

	if (statusVector [1])
		throw SQLEXCEPTION (statusVector [1], getIscStatusText (statusVector));

	return transactionHandle;
}
*/
//2002-06-08 New version suggested by CA
void* IscConnection::startTransaction()
{
    if (transactionHandle)
        return transactionHandle;

    ISC_STATUS statusVector [20];

    static char    iscTpb[5];

    iscTpb[0] = isc_tpb_version3;
    iscTpb[1] = isc_tpb_write;
    iscTpb[2] = isc_tpb_wait;
    /* Isolation level */
    switch( transactionIsolation )
    {
        case 0x00000008L:
            // SQL_TXN_SERIALIZABLE:
            iscTpb[3] = isc_tpb_consistency;
            break;

        case 0x00000004L:
            // SQL_TXN_REPEATABLE_READ:
            iscTpb[3] = isc_tpb_concurrency;
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

    isc_start_transaction( statusVector, &transactionHandle, 1, &attachment->databaseHandle,
            sizeof( iscTpb ), &iscTpb);

    if (statusVector [1])
        throw SQLEXCEPTION (statusVector [1], getIscStatusText (statusVector));

    return transactionHandle;
}


Statement* IscConnection::createStatement()
{
	IscStatement *statement = new IscStatement (this);
	statements.append (statement);

	return statement;
}


Clob* IscConnection::genHTML(Properties * parameters, long genHeaders)
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
		}
	catch (...)
		{
		delete attachment;
		attachment = NULL;
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
	char text [4096], *p = text;
	ISC_STATUS *status = statusVector;
	bool first = true;

	while (isc_interprete (p, &status))
		{
		while (*p)
			++p;
		*p++ = '\n';
		}

	if (p > text)
		--p;

	*p = 0;

	return text;
}


int IscConnection::getInfoItem(char * buffer, int infoItem, int defaultValue)
{
	for (char *p = buffer; *p != isc_info_end;)
		{
		char item = *p++;
		int length = isc_vax_integer (p, 2);
		p += 2;
		if (item == infoItem)
			return isc_vax_integer (p, length);
		p += length;
		}

	return defaultValue;			
}

JString IscConnection::getInfoString(char * buffer, int infoItem, const char * defaultString)
{
	for (char *p = buffer; *p != isc_info_end;)
		{
		char item = *p++;
		int length = isc_vax_integer (p, 2);
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
		isc_commit_retaining (statusVector, &transactionHandle);

		if (statusVector [1])
			throw SQLEXCEPTION (statusVector [1], getIscStatusText (statusVector));
		}
}
