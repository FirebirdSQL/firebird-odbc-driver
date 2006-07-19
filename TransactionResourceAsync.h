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

// TransactionResourceAsync.h interface for the TransactionResourceAsync class.
//
//////////////////////////////////////////////////////////////////////
#ifdef _WINDOWS

#if _MSC_VER > 1000

#if !defined(_TransactionResourceAsync_H_)
#define _TransactionResourceAsync_H_

namespace OdbcJdbcLibrary {

enum TRSTATE
{
	TR_NONE,
	TR_ENLISTING,
	TR_ENLISTED,
	TR_PREPARING,
	TR_PREPARED,
	TR_COMMITTING,
	TR_COMMITTED,
	TR_ABORTING,
	TR_ABORTED,
	TR_TMDOWN,
	TR_INVALID_STATE
};

class OdbcConnection;
 
class TransactionResourceAsync : public ITransactionResourceAsync
{
private:
	ULONG useCount;
	TRSTATE enTrState;
public:
	ITransactionEnlistmentAsync	*enlist;
	LONG isoLevel;
	OdbcConnection	*odbcConnection;

public:
	TransactionResourceAsync();
	~TransactionResourceAsync();

	STDMETHODIMP PrepareRequest( BOOL fRetaining, DWORD grfRM, BOOL fWantMoniker, BOOL fSinglePhase );
	STDMETHODIMP CommitRequest( DWORD grfRM, XACTUOW *pNewUOW );
	STDMETHODIMP AbortRequest( BOID *pboidReason, BOOL fRetaining, XACTUOW *pNewUOW );
	STDMETHODIMP TMDown( void );

	STDMETHODIMP QueryInterface( REFIID iid, LPVOID FAR* ppv );
	STDMETHODIMP_ (ULONG)AddRef( void );
	STDMETHODIMP_ (ULONG)Release( void );

public:
	TRSTATE getState( void ) { return enTrState; }
	void setState( TRSTATE enState ) { enTrState = enState; }

};

}; // end namespace OdbcJdbcLibrary

#endif // !defined(_TransactionResourceAsync_H_)

#endif // _MSC_VER > 1000

#endif // _WINDOWS
