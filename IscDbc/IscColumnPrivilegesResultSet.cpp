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

// IscColumnPrivilegesResultSet.cpp: implementation of the IscColumnPrivilegesResultSet class.
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include "IscDbc.h"
#include "IscColumnPrivilegesResultSet.h"
#include "IscConnection.h"
#include "IscDatabaseMetaData.h"
#include "IscResultSet.h"
#include "IscPreparedStatement.h"
#include "IscBlob.h"

#define TABLE_TYPE	4


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscColumnPrivilegesResultSet::IscColumnPrivilegesResultSet(IscDatabaseMetaData *metaData)
		: IscMetaDataResultSet(metaData)
{
	resultSet = NULL;
}

IscColumnPrivilegesResultSet::~IscColumnPrivilegesResultSet()
{
	if (resultSet)
		resultSet->release();
}

void IscColumnPrivilegesResultSet::getColumnPrivileges(const char * catalog, const char * schemaPattern, const char * tableNamePattern, const char * columnNamePattern)
{
	JString sql = "select NULL as table_cat,"
				          "NULL as table_schem,"
						  "tbl.rdb$relation_name as table_name,"
						  "usp.rdb$field_name as column_name,"
						  "usp.rdb$grantor as grantor,"
						  "usp.rdb$user as grantee,"
						  "usp.rdb$privilege as privilege,"
						  "usp.rdb$grant_option as isgrantable "
						  "from rdb$relations tbl, rdb$user_privileges usp\n"
						  " where tbl.rdb$relation_name = usp.rdb$relation_name\n";
	
	if (tableNamePattern)
		sql += expandPattern (" and tbl.rdb$relation_name %s '%s'", tableNamePattern);

	if (columnNamePattern)
		sql += expandPattern (" and usp.rdb$field_name %s '%s'", columnNamePattern);

	sql += " order by tbl.rdb$relation_name, usp.rdb$field_name, usp.rdb$privilege";

	prepareStatement (sql);
	numberColumns = 8;
}

bool IscColumnPrivilegesResultSet::next()
{
	if (!resultSet->next())
		return false;

	trimBlanks(3);
	trimBlanks(4);
	trimBlanks(5);
	trimBlanks(6);

	const char *privilege = resultSet->getString(7);

	switch ( *privilege )
	{
		case 'S':
			resultSet->setValue( 7, "SELECT" );
			break;

		case 'I':
			resultSet->setValue( 7, "INSERT" );
			break;

		case 'U':
			resultSet->setValue( 7, "UPDATE" );
			break;

		case 'D':
			resultSet->setValue( 7, "DELETE" );
			break;

		case 'R':
			resultSet->setValue( 7, "REFERENCES" );
			break;
	}

	int nullable = resultSet->getInt(8);
	if ( nullable )
		resultSet->setValue( 8, "YES" );
	else
		resultSet->setValue( 8, "NO" );

	return true;
}

int IscColumnPrivilegesResultSet::getColumnDisplaySize(int index)
{
	switch (index)
		{
		case TABLE_TYPE:				// change from blob to text
			return 128;
		}

	return Parent::getColumnDisplaySize (index);
}

int IscColumnPrivilegesResultSet::getColumnType(int index)
{
	switch (index)
		{
		case TABLE_TYPE:				// change from blob to text
			return JDBC_VARCHAR;
		}

	return Parent::getColumnType (index);
}

int IscColumnPrivilegesResultSet::getColumnPrecision(int index)
{
	switch (index)
		{
		case TABLE_TYPE:				// change from blob to text
			return 128;
		}

	return Parent::getPrecision (index);
}
