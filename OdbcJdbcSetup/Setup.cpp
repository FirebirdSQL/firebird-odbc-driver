/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl
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

#include "OdbcJdbcSetup.h"
#include <odbcinst.h>
#include "DsnDialog.h"
#include "Setup.h"
#include "../SetupAttributes.h"
#include "../SecurityPassword.h"

extern HINSTANCE m_hInstance;

namespace OdbcJdbcSetupLibrary {

using namespace classSecurityPassword;

#ifdef _WIN32
#define strncasecmp		strnicmp
#endif

#define ISLOWER(c)			((c) >= 'a' && (c) <= 'z')
#define UPPER(c)			((ISLOWER (c)) ? (c) - 'a' + 'A' : (c))
#define IS_END_TOKEN(c)		((c) == '\n' || (c) == '\r')
#define IS_CHECK_YES(c)		((c) == 'Y' || (c) == '1')
#define IS_CHECK_NO(c)		((c) == 'N' || (c) == '0')

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
	"OdbcJdbc.chm",
	NULL
	};

static const char *drivers [] = { "IscDbc", NULL };
static const char *charsets []= 
{ 
	"NONE", "ASCII", "BIG_5", "CYRL", "DOS437", "DOS850", "DOS852", "DOS857", "DOS860",
	"DOS861", "DOS863", "DOS865", "DOS866", "EUCJ_0208", "GB_2312", "ISO8859_1", 
	"ISO8859_2", "KSC_5601", "OCTETS", "SJIS_0208", "UNICODE_FSS", 
	"WIN1250", "WIN1251", "WIN1252", "WIN1253", "WIN1254", NULL
};

void MessageBoxError(char * stageExecuted, char * pathFile);
void MessageBoxInstallerError(char * stageExecuted, char * pathOut);
bool CopyFile(char * sourceFile, char * destFile);

/*
 *	To debug the control panel applet 
 *	1/ set the active project to OdbcJdbcSetup
 *	2/ set the executable to { full path to } rundll32.exe
 *	3/ Pass this command as the program argument:
 *		shell32.dll,Control_RunDLL odbccp32.cpl,,1
 *	4/ Set breakpoints as desired and hit the run button
 */
#if defined __BORLANDC__ || defined __MINGW32__
extern "C" __declspec( dllexport ) BOOL INSTAPI ConfigDSN(HWND		hWnd,
#else
BOOL INSTAPI ConfigDSN(HWND		hWnd,
#endif
			   WORD		fRequest,
			   LPCSTR	lpszDriver,
			   LPCSTR	lpszAttributes)
{
	if ( !lpszDriver || strncmp (lpszDriver, DRIVER_FULL_NAME, strlen(DRIVER_FULL_NAME)) )
	{
		SQLPostInstallerError ( ODBC_ERROR_INVALID_NAME,"Invalid driver name");
		return false;
	}

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
 *		regsvr32 .\odbcjdbcsetup.dll
 *
 *	To debug registration the project settings to call regsvr32.exe
 *  with the full path.
 *
 *  Use 
 *		..\debug\odbcjdbcsetup.dll
 *
 *  as the program argument
 *
 */

extern "C" __declspec( dllexport ) int INSTAPI DllRegisterServer (void)
{
	char fileName [256];
	JString msg;

	GetModuleFileName (m_hInstance, fileName, sizeof (fileName));

	char pathOut [256];
	WORD length = sizeof (pathOut);
	DWORD useCount;
	BOOL fRemoveDSN = FALSE;

	if ( !SQLInstallDriverEx (
			driverInfo,
			NULL, //fileName,
			pathOut,
			sizeof (pathOut),
			&length,
			ODBC_INSTALL_COMPLETE,
			&useCount))
	{
		MessageBoxInstallerError("Install Driver Failed", pathOut);
		return S_FALSE;
	}

	if ( useCount > 1 ) // On a case update
		SQLRemoveDriver(DRIVER_FULL_NAME, fRemoveDSN, &useCount);

	if( !memicmp( fileName, pathOut, strlen(pathOut)) )
	{
		MessageBox(NULL, " ERROR!\nPlease, use regsvr32.exe .\\OdbcJdbcSetup.dll", DRIVER_NAME, MB_ICONSTOP|MB_OK);
		SQLRemoveDriver(DRIVER_FULL_NAME, fRemoveDSN, &useCount);
        return S_FALSE;
	}

 	char *path = pathOut + strlen (pathOut);
	if (path != strrchr (pathOut, '\\') + 1)
		*path++ = '\\';
	char *tail = strrchr (fileName, '\\') + 1;

	for (const char **ptr = fileNames; *ptr; ++ptr)
	{
		strcpy (path, *ptr);
		strcpy (tail, *ptr);
		if (!CopyFile (fileName, pathOut))
		{
			SQLRemoveDriver(DRIVER_FULL_NAME, fRemoveDSN, &useCount);
			return S_FALSE;
		}
	}

	if (!SQLConfigDriver (
			NULL,
			ODBC_INSTALL_DRIVER,
			DRIVER_FULL_NAME, 
			NULL,
			DRIVER_FULL_NAME" was installed successfully",
			64,
			NULL))
	{
		MessageBoxInstallerError("Config Install", pathOut);
		SQLRemoveDriver(DRIVER_FULL_NAME, fRemoveDSN, &useCount);
        return S_FALSE;
	} 	

	return S_OK;
}

extern "C" __declspec( dllexport ) int INSTAPI DllUnregisterServer (void)
{
	JString msg;
	DWORD useCount = 0;
	BOOL fRemoveDSN = FALSE;

	if ( !SQLConfigDriver (
			NULL,
			ODBC_REMOVE_DRIVER,
			DRIVER_FULL_NAME, 
			NULL,
			DRIVER_FULL_NAME" was Uninstalled successfully",
			64,
			NULL))
	{
		MessageBoxInstallerError("Config Uninstall", NULL);
	} 	
	else if ( !SQLRemoveDriver(DRIVER_FULL_NAME, fRemoveDSN, &useCount) )
	{
		MessageBoxInstallerError("Uninstall Driver", NULL);
	}

	if ( !useCount )
	{
		bool bContinue = true;
		char pathFile[256], *path;
		WORD length = sizeof (pathFile);
		UINT lenSystemPath;

		lenSystemPath = GetSystemDirectory(pathFile,length-1);
		if ( !lenSystemPath )
		{
			MessageBoxError("GetSystemDirectory", pathFile);
			bContinue = false;
		}

		path = pathFile + lenSystemPath;
		*path++ = '\\';

		for (const char **ptr = fileNames; bContinue && *ptr; ++ptr)
		{
			strcpy (path, *ptr);
			if ( !DeleteFile (pathFile) )
			{
				//Best not to throw an error when we fail to remove ourself
				if ( !strstr(pathFile,"OdbcJdbcSetup") )
				{
					MessageBoxError("DeleteFile", pathFile);
					bContinue = false;
				}
			}
		}
	}

	return S_OK;
}

void MessageBoxError(char * stageExecuted, char * pathFile)
{
	JString msg;
	DWORD messageId = GetLastError();
	char temp[256];
	if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, messageId,
			0, temp, sizeof(temp), NULL))
	{
		msg.Format ("Format message failed %d\n", GetLastError());
		MessageBox(NULL, (const char*)msg,DRIVER_NAME, MB_ICONSTOP|MB_OK);
		return;
	}

	msg.Format ("%s (%s, %s) failed with %d\n%s\n", 
				stageExecuted, DRIVER_FULL_NAME, pathFile, messageId, temp);
	MessageBox(NULL, (const char*)msg,DRIVER_NAME, MB_ICONSTOP|MB_OK);
}

void MessageBoxInstallerError(char * stageExecuted, char * pathOut)
{
	JString msg;
    char message [SQL_MAX_MESSAGE_LENGTH];
    WORD        errCodeIn = 1;
    DWORD	errCodeOut = 0L;

    SQLInstallerError(errCodeIn, &errCodeOut, message, sizeof (message) - 1, NULL);

	if ( pathOut && *pathOut )
		msg.Format ("%s (%s, %s) failed with %d\n%s\n",
		                stageExecuted, DRIVER_FULL_NAME, pathOut, errCodeOut, message);
	else	
	    msg.Format ("%s (%s) failed with %d\n%s\n",
		                stageExecuted, DRIVER_FULL_NAME, errCodeOut, message);

    MessageBox(NULL, (const char*)msg,DRIVER_NAME, MB_ICONSTOP|MB_OK);
}

bool CopyFile(char * sourceFile, char * destFile)
{
	if ( (long)GetFileAttributes( sourceFile ) == -1 )
	{
		MessageBoxError("CopyFile", sourceFile);
		return false;
	}

	HFILE	src;
	HFILE	dst;
	OFSTRUCT reopenBuff;
	UINT uStyle = OF_READ | OF_SHARE_DENY_NONE;

	src = OpenFile( sourceFile, &reopenBuff, uStyle );
	if ( src == HFILE_ERROR )
	{
		MessageBoxError("CopyFile", sourceFile);
		return false;
	}

	uStyle = OF_WRITE | OF_CREATE;
	dst = OpenFile( destFile, &reopenBuff, uStyle );
	if ( src == HFILE_ERROR )
	{
		MessageBoxError("CopyFile", destFile);
		return false;
	}

	DWORD allSize = GetFileSize( (HANDLE)src, NULL );
 	unsigned long bufferSize = 32768;
	char *  bufferData = new char[ bufferSize ];
	DWORD dwRead;

	while ( allSize > 0 )
	{
		if ( !ReadFile( (HANDLE)src, bufferData, bufferSize, &dwRead, NULL) )
		{
			MessageBoxError("CopyFile", sourceFile);
			return false;
		}

		DWORD nWritten;
		if ( !WriteFile( (HANDLE)dst, bufferData, dwRead, &nWritten, NULL))
		{
			MessageBoxError("CopyFile", destFile);
			return false;
		}

		if (nWritten != dwRead)
		{
			MessageBox(NULL, "Disk full",DRIVER_NAME, MB_ICONSTOP|MB_OK);
			return false;
		}
		allSize -= dwRead;
	}

	CloseHandle( (HANDLE)src );
	CloseHandle( (HANDLE)dst );
	delete [] bufferData;

	return true;
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
		attributes = attr;
		dsn = getAttribute (SETUP_DSN);
	}
}

Setup::~Setup()
{

}

void Setup::configDsn()
{
	if ( !dsn.IsEmpty() )
		readAttributes();
	configureDialog();
}

void Setup::addDsn()
{
	dbName = getAttribute (SETUP_DBNAME);
	if ( dbName.IsEmpty() )
		dbName = getAttribute (KEY_DSN_DATABASE);

	client = getAttribute (SETUP_CLIENT);

	user = getAttribute (SETUP_USER);
	if ( user.IsEmpty() )
		user = getAttribute (KEY_DSN_UID);

	password = getAttribute (SETUP_PASSWORD);
	if ( password.IsEmpty() )
		password = getAttribute (KEY_DSN_PWD);

	jdbcDriver = getAttribute (SETUP_JDBC_DRIVER);
	if ( jdbcDriver.IsEmpty() )
		jdbcDriver = getAttribute (KEY_DSN_JDBC_DRIVER);
	if ( jdbcDriver.IsEmpty() )
		jdbcDriver = drivers [0];

	role = getAttribute (SETUP_ROLE);
	
	charset = getAttribute (SETUP_CHARSET);
	if ( charset.IsEmpty() )
		charset = getAttribute (KEY_DSN_CHARSET);

	readonlyTpb = getAttribute (SETUP_READONLY_TPB);
	nowaitTpb = getAttribute (SETUP_NOWAIT_TPB);
	dialect = getAttribute (SETUP_DIALECT);

	quoted = getAttribute (SETUP_QUOTED);
	if ( quoted.IsEmpty() )
		quoted = getAttribute (KEY_DSN_QUOTED);

	sensitive = getAttribute ( SETUP_SENSITIVE );
	autoQuoted = getAttribute ( SETUP_AUTOQUOTED );

	char chCheck = UPPER( *(const char*)readonlyTpb );
	
	if ( !IS_CHECK_YES(chCheck) && !IS_CHECK_NO(chCheck) )
		readonlyTpb = "N";

	chCheck = UPPER( *(const char*)nowaitTpb );
	
	if ( !IS_CHECK_YES(chCheck) && !IS_CHECK_NO(chCheck) )
		nowaitTpb = "N";

	chCheck = *(const char*)dialect;
	
	if ( chCheck != '1' && chCheck != '3' )
		dialect = "3";

	chCheck = UPPER( *(const char*)quoted );
	
	if ( !IS_CHECK_YES(chCheck) && !IS_CHECK_NO(chCheck) )
		quoted = "Y";

	chCheck = UPPER( *(const char*)sensitive );
	
	if ( !IS_CHECK_YES(chCheck) && !IS_CHECK_NO(chCheck) )
		sensitive = "N";

	chCheck = UPPER( *(const char*)autoQuoted );
	
	if ( !IS_CHECK_YES(chCheck) && !IS_CHECK_NO(chCheck) )
		autoQuoted = "N";

	if ( hWnd || dsn.IsEmpty() )
		configureDialog();
	else
	{
		SQLWriteDSNToIni(dsn, driver);
		writeAttributes();
	}
}

void Setup::removeDsn()
{
	if ( !dsn.IsEmpty() )
		SQLRemoveDSNFromIni (dsn);
}

JString Setup::getAttribute(const char * attribute)
{
	const char *p;
	int count = strlen (attribute);

	for (p = attributes; *p || *(p+1); ++p)
	{
		if ( p - attributes > 4096 )
			break; // attributes should be finished "\0\0"

		if (*p == *attribute && !strncasecmp (p, attribute, count) )
		{
			p += count;
			while (*p && (*p == ' ' || *p == '\t') )
				++p;
			if ( *p == '=' )
			{
				++p;
				while (*p && (*p == ' ' || *p == '\t') )
					++p;

				const char *q;
				for (q = p; *q && *q != ';' && !IS_END_TOKEN(*q); ++q)
					;
				return JString (p, q - p);
			}
		}
		while (*p && *p != ';' && !IS_END_TOKEN(*p) )
			++p;
	}

	return JString();
}

bool Setup::configureDialog()
{
	if ( jdbcDriver.IsEmpty() )
		jdbcDriver = drivers [0];

	CDsnDialog dialog (drivers, charsets);
	dialog.m_name = dsn;
	dialog.m_database = dbName;
	dialog.m_client = client;
	dialog.m_user = user;
	dialog.m_password = password;
	dialog.m_driver = jdbcDriver;
	dialog.m_role = role;
	dialog.m_charset = charset;

	if ( IS_CHECK_YES(*(const char*)readonlyTpb) )
		dialog.m_readonly = TRUE;
	else 
		dialog.m_readonly=FALSE;

	if ( IS_CHECK_YES(*(const char*)nowaitTpb) )
		dialog.m_nowait = TRUE;
	else 
		dialog.m_nowait=FALSE;

	if ( *(const char*)dialect == '1' )
		dialog.m_dialect3 = FALSE;
	else 
		dialog.m_dialect3 = TRUE;

	if ( IS_CHECK_YES(*(const char*)quoted) )
		dialog.m_quoted = TRUE;
	else 
		dialog.m_quoted = FALSE;

	if ( IS_CHECK_YES ( *(const char*)sensitive ) )
		dialog.m_sensitive = TRUE;
	else 
		dialog.m_sensitive = FALSE;

	if ( IS_CHECK_YES ( *(const char*)autoQuoted ) )
		dialog.m_autoQuoted = TRUE;
	else 
		dialog.m_autoQuoted = FALSE;

	if ( dialog.DoModal() != IDOK )
		return false;

	dsn = dialog.m_name;
	dbName = dialog.m_database;
	client = dialog.m_client;
	user = dialog.m_user;
	password = dialog.m_password;
	jdbcDriver = dialog.m_driver;
	role = dialog.m_role;
	charset = dialog.m_charset;

	if( dialog.m_readonly ) readonlyTpb = "Y";
	else readonlyTpb = "N";

	if( dialog.m_nowait ) nowaitTpb = "Y";
	else nowaitTpb = "N";

	if( dialog.m_dialect3 ) dialect = "3";
	else dialect = "1";

	if( dialog.m_quoted ) quoted = "Y";
	else quoted = "N";

	if( dialog.m_sensitive ) sensitive = "Y";
	else sensitive = "N";

	if( dialog.m_autoQuoted ) autoQuoted = "Y";
	else autoQuoted = "N";

	SQLWriteDSNToIni(dialog.m_name, driver);
	writeAttributes();

	return true;
}

void Setup::writeAttributes()
{
	writeAttribute (SETUP_DBNAME, dbName);
	writeAttribute (SETUP_CLIENT, client);
	writeAttribute (SETUP_USER, user);
	writeAttribute (SETUP_ROLE, role);
	writeAttribute (SETUP_CHARSET, charset);
	writeAttribute (SETUP_JDBC_DRIVER, jdbcDriver);
	writeAttribute (SETUP_READONLY_TPB, readonlyTpb);
	writeAttribute (SETUP_NOWAIT_TPB, nowaitTpb);
	writeAttribute (SETUP_DIALECT, dialect);
	writeAttribute (SETUP_QUOTED, quoted);
	writeAttribute (SETUP_SENSITIVE, sensitive);
	writeAttribute (SETUP_AUTOQUOTED, autoQuoted);

	char buffer[256];
	CSecurityPassword security;
	security.encode( (char*)(const char *)password, buffer );
	writeAttribute (SETUP_PASSWORD, buffer);
}

void Setup::readAttributes()
{
	dbName = readAttribute (SETUP_DBNAME);
	client = readAttribute (SETUP_CLIENT);
	user = readAttribute (SETUP_USER);
	jdbcDriver = readAttribute (SETUP_JDBC_DRIVER);
	role = readAttribute (SETUP_ROLE);
	charset = readAttribute (SETUP_CHARSET);
	readonlyTpb = readAttribute (SETUP_READONLY_TPB);
	nowaitTpb = readAttribute (SETUP_NOWAIT_TPB);
	dialect = readAttribute (SETUP_DIALECT);
	quoted = readAttribute (SETUP_QUOTED);
	sensitive = readAttribute (SETUP_SENSITIVE);
	autoQuoted = readAttribute (SETUP_AUTOQUOTED);

	JString pass = readAttribute (SETUP_PASSWORD);
	if ( pass.length() > 40 )
	{
		char buffer[256];
		CSecurityPassword security;
		security.decode( (char*)(const char *)pass, buffer );
		password = buffer;
	}
	else
		password = pass;
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

}; // end namespace OdbcJdbcSetupLibrary
