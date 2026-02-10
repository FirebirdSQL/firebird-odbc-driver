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

#include <memory>
#include <vector>
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

/// @brief Base class for all ODBC handle objects (Environment, Connection, Statement, Descriptor).
///
/// Provides common error handling (diagnostic records), string utility methods,
/// and the ODBC diagnostic header fields (SQL_DIAG_*). Every ODBC handle type
/// inherits from this class.
///
/// Error lifecycle:
///   1. clearErrors() at the start of each ODBC API call
///   2. postError() to record diagnostic records during execution
///   3. sqlGetDiagRec() / sqlGetDiagField() / sqlError() to retrieve them
///
/// Thread safety: Callers must hold the appropriate GUARD_* lock before
/// invoking any method on this object.
class OdbcObject  
{
public:
	/// Set the cursor row count for scrollable cursors (SQL_DIAG_CURSOR_ROW_COUNT).
	void setCursorRowCount (int count);
	/// Retrieve a diagnostic field value (implements SQLGetDiagField).
	virtual SQLRETURN sqlGetDiagField (int recNumber, int diagId, SQLPOINTER ptr, int bufferLength, SQLSMALLINT *stringLength);
	/// Copy a string value to a caller-supplied buffer, setting the returned length (SQLINTEGER* variant).
	SQLRETURN returnStringInfo(SQLPOINTER ptr, SQLSMALLINT maxLength, SQLINTEGER* returnLength, const char * value);
	/// Retrieve a diagnostic record (implements SQLGetDiagRec).
	SQLRETURN sqlGetDiagRec (int handleType, int recNumber, SQLCHAR*sqlState,SQLINTEGER*nativeErrorPtr,SQLCHAR*messageText,int bufferLength,SQLSMALLINT*textLengthPtr);

	// Phase 12 (12.2.2): Direct UTF-16 output — eliminates ConvertingString roundtrip.
	SQLRETURN sqlGetDiagRecW (int handleType, int recNumber, SQLWCHAR *sqlState, SQLINTEGER *nativeErrorPtr, SQLWCHAR *messageText, int bufferLength, SQLSMALLINT *textLengthPtr);
	virtual SQLRETURN sqlGetDiagFieldW (int recNumber, int diagId, SQLPOINTER ptr, int bufferLength, SQLSMALLINT *stringLength);
	/// Post an error with a given SQLSTATE and message string.
	OdbcError* postError (const char *state, JString msg);
	const char * getString (char **temp, const UCHAR *string, int length, const char *defaultValue);
	/// Post an error from a caught SQLException, mapping to the given SQLSTATE.
	OdbcError* postError (const char *sqlState, SQLException &exception);
	/// Transfer all error records from another OdbcObject into this one.
	void operator <<(OdbcObject * obj);
	/// Clear all diagnostic records and reset the info-posted flag.
	void clearErrors();
	/// Post a pre-constructed OdbcError into the diagnostic record list.
	OdbcError* postError (OdbcError *error);
	/// Retrieve the next error record (implements legacy SQLError).
	virtual SQLRETURN sqlError (UCHAR *stateBuffer, SQLINTEGER *nativeCode, UCHAR *msgBuffer, int msgBufferLength, SWORD *msgLength);
	bool appendString(const char * string, int stringLength, SQLCHAR * target, int targetSize, SQLSMALLINT * targetLength);
	/// Copy a C string to a caller-supplied buffer, truncating if necessary.
	bool setString(const char * string, SQLCHAR * target, int targetSize, SQLSMALLINT * targetLength);
	/// Copy a sized string to a caller-supplied buffer, truncating if necessary.
	bool setString (const SQLCHAR *string, int stringLength, SQLCHAR *target, int targetSize, SQLSMALLINT *targetLength);
	/// Compute the length of an ODBC string (handles SQL_NTS).
	int stringLength (const SQLCHAR *string, int givenLength);
	void notYetImplemented (const char *msg);
	/// Allocate a child handle (implements SQLAllocHandle).
	virtual SQLRETURN allocHandle (int handleType, SQLHANDLE *outputHandle);
	/// Return SQL_SUCCESS or SQL_SUCCESS_WITH_INFO based on whether info was posted.
	int sqlSuccess();
	/// Return the OdbcConnection that owns this object (NULL for environments).
	virtual OdbcConnection* getConnection() = 0;
	/// Return the handle type (env, connection, statement, descriptor).
	virtual OdbcObjectType getType() = 0;
	/// Record an error and return the given SQLRETURN code.
	int sqlReturn (int code, const char *state, const char *text, int nativeCode = 0);
	/// Copy a string value to a caller-supplied buffer, setting the returned length (SQLSMALLINT* variant).
	SQLRETURN returnStringInfo (SQLPOINTER ptr, SQLSMALLINT maxLength, SQLSMALLINT* returnLength, const char * value);
	OdbcObject();
	~OdbcObject();

	OdbcObject	*next;

protected:
	/// Error list managed via postError()/clearErrors().
	std::vector<std::unique_ptr<OdbcError>>	errors;
	bool		infoPosted;
	/// Row count for scrollable cursors (used heavily by OdbcStatement).
	SQLINTEGER	sqlDiagCursorRowCount;			// SQL_DIAG_CURSOR_ROW_COUNT

	/// Set the SQL_DIAG_ROW_COUNT diagnostic header field.
	/// Called by OdbcStatement after DML execution.
	void setDiagRowCount(SQLINTEGER count) { sqlDiagRowCount = count; }

private:
	// Diagnostic header fields — only accessed within OdbcObject methods.
	SQLCHAR *	sqlDiagDynamicFunction;			// SQL_DIAG_DYNAMIC_FUNCTION
	SQLINTEGER	sqlDiagDynamicFunctionCode;		// SQL_DIAG_DYNAMIC_FUNCTION_CODE
	SQLINTEGER	sqlDiagNumber;					// SQL_DIAG_NUMBER
	SQLRETURN	sqlDiagReturnCode;				// SQL_DIAG_RETURNCODE
	SQLINTEGER	sqlDiagRowCount;				// SQL_DIAG_ROW_COUNT
};

}; // end namespace OdbcJdbcLibrary

#endif // !defined(_ODBCOBJECT_H_)
