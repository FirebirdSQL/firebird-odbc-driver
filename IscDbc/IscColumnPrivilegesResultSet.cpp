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

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscColumnPrivilegesResultSet::IscColumnPrivilegesResultSet(IscDatabaseMetaData *metaData)
		: IscMetaDataResultSet(metaData)
{
	resultSet = NULL;
}

void IscColumnPrivilegesResultSet::getColumnPrivileges(const char * catalog, const char * schemaPattern, const char * tableNamePattern, const char * columnNamePattern)
{
	JString sql = "select NULL as table_cat,"
				          "NULL as table_schem,"
						  "tbl.rdb$relation_name as table_name,"
						  "tbl.rdb$field_name as column_name,"
						  "priv.rdb$grantor as grantor,"
						  "priv.rdb$user as grantee,"
						  "cast( priv.rdb$privilege as char(11) ) as privilege,"
						  "cast ( priv.rdb$grant_option as char(4) ) as is_grantable "
						  "from rdb$relation_fields tbl, rdb$user_privileges priv\n"
						  " where tbl.rdb$relation_name = priv.rdb$relation_name\n";
	
	if ( !metaData->allTablesAreSelectable() )
	{
		char buf[128];
		sprintf (buf, "and priv.rdb$object_type = 0\n"
					  "and priv.rdb$user = '%s' and priv.rdb$user_type = %d\n",
						metaData->getUserAccess(),metaData->getUserType());
		sql +=	buf;
	}

	if (tableNamePattern && *tableNamePattern)
		sql += expandPattern (" and ","tbl.rdb$relation_name", tableNamePattern);

	if (columnNamePattern && *columnNamePattern)
		sql += expandPattern (" and ","tbl.rdb$field_name", columnNamePattern);

	sql += " order by tbl.rdb$relation_name, tbl.rdb$field_name, priv.rdb$privilege";

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

	const char *grantor = resultSet->getString(5);
	const char *grantee = resultSet->getString(6);
	if(!strcmp(grantor,grantee))
		resultSet->setValue( 5, "_SYSTEM" );

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
	return Parent::getColumnDisplaySize (index);
}

int IscColumnPrivilegesResultSet::getColumnType(int index, int &realSqlType)
{
	return Parent::getColumnType (index, realSqlType);
}

int IscColumnPrivilegesResultSet::getColumnPrecision(int index)
{
	return Parent::getPrecision (index);
}

}; // end namespace IscDbcLibrary
