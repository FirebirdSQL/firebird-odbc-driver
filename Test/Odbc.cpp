/*
 *	PROGRAM:		Subschema Upgrade Utility
 *	MODULE:			Table.cpp
 *	DESCRIPTION:	Virtual Table class
 *
 * copyright (c) 1997 by James A. Starkey for IBPhoenix.
 */

#ifdef _WIN32
#include "stdafx.h"
#endif

#include <stdio.h>
#include "JString.h"
#include "Odbc.h"

int		sw_verbose;
int		sw_noUpdate;

FILE	*logFile;

int OdbcCheckCode (int retcode, SQLHANDLE handle, const char *string, int handleType)
{
/**************************************
 *
 *		O d b c C h e c k C o d e
 *
 **************************************
 *
 * Functional description
 *		Test driver.  Do something pretty ad hoc.
 *
 **************************************/
UCHAR	sqlState [128], text [SQL_MAX_MESSAGE_LENGTH];
SDWORD	nativeCode;
SWORD	textLength;
int		ok = false;
char	temp [256];

if (retcode == SQL_SUCCESS)
    return true;

JString message = string;

if (retcode == SQL_SUCCESS_WITH_INFO)
	message += " succeeded with information";
else
	message += " failed";

for (int n = 1; n < 10; ++n)
	{
	text [0] = 0;
	sqlState [0] = 0;
	int ret = SQLGetDiagRec (handleType, handle, n, sqlState, &nativeCode, 
					  text, sizeof (text) -1, &textLength);
	if (ret < 0 || ret == SQL_NO_DATA_FOUND)
		break;
	sprintf (temp, "\n%s (%s)", sqlState, text);
	message += temp;
	if (!strcmp ((char*)sqlState, "22005"))
		ok = true;
	else if (!strcmp ((char*)sqlState, "22008"))
		ok = true;
	else if (!strcmp ((char*)sqlState, "NA000"))
		ok = true;
	//break;
	}


if (logFile)
    fflush (logFile);
printf ("%s\n", message.getString());

#ifdef _WIN32
OutputDebugString (message);
OutputDebugString ("\n");
#endif

if (retcode == SQL_SUCCESS_WITH_INFO)
	return true;

return ok;
}
