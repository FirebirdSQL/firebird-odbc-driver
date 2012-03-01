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
#include <odbcinst.h>
#ifndef _WINDOWS
#include <dlfcn.h>
#endif

namespace OdbcJdbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

OdbcEnv::OdbcEnv()
{
	envShare = NULL;
	connections = NULL;
	useAppOdbcVersion = SQL_OV_ODBC3;

#ifdef _WINDOWS
	activeDrv = NULL;
	endDrv = NULL;
	activeDSN = NULL;
	endDSN = NULL;
#endif

#ifndef _WINDOWS
	if (!(odbcIniFileName = getenv ("ODBCINI")))
#endif
		odbcIniFileName = "ODBC.INI";

#ifndef _WINDOWS
	if (!(odbcInctFileName = getenv ("ODBCINST")))
#endif
		odbcInctFileName = "ODBCINST.INI";
}

OdbcEnv::~OdbcEnv()
{

}

void OdbcEnv::LockEnv()
{
	mutex.lock();
}

void OdbcEnv::UnLockEnv()
{
	mutex.release();
}

OdbcConnection* OdbcEnv::getConnection()
{
	return NULL;
}

OdbcObjectType OdbcEnv::getType()
{
	return odbcTypeEnv;
}

SQLRETURN OdbcEnv::allocHandle(int handleType, SQLHANDLE * outputHandle)
{
	clearErrors();
	*outputHandle = SQL_NULL_HDBC;

	if (handleType != SQL_HANDLE_DBC)
		return sqlReturn (SQL_ERROR, "HY000", "General Error");

	OdbcConnection *connection = new OdbcConnection (this);
	connection->next = connections;

	connections = connection;
	*outputHandle = (SQLHANDLE)connection;

	return sqlSuccess();
}

SQLRETURN OdbcEnv::sqlEndTran(int operation)
{
	clearErrors();
	SQLRETURN ret = SQL_SUCCESS;

	if ( !envShare->getCountConnection() )
		for (OdbcConnection *connection = connections; connection;
			 connection = (OdbcConnection*) connection->next)
			{
			SQLRETURN retcode = connection->sqlEndTran (operation);
			if (retcode != SQL_SUCCESS)
				ret = retcode;
			}
	else
		try
		{
			envShare->sqlEndTran (operation);
		}
		catch ( std::exception &ex )
		{
			SQLException &exception = (SQLException&)ex;
			postError ("HY000", exception);
			return SQL_ERROR;
		}

	return ret;
}

void OdbcEnv::connectionClosed(OdbcConnection * connection)
{
	OdbcObject **ptr;

	for (ptr = (OdbcObject**) &connections; *ptr; ptr = &((*ptr)->next))
		if (*ptr == connection)
		{
			*ptr = connection->next;
			break;
		}

	if( !connections )
		envShare = NULL;
}

SQLRETURN OdbcEnv::sqlGetEnvAttr(int attribute, SQLPOINTER ptr, int bufferLength, SQLINTEGER *lengthPtr)
{
	clearErrors();
	int value;
	char *string = NULL;

	try
	{
		switch (attribute)
			{
			case SQL_ATTR_CONNECTION_POOLING:
				value = SQL_CP_OFF;
				break;

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
			*(int*) ptr = value;

		if (lengthPtr)
			*lengthPtr = sizeof (int);
	}
	catch ( std::exception &ex )
	{
		SQLException &exception = (SQLException&)ex;
		postError ("HY000", exception);
		return SQL_ERROR;
	}

	return sqlSuccess();
}

SQLRETURN OdbcEnv::sqlSetEnvAttr(int attribute, SQLPOINTER value, int length)
{
	clearErrors();

	try
	{
		switch (attribute)
		{
		case SQL_ATTR_CONNECTION_POOLING:
		case SQL_ATTR_OUTPUT_NTS:
			break;

		case SQL_ATTR_ODBC_VERSION:
			useAppOdbcVersion = (intptr_t)value;
			break;

		default:
			return sqlReturn (SQL_ERROR, "HYC00", "Optional feature not implemented");
		}
	}
	catch ( std::exception &ex )
	{
		SQLException &exception = (SQLException&)ex;
		postError ("HY000", exception);
		return SQL_ERROR;
	}
			
	return sqlSuccess();
}

SQLRETURN OdbcEnv::sqlDrivers(SQLUSMALLINT direction,
							SQLCHAR * serverName,
							SQLSMALLINT	bufferLength1,
							SQLSMALLINT * nameLength1Ptr,
							SQLCHAR * description,
							SQLSMALLINT bufferLength2,
							SQLSMALLINT * nameLength2Ptr )
{
#ifdef _WINDOWS
	switch( direction )
	{
	case SQL_FETCH_NEXT:
		if ( activeDrv == NULL )
			getDrivers ();
		else if ( endDrv && !*endDrv )
		{
			activeDrv = endDrv = listDrv;
			return SQL_NO_DATA;
		}
		else
			activeDrv = endDrv;
		break;

	case SQL_FETCH_FIRST:
		getDrivers ();
		break;

	default :
		return sqlReturn (SQL_ERROR, "HY103", "Invalid retrieval code");
	}
	
	if ( endDrv && *endDrv )
	{
		while( *endDrv )
			++endDrv;
		++endDrv;
	}

	if ( activeDrv && !*activeDrv )
	{
		activeDrv = NULL;
		return SQL_NO_DATA;
	}

	if ( serverName && bufferLength1)
	{
		int lenDrv = (int)strlen(activeDrv);
		int len = MIN(lenDrv, (int)MAX(0, (int)bufferLength1-1));
		 
		if ( len > 0 ) 
			memcpy (serverName, activeDrv, len);

		((char*) (serverName)) [len] = 0;
		
		if ( nameLength1Ptr )
			*nameLength1Ptr = len;

		if (len && len < lenDrv)
			postError ("01004", "String data, right truncated");
	}

	if ( description && bufferLength2)
	{
		int lenDes = (int)strlen(DRIVER_FULL_NAME);
		int len = MIN(lenDes, (int)MAX(0, (int)bufferLength2-1));
		 
		if ( len > 0 ) 
			memcpy (description, DRIVER_FULL_NAME, len);

		((char*) (description)) [len] = 0;

		if ( nameLength2Ptr )
			*nameLength2Ptr = len;

		if (len && len < lenDes)
			postError ("01004", "String data, right truncated");
	}

#endif

	return sqlSuccess();
}

SQLRETURN OdbcEnv::sqlDataSources(SQLUSMALLINT direction,
								SQLCHAR * serverName,
								SQLSMALLINT	bufferLength1,
								SQLSMALLINT * nameLength1Ptr,
								SQLCHAR * description,
								SQLSMALLINT bufferLength2,
								SQLSMALLINT * nameLength2Ptr )
{
#ifdef _WINDOWS
	switch( direction )
	{
	case SQL_FETCH_NEXT:
		if ( activeDSN == NULL )
			getDataSources ( ODBC_BOTH_DSN );
		else if ( endDSN && !*endDSN )
		{
			activeDSN = endDSN = listDSN;
			return SQL_NO_DATA;
		}
		else
			activeDSN = endDSN;
		break;

	case SQL_FETCH_FIRST:
		getDataSources ( ODBC_BOTH_DSN );
		break;

	case SQL_FETCH_FIRST_USER:
		getDataSources ( ODBC_USER_DSN );
		break;

	case SQL_FETCH_FIRST_SYSTEM:
		getDataSources ( ODBC_SYSTEM_DSN );
		break;

	default :
		return sqlReturn (SQL_ERROR, "HY103", "Invalid retrieval code");
	}
	
	if ( endDSN && *endDSN )
	{
		while( *endDSN )
			++endDSN;
		++endDSN;
	}

	if ( activeDSN && !*activeDSN )
	{
		activeDSN = NULL;
		return SQL_NO_DATA;
	}

	if ( serverName && bufferLength1)
	{
		int lenDSN = (int)strlen(activeDSN);
		int len = MIN(lenDSN, (int)MAX(0, (int)bufferLength1-1));
		 
		if ( len > 0 ) 
			memcpy (serverName, activeDSN, len);

		((char*) (serverName)) [len] = 0;
		
		if ( nameLength1Ptr )
			*nameLength1Ptr = len;

		if (len && len < lenDSN)
			postError ("01004", "String data, right truncated");
	}

	if ( description && bufferLength2)
	{
		int lenDes = (int)strlen(DRIVER_FULL_NAME);
		int len = MIN(lenDes, (int)MAX(0, (int)bufferLength2-1));
		 
		if ( len > 0 ) 
			memcpy (description, DRIVER_FULL_NAME, len);

		((char*) (description)) [len] = 0;

		if ( nameLength2Ptr )
			*nameLength2Ptr = len;

		if (len && len < lenDes)
			postError ("01004", "String data, right truncated");
	}

#endif

	return sqlSuccess();
}

#ifdef _WINDOWS
BOOL OdbcEnv::getDrivers()
{
	const char	* odbcDrivers = "ODBC Drivers";
	char * ptStr, * ptStrEnd, * ptStrSave;
	char bufferDrv[SQL_MAX_DSN_LENGTH + 1];
	int lenName = (int)strlen(DRIVER_FULL_NAME);
	int n=0, nRead, nLen;

	clearErrors();

    nRead = SQLGetPrivateProfileString(odbcDrivers, NULL, "", listDrv, sizeof(listDrv),odbcInctFileName);
	ptStrSave = ptStrEnd = ptStr = listDrv;

	while( nRead > 0 && *ptStrEnd )
	{
		while( *ptStrEnd )
			++ptStrEnd;
		++ptStrEnd;
		nLen = ptStrEnd - ptStr; 
		nRead -= nLen;

		n = SQLGetPrivateProfileString(ptStr, "FileExtns", "", bufferDrv, sizeof(bufferDrv),odbcInctFileName);

		if ( strstr(bufferDrv, "*.fdb") || strstr(bufferDrv, "*.gdb") )
		{
			memcpy(ptStrSave,ptStr,nLen);
			ptStrSave += nLen;
		}

		ptStr = ptStrEnd;
	}
	
	*ptStrSave = '\0';
	activeDrv = endDrv = listDrv;

	return TRUE;
}

bool OdbcEnv::getDataSources( SQLUSMALLINT wConfigMode )
{
	const char	* odbcDataSources = "ODBC Data Sources";
	char * ptStr, * ptStrEnd, * ptStrSave;
	char bufferDSN[SQL_MAX_DSN_LENGTH + 1];
	int lenName = (int)strlen(DRIVER_FULL_NAME);
	int n=0, nRead, nLen;

	clearErrors();

	SQLSetConfigMode( wConfigMode );

    nRead = SQLGetPrivateProfileString(odbcDataSources, NULL, "", listDSN, sizeof(listDSN),odbcIniFileName);
	ptStrSave = ptStrEnd = ptStr = listDSN;

	while( nRead > 0 && *ptStrEnd )
	{
		while( *ptStrEnd )
			++ptStrEnd;
		++ptStrEnd;
		nLen = ptStrEnd - ptStr; 
		nRead -= nLen;

		n = SQLGetPrivateProfileString(odbcDataSources, ptStr, "", bufferDSN, sizeof(bufferDSN),odbcIniFileName);

		if ( !memcmp(bufferDSN, DRIVER_FULL_NAME, lenName) )
		{
			memcpy(ptStrSave,ptStr,nLen);
			ptStrSave += nLen;
		}

		ptStr = ptStrEnd;
	}
	
	*ptStrSave = '\0';
	activeDSN = endDSN = listDSN;

	return TRUE;
}
#endif

}; // end namespace OdbcJdbcLibrary
