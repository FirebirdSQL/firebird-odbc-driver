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
 *
 *	Changes
 *
 *	2002-05-20	OdbcEnv.cpp
 *
 *				Contributed by Robert Milharcic
 *				o allocHandle() - Fix typo in assignment to connections
 *	
 *
 *
 */

// OdbcEnv.cpp: implementation of the OdbcEnv class.
//
//////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include "OdbcJdbc.h"
#include "OdbcEnv.h"
#include "OdbcConnection.h"
#include "IscDbc/SQLException.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

OdbcEnv::OdbcEnv()
{
	connections = NULL;

#ifndef _WIN32
	if (!(odbcIniFileName = getenv ("ODBCINI")))
#endif
		odbcIniFileName = "ODBC.INI";
}

OdbcEnv::~OdbcEnv()
{

}

OdbcObjectType OdbcEnv::getType()
{
	return odbcTypeEnv;
}

RETCODE OdbcEnv::allocHandle(int handleType, SQLHANDLE * outputHandle)
{
	clearErrors();
	*outputHandle = SQL_NULL_HDBC;

	if (handleType != SQL_HANDLE_DBC)
		return sqlReturn (SQL_ERROR, "HY000", "General Error");

	OdbcConnection *connection = new OdbcConnection (this);
	connection->next = connections;
//Orig.
//	connections = connections;
//From R. Milharcic
	connections = connection;
	*outputHandle = (SQLHANDLE)connection;

	return sqlSuccess();
}

RETCODE OdbcEnv::sqlEndTran(int operation)
{
	clearErrors();
	RETCODE ret = SQL_SUCCESS;

	for (OdbcConnection *connection = connections; connection;
		 connection = (OdbcConnection*) connection->next)
		{
		RETCODE retcode = connection->sqlEndTran (operation);
		if (retcode != SQL_SUCCESS)
			ret = retcode;
		}

	return ret;
}

void OdbcEnv::connectionClosed(OdbcConnection * connection)
{
	for (OdbcObject **ptr = (OdbcObject**) &connections; *ptr; ptr =&((*ptr)->next))
		if (*ptr == connection)
			{
			*ptr = connection->next;
			break;
			}
}

RETCODE OdbcEnv::sqlGetEnvAttr(int attribute, SQLPOINTER ptr, int bufferLength, SQLINTEGER *lengthPtr)
{
	clearErrors();
	long value;
	char *string = NULL;

	try
	{
		switch (attribute)
			{
			case SQL_ATTR_ODBC_VERSION:
				value = SQL_OV_ODBC3;
				break;

			case SQL_ATTR_OUTPUT_NTS:
				value = SQL_TRUE;
				break;

			default:
				return sqlReturn (SQL_ERROR, "HYC00", "Optional feature not implemented");
			}

		if (string)
			return returnStringInfo (ptr, bufferLength, lengthPtr, string);

		if (ptr)
			*(long*) ptr = value;

		if (lengthPtr)
			*lengthPtr = sizeof (long);
	}
	catch (SQLException& exception)
	{
		postError ("HY000", exception);
		return SQL_ERROR;
	}

	return sqlSuccess();
}

RETCODE OdbcEnv::sqlSetEnvAttr(int attribute, SQLPOINTER value, int length)
{
	clearErrors();

	try
	{
		switch (attribute)
		{
			case SQL_ATTR_OUTPUT_NTS:
			case SQL_ATTR_ODBC_VERSION:
				break;

			default:
				return sqlReturn (SQL_ERROR, "HYC00", "Optional feature not implemented");
		}
	}
	catch (SQLException& exception)
	{
		postError ("HY000", exception);
		return SQL_ERROR;
	}
			
	return sqlSuccess();
}
