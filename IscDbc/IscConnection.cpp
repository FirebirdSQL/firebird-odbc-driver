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

#include <time.h>
#include <string.h>
#include "IscDbc.h"
#include "IscConnection.h"
#include "IscProceduresResultSet.h"
#include "SQLError.h"
#include "IscOdbcStatement.h"
#include "IscCallableStatement.h"
#include "IscDatabaseMetaData.h"
#include "Parameters.h"
#include "Attachment.h"
#include "Mlist.h"
#include "SupportFunctions.h"
#include "SetupAttributes.h"

namespace IscDbcLibrary {

extern SupportFunctions supportFn;

extern char charTable [];

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
		statement->freeStatementHandle();
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
			if ( TRA_nw )
			{
				iscTpb[3] = isc_tpb_rec_version;
				count = 4;
			}
			else
			{
				iscTpb[3] = isc_tpb_read_committed;
				iscTpb[4] = isc_tpb_rec_version;
			}
            break;

        case 0x00000002L:
        default:
            // SQL_TXN_READ_COMMITTED:
			if ( TRA_nw )
			{
				iscTpb[3] = isc_tpb_no_rec_version;
				count = 4;
			}
			else
			{
				iscTpb[3] = isc_tpb_read_committed;
				iscTpb[4] = isc_tpb_no_rec_version;
			}
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

InternalStatement* IscConnection::createInternalStatement()
{
	IscOdbcStatement *statement = new IscOdbcStatement (this);
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

int IscConnection::buildParamProcedure ( char *& string, int numInputParam )
{
	char * ptSrc = string;

	if ( !numInputParam )
	{
		while( *ptSrc && *ptSrc != '}' )
			ptSrc++;

		if ( *ptSrc == '}' )
			memmove(string, ptSrc, strlen(ptSrc) + 1 );

		return 0;
	}

	while( *ptSrc && *ptSrc != '(' )
		ptSrc++;

	if ( *ptSrc != '(' )
		return -1;

	ptSrc++; // '('

	int i = 0;
	bool nextParam = false;
	char * ptCh;

	while( *ptSrc && i < numInputParam )
	{
		SKIP_WHITE ( ptSrc );

		if ( *ptSrc == ')' )
			return -1;

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
			while ( *ptCh && *ptCh != ',' && *ptCh != ')' )
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

			if ( *ptCh && *ptCh != delimiter && *ptCh != ',' )
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

	SKIP_WHITE(ptSrc);

	if ( *ptSrc == ')' )
	{
		if ( i < numInputParam )
		{
			int offset = (numInputParam - i) * 2;
			memmove(ptSrc + offset, ptSrc, strlen(ptSrc) + 1 );

			while( i++ < numInputParam )
			{
				*ptSrc++ = ',';
				*ptSrc++ = '?';
			}
		}
		return 0;
	}
	else if ( !*ptSrc )
		return -1;

	ptCh = --ptSrc;

//  ASSERT ( *ptCh == ',' );

	while ( *ptCh && *ptCh != ')' )
		ptCh++;

	memmove(ptSrc, ptCh, strlen(ptCh) + 1 );
	string = ptSrc + 1;

	return 0;
}

bool IscConnection::getNativeSql (const char * inStatementText, long textLength1,
								char * outStatementText, long bufferLength,
								long * textLength2Ptr)
{
	bool bModify = false;
	char * ptIn = (char*)inStatementText;
	char * ptInEnd = ptIn + textLength1;
	char * ptOut = outStatementText;
	char * ptEndBracket = NULL;

#pragma FB_COMPILER_MESSAGE("IscConnection::getNativeSql - The temporary decision; FIXME!")

	while ( ptIn < ptInEnd )
	{
		if ( *ptIn == '{' )
			ptEndBracket = ptOut;

		*ptOut++ = *ptIn++;
	}

	*ptOut = '\0';
	int ignoreBracket = 0;

	if ( !ptEndBracket )
	{
		ptOut = outStatementText;

		SKIP_WHITE ( ptOut );

		if ( !strncasecmp (ptOut, "CREATE", 6) || !strncasecmp (ptOut, "ALTER", 5) )
		{
			if ( UPPER(*ptOut) == 'A' )
				ptOut += 5;
			else
				ptOut += 6;

			while ( *ptOut )
			{
				SKIP_WHITE ( ptOut );

				if ( *ptOut )
				{
					#define LENSTR_TINYINT 7 

					if ( !strncasecmp (ptOut, "TINYINT", LENSTR_TINYINT) && IS_END_TOKEN(*(ptOut+LENSTR_TINYINT)) )
					{
						const char * nameTinyint = "CHAR CHARACTER SET OCTETS";
						int lenNameTinyint = 25;
						int offset = lenNameTinyint - LENSTR_TINYINT;

						memmove(ptOut + offset, ptOut, strlen(ptOut) + 1 );
						memcpy(ptOut, nameTinyint, lenNameTinyint);
						ptOut += lenNameTinyint;
					}
					else 
						SKIP_NO_WHITE ( ptOut );
				}
			}
		}
	}
	else
	{
		while ( ptEndBracket )
		{
			ptIn = ptEndBracket;

			ptIn++; // '{'
			
			while( *ptIn == ' ' )ptIn++;

//	On a note		++ignoreBracket; // ignored { }
			if ( *ptIn == '?' || *(long*)ptIn == 0x6c6c6163 || *(long*)ptIn == 0x4c4c4143 )
			{	// Check '?' or 'call' or 'CALL'
				if ( *ptIn == '?' )
				{
					ptIn++;
					while( *ptIn == ' ' )ptIn++;

					if(*ptIn != '=')
						return false;

					ptIn++; // '='
					while( *ptIn == ' ' )ptIn++;
				}

				if ( *(long*)ptIn != 0x6c6c6163 && *(long*)ptIn != 0x4c4c4143 )
					return false;

				ptIn += 4; // 'call'

				while( *ptIn == ' ' )ptIn++;

				ptOut = ptEndBracket;
				int ignoreBr = ignoreBracket;

#define LENSTR_EXECUTE_PROCEDURE 18 

				int offset = LENSTR_EXECUTE_PROCEDURE - ( ptIn - ptOut );

				memmove(ptOut + offset, ptOut, strlen(ptOut) + 1 );
				memcpy(ptOut, "execute procedure ", LENSTR_EXECUTE_PROCEDURE);

				ptIn += offset; 
				ptOut += LENSTR_EXECUTE_PROCEDURE;

				char procedureName[256];
				char * end = procedureName;

				SKIP_WHITE ( ptIn );

				end = procedureName;

				if ( IS_QUOTE(*ptIn) )
				{
					ptIn++;
					
					while ( !(IS_END_TOKEN(*ptIn)) )
						*end++ = *ptIn++;

					end--;

					if ( !IS_QUOTE(*end) )
						return false;
				}
				else
					while ( !(IS_END_TOKEN(*ptIn)) )
						*end++ = UPPER(*ptIn), ++ptIn;

				*end = '\0';

				int count = getCountInputParamFromProcedure ( procedureName );
				
				if ( count == -1 ) // not found
					return false;

				int ret = buildParamProcedure ( ptIn, count );
				
				if ( ret == -1 ) 
					return false;

				ptOut = ptIn;

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
			else
			{
				ptOut = ptEndBracket;

				// Check 'oj' or 'OJ'
				if ( *(short*)ptIn == 0x6a6f || *(short*)ptIn == 0x4a4f )
					ptIn += 2; // 'oj'
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
	}

	if ( textLength2Ptr )
		*textLength2Ptr = ptOut - outStatementText;

	return bModify;
}

int IscConnection::getCountInputParamFromProcedure ( const char* procedureName )
{
	int num = -1; // not found
	IscProceduresResultSet * resultSet = 
		(IscProceduresResultSet*)getMetaData()->getProcedures ( NULL, NULL, procedureName );

	if ( resultSet )
	{
		if ( resultSet->getCountRowsStaticCursor() )
			num = resultSet->sqlda->getShort(4); // NUM_INPUT_PARAM
		resultSet->release();
	}

	return num;
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
	if(!autoCommit && setting && transactionPending)
		commitAuto();

	autoCommit = setting;
}

bool IscConnection::getAutoCommit()
{
	return autoCommit;
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
	transactionIsolation = level;
}

int IscConnection::getTransactionIsolation()
{
	return transactionIsolation;
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
	transactionExtInit = optTpb;
}

int	IscConnection::getDriverBuildKey()
{
	return MAJOR_VERSION * 1000000 + MINOR_VERSION * 10000 + BUILDNUM_VERSION;
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
		if ( statement->isActiveSelect() )
		{
			commitRetaining();
			return;
		}
	END_FOR;

	commit();
}

void IscConnection::rollbackAuto()
{
	FOR_OBJECTS (IscStatement*, statement, &statements)
		if ( statement->isActiveSelect() )
		{
			rollbackRetaining();
			return;
		}
	END_FOR;

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
			rollbackRetaining();
			throw SQLEXCEPTION (statusVector [1], getIscStatusText (statusVector));
		}
	}
	transactionPending = false;
}

void IscConnection::rollbackRetaining()
{
	if (transactionHandle)
	{
		ISC_STATUS statusVector [20];
		GDS->_rollback_retaining (statusVector, &transactionHandle);

		if (statusVector [1])
		{
			rollback();
			throw SQLEXCEPTION (statusVector [1], getIscStatusText (statusVector));
		}
	}
	transactionPending = false;
}

}; // end namespace IscDbcLibrary
