/*
 *	PROGRAM:		SQL Converter
 *	MODULE:			Odbc.h
 *	DESCRIPTION:	Help ODBC function declarations
 *
 * copyright (c) 1997 by James A. Starkey for IBPhoenix.
 */

#ifdef _WIN32
#include <afxdb.h>
#else
#include <sql.h>
#include <sqlext.h>
//#define OutputDebugString	puts
#endif

#define GET_STRING(stmt,col,data,len) OdbcCheckCode (SQLGetData(stmt, col, SQL_C_CHAR, data, sizeof (data), &len), stmt, "SQLGetData")
#define GET_LONG(stmt,col,data,len)   OdbcCheckCode (SQLGetData(stmt, col, SQL_C_SLONG, &data, sizeof (data), &len), stmt, "SQLGetData")

#define BIND_STRING(stmt,col,data,len) OdbcCheckCode (SQLBindCol(stmt, col, SQL_C_CHAR, data, sizeof (data), &len), stmt, "SQLBindData")
#define BIND_LONG(stmt,col,data,len)   OdbcCheckCode (SQLBindCol(stmt, col, SQL_C_SLONG, &data, sizeof (data), &len), stmt, "SQLBindData")

int OdbcCheckCode (int retcode, SQLHANDLE statement, const char *string, int handleType=SQL_HANDLE_STMT);

/***
void OdbcExecute (CDatabase*, const char* sql);
int OdbcTableEmpty (CDatabase*, const char* tableName);
***/
