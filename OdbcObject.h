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

// OdbcObject.h: interface for the OdbcObject class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ODBCOBJECT_H__ED260D94_1BC4_11D4_98DF_0000C01D2301__INCLUDED_)
#define AFX_ODBCOBJECT_H__ED260D94_1BC4_11D4_98DF_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "OdbcJdbc.h"
#include "JString.h"

enum OdbcObjectType {
    odbcTypeEnv,
	odbcTypeConnection,
	odbcTypeStatement,
	odbcTypeDescriptor,
	};

class OdbcError;
class SQLException;

class OdbcObject  
{
public:
	void setCursorRowCount (int count);
	int getCType (int type, bool isSigned);
	virtual RETCODE sqlGetDiagField (int recNumber, int diagId, SQLPOINTER ptr, int bufferLength, SQLSMALLINT *stringLength);
	RETCODE returnStringInfo(SQLPOINTER ptr, SQLSMALLINT maxLength, SQLINTEGER* returnLength, const char * value);
	RETCODE sqlGetDiagRec (int handleType, int recNumber, SQLCHAR*sqlState,SQLINTEGER*nativeErrorPtr,SQLCHAR*messageText,int bufferLength,SQLSMALLINT*textLengthPtr);
	OdbcError* postError (const char *state, JString msg);
	const char * getString (char **temp, const UCHAR *string, int length, const char *defaultValue);
	OdbcError* postError (const char *sqlState, SQLException& exception);
	void clearErrors();
	OdbcError* postError (OdbcError *error);
	virtual RETCODE sqlError (UCHAR *stateBuffer, SDWORD *nativeCode, UCHAR *msgBuffer, int msgBufferLength, SWORD *msgLength);
	bool appendString(const char * string, int stringLength, SQLCHAR * target, int targetSize, SQLSMALLINT * targetLength);
	bool setString(const char * string, SQLCHAR * target, int targetSize, SQLSMALLINT * targetLength);
	bool setString (const SQLCHAR *string, int stringLength, SQLCHAR *target, int targetSize, SQLSMALLINT *targetLength);
	int stringLength (const SQLCHAR *string, int givenLength);
	void notYetImplemented (const char *msg);
	virtual RETCODE allocHandle (int handleType, SQLHANDLE *outputHandle);
	int sqlSuccess();
	virtual OdbcObjectType getType() = 0;
	int sqlReturn (int code, const char *state, const char *text, int nativeCode = 0);
	RETCODE returnStringInfo (SQLPOINTER ptr, SQLSMALLINT maxLength, SQLSMALLINT* returnLength, const char * value);
	OdbcObject();
	virtual ~OdbcObject();

	OdbcError	*errors;
	bool		infoPosted;
	OdbcObject	*next;
	// Header Fields
	SQLINTEGER	sqlDiagCursorRowCount;			// SQL_DIAG_CURSOR_ROW_COUNT 
	SQLCHAR *	sqlDiagDynamicFunction;			// SQL_DIAG_DYNAMIC_FUNCTION 
	SQLINTEGER	sqlDiagDynamicFunctionCode;		// SQL_DIAG_DYNAMIC_FUNCTION_CODE
	SQLINTEGER	sqlDiagNumber;					// SQL_DIAG_NUMBER 
	SQLRETURN	sqlDiagReturnCode;				// SQL_DIAG_RETURNCODE
	SQLINTEGER	sqlDiagRowCount;				// SQL_DIAG_ROW_COUNT
};

#endif // !defined(AFX_ODBCOBJECT_H__ED260D94_1BC4_11D4_98DF_0000C01D2301__INCLUDED_)
