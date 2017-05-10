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
	services = NULL;
	properties = NULL;
	logFile = NULL;
	executedPart = 0;
#ifdef _WINDOWS
	hSemaphore = NULL;
#endif
}

CServiceClient::~CServiceClient()
{
#ifdef _WINDOWS
	if ( hSemaphore )
		CloseHandle( hSemaphore );
#endif

	if ( logFile )
		fclose( logFile );

	if ( properties )
		properties->release();

	if ( services )
	{
		services->closeService();
		services->release();
	}
}

bool CServiceClient::initServices( const char *sharedLibrary )
{
	try
	{
		services = createServices();

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

bool CServiceClient::openLogFile( const char *logFileName )
{
	logFile = fopen( logFileName, "wt" );
	if ( !logFile )
		return false;

	return true;
}

void CServiceClient::writeLogFile( char *outBuffer )
{
	if ( logFile )
	{
		fputs( outBuffer, logFile );
		fputs( "\n", logFile );
		fflush( logFile );
	}
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
		connection = createConnection();
		connection->createDatabase( properties->findValue( SETUP_DBNAME, NULL ),
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

void CServiceClient::openDatabase()
{
	Connection *connection;

	try
	{
		connection = createConnection();
		connection->openDatabase( properties->findValue( SETUP_DBNAME, NULL ),
								  properties );
		connection->close();
	}
	catch ( std::exception )
	{
		if ( connection )
			connection->close();
		throw;
	}
}

bool CServiceClient::dropDatabase()
{
	Connection *connection;

	try
	{
		connection = createConnection();
		connection->openDatabase( properties->findValue( SETUP_DBNAME, NULL ),
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
	executedPart = 0;
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

void CServiceClient::startUsersQuery()
{
	services->startUsersQuery( properties );
}

bool CServiceClient::nextQuery( char *outBuffer, int length, int &lengthOut, int &countError )
{
	return services->nextQuery( outBuffer, length, lengthOut, countError );
}

bool CServiceClient::nextQueryLimboTransactionInfo( char *outBuffer, int length, int &lengthOut )
{
	return services->nextQueryLimboTransactionInfo( outBuffer, length, lengthOut );
}

bool CServiceClient::nextQueryUserInfo( char *outBuffer, int length, int &lengthOut )
{
	return services->nextQueryUserInfo( outBuffer, length, lengthOut );
}

void CServiceClient::closeService()
{
	services->closeService();
}

void CServiceClient::openSemaphore( const char *nameSemaphore )
{
#ifdef _WINDOWS
	hSemaphore = OpenSemaphore( SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, FALSE, nameSemaphore );
	if ( !hSemaphore ) 
		hSemaphore = CreateSemaphore( NULL, 0, 1, nameSemaphore );
#endif
}

void CServiceClient::greenSemaphore()
{
#ifdef _WINDOWS
	ReleaseSemaphore( hSemaphore, 1, NULL );
#endif
}

bool CServiceClient::checkIncrementForBackup( char *&pt )
{
	bool inc = false;
	
	pt += 6; // offset 'gbak: '

	if ( *pt == 'w' && !strncasecmp ( pt, "writing ", sizeof ( "writing " ) - 1 ) )
	{
		pt += sizeof ( "writing " ) - 1;

		switch ( *pt )
		{
		case 'd':
			inc = !strncasecmp ( pt, "domains", sizeof ( "domains" ) - 1 );
			break;
		case 's':
			inc = !strncasecmp ( pt, "shadow files", sizeof ( "shadow files" ) - 1 )
				  || !strncasecmp ( pt, "stored procedures", sizeof ( "stored procedures" ) - 1 );
			break;
		case 't':
			inc = !strncasecmp ( pt, "tables", sizeof ( "tables" ) - 1 )
				  || !strncasecmp ( pt, "types", sizeof ( "types" ) - 1 )
				  || !strncasecmp ( pt, "triggers", sizeof ( "triggers" ) - 1 )
				  || !strncasecmp ( pt, "trigger messages", sizeof ( "trigger messages" ) - 1 )
				  || !strncasecmp ( pt, "table constraints", sizeof ( "table constraints" ) - 1 );
			break;
		case 'f':
			inc = !strncasecmp ( pt, "functions", sizeof ( "functions" ) - 1 )
				  || !strncasecmp ( pt, "filters", sizeof ( "filters" ) - 1 );
			break;
		case 'i':
			inc = !strncasecmp ( pt, "id generators", sizeof ( "id generators" ) - 1 );
			break;
		case 'e':
			inc = !strncasecmp ( pt, "exceptions", sizeof ( "exceptions" ) - 1 );
			break;
		case 'C':
			inc = !strncasecmp ( pt, "Character Sets", sizeof ( "Character Sets" ) - 1 )
				  || !strncasecmp ( pt, "Collations", sizeof ( "Collations" ) - 1 );
			break;
		case 'r':
			inc = !strncasecmp ( pt, "referential constraints", sizeof ( "referential constraints" ) - 1 );
			break;
		case 'c':
			inc = !strncasecmp ( pt, "check constraints", sizeof ( "check constraints" ) - 1 );
			break;
		case 'S':
			inc = !strncasecmp ( pt, "SQL roles", sizeof ( "SQL roles" ) - 1 );
			break;
		}
	}
	return inc;
}

bool CServiceClient::checkIncrementForRestore( char *&pt, char *outBufferHead )
{
	bool inc = false;
	
	pt += 6; // offset 'gbak: '
	*outBufferHead = '\0';

	if ( *pt == 'r' && !strncasecmp ( pt, "restoring ", sizeof ( "restoring " ) - 1 ) )
	{
		pt += sizeof ( "restoring " ) - 1;

		switch ( *pt )
		{
		case 'd':
			if ( !(executedPart & enDomains)
				&& !strncasecmp ( pt, "domain", sizeof ( "domain" ) - 1 ) )
			{
				executedPart |= enDomains;
				strcpy( outBufferHead, "<UL><B>Domains</UL></B>" );
				inc = true;
			}
			else if ( !(executedPart & enDataForTables)
				&& !strncasecmp ( pt, "data for table", sizeof ( "data for table" ) - 1 ) )
			{
				executedPart |= enDataForTables;
				strcpy( outBufferHead, "<UL><B>Data For Tables</UL></B>" );
				inc = true;
			}
			break;
		case 's':
			if ( !(executedPart & enStoredProcedures)
				&& !strncasecmp ( pt, "stored procedure", sizeof ( "stored procedure" ) - 1 ) )
			{
				executedPart |= enStoredProcedures;
				strcpy( outBufferHead, "<UL><B>Stored Procedures</UL></B>" );
				inc = true;
			}
			break;
		case 'S':
			if ( !(executedPart & enSqlRoles)
				&& !strncasecmp ( pt, "SQL role", sizeof ( "SQL role" ) - 1 ) )
			{
				executedPart |= enSqlRoles;
				strcpy( outBufferHead, "<UL><B>SQL Roles</UL></B>" );
				inc = true;
			}
			break;
		case 't':
			if ( !(executedPart & enTables)
				&& !strncasecmp ( pt, "table", sizeof ( "table" ) - 1 ) )
			{
				executedPart |= enTables;
				strcpy( outBufferHead, "<UL><B>Tables</UL></B>" );
				inc = true;
			}
			break;
		case 'f':
			if ( !(executedPart & enFunctions)
				&& !strncasecmp ( pt, "function", sizeof ( "function" ) - 1 ) )
			{
				executedPart |= enFunctions;
				strcpy( outBufferHead, "<UL><B>Functions</UL></B>" );
				inc = true;
			}
			break;
		case 'e':
			if ( !(executedPart & enExceptions)
				&& !strncasecmp ( pt, "exception", sizeof ( "exception" ) - 1 ) )
			{
				executedPart |= enExceptions;
				strcpy( outBufferHead, "<UL><B>Exceptions</UL></B>" );
				inc = true;
			}
			break;
		case 'g':
			if ( !(executedPart & enGenerators)
				&& !strncasecmp ( pt, "generator", sizeof ( "generator" ) - 1 ) )
			{
				executedPart |= enGenerators;
				strcpy( outBufferHead, "<UL><B>Generators</UL></B>" );
				inc = true;
			}
			break;
		}
	}
	else if ( *pt == 'c' && !strncasecmp ( pt, "creating indexes ", sizeof ( "creating indexes " ) - 1 ) )
	{
		strcpy( outBufferHead, "<UL><B>Creating Indexes</UL></B>" );
		inc = true;
	}
	else if ( *(pt + 4) == 'r' && !strncasecmp ( pt + 4, "restoring ", sizeof ( "restoring " ) - 1 ) )
	{
		pt += sizeof ( "restoring " ) - 1 + 4;

		switch ( *pt )
		{
		case 't':
			if ( !(executedPart & enTriggers)
				&& !strncasecmp ( pt, "trigger", sizeof ( "trigger" ) - 1 ) )
			{
				executedPart |= enTriggers;
				strcpy( outBufferHead, "<UL><B>Triggers</UL></B>" );
				inc = true;
			}
			break;
		case 'p':
			if ( !(executedPart & enPrivileges)
				&& !strncasecmp ( pt, "privilege", sizeof ( "privilege" ) - 1 ) )
			{
				executedPart |= enPrivileges;
				strcpy( outBufferHead, "<UL><B>Privileges</UL></B>" );
				inc = true;
			}
			break;
		}
	}

	return inc;
}

bool CServiceClient::checkIncrementForRepair( char *&pt )
{
	return true;
}

}; // end namespace OdbcJdbcSetupLibrary
