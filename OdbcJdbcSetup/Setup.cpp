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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "OdbcJdbcSetup.h"
#include "../IscDbc/Connection.h"
#include <odbcinst.h>
#include "DsnDialog.h"
#include "Setup.h"
#include "../SetupAttributes.h"
#include "../SecurityPassword.h"
#include "ServiceClient.h"
#ifdef _WINDOWS
#include <regstr.h>
#endif

namespace OdbcJdbcSetupLibrary {

extern HINSTANCE m_hInstance;
extern int currentCP;
extern TranslateString translate[];

using namespace IscDbcLibrary;
using namespace classSecurityPassword;

#ifdef _WINDOWS
#ifndef strncasecmp

#if _MSC_VER >= 1400 // VC80 and later
#define strncasecmp		_strnicmp
#else
#define strncasecmp		strnicmp
#endif // _MSC_VER >= 1400

#endif // strncasecmp
#endif

static const char *fileNames [] = {
	DRIVER_NAME DRIVER_EXT,
	NULL
	};

static const char *drivers [] = { DEFAULT_DRIVER, NULL };
static const char *charsets []= 
{ 
	"NONE", "ASCII", "BIG_5", "CYRL", "DOS437", "DOS850", "DOS852", "DOS857", "DOS860",
	"DOS861", "DOS863", "DOS865", "DOS866", "EUCJ_0208", "GB_2312", "ISO8859_1", 
	"ISO8859_2", "KSC_5601", "OCTETS", "SJIS_0208", "UNICODE_FSS", "UTF8", 
	"WIN1250", "WIN1251", "WIN1252", "WIN1253", "WIN1254", NULL
};

static const char *useshemas []= 
{ 
	"Set null field SCHEMA",
	"Remove SCHEMA from SQL query",
	"Use full SCHEMA",
	NULL
};

void MessageBoxError(const char * stageExecuted, char * pathFile);
bool MessageBoxInstallerError(const char * stageExecuted, char * pathOut);
bool MoveFileDelayUntilReboot(char * sourceFile, char * destFile);
bool IsFileInUse(const char * checkFile);
bool DelayRegisterOdbcJdbc( char *pathDestination, char *endPathDestination,
						    char *pathSource, char *endPathSource );
bool CopyFile(char * sourceFile, char * destFile);
void DeleteFiles( char *pathDestination, char *endPathDestination );
void addKeyValue( char *& strTemp, const char * key, const char * value = "" );
bool installDriver( void );
void initVersionDriver( char * buffer );
bool copyFilesDriver( char *pathDestination, char *endPathDestination,
					  char *pathSource, char *endPathSource );
bool removeVersionDriver( void );
bool checkedVersion( char *fullPath, char *endPath );

bool silentDisplay = false;
bool setInstallKey = false;

void getParamFromCommandLine()
{
	for ( int i = 1; i < __argc; i++ )
	{
		const char *argv = __argv[i];

		if ( argv[0] == '/' )
		{
			switch ( UPPER( *(argv + 1) ) )
			{
			case 'S':
				silentDisplay = true;
				break;

			case 'I':
				setInstallKey = true;
				break;
			}
		}
	}
}
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
BOOL INSTAPI ConfigDSN( HWND        hWnd,
#endif
                        WORD        fRequest,
                        LPCSTR      lpszDriver,
                        LPCSTR      lpszAttributes )
{
	if ( !lpszDriver || strncmp (lpszDriver, DRIVER_FULL_NAME, strlen(DRIVER_FULL_NAME)) )
	{
		SQLPostInstallerError( ODBC_ERROR_INVALID_NAME, _TR( IDS_ERROR_MESSAGE_04, "Invalid driver name" ) );
		return false;
	}

	Setup setup( hWnd, lpszDriver, lpszAttributes );
	switch ( fRequest )
	{
	case ODBC_CONFIG_DSN:
		setup.configDsn();
		break;

	case ODBC_ADD_DSN:
		if ( !setup.addDsn() )
			return false;
		break;

	case ODBC_REMOVE_DSN:
		setup.removeDsn();
		break;
	}

	return TRUE;
}

#if defined __BORLANDC__ || defined __MINGW32__
extern "C" __declspec( dllexport ) BOOL INSTAPI ConfigTranslator( HWND hWnd, DWORD *pvOptionWORD )
#else
BOOL INSTAPI ConfigTranslator( HWND hWnd, DWORD *pvOptionWORD )
#endif
{
#ifdef _DEBUG
	JString msg;
	msg.Format( "ConfigTranslator \n" );
	OutputDebugString( msg );
#endif
	return TRUE;
}

#if defined __BORLANDC__ || defined __MINGW32__
extern "C" __declspec( dllexport ) HRESULT INSTAPI DllInstall( BOOL install, LPCWSTR commandLine )
#else
HRESULT INSTAPI DllInstall( BOOL install, LPCWSTR commandLine )
#endif
{
	char fileName [MAX_PATH];
	char pathOut [MAX_PATH];
	WORD length = sizeof (pathOut) - 1;
	DWORD useCount;

	GetModuleFileName( m_hInstance, fileName, sizeof ( fileName ) );

	JString msg;
	msg.Format( "DllInstall cmdline %S %s\n", commandLine, fileName );
	OutputDebugString( msg );

	if ( !SQLInstallDriverEx( 
			DRIVER_FULL_NAME"\0" INSTALL_DRIVER "=" DRIVER_NAME ".DLL\0\0",
			NULL,
			pathOut,
			sizeof ( pathOut ),
			&length,
			ODBC_INSTALL_INQUIRY,
			&useCount ) )
	{
		MessageBoxInstallerError( _TR( IDS_ERROR_MESSAGE_05, "Install Driver Failed" ), pathOut );
        return S_FALSE;
	}

	if ( !strncasecmp( fileName, pathOut, strlen( pathOut ) ) )
	{
        return S_OK;
	}

 	char *path = pathOut + strlen( pathOut );
	if ( path != strrchr(pathOut, '\\') + 1 )
		*path++ = '\\';
	char *tail = strrchr( fileName, '\\' ) + 1;
	bool ret = copyFilesDriver( pathOut, path, fileName, tail );

	return ret ? S_OK : S_FALSE;
}

/*
 *	Registration can be performed with the following command:
 *
 *		regsvr32 .\OdbcFb.dll
 *
 *	To debug registration the project settings to call regsvr32.exe
 *  with the full path.
 *
 *  Use 
 *		..\debug\OdbcFb.dll
 *
 *  as the program argument
 *
 */

extern "C" STDAPI DllRegisterServer (void)
{
	char pathOut [MAX_PATH];
	WORD length = sizeof (pathOut) - 1;
	DWORD useCount;

	if ( !SQLInstallDriverEx( 
			DRIVER_FULL_NAME"\0" INSTALL_DRIVER "=" DRIVER_NAME ".DLL\0\0",
			NULL,
			pathOut,
			sizeof ( pathOut ),
			&length,
			ODBC_INSTALL_INQUIRY,
			&useCount ) )
	{
		MessageBoxInstallerError( _TR( IDS_ERROR_MESSAGE_05, "Install Driver Failed" ), pathOut );
        return S_FALSE;
	}

	return installDriver() ? S_OK : S_FALSE;
}

bool installDriver( void )
{
	char temp [80];
	char *fullDriverName = temp;
	char pathIn[MAX_PATH];
	char pathOut [MAX_PATH];
	WORD length = sizeof (pathOut) - 1;
	DWORD useCount;
	BOOL fRemoveDSN = FALSE;
	char strDriverInfo [512];

	fullDriverName = DRIVER_FULL_NAME;

	initVersionDriver( strDriverInfo );

	if (!setInstallKey)
	{
		GetModuleFileName( m_hInstance, pathIn, sizeof ( pathIn ) );
		char *tail = strrchr( pathIn, '\\' ) + 1;
		*tail = '\0';
	}

	if ( !SQLInstallDriverEx(
			strDriverInfo,
			setInstallKey ? NULL : pathIn,
			pathOut,
			sizeof (pathOut),
			&length,
			ODBC_INSTALL_COMPLETE,
			&useCount ) )
	{
		MessageBoxInstallerError( _TR( IDS_ERROR_MESSAGE_05, "Install Driver Failed" ), pathOut );
		SQLRemoveDriver( fullDriverName, fRemoveDSN, &useCount );
		return false;
	}

	if ( useCount > 1 ) // On a case update
		SQLRemoveDriver( fullDriverName, fRemoveDSN, &useCount );

	if ( !SQLConfigDriver(
			NULL,
			ODBC_INSTALL_DRIVER,
			fullDriverName,
			NULL,
			NULL,
			0,
			NULL ) )
	{
	    WORD errCodeIn = 1;
	    DWORD errCodeOut = 0L;
		SQLInstallerError( errCodeIn, &errCodeOut, NULL, 0, NULL );

		if ( ODBC_ERROR_LOAD_LIB_FAILED != errCodeOut )
		{
			MessageBoxInstallerError( _TR( IDS_ERROR_MESSAGE_07, "Config Install" ), pathOut );
			return false;
		}
	} 	

	return true;
}

bool checkedVersion( char *fullPath, char *endPath )
{
	int countAll = 0;
	int countVer = 0;
	bool ret = true;
	const char **ptr;
    DWORD handle;
    DWORD verInfoSize;

	for ( ptr = fileNames; *ptr && ret; ++ptr )
	{
		sprintf( endPath, "%s", *ptr );
		++countAll;

	    verInfoSize = GetFileVersionInfoSizeA( fullPath, &handle );

		if ( verInfoSize )
		{
			void *buffer = LocalAlloc( LPTR, verInfoSize );

			if ( buffer )
			{
				if ( GetFileVersionInfoA( fullPath, handle, verInfoSize, buffer ) )
				{
					char *version;
					UINT len;
                                        
					if ( VerQueryValueA( buffer, "\\StringFileInfo\\080904b0\\FileVersion", (void **)&version, &len ) && len )
					{
						if ( strcmp( FILE_VERSION_STR, version ) )
							ret = false;
						else
							++countVer;
					}
				}

				LocalFree( LocalHandle( buffer ) );
			}
		}
	}

	return ret && countAll == countVer;
}

bool copyFilesDriver( char *pathDestination, char *endPathDestination,
					  char *pathSource, char *endPathSource )
{
	bool statusCopy = true;
	bool statusDelayMove = false;
	const char **ptr;

	for ( ptr = fileNames; *ptr; ++ptr )
	{
		sprintf( endPathSource, "%s", *ptr );
		sprintf( endPathDestination, "%s", *ptr );

		if ( (long)GetFileAttributes( pathSource ) == -1 )
		{
			MessageBoxError( _TR( IDS_ERROR_MESSAGE_15, "CopyFile" ), pathSource );

			statusCopy = false;
			break;
		}

		if ( (long)GetFileAttributes( pathDestination ) != -1 )
			if ( IsFileInUse( pathDestination ) )
				statusDelayMove = true;
	}

	if ( !statusCopy )
		return false;

	if ( !statusDelayMove )
	{
		for ( ptr = fileNames; *ptr; ++ptr )
		{
			sprintf( endPathSource, "%s", *ptr );
			sprintf( endPathDestination, "%s", *ptr );
			CopyFile( pathSource, pathDestination );
		}
	}
	else
	{
		DelayRegisterOdbcJdbc( pathDestination, endPathDestination,
							   pathSource, endPathSource );
		MessageBox( NULL, 
					(const char*)"Please, reboot for use",
					DRIVER_NAME,
					MB_ICONINFORMATION|MB_OK );

		return false;
	}

	return true;
}
						  
extern "C" STDAPI DllUnregisterServer( void )
{
	return removeVersionDriver() ? S_OK : S_FALSE;
}

bool removeVersionDriver( void )
{
	bool ret = true;
	DWORD useCount = 0;
	BOOL fRemoveDSN = FALSE;
	char temp [80];
	char * fullDriverName = temp;

	fullDriverName = DRIVER_FULL_NAME;

	if ( !SQLConfigDriver( NULL,
						   ODBC_REMOVE_DRIVER,
						   fullDriverName,
						   NULL,
						   NULL,
						   0,
						   NULL ) )
	{
	    WORD errCodeIn = 1;
	    DWORD errCodeOut = 0L;
		SQLInstallerError( errCodeIn, &errCodeOut, NULL, 0, NULL );

		if ( ODBC_ERROR_COMPONENT_NOT_FOUND == errCodeOut )
			return true;

		MessageBoxInstallerError( _TR( IDS_ERROR_MESSAGE_08, "Config Uninstall" ), NULL );
		ret = false;
	}

	if ( !SQLRemoveDriver( fullDriverName, fRemoveDSN, &useCount ) )
	{
		MessageBoxInstallerError( _TR( IDS_ERROR_MESSAGE_09, "Uninstall Driver" ), NULL );
		ret = false;
	}

	if ( !useCount )
	{
		char pathFile[MAX_PATH], *path;
		WORD length = sizeof (pathFile);
		UINT lenSystemPath;

		lenSystemPath = GetSystemDirectory( pathFile, length - 1 );
		if ( !lenSystemPath )
		{
			MessageBoxError( _TR( IDS_ERROR_MESSAGE_10, "GetSystemDirectory" ), pathFile );
			ret = false;
		}
		else
		{
			path = pathFile + lenSystemPath;
			*path++ = '\\';
			DeleteFiles( pathFile, path );
		}
	}

	return ret;
}

extern "C" __declspec( dllexport ) BOOL INSTAPI ConfigDriver( HWND hwndParent, 
						   WORD fRequest, 
                           LPCSTR lpszDriver,
				           LPCSTR lpszArgs, 
                           LPSTR  lpszMsg, 
                           WORD   cbMsgMax, 
                           WORD 	*pcbMsgOut )
{
#ifdef _DEBUG
	JString msg;
	msg.Format( "ConfigDriver %s\n", lpszDriver );
	OutputDebugString( msg );
#endif
    return TRUE;
}

void MessageBoxError(const char * stageExecuted, char * pathFile)
{
	JString msg;
	DWORD messageId = GetLastError();
	char temp[MAX_PATH];
	if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, messageId,
			0, temp, sizeof(temp), NULL))
	{
		msg.Format( _TR( IDS_ERROR_MESSAGE_12, "Format message failed %d\n" ), GetLastError() );
		MessageBox(NULL, (const char*)msg,DRIVER_NAME, MB_ICONSTOP|MB_OK);
		return;
	}

	msg.Format( _TR( IDS_ERROR_MESSAGE_13, "%s (%s, %s) failed with %d\n%s\n" ), 
				stageExecuted, DRIVER_FULL_NAME, pathFile, messageId, temp );
	MessageBox(NULL, (const char*)msg,DRIVER_NAME, MB_ICONSTOP|MB_OK);
}

bool MessageBoxInstallerError(const char * stageExecuted, char * pathOut)
{
	RETCODE rc = SQL_SUCCESS_WITH_INFO;
	JString msg;
	JString msgTemp;
    char message[SQL_MAX_MESSAGE_LENGTH];
    WORD errCodeIn = 1;
    DWORD errCodeOut = 0L;
	WORD cbErrorMsg = 0;

	while ( rc == SQL_SUCCESS_WITH_INFO )
	{
		rc = SQLInstallerError( errCodeIn, &errCodeOut, message, sizeof ( message ) - 1, &cbErrorMsg );
		if ( cbErrorMsg )
			msgTemp += message;
	}

	if ( !msgTemp.IsEmpty() )
	{
		if ( pathOut && *pathOut )
			msg.Format( _TR( IDS_ERROR_MESSAGE_13, "%s (%s, %s) failed with %d\n%s\n" ),
							stageExecuted, DRIVER_FULL_NAME, pathOut, errCodeOut, (const char*)msgTemp );
		else	
			msg.Format( _TR( IDS_ERROR_MESSAGE_14, "%s (%s) failed with %d\n%s\n" ),
							stageExecuted, DRIVER_FULL_NAME, errCodeOut, (const char*)msgTemp );
	    MessageBox(NULL, (const char*)msg,DRIVER_NAME, MB_ICONSTOP|MB_OK);
		return true;
	}

	return false;
}

bool IsWinNT()
{
	OSVERSIONINFO osVersionInfo = { 0 };

	osVersionInfo.dwOSVersionInfoSize = sizeof ( osVersionInfo );
	GetVersionEx( &osVersionInfo );

	return osVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT; 
}

bool MoveFileDelayUntilReboot(char * sourceFile, char * destFile)
{
	if ( IsWinNT() )
		return !!MoveFileEx( sourceFile,
							 destFile,
						     MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING );

	char pathShortDestFile[MAX_PATH];
	char pathShortSourFile[MAX_PATH];

	if ( !GetShortPathName( destFile, pathShortDestFile, MAX_PATH ) )
		strcpy( pathShortDestFile, destFile );

	if ( !GetShortPathName( sourceFile, pathShortSourFile, MAX_PATH ) )
		strcpy( pathShortSourFile, sourceFile );

	return !!WritePrivateProfileString( "Rename", pathShortDestFile, pathShortSourFile, "WinInit.ini" );
}

bool IsFileInUse(const char * checkFile)
{
	HFILE	chk;
	OFSTRUCT reopenBuff;
	UINT uStyle = OF_WRITE | OF_SHARE_EXCLUSIVE;
	chk = OpenFile( checkFile, &reopenBuff, uStyle );
	if ( chk == HFILE_ERROR )
		return true;

	CloseHandle( (HANDLE)chk );
	return false;
}

bool DelayRegisterOdbcJdbc( char *pathDestination, char *endPathDestination,
						    char *pathSource, char *endPathSource )
{
    long res;
    HKEY hKey = NULL;
    char commandLine[ 2 * MAX_PATH + 2 ];
    DWORD   dwDisposition;

	strcpy( endPathSource, DRIVER_NAME".DLL" );
	strcpy( endPathDestination, "regsvr32.exe" );

	do
	{
		res = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
							  REGSTR_PATH_RUNONCE,
							  (ULONG)0,
							  NULL,
							  REG_OPTION_NON_VOLATILE,
							  KEY_ALL_ACCESS,
							  NULL,
							  &hKey,
							  &dwDisposition );

		if ( res != ERROR_SUCCESS )
			break;

		wsprintf( commandLine, "%s /s /i \"%s\"", pathDestination, pathSource );

		res = RegSetValueEx( hKey,
							 "OdbcJdbcSetupDelay",
							 0,
							 REG_SZ,
							 (PUCHAR)commandLine,
							 (DWORD)strlen( commandLine ) + 1 );

		if ( res != ERROR_SUCCESS )
			break;

	} while ( false );

    if ( hKey )
        RegCloseKey( hKey );

    return res == ERROR_SUCCESS;
}

bool CopyFile(char * sourceFile, char * destFile)
{
	if ( (long)GetFileAttributes( sourceFile ) == -1 )
	{
		MessageBoxError( _TR( IDS_ERROR_MESSAGE_15, "CopyFile" ), sourceFile );
		return false;
	}

	HFILE	src;
	HFILE	dst;
	OFSTRUCT reopenBuff;
	UINT uStyle = OF_READ | OF_SHARE_DENY_NONE;

	src = OpenFile( sourceFile, &reopenBuff, uStyle );
	if ( src == HFILE_ERROR )
	{
		MessageBoxError( _TR( IDS_ERROR_MESSAGE_15, "CopyFile" ), sourceFile );
		return false;
	}

	uStyle = OF_WRITE | OF_CREATE;
	dst = OpenFile( destFile, &reopenBuff, uStyle );
	if ( dst == HFILE_ERROR )
	{
		MessageBoxError( _TR( IDS_ERROR_MESSAGE_15, "CopyFile" ), destFile );
		CloseHandle( (HANDLE)src );
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
			MessageBoxError( _TR( IDS_ERROR_MESSAGE_15, "CopyFile" ), sourceFile );
			return false;
		}

		DWORD nWritten;
		if ( !WriteFile( (HANDLE)dst, bufferData, dwRead, &nWritten, NULL))
		{
			MessageBoxError( _TR( IDS_ERROR_MESSAGE_15, "CopyFile" ), destFile );
			return false;
		}

		if (nWritten != dwRead)
		{
			MessageBox( NULL, _TR( IDS_ERROR_MESSAGE_16, "Disk full" ), DRIVER_NAME, MB_ICONSTOP|MB_OK );
			return false;
		}
		allSize -= dwRead;
	}

	CloseHandle( (HANDLE)src );
	CloseHandle( (HANDLE)dst );
	delete [] bufferData;

	return true;
}

void DeleteFiles( char *pathDestination, char *endPathDestination )
{
	char pathShortFile[MAX_PATH];
	UINT sizeRenameSection = 32767;
	char *renameSection = NULL;
	UINT lenRenameSection;

	for ( const char **ptr = fileNames; *ptr; ++ptr )
	{
		sprintf( endPathDestination, "%s", *ptr );

		if ( (long)GetFileAttributes( pathDestination ) != -1 )
			if ( !DeleteFile( pathDestination ) )
			{
				DWORD messageId = GetLastError();

				if ( messageId == 32 || messageId == 5 )
				{
					const char *pt;
					const char *fileName;

					if ( !renameSection )
					{
						renameSection = new char[sizeRenameSection];
						lenRenameSection = GetPrivateProfileSection( "Rename",
																	 renameSection,
																	 sizeRenameSection,
																	 "WinInit.ini" );
					}


		            if ( !GetShortPathName( pathDestination, pathShortFile, MAX_PATH ) )
						strcpy( pathShortFile, pathDestination );

					for ( pt = renameSection; *pt; pt += strlen( pt ) + 1 )
						if ( fileName = strchr( pt, '=' ), fileName
							&& !strcasecmp( fileName + 1, pathShortFile ) )
							break;

					if ( !*pt )
					{
						lenRenameSection += sprintf( renameSection + lenRenameSection,
													 "Nul=%s",
													 pathShortFile ) + 1;
						renameSection[lenRenameSection] = 0;
					}
				}
			}
	}

	if ( renameSection )
	{
		WritePrivateProfileSection( "rename", renameSection, "WinInit.ini" );
		delete[] renameSection;
	}
}

void addKeyValue( char *& strTemp, const char * key, const char * value )
{
	while ( (*strTemp++ = *key++) );

	if ( *value )
	{
		--strTemp;
		*strTemp++ = '=';
		while ( (*strTemp++ = *value++) );
	}
}

void initVersionDriver( char * buffer )
{
	addKeyValue( buffer, DRIVER_FULL_NAME );
	addKeyValue( buffer, INSTALL_DRIVER, DRIVER_NAME DRIVER_EXT);
	addKeyValue( buffer, INSTALL_SETUP, DRIVER_NAME DRIVER_EXT );
	addKeyValue( buffer, INSTALL_FILE_EXT, VALUE_FILE_EXT );
	addKeyValue( buffer, INSTALL_API_LEVEL, VALUE_API_LEVEL );
	addKeyValue( buffer, INSTALL_CONNECT_FUN, VALUE_CONNECT_FUN );
	addKeyValue( buffer, INSTALL_FILE_USAGE, VALUE_FILE_USAGE );
	addKeyValue( buffer, INSTALL_DRIVER_VER, VALUE_DRIVER_VER );
	addKeyValue( buffer, INSTALL_SQL_LEVEL, VALUE_SQL_LEVEL	 );
	addKeyValue( buffer, "" );
}

//////////////////////////////////////////////////////////////////////
// Setup Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Setup::Setup( HWND windowHandle, const char *drvr, const char *attr )
{
	hWnd = windowHandle;
	jdbcDriver = drivers [0];
	serviceDb = enOpenDb;
	safeThread = "Y";

	if ( drvr )
		driver = drvr;

	if ( attr )
	{
		attributes = attr;
		dsn = getAttribute( SETUP_DSN );
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

void Setup::getParameters()
{
	description = getAttribute( SETUP_DESCRIPTION );

	client = getAttribute( SETUP_CLIENT );

	user = getAttribute( SETUP_USER );
	if ( user.IsEmpty() )
		user = getAttribute( KEY_DSN_UID );

	password = getAttribute( SETUP_PASSWORD );
	if ( password.IsEmpty() )
		password = getAttribute( KEY_DSN_PWD );

	jdbcDriver = getAttribute( SETUP_JDBC_DRIVER );
	if ( jdbcDriver.IsEmpty() )
		jdbcDriver = getAttribute( KEY_DSN_JDBC_DRIVER );
	if ( jdbcDriver.IsEmpty() )
		jdbcDriver = drivers [0];

	role = getAttribute( SETUP_ROLE );
	
	charset = getAttribute( SETUP_CHARSET );
	if ( charset.IsEmpty() )
		charset = getAttribute( KEY_DSN_CHARSET );

	readonlyTpb = getAttribute( SETUP_READONLY_TPB );
	nowaitTpb = getAttribute( SETUP_NOWAIT_TPB );
	dialect = getAttribute( SETUP_DIALECT );

	quoted = getAttribute( SETUP_QUOTED );
	if ( quoted.IsEmpty() )
		quoted = getAttribute( KEY_DSN_QUOTED );

	sensitive = getAttribute( SETUP_SENSITIVE );
	if ( sensitive.IsEmpty() )
		sensitive = getAttribute( KEY_DSN_SENSITIVE );

	autoQuoted = getAttribute( SETUP_AUTOQUOTED );
	if ( autoQuoted.IsEmpty() )
		autoQuoted = getAttribute( KEY_DSN_AUTOQUOTED );

	char chCheck = UPPER( *(const char*)readonlyTpb );
	
	if ( !IS_CHECK_YES( chCheck ) && !IS_CHECK_NO( chCheck ) )
		readonlyTpb = "N";

	chCheck = UPPER( *(const char*)nowaitTpb );
	
	if ( !IS_CHECK_YES( chCheck ) && !IS_CHECK_NO( chCheck ) )
		nowaitTpb = "N";

	chCheck = *(const char*)dialect;
	
	if ( chCheck != '1' && chCheck != '3' )
		dialect = "3";

	chCheck = UPPER( *(const char*)quoted );
	
	if ( !IS_CHECK_YES( chCheck ) && !IS_CHECK_NO( chCheck ) )
		quoted = "Y";

	chCheck = UPPER( *(const char*)sensitive );
	
	if ( !IS_CHECK_YES( chCheck ) && !IS_CHECK_NO( chCheck ) )
		sensitive = "N";

	chCheck = UPPER( *(const char*)autoQuoted );
	
	if ( !IS_CHECK_YES( chCheck ) && !IS_CHECK_NO( chCheck ) )
		autoQuoted = "N";

	pageSize = getAttribute( SETUP_PAGE_SIZE );
}

bool Setup::addDsn()
{
	getParameters();

	do
	{
		dbName = getAttribute( SETUP_DBNAME );
		if ( !dbName.IsEmpty() )
			break;
		
		dbName = getAttribute( KEY_DSN_DATABASE );
		if ( !dbName.IsEmpty() )
			break;

		dbName = getAttribute( KEY_DSN_CREATE_DB );
		if ( !dbName.IsEmpty() )
		{
			serviceDb = enCreateDb;
			break;
		}

		dbName = getAttribute( KEY_DSN_BACKUP_DB );
		if ( !dbName.IsEmpty() )
		{
			serviceDb = enBackupDb;
			break;
		}

		dbName = getAttribute( KEY_DSN_RESTORE_DB );
		if ( !dbName.IsEmpty() )
		{
			serviceDb = enRestoreDb;
			break;
		}

		dbName = getAttribute( KEY_DSN_REPAIR_DB );
		if ( !dbName.IsEmpty() )
			serviceDb = enRepairDb;

	} while ( false );

	if ( serviceDb )
	{
		ULONG serviceParameters;
		ULONG serviceOptions;
		JString logFileName;
		int lengthOut;
		int countError;
		char buffer[1024];
		CServiceClient services;

		if ( !services.initServices( jdbcDriver ) )
		{
			JString text;
			text.Format ( _TR( IDS_ERROR_MESSAGE_01, "Unable to connect to data source: library '%s' failed to load" ), (const char *)jdbcDriver );
			SQLPostInstallerError( ODBC_ERROR_CREATE_DSN_FAILED, (const char*)text );
			return false;
		}

		if ( !services.checkVersion() )
		{
			JString text;
			text.Format ( _TR( IDS_ERROR_MESSAGE_03, " Unable to load %s Library : can't find ver. %s " ), (const char *)jdbcDriver, DRIVER_VERSION );
			SQLPostInstallerError( ODBC_ERROR_CREATE_DSN_FAILED, (const char*)text );
			return false;
		}

		if ( !user.IsEmpty() )
			services.putParameterValue( SETUP_USER, user );
		if ( !password.IsEmpty() )
			services.putParameterValue( SETUP_PASSWORD, password );
		if ( !role.IsEmpty() )
			services.putParameterValue( SETUP_ROLE, role );
		if ( !dbName.IsEmpty() )
			services.putParameterValue( SETUP_DBNAME, dbName );
		if ( !client.IsEmpty() )
			services.putParameterValue( SETUP_CLIENT, client );

		logFileName = getAttribute( KEY_DSN_LOGFILE );
		if ( !logFileName.IsEmpty() && !services.openLogFile( logFileName ) )
		{
			JString text;
			text.Format ( " Unable to open log file %s ", (const char *)logFileName );
			SQLPostInstallerError( ODBC_ERROR_CREATE_DSN_FAILED, (const char*)text );
			return false;
		}

		switch ( serviceDb )
		{
		case enCreateDb:
			{
				if ( !charset.IsEmpty() )
					services.putParameterValue( KEY_DSN_CHARSET, charset );
				if ( !dialect.IsEmpty() )
					services.putParameterValue( SETUP_DIALECT, dialect );

				services.putParameterValue( FLAG_DATABASEACCESS, "1" );

				if ( !pageSize.IsEmpty() )
					services.putParameterValue( SETUP_PAGE_SIZE, pageSize );

				if ( !services.createDatabase() )
				{
					JString text;
					text.Format ( _TR( IDS_ERROR_MESSAGE_19, "Create database '%s' failed" ), (const char *)dbName );
					SQLPostInstallerError( ODBC_ERROR_CREATE_DSN_FAILED, (const char*)text );
					return false;
				}
			}
			return true;

		case enDropDb:
			{
				services.putParameterValue( FLAG_DATABASEACCESS, "2" );

				if ( !services.dropDatabase() )
				{
					JString text;
					text.Format ( _TR( IDS_ERROR_MESSAGE_19, "Drop database '%s' failed" ), (const char *)dbName );
					SQLPostInstallerError( ODBC_ERROR_CREATE_DSN_FAILED, (const char*)text );
					return false;
				}
			}
			return true;

		case enBackupDb:
			{
				JString backupFile;

				backupFile = getAttribute( KEY_DSN_BACKUPFILE );
				if ( backupFile.IsEmpty() )
				{
					JString text;
					text.Format ( "Is not specified backup file" );
					SQLPostInstallerError( ODBC_ERROR_CREATE_DSN_FAILED, (const char*)text );
					return false;
				}

				services.putParameterValue( KEY_DSN_BACKUPFILE, backupFile );

				try
				{
					countError = 0;
					services.openSemaphore( "OdbcJdbcBackup" );
					serviceParameters = 0; // default
					services.startBackupDatabase( serviceParameters );
					
					while ( services.nextQuery( buffer, sizeof ( buffer ), lengthOut, countError ) )
					{
						char *pt = buffer;

						if ( services.checkIncrementForBackup( pt ) )
							services.greenSemaphore();

						services.writeLogFile( buffer );
					}

					services.closeService();

					if ( countError )
					{
						JString text;
						text.Format ( "The backup file has errors %d. See log file.", countError );
						SQLPostInstallerError( ODBC_ERROR_CREATE_DSN_FAILED, (const char*)text );
						return false;
					}

					return true;
				}
				catch ( std::exception &ex )
				{
					char buffer[1024];
					SQLException &exception = (SQLException&)ex;
					JString text = exception.getText();
					sprintf( buffer, "sqlcode %d, fbcode %d - %s", exception.getSqlcode(), exception.getFbcode(), (const char*)text );
					SQLPostInstallerError( ODBC_ERROR_CREATE_DSN_FAILED, buffer );
				}
			}
			return false;

		case enRestoreDb:
			{
				JString backupFile;

				backupFile = getAttribute( KEY_DSN_BACKUPFILE );
				if ( backupFile.IsEmpty() )
				{
					JString text;
					text.Format ( "Is not specified backup file" );
					SQLPostInstallerError( ODBC_ERROR_CREATE_DSN_FAILED, (const char*)text );
					return false;
				}

				services.putParameterValue( KEY_DSN_BACKUPFILE, backupFile );

				try
				{
					char bufferHead[80];

					if ( !pageSize.IsEmpty() )
						services.putParameterValue( SETUP_PAGE_SIZE, pageSize );

					countError = 0;
					services.openSemaphore( "OdbcJdbcRestore" );
					serviceParameters = 0; // default
					services.startRestoreDatabase( serviceParameters );

					while ( services.nextQuery( buffer, sizeof ( buffer ), lengthOut, countError ) )
					{
						char *pt = buffer;

						if ( services.checkIncrementForRestore( pt, bufferHead ) )
							services.greenSemaphore();

						services.writeLogFile( buffer );
					}

					services.closeService();

					if ( countError )
					{
						JString text;
						text.Format ( "The restore file has errors %d. See log file.", countError );
						SQLPostInstallerError( ODBC_ERROR_CREATE_DSN_FAILED, (const char*)text );
						return false;
					}

					return true;
				}
				catch ( std::exception &ex )
				{
					char buffer[1024];
					SQLException &exception = (SQLException&)ex;
					JString text = exception.getText();
					sprintf( buffer, "sqlcode %d, fbcode %d - %s", exception.getSqlcode(), exception.getFbcode(), (const char*)text );
					SQLPostInstallerError( ODBC_ERROR_CREATE_DSN_FAILED, buffer );
				}
			}
			return false;

		case enRepairDb:
			{
				try
				{
					countError = 0;
					services.openSemaphore( "OdbcJdbcRepair" );
					serviceParameters = 0x0004; //enMendDb
					serviceOptions = 0x0020;    //enIgnoreChecksum

					services.startRepairDatabase( serviceParameters, serviceOptions );

					while ( services.nextQuery( buffer, sizeof ( buffer ), lengthOut, countError ) )
					{
						services.writeLogFile( buffer );
					}

					services.greenSemaphore();
					services.closeService();

					if ( countError )
					{
						JString text;
						text.Format ( "The repair file has errors %d. See log file.", countError );
						SQLPostInstallerError( ODBC_ERROR_CREATE_DSN_FAILED, (const char*)text );
						return false;
					}

					return true;
				}
				catch ( std::exception &ex )
				{
					char buffer[1024];
					SQLException &exception = (SQLException&)ex;
					JString text = exception.getText();
					sprintf( buffer, "sqlcode %d, fbcode %d - %s", exception.getSqlcode(), exception.getFbcode(), (const char*)text );
					SQLPostInstallerError( ODBC_ERROR_CREATE_DSN_FAILED, buffer );
				}
			}
			return false;
		}

		return false;
	}

	if ( hWnd || dsn.IsEmpty() )
		configureDialog();
	else if ( !SQLWriteDSNToIni(dsn, driver) )
	{
		MessageBoxInstallerError( _TR( IDS_ERROR_MESSAGE_17, "Config DSN" ), NULL );
		return false;
	}
	else
		writeAttributes();

	return true;
}

bool Setup::removeDsn()
{
	if ( !dsn.IsEmpty() )
		SQLRemoveDSNFromIni (dsn);

	getParameters();

	do
	{
		dbName = getAttribute( KEY_DSN_DROP_DB );
		if ( !dbName.IsEmpty() )
		{
			serviceDb = enDropDb;
			break;
		}

		dbName = getAttribute( KEY_DSN_BACKUP_DB );
		if ( !dbName.IsEmpty() )
		{
			serviceDb = enBackupDb;
			break;
		}

		dbName = getAttribute( KEY_DSN_RESTORE_DB );
		if ( !dbName.IsEmpty() )
		{
			serviceDb = enRestoreDb;
			break;
		}

		dbName = getAttribute( KEY_DSN_REPAIR_DB );
		if ( !dbName.IsEmpty() )
			serviceDb = enRepairDb;

	} while ( false );

	if ( serviceDb )
	{
		ULONG serviceParameters;
		ULONG serviceOptions;
		JString logFileName;
		int lengthOut;
		int countError;
		char buffer[1024];
		CServiceClient services;

		if ( !services.initServices( jdbcDriver ) )
		{
			JString text;
			text.Format ( _TR( IDS_ERROR_MESSAGE_01, "Unable to connect to data source: library '%s' failed to load" ), (const char *)jdbcDriver );
			SQLPostInstallerError( ODBC_ERROR_CREATE_DSN_FAILED, (const char*)text );
			return false;
		}

		if ( !services.checkVersion() )
		{
			JString text;
			text.Format ( _TR( IDS_ERROR_MESSAGE_03, " Unable to load %s Library : can't find ver. %s " ), (const char *)jdbcDriver, DRIVER_VERSION );
			SQLPostInstallerError( ODBC_ERROR_CREATE_DSN_FAILED, (const char*)text );
			return false;
		}

		if ( !user.IsEmpty() )
			services.putParameterValue( SETUP_USER, user );
		if ( !password.IsEmpty() )
			services.putParameterValue( SETUP_PASSWORD, password );
		if ( !role.IsEmpty() )
			services.putParameterValue( SETUP_ROLE, role );
		if ( !dbName.IsEmpty() )
			services.putParameterValue( SETUP_DBNAME, dbName );
		if ( !client.IsEmpty() )
			services.putParameterValue( SETUP_CLIENT, client );

		logFileName = getAttribute( KEY_DSN_LOGFILE );
		if ( !logFileName.IsEmpty() && !services.openLogFile( logFileName ) )
		{
			JString text;
			text.Format ( " Unable to open log file %s ", (const char *)logFileName );
			SQLPostInstallerError( ODBC_ERROR_CREATE_DSN_FAILED, (const char*)text );
			return false;
		}

		switch ( serviceDb )
		{
		case enDropDb:
			{
				services.putParameterValue( FLAG_DATABASEACCESS, "2" );

				if ( !services.dropDatabase() )
				{
					JString text;
					text.Format ( _TR( IDS_ERROR_MESSAGE_19, "Drop database '%s' failed" ), (const char *)dbName );
					SQLPostInstallerError( ODBC_ERROR_CREATE_DSN_FAILED, (const char*)text );
					return false;
				}
			}
			return true;

		case enBackupDb:
			{
				JString backupFile;

				backupFile = getAttribute( KEY_DSN_BACKUPFILE );
				if ( backupFile.IsEmpty() )
				{
					JString text;
					text.Format ( "Is not specified backup file" );
					SQLPostInstallerError( ODBC_ERROR_CREATE_DSN_FAILED, (const char*)text );
					return false;
				}

				services.putParameterValue( KEY_DSN_BACKUPFILE, backupFile );

				try
				{
					countError = 0;
					services.openSemaphore( "OdbcJdbcBackup" );
					serviceParameters = 0; // default
					services.startBackupDatabase( serviceParameters );
					
					while ( services.nextQuery( buffer, sizeof ( buffer ), lengthOut, countError ) )
					{
						char *pt = buffer;

						if ( services.checkIncrementForBackup( pt ) )
							services.greenSemaphore();

						services.writeLogFile( buffer );
					}

					services.closeService();

					if ( countError )
					{
						JString text;
						text.Format ( "The backup file has errors %d. See log file.", countError );
						SQLPostInstallerError( ODBC_ERROR_CREATE_DSN_FAILED, (const char*)text );
						return false;
					}

					return true;
				}
				catch ( std::exception &ex )
				{
					char buffer[1024];
					SQLException &exception = (SQLException&)ex;
					JString text = exception.getText();
					sprintf( buffer, "sqlcode %d, fbcode %d - %s", exception.getSqlcode(), exception.getFbcode(), (const char*)text );
					SQLPostInstallerError( ODBC_ERROR_CREATE_DSN_FAILED, buffer );
				}
			}
			return false;

		case enRestoreDb:
			{
				JString backupFile;

				backupFile = getAttribute( KEY_DSN_BACKUPFILE );
				if ( backupFile.IsEmpty() )
				{
					JString text;
					text.Format ( "Is not specified backup file" );
					SQLPostInstallerError( ODBC_ERROR_CREATE_DSN_FAILED, (const char*)text );
					return false;
				}

				services.putParameterValue( KEY_DSN_BACKUPFILE, backupFile );

				try
				{
					char bufferHead[80];

					if ( !pageSize.IsEmpty() )
						services.putParameterValue( SETUP_PAGE_SIZE, pageSize );

					countError = 0;
					services.openSemaphore( "OdbcJdbcRestore" );
					serviceParameters = 0; // default
					services.startRestoreDatabase( serviceParameters );

					while ( services.nextQuery( buffer, sizeof ( buffer ), lengthOut, countError ) )
					{
						char *pt = buffer;

						if ( services.checkIncrementForRestore( pt, bufferHead ) )
							services.greenSemaphore();

						services.writeLogFile( buffer );
					}

					services.closeService();

					if ( countError )
					{
						JString text;
						text.Format ( "The restore file has errors %d. See log file.", countError );
						SQLPostInstallerError( ODBC_ERROR_CREATE_DSN_FAILED, (const char*)text );
						return false;
					}

					return true;
				}
				catch ( std::exception &ex )
				{
					char buffer[1024];
					SQLException &exception = (SQLException&)ex;
					JString text = exception.getText();
					sprintf( buffer, "sqlcode %d, fbcode %d - %s", exception.getSqlcode(), exception.getFbcode(), (const char*)text );
					SQLPostInstallerError( ODBC_ERROR_CREATE_DSN_FAILED, buffer );
				}
			}
			return false;

		case enRepairDb:
			{
				try
				{
					countError = 0;
					services.openSemaphore( "OdbcJdbcRepair" );
					serviceParameters = 0x0004; //enMendDb
					serviceOptions = 0x0020;    //enIgnoreChecksum

					services.startRepairDatabase( serviceParameters, serviceOptions );

					while ( services.nextQuery( buffer, sizeof ( buffer ), lengthOut, countError ) )
					{
						services.writeLogFile( buffer );
					}

					services.greenSemaphore();
					services.closeService();

					if ( countError )
					{
						JString text;
						text.Format ( "The repair file has errors %d. See log file.", countError );
						SQLPostInstallerError( ODBC_ERROR_CREATE_DSN_FAILED, (const char*)text );
						return false;
					}

					return true;
				}
				catch ( std::exception &ex )
				{
					char buffer[1024];
					SQLException &exception = (SQLException&)ex;
					JString text = exception.getText();
					sprintf( buffer, "sqlcode %d, fbcode %d - %s", exception.getSqlcode(), exception.getFbcode(), (const char*)text );
					SQLPostInstallerError( ODBC_ERROR_CREATE_DSN_FAILED, buffer );
				}
			}
			return false;
		}

		return false;
	}

	return true;
}

JString Setup::getAttribute(const char * attribute)
{
	const char *p;
	int count = (int)strlen (attribute);

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
				for (q = p; !IS_END_TOKEN(*q); ++q)
					;
				return JString (p, q - p);
			}
		}
		while ( !IS_END_TOKEN(*p) )
			++p;
	}

	return JString();
}

bool Setup::configureDialog()
{
	if ( jdbcDriver.IsEmpty() )
		jdbcDriver = drivers [0];

	CDsnDialog dialog( hWnd ,drivers, charsets, useshemas );
	dialog.m_name = dsn;
	dialog.m_description = description;
	dialog.m_database = dbName;
	dialog.m_client = client;
	dialog.m_user = user;
	dialog.m_password = password;
	dialog.m_driver = jdbcDriver;
	dialog.m_role = role;
	dialog.m_charset = charset;
	dialog.m_useschema = useschema;
	dialog.m_locktimeout = locktimeout;

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

	if ( IS_CHECK_YES ( *(const char*)safeThread ) )
		dialog.m_safeThread = TRUE;
	else 
		dialog.m_safeThread = FALSE;

	do
	{
		intptr_t ret = dialog.DoModal();
		if ( ret != IDOK )
			return false;

		if ( SQLValidDSN( (const char *)dialog.m_name ) )
			break;

		if ( !MessageBoxInstallerError( _TR( IDS_ERROR_MESSAGE_17, "Config DSN" ), NULL ) )
		{
		    MessageBox( NULL, (const char*)_TR( IDS_ERROR_MESSAGE_18, 
						"Invalid characters are included \
						in the data source name: []{}(),;?*=!@\\" ),
						DRIVER_NAME, 
						MB_ICONSTOP | MB_OK );
		}

	} while ( true );

	if ( dsn != (const char *)dialog.m_name )
		SQLRemoveDSNFromIni( dsn );

	dsn = dialog.m_name;
	description = dialog.m_description;
	dbName = dialog.m_database;
	client = dialog.m_client;
	user = dialog.m_user;
	password = dialog.m_password;
	jdbcDriver = dialog.m_driver;
	role = dialog.m_role;
	charset = dialog.m_charset;
	useschema = dialog.m_useschema;
	locktimeout = dialog.m_locktimeout;

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

	if( dialog.m_safeThread ) safeThread = "Y";
	else safeThread = "N";

	if ( !SQLWriteDSNToIni( dialog.m_name, driver ) )
	{
		MessageBoxInstallerError( _TR( IDS_ERROR_MESSAGE_17, "Config DSN" ), NULL );
		return false;
	}
	else
		writeAttributes();

	return true;
}

void Setup::writeAttributes()
{
	writeAttribute( SETUP_DESCRIPTION, description );
	writeAttribute( SETUP_DBNAME, dbName );
	writeAttribute( SETUP_CLIENT, client );
	writeAttribute( SETUP_USER, user );
	writeAttribute( SETUP_ROLE, role );
	writeAttribute( SETUP_CHARSET, charset );
	writeAttribute( SETUP_JDBC_DRIVER, jdbcDriver );
	writeAttribute( SETUP_READONLY_TPB, readonlyTpb );
	writeAttribute( SETUP_NOWAIT_TPB, nowaitTpb );
	writeAttribute( SETUP_LOCKTIMEOUT, locktimeout );
	writeAttribute( SETUP_DIALECT, dialect );
	writeAttribute( SETUP_QUOTED, quoted );
	writeAttribute( SETUP_SENSITIVE, sensitive );
	writeAttribute( SETUP_AUTOQUOTED, autoQuoted );
	writeAttribute( SETUP_USESCHEMA, useschema );
	writeAttribute( SETUP_SAFETHREAD, safeThread );

	char buffer[256];
	CSecurityPassword security;
	security.encode( (char*)(const char *)password, buffer );
	writeAttribute( SETUP_PASSWORD, buffer );
}

void Setup::readAttributes()
{
	description = readAttribute( SETUP_DESCRIPTION );
	dbName = readAttribute( SETUP_DBNAME );
	client = readAttribute( SETUP_CLIENT );
	user = readAttribute( SETUP_USER );
	jdbcDriver = readAttribute( SETUP_JDBC_DRIVER );
	role = readAttribute( SETUP_ROLE );
	charset = readAttribute( SETUP_CHARSET );
	readonlyTpb = readAttribute( SETUP_READONLY_TPB );
	nowaitTpb = readAttribute( SETUP_NOWAIT_TPB );
	locktimeout = readAttribute( SETUP_LOCKTIMEOUT );
	dialect = readAttribute( SETUP_DIALECT );
	quoted = readAttribute( SETUP_QUOTED );
	sensitive = readAttribute( SETUP_SENSITIVE );
	autoQuoted = readAttribute( SETUP_AUTOQUOTED );
	useschema = readAttribute( SETUP_USESCHEMA );
	pageSize = 0;
	safeThread = readAttribute( SETUP_SAFETHREAD );

	JString pass = readAttribute( SETUP_PASSWORD );
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
	if (ret < 0) ret = 0;

	return JString (buffer, ret);
}

}; // end namespace OdbcJdbcSetupLibrary
