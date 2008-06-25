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

// ResourceManagerSink.cpp: Resource Manager Sink class.
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
#include "TransactionResourceAsync.h"
#include "ResourceManagerSink.h"

namespace OdbcJdbcLibrary {

ResourceManagerSink::ResourceManagerSink( void )
{
	tranResourceAsync = NULL;
}

ResourceManagerSink::~ResourceManagerSink( void )
{

}

STDMETHODIMP ResourceManagerSink::TMDown( void )
{
	if ( tranResourceAsync )
		return tranResourceAsync->TMDown();

	return E_UNEXPECTED;
}

void ResourceManagerSink::setResourceAsync( TransactionResourceAsync * ptResAsync )
{ 
	tranResourceAsync = ptResAsync;
}

}; // end namespace OdbcJdbcLibrary

#endif // _MSC_VER > 1000

#endif // _WINDOWS
