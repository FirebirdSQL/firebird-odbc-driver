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
 *
 *
 *	2003-03-24	main.cpp
 *				Contributed by Roger Gammans
 *				Fix SQLError prototype to match the prototype
 *				in the sql.h header file. This stops g++ 
 *				generating SQLError as a C++ name-mangled 
 *				entry point. A C function entry is used 
 *				allowing correct binding to the driver manager.
 *
 *	2002-10-11	main.cpp
 *				Contributed by C G Alvarez
 *              Implement SQLNumParams()
 *
 *	2002-10-11	main.cpp
 *				Contributed by C G Alvarez
 *              Implement SQLTablePrivileges()
 *
 *  2002-10-11  main.cpp
 *				Contributed by C G Alvarez
 *				Implement SQLColumnPrivileges()
 *
 *	2002-10-11	main.cpp
 *				Contributed by C G Alvarez
 *              Implement SQLGetDescField()
 *
 *  2002-08-02  main.cpp
 *				Contributed by C G Alvarez
 *				Implement SQLGetEnvAttr()
 *
 *
 *	2002-07-02	main.cpp
 *				Contributed by C G Alvarez
 *				Mapped calls to SQLGetConnectOption to SQLGetConnectAttr
 *				Mapped calls SQLGetStmtOption to SQLGetStmtAttr
 *				Mapped calls SQLSetStmtOption to SQLSetStmtAttr

 *
 *	2002-04-30  main.cpp
 *				Added suggestions by Carlos G Alvarez 
 *				o Test for logfile before trying to close it.
 *				o Changed parameter types for SQLSetConnectOption
 *	
 */
// D:\FIREDRV\OdbcJdbc\Visdata.exe

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
#include "OdbcJdbc.h"
#include "OdbcEnv.h"
#include "OdbcConnection.h"
#include "OdbcStatement.h"
#include "SafeEnvThread.h"

#define __MONITOR_EXECUTING

#ifdef _WIN32
#define OUTPUT_MONITOR_EXECUTING(msg)  OutputDebugString(msg"\n");
#else
#define OUTPUT_MONITOR_EXECUTING(msg)
#endif

#ifdef DEBUG
#define TRACE(msg)		trace (msg"\n")
#else
#define TRACE(msg)		OUTPUT_MONITOR_EXECUTING(msg)
#endif

#if(DRIVER_LOCKED_LEVEL == DRIVER_LOCKED_LEVEL_ENV)

#define GUARD					SafeDllThread wt
#define GUARD_ENV(arg)			GUARD
#define GUARD_HSTMT(arg)		GUARD
#define GUARD_HDBC(arg)			GUARD
#define GUARD_HDESC(arg)		GUARD
#define GUARD_HTYPE(arg1,arg2)	GUARD

#elif(DRIVER_LOCKED_LEVEL == DRIVER_LOCKED_LEVEL_CONNECT)

#define GUARD					SafeDllThread wt
#define GUARD_ENV(arg)			SafeEnvThread wt((OdbcEnv*)arg)
#define GUARD_HSTMT(arg)		SafeConnectThread wt(((OdbcStatement*)arg)->connection)
#define GUARD_HDBC(arg) 		SafeConnectThread wt((OdbcConnection*)arg)
#define GUARD_HDESC(arg)		SafeConnectThread wt(((OdbcDesc*)arg)->connection)
#define GUARD_HTYPE(arg,arg1)	SafeConnectThread wt(												\
									arg1==SQL_HANDLE_DBC ? (OdbcConnection*)arg:					\
									arg1==SQL_HANDLE_STMT ? ((OdbcStatement*)arg)->connection:		\
									arg1==SQL_HANDLE_DESC ? ((OdbcDesc*)arg)->connection : NULL )

#else

#define GUARD
#define GUARD_ENV(arg)
#define GUARD_HSTMT(arg)
#define GUARD_HDBC(arg)
#define GUARD_HDESC(arg)	
#define GUARD_HTYPE(arg1,arg2)

#endif

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
}

#ifdef _WIN32
HINSTANCE m_hInstance = NULL;

BOOL APIENTRY DllMain(  HINSTANCE hinstDLL, DWORD fdwReason, LPVOID )
{
	if ( fdwReason == DLL_PROCESS_ATTACH )
		m_hInstance = hinstDLL;

    return TRUE;
}
#endif

static RETCODE __SQLAllocHandle  (SQLSMALLINT arg0, SQLHANDLE arg1, SQLHANDLE * arg2)
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

///// SQLAllocConnect /////	ODBC 1.0	///// Deprecated

RETCODE SQL_API SQLAllocConnect  (HENV arg0,
			 HDBC * arg1)
{
	TRACE ("SQLAllocConnect");
	GUARD_ENV(arg0);

	return __SQLAllocHandle (SQL_HANDLE_DBC, arg0, arg1);
}

///// SQLAllocEnv /////		ODBC 1.0	///// Deprecated

RETCODE SQL_API SQLAllocEnv  (HENV * arg0)
{
	TRACE ("SQLAllocEnv");
	GUARD;

	return __SQLAllocHandle (SQL_HANDLE_ENV, SQL_NULL_HANDLE, arg0);
}

///// SQLAllocStmt /////	ODBC 1.0	///// Deprecated

RETCODE SQL_API SQLAllocStmt  (HDBC arg0,
		 HSTMT * arg1)
{
	TRACE ("SQLAllocStmt");
	GUARD_HDBC(arg0);

	return __SQLAllocHandle (SQL_HANDLE_STMT, arg0, arg1);
}

///// SQLBindCol /////	ODBC 1.0	///// ISO 92

RETCODE SQL_API SQLBindCol  (HSTMT arg0,
			UWORD arg1,
			SWORD arg2,
			PTR arg3,
			SDWORD arg4,
			SDWORD * arg5)
{
	TRACE ("SQLBindCol");
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlBindCol (arg1, arg2, arg3, arg4, arg5);
}

///// SQLCancel /////	ODBC 1.0	///// Deprecated

RETCODE SQL_API SQLCancel  (HSTMT arg0)
{
	TRACE ("SQLCancel");

	return ((OdbcStatement*) arg0)->sqlCancel ();
}

///// SQLColAttributes /////	ODBC 1.0	///// Deprecated

RETCODE SQL_API SQLColAttributes  (HSTMT arg0,
		 UWORD arg1,
		 UWORD arg2,
		 PTR arg3,
		 SWORD arg4,
		 SWORD * arg5,
		 SDWORD * arg6)
{
	TRACE("SQLColAttributes");
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlColAttributes (arg1, arg2, arg3, arg4, arg5, arg6);
}

///// SQLConnect /////	ODBC 1.0	///// ISO 92

RETCODE SQL_API SQLConnect  (HDBC arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UCHAR * arg3,
		 SWORD arg4,
		 UCHAR * arg5,
		 SWORD arg6)
{
	TRACE ("SQLConnect");
	GUARD_HDBC(arg0);

	return ((OdbcConnection*) arg0)->sqlConnect (arg1, arg2, arg3, arg4, arg5, arg6);
}

///// SQLDescribeCol /////	ODBC 1.0	///// ISO 92

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
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlDescribeCol (arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8);
}

///// SQLDisconnect /////	ODBC 1.0	///// ISO 92

RETCODE SQL_API SQLDisconnect  (HDBC arg0)
{
	TRACE ("SQLDisconnect");
	GUARD_ENV(((OdbcConnection*) arg0)->env);

#ifdef LOGGING
	if ( logFile )
	{
		fclose (logFile);
		logFile = NULL;
	}
#endif

	return ((OdbcConnection*) arg0)->sqlDisconnect();
}

///// SQLError /////	ODBC 1.0	///// Deprecated

RETCODE SQL_API SQLError  (HENV env,
		 HDBC connection,
		 HSTMT statement,
		 UCHAR * sqlState,
		 SDWORD * nativeErrorCode,
		 UCHAR * msgBuffer,
		 SWORD msgBufferLength,
		 SWORD *msgLength)
{
	TRACE("SQLError");

	if (statement)
	{
		GUARD_HSTMT(statement);
		return ((OdbcStatement*)statement)->sqlError (sqlState, nativeErrorCode, msgBuffer, msgBufferLength, msgLength);
	}
	if (connection)
	{
		GUARD_HDBC(connection);
		return ((OdbcConnection*)connection)->sqlError (sqlState, nativeErrorCode, msgBuffer, msgBufferLength, msgLength);
	}
	if (env)
		return ((OdbcEnv*)env)->sqlError (sqlState, nativeErrorCode, msgBuffer, msgBufferLength, msgLength);

	return SQL_ERROR;
}

///// SQLExecDirect /////	ODBC 1.0	///// ISO 92

RETCODE SQL_API SQLExecDirect  (HSTMT arg0,
		 UCHAR * arg1,
		 SDWORD arg2)
{
	TRACE ("SQLExecDirect");
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlExecuteDirect(arg1, arg2);
}

///// SQLExecute /////	ODBC 1.0	///// ISO 92

RETCODE SQL_API SQLExecute  (HSTMT arg0)
{
	TRACE("SQLExecute");
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlExecute();
}

///// SQLFetch /////	ODBC 1.0	///// ISO 92

RETCODE SQL_API SQLFetch  (HSTMT arg0)
{
	TRACE ("SQLFetch");
	GUARD_HSTMT(arg0);
	
	return ((OdbcStatement*) arg0)->sqlFetch();
}

///// SQLFreeConnect /////	ODBC 1.0	///// Deprecated

RETCODE SQL_API SQLFreeConnect  (HDBC arg0)
{
	TRACE ("SQLFreeconnect");
	GUARD_HDBC(arg0);

	delete (OdbcConnection*) arg0;
	return SQL_SUCCESS;
}

///// SQLFreeEnv /////	ODBC 3.0	///// ISO 92

RETCODE SQL_API SQLFreeEnv  (HENV arg0)
{
	TRACE ("SQLFreeEnv");

	delete (OdbcEnv*) arg0;
	return SQL_SUCCESS;
}

///// SQLFreeStmt /////	ODBC 1.0	///// ISO 92

RETCODE SQL_API SQLFreeStmt  (HSTMT arg0,
		 UWORD arg1)
{
	TRACE ("SQLFreeStmt");
	GUARD_HSTMT(arg0);

	if (arg1 == SQL_DROP)
	{
		delete (OdbcStatement*) arg0;
		return SQL_SUCCESS;
	}

	return ((OdbcStatement*) arg0)->sqlFreeStmt (arg1);
}

///// SQLGetCursorName /////	ODBC 1.0	///// ISO 92

RETCODE SQL_API SQLGetCursorName  (HSTMT arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 SWORD * arg3)
{
	TRACE ("SQLGetCursorName called\n");
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlGetCursorName (arg1, arg2, arg3);
}

///// SQLNumResultCols /////	ODBC 1.0	///// ISO 92

RETCODE SQL_API SQLNumResultCols  (HSTMT arg0, SWORD * arg1)
{
	TRACE ("SQLNumResultCols");
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlNumResultCols (arg1);
}

///// SQLPrepare /////	ODBC 1.0	///// ISO 92

RETCODE SQL_API SQLPrepare  (HSTMT arg0,
		 UCHAR * arg1,
		 SDWORD arg2)
{
	TRACE ("SQLPrepare");
	GUARD_HSTMT(arg0);
	
	return ((OdbcStatement*) arg0)->sqlPrepare (arg1, arg2, false);

}

///// SQLRowCount /////	ODBC 1.0	///// ISO 92

RETCODE SQL_API SQLRowCount  (HSTMT arg0, SDWORD * arg1)
{
	TRACE ("SQLRowCount");
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlRowCount (arg1);
}

///// SQLSetCursorName /////	ODBC 1.0	///// ISO 92

RETCODE SQL_API SQLSetCursorName  (HSTMT arg0,
		 UCHAR * arg1,
		 SWORD arg2)
{
	TRACE ("SQLSetCursorName");
	GUARD_HSTMT(arg0);

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
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlSetParam (arg1, arg2, arg3, arg4, arg5, arg6, arg7);
}

///// SQLTransact /////

RETCODE SQL_API SQLTransact  (HENV arg0,
		 HDBC arg1,
		 UWORD arg2)
{
	TRACE ("SQLTransact");

	if (arg0 == SQL_NULL_HENV)
	{
		GUARD_HDBC(arg1);
		return ((OdbcConnection*) arg1)->sqlEndTran (arg2);
	}

	return ((OdbcEnv*) arg0)->sqlEndTran (arg2);
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
	GUARD_HSTMT(arg0);

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
	GUARD_HDBC(arg0);

	return ((OdbcConnection*) arg0)->sqlDriverConnect (
				hWnd, szConnStrIn, cbConnStrIn,
				szConnStrOut, cbConnStrOut, pcbConnStrOut,
				uwMode);
}

///// SQLGetConnectOption /////  Level 1	///// Deprecated

RETCODE SQL_API SQLGetConnectOption  (HDBC arg0,
		 UWORD arg1,
		 PTR arg2)
{
//Added by C. G. A.
	TRACE ("SQLGetConnectOption");
	GUARD_HDBC(arg0);

	return ((OdbcConnection*) arg0)->sqlGetConnectAttr (arg1, arg2, 0, NULL);

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
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlGetData (arg1, arg2, arg3, arg4, arg5);
}

///// SQLGetFunctions /////

RETCODE SQL_API SQLGetFunctions  (HDBC arg0,
		 SQLUSMALLINT arg1,
		 SQLUSMALLINT  *arg2)
{
	TRACE ("SQLGetFunctions");
	GUARD_HDBC(arg0);

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
	GUARD_HDBC(arg0);

	return ((OdbcConnection*) arg0)->sqlGetInfo (arg1, arg2, arg3, arg4);
}

///// SQLGetStmtOption /////  Level 1

RETCODE SQL_API SQLGetStmtOption  (HSTMT arg0,
		 UWORD arg1,
		 PTR arg2)
{
	TRACE ("SQLGetStmtOption");
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlGetStmtAttr (arg1, arg2, 0, NULL);
}

///// SQLGetTypeInfo /////

RETCODE SQL_API SQLGetTypeInfo  (HSTMT arg0,
		 SWORD arg1)
{
	TRACE ("SQLGetTypeInfo");
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlGetTypeInfo (arg1);
}

///// SQLParamData /////

RETCODE SQL_API SQLParamData  (HSTMT arg0,
		 PTR * arg1)
{
	TRACE("SQLParamData");
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlParamData (arg1);
}

///// SQLPutData /////

RETCODE SQL_API SQLPutData  (HSTMT arg0,
		 PTR arg1,
		 SDWORD arg2)
{
	TRACE ("SQLPutData");
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlPutData (arg1, arg2);
}


///// SQLSetConnectOption /////  Level 1	///// Deprecated

//Proposed by Carlos Guzmn lvarez 2002-04-30
RETCODE SQL_API SQLSetConnectOption  (HDBC arg0,
                 SQLUSMALLINT arg1,
                 SQLUINTEGER arg2)
{
	TRACE ("SQLSetConnectOption");
	GUARD_HDBC(arg0);

	return ((OdbcConnection*) arg0)->sqlSetConnectAttr (arg1, (SQLPOINTER)arg2,0);
}


///// SQLSetStmtOption ///// Deprecated

RETCODE SQL_API SQLSetStmtOption  (HSTMT arg0,
		 UWORD arg1,
		 UDWORD arg2)
{
	TRACE ("SQLSetStmtOption");
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlSetStmtAttr (arg1, (SQLPOINTER) arg2, 0);
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
	GUARD_HSTMT(arg0);

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
	GUARD_HSTMT(arg0);

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
	GUARD_HSTMT(arg0);

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
	TRACE ("SQLBrowseConnect");
	GUARD_HDBC(arg0);

	return ((OdbcConnection*) arg0)->sqlBrowseConnect (arg1, arg2, arg3, arg4, arg5);
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
	TRACE ("SQLDataSources");
	GUARD_ENV(arg0);

	return ((OdbcEnv*)arg0)->sqlDataSources (arg1, arg2, arg3, arg4, arg5, arg6, arg7);
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
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlDescribeParam (arg1, arg2, arg3, arg4, arg5);
}

///// SQLExtendedFetch /////

RETCODE SQL_API SQLExtendedFetch  (HSTMT arg0,
		 UWORD arg1,
		 SDWORD arg2,
		 UDWORD * arg3,
		 UWORD * arg4)
{
	TRACE ("SQLExtendedFetch");
	GUARD_HSTMT(arg0);

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
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlForeignKeys (arg1, arg2, arg3, arg4, 
													arg5, arg6, arg7, arg8,
													arg9, arg10, arg11, arg12);
}

///// SQLMoreResults /////

RETCODE SQL_API SQLMoreResults  (HSTMT arg0)
{
	TRACE("SQLMoreResults");
	GUARD_HSTMT(arg0);

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
	TRACE ("SQLNativeSql");
	GUARD_HDBC(arg0);

	return ((OdbcConnection*) arg0)->sqlNativeSql (arg1, arg2, arg3, arg4, arg5);
}

///// SQLNumParams /////

RETCODE SQL_API SQLNumParams  (HSTMT arg0,
		 SWORD * arg1)
{
	TRACE("SQLMoreResults");
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlNumParams(arg1);		
}

///// SQLParamOptions /////

RETCODE SQL_API SQLParamOptions  (HSTMT arg0,
		 UDWORD arg1,
		 UDWORD * arg2)
{
	TRACE("SQLParamOptions");
	GUARD_HSTMT(arg0);

	((OdbcStatement*) arg0)->sqlSetStmtAttr(SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)arg1, 0);
	((OdbcStatement*) arg0)->sqlSetStmtAttr(SQL_ATTR_PARAMS_PROCESSED_PTR, (SQLPOINTER)arg2, 0);

	return SQL_SUCCESS;
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
	GUARD_HSTMT(arg0);

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
	GUARD_HSTMT(arg0);

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
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlProcedures (arg1,arg2,arg3,arg4,arg5,arg6);
}

///// SQLSetPos /////

RETCODE SQL_API SQLSetPos  (HSTMT arg0,
		 UWORD arg1,
		 UWORD arg2,
		 UWORD arg3)
{
	TRACE ("SQLSetPos");
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlSetPos (arg1, arg2, arg3);
}

///// SQLSetScrollOptions /////

RETCODE SQL_API SQLSetScrollOptions  (HSTMT arg0,
		 UWORD arg1,
		 SDWORD arg2,
		 UWORD arg3)
{
	TRACE ("SQLSetScrollOptions");
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlSetScrollOptions (arg1, arg2, arg3);
}

///// SQLTablePrivileges /////

RETCODE SQL_API SQLTablePrivileges  (
		 HSTMT arg0,
		 UCHAR * arg1,
		 SWORD arg2,
		 UCHAR * arg3,
		 SWORD arg4,
		 UCHAR * arg5,
		 SWORD arg6)
{
	TRACE ("SQLTablePrivileges");
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlTablePrivileges (arg1,arg2,arg3,arg4,arg5,arg6);
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
	TRACE ("SQLColumnPrivileges");
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlColumnPrivileges (arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8);
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
	TRACE ("SQLDrivers");
	GUARD_ENV(arg0);

	return ((OdbcEnv*)arg0)->sqlDrivers (arg1, arg2, arg3, arg4, arg5, arg6, arg7);
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
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlBindParameter (arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
}

///// SQLAllocHandle - global /////

RETCODE SQL_API SQLAllocHandle (SQLSMALLINT arg0, SQLHANDLE arg1, SQLHANDLE * arg2)
{
	TRACE ("SQLAllocHandle");

	switch( arg0 )
	{
	case SQL_HANDLE_ENV:
		{
			GUARD;
			return __SQLAllocHandle (arg0, arg1, arg2);
		}

	case SQL_HANDLE_DBC:
		{
			GUARD_ENV(arg1);
			return __SQLAllocHandle (arg0, arg1, arg2);
		}

	case SQL_HANDLE_STMT:
		{
			GUARD_HDBC(arg1);
			return __SQLAllocHandle (arg0, arg1, arg2);
		}

	case SQL_HANDLE_DESC:
		{
			GUARD_HDBC(arg1);
			return __SQLAllocHandle (arg0, arg1, arg2);
		}
	}

	return SQL_INVALID_HANDLE;
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
	TRACE ("SQLBindParam");
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlBindParameter (arg1, SQL_PARAM_INPUT, arg2, arg3, arg4, arg5, arg6, SQL_SETPARAM_VALUE_MAX, arg7);
}

///// SQLCloseCursor /////

RETCODE SQL_API SQLCloseCursor  (SQLHSTMT arg0)
{
	TRACE ("SQLCloseCursor");
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlCloseCursor();
}

///// SQLColAttribute ///// ODBC 3.0 ///// ISO 92

RETCODE SQL_API SQLColAttribute  (SQLHSTMT arg0,
		 SQLUSMALLINT arg1,
		 SQLUSMALLINT arg2,
		 SQLPOINTER arg3,
		 SQLSMALLINT arg4,
		 SQLSMALLINT * arg5,
		 SQLPOINTER arg6)
{
	TRACE ("SQLColAttribute");
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlColAttribute (arg1, arg2, arg3, arg4, arg5, arg6);
}

///// SQLCopyDesc /////
RETCODE SQL_API SQLCopyDesc  (SQLHDESC arg0,
		 SQLHDESC arg1)
{
	TRACE ("SQLCopyDesc");
	GUARD_HDESC(arg0);
	if( arg0 == NULL || arg1 == NULL )
		return SQL_ERROR;

	return *(OdbcDesc*)arg1 = *(OdbcDesc*)arg0;
}

///// SQLEndTran ///// ODBC 3.0 ///// ISO 92

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
			{
				GUARD_HDBC(arg1);
				return ((OdbcConnection*) arg1)->sqlEndTran (arg2);
			}
		}

	return SQL_INVALID_HANDLE;
}

///// SQLFetchScroll /////

RETCODE SQL_API SQLFetchScroll  (SQLHSTMT arg0,
		 SQLSMALLINT arg1,
		 SQLINTEGER arg2)
{
	TRACE ("SQLFetchScroll");
	GUARD_HSTMT(arg0);

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
			{
				GUARD_HDBC(arg1);
				delete (OdbcConnection*) arg1;
			}
			break;

		case SQL_HANDLE_STMT:
			{
				GUARD_HSTMT(arg1);
				delete (OdbcStatement*) arg1;
			}
			break;

		case SQL_HANDLE_DESC:
			{
				GUARD_HDESC(arg1);
				if ( ((OdbcDesc*)arg1)->headType == odtApplication )
					delete (OdbcDesc*) arg1;
			}
			break;

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
	GUARD_HDBC(arg0);

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
	TRACE ("SQLGetDescField");
	GUARD_HDESC(arg0);

	return ((OdbcDesc*) arg0)->sqlGetDescField (arg1, arg2, arg3, arg4, arg5);
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
	TRACE ("SQLGetDescRec");
	GUARD_HDESC(arg0);

	return ((OdbcDesc*) arg0)->sqlGetDescRec (arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
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
	GUARD_HTYPE(arg1,arg0);

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
	GUARD_HTYPE(arg1,arg0);

	return ((OdbcObject*) arg1)->sqlGetDiagRec (arg0, arg2,arg3,arg4,arg5,arg6,arg7);
}

///// SQLGetEnvAttr /////

RETCODE SQL_API SQLGetEnvAttr  (SQLHENV arg0,
		 SQLINTEGER arg1,
		 SQLPOINTER arg2,
		 SQLINTEGER arg3,
		 SQLINTEGER * arg4)
{
	TRACE ("SQLGetEnvAttr");

	return ((OdbcEnv*) arg0)->sqlGetEnvAttr (arg1, arg2, arg3, arg4);
}

///// SQLGetStmtAttr /////

RETCODE SQL_API SQLGetStmtAttr  (SQLHSTMT arg0,
		 SQLINTEGER arg1,
		 SQLPOINTER arg2,
		 SQLINTEGER arg3,
		 SQLINTEGER * arg4)
{
	TRACE ("SQLGetStmtAttr");
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlGetStmtAttr (arg1,arg2,arg3,arg4);
}

///// SQLSetConnectAttr /////

RETCODE SQL_API SQLSetConnectAttr  (SQLHDBC arg0,
		 SQLINTEGER arg1,
		 SQLPOINTER arg2,
		 SQLINTEGER arg3)
{
	TRACE ("SQLSetConnectAttr");
	GUARD_HDBC(arg0);

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
	GUARD_HDESC(arg0);

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
	TRACE ("SQLSetDescRec");
	GUARD_HDESC(arg0);

	return ((OdbcDesc*) arg0)->sqlSetDescRec (arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
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
	GUARD_HSTMT(arg0);

	return ((OdbcStatement*) arg0)->sqlSetStmtAttr (arg1, arg2, arg3);
}


///// SQLBulkOperations /////

RETCODE SQL_API SQLBulkOperations  (SQLHSTMT arg0,
			SQLSMALLINT arg1)
{
	GUARD_HSTMT(arg0);
#pragma FB_COMPILER_MESSAGE("SQLBulkOperations - Implemented; FIXME!")
	notYetImplemented("SQLBulkOperations called\n");
	return(SQL_SUCCESS);
}
