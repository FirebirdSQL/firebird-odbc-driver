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

#ifdef _WIN32
#include <windows.h>
#endif

#include <odbcinst.h>

extern "C"
{
#include <sql.h>
#include <sqlext.h>
#include <sqlucode.h>
}

#include <stdio.h>
#include "OdbcJdbc.h"
#include "OdbcEnv.h"
#include "OdbcConnection.h"
#include "OdbcStatement.h"
#include "SafeEnvThread.h"

void trace (const char *msg);

#ifdef _WIN32
#define OUTPUT_MONITOR_EXECUTING(msg)  OutputDebugString(msg"\n");
#else
#define OUTPUT_MONITOR_EXECUTING(msg)
#endif

#ifdef DEBUG
#define TRACE(msg)		trace (msg"\n")
#else
#ifdef __MONITOR_EXECUTING
#define TRACE(msg)		OUTPUT_MONITOR_EXECUTING(msg)
#else
#define TRACE(msg)		
#endif
#endif

#if(DRIVER_LOCKED_LEVEL == DRIVER_LOCKED_LEVEL_ENV)

#define GUARD					SafeDllThread wt
#define GUARD_ENV(arg)			GUARD
#define GUARD_HSTMT(arg)		GUARD
#define GUARD_HDBC(arg)			GUARD
#define GUARD_HDESC(arg)		GUARD
#define GUARD_HTYPE(arg1,arg2)	GUARD

#elif(DRIVER_LOCKED_LEVEL == DRIVER_LOCKED_LEVEL_CONNECT)

#define GUARD					SafeDllThread wt
#define GUARD_ENV(arg)			SafeEnvThread wt((OdbcEnv*)arg)
#define GUARD_HSTMT(arg)		SafeConnectThread wt(((OdbcStatement*)arg)->connection)
#define GUARD_HDBC(arg) 		SafeConnectThread wt((OdbcConnection*)arg)
#define GUARD_HDESC(arg)		SafeConnectThread wt(((OdbcDesc*)arg)->connection)
#define GUARD_HTYPE(arg,arg1)	SafeConnectThread wt(												\
									arg1==SQL_HANDLE_DBC ? (OdbcConnection*)arg:					\
									arg1==SQL_HANDLE_STMT ? ((OdbcStatement*)arg)->connection:		\
									arg1==SQL_HANDLE_DESC ? ((OdbcDesc*)arg)->connection : NULL )

#else

#define GUARD
#define GUARD_ENV(arg)
#define GUARD_HSTMT(arg)
#define GUARD_HDBC(arg)
#define GUARD_HDESC(arg)	
#define GUARD_HTYPE(arg1,arg2)

#endif

using namespace OdbcJdbcLibrary;
extern UINT codePage; // from Main.cpp

template <typename TypeRealLen = SQLSMALLINT>
class ConvertingString
{
	enum typestring { NONE, WIDECHARS, BYTESCHARS };

	SQLCHAR		*byteString;
	SQLWCHAR	*unicodeString;
	TypeRealLen	*realLength;
	int			lengthString;
	typestring	isWhy;
	bool		returnCountOfBytes;
	
public:
	ConvertingString() 
	{
		isWhy = NONE;
		returnCountOfBytes = true;
		unicodeString = NULL;
		byteString = NULL;
		realLength = NULL;
		lengthString = 0;
	}

	ConvertingString( int length, SQLWCHAR *wcString, TypeRealLen *pLength = NULL, bool retCountOfBytes = true )
	{
		realLength = pLength;
		returnCountOfBytes = retCountOfBytes;

		if ( wcString )
		{
			isWhy = BYTESCHARS;
			unicodeString = wcString;
			if ( length == SQL_NTS )
				lengthString = 0;
			else
				lengthString = length / 2;
		}
		else
			isWhy = NONE;

		Alloc();
	}

	ConvertingString( int length, SQLCHAR *mbString )
	{
		returnCountOfBytes = true;

		if ( mbString )
		{
			isWhy = WIDECHARS;
			byteString = mbString;
			lengthString = length;
		}
		else
			isWhy = NONE;

		Alloc();
	}

	ConvertingString( SQLWCHAR *wcString, int length )
	{
		realLength = NULL;
		unicodeString = NULL;
		returnCountOfBytes = true;
		isWhy = BYTESCHARS;

		if ( wcString )
			convUnicodeToString( wcString, length );
		else
		{
			byteString = NULL;
			lengthString = 0;
		}
	}
	
	ConvertingString( SQLCHAR *mbString, int length )
	{
		realLength = NULL;
		byteString = NULL;
		returnCountOfBytes = true;
		isWhy = WIDECHARS;

		if ( mbString )
			convStringToUnicode( mbString, length );
		else
		{
			unicodeString = NULL;
			lengthString = 0;
		}
	}

	operator SQLCHAR*()	{ return byteString; }
	operator SQLWCHAR*() { return unicodeString; }
	operator int() { return lengthString; }
	operator SQLSMALLINT() { return lengthString; }
	operator SQLINTEGER() { return lengthString; }

	~ConvertingString() 
	{
		switch ( isWhy )
		{
		case WIDECHARS:
			if ( byteString )
			{
				int len = WideCharToMultiByte( codePage, 0, unicodeString, -1,
										(LPSTR)byteString, lengthString, NULL, NULL );
				if ( len > 0 )
				{
					len--;
					byteString[ len ] = '\0';

					if ( realLength )
						*realLength = len;
				}
			}

			delete[] unicodeString;
			break;

		case BYTESCHARS:
			if ( unicodeString )
			{
				int len = MultiByteToWideChar( codePage, 0, (const char*)byteString, -1,
											  unicodeString, lengthString );
				if ( len > 0 )
				{
					len--;
					*(LPWSTR)(unicodeString + len) = L'\0';

					if ( realLength )
					{
						if ( returnCountOfBytes )
							*realLength = len * 2;
						else
							*realLength = len;
					}
				}
			}

			delete[] byteString;
			break;

		case NONE:
			if ( realLength && returnCountOfBytes )
				*realLength *= 2;
			break;
		}
	}

	SQLCHAR * convUnicodeToString( SQLWCHAR *wcString, int length )
	{
		if ( length == SQL_NTS )
			length = wcslen( wcString );

		int bytesNeeded = WideCharToMultiByte( codePage, 0, wcString, length, NULL, 0, NULL, NULL );
		byteString = new SQLCHAR[ bytesNeeded + 2 ];

		WideCharToMultiByte( codePage, 0, wcString, length, (LPSTR)byteString, bytesNeeded, NULL, NULL );

		byteString[ bytesNeeded ] = '\0';
		lengthString = bytesNeeded;

		return byteString;
	}

	SQLWCHAR * convStringToUnicode( SQLCHAR *mbString, int length )
	{
		if ( length == SQL_NTS )
			length = strlen( (char*)mbString );
		
		int nWCharNeeded = MultiByteToWideChar( codePage, MB_PRECOMPOSED, (const char*)mbString, length, NULL, 0 );
		unicodeString = new SQLWCHAR[ ( nWCharNeeded + 1 ) * 2 ];

		nWCharNeeded = MultiByteToWideChar( codePage, MB_PRECOMPOSED, (const char*)mbString, length,
						 unicodeString, nWCharNeeded );

		*(LPWSTR)(unicodeString + nWCharNeeded) = L'\0';
		lengthString = nWCharNeeded * 2;

		return unicodeString;
	} 

protected:
	void Alloc()
	{
		switch ( isWhy )
		{
		case WIDECHARS:
			if ( lengthString )
				unicodeString = new SQLWCHAR[ ( lengthString + 1 ) * 2 ];
			else
				unicodeString = NULL;
			break;

		case BYTESCHARS:
			if ( lengthString )
				byteString = new SQLCHAR[ lengthString + 2 ];
			else
				byteString = NULL;
			break;

		case NONE:
			unicodeString = NULL;
			byteString = NULL;
			lengthString = 0;
			break;
		}
	}
};

///// SQLColAttributesW /////	ODBC 1.0	///// Deprecated

SQLRETURN SQL_API SQLColAttributesW( SQLHSTMT hStmt, SQLUSMALLINT columnNumber,
								    SQLUSMALLINT fieldIdentifier, SQLPOINTER characterAttribute,
								    SQLSMALLINT bufferLength, SQLSMALLINT *stringLength,
								    SQLLEN *numericAttribute )
{
	TRACE("SQLColAttributesW");
	GUARD_HSTMT( hStmt );

	switch ( fieldIdentifier )
	{
	case SQL_DESC_LABEL:
	case SQL_DESC_BASE_COLUMN_NAME:
	case SQL_COLUMN_NAME:
	case SQL_DESC_NAME:
	case SQL_DESC_TYPE_NAME:
	case SQL_DESC_BASE_TABLE_NAME:
	case SQL_DESC_TABLE_NAME:
	case SQL_DESC_SCHEMA_NAME:
	case SQL_DESC_CATALOG_NAME:

		if ( bufferLength > 0 )
		{
			ConvertingString<> CharacterAttribute( bufferLength,
														(SQLWCHAR *)characterAttribute, stringLength );

			return ((OdbcStatement*) hStmt)->sqlColAttribute( columnNumber, fieldIdentifier,
											(SQLPOINTER)(SQLCHAR*)CharacterAttribute, CharacterAttribute,
											stringLength, numericAttribute );
		}
	}

	return ((OdbcStatement*) hStmt)->sqlColAttribute( columnNumber, fieldIdentifier,
													characterAttribute, bufferLength,
													stringLength, numericAttribute );
}

///// SQLConnectW /////	ODBC 1.0	///// ISO 92

SQLRETURN SQL_API SQLConnectW( SQLHDBC hDbc,
						      SQLWCHAR *serverName, SQLSMALLINT nameLength1,
						      SQLWCHAR *userName, SQLSMALLINT nameLength2,
						      SQLWCHAR *authentication, SQLSMALLINT nameLength3 )
{
	TRACE ("SQLConnectW");
	GUARD_HDBC( hDbc );

	ConvertingString<> ServerName( serverName, nameLength1 );
	ConvertingString<> UserName( userName, nameLength2 );
	ConvertingString<> Authentication( authentication, nameLength3 );

	return ((OdbcConnection*) hDbc)->sqlConnect( ServerName, ServerName, UserName,
												UserName, Authentication, Authentication );
}

///// SQLDescribeColW /////	ODBC 1.0	///// ISO 92

SQLRETURN SQL_API SQLDescribeColW( SQLHSTMT hStmt, SQLUSMALLINT columnNumber,
							      SQLWCHAR *columnName, SQLSMALLINT bufferLength,
								  SQLSMALLINT *nameLength, SQLSMALLINT *dataType, 
								  SQLULEN *columnSize, SQLSMALLINT *decimalDigits,
								  SQLSMALLINT *nullable )
{
	TRACE ("SQLDescribeColW");
	GUARD_HSTMT( hStmt );

	ConvertingString<> ColumnName( bufferLength, columnName, nameLength, false );

	return ((OdbcStatement*) hStmt)->sqlDescribeCol( columnNumber,
													ColumnName, ColumnName,
													nameLength, dataType, columnSize,
													decimalDigits, nullable );
}

///// SQLErrorW /////	ODBC 1.0	///// Deprecated

SQLRETURN SQL_API SQLErrorW( SQLHENV hEnv,
							SQLHDBC hDbc, SQLHSTMT hStmt,
							SQLWCHAR *sqlState, SQLINTEGER *nativeErrorCode,
							SQLWCHAR *msgBuffer, SQLSMALLINT msgBufferLength,
							SQLSMALLINT *msgLength )
{
	TRACE("SQLErrorW");

	ConvertingString<> State( 12, sqlState );
	ConvertingString<> Buffer( msgBufferLength, msgBuffer, msgLength );

	if ( hStmt )
	{
		GUARD_HSTMT( hStmt );
		return ((OdbcStatement*)hStmt)->sqlError( State, nativeErrorCode, Buffer,
												 Buffer, msgLength );
	}
	if ( hDbc )
	{
		GUARD_HDBC( hDbc );
		return ((OdbcConnection*)hDbc)->sqlError( State, nativeErrorCode, Buffer,
												 Buffer, msgLength );
	}
	if ( hEnv )
		return ((OdbcEnv*)hEnv)->sqlError( State, nativeErrorCode, Buffer,
										  Buffer, msgLength );

	return SQL_ERROR;
}

///// SQLExecDirectW /////	ODBC 1.0	///// ISO 92

SQLRETURN SQL_API SQLExecDirectW( SQLHSTMT hStmt, SQLWCHAR *statementText, SQLINTEGER textLength )
{
	TRACE ("SQLExecDirectW");
	GUARD_HSTMT( hStmt );

	ConvertingString<> StatementText( statementText, textLength );

	return ((OdbcStatement*) hStmt)->sqlExecuteDirect( StatementText, StatementText );
}

///// SQLGetCursorNameW /////	ODBC 1.0	///// ISO 92

SQLRETURN SQL_API SQLGetCursorNameW( SQLHSTMT hStmt, SQLWCHAR *cursorName,
									SQLSMALLINT bufferLength,  SQLSMALLINT *nameLength )
{
	TRACE ("SQLGetCursorNameW");
	GUARD_HSTMT( hStmt );

	ConvertingString<> CursorName( bufferLength, cursorName, nameLength );

	return ((OdbcStatement*) hStmt)->sqlGetCursorName( CursorName, CursorName, nameLength );
}

///// SQLPrepareW /////	ODBC 1.0	///// ISO 92

SQLRETURN SQL_API SQLPrepareW( SQLHSTMT hStmt,
							 SQLWCHAR *statementText, SQLINTEGER textLength )
{
	TRACE ("SQLPrepareW");
	GUARD_HSTMT( hStmt );

	ConvertingString<> StatementText( statementText, textLength );
	
	return ((OdbcStatement*) hStmt)->sqlPrepare( StatementText, StatementText );
}

///// SQLSetCursorNameW /////	ODBC 1.0	///// ISO 92

SQLRETURN SQL_API SQLSetCursorNameW( SQLHSTMT hStmt, SQLWCHAR *cursorName,
									SQLSMALLINT nameLength )
{
	TRACE ("SQLSetCursorNameW");
	GUARD_HSTMT( hStmt );

	ConvertingString<> CursorName( cursorName, nameLength );

	return ((OdbcStatement*) hStmt)->sqlSetCursorName( CursorName, CursorName );
}

///// SQLColumnsW /////

SQLRETURN SQL_API SQLColumnsW( SQLHSTMT hStmt,
							  SQLWCHAR *catalogName, SQLSMALLINT nameLength1,
							  SQLWCHAR *schemaName, SQLSMALLINT nameLength2,
							  SQLWCHAR *tableName, SQLSMALLINT nameLength3,
							  SQLWCHAR *columnName, SQLSMALLINT nameLength4 )
{
	TRACE ("SQLColumnsW");
	GUARD_HSTMT( hStmt );

	ConvertingString<> CatalogName( catalogName, nameLength1 );
	ConvertingString<> SchemaName( schemaName, nameLength2 );
	ConvertingString<> TableName( tableName, nameLength3 );
	ConvertingString<> ColumnName( columnName, nameLength4 );

	return ((OdbcStatement*) hStmt)->sqlColumns( CatalogName, CatalogName,
												SchemaName, SchemaName,
												TableName, TableName,
												ColumnName, ColumnName );
}

///// SQLDriverConnectW /////

SQLRETURN SQL_API SQLDriverConnectW( SQLHDBC hDbc, SQLHWND hWnd, SQLWCHAR *szConnStrIn,
								 	SQLSMALLINT cbConnStrIn, SQLWCHAR *szConnStrOut,
									SQLSMALLINT cbConnStrOutMax, SQLSMALLINT *pcbConnStrOut,
									SQLUSMALLINT fDriverCompletion )
{
	TRACE ("SQLDriverConnectW");
	GUARD_HDBC( hDbc );

	ConvertingString<> ConnStrIn( szConnStrIn, cbConnStrIn );
	ConvertingString<> ConnStrOut( cbConnStrOutMax * 2, szConnStrOut, pcbConnStrOut, false );

	return ((OdbcConnection*) hDbc)->sqlDriverConnect( hWnd, ConnStrIn, ConnStrIn,
													ConnStrOut, ConnStrOut, pcbConnStrOut,
													fDriverCompletion );
}

///// SQLGetConnectOptionW /////  Level 1	///// Deprecated

SQLRETURN SQL_API SQLGetConnectOptionW( SQLHDBC hDbc, SQLUSMALLINT option, SQLPOINTER value )
{
	TRACE ("SQLGetConnectOptionW");
	GUARD_HDBC( hDbc );

	int bufferLength;

	switch ( option )
	{
	case SQL_ATTR_CURRENT_CATALOG:
	case SQL_ATTR_TRACEFILE:
	case SQL_ATTR_TRANSLATE_LIB:
		{
			bufferLength = SQL_MAX_OPTION_STRING_LENGTH;
			ConvertingString<> ConnStrOut( bufferLength, (SQLWCHAR *)value );
			return ((OdbcConnection*) hDbc)->sqlGetConnectAttr( option,
										(SQLPOINTER)(SQLCHAR*)value, bufferLength, NULL);
		}
	default:
		bufferLength = 0;
	}

	return ((OdbcConnection*) hDbc)->sqlGetConnectAttr( option, value, bufferLength, NULL);
}

///// SQLGetInfoW /////

SQLRETURN SQL_API SQLGetInfoW( SQLHDBC hDbc, SQLUSMALLINT infoType, SQLPOINTER infoValue,
							  SQLSMALLINT bufferLength, SQLSMALLINT *stringLength )
{
	TRACE ("SQLGetInfoW");
	GUARD_HDBC( hDbc );

	switch ( infoType )
	{
	case SQL_ACCESSIBLE_PROCEDURES:
	case SQL_ACCESSIBLE_TABLES:
	case SQL_CATALOG_NAME:
	case SQL_CATALOG_NAME_SEPARATOR:
	case SQL_CATALOG_TERM:
	case SQL_COLLATION_SEQ:
	case SQL_COLUMN_ALIAS:
	case SQL_DATA_SOURCE_NAME:
	case SQL_DATA_SOURCE_READ_ONLY:
	case SQL_DATABASE_NAME:
	case SQL_DBMS_NAME:
	case SQL_DBMS_VER:
	case SQL_DESCRIBE_PARAMETER:
	case SQL_DM_VER:
	case SQL_DRIVER_NAME:
	case SQL_DRIVER_ODBC_VER:
	case SQL_DRIVER_VER:
	case SQL_EXPRESSIONS_IN_ORDERBY:
	case SQL_IDENTIFIER_QUOTE_CHAR:
	case SQL_INTEGRITY:
	case SQL_KEYWORDS:
	case SQL_LIKE_ESCAPE_CLAUSE:
	case SQL_MAX_ROW_SIZE_INCLUDES_LONG:
	case SQL_MULT_RESULT_SETS:
	case SQL_MULTIPLE_ACTIVE_TXN:
	case SQL_NEED_LONG_DATA_LEN:
	case SQL_ODBC_VER:
	case SQL_ORDER_BY_COLUMNS_IN_SELECT:
	case SQL_OUTER_JOINS:
	case SQL_PROCEDURE_TERM:
	case SQL_PROCEDURES:
	case SQL_ROW_UPDATES:
	case SQL_SCHEMA_TERM:
	case SQL_SEARCH_PATTERN_ESCAPE:
	case SQL_SERVER_NAME:
	case SQL_SPECIAL_CHARACTERS:
	case SQL_TABLE_TERM:
	case SQL_USER_NAME:
	case SQL_XOPEN_CLI_YEAR:

		if ( bufferLength > 0 )
		{
			ConvertingString<> InfoValue( bufferLength, (SQLWCHAR *)infoValue, stringLength );

			return ((OdbcConnection*) hDbc)->sqlGetInfo( infoType, (SQLPOINTER)(SQLCHAR*)InfoValue,
														InfoValue, stringLength );
		}
		else
		{
			SQLRETURN ret = ((OdbcConnection*) hDbc)->sqlGetInfo( infoType, infoValue,
															bufferLength, stringLength );
			*stringLength *= 2;
			return ret;
		}
	}

	return ((OdbcConnection*) hDbc)->sqlGetInfo( infoType, infoValue,
													bufferLength, stringLength );
}

///// SQLGetTypeInfoW /////

SQLRETURN SQL_API SQLGetTypeInfoW( SQLHSTMT hStmt, SQLSMALLINT dataType )
{
	TRACE ("SQLGetTypeInfoW");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlGetTypeInfo( dataType );
}

///// SQLSetConnectOptionW /////  Level 1	///// Deprecated

SQLRETURN SQL_API SQLSetConnectOptionW( SQLHDBC hDbc, SQLUSMALLINT option, SQLULEN value )
{
	TRACE ("SQLSetConnectOptionW");
	GUARD_HDBC( hDbc );

	switch ( option )
	{
	case SQL_ATTR_CURRENT_CATALOG:
	case SQL_ATTR_TRACEFILE:
	case SQL_ATTR_TRANSLATE_LIB:
		{
			SQLINTEGER bufferLength = SQL_MAX_OPTION_STRING_LENGTH;
			ConvertingString<> Value( (SQLWCHAR *)value, bufferLength );
			return ((OdbcConnection*) hDbc)->sqlSetConnectAttr( option, 
												(SQLPOINTER)(SQLCHAR*)Value, Value );
		}
	}

	return ((OdbcConnection*) hDbc)->sqlSetConnectAttr( option, (SQLPOINTER)value, 0 );
}


///// SQLSpecialColumnsW /////

SQLRETURN SQL_API SQLSpecialColumnsW( SQLHSTMT hStmt, SQLUSMALLINT identifierType,
									 SQLWCHAR *catalogName, SQLSMALLINT nameLength1,
									 SQLWCHAR *schemaName, SQLSMALLINT nameLength2,
									 SQLWCHAR *tableName, SQLSMALLINT nameLength3,
									 SQLUSMALLINT scope, SQLUSMALLINT nullable )
{
	TRACE ("SQLSpecialColumnsW");
	GUARD_HSTMT( hStmt );

	ConvertingString<> CatalogName( catalogName, nameLength1 );
	ConvertingString<> SchemaName( schemaName, nameLength2 );
	ConvertingString<> TableName( tableName, nameLength3 );

	return ((OdbcStatement*) hStmt)->sqlSpecialColumns( identifierType,
														CatalogName, CatalogName,
														SchemaName, SchemaName,
														TableName, TableName,
														scope, nullable);
}

///// SQLStatisticsW /////

SQLRETURN SQL_API SQLStatisticsW( SQLHSTMT hStmt,
								 SQLWCHAR *catalogName, SQLSMALLINT nameLength1,
								 SQLWCHAR *schemaName, SQLSMALLINT nameLength2,
								 SQLWCHAR *tableName, SQLSMALLINT nameLength3,
								 SQLUSMALLINT unique, SQLUSMALLINT reserved )
{
	TRACE ("SQLStatisticsW");
	GUARD_HSTMT( hStmt );

	ConvertingString<> CatalogName( catalogName, nameLength1 );
	ConvertingString<> SchemaName( schemaName, nameLength2 );
	ConvertingString<> TableName( tableName, nameLength3 );

	return ((OdbcStatement*) hStmt)->sqlStatistics( CatalogName, CatalogName,
													SchemaName, SchemaName,
													TableName, TableName,
													unique, reserved );
}

///// SQLTablesW /////

SQLRETURN SQL_API SQLTablesW( SQLHSTMT hStmt,
							 SQLWCHAR *catalogName, SQLSMALLINT nameLength1,
							 SQLWCHAR *schemaName, SQLSMALLINT nameLength2,
							 SQLWCHAR *tableName, SQLSMALLINT nameLength3,
							 SQLWCHAR *tableType, SQLSMALLINT nameLength4 )
{
	TRACE ("SQLTablesW");
	GUARD_HSTMT( hStmt );

	ConvertingString<> CatalogName( catalogName, nameLength1 );
	ConvertingString<> SchemaName( schemaName, nameLength2 );
	ConvertingString<> TableName( tableName, nameLength3 );
	ConvertingString<> TableType( tableType, nameLength4 );

	return ((OdbcStatement*) hStmt)->sqlTables( CatalogName, CatalogName,
												SchemaName, SchemaName,
												TableName, TableName,
												TableType, TableType );
}

///// SQLBrowseConnectW /////

SQLRETURN SQL_API SQLBrowseConnectW( SQLHDBC hDbc,
									SQLWCHAR *szConnStrIn, SQLSMALLINT cbConnStrIn,
									SQLWCHAR *szConnStrOut, SQLSMALLINT cbConnStrOutMax,
									SQLSMALLINT *pcbConnStrOut )
{
	TRACE ("SQLBrowseConnectW");
	GUARD_HDBC( hDbc );

	bool isByte = !( cbConnStrIn % 2 );

	ConvertingString<> ConnStrIn( szConnStrIn, isByte ? cbConnStrIn : cbConnStrIn * 2 );
	ConvertingString<> ConnStrOut( cbConnStrOutMax, szConnStrOut, pcbConnStrOut );

	return ((OdbcConnection*) hDbc)->sqlBrowseConnect( ConnStrIn, ConnStrIn,
													  ConnStrOut, ConnStrOut,
													  pcbConnStrOut );
}

///// SQLDataSourcesW /////

SQLRETURN SQL_API SQLDataSourcesW( SQLHENV hEnv,
								  SQLUSMALLINT direction, SQLWCHAR *serverName,
								  SQLSMALLINT bufferLength1, SQLSMALLINT *nameLength1,
								  SQLWCHAR *description, SQLSMALLINT bufferLength2,
								  SQLSMALLINT *nameLength2 )
{
	TRACE ("SQLDataSourcesW");
	GUARD_ENV( hEnv );

	ConvertingString<> ServerName( bufferLength1, serverName, nameLength1 );
	ConvertingString<> Description( bufferLength2, description, nameLength2 );

	return ((OdbcEnv*)hEnv)->sqlDataSources( direction, ServerName,
											ServerName, nameLength1, Description,
											Description, nameLength2 );
}

///// SQLForeignKeysW /////

SQLRETURN SQL_API SQLForeignKeysW( SQLHSTMT hStmt,
								  SQLWCHAR *szPkCatalogName, SQLSMALLINT cbPkCatalogName,
								  SQLWCHAR *szPkSchemaName, SQLSMALLINT cbPkSchemaName,
								  SQLWCHAR *szPkTableName, SQLSMALLINT cbPkTableName,
								  SQLWCHAR *szFkCatalogName, SQLSMALLINT cbFkCatalogName,
								  SQLWCHAR *szFkSchemaName, SQLSMALLINT cbFkSchemaName,
								  SQLWCHAR *szFkTableName, SQLSMALLINT cbFkTableName )
{
	TRACE ("SQLForeignKeysW");
	GUARD_HSTMT( hStmt );

	ConvertingString<> PkCatalogName( szPkCatalogName, cbPkCatalogName );
	ConvertingString<> PkSchemaName( szPkSchemaName, cbPkSchemaName );
	ConvertingString<> PkTableName( szPkTableName, cbPkTableName );
	ConvertingString<> FkCatalogName( szFkCatalogName, cbFkCatalogName );
	ConvertingString<> FkSchemaName( szFkSchemaName, cbFkSchemaName );
	ConvertingString<> FkTableName( szFkTableName, cbFkTableName );

	return ((OdbcStatement*) hStmt)->sqlForeignKeys( PkCatalogName, PkCatalogName,
													PkSchemaName, PkSchemaName,
													PkTableName, PkTableName,
													FkCatalogName, FkCatalogName,
													FkSchemaName, FkSchemaName,
													FkTableName, FkTableName );
}

///// SQLNativeSqlW /////

SQLRETURN SQL_API SQLNativeSqlW( SQLHDBC hDbc,
								SQLWCHAR *szSqlStrIn, SQLINTEGER cbSqlStrIn,
								SQLWCHAR *szSqlStr, SQLINTEGER cbSqlStrMax,
								SQLINTEGER *pcbSqlStr )
{
	TRACE ("SQLNativeSqlW");
	GUARD_HDBC( hDbc );

	if ( cbSqlStrIn == SQL_NTS )
		cbSqlStrIn = wcslen( szSqlStrIn );
	
	bool isByte = !( cbSqlStrIn % 2 );

	ConvertingString<> SqlStrIn( szSqlStrIn, cbSqlStrIn );
	ConvertingString<SQLINTEGER> SqlStr( cbSqlStrMax, szSqlStr, pcbSqlStr,
															isByte ? true : false );

	return ((OdbcConnection*) hDbc)->sqlNativeSql( SqlStrIn, SqlStrIn,
												  SqlStr, SqlStr, pcbSqlStr );
}

///// SQLPrimaryKeysW /////

SQLRETURN SQL_API SQLPrimaryKeysW( SQLHSTMT hStmt, 
								  SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
								  SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName,
								  SQLWCHAR *szTableName, SQLSMALLINT cbTableName )
{
	TRACE ("SQLPrimaryKeysW");
	GUARD_HSTMT( hStmt );

	ConvertingString<> CatalogName( szCatalogName, cbCatalogName );
	ConvertingString<> SchemaName( szSchemaName, cbSchemaName );
	ConvertingString<> TableName( szTableName, cbTableName );

	return ((OdbcStatement*) hStmt)->sqlPrimaryKeys( CatalogName, CatalogName,
													SchemaName, SchemaName,
													TableName, TableName );
}

///// SQLProcedureColumnsW /////

SQLRETURN SQL_API SQLProcedureColumnsW( SQLHSTMT hStmt,
									   SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
									   SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName,
									   SQLWCHAR *szProcName, SQLSMALLINT cbProcName,
									   SQLWCHAR *szColumnName, SQLSMALLINT cbColumnName )
{
	TRACE ("SQLProcedureColumnsW");
	GUARD_HSTMT( hStmt );

	ConvertingString<> CatalogName( szCatalogName, cbCatalogName );
	ConvertingString<> SchemaName( szSchemaName, cbSchemaName );
	ConvertingString<> ProcName( szProcName, cbProcName );
	ConvertingString<> ColumnName( szColumnName, cbColumnName );

	return ((OdbcStatement*) hStmt)->sqlProcedureColumns( CatalogName, CatalogName,
														 SchemaName, SchemaName,
														 ProcName, ProcName,
														 ColumnName, ColumnName );
}

///// SQLProceduresW /////

SQLRETURN SQL_API SQLProceduresW( SQLHSTMT hStmt,
								 SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
								 SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName,
								 SQLWCHAR *szProcName, SQLSMALLINT cbProcName )
{
	TRACE ("SQLProceduresW");
	GUARD_HSTMT( hStmt );

	ConvertingString<> CatalogName( szCatalogName, cbCatalogName );
	ConvertingString<> SchemaName( szSchemaName, cbSchemaName );
	ConvertingString<> ProcName( szProcName, cbProcName );

	return ((OdbcStatement*) hStmt)->sqlProcedures( CatalogName, CatalogName,
												   SchemaName, SchemaName,
												   ProcName, ProcName );
}

///// SQLTablePrivilegesW /////

SQLRETURN SQL_API SQLTablePrivilegesW( SQLHSTMT hStmt,
									  SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
									  SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName,
									  SQLWCHAR *szTableName, SQLSMALLINT cbTableName )
{
	TRACE ("SQLTablePrivilegesW");
	GUARD_HSTMT( hStmt );

	ConvertingString<> CatalogName( szCatalogName, cbCatalogName );
	ConvertingString<> SchemaName( szSchemaName, cbSchemaName );
	ConvertingString<> TableName( szTableName, cbTableName );

	return ((OdbcStatement*) hStmt)->sqlTablePrivileges( CatalogName, CatalogName,
														SchemaName, SchemaName,
														TableName, TableName );
}

///// SQLColumnPrivilegesW /////

SQLRETURN SQL_API SQLColumnPrivilegesW( SQLHSTMT hStmt,
									   SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
									   SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName,
									   SQLWCHAR *szTableName, SQLSMALLINT cbTableName,
									   SQLWCHAR *szColumnName, SQLSMALLINT cbColumnName )
{
	TRACE ("SQLColumnPrivilegesW");
	GUARD_HSTMT( hStmt );

	ConvertingString<> CatalogName( szCatalogName, cbCatalogName );
	ConvertingString<> SchemaName( szSchemaName, cbSchemaName );
	ConvertingString<> TableName( szTableName, cbTableName );
	ConvertingString<> ColumnName( szColumnName, cbColumnName );

	return ((OdbcStatement*) hStmt)->sqlColumnPrivileges( CatalogName, CatalogName,
														 SchemaName, SchemaName,
														 TableName, TableName,
														 ColumnName, ColumnName );
}

///// SQLDriversW /////

SQLRETURN SQL_API SQLDriversW( SQLHENV hEnv, SQLUSMALLINT fDirection,
							  SQLWCHAR *szDriverDesc, SQLSMALLINT cbDriverDescMax,
							  SQLSMALLINT *pcbDriverDesc,
							  SQLWCHAR *szDriverAttributes, SQLSMALLINT cbDrvrAttrMax,
							  SQLSMALLINT *pcbDrvrAttr )
{
	TRACE ("SQLDriversW");
	GUARD_ENV( hEnv );

	ConvertingString<> DriverDesc( cbDriverDescMax, szDriverDesc, pcbDriverDesc );
	ConvertingString<> DriverAttributes( cbDrvrAttrMax, szDriverAttributes, pcbDrvrAttr );

	return ((OdbcEnv*) hEnv)->sqlDrivers( fDirection, DriverDesc, DriverDesc,
										pcbDriverDesc, DriverAttributes, DriverAttributes,
										pcbDrvrAttr );
}

///// SQLColAttributeW ///// ODBC 3.0 ///// ISO 92

SQLRETURN SQL_API SQLColAttributeW( SQLHSTMT hStmt, SQLUSMALLINT columnNumber,
								   SQLUSMALLINT fieldIdentifier, SQLPOINTER characterAttribute,
								   SQLSMALLINT bufferLength, SQLSMALLINT *stringLength,
#ifdef _WIN64
								   SQLLEN *numericAttribute )
#else
								   SQLPOINTER numericAttribute )
#endif
{
	TRACE ("SQLColAttributeW");
	GUARD_HSTMT( hStmt );

	switch ( fieldIdentifier )
	{
	case SQL_DESC_LABEL:
	case SQL_DESC_BASE_COLUMN_NAME:
	case SQL_COLUMN_NAME:
	case SQL_DESC_NAME:
	case SQL_DESC_TYPE_NAME:
	case SQL_DESC_BASE_TABLE_NAME:
	case SQL_DESC_TABLE_NAME:
	case SQL_DESC_SCHEMA_NAME:
	case SQL_DESC_CATALOG_NAME:

		if ( bufferLength > 0 )
		{
			ConvertingString<> CharacterAttribute( bufferLength, 
									(SQLWCHAR *)characterAttribute, stringLength );

			return ((OdbcStatement*)hStmt)->sqlColAttribute( columnNumber, fieldIdentifier,
								(SQLPOINTER)(SQLCHAR*)CharacterAttribute, CharacterAttribute,
								stringLength, numericAttribute );
		}
	}

	return ((OdbcStatement*)hStmt)->sqlColAttribute( columnNumber, fieldIdentifier,
													characterAttribute, bufferLength,
													stringLength, numericAttribute );
}

///// SQLGetConnectAttrW /////

SQLRETURN SQL_API SQLGetConnectAttrW( SQLHDBC hDbc,
								     SQLINTEGER attribute, SQLPOINTER value,
								     SQLINTEGER bufferLength, SQLINTEGER *stringLength )
{
	TRACE ("SQLGetConnectAttrW");
	GUARD_HDBC( hDbc );

	switch ( attribute )
	{
	case SQL_ATTR_CURRENT_CATALOG:
	case SQL_ATTR_TRACEFILE:
	case SQL_ATTR_TRANSLATE_LIB:
		if ( bufferLength > 0 || bufferLength == SQL_NTS )
		{
			ConvertingString<SQLINTEGER> Value( bufferLength, (SQLWCHAR *)value, stringLength );

			return ((OdbcConnection*) hDbc)->sqlGetConnectAttr( attribute, 
											(SQLPOINTER)(SQLCHAR*)Value, Value, stringLength );
		}
	}

	return ((OdbcConnection*) hDbc)->sqlGetConnectAttr( attribute, value,
														bufferLength, stringLength );
}

///// SQLGetDescFieldW /////

SQLRETURN SQL_API SQLGetDescFieldW( SQLHDESC hDesc, SQLSMALLINT recNumber,
								   SQLSMALLINT fieldIdentifier, SQLPOINTER value, 
								   SQLINTEGER bufferLength, SQLINTEGER *stringLength )
{
	TRACE ("SQLGetDescFieldW");
	GUARD_HDESC( hDesc );

	switch ( fieldIdentifier )
	{
	case SQL_DESC_BASE_COLUMN_NAME:
	case SQL_DESC_BASE_TABLE_NAME:
	case SQL_DESC_CATALOG_NAME:
	case SQL_DESC_LABEL:
	case SQL_DESC_LITERAL_PREFIX:
	case SQL_DESC_LITERAL_SUFFIX:
	case SQL_DESC_LOCAL_TYPE_NAME:
	case SQL_DESC_NAME:
	case SQL_DESC_SCHEMA_NAME:
	case SQL_DESC_TABLE_NAME:
	case SQL_DESC_TYPE_NAME:

		if ( bufferLength > 0 || bufferLength == SQL_NTS )
		{
			bool isByte = !( bufferLength % 2 );

			ConvertingString<SQLINTEGER> Value( bufferLength, (SQLWCHAR *)value, stringLength );

			return ((OdbcDesc*) hDesc)->sqlGetDescField( recNumber, fieldIdentifier,
											(SQLPOINTER)(SQLCHAR*)Value, Value, stringLength );
		}
	}

	return ((OdbcDesc*) hDesc)->sqlGetDescField( recNumber, fieldIdentifier,
												value, bufferLength, stringLength );
}

///// SQLGetDescRecW /////

SQLRETURN SQL_API SQLGetDescRecW( SQLHDESC hDesc,
								 SQLSMALLINT recNumber, SQLWCHAR *name,
								 SQLSMALLINT bufferLength, SQLSMALLINT *stringLength,
								 SQLSMALLINT *type, SQLSMALLINT *subType, 
								 SQLLEN     *length, SQLSMALLINT *precision, 
								 SQLSMALLINT *scale, SQLSMALLINT *nullable )
{
	TRACE ("SQLGetDescRecW");
	GUARD_HDESC( hDesc );

	ConvertingString<> Name( bufferLength, name, stringLength );

	return ((OdbcDesc*) hDesc)->sqlGetDescRec( recNumber, Name, Name,
											  stringLength, type, subType, 
											  length, precision, scale, nullable );
}

///// SQLGetDiagFieldW /////

SQLRETURN SQL_API SQLGetDiagFieldW( SQLSMALLINT handleType, SQLHANDLE handle,
								   SQLSMALLINT recNumber, SQLSMALLINT diagIdentifier,
								   SQLPOINTER diagInfo, SQLSMALLINT bufferLength,
								   SQLSMALLINT *stringLength )
{
	TRACE ("SQLGetDiagFieldW");
	GUARD_HTYPE( handle, handleType );

	switch ( diagIdentifier )
	{
	case SQL_DIAG_DYNAMIC_FUNCTION:
	case SQL_DIAG_CLASS_ORIGIN:
	case SQL_DIAG_CONNECTION_NAME:
	case SQL_DIAG_MESSAGE_TEXT:
	case SQL_DIAG_SERVER_NAME:
	case SQL_DIAG_SQLSTATE:
	case SQL_DIAG_SUBCLASS_ORIGIN:

		if ( bufferLength > 0 || bufferLength == SQL_NTS )
		{
			ConvertingString<> DiagInfo( bufferLength, (SQLWCHAR *)diagInfo, stringLength );

			return ((OdbcObject*) handle)->sqlGetDiagField( recNumber, diagIdentifier,
											(SQLPOINTER)(SQLCHAR*)DiagInfo, DiagInfo, stringLength );
		}
	}

	return ((OdbcObject*) handle)->sqlGetDiagField( recNumber, diagIdentifier,
												diagInfo, bufferLength, stringLength );
}

///// SQLGetDiagRecW /////

SQLRETURN SQL_API SQLGetDiagRecW( SQLSMALLINT handleType, SQLHANDLE handle,
								 SQLSMALLINT recNumber, SQLWCHAR *sqlState,
								 SQLINTEGER *nativeError, SQLWCHAR *messageText,
								 SQLSMALLINT bufferLength, SQLSMALLINT *textLength )
{
	TRACE ("SQLGetDiagRecW");
	GUARD_HTYPE( handle, handleType );

	ConvertingString<> State( 12, sqlState );
	ConvertingString<> MessageText( bufferLength, messageText, textLength );

	return ((OdbcObject*) handle)->sqlGetDiagRec( handleType, recNumber, State,
												 nativeError, MessageText,
												 MessageText, textLength );
}

///// SQLGetStmtAttrW /////

SQLRETURN SQL_API SQLGetStmtAttrW( SQLHSTMT hStmt,
								  SQLINTEGER attribute, SQLPOINTER value,
								  SQLINTEGER bufferLength, SQLINTEGER *stringLength )
{
	TRACE ("SQLGetStmtAttrW");
	GUARD_HSTMT( hStmt );

	switch ( attribute )
	{
	case 11999:
	case 11998:
	case 11997:

		if ( bufferLength > 0 || bufferLength == SQL_NTS )
		{
			ConvertingString<SQLINTEGER> Value( bufferLength, (SQLWCHAR *)value, stringLength );

			return ((OdbcStatement*) hStmt)->sqlGetStmtAttr( attribute,
											(SQLPOINTER)(SQLCHAR*)Value, Value, stringLength );
		}
	}

	return ((OdbcStatement*) hStmt)->sqlGetStmtAttr( attribute, value,
													bufferLength, stringLength );
}

///// SQLSetConnectAttrW /////

SQLRETURN SQL_API SQLSetConnectAttrW( SQLHDBC hDbc, SQLINTEGER attribute,
									 SQLPOINTER value, SQLINTEGER stringLength )
{
	TRACE ("SQLSetConnectAttrW");
	GUARD_HDBC( hDbc );

	switch ( attribute )
	{
	case SQL_ATTR_CURRENT_CATALOG:
	case SQL_ATTR_TRACEFILE:
	case SQL_ATTR_TRANSLATE_LIB:

		if ( stringLength > 0 || stringLength == SQL_NTS )
		{
			ConvertingString<> Value( (SQLWCHAR *)value, stringLength );

			return ((OdbcConnection*) hDbc)->sqlSetConnectAttr( attribute,
														(SQLPOINTER)(SQLCHAR*)Value, Value );
		}
	}

	return ((OdbcConnection*) hDbc)->sqlSetConnectAttr( attribute, value, stringLength );
}

///// SQLSetDescFieldW /////

SQLRETURN SQL_API SQLSetDescFieldW( SQLHDESC hDesc,
								   SQLSMALLINT recNumber, SQLSMALLINT fieldIdentifier,
								   SQLPOINTER value, SQLINTEGER bufferLength )
{
	TRACE ("SQLSetDescFieldW");
	GUARD_HDESC( hDesc );

	switch ( fieldIdentifier )
	{
	case SQL_DESC_BASE_COLUMN_NAME:
	case SQL_DESC_BASE_TABLE_NAME:
	case SQL_DESC_CATALOG_NAME:
	case SQL_DESC_LABEL:
	case SQL_DESC_LITERAL_PREFIX:
	case SQL_DESC_LITERAL_SUFFIX:
	case SQL_DESC_LOCAL_TYPE_NAME:
	case SQL_DESC_NAME:
	case SQL_DESC_SCHEMA_NAME:
	case SQL_DESC_TABLE_NAME:
	case SQL_DESC_TYPE_NAME:

		if ( bufferLength > 0 || bufferLength == SQL_NTS )
		{
			int len;
			
			if ( bufferLength == SQL_NTS )
				len = wcslen( (SQLWCHAR *)value );
			else
				len = bufferLength / 2;

			ConvertingString<> Value( (SQLWCHAR *)value, len );

			return ((OdbcDesc*) hDesc)->sqlSetDescField( recNumber, fieldIdentifier,
													(SQLPOINTER)(SQLCHAR*)Value, Value );
		}
	}

	return ((OdbcDesc*) hDesc)->sqlSetDescField( recNumber, fieldIdentifier,
												value, bufferLength );
}

///// SQLSetStmtAttrW /////

SQLRETURN SQL_API SQLSetStmtAttrW( SQLHSTMT hStmt, SQLINTEGER attribute,
								  SQLPOINTER value, SQLINTEGER stringLength )
{
	TRACE ("SQLSetStmtAttrW");
	GUARD_HSTMT( hStmt );

	return ((OdbcStatement*) hStmt)->sqlSetStmtAttr( attribute, value, stringLength );
}
