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

#include <string.h>
#include "IscDbc.h"
#include "Attachment.h"
#include "SQLError.h"
#include "Parameters.h"
#include "IscConnection.h"

static char databaseInfoItems [] = { 
	isc_info_db_sql_dialect,
	isc_info_base_level,
	isc_info_ods_version,
	isc_info_version, 
	isc_info_page_size,
	isc_info_end 
	};


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Attachment::Attachment()
{
	useCount = 1;
	databaseHandle = NULL;
}

Attachment::~Attachment()
{
	ISC_STATUS statusVector [20];

	if (databaseHandle)
	{
		GDS->_detach_database (statusVector, &databaseHandle);
	}
}

void Attachment::openDatabase(const char *dbName, Properties *properties)
{
	databaseName = dbName;
	char dpb [256], *p = dpb;
	*p++ = isc_dpb_version1;

	const char *user = properties->findValue ("user", NULL);

	if (user)
		{
		userName = user;
		*p++ = isc_dpb_user_name,
		*p++ = strlen (user);
		for (const char *q = user; *q;)
			*p++ = *q++;
		}

	const char *password = properties->findValue ("password", NULL);

	if (password)
		{
		*p++ = isc_dpb_password,
		*p++ = strlen (password);
		for (const char *q = password; *q;)
			*p++ = *q++;
		}
	const char *role = properties->findValue ("role", NULL);

	if (role)
		{
		*p++ = isc_dpb_sql_role_name;
		*p++ = strlen (role);
		for (const char *q = role; *q;)
			*p++ = *q++;
		}

	const char *charset = properties->findValue ("charset", NULL);

	if (charset)
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
		JString text = IscConnection::getIscStatusText (statusVector);
		throw SQLEXCEPTION (statusVector [1], text);
		}

	char result [256]; // 100
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

				case isc_info_version:
					serverVersion = JString (p + 2, p [1]);
					break;

				case isc_info_page_size:
					pageSize = GDS->_vax_integer (p, length);
					break;
				}
			p += length;
			}
		}

	switch (databaseDialect)
		{
		case 0:
		case SQL_DIALECT_V5:
			quotedIdentifiers = false;
			break;

		case SQL_DIALECT_V6:
		default:
			quotedIdentifiers = true;
		}
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

