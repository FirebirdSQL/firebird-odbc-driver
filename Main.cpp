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
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 *
 *
 *
 *	2003-03-24	main.cpp
 *				Contributed by Roger Gammans
 *				Fix SQLError prototype to match the prototype
 *				in the sql.h header file. This stops g++ 
 *				generating SQLError as a C++ name-mangled 
 *				entry point. A C function entry is used 
 *				allowing correct binding to the driver manager.
 *
 *	2002-10-11	main.cpp
 *				Contributed by C G Alvarez
 *              Implement SQLNumParams()
 *
 *	2002-10-11	main.cpp
 *				Contributed by C G Alvarez
 *              Implement SQLTablePrivileges()
 *
 *  2002-10-11  main.cpp
 *				Contributed by C G Alvarez
 *				Implement SQLColumnPrivileges()
 *
 *	2002-10-11	main.cpp
 *				Contributed by C G Alvarez
 *              Implement SQLGetDescField()
 *
 *  2002-08-02  main.cpp
 *				Contributed by C G Alvarez
 *				Implement SQLGetEnvAttr()
 *
 *
 *	2002-07-02	main.cpp
 *				Contributed by C G Alvarez
 *				Mapped calls to SQLGetConnectOption to SQLGetConnectAttr
 *				Mapped calls SQLGetStmtOption to SQLGetStmtAttr
 *				Mapped calls SQLSetStmtOption to SQLSetStmtAttr

 *
 *	2002-04-30  main.cpp
 *				Added suggestions by Carlos G Alvarez 
 *				o Test for logfile before trying to close it.
 *				o Changed parameter types for SQLSetConnectOption
 *	
 */

#ifdef _WINDOWS
#include <windows.h>
#endif
#include <stdio.h>
#include <locale.h>
#include "OdbcJdbc.h"
#include "OdbcEnv.h"
#include "OdbcConnection.h"
#include "OdbcStatement.h"
#include "SafeEnvThread.h"
#include "Main.h"

#ifdef LOGGING
FILE	*logFile = NULL;
void logMsg (const char *msg)
{
	if (!logFile)
		logFile = fopen( LOG_FILE, "a+" );
	if (!logFile)
		OutputDebugString ("log file create failed\n");
	else
	{
		fputs (msg, logFile);
		OutputDebugString (msg);
	}
}
#endif
		
void notYetImplemented (const char *msg)
{
    LOG_MSG (msg);
}

void trace (const char *msg)
{
    LOG_MSG(msg);
}

using namespace OdbcJdbcLibrary;

#ifdef _WINDOWS
HINSTANCE m_hInstance = NULL;
UINT codePage = CP_ACP;

namespace OdbcJdbcLibrary {

#if _MSC_VER > 1000
void clearAtlResource();
#endif // _MSC_VER > 1000
void initCodePageTranslate(  int userLCID );

};

BOOL APIENTRY DllMainSetup(  HINSTANCE hinstDLL, DWORD fdwReason, LPVOID );

BOOL APIENTRY DllMain(  HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
	if ( fdwReason == DLL_PROCESS_ATTACH )
	{
		m_hInstance = hinstDLL;
		codePage = GetACP();
		initCodePageTranslate( GetUserDefaultLCID() );
		setlocale( LC_ALL, ".ACP" );
	}
	else if ( fdwReason == DLL_PROCESS_DETACH )
	{
#if _MSC_VER > 1000
		clearAtlResource();
#endif // _MSC_VER > 1000
	}

	DllMainSetup( hinstDLL, fdwReason, lpvReserved );

    return TRUE;
}
#endif

static SQLRETURN __SQLAllocHandle( SQLSMALLINT handleType, SQLHANDLE inputHandle, 
								SQLHANDLE *outputHandle )
{
	TRACE ("__SQLAllocHandle");

	if ( handleType == SQL_HANDLE_ENV )
	{
		if ( inputHandle != SQL_NULL_HANDLE || outputHandle == NULL)
			return SQL_ERROR;

		*outputHandle = (SQLHANDLE)new OdbcEnv;

		return SQL_SUCCESS;
	}

	OdbcObject *object = (OdbcObject*)inputHandle;

	if ( outputHandle == NULL )
		return object->sqlReturn (SQL_ERROR, "HY009", "Invalid use of null pointer");

	return object->allocHandle( handleType, outputHandle );
}

///// SQLAllocConnect /////	ODBC 1.0	///// Deprecated

SQLRETURN SQL_API SQLAllocConnect( SQLHENV hEnv, SQLHDBC *hDbc )
{
	TRACE ("SQLAllocConnect");
	GUARD_ENV( hEnv );

	return __SQLAllocHandle( SQL_HANDLE_DBC, hEnv, hDbc );
}

///// SQLAllocEnv /////		ODBC 1.0	///// Deprecated

SQLRETURN SQL_API SQLAllocEnv( SQLHENV *hEnv )
{
	TRACE ("SQLAllocEnv");
	GUARD;

	return __SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, hEnv );
}

///// SQLAllocStmt /////	ODBC 1.0	///// Deprecated

SQLRETURN SQL_API SQLAllocStmt( SQLHDBC hDbc, SQLHSTMT *hStmt )
{
	TRACE ("SQLAllocStmt");
	GUARD_HDBC( hDbc );

	return __SQLAllocHandle( SQL_HANDLE_STMT, hDbc, hStmt );
}

///// SQLBindCol /////	ODBC 1.0	///// ISO 92

SQLRETURN SQL_API SQLBindCol( SQLHSTMT hStmt, SQLUSMALLINT columnNumber, 
						   SQLSMALLINT targetType, SQLPOINTER targetValue, 
						   SQLLEN bufferLength, SQLLEN *strLen_or_Ind )
{
	TRACE ("SQLBindCol");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlBindCol( columnNumber, targetType, targetValue, 
												bufferLength, strLen_or_Ind );
}

///// SQLCancel /////	ODBC 1.0	///// Deprecated

SQLRETURN SQL_API SQLCancel( SQLHSTMT hStmt )
{
	TRACE ("SQLCancel");

	return ((OdbcStatement*) hStmt)->sqlCancel();
}

///// SQLColAttributes /////	ODBC 1.0	///// Deprecated

SQLRETURN SQL_API SQLColAttributes( SQLHSTMT hStmt, SQLUSMALLINT columnNumber,
								 SQLUSMALLINT fieldIdentifier, SQLPOINTER characterAttribute,
								 SQLSMALLINT bufferLength, SQLSMALLINT *stringLength,
								 SQLLEN *numericAttribute )
{
	TRACE("SQLColAttributes");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlColAttribute( columnNumber, fieldIdentifier,
													characterAttribute, bufferLength,
													stringLength, numericAttribute );
}

///// SQLConnect /////	ODBC 1.0	///// ISO 92

SQLRETURN SQL_API SQLConnect( SQLHDBC hDbc,
						   SQLCHAR *serverName, SQLSMALLINT nameLength1,
						   SQLCHAR *userName, SQLSMALLINT nameLength2,
						   SQLCHAR *authentication, SQLSMALLINT nameLength3 )
{
	TRACE ("SQLConnect");
	GUARD_HDBC( hDbc );

	SQLRETURN ret = ((OdbcConnection*) hDbc)->sqlConnect( serverName, nameLength1, userName,
												nameLength2, authentication, nameLength3 );
	LOG_PRINT(( logFile, 
				"SQLConnect            : Line %d\n"
				"   +status            : %d\n"
				"   +hDbc              : %p\n"
				"   +serverName        : %s\n"
				"   +userName          : %s\n"
				"   +authentication    : %s\n\n",
					__LINE__,
					ret,
					hDbc,
					serverName ? serverName : (SQLCHAR*)"",
					userName ? userName : (SQLCHAR*)"",
					authentication ? authentication : (SQLCHAR*)"" ));
	return ret;
}

///// SQLDescribeCol /////	ODBC 1.0	///// ISO 92

SQLRETURN SQL_API SQLDescribeCol( SQLHSTMT hStmt, SQLUSMALLINT columnNumber,
							   SQLCHAR *columnName, SQLSMALLINT bufferLength, 
							   SQLSMALLINT *nameLength, SQLSMALLINT *dataType, 
							   SQLULEN *columnSize, SQLSMALLINT *decimalDigits,
							   SQLSMALLINT *nullable )
{
	TRACE ("SQLDescribeCol");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlDescribeCol( columnNumber, columnName, bufferLength, 
												nameLength, dataType, columnSize, decimalDigits,
												nullable );
}

///// SQLDisconnect /////	ODBC 1.0	///// ISO 92

SQLRETURN SQL_API SQLDisconnect( SQLHDBC hDbc )
{
	TRACE ("SQLDisconnect");
	GUARD_ENV( ((OdbcConnection*) hDbc)->env );

	return ((OdbcConnection*) hDbc)->sqlDisconnect();
}

///// SQLError /////	ODBC 1.0	///// Deprecated

SQLRETURN SQL_API SQLError( SQLHENV hEnv,
						   SQLHDBC hDbc, SQLHSTMT hStmt,
						   SQLCHAR *sqlState, SQLINTEGER *nativeErrorCode,
						   SQLCHAR *msgBuffer, SQLSMALLINT msgBufferLength,
						   SQLSMALLINT *msgLength )
{
	TRACE("SQLError");

	if ( hStmt )
	{
		GUARD_HSTMT( hStmt );
		return ((OdbcStatement*)hStmt)->sqlError( sqlState, nativeErrorCode, msgBuffer,
													msgBufferLength, msgLength );
	}
	if ( hDbc )
	{
		GUARD_HDBC( hDbc );
		return ((OdbcConnection*)hDbc)->sqlError( sqlState, nativeErrorCode, msgBuffer,
													msgBufferLength, msgLength );
	}
	if ( hEnv )
		return ((OdbcEnv*)hEnv)->sqlError( sqlState, nativeErrorCode, msgBuffer, 
													msgBufferLength, msgLength );

	return SQL_ERROR;
}

///// SQLExecDirect /////	ODBC 1.0	///// ISO 92

SQLRETURN SQL_API SQLExecDirect( SQLHSTMT hStmt, SQLCHAR *statementText, SQLINTEGER textLength )
{
	TRACE ("SQLExecDirect");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlExecDirect( statementText, textLength );
}

///// SQLExecute /////	ODBC 1.0	///// ISO 92

SQLRETURN SQL_API SQLExecute( SQLHSTMT hStmt )
{
	TRACE("SQLExecute");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlExecute();
}

///// SQLFetch /////	ODBC 1.0	///// ISO 92

SQLRETURN SQL_API SQLFetch( SQLHSTMT hStmt )
{
	TRACE ("SQLFetch");
	GUARD_HSTMT( hStmt );
	
	return ((OdbcStatement*) hStmt)->sqlFetch();
}

///// SQLFreeConnect /////	ODBC 1.0	///// Deprecated

SQLRETURN SQL_API SQLFreeConnect( SQLHDBC hDbc )
{
	TRACE ("SQLFreeconnect");
	GUARD_HDBC( hDbc );

	delete (OdbcConnection*) hDbc;
	return SQL_SUCCESS;
}

///// SQLFreeEnv /////	ODBC 3.0	///// ISO 92

SQLRETURN SQL_API SQLFreeEnv( SQLHENV hEnv )
{
	TRACE ("SQLFreeEnv");

	delete (OdbcEnv*) hEnv;
	return SQL_SUCCESS;
}

///// SQLFreeStmt /////	ODBC 1.0	///// ISO 92

SQLRETURN SQL_API SQLFreeStmt( SQLHSTMT hStmt, SQLUSMALLINT option )
{
	TRACE ("SQLFreeStmt");
	GUARD_HSTMT( hStmt );

	if ( option == SQL_DROP )
	{
		delete (OdbcStatement*) hStmt;
		return SQL_SUCCESS;
	}

	return ((OdbcStatement*) hStmt)->sqlFreeStmt( option );
}

///// SQLGetCursorName /////	ODBC 1.0	///// ISO 92

SQLRETURN SQL_API SQLGetCursorName( SQLHSTMT hStmt, SQLCHAR *cursorName,
								 SQLSMALLINT bufferLength,  SQLSMALLINT *nameLength )
{
	TRACE ("SQLGetCursorName called\n");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlGetCursorName( cursorName, bufferLength, nameLength );
}

///// SQLNumResultCols /////	ODBC 1.0	///// ISO 92

SQLRETURN SQL_API SQLNumResultCols( SQLHSTMT hStmt, SQLSMALLINT *columnCount )
{
	TRACE ("SQLNumResultCols");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlNumResultCols( columnCount );
}

///// SQLPrepare /////	ODBC 1.0	///// ISO 92

SQLRETURN SQL_API SQLPrepare( SQLHSTMT hStmt,
           SQLCHAR *statementText, SQLINTEGER textLength )
{
	TRACE ("SQLPrepare");
	GUARD_HSTMT( hStmt );
	
	return ((OdbcStatement*) hStmt)->sqlPrepare( statementText, textLength );
}

///// SQLRowCount /////	ODBC 1.0	///// ISO 92

SQLRETURN SQL_API SQLRowCount( SQLHSTMT hStmt, SQLLEN* rowCount )
{
	TRACE ("SQLRowCount");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlRowCount( rowCount );
}

///// SQLSetCursorName /////	ODBC 1.0	///// ISO 92

SQLRETURN SQL_API SQLSetCursorName( SQLHSTMT hStmt, SQLCHAR *cursorName,
								 SQLSMALLINT nameLength )
{
	TRACE ("SQLSetCursorName");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlSetCursorName( cursorName, nameLength );
}

///// SQLSetParam ///// Deprecated in 2.0

SQLRETURN SQL_API SQLSetParam( SQLHSTMT hStmt,
						   SQLUSMALLINT parameterNumber, SQLSMALLINT valueType,
						   SQLSMALLINT parameterType, SQLULEN lengthPrecision,
						   SQLSMALLINT parameterScale, SQLPOINTER parameterValue,
						   SQLLEN *strLen_or_Ind )
{
	TRACE ("SQLSetParam");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlSetParam( parameterNumber, valueType,
													parameterType, lengthPrecision,
													parameterScale, parameterValue,
													strLen_or_Ind );
}

///// SQLTransact /////

SQLRETURN SQL_API SQLTransact( SQLHENV hEnv, SQLHDBC hDbc,
							SQLUSMALLINT completionType )
{
	TRACE ("SQLTransact");

	if ( hEnv == SQL_NULL_HENV )
	{
		GUARD_HDBC( hDbc );
		return ((OdbcConnection*) hDbc)->sqlEndTran( completionType );
	}

	GUARD_ENV( hEnv );
	return ((OdbcEnv*) hEnv)->sqlEndTran( completionType );
}

///// SQLColumns /////

SQLRETURN SQL_API SQLColumns( SQLHSTMT hStmt,
						   SQLCHAR *catalogName, SQLSMALLINT nameLength1,
						   SQLCHAR *schemaName, SQLSMALLINT nameLength2,
						   SQLCHAR *tableName, SQLSMALLINT nameLength3,
						   SQLCHAR *columnName, SQLSMALLINT nameLength4 )
{
	TRACE ("SQLColumns");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlColumns( catalogName, nameLength1,
												   schemaName, nameLength2,
												   tableName, nameLength3,
												   columnName, nameLength4 );
}

///// SQLDriverConnect /////

SQLRETURN SQL_API SQLDriverConnect( SQLHDBC hDbc, SQLHWND hWnd, SQLCHAR *szConnStrIn,
									SQLSMALLINT cbConnStrIn, SQLCHAR *szConnStrOut,
									SQLSMALLINT cbConnStrOutMax, SQLSMALLINT *pcbConnStrOut,
									SQLUSMALLINT fDriverCompletion )
{
	TRACE ("SQLDriverConnect");
	GUARD_HDBC( hDbc );

	SQLRETURN ret = ((OdbcConnection*) hDbc)->sqlDriverConnect( hWnd, szConnStrIn, cbConnStrIn,
													szConnStrOut, cbConnStrOutMax, pcbConnStrOut,
													fDriverCompletion );
	LOG_PRINT(( logFile, 
				"SQLDriverConnect   : Line %d\n"
				"   +status         : %d\n"
				"   +hDbc           : %p\n"
				"   +szConnStrIn    : %s\n"
				"   +szConnStrOut   : %s\n\n",
					__LINE__,
					ret,
					hDbc,
					szConnStrIn ? szConnStrIn : (SQLCHAR*)"",
					szConnStrOut ? szConnStrOut : (SQLCHAR*)"" ));

	return ret;
}

///// SQLGetConnectOption /////  Level 1	///// Deprecated

SQLRETURN SQL_API SQLGetConnectOption( SQLHDBC hDbc, SQLUSMALLINT option, SQLPOINTER value )
{
	TRACE ("SQLGetConnectOption");
	GUARD_HDBC( hDbc );

	int bufferLength;

	switch ( option )
	{
	case SQL_ATTR_CURRENT_CATALOG:
	case SQL_ATTR_TRACEFILE:
	case SQL_ATTR_TRANSLATE_LIB:
		bufferLength = SQL_MAX_OPTION_STRING_LENGTH;
		break;
	default:
		bufferLength = 0;
	}

	return ((OdbcConnection*) hDbc)->sqlGetConnectAttr( option, value, bufferLength, NULL);
}

///// SQLGetData /////

SQLRETURN SQL_API SQLGetData( SQLHSTMT hStmt, SQLUSMALLINT columnNumber,
							 SQLSMALLINT targetType, SQLPOINTER targetValue,
							 SQLLEN bufferLength, SQLLEN *strLen_or_Ind )
{
	TRACE ("SQLGetData");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlGetData( columnNumber, targetType, targetValue,
													bufferLength, strLen_or_Ind );
}

///// SQLGetFunctions /////

SQLRETURN SQL_API SQLGetFunctions( SQLHDBC hDbc, SQLUSMALLINT functionId,
								  SQLUSMALLINT *supported )
{
	TRACE ("SQLGetFunctions");
	GUARD_HDBC( hDbc );

	return ((OdbcConnection*) hDbc)->sqlGetFunctions( functionId, supported );
}

///// SQLGetInfo /////

SQLRETURN SQL_API SQLGetInfo( SQLHDBC hDbc, SQLUSMALLINT infoType, SQLPOINTER infoValue,
								SQLSMALLINT bufferLength, SQLSMALLINT *stringLength )
{
	TRACE ("SQLGetInfo");
	GUARD_HDBC( hDbc );

	return ((OdbcConnection*) hDbc)->sqlGetInfo( infoType, infoValue,
													bufferLength, stringLength );
}

///// SQLGetStmtOption /////  Level 1

SQLRETURN SQL_API SQLGetStmtOption( SQLHSTMT hStmt, SQLUSMALLINT option, SQLPOINTER value )
{
	TRACE ("SQLGetStmtOption");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlGetStmtAttr( option, value, 0, NULL );
}

///// SQLGetTypeInfo /////

SQLRETURN SQL_API SQLGetTypeInfo( SQLHSTMT hStmt, SQLSMALLINT dataType )
{
	TRACE ("SQLGetTypeInfo");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlGetTypeInfo( dataType );
}

///// SQLParamData /////

SQLRETURN SQL_API SQLParamData( SQLHSTMT hStmt, SQLPOINTER *value )
{
	TRACE("SQLParamData");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlParamData( value );
}

///// SQLPutData /////

SQLRETURN SQL_API SQLPutData( SQLHSTMT hStmt, SQLPOINTER data, SQLLEN strLen_or_Ind ) 
{
	TRACE ("SQLPutData");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlPutData( data, strLen_or_Ind );
}


///// SQLSetConnectOption /////  Level 1	///// Deprecated

SQLRETURN SQL_API SQLSetConnectOption( SQLHDBC hDbc, SQLUSMALLINT option, SQLULEN value )
{
	TRACE ("SQLSetConnectOption");
	GUARD_HDBC( hDbc );

	return ((OdbcConnection*) hDbc)->sqlSetConnectAttr( option, (SQLPOINTER)value, 0 );
}


///// SQLSetStmtOption ///// Deprecated

SQLRETURN SQL_API SQLSetStmtOption( SQLHSTMT hStmt, SQLUSMALLINT option, SQLULEN value )
{
	TRACE ("SQLSetStmtOption");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlSetStmtAttr( option, (SQLPOINTER)value, 0);
}

///// SQLSpecialColumns /////

SQLRETURN SQL_API SQLSpecialColumns( SQLHSTMT hStmt, SQLUSMALLINT identifierType,
									SQLCHAR *catalogName, SQLSMALLINT nameLength1,
									SQLCHAR *schemaName, SQLSMALLINT nameLength2,
									SQLCHAR *tableName, SQLSMALLINT nameLength3,
									SQLUSMALLINT scope, SQLUSMALLINT nullable )
{
	TRACE ("SQLSpecialColumns");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlSpecialColumns( identifierType,
														catalogName, nameLength1,
														schemaName, nameLength2,
														tableName, nameLength3,
														scope, nullable);
}

///// SQLStatistics /////

SQLRETURN SQL_API SQLStatistics( SQLHSTMT hStmt,
								SQLCHAR *catalogName, SQLSMALLINT nameLength1,
								SQLCHAR *schemaName, SQLSMALLINT nameLength2,
								SQLCHAR *tableName, SQLSMALLINT nameLength3,
								SQLUSMALLINT unique, SQLUSMALLINT reserved )
{
	TRACE ("SQLStatistics");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlStatistics( catalogName, nameLength1,
													schemaName, nameLength2,
													tableName, nameLength3,
													unique, reserved );
}

///// SQLTables /////

SQLRETURN SQL_API SQLTables( SQLHSTMT hStmt,
							SQLCHAR *catalogName, SQLSMALLINT nameLength1,
							SQLCHAR *schemaName, SQLSMALLINT nameLength2,
							SQLCHAR *tableName, SQLSMALLINT nameLength3,
							SQLCHAR *tableType, SQLSMALLINT nameLength4 )
{
	TRACE ("SQLTables");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlTables( catalogName, nameLength1,
												schemaName, nameLength2,
												tableName, nameLength3,
												tableType, nameLength4 );
}

///// SQLBrowseConnect /////

SQLRETURN SQL_API SQLBrowseConnect( SQLHDBC hDbc,
									SQLCHAR *szConnStrIn, SQLSMALLINT cbConnStrIn,
									SQLCHAR *szConnStrOut, SQLSMALLINT cbConnStrOutMax,
									SQLSMALLINT *pcbConnStrOut )
{
	TRACE ("SQLBrowseConnect");
	GUARD_HDBC( hDbc );

	return ((OdbcConnection*) hDbc)->sqlBrowseConnect( szConnStrIn, cbConnStrIn,
														szConnStrOut, cbConnStrOutMax,
														pcbConnStrOut );
}

///// SQLDataSources /////

SQLRETURN SQL_API SQLDataSources( SQLHENV hEnv,
								SQLUSMALLINT direction, SQLCHAR *serverName,
								SQLSMALLINT bufferLength1, SQLSMALLINT *nameLength1,
								SQLCHAR *description, SQLSMALLINT bufferLength2,
								SQLSMALLINT *nameLength2 )
{
	TRACE ("SQLDataSources");
	GUARD_ENV( hEnv );

	return ((OdbcEnv*)hEnv)->sqlDataSources( direction, serverName,
											bufferLength1, nameLength1, description,
											bufferLength2, nameLength2 );
}

///// SQLDescribeParam /////

SQLRETURN SQL_API SQLDescribeParam( SQLHSTMT hStmt, SQLUSMALLINT iPar,
									SQLSMALLINT *pfSqlType, SQLULEN *pcbParamDef,
									SQLSMALLINT *pibScale, SQLSMALLINT *pfNullable )

{
	TRACE("SQLDescribeParam");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlDescribeParam( iPar, pfSqlType, pcbParamDef,
														pibScale, pfNullable );
}

///// SQLExtendedFetch /////

SQLRETURN SQL_API SQLExtendedFetch( SQLHSTMT hStmt, SQLUSMALLINT fFetchType,
									SQLLEN iRow, SQLULEN *pcRow, SQLUSMALLINT *rgfRowStatus )
{
	TRACE ("SQLExtendedFetch");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlExtendedFetch( fFetchType, iRow, pcRow, rgfRowStatus );
}

///// SQLForeignKeys /////

SQLRETURN SQL_API SQLForeignKeys( SQLHSTMT hStmt,
									SQLCHAR *szPkCatalogName, SQLSMALLINT cbPkCatalogName,
									SQLCHAR *szPkSchemaName, SQLSMALLINT cbPkSchemaName,
									SQLCHAR *szPkTableName, SQLSMALLINT cbPkTableName,
									SQLCHAR *szFkCatalogName, SQLSMALLINT cbFkCatalogName,
									SQLCHAR *szFkSchemaName, SQLSMALLINT cbFkSchemaName,
									SQLCHAR *szFkTableName, SQLSMALLINT cbFkTableName )
{
	TRACE ("SQLForeignKeys");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlForeignKeys( szPkCatalogName, cbPkCatalogName,
														szPkSchemaName, cbPkSchemaName,
														szPkTableName, cbPkTableName,
														szFkCatalogName, cbFkCatalogName,
														szFkSchemaName, cbFkSchemaName,
														szFkTableName, cbFkTableName );
}

///// SQLMoreResults /////

SQLRETURN SQL_API SQLMoreResults( SQLHSTMT hStmt )
{
	TRACE("SQLMoreResults");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlMoreResults();
}

///// SQLNativeSql /////

SQLRETURN SQL_API SQLNativeSql( SQLHDBC hDbc,
								SQLCHAR *szSqlStrIn, SQLINTEGER cbSqlStrIn,
								SQLCHAR *szSqlStr, SQLINTEGER cbSqlStrMax,
								SQLINTEGER *pcbSqlStr )
{
	TRACE ("SQLNativeSql");
	GUARD_HDBC( hDbc );

	return ((OdbcConnection*) hDbc)->sqlNativeSql( szSqlStrIn, cbSqlStrIn,
													szSqlStr, cbSqlStrMax, pcbSqlStr );
}

///// SQLNumParams /////

SQLRETURN SQL_API SQLNumParams( SQLHSTMT hStmt, SQLSMALLINT *pcPar )
{
	TRACE("SQLMoreResults");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlNumParams( pcPar );
}

///// SQLParamOptions /////

SQLRETURN SQL_API SQLParamOptions( SQLHSTMT hStmt, SQLULEN cRow, SQLULEN *piRow )
{
	TRACE("SQLParamOptions");
	GUARD_HSTMT( hStmt );

	((OdbcStatement*) hStmt)->sqlSetStmtAttr( SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)cRow, 0 );
	((OdbcStatement*) hStmt)->sqlSetStmtAttr( SQL_ATTR_PARAMS_PROCESSED_PTR, (SQLPOINTER)piRow, 0 );

	return SQL_SUCCESS;
}

///// SQLPrimaryKeys /////

SQLRETURN SQL_API SQLPrimaryKeys( SQLHSTMT hStmt, 
								SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
								SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName,
								SQLCHAR *szTableName, SQLSMALLINT cbTableName )
{
	TRACE ("SQLPrimaryKeys");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlPrimaryKeys( szCatalogName, cbCatalogName,
													szSchemaName, cbSchemaName,
													szTableName, cbTableName );
}

///// SQLProcedureColumns /////

SQLRETURN SQL_API SQLProcedureColumns( SQLHSTMT hStmt,
									SQLCHAR *szCatalogName,
									SQLSMALLINT cbCatalogName,
									SQLCHAR *szSchemaName,
									SQLSMALLINT cbSchemaName,
									SQLCHAR *szProcName,
									SQLSMALLINT cbProcName,
									SQLCHAR *szColumnName,
									SQLSMALLINT cbColumnName )
{
	TRACE ("SQLProcedureColumns");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlProcedureColumns( szCatalogName, cbCatalogName,
															szSchemaName, cbSchemaName,
															szProcName, cbProcName,
															szColumnName, cbColumnName );
}

///// SQLProcedures /////

SQLRETURN SQL_API SQLProcedures( SQLHSTMT hStmt,
								SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
								SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName,
								SQLCHAR *szProcName, SQLSMALLINT cbProcName )
{
	TRACE ("SQLProcedures");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlProcedures( szCatalogName, cbCatalogName,
													szSchemaName, cbSchemaName,
													szProcName, cbProcName );
}

///// SQLSetPos /////

SQLRETURN SQL_API SQLSetPos( SQLHSTMT hStmt, SQLSETPOSIROW iRow,
								SQLUSMALLINT fOption, SQLUSMALLINT fLock )
{
	TRACE ("SQLSetPos");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlSetPos( iRow, fOption, fLock );
}

///// SQLSetScrollOptions /////

SQLRETURN SQL_API SQLSetScrollOptions( SQLHSTMT hStmt, SQLUSMALLINT fConcurrency,
										SQLLEN crowKeyset, SQLUSMALLINT crowRowset)
{
	TRACE ("SQLSetScrollOptions");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlSetScrollOptions( fConcurrency, crowKeyset, crowRowset );
}

///// SQLTablePrivileges /////

SQLRETURN SQL_API SQLTablePrivileges( SQLHSTMT hStmt,
										SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
										SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName,
										SQLCHAR *szTableName, SQLSMALLINT cbTableName )

{
	TRACE ("SQLTablePrivileges");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlTablePrivileges( szCatalogName, cbCatalogName,
														szSchemaName, cbSchemaName,
														szTableName, cbTableName );
}

///// SQLColumnPrivileges /////

SQLRETURN SQL_API SQLColumnPrivileges( SQLHSTMT hStmt,
										SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
										SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName,
										SQLCHAR *szTableName, SQLSMALLINT cbTableName,
										SQLCHAR *szColumnName, SQLSMALLINT cbColumnName )
{
	TRACE ("SQLColumnPrivileges");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlColumnPrivileges( szCatalogName, cbCatalogName,
															szSchemaName, cbSchemaName,
															szTableName, cbTableName,
															szColumnName, cbColumnName );
}

///// SQLDrivers /////

SQLRETURN SQL_API SQLDrivers( SQLHENV hEnv, SQLUSMALLINT fDirection,
							SQLCHAR *szDriverDesc, SQLSMALLINT cbDriverDescMax,
							SQLSMALLINT *pcbDriverDesc,
							SQLCHAR *szDriverAttributes, SQLSMALLINT cbDrvrAttrMax,
							SQLSMALLINT *pcbDrvrAttr )
{
	TRACE ("SQLDrivers");
	GUARD_ENV( hEnv );

	return ((OdbcEnv*) hEnv)->sqlDrivers( fDirection, szDriverDesc, cbDriverDescMax,
										pcbDriverDesc, szDriverAttributes, cbDrvrAttrMax,
										pcbDrvrAttr );
}

///// SQLBindParameter /////

SQLRETURN SQL_API SQLBindParameter( SQLHSTMT hStmt, SQLUSMALLINT iPar, SQLSMALLINT fParamType,
									SQLSMALLINT fCType, SQLSMALLINT fSqlType, SQLULEN cbColDef,
									SQLSMALLINT ibScale, SQLPOINTER rgbValue, SQLLEN cbValueMax,
									SQLLEN *pcbValue )
{
	TRACE ("SQLBindParameter");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlBindParameter( iPar, fParamType, fCType, fSqlType,
													cbColDef, ibScale, rgbValue, cbValueMax,
													pcbValue );
}

///// SQLAllocHandle - global /////

SQLRETURN SQL_API SQLAllocHandle( SQLSMALLINT fHandleType, SQLHANDLE hInput, SQLHANDLE *phOutput )
{
	TRACE ("SQLAllocHandle");

	switch( fHandleType )
	{
	case SQL_HANDLE_ENV:
		{
			GUARD;
			return __SQLAllocHandle( fHandleType, hInput, phOutput );
		}

	case SQL_HANDLE_DBC:
		{
			GUARD_ENV( hInput );
			return __SQLAllocHandle( fHandleType, hInput, phOutput );
		}

	case SQL_HANDLE_STMT:
		{
			GUARD_HDBC( hInput );
			return __SQLAllocHandle( fHandleType, hInput, phOutput );
		}

	case SQL_HANDLE_DESC:
		{
			GUARD_HDBC( hInput );
			return __SQLAllocHandle( fHandleType, hInput, phOutput );
		}
	}

	return SQL_INVALID_HANDLE;
}

///// SQLBindParam /////

SQLRETURN SQL_API SQLBindParam( SQLHSTMT hStmt, SQLUSMALLINT parameterNumber,
							 SQLSMALLINT valueType, SQLSMALLINT parameterType,
							 SQLULEN lengthPrecision, SQLSMALLINT parameterScale,
							 SQLPOINTER parameterValue, SQLLEN *strLen_or_Ind )
{
	TRACE ("SQLBindParam");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*)hStmt)->sqlBindParameter( parameterNumber, SQL_PARAM_INPUT,
									valueType, parameterType, lengthPrecision, parameterScale,
									parameterValue, SQL_SETPARAM_VALUE_MAX, strLen_or_Ind );
}

///// SQLCloseCursor /////

SQLRETURN SQL_API SQLCloseCursor  (SQLHSTMT arg0)
{
	TRACE ("SQLCloseCursor");
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlCloseCursor();
}

///// SQLColAttribute ///// ODBC 3.0 ///// ISO 92

SQLRETURN SQL_API SQLColAttribute( SQLHSTMT hStmt, SQLUSMALLINT columnNumber,
									SQLUSMALLINT fieldIdentifier, SQLPOINTER characterAttribute,
									SQLSMALLINT bufferLength, SQLSMALLINT *stringLength,
#ifdef _WIN64
									SQLLEN *numericAttribute )
#else
									SQLPOINTER numericAttribute )
#endif
{
	TRACE ("SQLColAttribute");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*)hStmt)->sqlColAttribute( columnNumber, fieldIdentifier,
													characterAttribute, bufferLength,
													stringLength, numericAttribute );
}

///// SQLCopyDesc ///// ODBC 3.0 /////

SQLRETURN SQL_API SQLCopyDesc( SQLHDESC sourceDescHandle, SQLHDESC targetDescHandle )
{
	TRACE ("SQLCopyDesc");
	GUARD_HDESC( sourceDescHandle );

	if( sourceDescHandle == NULL || targetDescHandle == NULL )
		return SQL_ERROR;

	return *(OdbcDesc*)targetDescHandle = *(OdbcDesc*)sourceDescHandle;
}

///// SQLEndTran ///// ODBC 3.0 ///// ISO 92

SQLRETURN SQL_API SQLEndTran( SQLSMALLINT handleType, SQLHANDLE handle,
								SQLSMALLINT completionType )
{
	TRACE ("SQLEndTran");

	switch ( handleType )
	{
	case SQL_HANDLE_DBC:
		{
			GUARD_HDBC( handle );
			return ((OdbcConnection*) handle)->sqlEndTran( completionType );
		}

	case SQL_HANDLE_ENV:
		{
			GUARD_ENV( handle );
			return ((OdbcEnv*) handle)->sqlEndTran( completionType );
		}
	}

	return SQL_INVALID_HANDLE;
}

///// SQLFetchScroll /////

SQLRETURN SQL_API SQLFetchScroll( SQLHSTMT hStmt,
								SQLSMALLINT fetchOrientation, SQLLEN fetchOffset )
{
	TRACE ("SQLFetchScroll");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlFetchScroll( fetchOrientation, fetchOffset );
}

///// SQLFreeHandle /////

SQLRETURN SQL_API SQLFreeHandle( SQLSMALLINT handleType, SQLHANDLE handle )
{
	TRACE ("SQLFreeHandle\n");

	switch ( handleType )
	{
	case SQL_HANDLE_ENV:
		{
			GUARD;
			delete (OdbcEnv*) handle;
		}
		break;

	case SQL_HANDLE_DBC:
		{
			GUARD_ENV( ((OdbcConnection*) handle)->env );
			delete (OdbcConnection*) handle;
		}
		break;

	case SQL_HANDLE_STMT:
		{
			GUARD_HSTMT( handle );
			delete (OdbcStatement*) handle;
		}
		break;

	case SQL_HANDLE_DESC:
		{
			GUARD_HDESC( handle );
			if ( ((OdbcDesc*) handle)->headType == odtApplication )
				delete (OdbcDesc*) handle;
		}
		break;

	default:
		return SQL_INVALID_HANDLE;
	}

	return SQL_SUCCESS;
}

///// SQLGetConnectAttr /////

SQLRETURN SQL_API SQLGetConnectAttr( SQLHDBC hDbc,
								   SQLINTEGER attribute, SQLPOINTER value,
								   SQLINTEGER bufferLength, SQLINTEGER *stringLength )
{
	TRACE ("SQLGetConnectAttr");
	GUARD_HDBC( hDbc );

	return ((OdbcConnection*) hDbc)->sqlGetConnectAttr( attribute, value,
														bufferLength, stringLength );
}

///// SQLGetDescField /////

SQLRETURN SQL_API SQLGetDescField( SQLHDESC hDesc,
					   SQLSMALLINT recNumber, SQLSMALLINT fieldIdentifier,
					   SQLPOINTER value, SQLINTEGER bufferLength, SQLINTEGER *stringLength )
{
	TRACE ("SQLGetDescField");
	GUARD_HDESC( hDesc );

	return ((OdbcDesc*) hDesc)->sqlGetDescField( recNumber, fieldIdentifier,
												value, bufferLength, stringLength );
}

///// SQLGetDescRec /////

SQLRETURN SQL_API SQLGetDescRec( SQLHDESC hDesc,
								SQLSMALLINT recNumber, SQLCHAR *name,
								SQLSMALLINT bufferLength, SQLSMALLINT *stringLength,
								SQLSMALLINT *type, SQLSMALLINT *subType, 
								SQLLEN     *length, SQLSMALLINT *precision, 
								SQLSMALLINT *scale, SQLSMALLINT *nullable )
{
	TRACE ("SQLGetDescRec");
	GUARD_HDESC( hDesc );

	return ((OdbcDesc*) hDesc)->sqlGetDescRec( recNumber, name,
											bufferLength, stringLength, type, subType, 
											length, precision, scale, nullable );
}

///// SQLGetDiagField /////

SQLRETURN SQL_API SQLGetDiagField( SQLSMALLINT handleType, SQLHANDLE handle,
								SQLSMALLINT recNumber, SQLSMALLINT diagIdentifier,
								SQLPOINTER diagInfo, SQLSMALLINT bufferLength,
								SQLSMALLINT *stringLength )
{
	TRACE ("SQLGetDiagField");
	GUARD_HTYPE( handle, handleType );

	return ((OdbcObject*) handle)->sqlGetDiagField( recNumber, diagIdentifier,
												diagInfo, bufferLength, stringLength );
}

///// SQLGetDiagRec /////

SQLRETURN SQL_API SQLGetDiagRec( SQLSMALLINT handleType, SQLHANDLE handle,
								SQLSMALLINT recNumber, SQLCHAR *sqlState,
								SQLINTEGER *nativeError, SQLCHAR *messageText,
								SQLSMALLINT bufferLength, SQLSMALLINT *textLength )
{
	TRACE ("SQLGetDiagRec");
	GUARD_HTYPE( handle, handleType );

	return ((OdbcObject*) handle)->sqlGetDiagRec( handleType, recNumber, sqlState,
													nativeError, messageText,
													bufferLength, textLength );
}

///// SQLGetEnvAttr /////

SQLRETURN SQL_API SQLGetEnvAttr( SQLHENV hEnv,
							   SQLINTEGER attribute, SQLPOINTER value,
							   SQLINTEGER bufferLength, SQLINTEGER *stringLength )
{
	TRACE ("SQLGetEnvAttr");

	return ((OdbcEnv*) hEnv)->sqlGetEnvAttr( attribute, value,
											bufferLength, stringLength );
}

///// SQLGetStmtAttr /////

SQLRETURN SQL_API SQLGetStmtAttr( SQLHSTMT hStmt,
								SQLINTEGER attribute, SQLPOINTER value,
								SQLINTEGER bufferLength, SQLINTEGER *stringLength )
{
	TRACE ("SQLGetStmtAttr");
	GUARD_HSTMT( hStmt );

	if ( bufferLength <= SQL_LEN_BINARY_ATTR_OFFSET )
		bufferLength = -bufferLength + SQL_LEN_BINARY_ATTR_OFFSET;

	return ((OdbcStatement*) hStmt)->sqlGetStmtAttr( attribute, value,
													bufferLength, stringLength );
}

///// SQLSetConnectAttr /////

SQLRETURN SQL_API SQLSetConnectAttr( SQLHDBC hDbc, SQLINTEGER attribute,
									SQLPOINTER value, SQLINTEGER stringLength )
{
	TRACE ("SQLSetConnectAttr");
	GUARD_HDBC( hDbc );

	if ( stringLength <= SQL_LEN_BINARY_ATTR_OFFSET )
		stringLength = -stringLength + SQL_LEN_BINARY_ATTR_OFFSET;

	return ((OdbcConnection*) hDbc)->sqlSetConnectAttr( attribute, value, stringLength );
}

///// SQLSetDescField /////

SQLRETURN SQL_API SQLSetDescField( SQLHDESC hDesc,
								   SQLSMALLINT recNumber, SQLSMALLINT fieldIdentifier,
								   SQLPOINTER value, SQLINTEGER bufferLength )
{
	TRACE ("SQLSetDescField");
	GUARD_HDESC( hDesc );

	return ((OdbcDesc*) hDesc)->sqlSetDescField( recNumber, fieldIdentifier,
												value, bufferLength );
}

///// SQLSetDescRec /////

SQLRETURN SQL_API SQLSetDescRec( SQLHDESC hDesc,
							   SQLSMALLINT recNumber, SQLSMALLINT type,
							   SQLSMALLINT subType, SQLLEN length,
							   SQLSMALLINT precision, SQLSMALLINT scale,
							   SQLPOINTER data, SQLLEN *stringLength,
							   SQLLEN *indicator )
{
	TRACE ("SQLSetDescRec");
	GUARD_HDESC( hDesc );

	return ((OdbcDesc*) hDesc)->sqlSetDescRec( recNumber, type, subType,
												length, precision, scale,
												data, stringLength, indicator );
}

///// SQLSetEnvAttr /////

SQLRETURN SQL_API SQLSetEnvAttr( SQLHENV hEnv,
							   SQLINTEGER attribute, SQLPOINTER value,
							   SQLINTEGER stringLength )
{
	TRACE ("SQLSetEnvAttr");

	return ((OdbcEnv*) hEnv)->sqlSetEnvAttr( attribute, value, stringLength );
}

///// SQLSetStmtAttr /////

SQLRETURN SQL_API SQLSetStmtAttr( SQLHSTMT hStmt, SQLINTEGER attribute,
								 SQLPOINTER value, SQLINTEGER stringLength )
{
	TRACE ("SQLSetStmtAttr");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlSetStmtAttr( attribute, value, stringLength );
}


///// SQLBulkOperations /////

SQLRETURN SQL_API SQLBulkOperations( SQLHSTMT hStmt, SQLSMALLINT operation )
{
	TRACE ("SQLBulkOperations");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlBulkOperations( operation );
}
