#include <stdio.h>
#include "Odbc.h"
#include "Print.h"


static HENV env;
static SQLHWND hWnd;

static HDBC testConnect (const char *);
static void test1 (HDBC);
static void test2 (HDBC);
static void test3 (HDBC);
static void test4 (HDBC);
static void test5 (HDBC);
static void test6 (HDBC);
static void test7 (HDBC);
static void test8 (HDBC);
static void testDisconnect (HDBC);


int main (int argc, const char **argv)
    {
    const char **end = argv + argc;
//    const char *connectString = "ODBC;DSN=FireBirdOdbc;DRIVER=OdbcJdbc;ROLE=cinnamon";
    const char *connectString = "DSN=FireBirdOdbc";

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
    if (HDBC connection = testConnect (connectString))
	{
	test1 (connection);
//	test2 (connection);
//	test3 (connection);
//	test4 (connection);
//	test5 (connection);
//	test6 (connection);
	test7 (connection);
//	test8 (connection);

	testDisconnect (connection);
	}

    return 0;
    }



HDBC testConnect (const char *connectString)
    {
    int ret = SQLAllocHandle (SQL_HANDLE_ENV, NULL, &env);
    ret = SQLSetEnvAttr (env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3, SQL_IS_UINTEGER);
    if (!OdbcCheckCode (ret, env, "SQLSetEnvAttr", SQL_HANDLE_ENV))
        return NULL;

    HDBC connection;
    ret = SQLAllocHandle (SQL_HANDLE_DBC, env, &connection);
    if (!OdbcCheckCode (ret, env, "SQLAllocHandle", SQL_HANDLE_ENV))
        return NULL;

    //ret = SQLAllocConnect (env, &connection);
    ret = SQLSetConnectAttr (connection, SQL_ATTR_ODBC_CURSORS, (SQLPOINTER) SQL_CUR_USE_ODBC, 0);
    if (!OdbcCheckCode (ret, connection, "SQLConnectAttr", SQL_HANDLE_DBC))
        return NULL;
    
    UCHAR buffer [128];
    SWORD bufferLength;
    ret = SQLDriverConnect (connection, hWnd, 
                            (UCHAR*) connectString, SQL_NTS,
                            buffer, sizeof (buffer), &bufferLength,
                            SQL_DRIVER_NOPROMPT);

    if (!OdbcCheckCode (ret, connection, "SQLDriverConnect", SQL_HANDLE_DBC))
        return NULL;

//    ret = SQLConnect (connection, (UCHAR*) "FireBird", SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS);
    if (!OdbcCheckCode (ret, connection, "SQLConnect", SQL_HANDLE_DBC))
       return NULL;

    ret = SQLSetConnectAttr (connection, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER) SQL_AUTOCOMMIT_OFF, 0);
    if (!OdbcCheckCode (ret, connection, "SQLSetConnectAttr", SQL_HANDLE_DBC))
        return NULL;
    
    return connection;

    }



void test1 (HDBC connection)
    {
    HSTMT statement;
    int ret = SQLAllocHandle (SQL_HANDLE_STMT, connection, &statement);
    if (!OdbcCheckCode (ret, connection, "SQLAllocHandle", SQL_HANDLE_DBC))
        return;

    Print print (statement);
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

    SWORD   type;
    UDWORD  precision;
    SWORD   scale;
    SWORD   nullable;

    ret = SQLDescribeParam (statement, 1, &type, &precision, &scale, &nullable);
    if (!OdbcCheckCode (ret, statement, "SQLDescribeParam"))
        return;

    char  robert[] = "Robert";
    ret = SQLBindParameter (statement, 1, SQL_PARAM_INPUT, 
                            SQL_C_CHAR, type, precision, scale, robert, 0, NULL);
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
        ret = SQLFetch (statement);
        //ret = SQLExtendedFetch (statement, SQL_FETCH_FIRST, 0, &rowCount, &rowStatusArray);
        //ret = SQLFetchScroll (statement, SQL_FETCH_NEXT, 0);
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

    }

void test2 (HDBC connection)
    {
    HSTMT statement;
    int ret = SQLAllocHandle (SQL_HANDLE_STMT, connection, &statement);
    if (!OdbcCheckCode (ret, connection, "SQLAllocHandle", SQL_HANDLE_DBC))
        return;

    Print print (statement);

    /*
     * Try an intentional error
     */

    ret = SQLExecDirect (statement, (UCHAR*) "select * from xyzzy", SQL_NTS);
    OdbcCheckCode (ret, statement, "SQLExecDirect w/ error");

    }

void test3 (HDBC connection)
    {
    HSTMT statement;
    int ret = SQLAllocHandle (SQL_HANDLE_STMT, connection, &statement);
    if (!OdbcCheckCode (ret, connection, "SQLAllocHandle", SQL_HANDLE_DBC))
        return;

    Print print (statement);


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


    ret = SQLExecDirect (statement, (UCHAR*) "drop table bar", SQL_NTS);
    if (!OdbcCheckCode (ret, statement, "SQLExecDirect"))
    return;

    ret = SQLExecDirect (statement, (UCHAR*) "drop table foo", SQL_NTS);
    if (!OdbcCheckCode (ret, statement, "SQLExecDirect"))
    return;

    }

void test4 (HDBC connection)
    {
    HSTMT statement;
    int ret = SQLAllocHandle (SQL_HANDLE_STMT, connection, &statement);
    if (!OdbcCheckCode (ret, connection, "SQLAllocHandle", SQL_HANDLE_DBC))
        return;

    Print print (statement);

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
    if (!OdbcCheckCode (ret, statement, "SQLStatistics"))
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


}

void test5 (HDBC connection)
    {
    HSTMT statement;
    int ret = SQLAllocHandle (SQL_HANDLE_STMT, connection, &statement);
    if (!OdbcCheckCode (ret, connection, "SQLAllocHandle", SQL_HANDLE_DBC))
        return;

    Print print (statement);
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

    }

void test6 (HDBC connection)
    {
    HSTMT statement;
    int ret = SQLAllocHandle (SQL_HANDLE_STMT, connection, &statement);
    if (!OdbcCheckCode (ret, connection, "SQLAllocHandle", SQL_HANDLE_DBC))
        return;

    Print print (statement);
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
    }

void test7 (HDBC connection)
    {
    HSTMT statement;
    int ret = SQLAllocHandle (SQL_HANDLE_STMT, connection, &statement);
    if (!OdbcCheckCode (ret, connection, "SQLAllocHandle", SQL_HANDLE_DBC))
        return;

    Print print (statement);
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

    ret = SQLPrepare  (statement, (UCHAR*) 
	"SELECT foodate, foots, footime, food from foo where foodate > '31 DEC 69' order by foodate", 
	    SQL_NTS);

    if (!OdbcCheckCode (ret, statement, "SQLPrepare"))
        return;

    ret = SQLExecute (statement);
    if (!OdbcCheckCode (ret, statement, "SQLExecute"))
        return;

    tagTIMESTAMP_STRUCT fooDate;

    ret = SQLBindCol (statement, 1, SQL_C_TIMESTAMP, &fooDate, sizeof (fooDate), NULL);
    if (!OdbcCheckCode (ret, statement, "SQLBindCol"))
        return;
    
    tagTIMESTAMP_STRUCT fooTS;

    ret = SQLBindCol (statement, 2, SQL_C_TIMESTAMP, &fooTS, sizeof (fooTS), NULL);
    if (!OdbcCheckCode (ret, statement, "SQLBindCol"))
        return;

    tagTIME_STRUCT fooTime;

    ret = SQLBindCol (statement, 3, SQL_C_TIME, &fooTime, sizeof (fooTime), NULL);
    if (!OdbcCheckCode (ret, statement, "SQLBindCol"))
        return;

    tagDATE_STRUCT fooD;

    ret = SQLBindCol (statement, 4, SQL_C_DATE, &fooD, sizeof (fooD), NULL);
    if (!OdbcCheckCode (ret, statement, "SQLBindCol"))
        return;

    print.printHeaders();

    for (;;)
        {
        //SQLUINTEGER rowCount;
        //SQLUSMALLINT rowStatusArray;
        ret = SQLFetch (statement);
        //ret = SQLExtendedFetch (statement, SQL_FETCH_FIRST, 0, &rowCount, &rowStatusArray);
        //ret = SQLFetchScroll (statement, SQL_FETCH_NEXT, 0);
        if (ret == SQL_NO_DATA_FOUND)
            break;
        if (!OdbcCheckCode (ret, statement, "SQLFetch"))
            break;
        print.printLine();

	tagTIMESTAMP_STRUCT datetime1;
        tagTIMESTAMP_STRUCT datetime2;
	tagDATE_STRUCT date;
	tagTIME_STRUCT time;

        ret = SQLGetData (statement, 0, SQL_C_DATE, &datetime1, 0, NULL);
        OdbcCheckCode (ret, statement, "SQLGetData");
        ret = SQLGetData (statement, 2, SQL_C_TIMESTAMP, &datetime2, 0, NULL);
        OdbcCheckCode (ret, statement, "SQLGetData");
        ret = SQLGetData (statement, 3, SQL_C_TIME, &time, 0, NULL);
        OdbcCheckCode (ret, statement, "SQLGetData");
        ret = SQLGetData (statement, 4, SQL_C_DATE, &date, 0, NULL);
        OdbcCheckCode (ret, statement, "SQLGetData");
      }

    ret = SQLFreeHandle (SQL_HANDLE_STMT, statement);
    if (!OdbcCheckCode (ret, statement, "SQLFreeHandle (statement"))
        return;

    }



void test8 (HDBC connection)
    {
    HSTMT statement;
    int ret = SQLAllocHandle (SQL_HANDLE_STMT, connection, &statement);
    if (!OdbcCheckCode (ret, connection, "SQLAllocHandle", SQL_HANDLE_DBC))
        return;

    Print print (statement);
    /*
     * Try a ordinary retrieval
     */

    ret = SQLCloseCursor (statement);
    if (!OdbcCheckCode (ret, statement, "SQLCloseCursor"))
        return;


    ret = SQLPrepare  (statement, (UCHAR*) 
          "INSERT INTO foo (foodate, foots, food, footime) \
	  values ('24 Feb 2001', '24 Feb 2001 18:05:04', '24 Feb 2001', '18:05:04' ) ", SQL_NTS);

    if (!OdbcCheckCode (ret, statement, "SQLPrepare"))
        return;

    ret = SQLExecute (statement);
    if (!OdbcCheckCode (ret, statement, "SQLExecute"))
        return;


    ret = SQLFreeHandle (SQL_HANDLE_STMT, statement);
    if (!OdbcCheckCode (ret, statement, "SQLFreeHandle (statement"))
        return;

    }



void testDisconnect (HDBC connection)
    {

    int ret = SQLEndTran (SQL_HANDLE_DBC, connection, SQL_COMMIT);
	if (!OdbcCheckCode (ret, connection, "SQLEndTrans"))
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


    }
