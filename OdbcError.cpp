/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
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
#include "OdbcError.h"

#define CLASS_ODBC		"ODBC 3.0"
#define CLASS_ISO		"ISO 9075"
#define CODE(code)		{code, 0},
#define HASH_SIZE		101

namespace OdbcJdbcLibrary {

using namespace classJString;

struct Hash {
	const char	*string;
	Hash		*collision;
	};

static Hash codes [] = {
	CODE ("01S00") 
	CODE ("01S01") 
	CODE ("01S02") 
	CODE ("01S06") 
	CODE ("01S07") 
	CODE ("07S01") 
	CODE ("08S01") 
	CODE ("21S01") 
	CODE ("21S02") 
	CODE ("25S01") 
	CODE ("25S02") 
	CODE ("25S03") 
	CODE ("42S01") 
	CODE ("42S02") 
	CODE ("42S11") 
	CODE ("42S12") 
	CODE ("42S21") 
	CODE ("42S22") 
	CODE ("HY095") 
	CODE ("HY097") 
	CODE ("HY098") 
	CODE ("HY099") 
	CODE ("HY100") 
	CODE ("HY101") 
	CODE ("HY105") 
	CODE ("HY107") 
	CODE ("HY109") 
	CODE ("HY110") 
	CODE ("HY111") 
	CODE ("HYT00") 
	CODE ("HYT01") 
	CODE ("IM001") 
	CODE ("IM002") 
	CODE ("IM003") 
	CODE ("IM004") 
	CODE ("IM005") 
	CODE ("IM006") 
	CODE ("IM007") 
	CODE ("IM008") 
	CODE ("IM010") 
	CODE ("IM011") 
	CODE ("IM012")
    {0, 0} };

static Hash *hashTable [HASH_SIZE];
static int init();
static int foo = init();

int init()
{
	for (Hash *code = codes; code->string; ++code)
		{
		int slot = JString::hash (code->string, HASH_SIZE);
		code->collision = hashTable [slot];
		hashTable [slot] = code;
		}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

OdbcError::OdbcError(int code, const char *state, JString errorMsg)
{
	strcpy (sqlState, state);
	nativeCode = code;
	msg = errorMsg;
	next = NULL;
	rowNumber = 0;
	columnNumber = 0;
}

OdbcError::~OdbcError()
{

}

RETCODE OdbcError::sqlGetDiagRec(UCHAR * stateBuffer, SDWORD * nativeCodePtr, UCHAR * msgBuffer, int msgBufferLength, SWORD * msgLength)
{
	if (stateBuffer)
		strcpy ((char*) stateBuffer, sqlState);
	
	if (nativeCodePtr)
		*nativeCodePtr = nativeCode;

	--msgBufferLength;
	int length = strlen (msg);

	if (msgLength)
		*msgLength = length;

	if (msgBufferLength <= 0 || !msgBuffer)
		return SQL_SUCCESS_WITH_INFO;

	if (length <= msgBufferLength)
		strcpy ((char*) msgBuffer, msg);
	else
		{
		memcpy (msgBuffer, msg, msgBufferLength);
		msgBuffer [msgBufferLength] = 0;
		return SQL_SUCCESS_WITH_INFO;
		}

	return SQL_SUCCESS;
}

RETCODE OdbcError::sqlGetDiagField(int diagId, SQLPOINTER ptr, int msgBufferLength, SQLSMALLINT *msgLength)
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
			Hash *code;
			string = CLASS_ODBC;
			for (code = hashTable [JString::hash (sqlState, HASH_SIZE)]; code;
				 code = code->collision)
				try
				{
					//if the server isn't running the code->string var 
					//will be null which causes unpredictable results.
					//We should probably present some better info,
					//or handle this further up the chain
					//but at least catching the error will stop
					//some programs from crashing. 
					strcmp (sqlState, code->string);
				}
				catch( ... )
				{
					string = CLASS_ODBC;
					break;
				}

			if ( !code )
				string = CLASS_ISO;
			}
			break;

		case SQL_DIAG_CONNECTION_NAME:
		case SQL_DIAG_SERVER_NAME:
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
	int length = strlen (string);

	if (msgLength)
		*msgLength = length;

	if (msgBufferLength <= 0 || !ptr)
		return SQL_SUCCESS_WITH_INFO;

	if (length <= msgBufferLength)
		{
		strcpy ((char*) msgBuffer, string);
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
