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

#ifdef _WINDOWS
#include <windows.h>
#else
#include <wchar.h>
#endif
#include <stdio.h>
#include "OdbcJdbc.h"
#include "OdbcEnv.h"
#include "OdbcConnection.h"
#include "OdbcStatement.h"
#include "SafeEnvThread.h"
#include "Main.h"

#define GETCONNECT_STMT( hStmt ) (((OdbcStatement*)hStmt)->connection)
#define GETCONNECT_DESC( hDesc ) (((OdbcDesc*)hDesc)->connection)
#define GETCONNECT_HNDL( hObjt ) (((OdbcObject*)hObjt)->getConnection())

extern FILE	*logFile;
using namespace OdbcJdbcLibrary;

#ifdef _WINDOWS
extern UINT codePage; // from Main.cpp
#endif

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
	OdbcConnection *connection;
	
public:
	void setConnection( OdbcConnection *connect )
	{
		connection = connect;
	}

	ConvertingString() 
	{
		isWhy = NONE;
		returnCountOfBytes = true;
		unicodeString = NULL;
		byteString = NULL;
		realLength = NULL;
		lengthString = 0;
		connection = NULL;
	}

	ConvertingString( int length, SQLWCHAR *wcString, TypeRealLen *pLength = NULL, bool retCountOfBytes = true )
	{
		connection = NULL;
		realLength = pLength;
		returnCountOfBytes = retCountOfBytes;

		if ( wcString )
		{
			isWhy = BYTESCHARS;
			unicodeString = wcString;
			if ( length == SQL_NTS )
				lengthString = 0;
			else if ( retCountOfBytes )
				lengthString = length / sizeof(wchar_t);
			else
				lengthString = length;
		}
		else
			isWhy = NONE;

		Alloc();
	}

	ConvertingString( OdbcConnection *connect, SQLWCHAR *wcString, int length )
	{
		connection = connect;
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

	operator SQLCHAR*()	{ return byteString; }
	int getLength() { return lengthString; }

	~ConvertingString() 
	{
		switch ( isWhy )
		{
		case BYTESCHARS:

			if ( unicodeString )
			{
				size_t len;

				if ( connection )
					len = connection->MbsToWcs( (wchar_t*)unicodeString, (const char*)byteString, lengthString );
				else
				{
#ifdef _WINDOWS
					len = MultiByteToWideChar( codePage, 0, (const char*)byteString, -1,
											  unicodeString, lengthString );
					if ( len > 0 )
						len--;
#else
					len = mbstowcs( (wchar_t*)unicodeString, (const char*)byteString, lengthString );
#endif
				}

				if ( len > 0 )
				{
					*(LPWSTR)(unicodeString + len) = L'\0';

					if ( realLength )
					{
						if ( returnCountOfBytes )
							*realLength = (TypeRealLen)( len * 2 );
						else
							*realLength = (TypeRealLen)len;
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
		size_t bytesNeeded;
		wchar_t *ptEndWC = NULL;
		wchar_t saveWC;

		if ( length == SQL_NTS )
			length = (int)wcslen( (const wchar_t*)wcString );
		else if ( wcString[length] != L'\0' )
		{
			ptEndWC = (wchar_t*)&wcString[length];
			saveWC = *ptEndWC;
			*ptEndWC = L'\0';
		}

		if ( connection )
			bytesNeeded = connection->WcsToMbs( NULL, (const wchar_t*)wcString, length );
		else
		{
#ifdef _WINDOWS
			bytesNeeded = WideCharToMultiByte( codePage, (DWORD)0, wcString, length, NULL, (int)0, NULL, NULL );
#else
			bytesNeeded = wcstombs( NULL, (const wchar_t*)wcString, length );
#endif
		}

		byteString = new SQLCHAR[ bytesNeeded + 2 ];

		if ( connection )
			bytesNeeded = connection->WcsToMbs( (char *)byteString, (const wchar_t*)wcString, bytesNeeded );
		else
		{
#ifdef _WINDOWS
			bytesNeeded = WideCharToMultiByte( codePage, 0, wcString, length, (LPSTR)byteString, (int)bytesNeeded, NULL, NULL );
#else
			bytesNeeded = wcstombs( (char *)byteString, (const wchar_t*)wcString, bytesNeeded );
#endif
		}

		byteString[ bytesNeeded ] = '\0';
		lengthString = (int)bytesNeeded;

		if ( ptEndWC )
			*ptEndWC = saveWC;

		return byteString;
	}

protected:
	void Alloc()
	{
		switch ( isWhy )
		{
		case BYTESCHARS:
			if ( lengthString )
			{
				byteString = new SQLCHAR[ lengthString + 2 ];
				memset(byteString, 0, lengthString + 2); 
			}
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
			CharacterAttribute.setConnection( GETCONNECT_STMT( hStmt ) );

			return ((OdbcStatement*) hStmt)->sqlColAttribute( columnNumber, fieldIdentifier,
											(SQLPOINTER)(SQLCHAR*)CharacterAttribute, CharacterAttribute.getLength(),
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

	ConvertingString<> ServerName( (OdbcConnection*)hDbc, serverName, nameLength1 );
	ConvertingString<> UserName( (OdbcConnection*)hDbc, userName, nameLength2 );
	ConvertingString<> Authentication( (OdbcConnection*)hDbc, authentication, nameLength3 );

	SQLRETURN ret = ((OdbcConnection*) hDbc)->sqlConnect( ServerName, ServerName.getLength(), UserName,
												UserName.getLength(), Authentication, Authentication.getLength() );
	LOG_PRINT(( logFile, 
				"SQLConnectW           : Line %d\n"
				"   +status            : %d\n"
				"   +hDbc              : %p\n"
				"   +serverName        : %S\n"
				"   +userName          : %S\n"
				"   +authentication    : %S\n\n",
					__LINE__,
					ret,
					hDbc,
					serverName ? serverName : (SQLWCHAR*)"",
					userName ? userName : (SQLWCHAR*)"",
					authentication ? authentication : (SQLWCHAR*)"" ));

	return ret;
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
	ColumnName.setConnection( GETCONNECT_STMT( hStmt ) );

	return ((OdbcStatement*) hStmt)->sqlDescribeCol( columnNumber,
													ColumnName, ColumnName.getLength(),
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
	ConvertingString<> Buffer( msgBufferLength, msgBuffer, msgLength, false );

	if ( hStmt )
	{
		GUARD_HSTMT( hStmt );
		Buffer.setConnection( GETCONNECT_STMT( hStmt ) );
		return ((OdbcStatement*)hStmt)->sqlError( State, nativeErrorCode, Buffer,
												 Buffer.getLength(), msgLength );
	}
	if ( hDbc )
	{
		GUARD_HDBC( hDbc );
		Buffer.setConnection( (OdbcConnection*)hDbc );
		return ((OdbcConnection*)hDbc)->sqlError( State, nativeErrorCode, Buffer,
												 Buffer.getLength(), msgLength );
	}
	if ( hEnv )
		return ((OdbcEnv*)hEnv)->sqlError( State, nativeErrorCode, Buffer,
										  Buffer.getLength(), msgLength );

	return SQL_ERROR;
}

///// SQLExecDirectW /////	ODBC 1.0	///// ISO 92

SQLRETURN SQL_API SQLExecDirectW( SQLHSTMT hStmt, SQLWCHAR *statementText, SQLINTEGER textLength )
{
	TRACE ("SQLExecDirectW");
	GUARD_HSTMT( hStmt );

	ConvertingString<> StatementText( GETCONNECT_STMT( hStmt ), statementText, textLength );

	return ((OdbcStatement*) hStmt)->sqlExecDirect( StatementText, StatementText.getLength() );
}

///// SQLGetCursorNameW /////	ODBC 1.0	///// ISO 92

SQLRETURN SQL_API SQLGetCursorNameW( SQLHSTMT hStmt, SQLWCHAR *cursorName,
									SQLSMALLINT bufferLength,  SQLSMALLINT *nameLength )
{
	TRACE ("SQLGetCursorNameW");
	GUARD_HSTMT( hStmt );

	bool isByte = false;
	ConvertingString<> CursorName( bufferLength, cursorName, nameLength, isByte );
	CursorName.setConnection( GETCONNECT_STMT( hStmt ) );

	return ((OdbcStatement*) hStmt)->sqlGetCursorName( CursorName, CursorName.getLength(), nameLength );
}

///// SQLPrepareW /////	ODBC 1.0	///// ISO 92

SQLRETURN SQL_API SQLPrepareW( SQLHSTMT hStmt,
							 SQLWCHAR *statementText, SQLINTEGER textLength )
{
	TRACE ("SQLPrepareW");
	GUARD_HSTMT( hStmt );

	ConvertingString<> StatementText( GETCONNECT_STMT( hStmt ), statementText, textLength );
	
	return ((OdbcStatement*) hStmt)->sqlPrepare( StatementText, StatementText.getLength() );
}

///// SQLSetCursorNameW /////	ODBC 1.0	///// ISO 92

SQLRETURN SQL_API SQLSetCursorNameW( SQLHSTMT hStmt, SQLWCHAR *cursorName,
									SQLSMALLINT nameLength )
{
	TRACE ("SQLSetCursorNameW");
	GUARD_HSTMT( hStmt );

	ConvertingString<> CursorName( GETCONNECT_STMT( hStmt ), cursorName, nameLength );

	return ((OdbcStatement*) hStmt)->sqlSetCursorName( CursorName, CursorName.getLength() );
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

	ConvertingString<> CatalogName( GETCONNECT_STMT( hStmt ), catalogName, nameLength1 );
	ConvertingString<> SchemaName( GETCONNECT_STMT( hStmt ), schemaName, nameLength2 );
	ConvertingString<> TableName( GETCONNECT_STMT( hStmt ), tableName, nameLength3 );
	ConvertingString<> ColumnName( GETCONNECT_STMT( hStmt ), columnName, nameLength4 );

	return ((OdbcStatement*) hStmt)->sqlColumns( CatalogName, CatalogName.getLength(),
												SchemaName, SchemaName.getLength(),
												TableName, TableName.getLength(),
												ColumnName, ColumnName.getLength() );
}

///// SQLDriverConnectW /////

SQLRETURN SQL_API SQLDriverConnectW( SQLHDBC hDbc, SQLHWND hWnd, SQLWCHAR *szConnStrIn,
								 	SQLSMALLINT cbConnStrIn, SQLWCHAR *szConnStrOut,
									SQLSMALLINT cbConnStrOutMax, SQLSMALLINT *pcbConnStrOut,
									SQLUSMALLINT fDriverCompletion )
{
	TRACE ("SQLDriverConnectW");
	GUARD_HDBC( hDbc );

	ConvertingString<> ConnStrIn( (OdbcConnection*)hDbc, szConnStrIn, cbConnStrIn );
	ConvertingString<> ConnStrOut( cbConnStrOutMax, szConnStrOut, pcbConnStrOut, false );
	ConnStrOut.setConnection( (OdbcConnection*)hDbc );

	SQLRETURN ret = ((OdbcConnection*) hDbc)->sqlDriverConnect( hWnd, ConnStrIn, ConnStrIn.getLength(),
													ConnStrOut, ConnStrOut.getLength(), pcbConnStrOut,
													fDriverCompletion );
	LOG_PRINT(( logFile, 
				"SQLDriverConnectW     : Line %d\n"
				"   +status            : %d\n"
				"   +hDbc              : %p\n"
				"   +szConnStrIn       : %S\n"
				"   +szConnStrOut      : %S\n\n",
					__LINE__,
					ret,
					hDbc,
					szConnStrIn ? szConnStrIn : (SQLWCHAR*)"",
					szConnStrOut ? szConnStrOut : (SQLWCHAR*)"" ));

	return ret;
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
			ConnStrOut.setConnection( (OdbcConnection*)hDbc );

			return ((OdbcConnection*) hDbc)->sqlGetConnectAttr( option,
													ConnStrOut, ConnStrOut.getLength(), NULL );
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
			InfoValue.setConnection( (OdbcConnection*)hDbc );

			return ((OdbcConnection*) hDbc)->sqlGetInfo( infoType, (SQLPOINTER)(SQLCHAR*)InfoValue,
														InfoValue.getLength(), stringLength );
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

			ConvertingString<> Value( (OdbcConnection*)hDbc, (SQLWCHAR *)value, bufferLength );

			return ((OdbcConnection*) hDbc)->sqlSetConnectAttr( option, 
												(SQLPOINTER)(SQLCHAR*)Value, Value.getLength() );
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

	ConvertingString<> CatalogName( GETCONNECT_STMT( hStmt ), catalogName, nameLength1 );
	ConvertingString<> SchemaName( GETCONNECT_STMT( hStmt ), schemaName, nameLength2 );
	ConvertingString<> TableName( GETCONNECT_STMT( hStmt ), tableName, nameLength3 );

	return ((OdbcStatement*) hStmt)->sqlSpecialColumns( identifierType,
														CatalogName, CatalogName.getLength(),
														SchemaName, SchemaName.getLength(),
														TableName, TableName.getLength(),
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

	ConvertingString<> CatalogName( GETCONNECT_STMT( hStmt ), catalogName, nameLength1 );
	ConvertingString<> SchemaName( GETCONNECT_STMT( hStmt ), schemaName, nameLength2 );
	ConvertingString<> TableName( GETCONNECT_STMT( hStmt ), tableName, nameLength3 );

	return ((OdbcStatement*) hStmt)->sqlStatistics( CatalogName, CatalogName.getLength(),
													SchemaName, SchemaName.getLength(),
													TableName, TableName.getLength(),
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

	ConvertingString<> CatalogName( GETCONNECT_STMT( hStmt ), catalogName, nameLength1 );
	ConvertingString<> SchemaName( GETCONNECT_STMT( hStmt ), schemaName, nameLength2 );
	ConvertingString<> TableName( GETCONNECT_STMT( hStmt ), tableName, nameLength3 );
	ConvertingString<> TableType( GETCONNECT_STMT( hStmt ), tableType, nameLength4 );

	return ((OdbcStatement*) hStmt)->sqlTables( CatalogName, CatalogName.getLength(),
												SchemaName, SchemaName.getLength(),
												TableName, TableName.getLength(),
												TableType, TableType.getLength() );
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

	ConvertingString<> ConnStrIn( (OdbcConnection*)hDbc, szConnStrIn, isByte ? cbConnStrIn : cbConnStrIn * 2 );
	ConvertingString<> ConnStrOut( cbConnStrOutMax, szConnStrOut, pcbConnStrOut );
	ConnStrOut.setConnection( (OdbcConnection*)hDbc );

	return ((OdbcConnection*) hDbc)->sqlBrowseConnect( ConnStrIn, ConnStrIn.getLength(),
													  ConnStrOut, ConnStrOut.getLength(),
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
											ServerName.getLength(), nameLength1, Description,
											Description.getLength(), nameLength2 );
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

	ConvertingString<> PkCatalogName( GETCONNECT_STMT( hStmt ), szPkCatalogName, cbPkCatalogName );
	ConvertingString<> PkSchemaName( GETCONNECT_STMT( hStmt ), szPkSchemaName, cbPkSchemaName );
	ConvertingString<> PkTableName( GETCONNECT_STMT( hStmt ), szPkTableName, cbPkTableName );
	ConvertingString<> FkCatalogName( GETCONNECT_STMT( hStmt ), szFkCatalogName, cbFkCatalogName );
	ConvertingString<> FkSchemaName( GETCONNECT_STMT( hStmt ), szFkSchemaName, cbFkSchemaName );
	ConvertingString<> FkTableName( GETCONNECT_STMT( hStmt ), szFkTableName, cbFkTableName );

	return ((OdbcStatement*) hStmt)->sqlForeignKeys( PkCatalogName, PkCatalogName.getLength(),
													PkSchemaName, PkSchemaName.getLength(),
													PkTableName, PkTableName.getLength(),
													FkCatalogName, FkCatalogName.getLength(),
													FkSchemaName, FkSchemaName.getLength(),
													FkTableName, FkTableName.getLength() );
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
		cbSqlStrIn = (SQLINTEGER)wcslen( (const wchar_t*)szSqlStrIn );
	
	bool isByte = !( cbSqlStrIn % 2 );

	ConvertingString<> SqlStrIn( (OdbcConnection*)hDbc, szSqlStrIn, cbSqlStrIn );
	ConvertingString<SQLINTEGER> SqlStr( cbSqlStrMax, szSqlStr, pcbSqlStr,
															isByte ? true : false );
	SqlStr.setConnection( (OdbcConnection*)hDbc );

	return ((OdbcConnection*) hDbc)->sqlNativeSql( SqlStrIn, SqlStrIn.getLength(),
												  SqlStr, SqlStr.getLength(), pcbSqlStr );
}

///// SQLPrimaryKeysW /////

SQLRETURN SQL_API SQLPrimaryKeysW( SQLHSTMT hStmt, 
								  SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
								  SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName,
								  SQLWCHAR *szTableName, SQLSMALLINT cbTableName )
{
	TRACE ("SQLPrimaryKeysW");
	GUARD_HSTMT( hStmt );

	ConvertingString<> CatalogName( GETCONNECT_STMT( hStmt ), szCatalogName, cbCatalogName );
	ConvertingString<> SchemaName( GETCONNECT_STMT( hStmt ), szSchemaName, cbSchemaName );
	ConvertingString<> TableName( GETCONNECT_STMT( hStmt ), szTableName, cbTableName );

	return ((OdbcStatement*) hStmt)->sqlPrimaryKeys( CatalogName, CatalogName.getLength(),
													SchemaName, SchemaName.getLength(),
													TableName, TableName.getLength() );
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

	ConvertingString<> CatalogName( GETCONNECT_STMT( hStmt ), szCatalogName, cbCatalogName );
	ConvertingString<> SchemaName( GETCONNECT_STMT( hStmt ), szSchemaName, cbSchemaName );
	ConvertingString<> ProcName( GETCONNECT_STMT( hStmt ), szProcName, cbProcName );
	ConvertingString<> ColumnName( GETCONNECT_STMT( hStmt ), szColumnName, cbColumnName );

	return ((OdbcStatement*) hStmt)->sqlProcedureColumns( CatalogName, CatalogName.getLength(),
														 SchemaName, SchemaName.getLength(),
														 ProcName, ProcName.getLength(),
														 ColumnName, ColumnName.getLength() );
}

///// SQLProceduresW /////

SQLRETURN SQL_API SQLProceduresW( SQLHSTMT hStmt,
								 SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
								 SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName,
								 SQLWCHAR *szProcName, SQLSMALLINT cbProcName )
{
	TRACE ("SQLProceduresW");
	GUARD_HSTMT( hStmt );

	ConvertingString<> CatalogName( GETCONNECT_STMT( hStmt ), szCatalogName, cbCatalogName );
	ConvertingString<> SchemaName( GETCONNECT_STMT( hStmt ), szSchemaName, cbSchemaName );
	ConvertingString<> ProcName( GETCONNECT_STMT( hStmt ), szProcName, cbProcName );

	return ((OdbcStatement*) hStmt)->sqlProcedures( CatalogName, CatalogName.getLength(),
												   SchemaName, SchemaName.getLength(),
												   ProcName, ProcName.getLength() );
}

///// SQLTablePrivilegesW /////

SQLRETURN SQL_API SQLTablePrivilegesW( SQLHSTMT hStmt,
									  SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
									  SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName,
									  SQLWCHAR *szTableName, SQLSMALLINT cbTableName )
{
	TRACE ("SQLTablePrivilegesW");
	GUARD_HSTMT( hStmt );

	ConvertingString<> CatalogName( GETCONNECT_STMT( hStmt ), szCatalogName, cbCatalogName );
	ConvertingString<> SchemaName( GETCONNECT_STMT( hStmt ), szSchemaName, cbSchemaName );
	ConvertingString<> TableName( GETCONNECT_STMT( hStmt ), szTableName, cbTableName );

	return ((OdbcStatement*) hStmt)->sqlTablePrivileges( CatalogName, CatalogName.getLength(),
														SchemaName, SchemaName.getLength(),
														TableName, TableName.getLength() );
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

	ConvertingString<> CatalogName( GETCONNECT_STMT( hStmt ), szCatalogName, cbCatalogName );
	ConvertingString<> SchemaName( GETCONNECT_STMT( hStmt ), szSchemaName, cbSchemaName );
	ConvertingString<> TableName( GETCONNECT_STMT( hStmt ), szTableName, cbTableName );
	ConvertingString<> ColumnName( GETCONNECT_STMT( hStmt ), szColumnName, cbColumnName );

	return ((OdbcStatement*) hStmt)->sqlColumnPrivileges( CatalogName, CatalogName.getLength(),
														 SchemaName, SchemaName.getLength(),
														 TableName, TableName.getLength(),
														 ColumnName, ColumnName.getLength() );
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

	return ((OdbcEnv*) hEnv)->sqlDrivers( fDirection, DriverDesc, DriverDesc.getLength(),
										pcbDriverDesc, DriverAttributes, DriverAttributes.getLength(),
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
			CharacterAttribute.setConnection( GETCONNECT_STMT( hStmt ) );

			return ((OdbcStatement*)hStmt)->sqlColAttribute( columnNumber, fieldIdentifier,
								(SQLPOINTER)(SQLCHAR*)CharacterAttribute, CharacterAttribute.getLength(),
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
			Value.setConnection( (OdbcConnection*)hDbc );

			return ((OdbcConnection*) hDbc)->sqlGetConnectAttr( attribute, 
											(SQLPOINTER)(SQLCHAR*)Value, Value.getLength(), stringLength );
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
			Value.setConnection( GETCONNECT_DESC( hDesc ) );

			return ((OdbcDesc*) hDesc)->sqlGetDescField( recNumber, fieldIdentifier,
											(SQLPOINTER)(SQLCHAR*)Value, Value.getLength(), stringLength );
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
	Name.setConnection( GETCONNECT_DESC( hDesc ) );

	return ((OdbcDesc*) hDesc)->sqlGetDescRec( recNumber, Name, Name.getLength(),
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
			DiagInfo.setConnection( GETCONNECT_HNDL( handle ) );

			return ((OdbcObject*) handle)->sqlGetDiagField( recNumber, diagIdentifier,
											(SQLPOINTER)(SQLCHAR*)DiagInfo, DiagInfo.getLength(), stringLength );
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
// MSDN
// DOC: ODBC Spec Incorrectly says SQLGetDiagRecW Takes in Bufferlength as the Number of Bytes
// PSS ID Number: 243526
// Article Last Modified on 8/23/2001

// should be Number of Bytes!!!
// however ODBC.DLL wait Number of Character
	bool isByte = false;
	ConvertingString<> MessageText( bufferLength, messageText, textLength, isByte );
	MessageText.setConnection( GETCONNECT_HNDL( handle ) );

	return ((OdbcObject*) handle)->sqlGetDiagRec( handleType, recNumber, State,
												 nativeError, MessageText,
												 MessageText.getLength(), textLength );
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
	case SQL_FBGETSTMT_PLAN:
	case SQL_FBGETSTMT_TYPE:
	case SQL_FBGETSTMT_INFO:

		if ( bufferLength <= SQL_LEN_BINARY_ATTR_OFFSET )
			bufferLength = -bufferLength + SQL_LEN_BINARY_ATTR_OFFSET;
		else if ( bufferLength > 0 || bufferLength == SQL_NTS )
		{
			ConvertingString<SQLINTEGER> Value( bufferLength, (SQLWCHAR *)value, stringLength );
			Value.setConnection( GETCONNECT_STMT( hStmt ) );

			return ((OdbcStatement*) hStmt)->sqlGetStmtAttr( attribute,
											(SQLPOINTER)(SQLCHAR*)Value, Value.getLength(), stringLength );
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
			ConvertingString<> Value( (OdbcConnection*)hDbc, (SQLWCHAR *)value, stringLength );

			return ((OdbcConnection*) hDbc)->sqlSetConnectAttr( attribute,
														(SQLPOINTER)(SQLCHAR*)Value, Value.getLength() );
		}
	}

	if ( stringLength <= SQL_LEN_BINARY_ATTR_OFFSET )
		stringLength = -stringLength + SQL_LEN_BINARY_ATTR_OFFSET;

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
				len = (int)wcslen( (const wchar_t*)value );
			else
				len = bufferLength / sizeof(wchar_t);

			ConvertingString<> Value( GETCONNECT_DESC( hDesc ), (SQLWCHAR *)value, len );

			return ((OdbcDesc*) hDesc)->sqlSetDescField( recNumber, fieldIdentifier,
													(SQLPOINTER)(SQLCHAR*)Value, Value.getLength() );
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
