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
 *  2002-10-11	Attachment.cpp
 *				Contributed by C. G. Alvarez
 *				Added isc_info_page_size
 *              to openDatabase()
 *
 *
 */

// Attachment.cpp: implementation of the Attachment class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "IscDbc.h"
#include "Attachment.h"
#include "SQLError.h"
#include "Parameters.h"
#include "IscConnection.h"

static char databaseInfoItems [] = { 
	isc_info_db_id,
	isc_info_db_sql_dialect,
	isc_info_base_level,
	isc_info_user_names,
	isc_info_ods_version,
	isc_info_firebird_version,
	isc_info_version, 
	isc_info_page_size,
	isc_info_end 
	};


namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Attachment::Attachment()
{
	useCount = 1;
	GDS = NULL;
	databaseHandle = NULL;
	databaseAccess = OPEN_DB;
	transactionHandle = NULL;
	admin = true;
	isRoles = false;
	userType = 8;
	charsetCode = 0; // NONE
	useSchemaIdentifier = 0;
	useLockTimeoutWaitTransactions = 0;
	databaseProductName = "Interbase";
	majorFb = 1;
	minorFb = 0;
	versionFb = 0;
}

Attachment::~Attachment()
{
	ISC_STATUS statusVector [20];

	if (databaseHandle)
		GDS->_detach_database (statusVector, &databaseHandle);

	if( GDS )
	{
		delete GDS;
		GDS = NULL;
	}
}

void Attachment::loadClientLiblary( Properties *properties )
{
	const char *clientDefault = NULL;
	const char *client = properties->findValue ("client", NULL);

	if ( !client || !*client )
#if defined (_WINDOWS)
		client = "gds32.dll",
		clientDefault = "fbclient.dll";
#elif defined (__APPLE__)
		client = "libgds.dylib",
		clientDefault = "libfbclient.dylib";
#else
		client = "libgds.so",
		clientDefault = "libfbclient.so";
#endif

	GDS = new CFbDll();
	if ( !GDS->LoadDll (client, clientDefault) )
	{
		JString text;
		text.Format ("Unable to connect to data source: library '%s' failed to load", client);
		throw SQLEXCEPTION( -904, 335544375l, text );
	}
}

void Attachment::createDatabase(const char *dbName, Properties *properties)
{
	char sql[1024];
	char *p = sql;

	if( !GDS )
		loadClientLiblary( properties );

	p += sprintf( p, "CREATE DATABASE \'%s\' ", dbName );

	const char *dialect = properties->findValue ("dialect", NULL);

	if ( dialect && *dialect == '1')
		databaseDialect = SQL_DIALECT_V5;
	else
		databaseDialect = SQL_DIALECT_V6;

	const char *user = properties->findValue ("user", NULL);

	if (user && *user)
		p += sprintf( p, "USER \'%s\' ", user );

	const char *password = properties->findValue ("password", NULL);

	if (password && *password)
		p += sprintf( p, "PASSWORD \'%s\' ", password );

	const char *pagesize = properties->findValue ("pagesize", NULL);

	if (pagesize && *pagesize)
		p += sprintf( p, "PAGE_SIZE %s ", pagesize );

	const char *charset = properties->findValue ("charset", NULL);
	if ( !charset )
		charset = properties->findValue ("characterset", NULL);

	if (charset && *charset)
		p += sprintf( p, "DEFAULT CHARACTER SET %s ", charset );

	*p = '\0';

	isc_tr_handle   trans = NULL;
	ISC_STATUS      statusVector[20];

	if ( GDS->_dsql_execute_immediate( statusVector, &databaseHandle, &trans, 0, (char*)sql, databaseDialect, NULL ) )
    {
		if ( statusVector[1] )
			throw SQLEXCEPTION ( GDS->_sqlcode( statusVector ), statusVector [1], getIscStatusText( statusVector ) );
	}
	
	if ( trans && GDS->_commit_transaction( statusVector, &trans ) )
    {
		if ( statusVector[1] )
			throw SQLEXCEPTION ( GDS->_sqlcode( statusVector ), statusVector [1], getIscStatusText( statusVector ) );
	}
}

void Attachment::openDatabase(const char *dbName, Properties *properties)
{
	if( !GDS )
		loadClientLiblary( properties );

	const char *dialect = properties->findValue ("dialect", NULL);

	isRoles = false;
	databaseName = dbName;
	char dpb [2048], *p = dpb;
	*p++ = isc_dpb_version1;

	const char *user = properties->findValue ("user", NULL);

	if (user && *user)
	{
		userName = user;
		userAccess = user;
		userType = 8;
		*p++ = isc_dpb_user_name,
		*p++ = (char)strlen (user);
		for (const char *q = user; *q;)
			*p++ = *q++;
	}
	else
	{
		*p++ = isc_dpb_user_name,
		*p++ = 0;
	}

	const char *password = properties->findValue ("password", NULL);

	if (password && *password)
	{
		*p++ = isc_dpb_password,
		*p++ = (char)strlen (password);
		for (const char *q = password; *q;)
			*p++ = *q++;
	}
	else
	{
		*p++ = isc_dpb_password,
		*p++ = 0;
	}

	const char *timeout = properties->findValue ("timeout", NULL);

	if (timeout && *timeout)
	{
		connectionTimeout = atoi(timeout);

		*p++ = isc_dpb_connect_timeout;
		*p++ = sizeof (int);
		*p++ = (char)connectionTimeout;
		*p++ = (char)(connectionTimeout >> 8);
		*p++ = (char)(connectionTimeout >> 16);
		*p++ = (char)(connectionTimeout >> 24);
	}

	const char *role = properties->findValue ("role", NULL);

	if (role && *role)
	{
		userAccess = role;

		char *ch = (char *)(const char *)userAccess;
		while ( (*ch = UPPER ( *ch )) )
			++ch;

		userType = 13;
		isRoles = true;
		*p++ = isc_dpb_sql_role_name;
		*p++ = (char)strlen (role);
		for (const char *q = role; *q;)
			*p++ = *q++;
	}

	const char *charset = properties->findValue ("charset", NULL);
	if ( !charset )
		charset = properties->findValue ("characterset", NULL);

	if (charset && *charset)
	{
		*p++ = isc_dpb_lc_ctype;
		*p++ = (char)strlen (charset);
		for (const char *q = charset; *q;)
			*p++ = *q++;

		charsetCode = findCharsetsCode( charset );
	}

	const char *property = properties->findValue ("databaseAccess", NULL);

	if ( property )
		databaseAccess = (int)(*property - '0');
	else
		databaseAccess = 0;

	int dpbLength = p - dpb;
	ISC_STATUS statusVector [20];

	if (GDS->_attach_database (statusVector, (short)strlen (dbName), (char*) dbName, &databaseHandle, 
							 dpbLength, dpb))
	{
		if ( statusVector [1] == 335544344l )
		{
			if ( databaseAccess == CREATE_DB )
				createDatabase( dbName, properties );
			else
			{
				JString text;
				
				switch ( statusVector [7] )
				{
				case isc_io_access_err:
					text.Format ("File Database is used by another process");
					break;

				case isc_io_open_err:
					text.Format ("File Database is not found");
					break;

				default:
					text.Format ("Unavailable Database");
					break;
				}

				throw SQLEXCEPTION ( GDS->_sqlcode( statusVector ), statusVector [1], text );
			}
		}
		else
			throw SQLEXCEPTION ( GDS->_sqlcode( statusVector ), statusVector [1], getIscStatusText( statusVector ) );
	}
	else if ( databaseAccess == DROP_DB )
	{
		if ( GDS->_drop_database ( statusVector, &databaseHandle ) )
			throw SQLEXCEPTION ( GDS->_sqlcode( statusVector ), statusVector [1], getIscStatusText( statusVector ) );
		return;
	}

	char result [2048];
	databaseDialect = SQL_DIALECT_V5;

	if (!GDS->_database_info (statusVector, &databaseHandle, sizeof (databaseInfoItems), databaseInfoItems, sizeof (result), result))
	{
 		for (p = result; p < result + sizeof (result) && *p != isc_info_end;)
		{
			char item = *p++;
			int length = GDS->_vax_integer (p, 2);
			p += 2;
			switch (item)
			{
			case isc_info_db_id:
				{
					char * next = p + 2 + *( p + 1 );

					databaseNameFromServer.Format( "%.*s", *( p + 1 ), p + 2 );
					databaseServerName.Format( "%.*s", *next, next + 1 );
				}
				break;

			case isc_info_db_sql_dialect:
				databaseDialect = GDS->_vax_integer (p, length);
				break;
			
			case isc_info_base_level:
				serverBaseLevel = GDS->_vax_integer (p, length);
				break;

			case isc_info_user_names:
				if ( userAccess.IsEmpty() )
				{
					userName = JString ( p + 1, (int)*p );
					userAccess = userName;
					userType = 8;
				}
				break;

			case isc_info_firebird_version:
				{
					int level = 0;
					char * start = p + 2;
					char * beg = start;
					char * end = beg + p [1];
					
					while ( beg < end )
					{
						if ( *beg >= '0' && *beg <= '9' )
						{
							switch ( ++level )
							{
							case 1:
								majorFb = atoi(beg);
								while( *++beg != '.' );
								break;
							case 2:
								minorFb = atoi(beg);
								while( *++beg != '.' );
								break;
							default:
								versionFb = atoi(beg);
								while( *beg >= '0' && *beg <= '9' || *beg == ' ')
									beg++;
								if ( *beg == '.' )
									break;
								beg = end;
								break;
							}
						}
						else
							beg++;
					}
					databaseProductName = "Firebird";
				}
				break;

			case isc_info_version:
				{
					JString productName;
					int level = 0;
					int major = 0, minor = 0, version = 0;
					char * start = p + 2;
					char * beg = start;
					char * end = beg + p [1];
					char * tmp = NULL;
					
					while ( beg < end )
					{
						if ( *beg >= '0' && *beg <= '9' )
						{
							switch ( ++level )
							{
							case 1:
								tmp = beg;
								major = atoi(beg);
								while( *++beg != '.' );
								break;
							case 2:
								minor = atoi(beg);
								while( *++beg != '.' );
								break;
							default: // Firebird (##.##.##.####) and Yaffil(##.##.####)
								version = atoi(beg);
								while( *beg >= '0' && *beg <= '9' || *beg == ' ')
									beg++;
								if ( *beg == '.' )
									break;
								if ( beg < end )
								{
									productName = JString( beg, end - beg );

									char *endBeg = beg;

									while( *endBeg != ' ' )
										endBeg++;

									databaseProductName = JString( beg, endBeg - beg );
								}
								beg = end;
								break;
							}
						}
						else
							beg++;
					}
					serverVersion.Format( "%02d.%02d.%04d %.*s %s",major,minor,version, tmp ? tmp - start : 0, start, (const char*)productName );
				}
				break;

			case isc_info_page_size:
				pageSize = GDS->_vax_integer (p, length);
				break;
			}
			p += length;
		}
	}
	
	if ( dialect && *dialect == '1')
		databaseDialect = SQL_DIALECT_V5;
	else
		databaseDialect = SQL_DIALECT_V6;

	property = properties->findValue ("quoted", NULL);

	if ( property && *property == 'Y')
		quotedIdentifier = true;
	else
		quotedIdentifier = false;

	property = properties->findValue ("sensitive", NULL);

	if ( property && *property == 'Y')
		sensitiveIdentifier = true;
	else
		sensitiveIdentifier = false;

	property = properties->findValue ("autoQuoted", NULL);

	if ( property && *property == 'Y')
		autoQuotedIdentifier = true;
	else
		autoQuotedIdentifier = false;

	property = properties->findValue ("useSchema", NULL);

	if ( property )
	{
		switch ( *property )
		{
		case '1': // remove SCHEMA from SQL query
			useSchemaIdentifier = 1;
			break;

		case '2': // use full SCHEMA
			useSchemaIdentifier = 2;
			break;

		default:
		case '0': // set null field SCHEMA
			useSchemaIdentifier = 0;
			break;
		}
	}
	else
		useSchemaIdentifier = 0;

	property = properties->findValue ("useLockTimeout", NULL);

	if (property && *property)
		useLockTimeoutWaitTransactions = atoi(property);

	property = properties->findValue ("dsn", NULL);

	if (property && *property)
		dsn = property;

	checkAdmin();
}

JString Attachment::getIscStatusText(ISC_STATUS * statusVector)
{
	char text[4096], *p = text;

	while ( GDS->_interprete( p, &statusVector  ) )
	{
		while ( *p ) ++p;
		*p++ = '\n';
	}

	if ( p > text )
		--p;

	*p = 0;

	return text;
}

void Attachment::addRef()
{
	++useCount;
}

int Attachment::release()
{
	if (--useCount == 0)
	{
		delete this;
		return 0;
	}

	return useCount;
}

int Attachment::getDatabaseDialect()
{
	return databaseDialect;
}

void Attachment::checkAdmin()
{
	QUAD adm1 = (QUAD)71752869960019.0;
	QUAD adm2 = (QUAD)107075219978611.0;
	QUAD user = (QUAD)0;
	memcpy((void *)&user,(const char *)userName,6);

	admin = user == adm1 || user == adm2;

	if ( admin )
	{
		userAccess = "";
		userType = 0;
	}
}

bool Attachment::isAdmin()
{
	return admin;
}

JString& Attachment::getUserAccess()
{
	return userAccess;
}

int Attachment::getUserType()
{
	return userType;
}

}; // end namespace IscDbcLibrary
