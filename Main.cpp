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


#ifdef _WIN32
#include <windows.h>
#endif

#include <odbcinst.h>

extern "C"
{
#include <sql.h>
#include <sqlext.h>
}

#include <stdio.h>
#include "OdbcEnv.h"
#include "OdbcConnection.h"
#include "OdbcStatement.h"

//#define DEBUG

#ifdef DEBUG
#define TRACE(msg)		trace (msg)
#else
#define TRACE(msg)
#endif

//#define LOGGING
#ifdef LOGGING
FILE	*logFile = NULL;
void logMsg (const char *msg)
{
	if (!logFile)
		logFile = fopen (LOG_FILE, "w");
	if (!logFile)
		OutputDebugString ("log file create failed\n");
	else
	{
		fputs (msg, logFile);
		OutputDebugString (msg);
	}
}
#endif
		
void notYetImplemented (const char *msg)
{
    LOG_MSG (msg);
}

void trace (const char *msg)
{
    LOG_MSG(msg);
    LOG_MSG("\n");	
}

// __SQLAllocHandle
// Local variant of call to defeat the dynamic link
// mechanism.

static RETCODE SQL_API __SQLAllocHandle  (SQLSMALLINT arg0,
		 SQLHANDLE arg1,
		 SQLHANDLE * arg2)
{
	TRACE ("__SQLAllocHandle");

	if (arg0 == SQL_HANDLE_ENV)
		{
		if (arg1 != SQL_NULL_HANDLE || arg2 == NULL)
			return SQL_ERROR;
		*arg2 = (SQLHANDLE)new OdbcEnv;
		return SQL_SUCCESS;
		}

	OdbcObject *object = (OdbcObject*) arg1;

	if (arg2 == NULL)
		return object->sqlReturn (SQL_ERROR, "HY009", "Invalid use of null pointer");

	return object->allocHandle (arg0, arg2);
}

///// SQLAllocConnect /////

RETCODE SQL_API SQLAllocConnect  (HENV arg0,
			 HDBC * arg1)
{
	TRACE ("SQLAllocConnect");

	return __SQLAllocHandle (SQL_HANDLE_DBC, arg0, arg1);
}

///// SQLAllocEnv /////

RETCODE SQL_API SQLAllocEnv  (HENV * arg0)
{
	TRACE ("SQLAllocEnv");

	return __SQLAllocHandle (SQL_HANDLE_ENV, SQL_NULL_HANDLE, arg0);
}

///// SQLAllocStmt /////

RETCODE SQL_API SQLAllocStmt  (HDBC arg0,
		 HSTMT * arg1)
{
	TRACE ("SQLAllocStmt");

	return __SQLAllocHandle (SQL_HANDLE_STMT, arg0, arg1);
}

///// SQLBindCol /////

RETCODE SQL_API SQLBindCol  (HSTMT arg0,
			UWORD arg1,
			SWORD arg2,
			PTR arg3,
			SDWORD arg4,
			SDWORD * arg5)
{
	TRACE ("SQLBindCol");

	return ((OdbcStatement*) arg0)->sqlBindCol (arg1, arg2, arg3, arg4, arg5);
}

///// SQLCancel /////

RETCODE SQL_API SQLCancel  (HSTMT arg0)
{
	TRACE ("SQLCancel");

	return ((OdbcStatement*) arg0)->sqlCancel ();
}

///// SQLColAttributes /////

RETCODE SQL_API SQLColAttributes  (HSTMT arg0,
		 UWORD arg1,
		 UWORD arg2,
		 PTR arg3,
		 SWORD arg4,
		 SWORD * arg5,
		 SDWORD * arg6)
{
	TRACE("SQLColAttributes");

	return ((OdbcStatement*) arg0)->sqlColAttributes (arg1, arg2, arg3, arg4, arg5, arg6);
}

///// SQLConnect /////

RETCODE SQL_API SQLConnect  (HDBC arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UCHAR * arg3,
		 SWORD arg4,
		 UCHAR * arg5,
		 SWORD arg6)
{
	unsigned char * role, r[] = "";
	role = r;

	TRACE ("SQLConnect");

	return ((OdbcConnection*) arg0)->sqlConnect (arg1, arg2, arg3, arg4, arg5, arg6, role, 0);
}

///// SQLDescribeCol /////

RETCODE SQL_API SQLDescribeCol  (HSTMT arg0,
		 UWORD arg1,
		 UCHAR * arg2,
		 SWORD arg3,
		 SWORD * arg4,
		 SWORD * arg5,
		 UDWORD * arg6,
		 SWORD * arg7,
		 SWORD * arg8)
{
	TRACE ("SQLDescribeCol");

	return ((OdbcStatement*) arg0)->sqlDescribeCol (arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8);
}

///// SQLDisconnect /////

RETCODE SQL_API SQLDisconnect  (HDBC arg0)
{
	TRACE ("SQLDisconnect");
#ifdef LOGGING
	fclose (logFile);
#endif

	return ((OdbcConnection*) arg0)->sqlDisconnect();
}

///// SQLError /////

RETCODE SQL_API SQLError  (OdbcEnv *env,
		 OdbcConnection *connection,
		 OdbcStatement *statement,
		 UCHAR * sqlState,
		 SDWORD * nativeErrorCode,
		 UCHAR * msgBuffer,
		 SWORD msgBufferLength,
		 SWORD *msgLength)
{
	TRACE("SQLError");

	if (statement)
		return statement->sqlError (sqlState, nativeErrorCode, msgBuffer, msgBufferLength, msgLength);

	if (connection)
		return connection->sqlError (sqlState, nativeErrorCode, msgBuffer, msgBufferLength, msgLength);

	if (env)
		return env->sqlError (sqlState, nativeErrorCode, msgBuffer, msgBufferLength, msgLength);

	return SQL_ERROR;
}

///// SQLExecDirect /////

RETCODE SQL_API SQLExecDirect  (HSTMT arg0,
		 UCHAR * arg1,
		 SDWORD arg2)
{
	TRACE ("SQLExecDirect");

	return ((OdbcStatement*) arg0)->sqlExecuteDirect(arg1, arg2);
}

///// SQLExecute /////

RETCODE SQL_API SQLExecute  (HSTMT arg0)
{
	TRACE("SQLExecute");

	return ((OdbcStatement*) arg0)->sqlExecute();
}

///// SQLFetch /////

RETCODE SQL_API SQLFetch  (HSTMT arg0)
{
	TRACE ("SQLFetch");
	
	return ((OdbcStatement*) arg0)->sqlFetch();
}

///// SQLFreeConnect /////

RETCODE SQL_API SQLFreeConnect  (HDBC arg0)
{
	TRACE ("SQLFreeconnect");

	return SQLFreeHandle (SQL_HANDLE_DBC, arg0);
}

///// SQLFreeEnv /////

RETCODE SQL_API SQLFreeEnv  (HENV arg0)
{
	TRACE ("SQLFreeEnv");

	return SQLFreeHandle (SQL_HANDLE_ENV, arg0);
}

///// SQLFreeStmt /////

RETCODE SQL_API SQLFreeStmt  (HSTMT arg0,
		 UWORD arg1)
{
	TRACE ("SQLFreeStmt");

	if (arg1 == SQL_DROP)
		return SQLFreeHandle (SQL_HANDLE_STMT, arg0);

	return ((OdbcStatement*) arg0)->sqlFreeStmt (arg1);
}

///// SQLGetCursorName /////

RETCODE SQL_API SQLGetCursorName  (HSTMT arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 SWORD * arg3)
{
	TRACE ("SQLGetCursorName called\n");

	return ((OdbcStatement*) arg0)->sqlGetCursorName (arg1, arg2, arg3);
}

///// SQLNumResultCols /////

RETCODE SQL_API SQLNumResultCols  (HSTMT arg0, SWORD * arg1)
{
	TRACE ("SQLNumResultCols");

	return ((OdbcStatement*) arg0)->sqlNumResultCols (arg1);
}

///// SQLPrepare /////

RETCODE SQL_API SQLPrepare  (HSTMT arg0,
		 UCHAR * arg1,
		 SDWORD arg2)
{
	TRACE ("SQLPrepare");
	
	return ((OdbcStatement*) arg0)->sqlPrepare (arg1, arg2);
}

///// SQLRowCount /////

RETCODE SQL_API SQLRowCount  (HSTMT arg0, SDWORD * arg1)
{
	TRACE ("SQLRowCount");

	return ((OdbcStatement*) arg0)->sqlRowCount (arg1);
}

///// SQLSetCursorName /////

RETCODE SQL_API SQLSetCursorName  (HSTMT arg0,
		 UCHAR * arg1,
		 SWORD arg2)
{
	TRACE ("SQLSetCursorName");

	return ((OdbcStatement*) arg0)->sqlSetCursorName (arg1, arg2);
}

///// SQLSetParam ///// Deprecated in 2.0

RETCODE SQL_API SQLSetParam  (HSTMT arg0,
		 UWORD arg1,
		 SWORD arg2,
		 SWORD arg3,
		 UDWORD arg4,
		 SWORD arg5,
		 PTR arg6,
		 SDWORD * arg7)
{
	TRACE ("SQLSetParam");
	return ((OdbcStatement*) arg0)->sqlSetParam (arg1, arg2, arg3, arg4, arg5, arg6, arg7);
}

///// SQLTransact /////

RETCODE SQL_API SQLTransact  (HENV arg0,
		 HDBC arg1,
		 UWORD arg2)
{
	TRACE ("SQLTransact");

	if (arg0 == SQL_NULL_HDBC)
		return SQLEndTran (SQL_HANDLE_DBC, arg1, arg2);

	return SQLEndTran (SQL_HANDLE_ENV, arg0, arg2);
}

///// SQLColumns /////

RETCODE SQL_API SQLColumns  (HSTMT arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UCHAR * arg3,
		 SWORD arg4,
		 UCHAR * arg5,
		 SWORD arg6,
		 UCHAR * arg7,
		 SWORD arg8)
{
	TRACE ("SQLColumns");
	return ((OdbcStatement*) arg0)->sqlColumns (arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8);
}

///// SQLDriverConnect /////

RETCODE SQL_API SQLDriverConnect  (HDBC arg0,
		 HWND hWnd,
		 UCHAR * szConnStrIn,
		 SWORD cbConnStrIn,
		 UCHAR * szConnStrOut,
		 SWORD cbConnStrOut,
		 SQLSMALLINT *pcbConnStrOut,
		 UWORD uwMode)
{
	TRACE ("SQLDriverConnect");

	return ((OdbcConnection*) arg0)->sqlDriverConnect (
				hWnd, szConnStrIn, cbConnStrIn,
				szConnStrOut, cbConnStrOut, pcbConnStrOut,
				uwMode);
	/***
	// This really doesn't show nearly all that you need to know
	// about driver connect, read the programmer's reference

	notYetImplemented("SQLDriverConnect called\n");

	if ((cbConnStrIn == SQL_NTS) && (szConnStrIn))
		cbConnStrIn = strlen((char*) szConnStrIn);

	MessageBox(hWnd,
		   "Connection dialog would go here",
		   "Sample driver",
		   MB_OK);

	if ((szConnStrOut) && cbConnStrOut > 0)
	{
		strncpy((char*) szConnStrOut,
		        (char*) szConnStrIn,
			(cbConnStrIn == SQL_NTS) ? cbConnStrOut - 1 : 
						min(cbConnStrOut,cbConnStrIn));

		szConnStrOut[cbConnStrOut - 1] = '\0';
	}

	if (pcbConnStrOut)
		*pcbConnStrOut = cbConnStrIn;

	return(SQL_SUCCESS);
	***/
}

///// SQLGetConnectOption /////  Level 1

RETCODE SQL_API SQLGetConnectOption  (HDBC arg0,
		 UWORD arg1,
		 PTR arg2)
{
	notYetImplemented("SQLGetConnectOption called\n");
	return(SQL_SUCCESS);
}

///// SQLGetData /////

RETCODE SQL_API SQLGetData  (HSTMT arg0,
		 UWORD arg1,
		 SWORD arg2,
		 PTR arg3,
		 SDWORD arg4,
		 SDWORD * arg5)
{
	TRACE ("SQLGetData");

	return ((OdbcStatement*) arg0)->sqlGetData (arg1, arg2, arg3, arg4, arg5);
}

///// SQLGetFunctions /////

RETCODE SQL_API SQLGetFunctions  (HDBC arg0,
		 SQLUSMALLINT arg1,
		 SQLUSMALLINT  *arg2)
{
	TRACE ("SQLGetFunctions");

	return ((OdbcConnection*) arg0)->sqlGetFunctions (arg1, arg2);
}

///// SQLGetInfo /////

RETCODE SQL_API SQLGetInfo  (HDBC arg0,
		 UWORD arg1,
		 PTR arg2,
		 SWORD arg3,
		 SWORD * arg4)
{
	TRACE ("SQLGetInfo");

	return ((OdbcConnection*) arg0)->sqlGetInfo (arg1, arg2, arg3, arg4);
}

///// SQLGetStmtOption /////  Level 1

RETCODE SQL_API SQLGetStmtOption  (HSTMT arg0,
		 UWORD arg1,
		 PTR arg2)
{
	notYetImplemented("SQLGetStmtOption called\n");
	return(SQL_SUCCESS);
}

///// SQLGetTypeInfo /////

RETCODE SQL_API SQLGetTypeInfo  (HSTMT arg0,
		 SWORD arg1)
{
	TRACE ("SQLGetTypeInfo");

	return ((OdbcStatement*) arg0)->sqlGetTypeInfo (arg1);
}

///// SQLParamData /////

RETCODE SQL_API SQLParamData  (HSTMT arg0,
		 PTR * arg1)
{
	TRACE("SQLParamData");

	return ((OdbcStatement*) arg0)->sqlParamData (arg1);
}

///// SQLPutData /////

RETCODE SQL_API SQLPutData  (HSTMT arg0,
		 PTR arg1,
		 SDWORD arg2)
{
	TRACE ("SQLPutData");
	return ((OdbcStatement*) arg0)->sqlPutData (arg1, arg2);
}

///// SQLSetConnectOption ///// Deprecated

RETCODE SQL_API SQLSetConnectOption  (HDBC arg0,
		 UWORD arg1,
		 SQLPOINTER arg2)
{
	TRACE ("SQLSetConnectOption");

	return SQLSetConnectAttr (arg0, arg1, arg2, 0);
}

///// SQLSetStmtOption ///// Deprecated

RETCODE SQL_API SQLSetStmtOption  (HSTMT arg0,
		 UWORD arg1,
		 UDWORD arg2)
{
	notYetImplemented("SQLSetStmtOption called\n");
	return(SQL_SUCCESS);
}

///// SQLSpecialColumns /////

RETCODE SQL_API SQLSpecialColumns  (HSTMT arg0,
		 UWORD arg1,
		 UCHAR * arg2,
		 SWORD arg3,
		 UCHAR * arg4,
		 SWORD arg5,
		 UCHAR * arg6,
		 SWORD arg7,
		 UWORD arg8,
		 UWORD arg9)
{
	TRACE ("SQLSpecialColumns");

	return ((OdbcStatement*) arg0)->sqlSpecialColumns (arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
}

///// SQLStatistics /////

RETCODE SQL_API SQLStatistics  (HSTMT arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UCHAR * arg3,
		 SWORD arg4,
		 UCHAR * arg5,
		 SWORD arg6,
		 UWORD arg7,
		 UWORD arg8)
{
	TRACE ("SQLStatistics");

	return ((OdbcStatement*) arg0)->sqlStatistics (arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
}

///// SQLTables /////

RETCODE SQL_API SQLTables  (HSTMT arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UCHAR * arg3,
		 SWORD arg4,
		 UCHAR * arg5,
		 SWORD arg6,
		 UCHAR * arg7,
		 SWORD arg8)
{
	TRACE ("SQLTables");
	return ((OdbcStatement*) arg0)->sqlTables (arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8);
}

///// SQLBrowseConnect /////

RETCODE SQL_API SQLBrowseConnect  (HDBC arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UCHAR * arg3,
		 SWORD arg4,
		 SWORD * arg5)
{
	notYetImplemented("SQLBrowseConnect called\n");
	return(SQL_SUCCESS);
}

///// SQLColumnPrivileges /////

RETCODE SQL_API SQLColumnPrivileges  (HSTMT arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UCHAR * arg3,
		 SWORD arg4,
		 UCHAR * arg5,
		 SWORD arg6,
		 UCHAR * arg7,
		 SWORD arg8)
{
	notYetImplemented("SQLColumnPrivileges called\n");
	return(SQL_SUCCESS);
}

///// SQLDataSources /////

RETCODE SQL_API SQLDataSources  (HENV arg0,
		 UWORD arg1,
		 UCHAR * arg2,
		 SWORD arg3,
		 SWORD * arg4,
		 UCHAR * arg5,
		 SWORD arg6,
		 SWORD * arg7)
{
	notYetImplemented("SQLDataSources called\n");
	return(SQL_SUCCESS);
}

///// SQLDescribeParam /////

RETCODE SQL_API SQLDescribeParam  (HSTMT arg0,
		 UWORD arg1,
		 SWORD * arg2,
		 UDWORD * arg3,
		 SWORD * arg4,
		 SWORD * arg5)
{
	TRACE("SQLDescribeParam");

	return ((OdbcStatement*) arg0)->sqlDescribeParam (arg1, arg2, arg3, arg4, arg4);
}

///// SQLExtendedFetch /////

RETCODE SQL_API SQLExtendedFetch  (HSTMT arg0,
		 UWORD arg1,
		 SDWORD arg2,
		 UDWORD * arg3,
		 UWORD * arg4)
{
	TRACE ("SQLExtendedFetch");

	return ((OdbcStatement*) arg0)->sqlExtendedFetch(arg1, arg2, arg3, arg4);
}

///// SQLForeignKeys /////

RETCODE SQL_API SQLForeignKeys  (HSTMT arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UCHAR * arg3,
		 SWORD arg4,
		 UCHAR * arg5,
		 SWORD arg6,
		 UCHAR * arg7,
		 SWORD arg8,
		 UCHAR * arg9,
		 SWORD arg10,
		 UCHAR * arg11,
		 SWORD arg12)
{
	TRACE ("SQLForeignKeys");

	return ((OdbcStatement*) arg0)->sqlForeignKeys (arg1, arg2, arg3, arg4, 
													arg5, arg6, arg7, arg8,
													arg9, arg10, arg11, arg12);
}

///// SQLMoreResults /////

RETCODE SQL_API SQLMoreResults  (HSTMT arg0)
{
	TRACE("SQLMoreResults");

	return ((OdbcStatement*) arg0)->sqlMoreResults();
}

///// SQLNativeSql /////

RETCODE SQL_API SQLNativeSql  (HDBC arg0,
		 UCHAR * arg1,
		 SDWORD arg2,
		 UCHAR * arg3,
		 SDWORD arg4,
		 SDWORD * arg5)
{
	notYetImplemented("SQLNumParams called\n");
	return(SQL_SUCCESS);
}

///// SQLNumParams /////

RETCODE SQL_API SQLNumParams  (HSTMT arg0,
		 SWORD * arg1)
{
	notYetImplemented("SQLNumParams called\n");
	return(SQL_SUCCESS);
}

///// SQLParamOptions /////

RETCODE SQL_API SQLParamOptions  (HSTMT arg0,
		 UDWORD arg1,
		 UDWORD * arg2)
{
	notYetImplemented("SQLParamOptions called\n");
	return(SQL_SUCCESS);
}

///// SQLPrimaryKeys /////

RETCODE SQL_API SQLPrimaryKeys  (HSTMT arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UCHAR * arg3,
		 SWORD arg4,
		 UCHAR * arg5,
		 SWORD arg6)
{
	TRACE ("SQLPrimaryKeys");

	return ((OdbcStatement*) arg0)->sqlPrimaryKeys (arg1, arg2, arg3, arg4, arg5, arg6);
}

///// SQLProcedureColumns /////

RETCODE SQL_API SQLProcedureColumns  (HSTMT arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UCHAR * arg3,
		 SWORD arg4,
		 UCHAR * arg5,
		 SWORD arg6,
		 UCHAR * arg7,
		 SWORD arg8)
{
	TRACE ("SQLProcedureColumns");

	return ((OdbcStatement*) arg0)->sqlProcedureColumns (arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8);
}

///// SQLProcedures /////

RETCODE SQL_API SQLProcedures  (HSTMT arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UCHAR * arg3,
		 SWORD arg4,
		 UCHAR * arg5,
		 SWORD arg6)
{
	TRACE ("SQLProcedures");

	return ((OdbcStatement*) arg0)->sqlProcedures (arg1,arg2,arg3,arg4,arg5,arg6);
}

///// SQLSetPos /////

RETCODE SQL_API SQLSetPos  (HSTMT arg0,
		 UWORD arg1,
		 UWORD arg2,
		 UWORD arg3)
{
	notYetImplemented("SQLSetPos called\n");
	return(SQL_SUCCESS);
}

///// SQLSetScrollOptions /////

RETCODE SQL_API SQLSetScrollOptions  (HSTMT arg0,
		 UWORD arg1,
		 SDWORD arg2,
		 UWORD arg3)
{
	notYetImplemented("SQLSetScrollOptions called\n");
	return(SQL_SUCCESS);
}

///// SQLTablePrivileges /////

RETCODE SQL_API SQLTablePrivileges  (HSTMT arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UCHAR * arg3,
		 SWORD arg4,
		 UCHAR * arg5,
		 SWORD arg6)
{
	notYetImplemented("SQLTablePrivileges called\n");
	return(SQL_SUCCESS);
}

///// SQLDrivers /////

RETCODE SQL_API SQLDrivers  (HENV arg0,
		 UWORD arg1,
		 UCHAR * arg2,
		 SWORD arg3,
		 SWORD * arg4,
		 UCHAR * arg5,
		 SWORD arg6,
		 SWORD * arg7)
{
	notYetImplemented("SQLDrivers called\n");
	return(SQL_SUCCESS);
}

///// SQLBindParameter /////

RETCODE SQL_API SQLBindParameter  (HSTMT arg0,
		 UWORD arg1,
		 SWORD arg2,
		 SWORD arg3,
		 SWORD arg4,
		 UDWORD arg5,
		 SWORD arg6,
		 PTR arg7,
		 SDWORD arg8,
			SDWORD * arg9)
{
	TRACE ("SQLBindParameter");

	return ((OdbcStatement*) arg0)->sqlBindParameter (arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
}

///// SQLAllocHandle - global /////

RETCODE SQL_API SQLAllocHandle (SQLSMALLINT arg0,
		 SQLHANDLE arg1,
		 SQLHANDLE * arg2)
{
	return __SQLAllocHandle (arg0, arg1, arg2);
}


///// SQLBindParam /////

RETCODE SQL_API SQLBindParam  (SQLHSTMT arg0,
		 SQLUSMALLINT arg1,
		 SQLSMALLINT arg2,
		 SQLSMALLINT arg3,
		 SQLUINTEGER arg4,
		 SQLSMALLINT arg5,
		 SQLPOINTER arg6,
		 SQLINTEGER * arg7)
{
	notYetImplemented("SQLBindParam called\n");
	return(SQL_SUCCESS);
}

///// SQLCloseCursor /////

RETCODE SQL_API SQLCloseCursor  (SQLHSTMT arg0)
{
	TRACE ("SQLCloseCursor");

	return ((OdbcStatement*) arg0)->sqlCloseCursor();
}

///// SQLColAttribute /////

RETCODE SQL_API SQLColAttribute  (SQLHSTMT arg0,
		 SQLUSMALLINT arg1,
		 SQLUSMALLINT arg2,
		 SQLPOINTER arg3,
		 SQLSMALLINT arg4,
		 SQLSMALLINT * arg5,
		 SQLPOINTER arg6)
{
	TRACE ("SQLColAttribute");

	return ((OdbcStatement*) arg0)->sqlColAttribute (arg1, arg2, arg3, arg4, arg5, arg6);
}

///// SQLCopyDesc /////

RETCODE SQL_API SQLCopyDesc  (SQLHDESC arg0,
		 SQLHDESC arg1)
{
	notYetImplemented("SQLCopyDesc called\n");
	return(SQL_SUCCESS);
}

///// SQLEndTran /////

RETCODE SQL_API SQLEndTran  (SQLSMALLINT arg0,
		 SQLHANDLE arg1,
		 SQLSMALLINT arg2)
{
	TRACE ("SQLEndTran");

	switch (arg0)
		{
		case SQL_HANDLE_ENV:
			return ((OdbcEnv*) arg1)->sqlEndTran (arg2);

		case SQL_HANDLE_DBC:
			return ((OdbcConnection*) arg1)->sqlEndTran (arg2);
		}

	return SQL_INVALID_HANDLE;
}

///// SQLFetchScroll /////

RETCODE SQL_API SQLFetchScroll  (SQLHSTMT arg0,
		 SQLSMALLINT arg1,
		 SQLINTEGER arg2)
{
	TRACE ("SQLFetchScroll");

	return ((OdbcStatement*) arg0)->sqlFetchScroll (arg1, arg2);
}

///// SQLFreeHandle /////

RETCODE SQL_API SQLFreeHandle  (SQLSMALLINT arg0,
		 SQLHANDLE arg1)
{
	TRACE ("SQLFreeHandle\n");

	switch (arg0)
		{
		case SQL_HANDLE_ENV:
			delete (OdbcEnv*) arg1;
			break;

		case SQL_HANDLE_DBC:
			delete (OdbcConnection*) arg1;
			break;

		case SQL_HANDLE_STMT:
			delete (OdbcStatement*) arg1;
			break;

		case SQL_HANDLE_DESC:
			notYetImplemented ("SQLFreeHandle DESC");

		default:
			return SQL_INVALID_HANDLE;
		}

	return SQL_SUCCESS;
}

///// SQLGetConnectAttr /////

RETCODE SQL_API SQLGetConnectAttr  (SQLHDBC arg0,
		 SQLINTEGER arg1,
		 SQLPOINTER arg2,
		 SQLINTEGER arg3,
		 SQLINTEGER * arg4)
{
	TRACE ("SQLGetConnectAttr");

	return ((OdbcConnection*) arg0)->sqlGetConnectAttr (arg1, arg2, arg3, arg4);
}

///// SQLGetDescField /////

RETCODE SQL_API SQLGetDescField  (SQLHDESC arg0,
		 SQLSMALLINT arg1,
		 SQLSMALLINT arg2,
		 SQLPOINTER arg3,
		 SQLINTEGER arg4,
		 SQLINTEGER * arg5)
{
	notYetImplemented("SQLGetDescField called\n");
	return(SQL_SUCCESS);
}

///// SQLGetDescRec /////

RETCODE SQL_API SQLGetDescRec  (SQLHDESC arg0,
		 SQLSMALLINT arg1,
		 SQLCHAR * arg2,
		 SQLSMALLINT arg3,
		 SQLSMALLINT * arg4,
		 SQLSMALLINT * arg5,
		 SQLSMALLINT * arg6,
		 SQLINTEGER  * arg7,
		 SQLSMALLINT * arg8,
		 SQLSMALLINT * arg9,
		 SQLSMALLINT * arg10)
{
	notYetImplemented("SQLGetDescRec called\n");
	return(SQL_SUCCESS);
}

///// SQLGetDiagField /////

RETCODE SQL_API SQLGetDiagField  (SQLSMALLINT arg0,
		 SQLHANDLE arg1,
		 SQLSMALLINT arg2,
		 SQLSMALLINT arg3,
		 SQLPOINTER arg4,
		 SQLSMALLINT arg5,
		 SQLSMALLINT * arg6)
{
	TRACE ("SQLGetDiagField");

	return ((OdbcObject*) arg1)->sqlGetDiagField (arg2,arg3,arg4,arg5,arg6);
}

///// SQLGetDiagRec /////

RETCODE SQL_API SQLGetDiagRec  (SQLSMALLINT arg0,
		 SQLHANDLE arg1,
		 SQLSMALLINT arg2,
		 SQLCHAR * arg3,
		 SQLINTEGER * arg4,
		 SQLCHAR * arg5,
		 SQLSMALLINT arg6,
		 SQLSMALLINT * arg7)
{
	TRACE ("SQLGetDiagRec");

	return ((OdbcObject*) arg1)->sqlGetDiagRec (arg0, arg2,arg3,arg4,arg5,arg6,arg7);
}

///// SQLGetEnvAttr /////

RETCODE SQL_API SQLGetEnvAttr  (SQLHENV arg0,
		 SQLINTEGER arg1,
		 SQLPOINTER arg2,
		 SQLINTEGER arg3,
		 SQLINTEGER * arg4)
{
	notYetImplemented("SQLGetEnvAttr called\n");
	return(SQL_SUCCESS);
}

///// SQLGetStmtAttr /////

RETCODE SQL_API SQLGetStmtAttr  (SQLHSTMT arg0,
		 SQLINTEGER arg1,
		 SQLPOINTER arg2,
		 SQLINTEGER arg3,
		 SQLINTEGER * arg4)
{
	TRACE ("SQLGetStmtAttr");

	return ((OdbcStatement*) arg0)->sqlGetStmtAttr (arg1,arg2,arg3,arg4);
}

///// SQLSetConnectAttr /////

RETCODE SQL_API SQLSetConnectAttr  (SQLHDBC arg0,
		 SQLINTEGER arg1,
		 SQLPOINTER arg2,
		 SQLINTEGER arg3)
{
	TRACE ("SQLSetConnectAttr");

	return ((OdbcConnection*) arg0)->sqlSetConnectAttr (arg1, arg2, arg3);
}

///// SQLSetDescField /////

RETCODE SQL_API SQLSetDescField  (SQLHDESC arg0,
		 SQLSMALLINT arg1,
		 SQLSMALLINT arg2,
		 SQLPOINTER arg3,
		 SQLINTEGER arg4)
{
	TRACE ("SQLSetDescField");

	return ((OdbcDesc*) arg0)->sqlSetDescField (arg1, arg2, arg3, arg4);
}

///// SQLSetDescRec /////

RETCODE SQL_API SQLSetDescRec  (SQLHDESC arg0,
		 SQLSMALLINT arg1,
		 SQLSMALLINT arg2,
		 SQLSMALLINT arg3,
		 SQLINTEGER arg4,
		 SQLSMALLINT arg5,
		 SQLSMALLINT arg6,
		 SQLPOINTER arg7,
		 SQLINTEGER * arg8,
		 SQLINTEGER * arg9)
{
	notYetImplemented("SQLSetDescRec called\n");
	return(SQL_SUCCESS);
}

///// SQLSetEnvAttr /////

RETCODE SQL_API SQLSetEnvAttr  (SQLHENV arg0,
		 SQLINTEGER arg1,
		 SQLPOINTER arg2,
		 SQLINTEGER arg3)
{
	TRACE ("SQLSetEnvAttr");

	return ((OdbcEnv*) arg0)->sqlSetEnvAttr (arg1, arg2, arg3);
}

///// SQLSetStmtAttr /////

RETCODE SQL_API SQLSetStmtAttr  (SQLHSTMT arg0,
		 SQLINTEGER arg1,
		 SQLPOINTER arg2,
		 SQLINTEGER arg3)
{
	TRACE ("SQLSetStmtAttr");

	return ((OdbcStatement*) arg0)->sqlSetStmtAttr (arg1, arg2, arg3);
}


///// SQLBulkOperations /////

RETCODE SQL_API SQLBulkOperations  (SQLHSTMT arg0,
			SQLSMALLINT arg1)
{
	notYetImplemented("SQLBulkOperations called\n");
	return(SQL_SUCCESS);
}
