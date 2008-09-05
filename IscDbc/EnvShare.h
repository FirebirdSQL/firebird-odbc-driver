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
// EnvShare.h: interface for the EnvShare class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_EnvShare_H_)
#define _EnvShare_H_

#include <string.h>
#include "Connection.h"
#include "ListParamTransaction.h"

namespace IscDbcLibrary {

class IscConnection;

class EnvShare : public EnvironmentShare
{
public:
	EnvShare();
	~EnvShare();

	void	startTransaction();
	int		getCountConnection () { return countConnection; }
	void	sqlEndTran(int operation);

	bool	addConnection (IscConnection * connect);
	void	removeConnection (IscConnection * connect);

	void	clear();
	void	commit();
	void	rollback();

	void	addParamTransactionToList( CNodeParamTransaction &par );
	bool	findParamTransactionFromList( CNodeParamTransaction &par );
	JString getDatabaseServerName();

public:
	IscConnection	*connections[MAX_COUNT_DBC_SHARE];
	int				countConnection;
	isc_tr_handle	transactionHandle;

	ListParamTransaction *listTransaction;
	JString			databaseServerName;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_EnvShare_H_)
