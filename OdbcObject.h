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

// OdbcObject.h: interface for the OdbcObject class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ODBCOBJECT_H_)
#define _ODBCOBJECT_H_

#if defined(__GNUC__)
#include <inttypes.h>
#endif

#include "OdbcJdbc.h"
#include "IscDbc/JString.h"
#include "IscDbc/SQLException.h"

namespace OdbcJdbcLibrary {

using namespace classJString;
using namespace IscDbcLibrary;

enum OdbcObjectType {
    odbcTypeEnv,
	odbcTypeConnection,
	odbcTypeStatement,
	odbcTypeDescriptor
	};

class OdbcError;
class OdbcConnection;

class OdbcObject  
{
public:
	void setCursorRowCount (int count);
	virtual SQLRETURN sqlGetDiagField (int recNumber, int diagId, SQLPOINTER ptr, int bufferLength, SQLSMALLINT *stringLength);
	SQLRETURN returnStringInfo(SQLPOINTER ptr, SQLSMALLINT maxLength, SQLINTEGER* returnLength, const char * value);
	SQLRETURN sqlGetDiagRec (int handleType, int recNumber, SQLCHAR*sqlState,SQLINTEGER*nativeErrorPtr,SQLCHAR*messageText,int bufferLength,SQLSMALLINT*textLengthPtr);
	OdbcError* postError (const char *state, JString msg);
	const char * getString (char **temp, const UCHAR *string, int length, const char *defaultValue);
	OdbcError* postError (const char *sqlState, SQLException &exception);
	void operator <<(OdbcObject * obj);
	void clearErrors();
	OdbcError* postError (OdbcError *error);
	virtual SQLRETURN sqlError (UCHAR *stateBuffer, SQLINTEGER *nativeCode, UCHAR *msgBuffer, int msgBufferLength, SWORD *msgLength);
	bool appendString(const char * string, int stringLength, SQLCHAR * target, int targetSize, SQLSMALLINT * targetLength);
	bool setString(const char * string, SQLCHAR * target, int targetSize, SQLSMALLINT * targetLength);
	bool setString (const SQLCHAR *string, int stringLength, SQLCHAR *target, int targetSize, SQLSMALLINT *targetLength);
	int stringLength (const SQLCHAR *string, int givenLength);
	void notYetImplemented (const char *msg);
	virtual SQLRETURN allocHandle (int handleType, SQLHANDLE *outputHandle);
	int sqlSuccess();
	virtual OdbcConnection* getConnection() = 0;
	virtual OdbcObjectType getType() = 0;
	int sqlReturn (int code, const char *state, const char *text, int nativeCode = 0);
	SQLRETURN returnStringInfo (SQLPOINTER ptr, SQLSMALLINT maxLength, SQLSMALLINT* returnLength, const char * value);
	OdbcObject();
	~OdbcObject();

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

}; // end namespace OdbcJdbcLibrary

#endif // !defined(_ODBCOBJECT_H_)
