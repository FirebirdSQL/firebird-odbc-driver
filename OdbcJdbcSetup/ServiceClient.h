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

// ServiceClient.h interface for the ServiceClient class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(_ServiceClient_H_)
#define _ServiceClient_H_

namespace OdbcJdbcSetupLibrary {

using namespace classJString;
using namespace IscDbcLibrary;

class CServiceClient
{
	enum enumRestoreExecutedPart
	{ 	
		enDomains            = 0x0001,
		enTables             = 0x0002,
		enFunctions          = 0x0004,
		enGenerators         = 0x0008,
		enStoredProcedures   = 0x0010,
		enExceptions         = 0x0020,
		enDataForTables      = 0x0040,
		enTriggers           = 0x0080,
		enPrivileges         = 0x0100,
		enSqlRoles           = 0x0200
	};

	HINSTANCE libraryHandle;

public:

	bool initServices( const char *sharedLibrary = NULL );
	bool checkVersion( void );
	bool createDatabase( void );
	void openDatabase( void );
	bool dropDatabase( void );
	void startBackupDatabase( ULONG options );
	void startRestoreDatabase( ULONG options );
	void exitRestoreDatabase( void );
	void startStaticticsDatabase( ULONG options );
	void startShowDatabaseLog( void );
	void startRepairDatabase( ULONG options, ULONG optionsValidate );
	void startUsersQuery( void );
	bool nextQuery( char *outBuffer, int length, int &lengthOut, int &countError );
	bool nextQueryLimboTransactionInfo( char *outBuffer, int length, int &lengthOut );
	bool nextQueryUserInfo( char *outBuffer, int lengthOut, int &lengthRealOut );
	void closeService();
	bool openLogFile( const char *logFileName );
	void writeLogFile( char *outBuffer );
	void putParameterValue( const char * name, const char * value );
	bool checkIncrementForBackup( char *&pt );
	bool checkIncrementForRestore( char *&pt, char *outBufferHead );
	bool checkIncrementForRepair( char *&pt );
	void openSemaphore( const char *nameSemaphore );
	void greenSemaphore( void );

public:

	CServiceClient( void );
	~CServiceClient( void );

	ServiceManager  *services;
	Properties      *properties;
	ULONG           executedPart;
	FILE            *logFile;
#ifdef _WINDOWS
	HANDLE          hSemaphore;
#endif
};
  
}; // end namespace OdbcJdbcSetupLibrary

#endif // !defined(_ServiceClient_H_)
