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

// ServiceClient.cpp: Service Client Manager class.
//
//////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "OdbcJdbcSetup.h"
#include "../IscDbc/Connection.h"
#include <odbcinst.h>
#include "DsnDialog.h"
#include "Setup.h"
#include "../SetupAttributes.h"
#include "../SecurityPassword.h"
#include "ServiceClient.h"

namespace OdbcJdbcSetupLibrary {

using namespace classJString;
using namespace IscDbcLibrary;

CServiceClient::CServiceClient()
{
	libraryHandle = NULL;
	services = NULL;
	properties = NULL;
}

CServiceClient::~CServiceClient()
{
	if ( properties )
		properties->release();

	if ( services )
	{
		services->closeService();
		services->release();
	}

	if ( libraryHandle )
		CLOSE_SHARE_LIBLARY( libraryHandle );
}

bool CServiceClient::initServices( const char *sharedLibrary )
{
	try
	{
		if ( !sharedLibrary )
			sharedLibrary = DEFAULT_DRIVER;

		libraryHandle = OPEN_SHARE_LIBLARY( sharedLibrary );
		if ( !libraryHandle )
			return false;

		ServiceManagerFn fn = (ServiceManagerFn)GET_ENTRY_POINT( libraryHandle, ENTRY_DLL_CREATE_SERVICES );

		if ( !fn )
			return false;

		services = (fn)();

		if ( !services )
			return false;

		properties = services->allocProperties();

		if ( !properties )
			return false;
	}
	catch ( std::exception &ex )
	{
		SQLException &exception = (SQLException&)ex;
		JString text = exception.getText();

		if ( services )
		{
			services->release();
			services = NULL;
			return false;
		}
	}

	return true;
}

bool CServiceClient::checkVersion()
{
	if ( DRIVER_BUILD_KEY != services->getDriverBuildKey() )
		return false;

	return true;
}

void CServiceClient::putParameterValue( const char * name, const char * value )
{
	properties->putValue( name, value );
}

bool CServiceClient::createDatabase()
{
	Connection *connection;

	try
	{
		ConnectFn fn = (ConnectFn)GET_ENTRY_POINT( libraryHandle, ENTRY_DLL_CREATE_CONNECTION );
		if (!fn)
			return false;

		connection = (fn)();
		connection->openDatabase ( properties->findValue( SETUP_DBNAME, NULL ),
								   properties );
		connection->close();
	}
	catch ( std::exception &ex )
	{
		SQLException &exception = (SQLException&)ex;
		JString text = exception.getText();

		if ( connection )
			connection->close();

		return false;
	}

	return true;
}

void CServiceClient::startBackupDatabase( ULONG options )
{
	services->startBackupDatabase( properties, options );
}

void CServiceClient::startRestoreDatabase( ULONG options )
{
	services->startRestoreDatabase( properties, options );
}

void CServiceClient::exitRestoreDatabase()
{
	services->exitRestoreDatabase();
}

void CServiceClient::startStaticticsDatabase( ULONG options )
{
	services->startStaticticsDatabase( properties, options );
}

void CServiceClient::startShowDatabaseLog()
{
	services->startShowDatabaseLog( properties );
}

void CServiceClient::startRepairDatabase( ULONG options, ULONG optionsValidate )
{
	services->startRepairDatabase( properties, options, optionsValidate );
}

bool CServiceClient::nextQuery( char *outBuffer, int length, int &lengthOut )
{
	return services->nextQuery( outBuffer, length, lengthOut );
}

bool CServiceClient::nextQueryLimboTransactionInfo( char *outBuffer, int length, int &lengthOut )
{
	return services->nextQueryLimboTransactionInfo( outBuffer, length, lengthOut );
}

void CServiceClient::closeService()
{
	services->closeService();
}

}; // end namespace OdbcJdbcSetupLibrary
