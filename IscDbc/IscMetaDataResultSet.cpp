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
 *
 *	2002-08-12	IscMetaDataResultSet.cpp
 *				Added changes from C. G. Alvarez to so that
 *				SQLColAttributes() called with SQL_COLUMN_TYPE_NAME returns 
 *				the name of the type instead of the number of the type
 *
 */

// IscMetaDataResultSet.cpp: implementation of the IscMetaDataResultSet class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "IscDbc.h"
#include "IscMetaDataResultSet.h"
#include "IscDatabaseMetaData.h"
#include "IscResultSet.h"
#include "IscPreparedStatement.h"
#include "SQLError.h"
#include "IscConnection.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscMetaDataResultSet::IscMetaDataResultSet(IscDatabaseMetaData *meta) : IscResultSet (NULL)
{
	metaData = meta;
	statement = NULL;
}

IscMetaDataResultSet::~IscMetaDataResultSet()
{
	if ( statement )
		statement->release();
}

void IscMetaDataResultSet::prepareStatement(const char * sql)
{
	close();
	statement = (IscPreparedStatement *)metaData->connection->prepareStatement (sql);
	statement->executeMetaDataQuery();
	initResultSet((IscStatement*)statement);
}

void IscMetaDataResultSet::trimBlanks(int id)
{
	int len;
	char * data = sqlda->getText (id, len);
	char * end = data + len - 1;

	while (end > data && *end == ' ')
		*end-- = '\0';
}

bool IscMetaDataResultSet::isWildcarded(const char * pattern)
{
	for (const char *p = pattern; *p; ++p)
		if (*p == '%' || *p == '\\' || *p == '*')
			return true;

	return false;
}

JString IscMetaDataResultSet::expandPattern(const char *prefix, const char * string, const char * pattern)
{
	char temp [256];

	if (isWildcarded (pattern))
		sprintf (temp, "%s (%s like '%s %%' ESCAPE '\\' or %s like '%s' ESCAPE '\\')\n",
							prefix, string, pattern, string, pattern);
	else
		sprintf (temp, "%s %s = \'%s\'\n",prefix, string, pattern);

	return temp;
}
