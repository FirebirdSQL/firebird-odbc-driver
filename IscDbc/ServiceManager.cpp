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
 *  Copyright (c) 2005 Vladimir Tsvigun
 *  All Rights Reserved.
 */

// ServiceManager.cpp: implementation of the ServiceManager class.
//
//////////////////////////////////////////////////////////////////////

#include <memory.h>
#include <stdlib.h>
#include "IscDbc.h"
#include "IscConnection.h"
#include "SQLError.h"
#include "Parameters.h"
#include "../SetupAttributes.h"
#include "ServiceManager.h"

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern "C" ServiceManager* createServices()
{
	return new CServiceManager;
}

CServiceManager::CServiceManager()
{
	useCount = 1;
	GDS = NULL;
	properties = NULL;
}

CServiceManager::~CServiceManager()
{
	if( GDS )
		delete GDS;
}

Properties* CServiceManager::allocProperties()
{
	return new Parameters;
}

int	CServiceManager::getDriverBuildKey()
{
	return DRIVER_BUILD_KEY;
}

void CServiceManager::loadShareLibrary()
{
	const char *clientDefault = NULL;
	const char *client = properties->findValue( SETUP_CLIENT, NULL );

	if ( !client || !*client )
		client = NAME_CLIENT_SHARE_LIBRARY,
		clientDefault = NAME_DEFAULT_CLIENT_SHARE_LIBRARY;

	GDS = new CFbDll();
	if ( !GDS->LoadDll( client, clientDefault ) )
	{
		JString text;
		text.Format( "Unable to connect to data source: library '%s' failed to load", client );
		throw SQLEXCEPTION( -904, 335544375l, text );
	}
}

void CServiceManager::addRef()
{
	++useCount;
}

int CServiceManager::release()
{
	if ( !--useCount )
		delete this;

	return useCount;
}

}; // end namespace IscDbcLibrary
