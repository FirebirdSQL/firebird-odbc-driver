#include <stdio.h>
//#include <windows.h>
#include "Odbc.h"
#include <ODBCINST.h>
#include "Database.h"
#include "Print.h"

#define DRIVER		"JimOdbc\0"\
					"Driver=d:\\OdbcJdbc\\Debug\\OdbcJdbc.dll\0"\
	                 "Setup=d:\\OdbcJdbc\\Debug\\OdbcJdbcSetup.dll\0"\
					"FileExtns=*.gdb\0"\
					"\0"


#define PATH		"d:\\OdbcJdbc\\Debug"

static void install();
static void test1 (const char *connectString);
static void test2 (const char *connectString);

/***
class Interface
    {
	public:
	virtual void method () = 0;
	};

class Implementation
    {
	public:
	virtual void method ()
		{
		printf ("Eat a rock\n");
		}
	};

class Child : public Interface, public Implementation
	{
	public:
	using Implementation::method;
	};
***/

main (int argc, const char **argv)
{
	//Child child;
	//child.method();

	const char **end = argv + argc;
	//_CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF);
	//const char *connectString = "ODBC;DSN=Fred;DRIVER=JimOdbc";
	const char *connectString = "ODBC;DSN=Employees;DRIVER=JimOdbc";

	for (++argv; argv < end;)
		{
		const char *p = *argv++;
		if (p [0] == '-')
			switch (p [1])
				{
				case 'i':
					install();
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
	//test2 (connectString);

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

	ret = SQLConnect (connection, (UCHAR*) "Employees", SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS);
	if (!OdbcCheckCode (ret, connection, "SQLConnect", SQL_HANDLE_DBC))
		return;

	HSTMT statement;
	ret = SQLAllocHandle (SQL_HANDLE_STMT, connection, &statement);
	if (!OdbcCheckCode (ret, connection, "SQLAllocHandle", SQL_HANDLE_DBC))
		return;
	Print print (statement);

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


	ret = SQLPrepare (statement, (UCHAR*) 
				"SELECT first_name, last_name, hire_date FROM EMPLOYEE "
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

	print.printHeaders();

	for (;;)
		{
		ret = SQLFetch (statement);
		if (!OdbcCheckCode (ret, statement, "SQLExecute"))
			break;
		print.printLine();
		tagTIMESTAMP_STRUCT date;
		int retcode = SQLGetData (statement, 3, SQL_C_TIMESTAMP, &date, 0, NULL);
		OdbcCheckCode (retcode, statement, "SQLGetData");
		}

	ret = SQLFreeHandle (SQL_HANDLE_STMT, statement);
}


void test2 (const char *connectString)
{
	Database database;
	database.OpenEx (connectString, CDatabase::noOdbcDialog);
	database.getTables();
}


void install()
{
	char pathOut [256];
	WORD length;
	DWORD usageCount;

	BOOL ret = SQLInstallDriverEx (
			DRIVER, 
			PATH, pathOut, sizeof (pathOut), &length,
			ODBC_INSTALL_COMPLETE,
			&usageCount);
	if (!ret)
		{
		DWORD errorCode;
		char string [256];
		WORD length;
		SQLInstallerError (1, &errorCode, string, sizeof (string), &length);
		printf ("Odbc error %d: %s\n", errorCode, string);
		}
}