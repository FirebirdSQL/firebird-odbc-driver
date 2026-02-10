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

// OdbcObject.cpp: implementation of the OdbcObject class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include "OdbcJdbc.h"
#include "OdbcEnv.h"
#include "OdbcConnection.h"
#include "OdbcStatement.h"
#include "OdbcObject.h"
#include "OdbcError.h"
#include "IscDbc/SQLException.h"

namespace OdbcJdbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

OdbcObject::OdbcObject()
{
	next = NULL; // NOMEY
	infoPosted = false;
	sqlDiagCursorRowCount = 0;			// SQL_DIAG_CURSOR_ROW_COUNT 
	sqlDiagDynamicFunction = NULL;		// SQL_DIAG_DYNAMIC_FUNCTION 
	sqlDiagDynamicFunctionCode = 0;		// SQL_DIAG_DYNAMIC_FUNCTION_CODE
	sqlDiagNumber = 0;					// SQL_DIAG_NUMBER 
	sqlDiagReturnCode = SQL_SUCCESS;	// SQL_DIAG_RETURNCODE
	sqlDiagRowCount = 0;				// SQL_DIAG_ROW_COUNT
}

OdbcObject::~OdbcObject()
{
	clearErrors();
}

SQLRETURN OdbcObject::returnStringInfo(SQLPOINTER ptr, SQLSMALLINT maxLength, SQLSMALLINT* returnLength, const char * value)
{
	int count = (int)strlen (value);

	if (returnLength)
		*returnLength = count;

	if ( ptr && maxLength > 0 )
	{
		--maxLength;
		if (count <= maxLength)
		{
			strcpy ((char*) ptr, value);
			return sqlSuccess();
		}

		memcpy (ptr, value, maxLength);
		((char*) ptr) [maxLength] = 0;
		// returnLength already set to full count above (per ODBC spec)
	}

	return sqlReturn (SQL_SUCCESS_WITH_INFO, "01004", "String data, right truncated");
}

SQLRETURN OdbcObject::returnStringInfo(SQLPOINTER ptr, SQLSMALLINT maxLength, SQLINTEGER *returnLength, const char *value)
{
	// Delegate to the SQLSMALLINT* overload and widen the result
	SQLSMALLINT shortLength = 0;
	SQLRETURN ret = returnStringInfo(ptr, maxLength, &shortLength, value);
	if (returnLength)
		*returnLength = shortLength;
	return ret;
}

int OdbcObject::sqlReturn(int code, const char * state, const char *text, int nativeCode)
{
	postError (new OdbcError (nativeCode, state, text));

	return code;
}

int OdbcObject::sqlSuccess()
{
	if (infoPosted)
		return SQL_SUCCESS_WITH_INFO;

	return SQL_SUCCESS;
}

SQLRETURN OdbcObject::allocHandle(int handleType, SQLHANDLE * outputHandle)
{
	*outputHandle = (SQLHANDLE)NULL;

	return sqlReturn (SQL_ERROR, "HY092", "Invalid attribute/option identifier");
}

void OdbcObject::notYetImplemented(const char * msg)
{
}

int OdbcObject::stringLength(const SQLCHAR *string, int givenLength)
{
	if (!string)
		return 0;

	if (givenLength == SQL_NTS)
		return (int)strlen ((char*) string);

	return givenLength;
}

// Return "true" on overflow

bool OdbcObject::setString(const SQLCHAR * string, int stringLength, SQLCHAR * target, int targetSize, SQLSMALLINT * targetLength)
{
	--targetSize;

	if (targetLength)
		*targetLength = stringLength;

	if (target)
		{
		if (stringLength <= targetSize)
			{
			memcpy (target, string, stringLength);
			target [stringLength] = 0;
			return false;
			}

		memcpy (target, string, targetSize);
		target [targetSize] = 0;
		postError (new OdbcError (0, "01004", "String data, right truncated"));
		}

	return true;
}

bool OdbcObject::setString(const char * string, SQLCHAR *target, int targetSize, SQLSMALLINT * targetLength)
{
	return setString ((SQLCHAR*) string, (int)strlen (string), target, targetSize, targetLength);
}

bool OdbcObject::appendString(const char * string, int stringLength, SQLCHAR * target, int targetSize, SQLSMALLINT * targetLength)
{
	--targetSize;
    int length = *targetLength;
	*targetLength += stringLength;
	int l = targetSize - length;

	if (stringLength <= l)
		{
		memcpy (target + length, string, stringLength);
		target [length + stringLength] = 0;
		return false;
		}

	if (l > 0)
		memcpy (target, string, targetSize - length);

	target [targetSize] = 0;

	return true;
}

SQLRETURN OdbcObject::sqlError(UCHAR * stateBuffer, SQLINTEGER * nativeCode, UCHAR * msgBuffer, int msgBufferLength, SWORD * msgLength)
{
	if (!errors.empty())
	{
		auto error = std::move(errors.front());
		errors.erase(errors.begin());
		return error->sqlGetDiagRec (stateBuffer, nativeCode, msgBuffer, msgBufferLength, msgLength);
	}

	strcpy ((char*) stateBuffer, "00000");
	msgBuffer [0] = 0;
	*msgLength = 0;

	return SQL_NO_DATA_FOUND;
}

OdbcError* OdbcObject::postError(OdbcError * error)
{
	infoPosted = true;
	error->connection = getConnection();
	errors.push_back(std::unique_ptr<OdbcError>(error));

	return error;
}

void OdbcObject::operator <<(OdbcObject * obj)
{
	for (auto& error : obj->errors)
	{
		error->connection = getConnection();
		errors.push_back(std::move(error));
	}

	infoPosted = obj->infoPosted;
	sqlDiagCursorRowCount = obj->sqlDiagCursorRowCount;
	sqlDiagDynamicFunction = obj->sqlDiagDynamicFunction;
	sqlDiagDynamicFunctionCode = obj->sqlDiagDynamicFunctionCode;
	sqlDiagNumber = obj->sqlDiagNumber;
	sqlDiagReturnCode = obj->sqlDiagReturnCode;
	sqlDiagRowCount = obj->sqlDiagRowCount;

	obj->errors.clear();
	obj->clearErrors();
}

void OdbcObject::clearErrors()
{
	if (!infoPosted)
		return;

	errors.clear();

	infoPosted = false;
	sqlDiagDynamicFunction = NULL;		// SQL_DIAG_DYNAMIC_FUNCTION 
	sqlDiagDynamicFunctionCode = 0;		// SQL_DIAG_DYNAMIC_FUNCTION_CODE
	sqlDiagNumber = 0;					// SQL_DIAG_NUMBER 
	sqlDiagReturnCode = SQL_SUCCESS;	// SQL_DIAG_RETURNCODE
	sqlDiagRowCount = 0;				// SQL_DIAG_ROW_COUNT
}


OdbcError* OdbcObject::postError(const char * sqlState, SQLException &exception)
{
	return postError( new OdbcError( exception.getSqlcode(), exception.getFbcode(), sqlState, exception.getText() ) );
}

const char * OdbcObject::getString(char * * temp, const UCHAR * string, int length, const char *defaultValue)
{
	if (!string)
		return defaultValue;

	if (length == SQL_NTS)
		return (char*) string;

	char *ret = *temp;
	memcpy (ret, string, length);
	ret [length] = 0;
	*temp += length + 1;

	return ret;	
}

OdbcError* OdbcObject::postError(const char * state, JString msg)
{
	return postError (new OdbcError (0, state, msg));
}

SQLRETURN OdbcObject::sqlGetDiagRec(int handleType, int recNumber, SQLCHAR * stateBuffer, SQLINTEGER * nativeCode, SQLCHAR * msgBuffer, int msgBufferLength, SQLSMALLINT * msgLength)
{
	if (recNumber >= 1 && recNumber <= (int)errors.size())
		return errors[recNumber - 1]->sqlGetDiagRec (stateBuffer, nativeCode, msgBuffer, msgBufferLength, msgLength);

	if (stateBuffer)
		strcpy ((char*) stateBuffer, "00000");

	if (msgBuffer)
		msgBuffer [0] = 0;

    if ( msgLength )
        *msgLength = 0;

	return SQL_NO_DATA;
}

// Phase 12 (12.2.2): Direct UTF-16 diagnostic record output.
SQLRETURN OdbcObject::sqlGetDiagRecW(int handleType, int recNumber, SQLWCHAR * stateBuffer, SQLINTEGER * nativeCode, SQLWCHAR * msgBuffer, int msgBufferLength, SQLSMALLINT * msgLength)
{
	if (recNumber >= 1 && recNumber <= (int)errors.size())
		return errors[recNumber - 1]->sqlGetDiagRecW(stateBuffer, nativeCode, msgBuffer, msgBufferLength, msgLength);

	if (stateBuffer)
	{
		stateBuffer[0] = (SQLWCHAR)'0';
		stateBuffer[1] = (SQLWCHAR)'0';
		stateBuffer[2] = (SQLWCHAR)'0';
		stateBuffer[3] = (SQLWCHAR)'0';
		stateBuffer[4] = (SQLWCHAR)'0';
		stateBuffer[5] = (SQLWCHAR)0;
	}

	if (msgBuffer)
		msgBuffer[0] = (SQLWCHAR)0;

	if (msgLength)
		*msgLength = 0;

	return SQL_NO_DATA;
}

// Phase 12 (12.2.2): Direct UTF-16 diagnostic field output.
SQLRETURN OdbcObject::sqlGetDiagFieldW(int recNumber, int diagId, SQLPOINTER ptr, int bufferLength, SQLSMALLINT *stringLength)
{
	// Header fields (recNumber == 0) — numeric, no conversion needed
	switch( diagId )
	{
	case SQL_DIAG_CURSOR_ROW_COUNT:
		if (ptr)
			*(SQLINTEGER*)ptr = sqlDiagCursorRowCount;
		return SQL_SUCCESS;

	case SQL_DIAG_DYNAMIC_FUNCTION:
		if (ptr)
			*(SQLWCHAR*)ptr = (SQLWCHAR)0;
		return SQL_SUCCESS;

	case SQL_DIAG_DYNAMIC_FUNCTION_CODE:
		if (ptr)
			*(SQLINTEGER*)ptr = sqlDiagDynamicFunctionCode;
		return SQL_SUCCESS;

	case SQL_DIAG_NUMBER:
		if (ptr)
			*(SQLINTEGER*)ptr = (SQLINTEGER)errors.size();
		return SQL_SUCCESS;

	case SQL_DIAG_RETURNCODE:
		if (ptr)
			*(SQLRETURN*)ptr = sqlDiagReturnCode;
		return SQL_SUCCESS;

	case SQL_DIAG_ROW_COUNT:
		if (ptr)
			*(SQLLEN*)ptr = sqlDiagRowCount;
		return SQL_SUCCESS;
	}

	// Record-level fields (recNumber >= 1)
	if (ptr)
		*(SQLWCHAR*)ptr = (SQLWCHAR)0;

	if (recNumber >= 1 && recNumber <= (int)errors.size())
		return errors[recNumber - 1]->sqlGetDiagFieldW(diagId, ptr, bufferLength, stringLength);

	return SQL_NO_DATA;
}

SQLRETURN OdbcObject::sqlGetDiagField(int recNumber, int diagId, SQLPOINTER ptr, int bufferLength, SQLSMALLINT *stringLength)
{
	// Header fields (recNumber == 0)
	switch( diagId )
	{
	case SQL_DIAG_CURSOR_ROW_COUNT:
		if (ptr)
			*(SQLINTEGER*)ptr = sqlDiagCursorRowCount;
		return SQL_SUCCESS;

	case SQL_DIAG_DYNAMIC_FUNCTION:
		if (ptr)
			*(SQLCHAR *)ptr = 0; // sqlDiagDynamicFunction
		return SQL_SUCCESS;

	case SQL_DIAG_DYNAMIC_FUNCTION_CODE:
		if (ptr)
			*(SQLINTEGER*)ptr = sqlDiagDynamicFunctionCode;
		return SQL_SUCCESS;

	case SQL_DIAG_NUMBER:
		if (ptr)
		{
			*(SQLINTEGER*)ptr = (SQLINTEGER)errors.size();
		}
		return SQL_SUCCESS;

	case SQL_DIAG_RETURNCODE:
		if (ptr)
			*(SQLRETURN*)ptr = sqlDiagReturnCode;
		return SQL_SUCCESS;

	case SQL_DIAG_ROW_COUNT:
		if (ptr)
			*(SQLLEN*)ptr = sqlDiagRowCount;
		return SQL_SUCCESS;
	}

	// Record-level fields (recNumber >= 1)
	if (ptr)
		*(char*)ptr = '\0';

	if (recNumber >= 1 && recNumber <= (int)errors.size())
		return errors[recNumber - 1]->sqlGetDiagField (diagId, ptr, bufferLength, stringLength);

	return SQL_NO_DATA;
}

void OdbcObject::setCursorRowCount(int count)
{
	sqlDiagCursorRowCount = count;
}

}; // end namespace OdbcJdbcLibrary
