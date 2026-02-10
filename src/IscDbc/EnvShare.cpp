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
 *  Copyright (c) 2004 Vladimir Tsvigun
 *  All Rights Reserved.
 */

//  
// EnvShare.cpp: EnvShare class.
//
//////////////////////////////////////////////////////////////////////

#include "stdio.h"
#include <stdlib.h>
#ifndef _WINDOWS
#include <unistd.h>
#endif
#include "IscDbc.h"
#include "SQLError.h"
#include "EnvShare.h"
#include "IscConnection.h"
#include "Attachment.h"

using namespace Firebird;

namespace IscDbcLibrary {

// Construct-on-first-use idiom to avoid static initialization order fiasco.
// Returns a reference to the singleton EnvShare instance, which is created
// the first time this function is called (thread-safe in C++11+).
EnvShare& getEnvironmentShareInstance()
{
	static EnvShare instance;
	return instance;
}

EnvShare::EnvShare()
{
	listTransaction = NULL;
	clear();
}

EnvShare::~EnvShare()
{
	delete listTransaction;
}

void EnvShare::clear()
{
	memset( connections, 0, sizeof (connections) );
	countConnection = 0;
	transactionHandle = NULL;
}

bool EnvShare::addConnection (IscConnection * connect)
{
	if ( countConnection >= MAX_COUNT_DBC_SHARE )
		return false;
	
	int n = countConnection;

	while ( n-- )
		if ( connections[n] == connect )
			return true;

	connections[countConnection++] = connect;
	return true;
}

void EnvShare::removeConnection (IscConnection * connect)
{
	for (int i = 0; i < countConnection; i++ )
		if ( connections[i] == connect )
		{
			if ( countConnection - i )
				memmove (&connections[i], &connections[i+1], sizeof(connections[0]) * (countConnection - i) );
			countConnection--;
			break;
		}
}

void EnvShare::startTransaction()
{
	if ( transactionHandle )
		return;

	if ( countConnection )
	{
		int i;
		CFbDll *GDS = connections[0]->GDS;

		IDtc* dtc = GDS->_master->getDtc();
		ThrowStatusWrapper status( GDS->_status );
		IDtcStart* dtcBuilder = nullptr;
		try
		{
			dtcBuilder = dtc->startBuilder( &status );

			for( i = 0; i < countConnection; i++ ) {
				dtcBuilder->addAttachment( &status, connections[i]->databaseHandle );
			}

			transactionHandle = dtcBuilder->start( &status );

			for ( i = 0; i < countConnection; i++ )
				connections[i]->attachment->transactionHandle = transactionHandle;

			dtcBuilder->dispose();
			dtcBuilder = nullptr;
		}
		catch( const FbException& error )
		{
			if( dtcBuilder ) dtcBuilder->dispose();
			const ISC_STATUS * statusVector = error.getStatus()->getErrors();
			throw SQLEXCEPTION( GDS->getSqlCode( statusVector ), statusVector [1], GDS->getIscStatusText( error.getStatus() ) );
		}
	}
}

void EnvShare::sqlEndTran(int operation)
{
	switch (operation)
	{
	case 0: // SQL_COMMIT
		commit();
		break;

	case 1: // SQL_ROLLBACK
		rollback();
	}
}

void EnvShare::commit()
{
	if ( transactionHandle )
	{
		CFbDll *GDS = connections[0]->GDS;
		ThrowStatusWrapper status( GDS->_status );
		try
		{
			transactionHandle->commit( &status );
			transactionHandle = nullptr;
			connections[0]->transactionInfo.transactionPending = false;
		}
		catch( const FbException& error )
		{
			rollback();
			const ISC_STATUS * statusVector = error.getStatus()->getErrors();
			throw SQLEXCEPTION( GDS->getSqlCode( statusVector ), statusVector [1], GDS->getIscStatusText( error.getStatus() ) );
		}
	}
}

void EnvShare::rollback()
{
	if ( transactionHandle )
	{
		CFbDll *GDS = connections[0]->GDS;
		ThrowStatusWrapper status( GDS->_status );
		try
		{
			transactionHandle->rollback( &status );
			transactionHandle = nullptr;
			connections[0]->transactionInfo.transactionPending = false;
		}
		catch( const FbException& error )
		{
			if( transactionHandle ) {
				transactionHandle->release();
				transactionHandle = nullptr;
			}
			const ISC_STATUS * statusVector = error.getStatus()->getErrors();
			throw SQLEXCEPTION( GDS->getSqlCode( statusVector ), statusVector [1], GDS->getIscStatusText( error.getStatus() ) );
		}
	}
}

void EnvShare::addParamTransactionToList( CNodeParamTransaction &par )
{
	int node;

	if ( !listTransaction )
		listTransaction = new ListParamTransaction( 5 );

	node = listTransaction->SearchAndInsert( &par );
	if ( node < 0 ) node = ~node;
	(*listTransaction)[node] = par;
}

bool EnvShare::findParamTransactionFromList( CNodeParamTransaction &par )
{
	if ( !listTransaction )
		return false;

	int node = listTransaction->Search( &par );
	if ( node == ~0 )
		return false;

	par = (*listTransaction)[node];
	return true;
}

JString EnvShare::getDatabaseServerName()
{
	if ( databaseServerName.IsEmpty() )
	{
		ULONG nSize = 256;
#ifdef _WINDOWS
		GetComputerName( databaseServerName.getBuffer( nSize ), &nSize );
#else
		gethostname( databaseServerName.getBuffer( nSize ), nSize );
#endif
	}

	return databaseServerName;
}

}; // end namespace IscDbcLibrary
