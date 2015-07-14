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
 */

// OdbcError.cpp: implementation of the OdbcError class.
//
//////////////////////////////////////////////////////////////////////

#include <string.h>
#include "OdbcJdbc.h"
#include "OdbcEnv.h"
#include "OdbcConnection.h"
#include "OdbcError.h"

#define LABEL_ODBC				"[ODBC Firebird Driver]";

#define CLASS_ODBC				"ODBC 3.0"
#define CLASS_ISO				"ISO 9075"
#define CODE_ISO(code,mes)		{false, code, NULL},
#define CODE_ODBC(code,mes)		{true, code, NULL},
#define HASH_SIZE				211
#define HASH_SIZE_ERROR			21
#define CODE_ERR(linkOdbc,code)	{linkOdbc, code, NULL},

static inline unsigned CALC_HASH(const char* pt)
{
	unsigned int x;
	memcpy(&x, pt + 1, sizeof(x));
	return x % HASH_SIZE;
}

namespace OdbcJdbcLibrary {

using namespace classJString;

struct ListErrorLinks
{
	short			linkOdbcError;
	int				codeError;
	ListErrorLinks	*collision;

} listSqlErrorLink[] = {

	CODE_ERR( 53,    -1 ) // 42000 :: Syntax error
	CODE_ERR(  0,   -84 ) // 
	CODE_ERR(  0,   -85 ) // 
	CODE_ERR(  0,  -103 ) // 
	CODE_ERR(  0,  -104 ) // 
	CODE_ERR(  0,  -105 ) // 
	CODE_ERR(  0,  -150 ) // 
	CODE_ERR(  0,  -151 ) // 
	CODE_ERR(  0,  -155 ) // 
	CODE_ERR(  0,  -157 ) // 
	CODE_ERR(  0,  -158 ) // 
	CODE_ERR(  0,  -162 ) // 
	CODE_ERR(  0,  -170 ) // 
	CODE_ERR(  0,  -171 ) // 
	CODE_ERR(  0,  -172 ) // 
	CODE_ERR(  0,  -203 ) // 
	CODE_ERR( 55,  -204 ) // 42S02 :: isc_dsql_relation_err
	CODE_ERR(  0,  -205 ) // 
	CODE_ERR(  0,  -206 ) // 
	CODE_ERR(  0,  -208 ) // 
	CODE_ERR(  0,  -219 ) // 
	CODE_ERR(  0,  -230 ) // 
	CODE_ERR(  0,  -231 ) // 
	CODE_ERR(  0,  -232 ) // 
	CODE_ERR(  0,  -233 ) // 
	CODE_ERR(  0,  -234 ) // 
	CODE_ERR(  0,  -235 ) // 
	CODE_ERR(  0,  -236 ) // 
	CODE_ERR(  0,  -237 ) // 
	CODE_ERR(  0,  -238 ) // 
	CODE_ERR(  0,  -239 ) // 
	CODE_ERR(  0,  -240 ) // 
	CODE_ERR(  0,  -241 ) // 
	CODE_ERR(  0,  -242 ) // 
	CODE_ERR(  0,  -243 ) // 
	CODE_ERR(  0,  -244 ) // 
	CODE_ERR(  0,  -245 ) // 
	CODE_ERR(  0,  -246 ) // 
	CODE_ERR(  0,  -247 ) // 
	CODE_ERR(  0,  -248 ) // 
	CODE_ERR(  0,  -249 ) // 
	CODE_ERR(  0,  -250 ) // 
	CODE_ERR(  0,  -251 ) // 
	CODE_ERR(  0,  -252 ) // 
	CODE_ERR(  0,  -253 ) // 
	CODE_ERR(  0,  -254 ) // 
	CODE_ERR(  0,  -255 ) // 
	CODE_ERR(  0,  -257 ) // 
	CODE_ERR(  0,  -258 ) // 
	CODE_ERR(  0,  -259 ) // 
	CODE_ERR(  0,  -260 ) // 
	CODE_ERR(  0,  -261 ) // 
	CODE_ERR(  0,  -281 ) // 
	CODE_ERR(  0,  -282 ) // 
	CODE_ERR(  0,  -283 ) // 
	CODE_ERR(  0,  -284 ) // 
	CODE_ERR(  0,  -291 ) // 
	CODE_ERR(  0,  -292 ) // 
	CODE_ERR(  0,  -293 ) // 
	CODE_ERR(  0,  -294 ) // 
	CODE_ERR(  0,  -295 ) // 
	CODE_ERR(  0,  -296 ) // 
	CODE_ERR(  0,  -297 ) // 
	CODE_ERR(  0,  -313 ) // 
	CODE_ERR(  0,  -314 ) // 
	CODE_ERR(  0,  -315 ) // 
	CODE_ERR(  0,  -383 ) // 
	CODE_ERR(  0,  -401 ) // 
	CODE_ERR(  0,  -402 ) // 
	CODE_ERR(  0,  -406 ) // 
	CODE_ERR(  0,  -407 ) // 
	CODE_ERR(  0,  -413 ) // 
	CODE_ERR(  0,  -501 ) // 
	CODE_ERR(  0,  -502 ) // 
	CODE_ERR(  0,  -504 ) // 
	CODE_ERR(  0,  -508 ) // 
	CODE_ERR(  0,  -510 ) // 
	CODE_ERR(  0,  -518 ) // 
	CODE_ERR(  0,  -519 ) // 
	CODE_ERR(  0,  -530 ) // 
	CODE_ERR(  0,  -531 ) // 
	CODE_ERR(  0,  -532 ) // 
	CODE_ERR(  0,  -551 ) // 
	CODE_ERR(  0,  -552 ) // 
	CODE_ERR(  0,  -553 ) // 
	CODE_ERR(  0,  -595 ) // 
	CODE_ERR(  0,  -596 ) // 
	CODE_ERR(  0,  -597 ) // 
	CODE_ERR(  0,  -598 ) // 
	CODE_ERR(  0,  -599 ) // 
	CODE_ERR(  0,  -600 ) // 
	CODE_ERR(  0,  -601 ) // 
	CODE_ERR(  0,  -604 ) // 
	CODE_ERR(  0,  -605 ) // 
	CODE_ERR(  0,  -607 ) // 
	CODE_ERR(  0,  -612 ) // 
	CODE_ERR(  0,  -615 ) // 
	CODE_ERR(  0,  -616 ) // 
	CODE_ERR(  0,  -617 ) // 
	CODE_ERR(  0,  -618 ) // 
	CODE_ERR(  0,  -625 ) // 
	CODE_ERR(  0,  -637 ) // 
	CODE_ERR(  0,  -660 ) // 
	CODE_ERR(  0,  -663 ) // 
	CODE_ERR(  0,  -664 ) // 
	CODE_ERR(  0,  -677 ) // 
	CODE_ERR(  0,  -685 ) // 
	CODE_ERR(  0,  -689 ) // 
	CODE_ERR(  0,  -690 ) // 
	CODE_ERR(  0,  -691 ) // 
	CODE_ERR(  0,  -692 ) // 
	CODE_ERR(  0,  -693 ) // 
	CODE_ERR(  0,  -694 ) // 
	CODE_ERR(  0,  -802 ) // 
	CODE_ERR(  0,  -803 ) // 
	CODE_ERR(  0,  -804 ) // 
	CODE_ERR(  0,  -806 ) // 
	CODE_ERR(  0,  -807 ) // 
	CODE_ERR(  0,  -808 ) // 
	CODE_ERR(  0,  -809 ) // 
	CODE_ERR(  0,  -810 ) // 
	CODE_ERR(  0,  -811 ) // 
	CODE_ERR(  0,  -816 ) // 
	CODE_ERR(  0,  -817 ) // 
	CODE_ERR(  0,  -820 ) // 
	CODE_ERR(  0,  -823 ) // 
	CODE_ERR(  0,  -824 ) // 
	CODE_ERR(  0,  -825 ) // 
	CODE_ERR(  0,  -826 ) // 
	CODE_ERR(  0,  -827 ) // 
	CODE_ERR(  0,  -828 ) // 
	CODE_ERR(  0,  -829 ) // 
	CODE_ERR(  0,  -830 ) // 
	CODE_ERR(  0,  -831 ) // 
	CODE_ERR(  0,  -832 ) // 
	CODE_ERR(  0,  -833 ) // 
	CODE_ERR(  0,  -834 ) // 
	CODE_ERR(  0,  -835 ) // 
	CODE_ERR(  0,  -836 ) // 
	CODE_ERR(  0,  -837 ) // 
	CODE_ERR(  0,  -838 ) // 
	CODE_ERR(  0,  -839 ) // 
	CODE_ERR(  0,  -840 ) // 
	CODE_ERR(  0,  -841 ) // 
	CODE_ERR(  0,  -842 ) // 
	CODE_ERR(  0,  -901 ) // 
	CODE_ERR(  0,  -902 ) // 
	CODE_ERR(  0,  -904 ) // 
	CODE_ERR(  0,  -906 ) // 
	CODE_ERR(  0,  -909 ) // 
	CODE_ERR(  0,  -911 ) // 
	CODE_ERR( 50,  -913 ) // 40001 :: lock conflict
	CODE_ERR(  0,  -922 ) // 
	CODE_ERR(  0,  -923 ) // 
	CODE_ERR(  0,  -924 ) // 
	CODE_ERR(  0,  -926 ) // 
};

template < int sizeHASH >
class DefaultCalcHash
{
public:
	static int calcHash( const int codeError )
	{
		return ((unsigned short)codeError) % sizeHASH;
	}
};

template< int sizeHASH >
class FbErrorCalcHash
{
public:
	static int calcHash( const int codeError )
	{
		return ((unsigned int)(codeError | 0x0100 )) % sizeHASH;
	}
};

template< int sizeHASH, typename Calc = DefaultCalcHash< sizeHASH > >
class CListErrorLinks
{
	ListErrorLinks **hashTable;

public:
	CListErrorLinks( ListErrorLinks * list, int count ) 
	{ 
		hashTable = new ListErrorLinks*[sizeHASH];
		memset( hashTable, 0, sizeof( ListErrorLinks* ) * sizeHASH );

		ListErrorLinks *row = list;

		do
		{
			int slot = Calc::calcHash( row->codeError );
			row->collision = hashTable[slot];
			hashTable[slot] = row++;

		} while ( --count );
	}

	~CListErrorLinks() 
	{	
		delete[] hashTable;
	}

	bool findError( int sqlError, short &link )
	{
		ListErrorLinks *code = hashTable[ Calc::calcHash( sqlError ) ];

		for ( ; code; code = code->collision )
			if ( sqlError == code->codeError )
			{
				link = code->linkOdbcError;
				if ( !link )
					return false;
				return true;
			}

		return false;
	}
};

ListErrorLinks listErrorLink[] = {

	CODE_ERR( 50,  335544336L ) // 40001 :: isc_deadlock
	CODE_ERR( 50,  335544345L ) // 40001 :: isc_lock_conflict
	CODE_ERR( 39,  335544347L ) // 23000 :: isc_not_valid
	CODE_ERR( 39,  335544349L ) // 23000 :: isc_no_dup
	CODE_ERR( 20,  335544375L ) // 08001 :: isc_unavailable
	CODE_ERR( 23,  335544421L ) // 08004 :: isc_connect_reject
	CODE_ERR( 39,  335544466L ) // 23000 :: isc_foreign_key
	CODE_ERR( 45,  335544472L ) // 28000 :: isc_login
	CODE_ERR( 39,  335544558L ) // 23000 :: isc_check_constraint
	CODE_ERR( 59,  335544578L ) // 42S22 :: isc_dsql_field_err
	CODE_ERR( 55,  335544580L ) // 42S02 :: isc_dsql_relation_err
	CODE_ERR( 25,  335544648L ) // 08S01 :: isc_conn_lost
	CODE_ERR( 39,  335544665L ) // 23000 :: isc_unique_key_violation
	CODE_ERR( 20,  335544721L ) // 08001 :: isc_network_error
	CODE_ERR( 25,  335544726L ) // 08S01 :: isc_net_read_err
	CODE_ERR( 25,  335544727L ) // 08S01 :: isc_net_write_err
	CODE_ERR( 25,  335544741L ) // 08S01 :: isc_lost_db_connection
	CODE_ERR( 23,  335544744L ) // 08004 :: isc_max_att_exceeded
	CODE_ERR( 66,  335544794L ) // HY008 :: isc_cancelled
};

CListErrorLinks< HASH_SIZE > 
	listSqlError( listSqlErrorLink, sizeof( listSqlErrorLink ) / sizeof( *listSqlErrorLink ) );

CListErrorLinks< HASH_SIZE_ERROR, FbErrorCalcHash< HASH_SIZE_ERROR > > 
	listServerError( listErrorLink, sizeof( listErrorLink ) / sizeof( *listErrorLink ) );

struct Hash 
{
	bool		subClassOdbc;
	const char	*string;
	Hash		*collision;
};

//
// Warning! Message not used, it's comments
//
Hash codes [] = {
	CODE_ISO ( "01000", "General warning" )									// 0
	CODE_ISO ( "01001", "Cursor operation conflict" )						// 1
	CODE_ISO ( "01002", "Disconnect error SQLDisconnect" )					// 2		
	CODE_ISO ( "01003", "NULL value eliminated in set function" )			// 3
	CODE_ISO ( "01004", "String data, right truncated" )					// 4	
	CODE_ISO ( "01006", "Privilege not revoked" )							// 5
	CODE_ISO ( "01007", "Privilege not granted" )							// 6
	CODE_ODBC( "01S00", "Invalid connection string attribute" )				// 7
	CODE_ODBC( "01S01", "Error in row" )									// 8
	CODE_ODBC( "01S02", "Option value changed" )							// 9
	CODE_ODBC( "01S06", "Attempt to fetch before the result set returned the first rowset" )	// 10
	CODE_ODBC( "01S07", "Fractional truncation" )							// 11
	CODE_ODBC( "01S08", "Error saving File DSN" )							// 12
	CODE_ODBC( "01S09", "Invalid keyword" )									// 13
	CODE_ISO ( "07001", "Wrong number of parameters" )						// 14
	CODE_ISO ( "07002", "COUNT field incorrect" )							// 15
	CODE_ISO ( "07005", "Prepared statement not a cursor-specification" )	// 16
	CODE_ISO ( "07006", "Restricted data type attribute violation" )		// 17
	CODE_ISO ( "07009", "Invalid descriptor index" )						// 18
	CODE_ODBC( "07S01", "Invalid use of default parameter" )				// 19
	CODE_ISO ( "08001", "Client unable to establish connection" )			// 20
	CODE_ISO ( "08002", "Connection name in use" )							// 21
	CODE_ISO ( "08003", "Connection does not exist" )						// 22
	CODE_ISO ( "08004", "Server rejected the connection" )					// 23
	CODE_ISO ( "08007", "Connection failure during transaction" )			// 24
	CODE_ODBC( "08S01", "Communication link failure" )						// 25
	CODE_ODBC( "21S01", "Insert value list does not match column list" )	// 26
	CODE_ODBC( "21S02", "Degree of derived table does not match column list" )	// 27
	CODE_ISO ( "22001", "String data, right truncated" )					// 28
	CODE_ISO ( "22002", "Indicator variable required but not supplied" )	// 29
	CODE_ISO ( "22003", "Numeric value out of range" )						// 30
	CODE_ISO ( "22007", "Invalid datetime format" )							// 31
	CODE_ISO ( "22008", "Datetime field overflow" )							// 32
	CODE_ISO ( "22012", "Division by zero" )								// 33
	CODE_ISO ( "22015", "Interval field overflow" )							// 34
	CODE_ISO ( "22018", "Invalid character value for cast specification" )	// 35
	CODE_ISO ( "22019", "Invalid escape character" )						// 36
	CODE_ISO ( "22025", "Invalid escape sequence" )							// 37
	CODE_ISO ( "22026", "String data, length mismatch" )					// 38
	CODE_ISO ( "23000", "Integrity constraint violation" )					// 39
	CODE_ISO ( "24000", "Invalid cursor state" )							// 40
	CODE_ISO ( "25000", "Invalid transaction state" )						// 41
	CODE_ODBC( "25S01", "Transaction state" )								// 42
	CODE_ODBC( "25S02", "Transaction is still active" )						// 43
	CODE_ODBC( "25S03", "Transaction is rolled back" )						// 44
	CODE_ISO ( "28000", "Invalid authorization specification" )				// 45
	CODE_ISO ( "34000", "Invalid cursor name" )								// 46
	CODE_ISO ( "3C000", "Duplicate cursor name" )							// 47
	CODE_ISO ( "3D000", "Invalid catalog name" )							// 48
	CODE_ISO ( "3F000", "Invalid schema name" )								// 49
	CODE_ISO ( "40001", "Serialization failure" )							// 50
	CODE_ISO ( "40002", "Integrity constraint violation" )					// 51
	CODE_ISO ( "40003", "Statement completion unknown" )					// 52
	CODE_ISO ( "42000", "Syntax error or access violation" )				// 53
	CODE_ODBC( "42S01", "Base table or view already exists" )				// 54
	CODE_ODBC( "42S02", "Base table or view not found" )					// 55
	CODE_ODBC( "42S11", "Index already exists" )							// 56
	CODE_ODBC( "42S12", "Index not found" )									// 57
	CODE_ODBC( "42S21", "Column already exists" )							// 58
	CODE_ODBC( "42S22", "Column not found" )								// 59
	CODE_ISO ( "44000", "WITH CHECK OPTION violation" )						// 60
	CODE_ISO ( "HY000", "General error" )									// 61
	CODE_ISO ( "HY001", "Memory allocation error" )							// 62
	CODE_ISO ( "HY003", "Invalid application buffer type" )					// 63
	CODE_ISO ( "HY004", "Invalid SQL data type" )							// 64
	CODE_ISO ( "HY007", "Associated statement is not prepared" )			// 65
	CODE_ISO ( "HY008", "Operation canceled" )								// 66
	CODE_ISO ( "HY009", "Invalid use of null pointer" )						// 67
	CODE_ISO ( "HY010", "Function sequence error" )							// 68
	CODE_ISO ( "HY011", "Attribute cannot be set now" )						// 69
	CODE_ISO ( "HY012", "Invalid transaction operation code" )				// 70
	CODE_ISO ( "HY013", "Memory management error" )							// 71
	CODE_ISO ( "HY014", "Limit on the number of handles exceeded" )			// 72
	CODE_ISO ( "HY015", "No cursor name available" )						// 73
	CODE_ISO ( "HY016", "Cannot modify an implementation row descriptor" )	// 74
	CODE_ISO ( "HY017", "Invalid use of an automatically allocated descriptor handle" )	// 75
	CODE_ISO ( "HY018", "Server declined cancel request" )					// 76
	CODE_ISO ( "HY019", "Non-character and non-binary data sent in pieces" )// 77
	CODE_ISO ( "HY020", "Attempt to concatenate a null value" )				// 78
	CODE_ISO ( "HY021", "Inconsistent descriptor information" )				// 79
	CODE_ISO ( "HY024", "Invalid attribute value" )							// 80
	CODE_ISO ( "HY090", "Invalid string or buffer length" )					// 81
	CODE_ISO ( "HY091", "Invalid descriptor field identifier" )				// 82
	CODE_ISO ( "HY092", "Invalid attribute/option identifier" )				// 83
	CODE_ODBC( "HY095", "Function type out of range" )						// 84
	CODE_ISO ( "HY096", "Invalid information type" )						// 85
	CODE_ODBC( "HY097", "Column type out of range" )						// 86
	CODE_ODBC( "HY098", "Scope type out of range" )							// 87
	CODE_ODBC( "HY099", "Nullable type out of range" )						// 88
	CODE_ODBC( "HY100", "Uniqueness option type out of range" )				// 89
	CODE_ODBC( "HY101", "Accuracy option type out of range" )				// 90
	CODE_ISO ( "HY103", "Invalid retrieval code" )							// 91
	CODE_ISO ( "HY104", "Invalid precision or scale value" )				// 92
	CODE_ODBC( "HY105", "Invalid parameter type" )							// 93
	CODE_ISO ( "HY106", "Fetch type out of range" )							// 94
	CODE_ODBC( "HY107", "Row value out of range" )							// 95
	CODE_ODBC( "HY109", "Invalid cursor position" )							// 96
	CODE_ODBC( "HY110", "Invalid driver completion" )						// 97
	CODE_ODBC( "HY111", "Invalid bookmark value" )							// 98
	CODE_ISO ( "HYC00", "Optional feature not implemented" )				// 99
	CODE_ODBC( "HYT00", "Timeout expired" )									// 100
	CODE_ODBC( "HYT01", "Connection timeout expired" )						// 101
	CODE_ODBC( "IM001", "Driver does not support this function" )			// 102
	CODE_ODBC( "IM002", "Data source name not found and no default driver specified" )	// 103
	CODE_ODBC( "IM003", "Specified driver could not be loaded" )			// 104
	CODE_ODBC( "IM004", "Driver's SQLAllocHandle on SQL_HANDLE_ENV failed" )// 105
	CODE_ODBC( "IM005", "Driver's SQLAllocHandle on SQL_HANDLE_DBC failed" )// 106
	CODE_ODBC( "IM006", "Driver's SQLSetConnectAttr failed" )				// 107
	CODE_ODBC( "IM007", "No data source or driver specified; dialog prohibited" )	// 108
	CODE_ODBC( "IM008", "Dialog failed" )									// 109
	CODE_ODBC( "IM009", "Unable to load translation DLL" )					// 110
	CODE_ODBC( "IM010", "Data source name too long" )						// 111
	CODE_ODBC( "IM011", "Driver name too long" )							// 112
	CODE_ODBC( "IM012", "DRIVER keyword syntax error" )						// 113
	CODE_ODBC( "IM013", "Trace file error" )								// 114
	CODE_ODBC( "IM014", "Invalid name of File DSN" )						// 115
	CODE_ODBC( "IM015", "Corrupt file data source" )						// 116
};

class CListOdbcError
{
	Hash **hashTable;

public:
	CListOdbcError() 
	{ 
		hashTable = new Hash*[HASH_SIZE];
		memset( hashTable, 0, sizeof ( Hash* ) * HASH_SIZE );

		int count = sizeof( codes ) / sizeof( *codes );
		Hash *row = codes;

		do
		{
			int slot = CALC_HASH( row->string );
			row->collision = hashTable[slot];
			hashTable[slot] = row++;

		} while ( --count );
	}

	~CListOdbcError()
	{	
		delete[] hashTable;
	}

	bool findError( const char *sqlState, short &link )
	{
		Hash *code = hashTable[CALC_HASH( sqlState )];

		for ( ; code; code = code->collision )
			if ( !strncasecmp( sqlState, code->string, 5 ) ) // HY000 = 5 Always
			{
				link = code - codes;
				return true;
			}

		return false;
	}

} listODBCError;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

OdbcError::OdbcError(int code, const char *state, JString errorMsg)
{
	short link;

	msg = LABEL_ODBC;
	nativeCode = code;

	if ( code && listSqlError.findError( code, link ) )
	{
		Hash &code = codes[link];
		memcpy( sqlState, code.string, 6 );
	}
	else
		memcpy( sqlState, state, 6 ); // Always 6

	msg += errorMsg;
	next = NULL;
	rowNumber = 0;
	columnNumber = 0;
	connection = NULL;
}

OdbcError::OdbcError(int code, int fbcode, const char *state, JString errorMsg)
{
	short link;
	bool changedState = false;

	msg = LABEL_ODBC;
	nativeCode = code;

	if ( fbcode )
	{
		msg += "[Firebird]";

		if ( listServerError.findError( fbcode, link ) )
		{
			Hash &code = codes[link];
			memcpy( sqlState, code.string, 6 );
			changedState = true;
		}
	}

	if ( !changedState )
	{
		if ( code && listSqlError.findError( code, link ) )
		{
			Hash &code = codes[link];
			memcpy( sqlState, code.string, 6 );
		}
		else
			memcpy( sqlState, state, 6 ); // Always 6
	}

	msg += errorMsg;
	next = NULL;
	rowNumber = 0;
	columnNumber = 0;
	connection = NULL;
}

OdbcError::~OdbcError()
{

}

SQLRETURN OdbcError::sqlGetDiagRec(UCHAR * stateBuffer, SQLINTEGER * nativeCodePtr, UCHAR * msgBuffer, int msgBufferLength, SWORD * msgLength)
{
	if (stateBuffer)
		strcpy ((char*) stateBuffer, sqlState);
	
	if (nativeCodePtr)
		*nativeCodePtr = nativeCode;

	--msgBufferLength;
	int length = (int)strlen (msg);

	if (msgLength)
		*msgLength = length;

	if (msgBufferLength <= 0 || !msgBuffer)
		return SQL_SUCCESS_WITH_INFO;

	if (length <= msgBufferLength)
		{
		strcpy ((char*) msgBuffer, msg);
		msgBuffer [length] = 0;
		}
	else
		{
		memcpy (msgBuffer, msg, msgBufferLength);
		msgBuffer [msgBufferLength] = 0;
		return SQL_SUCCESS_WITH_INFO;
		}

	return SQL_SUCCESS;
}

SQLRETURN OdbcError::sqlGetDiagField(int diagId, SQLPOINTER ptr, int msgBufferLength, SQLSMALLINT *msgLength)
{
	const char *string = NULL;
	int value;

	switch (diagId)
		{
		case SQL_DIAG_CLASS_ORIGIN:
			if (sqlState [0] == 'I' && sqlState [1] == 'M')
				string = CLASS_ODBC;
			else
				string = CLASS_ISO;
			break;

		case SQL_DIAG_SUBCLASS_ORIGIN:
			{
			short link;
			string = CLASS_ISO;

			if ( listODBCError.findError( sqlState, link ) )
				if ( codes[link].subClassOdbc )
					string = CLASS_ODBC;
			}
			break;

		case SQL_DIAG_CONNECTION_NAME:
			if ( connection )
				string = connection->dsn;
			else
				string = "";
			break;

		case SQL_DIAG_SERVER_NAME:
			if ( connection && connection->connection )
				string = connection->getMetaData()->getDatabaseProductName();
			else
				string = "";
			break;
		
		case SQL_DIAG_MESSAGE_TEXT:
			string = msg;
			break;
		
		case SQL_DIAG_NATIVE:
			value = nativeCode;
			break;
		
		case SQL_DIAG_SQLSTATE:
			string = sqlState;
			break;			

		case SQL_DIAG_ROW_NUMBER:
			value = rowNumber;
			break;

		case SQL_DIAG_COLUMN_NUMBER:
			value = columnNumber;
			break;

		default:
			return SQL_ERROR;
		}

	if (!string)
		{
		*(SQLINTEGER*) ptr = value;
		return SQL_SUCCESS;
		}

	--msgBufferLength;
	char *msgBuffer = (char*) ptr;
	int length = (int)strlen (string);

	if (msgLength)
		*msgLength = length;

	if (msgBufferLength <= 0 || !ptr)
		return SQL_SUCCESS_WITH_INFO;

	if (length <= msgBufferLength)
		{
		strcpy ((char*) msgBuffer, string);
		msgBuffer [length] = 0;
		return SQL_SUCCESS;
		}

	memcpy (msgBuffer, string, msgBufferLength);
	msgBuffer [msgBufferLength] = 0;

	return SQL_SUCCESS_WITH_INFO;
}

void OdbcError::setRowNumber(int number)
{
	rowNumber = number;
}

void OdbcError::setColumnNumber(int column, int row)
{
	columnNumber = column;
	rowNumber = row;
}

}; // end namespace OdbcJdbcLibrary
