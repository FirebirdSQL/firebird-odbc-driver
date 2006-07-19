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

// TransactionResourceAsync.cpp: Transaction Resource Async class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _WINDOWS

#if _MSC_VER > 1000

#define _ATL_FREE_THREADED
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <atlwin.h>

#include "txdtc.h"
#include "xolehlp.h" 
#include "txcoord.h"
#include "transact.h"

#include "OdbcEnv.h"
#include "OdbcConnection.h"
#include "TransactionResourceAsync.h"
#include "ResourceManagerSink.h"

CComModule _Module;

namespace OdbcJdbcLibrary {

HINSTANCE instanceResourceManager = NULL;

static GUID ResourceManagerGuid = 
	{ 0x63726561, 0x7465, 0x6420, { 0x62, 0x79, 0x20, 0x56, 0x6C, 0x2E, 0x54, 0x73 } };

IUnknown *transactionManager = NULL; // DTC
IResourceManagerFactory	*resourceManagerFactory = NULL;

typedef HRESULT (*DtcGetTransactionManager)( char *pszHost,
											 char *pszTmName,
											 REFIID rid,
											 DWORD	dwReserved1,
											 WORD	wcbReserved2,
											 void *pvReserved2,
											 void **ppvObject );

DtcGetTransactionManager fnDtcGetTransactionManager = NULL;

void clearAtlResource()
{
	if ( transactionManager )
	{
		transactionManager->Release();
		transactionManager = NULL;
	}

	if ( resourceManagerFactory )
	{
		resourceManagerFactory->Release();
		resourceManagerFactory = NULL;
	}
}

bool OdbcConnection::IsInstalledMsTdsInterface()
{
	if ( !instanceResourceManager )
	{
		instanceResourceManager = LoadLibrary("xolehlp.dll");
	
		if ( !instanceResourceManager )
			return false;

		fnDtcGetTransactionManager = (DtcGetTransactionManager)GetProcAddress(
															instanceResourceManager,
															"DtcGetTransactionManager" );
		if ( !fnDtcGetTransactionManager )
		{
			FreeLibrary( instanceResourceManager );
			instanceResourceManager = NULL;
			return false;
		}
	}

	return true;
}

bool OdbcConnection::enlistTransaction( SQLPOINTER transaction )
{
	HRESULT hr;

	if ( !transactionManager )
	{
		hr = fnDtcGetTransactionManager( NULL,
										 NULL,
										 IID_IUnknown,
										 0,
										 0,
										 0,
										 (LPVOID*)&transactionManager );
		if ( S_OK != hr )
			return false;

		if ( !resourceManagerFactory )
		{
			hr = transactionManager->QueryInterface( IID_IResourceManagerFactory,
													(LPVOID*)&resourceManagerFactory );
		
			if ( S_OK != hr )
				return false;
		}
	}

	if ( enlistConnect )
	{
		return false;
	}

	TransactionResourceAsync *tranResAsync = new TransactionResourceAsync;
	CComObject<ResourceManagerSink> *pSink = NULL;

	hr = CComObject<ResourceManagerSink>::CreateInstance( &pSink );

	pSink->setResourceAsync( tranResAsync );
	tranResAsync->setState( TR_ENLISTING );

	IResourceManagerSink *ptISink = NULL;
	pSink->QueryInterface( IID_IResourceManagerSink, (void **)&ptISink );

	IResourceManager *ptIResMgr = NULL;
	
	{
		GUID SinkGuid = ResourceManagerGuid;
		static int J = 0;

		SinkGuid.Data3 = ++J;

		hr = resourceManagerFactory->Create( &SinkGuid,
											"OdbcJdbcRm", 
											ptISink, 
											&ptIResMgr );
	}

	ptISink->Release();

	hr = ptIResMgr->Enlist( (ITransaction*)transaction,
							tranResAsync,
#if _MSC_VER > 1200
							(XACTUOW*)&ResourceManagerGuid,
#else
							&ResourceManagerGuid,
#endif
							&tranResAsync->isoLevel,
							&tranResAsync->enlist );

	tranResAsync->odbcConnection = this;
	enlistConnect = true;
	tranResAsync->setState( TR_ENLISTED );

	return true;
} 

TransactionResourceAsync::TransactionResourceAsync( void )
{
	enTrState = TR_NONE;
	useCount = 0;
	enlist = NULL;
	odbcConnection = NULL;
	isoLevel = 0;
}

TransactionResourceAsync::~TransactionResourceAsync( void )
{
	if ( enlist )
		enlist->Release();

	if ( odbcConnection )
		odbcConnection->enlistConnect = false;
}

STDMETHODIMP TransactionResourceAsync::PrepareRequest( BOOL fRetaining,
													   DWORD grfRM,
													   BOOL fWantMoniker,
													   BOOL fSinglePhase )
{
	HRESULT	hr = S_OK;
	TRSTATE	enTrState = getState();

	if ( TR_ENLISTED != enTrState )
	{
		setState ( TR_INVALID_STATE );

		hr = enlist->PrepareRequestDone( E_UNEXPECTED, NULL, NULL );

		if ( hr != S_OK )
			return E_FAIL;

		return E_UNEXPECTED;
	}

	setState( TR_PREPARING );

	if ( fSinglePhase == TRUE )
	{
		try
		{
			if ( odbcConnection )
				odbcConnection->connection->commitAuto();
		}
		catch (...)
		{
			if ( odbcConnection )
				odbcConnection->connection->rollbackAuto();

			hr = enlist->PrepareRequestDone( E_FAIL, NULL, NULL );
			_ASSERTE( hr == S_OK );

			setState( TR_INVALID_STATE );
			return E_FAIL;
		}

		hr = enlist->PrepareRequestDone( XACT_S_SINGLEPHASE, NULL, NULL );

		if ( hr != S_OK )
			return E_FAIL;

		return S_OK;
	}

	try
	{
		if ( odbcConnection )
			odbcConnection->connection->prepareTransaction();
	}
	catch (...)
	{
		if ( odbcConnection )
			odbcConnection->connection->rollbackAuto();

		setState( TR_ABORTED );

		hr = enlist->PrepareRequestDone( E_FAIL, NULL, NULL );
		_ASSERTE( hr == S_OK );

		return E_FAIL;
	}

	setState( TR_PREPARED );

	hr = enlist->PrepareRequestDone( S_OK, NULL, NULL );
	_ASSERTE( hr == S_OK );

	return S_OK;
}

STDMETHODIMP TransactionResourceAsync::CommitRequest( DWORD grfRM, XACTUOW *pNewUOW )
{
	HRESULT	hr = S_OK;
	TRSTATE	enTrState = getState();

	if ( TR_PREPARED != enTrState )
	{
		setState ( TR_INVALID_STATE );

		hr = enlist->PrepareRequestDone( E_UNEXPECTED, NULL, NULL );

		if ( hr != S_OK )
			return E_FAIL;

		return E_UNEXPECTED;
	}

	setState( TR_COMMITTING );

	try
	{
		if ( odbcConnection )
			odbcConnection->connection->commitAuto();
	}
	catch (...)
	{
		setState( TR_INVALID_STATE );

		hr = enlist->CommitRequestDone( E_FAIL );
		_ASSERTE( hr == S_OK );

		return E_FAIL;
	}

	setState( TR_COMMITTED );

	hr = enlist->CommitRequestDone( S_OK );
	_ASSERTE( hr == S_OK );

	return S_OK;
}

STDMETHODIMP TransactionResourceAsync::AbortRequest( BOID *pboidReason,
													 BOOL fRetaining,
													 XACTUOW *pNewUOW )
{
	HRESULT	hr = S_OK;
	TRSTATE	enTrState = getState();

	if ( TR_ENLISTED != enTrState && TR_PREPARED != enTrState )
	{
		setState ( TR_INVALID_STATE );

		hr = enlist->PrepareRequestDone( E_UNEXPECTED, NULL, NULL );

		if ( hr != S_OK )
			return E_FAIL;

		return E_UNEXPECTED;
	}

	setState( TR_ABORTING );

	try
	{
		if ( odbcConnection )
			odbcConnection->connection->rollbackAuto();
	}
	catch (...)
	{
		setState( TR_INVALID_STATE );

		hr = enlist->AbortRequestDone( E_FAIL );
		_ASSERTE( hr == S_OK );

		return E_FAIL;
	}

	setState( TR_ABORTED );

	hr = enlist->AbortRequestDone( S_OK );
	_ASSERTE( hr == S_OK );

	return  S_OK;
}

STDMETHODIMP TransactionResourceAsync::TMDown( void )
{
	setState( TR_TMDOWN );
	clearAtlResource();
	return S_OK;
}

STDMETHODIMP TransactionResourceAsync::QueryInterface( REFIID iid, LPVOID *ppv )
{
	*ppv = 0;

	if ( IID_IUnknown == iid || IID_ITransactionResourceAsync == iid )
		*ppv = this;
	else
		return ResultFromScode( E_NOINTERFACE );

	((LPUNKNOWN)*ppv)->AddRef();

	return S_OK;
}

STDMETHODIMP_ (ULONG)TransactionResourceAsync::AddRef( void )
{
	return ++useCount;
}

STDMETHODIMP_ (ULONG)TransactionResourceAsync::Release( void )
{
	if ( --useCount )
		return useCount;

	delete this;
	return 0;
}

}; // end namespace OdbcJdbcLibrary

#endif // _MSC_VER > 1000

#endif // _WINDOWS
