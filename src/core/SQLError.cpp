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

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "IscDbc.h"
#include "SQLError.h"
#include "Stream.h"
#include <fb-cpp/Exception.h>

#ifdef _WINDOWS
#define vsnprintf	_vsnprintf
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

namespace IscDbcLibrary {



SQLError SQLError::fromDatabaseException(const fbcpp::DatabaseException& ex)
{
	return fromDatabaseException(RUNTIME_ERROR, ex);
}

SQLError SQLError::fromDatabaseException(SqlCode fallbackCode, const fbcpp::DatabaseException& ex)
{
	auto errorCode = static_cast<__int64>(ex.getErrorCode());
	return SQLError(static_cast<int>(fallbackCode), errorCode, ex.what());
}

SQLError::SQLError (SqlCode code, const char *txt, ...)
{
/**************************************
 *
 *		S Q L E x c e p t i o n
 *
 **************************************
 *
 * Functional description
 *		SQL exception -- quite generic.
 *
 **************************************/
	va_list		args;
	va_start	(args, txt);
	char		temp [1024];

	stackTrace.clear();
	useCount = 0;

	if (vsnprintf (temp, sizeof (temp) - 1, txt, args) < 0)
		temp [sizeof (temp) - 1] = 0;

	text = temp;
	sqlcode = (int) code;
	fbcode = 0;
}

SQLError::SQLError(Stream * trace, SqlCode code, const char * txt, ...)
{
/**************************************
 *
 *		S Q L E x c e p t i o n
 *
 **************************************
 *
 * Functional description
 *		SQL exception -- quite generic.
 *
 **************************************/
	va_list		args;
	va_start	(args, txt);
	char		temp [1024];

	useCount = 0;
	int length = trace->getLength();
	stackTrace.resize(length);
	trace->getSegment (0, length, stackTrace.data());

	if (vsnprintf (temp, sizeof (temp) - 1, txt, args) < 0)
		temp [sizeof (temp) - 1] = 0;

	text = temp;
	sqlcode = (int) code;
	fbcode = 0;
}

SQLError::~SQLError () throw()
{
/**************************************
 *
 *		~ S Q L E x c e p t i o n
 *
 **************************************
 *
 * Functional description
 *		Object destructor.
 *
 **************************************/

}

int SQLError::getSqlcode ()
{
/**************************************
 *
 *		g e t S q l c o d e
 *
 **************************************
 *
 * Functional description
 *		Get standard sql code.
 *
 **************************************/

return sqlcode;
}

int SQLError::getFbcode ()
{
/**************************************
 *
 *		g e t F b c o d e
 *
 **************************************
 *
 * Functional description
 *		Get server error code.
 *
 **************************************/

return fbcode;
}

const char *SQLError::getText ()
{
/**************************************
 *
 *		g e t T e x t
 *
 **************************************
 *
 * Functional description
 *		Get text of exception.
 *
 **************************************/

return text.c_str();
}

SQLError::operator const char* ()
{
/**************************************
 *
 *		o p e r a t o r   c h a r *
 *
 **************************************
 *
 * Functional description
 *		Return string as string.
 *
 **************************************/

return getText();
}

SQLError::SQLError( int code, __int64 codefb, const char * txt, ...)
{
	va_list		args;
	va_start	(args, txt);
	char		temp [1024];

	useCount = 0;
	stackTrace.clear();

	if (vsnprintf (temp, sizeof (temp) - 1, txt, args) < 0)
		temp [sizeof (temp) - 1] = 0;

	text = temp;
	sqlcode = (int) code;
	fbcode = (int) codefb;
}

const char* SQLError::getTrace()
{
	return stackTrace.c_str();
}

void SQLError::addRef()
{
	++useCount;
}

int SQLError::release()
{
	//ASSERT (useCount > 0);

	if (--useCount == 0)
		{
		delete this;
		return 0;
		}

	return useCount;
}

}; // end namespace IscDbcLibrary
