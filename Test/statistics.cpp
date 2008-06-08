#include <stdio.h>
#include "Odbc.h"
#include "Print.h"


static HENV env;
static SQLHWND hWnd;

static HDBC testConnect (const char *);
static void test4 (HDBC);
static void testDisconnect (HDBC);


main (int argc, const char **argv)
    {
    const char **end = argv + argc;
//    const char *connectString = "ODBC;DSN=FireBirdOdbc;DRIVER=OdbcJdbc;ROLE=cinnamon";
    const char *connectString = "DSN=FireBirdOdbc";

int n;
const char  *buf = "2000 \"c:\\harrison\\\"";
int size;
char dir_name [120];

n = sscanf(buf,  "%ld \"%[^\"]", &size, dir_name);
if (n == 2)
	printf ("%ld, %s\n", size, dir_name);


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
//	test1 (connection);
//	test2 (connection);
//	test3 (connection);
	test4 (connection);
//	test5 (connection);
//	test6 (connection);
//	test7 (connection);
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





void test4 (HDBC connection)
    {
    HSTMT statement;
	short	count, numberColumns;
	Column *columns;
	char	* buffer;

    int ret = SQLAllocHandle (SQL_HANDLE_STMT, connection, &statement);
    if (!OdbcCheckCode (ret, connection, "SQLAllocHandle", SQL_HANDLE_DBC))
        return;

    Print print (statement);



    ret = SQLStatistics (statement, 
                         (UCHAR*)"%", SQL_NTS, 
                         (UCHAR*)"%", SQL_NTS, 
                         (UCHAR*) "%", SQL_NTS,
                         SQL_INDEX_ALL,
                         SQL_QUICK);
    if (!OdbcCheckCode (ret, statement, "SQLStatistics"))
        return;
	
	RETCODE retcode = SQLNumResultCols (statement, &count);

	if (!OdbcCheckCode (retcode, statement, "SQLNumResultCols"))
		return;

	numberColumns = count;
	columns = new Column [numberColumns];
	Column *column = columns;
	int offset = 0;

	for (int n = 1; n <= numberColumns; ++n, ++column)
		{
		SWORD nameLength,
		retcode = SQLDescribeCol (statement, n, 
					  (UCHAR*) column->name, sizeof (column->name), &nameLength,
					  &column->sqlType, 
					  &column->precision, 
					  &column->scale, 
					  &column->nullable);
		if (!OdbcCheckCode (retcode, statement, "SQLDescribeCol"))
			return;
		if (column->precision > 350)
			column->precision = 350;
		column->offset = offset;
		column->headerLength = strlen (column->name);
		column->length = ((int) column->precision > column->headerLength) ? 
				column->precision : column->headerLength;
		offset += column->length + 1;
		}

	buffer = new char [offset + 1];

	char p1 [3];
	long p1_len;

	retcode = SQLBindCol (statement, 1, 
			SQL_C_CHAR,&p1, sizeof (p1), &p1_len);
	if (!OdbcCheckCode (retcode, statement, "SQLBindCol1"))
			return;

	char p2 [3];
	long p2_len;

	retcode = SQLBindCol (statement, 2, 
			SQL_C_CHAR,&p2, sizeof (p2), &p2_len);
	if (!OdbcCheckCode (retcode, statement, "SQLDescribeCol"))
			return;

	char p3 [33];
	long p3_len;

	retcode = SQLBindCol (statement, 3, 
			SQL_C_CHAR,&p3, sizeof (p3), &p3_len);
	if (!OdbcCheckCode (retcode, statement, "SQLDescribeCol"))
			return;


	short p4;
	long p4_len;

	retcode = SQLBindCol (statement, 4, 
			SQL_C_SHORT,&p4, sizeof (p4), &p4_len);
	if (!OdbcCheckCode (retcode, statement, "SQLDescribeCol"))
			return;

	char p5 [3];
	long p5_len;

	retcode = SQLBindCol (statement, 5, 
			SQL_C_CHAR,&p5, sizeof (p5), &p5_len);
	if (!OdbcCheckCode (retcode, statement, "SQLDescribeCol"))
			return;


	char p6 [33];
	long p6_len;

	retcode = SQLBindCol (statement, 6, 
			SQL_C_CHAR,&p6, sizeof (p6), &p6_len);
	if (!OdbcCheckCode (retcode, statement, "SQLDescribeCol"))
			return;


	long p7;
	long p7_len;

	retcode = SQLBindCol (statement, 7, 
			SQL_C_LONG,&p7, sizeof (p7), &p7_len);
	if (!OdbcCheckCode (retcode, statement, "SQLDescribeCol"))
			return;

	short p8;
	long p8_len;

	retcode = SQLBindCol (statement, 8, 
			SQL_C_SHORT,&p8, sizeof (p8), &p8_len);
	if (!OdbcCheckCode (retcode, statement, "SQLDescribeCol"))
			return;


	char p9 [33];
	long p9_len;

	retcode = SQLBindCol (statement, 9, 
			SQL_C_CHAR,&p9, sizeof (p9), &p9_len);
	if (!OdbcCheckCode (retcode, statement, "SQLDescribeCol"))
			return;


	short p10;
	long p10_len;

	retcode = SQLBindCol (statement, 10, 
			SQL_C_SHORT,&p10, sizeof (p10), &p10_len);
	if (!OdbcCheckCode (retcode, statement, "SQLDescribeCol"))
			return;

	double p11;
	long p11_len;

	retcode = SQLBindCol (statement, 11, 
			SQL_C_DOUBLE,&p11, sizeof (p11), &p11_len);
	if (!OdbcCheckCode (retcode, statement, "SQLbind11"))
			return;

	char p12 [3];
	long p12_len;

	retcode = SQLBindCol (statement, 12, 
			SQL_C_CHAR,&p12, sizeof (p12), &p12_len);
	if (!OdbcCheckCode (retcode, statement, "SQLbind12"))
			return;



	char p13 [3];
	long p13_len;

	retcode = SQLBindCol (statement, 13, 
			SQL_C_CHAR,&p13, sizeof (p13), &p13_len);
	if (!OdbcCheckCode (retcode, statement, "SQLbind13"))
			return;

	retcode = SQLFetch (statement);
	if (!OdbcCheckCode (retcode, statement, "SQLFetch"))
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
