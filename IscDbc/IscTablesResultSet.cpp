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
 */

// IscTablesResultSet.cpp: implementation of the IscTablesResultSet class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include "IscDbc.h"
#include "IscTablesResultSet.h"
#include "IscConnection.h"
#include "IscDatabaseMetaData.h"
#include "IscResultSet.h"
#include "IscPreparedStatement.h"
#include "IscBlob.h"

#define TABLE_TYPE	4


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscTablesResultSet::IscTablesResultSet(IscDatabaseMetaData *metaData)
		: IscMetaDataResultSet(metaData)
{
	resultSet = NULL;
}

IscTablesResultSet::~IscTablesResultSet()
{
	if (resultSet)
		resultSet->release();
}

void IscTablesResultSet::getTables(const char * catalog, const char * schemaPattern, const char * tableNamePattern, int typeCount, const char * * types)
{
	JString sql = "select NULL as table_cat,"
				          "NULL as table_schem,"
						  "rdb$relation_name as table_name,"
						  "rdb$view_blr as table_type,"
						  "rdb$description as remarks,"
						  "rdb$system_flag "
						  "from rdb$relations\n";

	const char *sep = " where (";

	if (tableNamePattern && *tableNamePattern)
		{
		sql += expandPattern (" where rdb$relation_name %s '%s'\n", tableNamePattern);
		sep = " and (";
		}

	JString adjunct;
		
	for (int n = 0; n < typeCount; ++n)
		if (!strcmp (types [n], "TABLE"))
			{
			adjunct += sep;
			adjunct += "(rdb$view_blr is null and rdb$system_flag = 0)";
			sep = " or ";
			}
		else if (!strcmp (types [n], "VIEW"))
			{
			adjunct += sep;
			adjunct += "rdb$view_blr is not null";
			sep = " or ";
			}
		else if (!strcmp (types [n], "SYSTEM TABLE"))
			{
			adjunct += sep;
			adjunct += "(rdb$view_blr is null and rdb$system_flag = 1)";
			sep = " or ";
			}

	if (!adjunct.IsEmpty())
		{
		sql += adjunct;
		sql += ")\n";
		}

	sql += " order by rdb$system_flag desc, rdb$owner_name, rdb$relation_name";
	
	prepareStatement (sql);
	numberColumns = 5;
}

bool IscTablesResultSet::next()
{
	if (!resultSet->next())
		return false;

	const char *type = "TABLE";

	if (resultSet->getInt (6))
		type = "SYSTEM TABLE";
	else
		{
		Blob *blob = resultSet->getBlob (4);
		if (!resultSet->wasNull())
			type = "VIEW";
		//blob->release();
		}

	resultSet->setValue (4, type);
	trimBlanks (3);

	return true;
}

int IscTablesResultSet::getColumnDisplaySize(int index)
{
	switch (index)
		{
		case TABLE_TYPE:				// change from blob to text
			return 128;
		}

	return Parent::getColumnDisplaySize (index);
}

int IscTablesResultSet::getColumnType(int index)
{
	switch (index)
		{
		case TABLE_TYPE:				// change from blob to text
			return JDBC_VARCHAR;
		}

	return Parent::getColumnType (index);
}

int IscTablesResultSet::getColumnPrecision(int index)
{
	switch (index)
		{
		case TABLE_TYPE:				// change from blob to text
			return 128;
		}

	return Parent::getPrecision (index);
}
