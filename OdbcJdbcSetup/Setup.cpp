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
 *	2002-06-08	Setup.cpp
 *				Added changes suggested by C. G. Alvarez to 
 *				correctly locate the driver if already 
 *				installed and to correctly report any errors.
 *
 *  2002-04-30	Added 'role' fix from Paul Schmidt		(PCR)
 *
 */

//--------------------------------------------------
// CONFDSN: trivial configure DSN for the trival
//            sample driver.
//
// Copyright (C) 1997 by Microsoft Corporation.
//--------------------------------------------------
// This doesn't do anything interesting, the sample
// driver is really for testing the setup sample
//--------------------------------------------------

#include "OdbcJdbcSetup.h"
#include <odbcinst.h>
#include "DsnDialog.h"
#include "Setup.h"
#include "../SetupAttributes.h"

const char *driverInfo =
	DRIVER_FULL_NAME"\0"
	"Driver=OdbcJdbc.dll\0"
	"Setup=OdbcJdbcSetup.dll\0"
	"FileExtns=*.fdb,*.gdb\0"
	"APILevel=1\0"
	"ConnectFunctions=YYY\0"
	"FileUsage=0\0"
	"DriverODBCVer=03.00\0"
	"SQLLevel=1\0"
	"\0";

static const char *fileNames [] = {
	"OdbcJdbcSetup.dll",
	"OdbcJdbc.dll",
	"IscDbc.dll",
	NULL
	};

static const char *drivers [] = { "IscDbc", NULL };
extern HINSTANCE m_hInstance;

/*
 *	To debug the control panel applet 
 *	1/ set the active project to OdbcJdbcSetup
 *	2/ set the executable to { full path to } rundll32.exe
 *	3/ Pass this command as the program argument:
 *		shell32.dll,Control_RunDLL odbccp32.cpl,,1
 *	4/ Set breakpoints as desired and hit the run button
 */
#ifdef __BORLANDC__
extern "C" __declspec( dllexport ) BOOL INSTAPI ConfigDSN(HWND		hWnd,
#else
BOOL INSTAPI ConfigDSN(HWND		hWnd,
#endif
			   WORD		fRequest,
			   LPCSTR	lpszDriver,
			   LPCSTR	lpszAttributes)
{
	Setup setup (hWnd, lpszDriver, lpszAttributes);
	switch (fRequest)
	{
	case ODBC_CONFIG_DSN:
		setup.configDsn();
		break;

	case ODBC_ADD_DSN:
		setup.addDsn();
		break;

	case ODBC_REMOVE_DSN:
		setup.removeDsn();
		break;
	}

	return true;
}

/*
 *	Registration can be performed with the following command:
 *
 *		regsvr32 odbcjdbcsetup.dll
 *
 *	To debug registration the project settings to call regsvr32.exe
 *  with the full path.
 *
 *  Use 
 *		../debug/odbcjdbcsetup.dll
 *
 *  as the program argument
 *
 */

extern "C" __declspec( dllexport ) int DllRegisterServer (void)
{
	char fileName [256];
	JString msg;

	GetModuleFileName (m_hInstance, fileName, sizeof (fileName));

	char pathOut [256];
	WORD length = sizeof (pathOut);
	DWORD useCount;

	if (!SQLInstallDriverEx (
			driverInfo,
			NULL, //fileName,
			pathOut,
			sizeof (pathOut),
			&length,
			ODBC_INSTALL_COMPLETE,
			&useCount))
	{
		char message [SQL_MAX_MESSAGE_LENGTH];
        WORD	errCodeIn = 1;
		DWORD *	errCodeOut = 0L;
		
		SQLInstallerError(errCodeIn, errCodeOut, message, sizeof (message) - 1,
			NULL);
		
		msg.Format ("Install Driver Complete (%s, %s) failed with %d\n%s\n", 
						fileName, pathOut, errCodeOut, message);
        MessageBox(NULL, (const char*)msg,DRIVER_NAME, MB_ICONSTOP|MB_OK);
		return FALSE;
	}


	char *path = pathOut + strlen (pathOut);
	if (path != strrchr (pathOut, '\\') + 1)
		*path++ = '\\';
	char *tail = strrchr (fileName, '\\') + 1;

	for (const char **ptr = fileNames; *ptr; ++ptr)
	{
		strcpy (path, *ptr);
		strcpy (tail, *ptr);
		if (!CopyFile (fileName, pathOut, false))
		{
			DWORD messageId = GetLastError();
			char temp[256];
			if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, messageId,
					0, temp, sizeof(temp), NULL))
			{
				msg.Format ("Format message failed %d\n", GetLastError());
				MessageBox(NULL, (const char*)msg,DRIVER_NAME, MB_ICONSTOP|MB_OK);
				return FALSE;
			}

			msg.Format ("CopyFile (%s, %s) failed with %d\n%s\n", 
						fileName, pathOut, messageId, temp);
	        MessageBox(NULL, (const char*)msg,DRIVER_NAME, MB_ICONSTOP|MB_OK);
			return FALSE;
		}
	}


	if (!SQLConfigDriver (
			NULL,
			ODBC_INSTALL_DRIVER,
//			"OdbcJdbc",
			"Firebird/InterBase(r) driver", 
			NULL,
			"Firebird/InterBase(r) driver was installed successfully",
			64,
			NULL))

/*		{
		msg.Format ("Config Install (%s, %s) failed with %d\n", 
						fileName, pathOut, GetLastError());
		AfxMessageBox (msg);
		return FALSE;
		}
*/
/*
 * The original code uses GetLastError() 
 * but this doesn't report the true error.
 */
	{
        char message [SQL_MAX_MESSAGE_LENGTH];
        WORD        errCodeIn = 1;
        DWORD *    errCodeOut = 0L;

        SQLInstallerError(errCodeIn, errCodeOut, message, sizeof (message) - 1,
            NULL);

        msg.Format ("Config Install (%s, %s) failed with %d\n%s\n",
                        fileName, pathOut, errCodeOut, message);

        MessageBox(NULL, (const char*)msg,DRIVER_NAME, MB_ICONSTOP|MB_OK);

        return FALSE;
	} 	

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Setup Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Setup::Setup(HWND windowHandle, const char *drvr, const char *attr)
{
	hWnd = windowHandle;
	jdbcDriver = drivers [0];

	if (drvr)
		driver = drvr;

	if (attr)
	{
		//MessageBox (hWnd, attr, "Attributes", 0);
		attributes = attr;
		dsn = getAttribute (SETUP_DSN);
		readAttributes();
	}
}

Setup::~Setup()
{

}

void Setup::configDsn()
{
	configureDialog();
}

void Setup::addDsn()
{
	configureDialog();
}

void Setup::removeDsn()
{
	SQLRemoveDSNFromIni (dsn);
}

JString Setup::getAttribute(const char * attribute)
{
	const char *p;
	int count = strlen (attribute);

	for (p = attributes; *p; ++p)
	{
		if (*p == *attribute && !strncmp (p, attribute, count) && p [count] == '=')
		{
			const char *q;
			p += count + 1;
			for (q = p; *q && *q != ';'; ++q)
				;
			return JString (p, q - p);
		}
		while (*p && *p++ != ';')
			;
	}

	return JString();
}

bool Setup::configureDialog()
{
	if (jdbcDriver == "")
		jdbcDriver = drivers [0];

	CDsnDialog dialog (drivers);
	dialog.m_name = dsn;
	dialog.m_database = dbName;
	dialog.m_user = user;
	dialog.m_password = password;
	dialog.m_driver = jdbcDriver;
	dialog.m_role = role;

	if(*(const char*)readonlyTpb=='Y')	dialog.m_readonly=TRUE;
	else dialog.m_readonly=FALSE;

	if(*(const char*)nowaitTpb=='Y')	dialog.m_nowait=TRUE;
	else dialog.m_nowait=FALSE;

	if (dialog.DoModal() != IDOK)
		return false;

	dsn = dialog.m_name;
	dbName = dialog.m_database;
	user = dialog.m_user;
	password = dialog.m_password;
	jdbcDriver = dialog.m_driver;
	role = dialog.m_role;

	if(dialog.m_readonly)readonlyTpb="Y";
	else readonlyTpb="N";

	if(dialog.m_nowait)nowaitTpb="Y";
	else nowaitTpb="N";

	SQLWriteDSNToIni(dialog.m_name, driver);
	writeAttributes();

	return true;
}

void Setup::writeAttributes()
{
	writeAttribute (SETUP_DBNAME, dbName);
	writeAttribute (SETUP_USER, user);
	writeAttribute (SETUP_PASSWORD, password);
	writeAttribute (SETUP_ROLE, role);
	writeAttribute (SETUP_JDBC_DRIVER, jdbcDriver);
	writeAttribute (SETUP_READONLY_TPB, readonlyTpb);
	writeAttribute (SETUP_NOWAIT_TPB, nowaitTpb);
}

void Setup::readAttributes()
{
	dbName = readAttribute (SETUP_DBNAME);	
	user = readAttribute (SETUP_USER);	
	password = readAttribute (SETUP_PASSWORD);
	jdbcDriver = readAttribute (SETUP_JDBC_DRIVER);
	role = readAttribute (SETUP_ROLE);
	readonlyTpb = readAttribute (SETUP_READONLY_TPB);
	nowaitTpb = readAttribute (SETUP_NOWAIT_TPB);
}

void Setup::writeAttribute(const char * attribute, const char * value)
{
    SQLWritePrivateProfileString(dsn, attribute, value, "ODBC.INI");
}

JString Setup::readAttribute (const char * attribute)
{
	char buffer [256];

	int ret = SQLGetPrivateProfileString (dsn, attribute, "", buffer, sizeof (buffer), "ODBC.INI");

	return JString (buffer, ret);
}
