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

namespace IscDbcLibrary {

EnvShare environmentShare;

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
		ISC_STATUS statusVector [20];
		CFbDll *GDS = connections[0]->GDS;
		struct 
		{
			isc_db_handle db;
			char *opt;
			int countOpt;
		} shDb[MAX_COUNT_DBC_SHARE] = {0};

		for ( i = 0; i < countConnection; i++ )
		{
			shDb[i].db = connections[i]->databaseHandle;
		}

		GDS->_start_transaction(statusVector, &transactionHandle, countConnection, 
			&shDb[0].db, shDb[0].countOpt, shDb[0].opt,
			&shDb[1].db, shDb[1].countOpt, shDb[1].opt,
			&shDb[2].db, shDb[2].countOpt, shDb[2].opt,
			&shDb[3].db, shDb[3].countOpt, shDb[3].opt,
			&shDb[4].db, shDb[4].countOpt, shDb[4].opt,
			&shDb[5].db, shDb[5].countOpt, shDb[5].opt,
			&shDb[6].db, shDb[6].countOpt, shDb[6].opt,
			&shDb[7].db, shDb[7].countOpt, shDb[7].opt,
			&shDb[8].db, shDb[8].countOpt, shDb[8].opt,
			&shDb[9].db, shDb[9].countOpt, shDb[9].opt
			);

		if ( statusVector [1] )
			throw SQLEXCEPTION( connections[0]->GDS->_sqlcode( statusVector ), statusVector [1], connections[0]->attachment->getIscStatusText( statusVector ) );

		for ( i = 0; i < countConnection; i++ )
			connections[i]->attachment->transactionHandle = transactionHandle;
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
		ISC_STATUS statusVector [20];
		connections[0]->GDS->_commit_transaction (statusVector, &transactionHandle);
		connections[0]->transactionInfo.transactionPending = false;

		if ( statusVector [1] )
		{
			rollback();
			throw SQLEXCEPTION( connections[0]->GDS->_sqlcode( statusVector ), statusVector [1], connections[0]->attachment->getIscStatusText( statusVector ) );
		}
	}
}

void EnvShare::rollback()
{
	if ( transactionHandle )
	{
		ISC_STATUS statusVector [20];
		connections[0]->GDS->_rollback_transaction (statusVector, &transactionHandle);
		connections[0]->transactionInfo.transactionPending = false;

		if ( statusVector [1] )
			throw SQLEXCEPTION( connections[0]->GDS->_sqlcode( statusVector ), statusVector [1], connections[0]->attachment->getIscStatusText( statusVector ) );
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
