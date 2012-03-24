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

// OdbcError.h: interface for the OdbcError class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ODBCERROR_H_)
#define _ODBCERROR_H_

#include "IscDbc/JString.h"

namespace OdbcJdbcLibrary {

using namespace classJString;

class OdbcConnection;

class OdbcError  
{
public:
	void setColumnNumber (int column, int row);
	void setRowNumber (int number);
	SQLRETURN sqlGetDiagField (int diagId, SQLPOINTER ptr, int bufferLength, SQLSMALLINT *stringLength);
	SQLRETURN sqlGetDiagRec (UCHAR *stateBuffer, SQLINTEGER *nativeCode, UCHAR *msgBuffer, int msgBufferLength, SWORD *msgLength);
	OdbcError(int code, const char *state, JString errorMsg);
	OdbcError(int code, int fbcode, const char *state, JString errorMsg);
	~OdbcError();

	OdbcConnection	*connection;
	OdbcError		*next;
	char			sqlState[6];
	JString			msg;
	int				nativeCode;
	int				rowNumber;
	int				columnNumber;
};

}; // end namespace OdbcJdbcLibrary

#endif // !defined(_ODBCERROR_H_)
