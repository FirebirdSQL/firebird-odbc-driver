// Print.cpp: implementation of the Print class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "Odbc.h"
#include "Print.h"

#define MAX_COLUMN_LENGTH	26
#define MAX(a,b)		((a) >= (b) ? (a) : (b))
#define MIN(a,b)		((a) <= (b) ? (a) : (b))

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Print::Print(HSTMT statementHandle)
{
	statement = statementHandle;
	columns = NULL;
	buffer = NULL;
}

Print::~Print()
{
	reset();
}

void Print::printHeaders()
{
	getDescription();

	char *p = buffer;
	Column *column, *end = columns + numberColumns;

	for (column = columns; column < end; ++column)
		{
		char *start = buffer + column->offset + (column->length - column->headerLength) / 2;
		while (p < start)
			*p++ = ' ';
		for (const char *q = column->name; *q;)
			*p++ = *q++;
		}

	*p = 0;
	p = buffer;
	printf ("%s\n", buffer);

	for (column = columns; column < end; ++column)
		{
		char *start = buffer + column->offset;
		while (p < start)
			*p++ = ' ';
		for (int n = 0; n < column->length; ++n)
			*p++ = '=';
		}

	*p = 0;
	printf ("%s\n\n", buffer);
}

void Print::printLine()
{
	char *p = buffer;
	Column *column = columns;

	for (int n = 1; n <= numberColumns; ++n, ++column)
		{
		char *start = buffer + column->offset + (column->length - column->precision) / 2;
		while (p < start)
			*p++ = ' ';
		SDWORD length;
		*p = 0;
		int retcode = SQLGetData (statement, n, SQL_C_CHAR, p, column->precision + 1, &length);
		if (!OdbcCheckCode (retcode, statement, "SQLGetData"))
			return;
		while (*p)
			++p;
		}

	*p = 0;
	printf ("%s\n", buffer);
}

void Print::getDescription()
{
	reset();
	SWORD	count;

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
		if (column->precision > MAX_COLUMN_LENGTH)
			column->precision = MAX_COLUMN_LENGTH;
		column->offset = offset;
		column->headerLength = strlen (column->name);
		column->length = MAX ((int) column->precision, column->headerLength);
		offset += column->length + 1;
		}

	buffer = new char [offset + 1];
}

void Print::reset()
{
	if (columns)
		{
		delete [] columns;
		columns = NULL;
		}

	if (buffer)
		{
		delete [] buffer;
		buffer = NULL;
		}
}

void Print::skip()
{
	printf ("\n");
}

void Print::execute()
{
	int ret = SQLExecute (statement);

	if (!OdbcCheckCode (ret, statement, "SQLExecute"))
		return;

	printAll();
}

void Print::printAll()
{
	printHeaders();

	for (;;)
		{
		int retcode = SQLFetch (statement);
		if (retcode == SQL_NO_DATA_FOUND)
			break;
		if (!OdbcCheckCode (retcode, statement, "SQLFetch"))
			break;
		printLine();
		}
}
