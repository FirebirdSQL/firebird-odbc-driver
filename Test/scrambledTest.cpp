#include <stdio.h>
#include "Odbc.h"
#include "Print.h"

static void test1 (const char *connectString);
static void test2 (const char *connectString);
static bool SimpleRetrieval ();

main (int argc, const char **argv)
{
	const char **end = argv + argc;
	const char *connectString = "ODBC;DSN=FireBird;DRIVER=OdbcJdbc";
	Print print;

	for (++argv; argv < end;)
		{
		const char *p = *argv++;
		if (p [0] == '-')
			switch (p [1])
				{
				case 'i':
					//install();
					break;

				case 'c':
					connectString = *argv++;
					break;

				default:
					printf ("Don't understand switch '%s'\n", p);
					return 1;
				}
		}

	test1 (connectString);

	return 0;
}

void test1 (const char *connectString)
{
	HENV env;
	int ret = SQLAllocHandle (SQL_HANDLE_ENV, NULL, &env);
	ret = SQLSetEnvAttr (env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3, SQL_IS_UINTEGER);
	if (!OdbcCheckCode (ret, env, "SQLSetEnvAttr", SQL_HANDLE_ENV))
		return;

	HDBC connection;
	ret = SQLAllocHandle (SQL_HANDLE_DBC, env, &connection);
	if (!OdbcCheckCode (ret, env, "SQLAllocHandle", SQL_HANDLE_ENV))
		return;

	//ret = SQLAllocConnect (env, &connection);

	/***
	UCHAR buffer [128];
	SWORD bufferLength;
	ret = SQLDriverConnect (connection, NULL, 
							(UCHAR*) connectString,	SQL_NTS,
							buffer, sizeof (buffer), &bufferLength,
							SQL_DRIVER_NOPROMPT);

	if (!OdbcCheckCode (ret, connection, "SQLDriverConnect", SQL_HANDLE_DBC))
		return;
	***/

	ret = SQLSetConnectAttr (connection, SQL_ATTR_ODBC_CURSORS, (SQLPOINTER) SQL_CUR_USE_ODBC, 0);
	if (!OdbcCheckCode (ret, connection, "SQLConnect", SQL_HANDLE_DBC))
		return;

	ret = SQLConnect (connection, (UCHAR*) "FireBird", SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS);
	if (!OdbcCheckCode (ret, connection, "SQLConnect", SQL_HANDLE_DBC))
		return;

	ret = SQLSetConnectAttr (connection, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER) SQL_AUTOCOMMIT_OFF, 0);
	if (!OdbcCheckCode (ret, connection, "SQLSetConnectAttr", SQL_HANDLE_DBC))
		return;

	HSTMT statement;
	ret = SQLAllocHandle (SQL_HANDLE_STMT, connection, &statement);
	if (!OdbcCheckCode (ret, connection, "SQLAllocHandle", SQL_HANDLE_DBC))
		return;

    if (!SimpleRetrieval ())
	    return;
	/*
	 * Try an intentional error
	 */

	ret = SQLExecDirect (statement, (UCHAR*) "select * from xyzzy", SQL_NTS);
	OdbcCheckCode (ret, statement, "SQLExecDirect w/ error");

	/*
	 * Try creating, populating, and delete a table
	 */

	ret = SQLExecDirect (statement, (UCHAR*) "drop table bar", SQL_NTS);
	ret = SQLExecDirect (statement, (UCHAR*) "drop table foo", SQL_NTS);
	ret = SQLExecDirect (statement, 
		(UCHAR*) "create table foo (f1 smallint not null primary key, c2 decimal (18,5))", SQL_NTS);
		//(UCHAR*) "create table foo (f1 numeric (8,2) not null primary key)", SQL_NTS);
	if (!OdbcCheckCode (ret, statement, "SQLExecDirect"))
		return;

	ret = SQLExecDirect (statement, 
		(UCHAR*) "create table bar (f1 smallint references foo)", SQL_NTS);
	if (!OdbcCheckCode (ret, statement, "SQLExecDirect"))
		return;

	ret = SQLExecDirect (statement, 
		//(UCHAR*) "insert into foo values (123)", SQL_NTS);
		(UCHAR*) "insert into foo values (123, 8734402384571.45)", SQL_NTS);
	if (!OdbcCheckCode (ret, statement, "SQLExecDirect"))
		return;

	ret = SQLExecDirect (statement, 
		//(UCHAR*) "insert into foo values (123)", SQL_NTS);
		(UCHAR*) "insert into foo values (456, 0.00012)", SQL_NTS);
	if (!OdbcCheckCode (ret, statement, "SQLExecDirect"))
		return;

	ret = SQLExecDirect (statement, (UCHAR*) "select * from foo", SQL_NTS);
	if (!OdbcCheckCode (ret, statement, "SQLExecDirect"))
		return;

	print.printAll();

	ret = SQLExecDirect (statement, (UCHAR*) "drop table bar", SQL_NTS);
	if (!OdbcCheckCode (ret, statement, "SQLExecDirect"))
		return;

	ret = SQLExecDirect (statement, (UCHAR*) "drop table foo", SQL_NTS);
	if (!OdbcCheckCode (ret, statement, "SQLExecDirect"))
		return;


	/*
	 * Try some meta-data retrievals
	 */

	ret = SQLTables (statement, 
						 NULL, SQL_NTS, 
						 (UCHAR*) NULL, SQL_NTS, 
						 (UCHAR*) "%", SQL_NTS,
						 (UCHAR*) "TABLE, 'VIEW'", SQL_NTS);
	if (!OdbcCheckCode (ret, statement, "SQLProcedures"))
		return;
	print.printAll();

	ret = SQLProcedures (statement, 
						 NULL, SQL_NTS, 
						 NULL, SQL_NTS, 
						 (UCHAR*) "%", SQL_NTS);
	if (!OdbcCheckCode (ret, statement, "SQLProcedures"))
		return;
	print.printAll();

	ret = SQLProcedureColumns (statement, 
						 (UCHAR*)"aBC", SQL_NTS, 
						 (UCHAR*)"DEF", SQL_NTS, 
						 (UCHAR*) "%", SQL_NTS,
						 (UCHAR*) "%", SQL_NTS);
	if (!OdbcCheckCode (ret, statement, "SQLProcedureColumns"))
		return;
	print.printAll();

	ret = SQLStatistics (statement, 
						 (UCHAR*)"%", SQL_NTS, 
						 (UCHAR*)"%", SQL_NTS, 
						 (UCHAR*) "%", SQL_NTS,
						 SQL_INDEX_ALL,
						 SQL_QUICK);
	if (!OdbcCheckCode (ret, statement, "SQLProcedureColumns"))
		return;
	print.printAll();

	
	ret = SQLGetTypeInfo (statement, SQL_ALL_TYPES);
	if (!OdbcCheckCode (ret, statement, "SQLGetTypeInfo"))
		return;
	print.printAll();

	
	ret = SQLColumns (statement, 
						 (UCHAR*)"%", SQL_NTS, 
						 (UCHAR*)"%", SQL_NTS, 
						 (UCHAR*) "EMPLOYEE", SQL_NTS,
						 (UCHAR*) "%", SQL_NTS);
	if (!OdbcCheckCode (ret, statement, "SQLProcedureColumns"))
		return;
	print.printHeaders();

	for (;;)
		{
		ret = SQLFetch (statement);
		if (ret == SQL_NO_DATA_FOUND)
			break;
		if (!OdbcCheckCode (ret, statement, "SQLFetch"))
			break;
		//print.printLine();
		short nullable;
		int retcode = SQLGetData (statement, 11, SQL_C_SHORT, &nullable, 0, NULL);
		OdbcCheckCode (retcode, statement, "SQLGetData");
		}

	/*
	 * Try storing a blob.
	 */

	ret = SQLExecDirect (statement, (UCHAR*) "drop table blobs", SQL_NTS);
	ret = SQLExecDirect (statement, (UCHAR*) "create table blobs (stuff blob)", SQL_NTS);
	if (!OdbcCheckCode (ret, statement, "SQLExecDirect"))
		return;

	char *blobString = "This is blob content.";
	ret = SQLBindParameter (statement, 1, SQL_PARAM_INPUT, 
						    SQL_C_CHAR, SQL_LONGVARBINARY, 50, 0, blobString, strlen (blobString) + 1, NULL);
	if (!OdbcCheckCode (ret, statement, "SQLBindParameter"))
		return;

	ret = SQLExecDirect (statement, (UCHAR*) "insert into blobs values (?)", SQL_NTS);
	if (!OdbcCheckCode (ret, statement, "SQLExecDirect"))
		return;

	ret = SQLFreeStmt (statement, SQL_RESET_PARAMS);
	if (!OdbcCheckCode (ret, statement, "SQLFreeStmt SQL_RESET_PARAMS"))
		return;

	ret = SQLExecDirect (statement, (UCHAR*) "select * from blobs", SQL_NTS);
	if (!OdbcCheckCode (ret, statement, "SQLExecDirect"))
		return;

	print.printAll();

	/*
	 * Try a store procedure
	 */

	long count;
	int parameter = 1;
	/***
	ret = SQLBindParameter (statement, parameter++, SQL_PARAM_INPUT, 
						    SQL_C_CHAR, SQL_CHAR, 3, 0, "623", 0, NULL);
	if (!OdbcCheckCode (ret, statement, "SQLBindParameter"))
		return;
	***/

	ret = SQLBindParameter (statement, parameter++, SQL_PARAM_OUTPUT, 
						    SQL_C_SLONG, SQL_INTEGER, 8, 0, &count, 0, NULL);
	if (!OdbcCheckCode (ret, statement, "SQLBindParameter"))
		return;

	ret = SQLExecDirect (statement, (UCHAR*) "{ call count_employees (623)}", SQL_NTS);
	if (!OdbcCheckCode (ret, statement, "SQLExecDirect"))
		return;

	printf ("\nNumber of employees: %d\n\n", count);

	/*
	 * Try a ordinary retrieval
	 */

	ret = SQLCloseCursor (statement);
	if (!OdbcCheckCode (ret, statement, "SQLCloseCursor"))
		return;

	ret = SQLSetStmtAttr (statement, SQL_ATTR_CURSOR_SCROLLABLE, (void*) SQL_SCROLLABLE, 0);
	if (!OdbcCheckCode (ret, statement, "SQLSetStmtAttr SQL_ATTR_CURSOR_SCROLLABLE, SQL_SCROLLABLE"))
		return;

	ret = SQLSetStmtAttr (statement, SQL_ATTR_CURSOR_TYPE, (void*) SQL_CURSOR_STATIC, 0);
	if (!OdbcCheckCode (ret, statement, "SQLSetStmtAttr SQL_ATTR_CURSOR_TYPE, SQL_CURSOR_STATIC"))
		return;

	ret = SQLPrepare (statement, (UCHAR*) 
				"SELECT first_name, last_name, hire_date, salary FROM EMPLOYEE "
				"WHERE FIRST_NAME=?", SQL_NTS);

	if (!OdbcCheckCode (ret, statement, "SQLPrepare"))
		return;

	SWORD	type;
	UDWORD	precision;
	SWORD	scale;
	SWORD	nullable;

	ret = SQLDescribeParam (statement, 1, &type, &precision, &scale, &nullable);
	if (!OdbcCheckCode (ret, statement, "SQLDescribeParam"))
		return;

	ret = SQLBindParameter (statement, 1, SQL_PARAM_INPUT, 
						    SQL_C_CHAR, type, precision, scale, "Robert", 0, NULL);
	if (!OdbcCheckCode (ret, statement, "SQLBindParameter"))
		return;

	ret = SQLExecute (statement);
	if (!OdbcCheckCode (ret, statement, "SQLExecute"))
		return;

	UCHAR firstName [30];
	SQLINTEGER firstNameLength;
	tagDATE_STRUCT hireDate;

	ret = SQLBindCol (statement, 1, SQL_C_CHAR, firstName, sizeof (firstName), &firstNameLength);
	if (!OdbcCheckCode (ret, statement, "SQLBindCol"))
		return;

	ret = SQLBindCol (statement, 3, SQL_C_DATE, &hireDate, sizeof (hireDate), NULL);
	if (!OdbcCheckCode (ret, statement, "SQLBindCol"))
		return;

	print.printHeaders();

	for (;;)
		{
		//SQLUINTEGER rowCount;
		//SQLUSMALLINT rowStatusArray;
		//ret = SQLFetch (statement);
		//ret = SQLExtendedFetch (statement, SQL_FETCH_FIRST, 0, &rowCount, &rowStatusArray);
		ret = SQLFetchScroll (statement, SQL_FETCH_NEXT, 0);
		if (ret == SQL_NO_DATA_FOUND)
			break;
		if (!OdbcCheckCode (ret, statement, "SQLFetch"))
			break;
		print.printLine();
		tagTIMESTAMP_STRUCT date;
		ret = SQLGetData (statement, 3, SQL_C_TIMESTAMP, &date, 0, NULL);
		OdbcCheckCode (ret, statement, "SQLGetData");
		char salary [32];
		ret = SQLGetData (statement, 4, SQL_C_CHAR, salary, sizeof (salary), NULL);
		OdbcCheckCode (ret, statement, "SQLGetData");
		}

	ret = SQLFreeHandle (SQL_HANDLE_STMT, statement);
	if (!OdbcCheckCode (ret, statement, "SQLFreeHandle (statement"))
		return;

    ret = SQLEndTran (SQL_HANDLE_DBC, connection, SQL_COMMIT);
	if (!OdbcCheckCode (ret, statement, "SQLEndTrans"))
		return;

    ret = SQLDisconnect (connection);
	if (!OdbcCheckCode (ret, connection, "SQLDisconnect", SQL_HANDLE_DBC))
		return;

	ret = SQLFreeHandle (SQL_HANDLE_DBC, connection);
	if (!OdbcCheckCode (ret, connection, "SQLFreeHandle (connection)", SQL_HANDLE_DBC))
		return;

	ret = SQLFreeHandle (SQL_HANDLE_ENV, env);
	if (!OdbcCheckCode (ret, env, "SQLFreeHandle (env)", SQL_HANDLE_ENV))
		return;


bool	SimpleRetrieval ()
	{
					/*
	 * Try a ordinary retrieval
	 */

	ret = SQLCloseCursor (statement);
	if (!OdbcCheckCode (ret, statement, "SQLCloseCursor"))
		return FALSE;

	ret = SQLSetStmtAttr (statement, SQL_ATTR_CURSOR_SCROLLABLE, (void*) SQL_SCROLLABLE, 0);
	if (!OdbcCheckCode (ret, statement, "SQLSetStmtAttr SQL_ATTR_CURSOR_SCROLLABLE, SQL_SCROLLABLE"))
		return FALSE;

	ret = SQLSetStmtAttr (statement, SQL_ATTR_CURSOR_TYPE, (void*) SQL_CURSOR_STATIC, 0);
	if (!OdbcCheckCode (ret, statement, "SQLSetStmtAttr SQL_ATTR_CURSOR_TYPE, SQL_CURSOR_STATIC"))
		return FALSE;

	ret = SQLPrepare (statement, (UCHAR*) 
				"SELECT first_name, last_name, hire_date, salary FROM EMPLOYEE "
				"WHERE FIRST_NAME=?", SQL_NTS);

	if (!OdbcCheckCode (ret, statement, "SQLPrepare"))
		return FALSE;

	SWORD	type;
	UDWORD	precision;
	SWORD	scale;
	SWORD	nullable;

	ret = SQLDescribeParam (statement, 1, &type, &precision, &scale, &nullable);
	if (!OdbcCheckCode (ret, statement, "SQLDescribeParam"))
		return FALSE;

	ret = SQLBindParameter (statement, 1, SQL_PARAM_INPUT, 
						    SQL_C_CHAR, type, precision, scale, "Robert", 0, NULL);
	if (!OdbcCheckCode (ret, statement, "SQLBindParameter"))
		return FALSE;

	ret = SQLExecute (statement);
	if (!OdbcCheckCode (ret, statement, "SQLExecute"))
		return FALSE;

	UCHAR firstName [30];
	SQLINTEGER firstNameLength;
	tagDATE_STRUCT hireDate;

	ret = SQLBindCol (statement, 1, SQL_C_CHAR, firstName, sizeof (firstName), &firstNameLength);
	if (!OdbcCheckCode (ret, statement, "SQLBindCol"))
		return FALSE;

	ret = SQLBindCol (statement, 3, SQL_C_DATE, &hireDate, sizeof (hireDate), NULL);
	if (!OdbcCheckCode (ret, statement, "SQLBindCol"))
		return FALSE;

	print.printHeaders();

	for (;;)
		{
		//SQLUINTEGER rowCount;
		//SQLUSMALLINT rowStatusArray;
		//ret = SQLFetch (statement);
		//ret = SQLExtendedFetch (statement, SQL_FETCH_FIRST, 0, &rowCount, &rowStatusArray);
		ret = SQLFetchScroll (statement, SQL_FETCH_NEXT, 0);
		if (ret == SQL_NO_DATA_FOUND)
			break;
		if (!OdbcCheckCode (ret, statement, "SQLFetch"))
			break;
		print.printLine();
		tagTIMESTAMP_STRUCT date;
		ret = SQLGetData (statement, 3, SQL_C_TIMESTAMP, &date, 0, NULL);
		OdbcCheckCode (ret, statement, "SQLGetData");
		char salary [32];
		ret = SQLGetData (statement, 4, SQL_C_CHAR, salary, sizeof (salary), NULL);
		OdbcCheckCode (ret, statement, "SQLGetData");
		}

	ret = SQLFreeHandle (SQL_HANDLE_STMT, statement);
	if (!OdbcCheckCode (ret, statement, "SQLFreeHandle (statement"))
		return FALSE;

	Print print (statement);
    return TRUE;
    }
}


void test2 ()



}
