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
	isc_info_db_sql_dialect,
	isc_info_base_level,
	isc_info_user_names,
	isc_info_ods_version,
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
	transactionHandle = NULL;
	admin = true;
	isRoles = false;
	userType = 8;
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

void Attachment::openDatabase(const char *dbName, Properties *properties)
{
	if( !GDS )
	{
		const char *clientDefault = NULL;
		const char *client = properties->findValue ("client", NULL);
		if ( !client || !*client )
#ifdef _WIN32
			client = "gds32.dll",
			clientDefault = "fbclient.dll";
#else
			client = "libgds.so",
			clientDefault = "libfbclient.so";
#endif

		GDS = new CFbDll();
		if ( !GDS->LoadDll (client, clientDefault) )
		{
			JString text;
			text.Format ("Unable to connect to data source: library '%s' failed to load", client);
			throw SQLEXCEPTION (8001, text);
		}
	}

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
		*p++ = strlen (user);
		for (const char *q = user; *q;)
			*p++ = *q++;
	}

	const char *password = properties->findValue ("password", NULL);

	if (password && *password)
	{
		*p++ = isc_dpb_password,
		*p++ = strlen (password);
		for (const char *q = password; *q;)
			*p++ = *q++;
	}

	const char *role = properties->findValue ("role", NULL);

	if (role && *role)
	{
		userAccess = role;
		userType = 13;
		isRoles = true;
		*p++ = isc_dpb_sql_role_name;
		*p++ = strlen (role);
		for (const char *q = role; *q;)
			*p++ = *q++;
	}

	const char *charset = properties->findValue ("charset", NULL);

	if (charset && *charset)
	{
		*p++ = isc_dpb_lc_ctype;
		*p++ = strlen (charset);
		for (const char *q = charset; *q;)
			*p++ = *q++;
	}

	int dpbLength = p - dpb;
	ISC_STATUS statusVector [20];

	if (GDS->_attach_database (statusVector, strlen (dbName), (char*) dbName, &databaseHandle, 
							 dpbLength, dpb))
	{
		JString text = getIscStatusText (statusVector);
		throw SQLEXCEPTION (statusVector [1], text);
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
			
			case isc_info_version:
				{
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
									databaseProductName = JString( beg, end - beg );
								beg = end;
								break;
							}
						}
						else
							beg++;
					}
					serverVersion.Format("%02d.%02d.%04d %.*s %s",major,minor,version, tmp ? tmp - start : 0, start, (const char*)databaseProductName);
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

	const char *quoted = properties->findValue ("quoted", NULL);

	if ( quoted && *quoted == 'Y')
		quotedIdentifiers = true;
	else
		quotedIdentifiers = false;

	checkAdmin();
}

JString Attachment::getIscStatusText(ISC_STATUS * statusVector)
{
	char text [4096], *p = text;
	ISC_STATUS *status = statusVector;
	bool first = true;

	while ( GDS->_interprete (p, &status) )
	{
		while (*p)
			++p;
		*p++ = '\n';
	}

	if (p > text)
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

void Attachment::existsAccess(char *& stringOut, const char *prefix, const char * relobject, int typeobject, const char *suffix)
{
	int len = sprintf (stringOut,	" %s exists( select cast(1 as integer) from rdb$user_privileges priv\n"
					"\t\twhere %s.rdb$%s = priv.rdb$relation_name\n"
					"\t\t\tand priv.rdb$privilege = 'S' and priv.rdb$object_type = %d\n"
					"\t\t\tand priv.rdb$user = '%s' and priv.rdb$user_type = %d ) %s \n",
						prefix, relobject, 
						!typeobject ? "relation_name" : "procedure_name",
						typeobject, (const char *)userAccess, userType, suffix);
	stringOut += len;
}

}; // end namespace IscDbcLibrary
