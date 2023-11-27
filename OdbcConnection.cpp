/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
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
 *	2003-03-24	OdbcConnection.cpp
 *				Contributed by Norbert Meyer
 *				delete Statements before close connection 
 *				(statement-destructor needs connection-pointer 
 *				for call connection->deleteStatement.  
 *				If connection->deleteStatement not called, 
 *				you get an AV if you use Statements and call 
 *				  SQLDisconnect(...); 
 *				  SQLFreeHandle(..., connection);
 *
 *	2002-12-05	OdbcConnection.cpp 
 *				Contributed by C. Guzman Alvarez
 *				SQLGetInfo returns more info.
 *				Solve error in SQL_ORDER_BY_COLUMNS_IN_SELECT.
 *
 *	2002-08-12	OdbcConnection.cpp 
 *				Contributed by C. G. Alvarez
 *				Added SQL_API_SQLGETCONNECTOPTION to list of 
 *				supported functions
 *
 *				Added more items to sqlGetInfo()
 *
 *  2002-07-02  OdbcConnection.cpp 
 *				Added better management of txn isolation
 *				Added fix to enable setting the asyncEnable property
 *				contributed by C. G. Alvarez
 *
 *  2002-07-01  OdbcConnection.cpp 
 *				Added SQL_API_SQLSETCONNECTOPTION to 
 *				supportedFunctions	C. G. Alvarez
 *
 *	2002-06-26	OdbcConnection::sqlGetInfo
 *				Added call to clearErrors() at start of 
 *				the method(Roger Gammans).
 *
 *	2002-06-25  OdbcConnection.cpp  
 *				Contributed by C. G. Alvarez
 *				Return Database Server Name from sqlGetInfo
 *
 *
 *	2002-06-08  OdbcConnection.cpp 
 *				Contributed by C. G. Alvarez
 *				sqlSetConnectAttr() and connect()
 *				now supports SQL_ATTR_TXN_ISOLATION
 *
 *
 *	2002-06-08	OdbcConnection.cpp
 *				Contributed by C. G. Alvarez
 *				Changed sqlDriverConnect() to better support
 *				Crystal Reports.
 */

// OdbcConnection.cpp: implementation of the OdbcConnection class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#ifndef _WINDOWS
#include <unistd.h>
#include <dlfcn.h>
#endif
#include <string.h>
#include "OdbcJdbc.h"
#include "OdbcEnv.h"
#include "OdbcConnection.h"
#include <odbcinst.h>
#include "IscDbc/Connection.h"
#include "IscDbc/SQLException.h"
#include "OdbcStatement.h"
#include "OdbcDesc.h"
#include "ConnectDialog.h"
#include "SecurityPassword.h"

#define SQL_FBGETPAGEDB			180
#define SQL_FBGETWALDB			181
#define SQL_FBGETSTATINFODB		182

#define BUILD_ODBC_VER(zero1,major,zero2,minor,zero3,build) zero1 #major "." zero2 #minor "." zero3 #build
#define STR_BUILD_ODBC_VER(zero1,major,zero2,minor,zero3,build) BUILD_ODBC_VER( zero1, major, zero2, minor, zero3, build )

#define ODBC_DRIVER_VERSION	"03.51.0000"
#define ODBC_VERSION_NUMBER	STR_BUILD_ODBC_VER( ZERO_MAJOR, MAJOR_VERSION, ZERO_MINOR, MINOR_VERSION, ZERO_BUILDNUM, BUILDNUM_VERSION )

namespace OdbcJdbcLibrary {

using namespace IscDbcLibrary;
using namespace classSecurityPassword;

static const int supportedFunctions [] = {
            // Deprecated but important stuff

            SQL_API_SQLALLOCCONNECT,
            SQL_API_SQLALLOCENV,
            SQL_API_SQLALLOCSTMT,
            SQL_API_SQLFREECONNECT,
            SQL_API_SQLFREEENV,
            SQL_API_SQLFREESTMT,
            SQL_API_SQLCOLATTRIBUTES,
            SQL_API_SQLERROR,
            SQL_API_SQLSETPARAM,
            SQL_API_SQLTRANSACT,
            SQL_API_SQLSETCONNECTOPTION,
            SQL_API_SQLGETCONNECTOPTION,

            SQL_API_SQLENDTRAN,

            SQL_API_SQLALLOCHANDLE,
            SQL_API_SQLGETDESCFIELD,
            SQL_API_SQLBINDCOL,
            SQL_API_SQLGETDESCREC,
            SQL_API_SQLCANCEL,
            SQL_API_SQLGETDIAGFIELD,
            SQL_API_SQLCLOSECURSOR,
            SQL_API_SQLGETDIAGREC,
            SQL_API_SQLCOLATTRIBUTE,
            SQL_API_SQLGETENVATTR,
            SQL_API_SQLCONNECT,
            SQL_API_SQLGETFUNCTIONS,
            SQL_API_SQLCOPYDESC,
            SQL_API_SQLGETINFO,
            SQL_API_SQLDATASOURCES,
            SQL_API_SQLGETSTMTATTR,
            SQL_API_SQLDESCRIBECOL,
            SQL_API_SQLGETTYPEINFO,
            SQL_API_SQLDISCONNECT,
            SQL_API_SQLNUMRESULTCOLS,
            SQL_API_SQLDRIVERS,
            SQL_API_SQLPARAMDATA,
            SQL_API_SQLPREPARE,
            SQL_API_SQLEXECDIRECT,
            SQL_API_SQLPUTDATA,
            SQL_API_SQLEXECUTE,
            SQL_API_SQLROWCOUNT,
            SQL_API_SQLFETCH,
            SQL_API_SQLSETCONNECTATTR,
            SQL_API_SQLFETCHSCROLL,
            SQL_API_SQLSETCURSORNAME,
            SQL_API_SQLFREEHANDLE,
            SQL_API_SQLSETDESCFIELD,
            SQL_API_SQLFREESTMT,
            SQL_API_SQLSETDESCREC,
            SQL_API_SQLGETCONNECTATTR,
            SQL_API_SQLSETENVATTR,
            SQL_API_SQLGETCURSORNAME,
            SQL_API_SQLSETSTMTATTR,
            SQL_API_SQLGETDATA,
			SQL_API_SQLSETSCROLLOPTIONS,

			SQL_API_SQLEXTENDEDFETCH,	//Deprecated ODBC 1.0 call. Replaced by SQL_API_SQLFETCHSCROLL in ODBC 3.0

            // The following is a list of valid values for FunctionId for functions conforming to the X/Open standards - compliance level,,

            SQL_API_SQLCOLUMNS,
            SQL_API_SQLSTATISTICS,
            SQL_API_SQLSPECIALCOLUMNS,
            SQL_API_SQLTABLES,

            //The following is a list of valid values for FunctionId for functions conforming to the ODBC standards - compliance level,,
            SQL_API_SQLBINDPARAMETER,
            SQL_API_SQLNATIVESQL,
            SQL_API_SQLBROWSECONNECT,
            SQL_API_SQLNUMPARAMS,
            SQL_API_SQLBULKOPERATIONS,
            SQL_API_SQLPRIMARYKEYS,
            SQL_API_SQLCOLUMNPRIVILEGES,
            SQL_API_SQLPROCEDURECOLUMNS,
            SQL_API_SQLDESCRIBEPARAM,
            SQL_API_SQLPROCEDURES,
            SQL_API_SQLDRIVERCONNECT,
            SQL_API_SQLSETPOS,
            SQL_API_SQLFOREIGNKEYS,
            SQL_API_SQLTABLEPRIVILEGES,
            SQL_API_SQLMORERESULTS,
        };

enum InfoType { infoNone, infoString, infoShort, infoLong, infoUnsupported };

struct TblInfoItem
{
	int			item;
#ifdef DEBUG
	const char	*name;
#endif
	InfoType	type;
	const char	*value;
};

#ifdef DEBUG
#define CITEM(item,value)	{item, #item, infoString, value},
#define SITEM(item,value)	{item, #item, infoShort, (char*) value},
#define NITEM(item,value)	{item, #item, infoLong, (char*) value},
#define UITEM(item,value)	{item, #item, infoUnsupported, (char*) value},
#define LAST_ITEM			{ 0, NULL, infoNone, NULL }
#else
#define CITEM(item,value)	{item, infoString, value},
#define SITEM(item,value)	{item, infoShort, (char*) value},
#define NITEM(item,value)	{item, infoLong, (char*) value},
#define UITEM(item,value)	{item, infoUnsupported, (char*) value},
#define LAST_ITEM			{ 0, infoNone, NULL }
#endif

static const TblInfoItem tblInfoItems [] = {
#include "InfoItems.h"
			LAST_ITEM
        };

struct InfoItem
{
#ifdef DEBUG
	const char	*name;
#endif
	InfoType	type;
	const char	*value;
};

#define INFO_SLOT(n)		((n < 10000) ? n : n - 10000 + 200)
#define INFO_SLOTS			400

static SQLUSMALLINT functionsArray [100];
static SQLSMALLINT  functionsBitmap [SQL_API_ODBC3_ALL_FUNCTIONS_SIZE];
static bool moduleInit();
static InfoItem infoItems [INFO_SLOTS];
static bool foo = moduleInit();

/***
#define SQL_FUNC_EXISTS(pfExists, uwAPI) \
				((*(((UWORD*) (pfExists)) + ((uwAPI) >> 4)) \
					& (1 << ((uwAPI) & 0x000F)) \
 				 ) ? SQL_TRUE : SQL_FALSE \
***/

bool moduleInit()
{
	for (unsigned int n = 0; n < sizeof (supportedFunctions) / sizeof (supportedFunctions [0]); ++n)
	{
		int fn = supportedFunctions [n];

		if ( fn < 100 )
		{
			//
			// SQL_API_ALL_FUNCTIONS is used by an ODBC 2.x application 
			// to determine support of ODBC 2.x and earlier functions
			//
			// where functionsArray should be 100 elements
			//
			functionsArray [fn] = SQL_TRUE;
		}

		ASSERT ((fn >> 4) < SQL_API_ODBC3_ALL_FUNCTIONS_SIZE);
		functionsBitmap [fn >> 4] |= 1 << (fn & 0xf);
	}

	for (const TblInfoItem *t = tblInfoItems; t->type; ++t)
	{
		int slot = INFO_SLOT (t->item);
		ASSERT (slot >= 0 && slot < INFO_SLOTS);
		InfoItem *item = infoItems + slot;
#ifdef DEBUG
		ASSERT (item->name == NULL);
		item->name = t->name;
#endif
		item->type = t->type;
		item->value = t->value;
	}

	bool test = SQL_FUNC_EXISTS (functionsBitmap, SQL_API_SQLALLOCHANDLE);

	return test;
}

static int getDriverBuildKey()
{
	return MAJOR_VERSION * 1000000 + MINOR_VERSION * 10000 + BUILDNUM_VERSION;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

OdbcConnection::OdbcConnection(OdbcEnv *parent)
{
	env					= parent;
	connected			= false;
	safeThread			= true;
	levelBrowseConnect	= 0;
	databaseAccess		= OPEN_DB;
	connectionTimeout	= 0;
	connection			= NULL;
	statements			= NULL;
	descriptors			= NULL;
	userEvents			= NULL;
	asyncEnabled		= SQL_ASYNC_ENABLE_OFF;
	autoCommit			= true;
	cursors				= SQL_CUR_USE_DRIVER; //Org
	statementNumber		= 0;
	accessMode			= SQL_MODE_READ_WRITE;
	transactionIsolation = SQL_TXN_READ_COMMITTED; //suggested by CGA.
	optTpb				= 0;
	defOptions			= 0;
	dialect3			= true;
	quotedIdentifier	= true;
	sensitiveIdentifier  = false;
	autoQuotedIdentifier = false;
	userEventsInterfase	= NULL;
	charsetCode			= 0; // NONE

#ifdef _WINDOWS
#if _MSC_VER > 1000
	enlistConnect = false;
#endif // _MSC_VER > 1000
	WcsToMbs			= _WcsToMbs;
	MbsToWcs			= _MbsToWcs;
#else
	WcsToMbs			= wcstombs;
	MbsToWcs			= mbstowcs;
#endif // _WINDOWS

}

OdbcConnection::~OdbcConnection()
{
	releaseObjects();	
}

void OdbcConnection::releaseObjects()
{
	if ( userEvents )
		userEvents->release();

	if ( userEventsInterfase )
		delete userEventsInterfase;

	while ( statements )
	{
		OdbcStatement* statement = statements;
		statements = (OdbcStatement*)statement->next;
		delete statement;
	}
	
	while ( descriptors )
	{
		OdbcDesc* descriptor = descriptors;
		descriptors = (OdbcDesc*)descriptor->next;
		delete descriptor;
	}

	if ( connection )
		connection->close();

	if ( env )
		env->connectionClosed (this);
}

OdbcConnection* OdbcConnection::getConnection()
{
	return this;
}

OdbcObjectType OdbcConnection::getType()
{
	return odbcTypeConnection;
}

SQLRETURN OdbcConnection::sqlSetConnectAttr( SQLINTEGER attribute, SQLPOINTER value, SQLINTEGER stringLength )
{
	clearErrors();

	switch ( attribute )
	{
#ifdef _WINDOWS
#if _MSC_VER > 1000

	case 1207: // SQL_ENLIST_IN_DTC Enlist connection in the DTC transaction

		if ( !IsInstalledMsTdsInterface() )
		{
			return sqlReturn( SQL_ERROR, 
							  "IM001", 
							  "Unable start DTC transaction : library 'xolehlp.dll' failed to load" );
		}

		enlistTransaction( value );
		autoCommit = false;
		if ( connection )
			connection->setAutoCommit( autoCommit );
		break;

#endif // _MSC_VER > 1000
#endif

	case SQL_ATTR_ANSI_APP:
		if ( (intptr_t) value == SQL_AA_FALSE )
			return sqlReturn (SQL_ERROR, "IM001", "Driver does not support this function");
		break;

	case SQL_ATTR_HANDLE_DBC_SHARE: // 4000
		if (connection)
		{
			if ( (intptr_t) value )
				connection->connectionToEnvShare();
			else
				connection->connectionFromEnvShare();
		}
		break;

	case SQL_ATTR_LOGIN_TIMEOUT:
		connectionTimeout = (intptr_t) value;
		break;

	case SQL_ATTR_AUTOCOMMIT:
		autoCommit = (intptr_t) value == SQL_AUTOCOMMIT_ON;
		if (connection)
			connection->setAutoCommit (autoCommit);
		break;

	case SQL_ATTR_ODBC_CURSORS:
		cursors = (intptr_t) value;
		break;

		//Added by CA
	case SQL_ATTR_TXN_ISOLATION:
		transactionIsolation = (intptr_t)value;
		if ( connection )
			connection->setTransactionIsolation( (intptr_t) value );
		break;

		//Added by CA
	case SQL_ATTR_ASYNC_ENABLE:
		asyncEnabled = (intptr_t) value;
		break;

	case SQL_ATTR_ACCESS_MODE:
		accessMode = (intptr_t)value;
		break;

	case SQL_FB_INIT_EVENTS:
		if ( !connection )
			return sqlReturn( SQL_ERROR, "08003", "Connection does not exist" );

		if ( stringLength != sizeof ( ODBC_EVENTS_BLOCK_INFO ) )
			return sqlReturn( SQL_ERROR, "01S02", "Option value changed" );

		initUserEvents( (PODBC_EVENTS_BLOCK_INFO)value );
		break;

	case SQL_FB_UPDATECOUNT_EVENTS:
		if ( !connection )
			return sqlReturn( SQL_ERROR, "08003", "Connection does not exist" );

		if ( !userEventsInterfase )
			return sqlReturn( SQL_ERROR, "01S02", "Option value changed" );

		updateResultEvents( (char*)value );
		break;

	case SQL_FB_REQUEUE_EVENTS:

		if ( !userEventsInterfase )
			return sqlReturn( SQL_ERROR, "01S02", "Option value changed" );

		requeueEvents();
		break;
	}

	return sqlSuccess();
}

SQLRETURN OdbcConnection::sqlDriverConnect(SQLHWND hWnd, const SQLCHAR * connectString, int connectStringLength, SQLCHAR * outConnectBuffer, int connectBufferLength, SQLSMALLINT * outStringLength, int driverCompletion)
{
	clearErrors();

	if (connected)
		return sqlReturn (SQL_ERROR, "08002", "Connection name is use");

	if (connectStringLength < 0 && connectStringLength != SQL_NTS)
		return sqlReturn (SQL_ERROR, "HY090", "Invalid string or buffer length");

	switch (driverCompletion)
	{
	case SQL_DRIVER_COMPLETE:
	case SQL_DRIVER_COMPLETE_REQUIRED:
		break;

	case SQL_DRIVER_PROMPT:
		if (hWnd == NULL)
			return sqlReturn (SQL_ERROR, "HY092", "Invalid attribute/option identifier");
		break;
	}

	int length = stringLength (connectString, connectStringLength);
	const char *end = (const char*) connectString + length;
	JString driver = DRIVER_FULL_NAME;

	for (const char *p = (const char*) connectString; p < end;)
	{
		char name [256];
		char value [256];
		int nameLength;
		char *q = name;
		char c;
		while (p < end && (c = *p++) && c != '=' && c != ';')
			*q++ = c;
		*q = 0;
		nameLength = q - name;

		if ( !nameLength )
			continue;

		q = value;
		if (c == '=')
		{
			char term = ';';

			while (p < end && *p == ' ') p++;

			if (*p == '{')
			{
				term = '}';
				p++;
			}

			while (p < end && (c = *p++) != term)
				*q++ = c;

			if (term != ';')
				while (p < end && (c = *p++) != ';');
		}
		*q = 0;

#		define IS_KEYWORD(keyword) \
				( nameLength == LEN_KEY( keyword ) && !strncasecmp( name, keyword, LEN_KEY( keyword ) ) )

		if ( IS_KEYWORD( SETUP_DSN ) )
			dsn = value;
		else if ( IS_KEYWORD( SETUP_DESCRIPTION ) )
			description = value;
		else if ( IS_KEYWORD( KEY_FILEDSN ) )
			filedsn = value;
		else if ( IS_KEYWORD( KEY_SAVEDSN ) )
			savedsn = value;
		else if ( IS_KEYWORD( KEY_DSN_DATABASE ) || IS_KEYWORD( SETUP_DBNAME ) )
			databaseName = value;
		else if ( IS_KEYWORD( SETUP_DBNAMEALWAYS ) || IS_KEYWORD( KEY_DSN_CREATE_DB ) )
		{
			databaseName = value;
			databaseAccess = CREATE_DB;
		}
		else if ( IS_KEYWORD( KEY_DSN_DROP_DB ) )
		{
			databaseName = value;
			databaseAccess = DROP_DB;
		}
		else if ( IS_KEYWORD( SETUP_PAGE_SIZE ) )
			pageSize = value;
		else if ( IS_KEYWORD( SETUP_TIMEOUT ) )
			connectionTimeout = atoi( value );
		else if ( IS_KEYWORD( SETUP_CLIENT ) )
			client = value;
		else if ( IS_KEYWORD( KEY_DSN_UID ) || IS_KEYWORD( SETUP_USER ) )
			account = value;
		else if ( IS_KEYWORD( KEY_DSN_PWD )	|| IS_KEYWORD( SETUP_PASSWORD ) )
		{
			if ( strlen(value) > 40 )
			{
				char buffer[256];
				CSecurityPassword security;
				security.decode( value, buffer );
				password = buffer;
			}
			else
				password = value;
		}
		else if ( IS_KEYWORD( SETUP_ROLE ) )
			role = value;
		else if ( IS_KEYWORD( KEY_DSN_CHARSET ) || IS_KEYWORD( SETUP_CHARSET ) )
			charset = value;
		else if ( IS_KEYWORD( SETUP_DRIVER ) )
			driver = value;
		else if ( IS_KEYWORD( KEY_DSN_JDBC_DRIVER ) || IS_KEYWORD( SETUP_JDBC_DRIVER ) )
			jdbcDriver = value;
		else if ( IS_KEYWORD( SETUP_READONLY_TPB ) )
		{
			if( *value == 'Y')
				optTpb |=TRA_ro;

			defOptions |= DEF_READONLY_TPB;
		}
		else if ( IS_KEYWORD( SETUP_DIALECT ) )
		{
			if( *value == '1')
				dialect3 = false;

			defOptions |= DEF_DIALECT;
		}
		else if ( IS_KEYWORD( SETUP_NOWAIT_TPB ) )
		{
			if( *value == 'Y')
				optTpb |=TRA_nw;

			defOptions |= DEF_NOWAIT_TPB;
		}
		else if ( IS_KEYWORD( KEY_DSN_QUOTED ) || IS_KEYWORD( SETUP_QUOTED ) )
		{
			if( *value == 'N')
				quotedIdentifier = false;

			defOptions |= DEF_QUOTED;
		}
		else if ( IS_KEYWORD( KEY_DSN_SENSITIVE ) || IS_KEYWORD( SETUP_SENSITIVE ) )
		{
			if( *value == 'Y')
				sensitiveIdentifier = true;

			defOptions |= DEF_SENSITIVE;
		}
		else if ( IS_KEYWORD( KEY_DSN_AUTOQUOTED ) || IS_KEYWORD( SETUP_AUTOQUOTED ) )
		{
			if( *value == 'Y')
				autoQuotedIdentifier = true;

			defOptions |= DEF_AUTOQUOTED;
		}
		else if ( IS_KEYWORD( KEY_DSN_USESCHEMA ) || IS_KEYWORD( SETUP_USESCHEMA ) )
			useSchemaIdentifier = value;
		else if ( IS_KEYWORD( KEY_DSN_LOCKTIMEOUT ) || IS_KEYWORD( SETUP_LOCKTIMEOUT ) )
			useLockTimeoutWaitTransactions = value;
		else if ( IS_KEYWORD( KEY_DSN_SAFETHREAD ) || IS_KEYWORD( SETUP_SAFETHREAD ) )
		{
			if( *value == 'N')
				safeThread = false;

			defOptions |= DEF_SAFETHREAD;
		}
		else if ( IS_KEYWORD( "ODBC" ) )
			;
		else
			postError ("01S00", "Invalid connection string attribute");
	}

	expandConnectParameters();

	char returnString [1024], *r = returnString;

	*r = '\0';

	if (!dsn.IsEmpty())
	{
		r = appendString (r, SETUP_DSN"=");
		r = appendString (r, dsn);
	}

	if (!driver.IsEmpty())
	{
		if ( r > returnString )
			r = appendString (r, ";" SETUP_DRIVER"=");
		else
			r = appendString (r, SETUP_DRIVER"=");
		r = appendString (r, driver);
	}

	if (!databaseName.IsEmpty())
	{
		r = appendString (r, ";" SETUP_DBNAME"=");
		r = appendString (r, databaseName);
	}

	if (!charset.IsEmpty())
	{
		r = appendString (r, ";" KEY_DSN_CHARSET"=");
		r = appendString (r, charset);
	}

#ifdef _WINDOWS
	if ( driverCompletion != SQL_DRIVER_NOPROMPT 
		&& ( account.IsEmpty() || password.IsEmpty() ) )
	{
		CConnectDialog dlg;
		dlg.m_user = account;
		dlg.m_password = password;
		dlg.m_role = role;

		if ( IDOK != dlg.DoModal() )
		{
			postError ("28000", "Invalid authorization specification");
			return SQL_ERROR;
		}

		account = dlg.m_user;
		password = dlg.m_password;
		role = dlg.m_role;
	}
#endif // _WINDOWS

	SQLRETURN ret = connect (jdbcDriver, databaseName, account, password, role, charset);

	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
		return ret;

	if ( levelBrowseConnect )
		levelBrowseConnect = 0;

	if ( outConnectBuffer && connectBufferLength )
	{
		if (!password.IsEmpty())
		{
			r = appendString (r, ";" KEY_DSN_PWD"=");

			if ( connection->getUseAppOdbcVersion() == SQL_OV_ODBC3 )
			{
				char buffer[256];
				CSecurityPassword security;
				security.encode( (char*)(const char *)password, buffer );
				r = appendString (r, buffer);
			}
			else
				r = appendString (r, password);
		}

		if (!account.IsEmpty())
		{
			r = appendString (r, ";" KEY_DSN_UID"=");
			r = appendString (r, account);
		}

		if (!role.IsEmpty())
		{
			r = appendString (r, ";" SETUP_ROLE"=");
			r = appendString (r, role);
		}

		if (!client.IsEmpty())
		{
			r = appendString (r, ";" SETUP_CLIENT"=");
			r = appendString (r, client);
		}

		if (!filedsn.IsEmpty())
		{
			r = appendString (r, ";" KEY_FILEDSN"=");
			r = appendString (r, filedsn);
		}

		*r++ = ';';
		*r = '\0';

		if (setString ((UCHAR*) returnString, r - returnString, outConnectBuffer, connectBufferLength, outStringLength))
			postError ("01004", "String data, right truncated");
	}

	if (!savedsn.IsEmpty())
		saveConnectParameters();

	return sqlSuccess();
}

SQLRETURN OdbcConnection::sqlBrowseConnect(SQLCHAR * inConnectionString, SQLSMALLINT stringLength1, 
										 SQLCHAR * outConnectionString, SQLSMALLINT bufferLength, 
										 SQLSMALLINT * stringLength2Ptr)
{
	bool bFullConnectionString = false;
	clearErrors();

	if ( !levelBrowseConnect && connected)
		return sqlReturn (SQL_ERROR, "08002", "Connection name is use");

	if (stringLength1 < 0 && stringLength1 != SQL_NTS)
		return sqlReturn (SQL_ERROR, "HY090", "Invalid string or buffer length");

	int length = stringLength (inConnectionString, stringLength1);
	const char *end = (const char*) inConnectionString + length;
	JString driver = DRIVER_FULL_NAME;

	levelBrowseConnect = 0;

	for (const char *p = (const char*) inConnectionString; p < end;)
	{
		char name [256];
		char value [256];
		char *q = name;
		char c;

		while (p < end && ((c = *p),c == ' ' || c == '*'))
			++p;

		while (p < end && (c = *p++),c != '=' && c != ';')
			*q++ = c;

		*q = 0;
		q = value;
		if (c == '=')
			while (p < end && (c = *p++) != ';')
				*q++ = c;
		*q = 0;
		if (!strncasecmp (name, "DSN", 3))
		{
			if (dsn != (const char *)value)
			{
				dsn = value;
				levelBrowseConnect = 1;
				break;
			}
		}
		else if (!strncasecmp (name, "DRIVER", 6))
		{
			if (driver != (const char *)value)
			{
				driver = value;
				levelBrowseConnect = 1;
				break;
			}
		}
		else if (!strncasecmp (name, "JDBC_DRIVER", 11))
		{
			if (jdbcDriver != (const char *)value)
			{
				jdbcDriver = value;
				levelBrowseConnect = 1;
				break;
			}
		}
		else if (!strncasecmp (name, "DBNAME", 6))
		{
			levelBrowseConnect = 3;
			databaseName = value;
			bFullConnectionString = true;
		}
		else if (!strncasecmp (name, "UID", 3))
		{
			account = value;
			levelBrowseConnect = 2;
		}
		else if (!strncasecmp (name, "PWD", 3))
		{
			password = value;
			levelBrowseConnect = 2;
		}
		else if (!strncasecmp (name, "ROLE", 4))
		{
			role = value;
			levelBrowseConnect = 2;
		}
		else if (!strncasecmp (name, "CHARSET", 7))
		{
			charset = value;
			levelBrowseConnect = 2;
		}
		else if (!strncasecmp (name, "ODBC", 4))
			;
		else
			postError ("01S00", "Invalid connection string attribute");
	}

	if( levelBrowseConnect == 1 )
		expandConnectParameters();

	char returnString [1024], *r = returnString;
	*r = '\0';

	switch ( levelBrowseConnect )
	{
	case 1:
		r = appendString (r, "*ROLE=");
		if (!role.IsEmpty())
			r = appendString (r, role);
		else
			r = appendString (r, "?");

		r = appendString (r, "*CHARSET=");
		if (!charset.IsEmpty())
			r = appendString (r, charset);
		else
			r = appendString (r, "?");

		r = appendString (r, ";UID=");
		if (!account.IsEmpty())
			r = appendString (r, account);
		else
			r = appendString (r, "?");

		r = appendString (r, ";PWD=");
		if (!password.IsEmpty())
			r = appendString (r, password);
		else
			r = appendString (r, "?");

		break;

	case 2:
		r = appendString (r, "DBNAME=");
		if (!databaseName.IsEmpty())
			r = appendString (r, databaseName);
		else
			r = appendString (r, "?");
		break;

	case 3:
		if (!dsn.IsEmpty())
		{
			r = appendString (r, "DSN=");
			r = appendString (r, dsn);
		}

		if (!driver.IsEmpty())
		{
			r = appendString (r, ";DRIVER=");
			r = appendString (r, driver);
		}

		if (!role.IsEmpty())
		{
			r = appendString (r, ";ROLE=");
			r = appendString (r, role);
		}

		if (!charset.IsEmpty())
		{
			r = appendString (r, ";CHARSET=");
			r = appendString (r, charset);
		}

		if (!account.IsEmpty())
		{
			r = appendString (r, ";UID=");
			r = appendString (r, account);
		}

		if (!password.IsEmpty())
		{
			r = appendString (r, ";PWD=");
			r = appendString (r, password);
		}

		if (!databaseName.IsEmpty())
		{
			r = appendString (r, ";DBNAME=");
			r = appendString (r, databaseName);
		}
		break;
	}

	if ( outConnectionString && bufferLength )
	{
		if (setString ((UCHAR*) returnString, r - returnString, outConnectionString, bufferLength, stringLength2Ptr))
			postError ("01004", "String data, right truncated");
	}

	if ( bFullConnectionString == false )
		return SQL_NEED_DATA;
	else
	{
		SQLRETURN ret = connect (jdbcDriver, databaseName, account, password, role, charset);

		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
			return ret;
	}

	return sqlSuccess();
}

SQLRETURN OdbcConnection::sqlNativeSql( SQLCHAR * inStatementText, SQLINTEGER textLength1,
										SQLCHAR * outStatementText, SQLINTEGER bufferLength,
										SQLINTEGER * textLength2Ptr )
{
	clearErrors();

	if ( !inStatementText )
		return sqlReturn( SQL_ERROR, "HY009", "Invalid use of null pointer" );

	if ( textLength1 == SQL_NTS )
		textLength1 = (SQLINTEGER)strlen( (const char *)inStatementText );
	else if ( textLength1 < 0 )
		return sqlReturn( SQL_ERROR, "HY090", "Invalid string or buffer length" );

	JString tempNative;
	int textLength = textLength1 + 4096;
	const char * outText;
	SQLRETURN ret = SQL_SUCCESS;

	try
	{
		if ( !connection->getNativeSql( (const char *)inStatementText, textLength1, 
							tempNative.getBuffer ( textLength ), textLength, &textLength ) )
		{
			textLength = textLength1;
			outText = (const char *)inStatementText;
		}
		else
			outText = (const char *)tempNative;

	}
	catch ( std::exception &ex )
	{
		SQLException &exception = (SQLException&)ex;
		postError( "HY000", exception );
		return SQL_ERROR;
	}

	if( textLength2Ptr )
		*textLength2Ptr = textLength;

	if ( outStatementText )
	{
		if ( textLength >= bufferLength )
		{
			textLength = bufferLength - 1;
			postError( "01004", "String data, right truncated" );
			ret = SQL_SUCCESS_WITH_INFO;
		}

		memcpy( outStatementText, outText, textLength );
		outStatementText[textLength] = '\0';
	}

	return ret;
}

JString OdbcConnection::readAttribute(const char * attribute)
{
	char buffer [256];

	int ret = SQLGetPrivateProfileString (dsn, attribute, "", buffer, sizeof (buffer), env->odbcIniFileName);
	if (ret < 0) ret = 0;

	return JString (buffer, ret);
}

JString OdbcConnection::readAttributeFileDSN(const char * attribute)
{
	char buffer [256];
	unsigned short ret;

	if ( SQLReadFileDSN (filedsn, "ODBC", attribute, buffer, sizeof (buffer), &ret) )
		return JString (buffer, ret);

	return JString ("", 0);
}

void OdbcConnection::writeAttributeFileDSN(const char * attribute, const char * value)
{
#ifdef _IODBCUNIX_H
	// note: (char*)value - only for iODBC from Linux
	SQLWriteFileDSN (savedsn, "ODBC", attribute, (char*)value);
#else
	SQLWriteFileDSN (savedsn, "ODBC", attribute, value);
#endif
}

SQLRETURN OdbcConnection::sqlGetFunctions(SQLUSMALLINT functionId, SQLUSMALLINT * supportedPtr)
{
	clearErrors();

	switch (functionId)
	{
	case SQL_API_ODBC3_ALL_FUNCTIONS:
		memcpy (supportedPtr, functionsBitmap, sizeof (functionsBitmap));
		return sqlSuccess();

	case SQL_API_ALL_FUNCTIONS:
		memcpy (supportedPtr, functionsArray, sizeof (functionsArray));
		return sqlSuccess();
	}

	/***
	if (functionId >= 0 && functionId < SQL_API_ODBC3_ALL_FUNCTIONS_SIZE  * 16)
		return sqlReturn (SQL_ERROR, "HY095", "Function type out of range");
	***/

	*supportedPtr = (SQL_FUNC_EXISTS (functionsBitmap, functionId)) ? SQL_TRUE : SQL_FALSE;

	return sqlSuccess();
}

SQLRETURN OdbcConnection::sqlDisconnect()
{
	clearErrors();

	if (!connected)
	{
		if ( levelBrowseConnect )
		{
			levelBrowseConnect = 0;
			return sqlSuccess();
		}
		return sqlReturn (SQL_ERROR, "08003", "Connection does not exist");
	}

	if (connection->getTransactionPending())
		return sqlReturn (SQL_ERROR, "25000", "Invalid transaction state");

	try
	{
		connection->commit();
		releaseObjects();
		connection = NULL;
		connected = false;
	}
	catch ( std::exception &ex )
	{
		SQLException &exception = (SQLException&)ex;
		postError ("01002", exception);
		connection = NULL;
		connected = false;
		return SQL_SUCCESS_WITH_INFO;
	}

	return sqlSuccess();
}

SQLRETURN OdbcConnection::sqlGetInfo( SQLUSMALLINT type, SQLPOINTER ptr, SQLSMALLINT maxLength, SQLSMALLINT * actualLength )
{
	clearErrors();

#ifdef DEBUG
	char temp [256];
#endif
	int slot = INFO_SLOT (type);
	InfoItem *item = infoItems + slot;
	int n;

	if ( slot < 0 || slot >= INFO_SLOTS || item->type == infoNone )
		return sqlReturn (SQL_ERROR, "HY096", "Information type out of range");

	const char *string = item->value;
	uintptr_t value = (uintptr_t) item->value;
	DatabaseMetaData *metaData = NULL;

	if (connection)
		metaData = connection->getMetaData();
	else
		switch (type)
		{
		case SQL_ODBC_VER:
		case SQL_DRIVER_ODBC_VER:
		case SQL_ODBC_API_CONFORMANCE:
			break;

		default:
			return sqlReturn (SQL_ERROR, "08003", "Connection does not exist");
		}

	switch (type)
	{
	case SQL_FBGETPAGEDB:
		metaData->getSqlStrPageSizeBd(ptr,maxLength,actualLength);
		return SQL_SUCCESS;

	case SQL_FBGETWALDB:
		metaData->getSqlStrWalInfoBd(ptr,maxLength,actualLength);
		return SQL_SUCCESS;

	case SQL_FBGETSTATINFODB:
		metaData->getStrStatInfoBd(ptr,maxLength,actualLength);
		return SQL_SUCCESS;

	case SQL_CURSOR_COMMIT_BEHAVIOR:
		if (metaData->supportsOpenCursorsAcrossCommit())
			value = SQL_CB_PRESERVE;
		else if (metaData->supportsOpenStatementsAcrossCommit())
			value = SQL_CB_CLOSE;
		else
			value = SQL_CB_DELETE;
		break;

	case SQL_CURSOR_ROLLBACK_BEHAVIOR:
		if (metaData->supportsOpenCursorsAcrossRollback())
			value = SQL_CB_PRESERVE;
		else if (metaData->supportsOpenStatementsAcrossRollback())
			value = SQL_CB_CLOSE;
		else
			value = SQL_CB_DELETE;
		break;

	case SQL_IDENTIFIER_QUOTE_CHAR:
		string = metaData->getIdentifierQuoteString();
		break;

	case SQL_DATABASE_NAME:
		string = databaseName;
		break;

	case SQL_CATALOG_LOCATION:
		value = (metaData->isCatalogAtStart()) ? SQL_CL_START : SQL_CL_END;
		break;

	case SQL_CORRELATION_NAME:
		value = (metaData->supportsTableCorrelationNames()) ? SQL_CN_ANY : SQL_CN_NONE;
		break;

	case SQL_COLUMN_ALIAS:
		string = (metaData->supportsColumnAliasing()) ? "Y" : "N";
		break;

	case SQL_CATALOG_NAME:
		string = metaData->getCatalogTerm();
		string = (string [0]) ? "Y" : "N";
		break;

	case SQL_CATALOG_TERM:
		string = metaData->getCatalogTerm();
		break;

	case SQL_CATALOG_NAME_SEPARATOR:
		string = metaData->getCatalogSeparator();
		break;

	case SQL_CATALOG_USAGE:
		if (metaData->supportsCatalogsInDataManipulation())
			value |= SQL_CU_DML_STATEMENTS;
		if (metaData->supportsCatalogsInTableDefinitions())
			value |= SQL_CU_TABLE_DEFINITION;
		if (metaData->supportsCatalogsInIndexDefinitions())
			value |= SQL_CU_INDEX_DEFINITION;
		if (metaData->supportsCatalogsInPrivilegeDefinitions())
			value |= SQL_CU_PRIVILEGE_DEFINITION;
		if (metaData->supportsCatalogsInProcedureCalls())
			value |= SQL_CU_PROCEDURE_INVOCATION;
		break;

	case SQL_SCHEMA_USAGE:
		if (metaData->supportsSchemasInDataManipulation())
			value |= SQL_SU_DML_STATEMENTS;
		if (metaData->supportsSchemasInProcedureCalls())
			value |= SQL_SU_PROCEDURE_INVOCATION;
		if (metaData->supportsSchemasInTableDefinitions())
			value |= SQL_SU_TABLE_DEFINITION;
		if (metaData->supportsCatalogsInIndexDefinitions())
			value |= SQL_SU_INDEX_DEFINITION;
		if (metaData->supportsSchemasInPrivilegeDefinitions())
			value |= SQL_SU_PRIVILEGE_DEFINITION;
		break;

	case SQL_SERVER_NAME:
		if ( databaseServerName.IsEmpty() )
			databaseServerName = metaData->getDatabaseServerName();
		string = databaseServerName;
		break;

	case SQL_DATA_SOURCE_NAME:
		string = dsn;
		break;

	case SQL_DATA_SOURCE_READ_ONLY:
		string = (metaData->isReadOnly()) ? "Y" : "N";
		break;

	case SQL_DBMS_NAME:
		string = metaData->getDatabaseProductName();
		break;

	case SQL_DBMS_VER:
		string = metaData->getDatabaseProductVersion();
		break;

	case SQL_DEFAULT_TXN_ISOLATION:
		value = metaData->getDefaultTransactionIsolation();
		break;

	case SQL_DESCRIBE_PARAMETER:
		string = (metaData->supportsStatementMetaData()) ? "Y" : "N";
		break;

	case SQL_DRIVER_HDBC:
		value = (uintptr_t) this;
		break;

	case SQL_DRIVER_HENV:
		value = (uintptr_t) env;
		break;

	case SQL_USER_NAME:
		string = account;
		break;

	case SQL_SCHEMA_TERM:
		string = metaData->getSchemaTerm();
		break;

	case SQL_SQL_CONFORMANCE:
		{
			if ( metaData->supportsANSI92EntryLevelSQL() )
				value = SQL_SC_SQL92_ENTRY;	// Entry level SQL-92 compliant.
			if ( metaData->supportsANSI92IntermediateSQL() )
				value = SQL_SC_SQL92_INTERMEDIATE; // Intermediate level SQL-92 compliant.
			if ( metaData->supportsANSI92FullSQL() )
				value = SQL_SC_SQL92_FULL; // Full level SQL-92 compliant.
		}
		break;

	case SQL_ODBC_SQL_CONFORMANCE:
		{
			if ( metaData->supportsMinimumSQLGrammar() )
				value = SQL_OSC_MINIMUM;
			if ( metaData->supportsCoreSQLGrammar() )
				value = SQL_OSC_CORE;
			if ( metaData->supportsExtendedSQLGrammar() )
				value = SQL_OSC_EXTENDED;
		}
		break;

	case SQL_KEYWORDS:
		string = metaData->getSQLKeywords();
		break;

	case SQL_SPECIAL_CHARACTERS:
		string = metaData->getExtraNameCharacters();
		break;

	case SQL_LIKE_ESCAPE_CLAUSE:
		string = metaData->supportsLikeEscapeClause() ? "Y" : "N";
		break;

	case SQL_TXN_ISOLATION_OPTION:
		for (n = SQL_TXN_READ_UNCOMMITTED; n <= SQL_TXN_SERIALIZABLE; n *= 2)
			if (metaData->supportsTransactionIsolationLevel (n))
				value |= n;
		break;

	case SQL_MAX_DRIVER_CONNECTIONS:
		value = metaData->getMaxConnections();
		break;

	case SQL_MAX_COLUMN_NAME_LEN:
		value = metaData->getMaxColumnNameLength();
		break;

	case SQL_MAX_TABLE_NAME_LEN:
		value = metaData->getMaxTableNameLength();
		break;

	case SQL_MAX_CURSOR_NAME_LEN:
		value = metaData->getMaxCursorNameLength();
		break;

	case SQL_MAX_USER_NAME_LEN:
		value = metaData->getMaxUserNameLength();
		break;

	case SQL_MAX_COLUMNS_IN_INDEX:
		value = metaData->getMaxColumnsInIndex();
		break;

	case SQL_MAX_COLUMNS_IN_TABLE:
		value = metaData->getMaxColumnsInTable();
		break;

	case SQL_MAX_INDEX_SIZE:
		value = metaData->getMaxIndexLength();
		break;

	case SQL_MAX_SCHEMA_NAME_LEN:
		value = metaData->getMaxSchemaNameLength();
		break;

	case SQL_MAX_CATALOG_NAME_LEN:
		value = metaData->getMaxCatalogNameLength();
		break;

	case SQL_MAX_ROW_SIZE:
		value = metaData->getMaxRowSize();
		break;

	case SQL_MAX_ROW_SIZE_INCLUDES_LONG:
		string = metaData->doesMaxRowSizeIncludeBlobs() ? "Y" : "N";
		break;

	case SQL_MAX_TABLES_IN_SELECT:
		value = metaData->getMaxTablesInSelect();
		break;

	case SQL_GROUP_BY:
		if ( metaData->supportsGroupBy() )
			value = SQL_GB_GROUP_BY_CONTAINS_SELECT;
		else
			value = SQL_GB_NOT_SUPPORTED;
		break;

	case SQL_SEARCH_PATTERN_ESCAPE:
		if ( metaData->supportsLikeEscapeClause() )
			string = metaData->getSearchStringEscape();
		else
			string = "";
			break;

	case SQL_PROCEDURES:
		string = metaData->supportsStoredProcedures() ? "Y" : "N";
		break;

	case SQL_MAX_PROCEDURE_NAME_LEN:
		value = metaData->getMaxProcedureNameLength();
		break;

	case SQL_SUBQUERIES:
		{
			if ( metaData->supportsSubqueriesInComparisons() )
				value |= SQL_SQ_COMPARISON;
			if ( metaData->supportsSubqueriesInExists() )
				value |= SQL_SQ_EXISTS;
			if ( metaData->supportsSubqueriesInIns() )
				value |= SQL_SQ_IN;
			if ( metaData->supportsSubqueriesInQuantifieds() )
				value |= SQL_SQ_QUANTIFIED;
			if ( metaData->supportsCorrelatedSubqueries() )
				value |= SQL_SQ_CORRELATED_SUBQUERIES;
		}
		break;

	case SQL_UNION:
		{
			if ( metaData->supportsUnion() )
				value |= SQL_U_UNION;
			if ( metaData->supportsUnionAll() )
				value |= SQL_U_UNION_ALL;
		}
		break;

	case SQL_EXPRESSIONS_IN_ORDERBY:
		string = metaData->supportsExpressionsInOrderBy() ? "Y" : "N";
		break;

	case SQL_ORDER_BY_COLUMNS_IN_SELECT:
		string = metaData->supportsOrderByUnrelated() ? "Y" : "N";
		break;

	case SQL_IDENTIFIER_CASE:
		if (metaData->storesUpperCaseIdentifiers())
			value = SQL_IC_UPPER;
		else if (metaData->storesLowerCaseIdentifiers())
			value = SQL_IC_LOWER;
		else if (metaData->storesMixedCaseIdentifiers())
			value = SQL_IC_MIXED;
		else
			value = SQL_IC_SENSITIVE;
		break;

	case SQL_QUOTED_IDENTIFIER_CASE:
		if (metaData->storesUpperCaseQuotedIdentifiers())
			value = SQL_IC_UPPER;
		else if (metaData->storesLowerCaseQuotedIdentifiers())
			value = SQL_IC_LOWER;
		else if (metaData->storesMixedCaseQuotedIdentifiers())
			value = SQL_IC_MIXED;
		else
			value = SQL_IC_SENSITIVE;
		break;

	case SQL_NON_NULLABLE_COLUMNS:
		if(metaData->supportsNonNullableColumns())
			value = SQL_NNC_NON_NULL;
		break;

	case SQL_NULL_COLLATION:
		value = 0;
		if(metaData->nullsAreSortedHigh())
			value |= SQL_NC_HIGH;
		if(metaData->nullsAreSortedLow())
			value |= SQL_NC_LOW;
		if(metaData->nullsAreSortedAtStart())
			value |= SQL_NC_START;
		if(metaData->nullsAreSortedAtEnd())
			value |= SQL_NC_END;
		break;

	case SQL_PROCEDURE_TERM:
		string = metaData->getProcedureTerm();
		break;
	}

	switch (item->type)
	{
	case infoString:
#ifdef DEBUG
		sprintf (temp, "  %s (string) %.128s\n", item->name, string);
		OutputDebugString (temp);
#endif
		return (setString (string, (SQLCHAR*) ptr, maxLength, actualLength)) ?
		       SQL_SUCCESS_WITH_INFO : SQL_SUCCESS;

	case infoShort:
#ifdef DEBUG
		sprintf (temp, "  %s (short) %d\n", item->name, value);
		OutputDebugString (temp);
#endif
		*((SQLUSMALLINT*) ptr) = (SQLUSMALLINT) value;
		if (actualLength)
			*actualLength = sizeof (SQLUSMALLINT);
		break;

	case infoLong:
#ifdef DEBUG
		sprintf (temp, "  %s (int) %d\n", item->name, value);
		OutputDebugString (temp);
#endif
		if ( maxLength == sizeof (SQLUSMALLINT) )
		{
			*((SQLUSMALLINT*) ptr) = (SQLUSMALLINT)value;
			if (actualLength)
				*actualLength = sizeof (SQLUSMALLINT);
		}
		else
		{
			*((SQLUINTEGER*) ptr) = value;
			if (actualLength)
				*actualLength = sizeof (SQLUINTEGER);
		}
		break;

	case infoUnsupported:
#ifdef DEBUG
		sprintf (temp, "  %s (string) %s\n", item->name, "*unsupported*");
		OutputDebugString (temp);
#endif
		*((SQLUINTEGER*) ptr) = value;
		break;
	}

	return sqlSuccess();
}

char* OdbcConnection::appendString(char * ptr, const char * string)
{
	while (*string)
		*ptr++ = *string++;

	return ptr;
}

SQLRETURN OdbcConnection::allocHandle(int handleType, SQLHANDLE * outputHandle)
{
	clearErrors();
	if ( handleType == SQL_HANDLE_DESC )
	{
		OdbcDesc * desc = allocDescriptor(odtApplication);
		desc->headAllocType = SQL_DESC_ALLOC_USER;
		*outputHandle = desc;
		return sqlSuccess();
	}
	else if (handleType == SQL_HANDLE_STMT)
	{
		*outputHandle = SQL_NULL_HDBC;

		OdbcStatement *statement = new OdbcStatement (this, statementNumber++);
		statement->next = statements;
		statements = statement;
		*outputHandle = (SQLHANDLE)statement;

		return sqlSuccess();
	}

	return sqlReturn (SQL_ERROR, "HY000", "General Error");
}

DatabaseMetaData* OdbcConnection::getMetaData()
{
	return connection->getMetaData();
}

void OdbcConnection::Lock()
{
	connection->getMetaData()->LockThread();
}

void OdbcConnection::UnLock()
{
	connection->getMetaData()->UnLockThread();
}

SQLRETURN OdbcConnection::sqlConnect(const SQLCHAR *dataSetName, int dsnLength, SQLCHAR *uid, int uidLength, SQLCHAR * passwd, int passwdLength)
{
	clearErrors();

	if (connected)
		return sqlReturn (SQL_ERROR, "08002", "Connection name is use");

	char temp [1024], *p = temp;

	dsn = getString (&p, dataSetName, dsnLength, "");
	account = getString (&p, uid, uidLength, "");
	password = getString (&p, passwd, passwdLength, "");
	role = "";
	charset = "";
	expandConnectParameters();
	SQLRETURN ret = connect (jdbcDriver, databaseName, account, password, role, charset);

	if (ret != SQL_SUCCESS)
		return ret;

	if ( levelBrowseConnect )
		levelBrowseConnect = 0;

	return sqlSuccess();
}

SQLRETURN OdbcConnection::connect(const char *sharedLibrary, const char * databaseName, const char * account, const char * password, const char * role, const char * charset)
{
	Properties *properties = NULL;

	try
	{
		connection = createConnection();

		if ( getDriverBuildKey() != connection->getDriverBuildKey() )
		{
			connection->close();
			connection = NULL;
			env->envShare = NULL;

			JString text;
			text.Format( " Unable to load %s Library : can't find ver. %s ", sharedLibrary, DRIVER_VERSION );
			return sqlReturn( SQL_ERROR, "HY000", text );
		}

		properties = connection->allocProperties();
		if (account)
			properties->putValue ("user", account);
		if (password)
			properties->putValue ("password", password);
		if (role)
			properties->putValue ("role", role);
		if (charset)
			properties->putValue ("charset", charset);
		if (client)
			properties->putValue ("client", client);
		if (dsn)
			properties->putValue ("dsn", dsn);

		properties->putValue ("dialect", dialect3 ? "3" : "1");

		properties->putValue ("quoted", quotedIdentifier ? "Y" : "N");
		properties->putValue ("sensitive", sensitiveIdentifier ? "Y" : "N");
		properties->putValue ("autoQuoted", autoQuotedIdentifier ? "Y" : "N");

		properties->putValue ("databaseAccess",
								databaseAccess == CREATE_DB ? "1" 
								: databaseAccess == DROP_DB ? "2" 
								: "0");

		if (useSchemaIdentifier)
			properties->putValue ("useSchema", useSchemaIdentifier);

		if (useLockTimeoutWaitTransactions)
			properties->putValue ("useLockTimeout", useLockTimeoutWaitTransactions);

		if (pageSize)
			properties->putValue ("pagesize", pageSize);

		if (connectionTimeout)
		{
			char buffer[256];
			sprintf (buffer, "%d", connectionTimeout);
			properties->putValue ("timeout", buffer);
		}

		connection->openDatabase (databaseName, properties);
		properties->release();

		env->envShare = connection->getEnvironmentShare();

		// Next two lines added by CA
		connection->setAutoCommit( autoCommit );
		connection->setTransactionIsolation( transactionIsolation );
		connection->setExtInitTransaction( optTpb );
		connection->setUseAppOdbcVersion( env->useAppOdbcVersion );
		charsetCode = connection->getConnectionCharsetCode();
		WcsToMbs = connection->getConnectionWcsToMbs();
		MbsToWcs = connection->getConnectionMbsToWcs();
	}
	catch ( std::exception &ex )
	{
		SQLException &exception = (SQLException&)ex;
		if ( env->envShare )
			env->envShare = NULL;

		if ( properties )
			properties->release();

		connection->close();
		connection = NULL;

		return sqlReturn( SQL_ERROR, "08004", exception.getText(), exception.getSqlcode() );
	}

	connected = true;

	return SQL_SUCCESS;
}

SQLRETURN OdbcConnection::sqlEndTran(int operation)
{
	clearErrors();

	if (connection)
		try
		{
			switch (operation)
			{
			case SQL_COMMIT:
				connection->commitAuto();
				break;

			case SQL_ROLLBACK:
				connection->rollbackAuto();
			}
		}
		catch ( std::exception &ex )
		{
			SQLException &exception = (SQLException&)ex;
			postError ("S1000", exception);
			return SQL_ERROR;
		}

	return sqlSuccess();
}

SQLRETURN OdbcConnection::sqlExecuteCreateDatabase(const char * sqlString)
{
	clearErrors();

	try
	{
		connection->sqlExecuteCreateDatabase( sqlString );
	}
	catch ( std::exception &ex )
	{
		SQLException &exception = (SQLException&)ex;
		postError( "HY000", exception );
		return SQL_ERROR;
	}

	return sqlSuccess();
}

void OdbcConnection::statementDeleted(OdbcStatement * statement)
{
	for (OdbcObject **ptr = (OdbcObject**) &statements; *ptr; ptr =&((*ptr)->next))
		if (*ptr == statement)
		{
			*ptr = statement->next;
			break;
		}
}

void OdbcConnection::expandConnectParameters()
{
	if (!dsn.IsEmpty())
	{
		JString options;

		if (description.IsEmpty())
			description = readAttribute (SETUP_DESCRIPTION);

		if (databaseName.IsEmpty())
			databaseName = readAttribute (SETUP_DBNAME);

		if (client.IsEmpty())
			client = readAttribute (SETUP_CLIENT);

		if (account.IsEmpty())
			account = readAttribute (SETUP_USER);

		if (password.IsEmpty())
		{
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

		if (jdbcDriver.IsEmpty())
			jdbcDriver = readAttribute (SETUP_JDBC_DRIVER);

		if (role.IsEmpty())
			role = readAttribute(SETUP_ROLE);

		if (charset.IsEmpty())
			charset = readAttribute(SETUP_CHARSET);

		if ( !(defOptions & DEF_READONLY_TPB) )
		{
			options = readAttribute(SETUP_READONLY_TPB);

			if(*(const char *)options == 'Y')
				optTpb |=TRA_ro;
		}

		if ( !(defOptions & DEF_NOWAIT_TPB) )
		{
			options = readAttribute(SETUP_NOWAIT_TPB);

			if(*(const char *)options == 'Y')
				optTpb |=TRA_nw;
		}

		if ( !(defOptions & DEF_DIALECT) )
		{
			options = readAttribute(SETUP_DIALECT);

			if(*(const char *)options == '1')
				dialect3 = false;
		}

		if (useSchemaIdentifier.IsEmpty())
			useSchemaIdentifier = readAttribute(SETUP_USESCHEMA);

		if (useLockTimeoutWaitTransactions.IsEmpty())
			useLockTimeoutWaitTransactions = readAttribute(SETUP_LOCKTIMEOUT);

		if ( !(defOptions & DEF_SAFETHREAD) )
		{
			options = readAttribute(SETUP_SAFETHREAD);

			if(*(const char *)options == 'N')
				safeThread = false;
		}

		if ( !(defOptions & DEF_QUOTED) )
		{
			options = readAttribute(SETUP_QUOTED);

			if(*(const char *)options == 'N')
				quotedIdentifier = false;
		}

		if ( !(defOptions & DEF_SENSITIVE) )
		{
			options = readAttribute(SETUP_SENSITIVE);

			if(*(const char *)options == 'Y')
				sensitiveIdentifier = true;
		}

		if ( !(defOptions & DEF_AUTOQUOTED) )
		{
			options = readAttribute(SETUP_AUTOQUOTED);

			if(*(const char *)options == 'Y')
				autoQuotedIdentifier = true;
		}
	}
	else if (!filedsn.IsEmpty())
	{
		JString options;

		if (description.IsEmpty())
			description = readAttributeFileDSN (SETUP_DESCRIPTION);

		if (databaseName.IsEmpty())
			databaseName = readAttributeFileDSN (SETUP_DBNAME);

		if (client.IsEmpty())
			client = readAttributeFileDSN (SETUP_CLIENT);

		if (account.IsEmpty())
			account = readAttributeFileDSN (SETUP_USER);

		if (password.IsEmpty())
		{
			JString pass = readAttributeFileDSN (SETUP_PASSWORD);
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

		if (jdbcDriver.IsEmpty())
			jdbcDriver = readAttributeFileDSN (SETUP_JDBC_DRIVER);

		if (role.IsEmpty())
			role = readAttributeFileDSN (SETUP_ROLE);

		if (charset.IsEmpty())
			charset = readAttributeFileDSN (SETUP_CHARSET);

		if ( !(defOptions & DEF_READONLY_TPB) )
		{
			options = readAttributeFileDSN (SETUP_READONLY_TPB);

			if(*(const char *)options == 'Y')
				optTpb |=TRA_ro;
		}

		if ( !(defOptions & DEF_NOWAIT_TPB) )
		{
			options = readAttributeFileDSN (SETUP_NOWAIT_TPB);

			if(*(const char *)options == 'Y')
				optTpb |=TRA_nw;
		}

		if ( !(defOptions & DEF_DIALECT) )
		{
			options = readAttributeFileDSN (SETUP_DIALECT);

			if(*(const char *)options == '1')
				dialect3 = false;
		}

		if (useSchemaIdentifier.IsEmpty())
			useSchemaIdentifier = readAttributeFileDSN (SETUP_USESCHEMA);

		if (useLockTimeoutWaitTransactions.IsEmpty())
			useLockTimeoutWaitTransactions = readAttributeFileDSN (SETUP_LOCKTIMEOUT);

		if ( !(defOptions & DEF_SAFETHREAD) )
		{
			options = readAttribute(SETUP_SAFETHREAD);

			if(*(const char *)options == 'N')
				safeThread = false;
		}

		if ( !(defOptions & DEF_QUOTED) )
		{
			options = readAttributeFileDSN (SETUP_QUOTED);

			if(*(const char *)options == 'N')
				quotedIdentifier = false;
		}

		if ( !(defOptions & DEF_SENSITIVE) )
		{
			options = readAttribute(SETUP_SENSITIVE);

			if(*(const char *)options == 'Y')
				sensitiveIdentifier = true;
		}

		if ( !(defOptions & DEF_AUTOQUOTED) )
		{
			options = readAttribute(SETUP_AUTOQUOTED);

			if(*(const char *)options == 'Y')
				autoQuotedIdentifier = true;
		}

		if (dsn.IsEmpty())
		{
			dsn = readAttributeFileDSN (SETUP_DSN);
			if (!dsn.IsEmpty())
				expandConnectParameters();
		}
	}

	if (jdbcDriver.IsEmpty())
		jdbcDriver = DEFAULT_DRIVER;
}

void OdbcConnection::saveConnectParameters()
{
	writeAttributeFileDSN (SETUP_DRIVER, DRIVER_FULL_NAME);
	writeAttributeFileDSN (SETUP_DESCRIPTION, description);
	writeAttributeFileDSN (SETUP_DBNAME, databaseName);
	writeAttributeFileDSN (SETUP_CLIENT, client);
	writeAttributeFileDSN (SETUP_USER, account);
	writeAttributeFileDSN (SETUP_ROLE, role);
	writeAttributeFileDSN (SETUP_CHARSET, charset);
	writeAttributeFileDSN (SETUP_JDBC_DRIVER, jdbcDriver);
	writeAttributeFileDSN (SETUP_READONLY_TPB, (optTpb & TRA_ro) ? "Y" : "N");
	writeAttributeFileDSN (SETUP_NOWAIT_TPB, (optTpb & TRA_nw) ? "Y" : "N");
	writeAttributeFileDSN (SETUP_DIALECT, dialect3 ? "3" : "1");
	writeAttributeFileDSN (SETUP_QUOTED, quotedIdentifier ? "Y" : "N");
	writeAttributeFileDSN (SETUP_SENSITIVE, sensitiveIdentifier ? "Y" : "N");
	writeAttributeFileDSN (SETUP_AUTOQUOTED, autoQuotedIdentifier ? "Y" : "N");
	writeAttributeFileDSN (SETUP_USESCHEMA, useSchemaIdentifier);
	writeAttributeFileDSN (SETUP_LOCKTIMEOUT, useLockTimeoutWaitTransactions);
	writeAttributeFileDSN (SETUP_SAFETHREAD, safeThread ? "Y" : "N");

	char buffer[256];
	CSecurityPassword security;
	security.encode( (char*)(const char *)password, buffer );
	writeAttributeFileDSN (SETUP_PASSWORD, buffer);
}

OdbcDesc* OdbcConnection::allocDescriptor(OdbcDescType type)
{
	OdbcDesc *descriptor = new OdbcDesc (type, this);
	descriptor->next = descriptors;
	descriptors = descriptor;

	return descriptor;
}

void OdbcConnection::descriptorDeleted(OdbcDesc * descriptor)
{
	for (OdbcDesc **ptr = &descriptors; *ptr; ptr = (OdbcDesc**)&(*ptr)->next)
		if (*ptr == descriptor)
		{
			*ptr = (OdbcDesc*) descriptor->next;
			break;
		}
}

SQLRETURN OdbcConnection::sqlGetConnectAttr(int attribute, SQLPOINTER ptr, int bufferLength, SQLINTEGER *lengthPtr)
{
	clearErrors();
	int value;
	const char *string = NULL;

	switch (attribute)
	{
	case SQL_ATTR_ASYNC_ENABLE:
		value = asyncEnabled;
		break;

	case SQL_ATTR_ACCESS_MODE:			//   101		
		value = accessMode;
		break;

	case SQL_TXN_ISOLATION:			//   108
		if ( connection )
			value = connection->getTransactionIsolation();
		else
			value = transactionIsolation;
		break;

	case SQL_AUTOCOMMIT:			//   102
		value = (autoCommit) ? SQL_AUTOCOMMIT_ON : SQL_AUTOCOMMIT_OFF;
		break;

	case SQL_ATTR_ODBC_CURSORS: // SQL_ODBC_CURSORS   110
		value = cursors;
		break;

	case SQL_ATTR_CONNECTION_DEAD:
		value = SQL_CD_FALSE;
		break;

	case SQL_ATTR_AUTO_IPD:			// 10001
		value = SQL_TRUE;
		break;

	case SQL_ATTR_CURRENT_CATALOG:
		string = databaseName;
		break;

	case SQL_LOGIN_TIMEOUT:			//   103
	case SQL_OPT_TRACE:				//   104
	case SQL_OPT_TRACEFILE:			//   105
	case SQL_TRANSLATE_DLL:			//   106
	case SQL_TRANSLATE_OPTION:		//   107
	case SQL_QUIET_MODE:			//   111
	case SQL_PACKET_SIZE:			//   112

	default:
		return sqlReturn (SQL_ERROR, "HYC00", "Optional feature not implemented");
	}

	SQLINTEGER len = bufferLength;
	lengthPtr = &len;
	if (string)
		return returnStringInfo (ptr, bufferLength, lengthPtr, string);

	if (ptr)
		*(int*) ptr = value;

	if (lengthPtr)
		*lengthPtr = sizeof (int);

	return sqlSuccess();
}

void OdbcConnection::initUserEvents( PODBC_EVENTS_BLOCK_INFO infoEvents )
{
	try
	{
		PropertiesEvents *propertiesEvents = connection->allocPropertiesEvents();
		PODBC_EVENT_INFO nextNameEvent = infoEvents->events;

		for ( int i = 0; i < infoEvents->count; i++, nextNameEvent++ )
		{
			propertiesEvents->putNameEvent( nextNameEvent->nameEvent );
			nextNameEvent->countEvents = ~0lu;
			nextNameEvent->changed = false;
		}

		userEvents = connection->prepareUserEvents( propertiesEvents, infoEvents->lpAstProc );
		propertiesEvents->release();

		userEventsInterfase = new ODBC_USER_EVENTS_INTERFASE;
		userEventsInterfase->userData = infoEvents->userData;
		userEventsInterfase->hdbc = infoEvents->hdbc;
		userEventsInterfase->events = infoEvents->events;
		userEventsInterfase->count = infoEvents->count;
	}
	catch ( std::exception &ex )
	{
		SQLException &exception = (SQLException&)ex;
		postError( "HY000", exception );
	}
}

void OdbcConnection::updateResultEvents( char *updated )
{
	try
	{
		userEvents->updateResultEvents( updated );

		PODBC_EVENT_INFO nextNameEvent = userEventsInterfase->events;

		for ( int i = 0; i < userEventsInterfase->count; i++, nextNameEvent++ )
		{
			nextNameEvent->countEvents = userEvents->getCountEvents( i );
			nextNameEvent->changed = userEvents->isChanged( i );
		}
	}
	catch ( std::exception &ex )
	{
		SQLException &exception = (SQLException&)ex;
		postError( "HY000", exception );
	}
}

void OdbcConnection::requeueEvents()
{
	try
	{
		userEvents->queEvents( userEventsInterfase );
	}
	catch ( std::exception &ex )
	{
		SQLException &exception = (SQLException&)ex;
		postError( "HY000", exception );
	}
}

}; // end namespace OdbcJdbcLibrary
