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

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscColumnPrivilegesResultSet::IscColumnPrivilegesResultSet(IscDatabaseMetaData *metaData)
		: IscMetaDataResultSet(metaData)
{
}

void IscColumnPrivilegesResultSet::getColumnPrivileges(const char * catalog, const char * schemaPattern, const char * tableNamePattern, const char * columnNamePattern)
{
	JString sql = "select cast (NULL as char(7)) as table_cat,"
				          "cast (NULL as char(7)) as table_schem,"
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
	if (!IscResultSet::next())
		return false;

	trimBlanks(3);
	trimBlanks(4);
	trimBlanks(5);
	trimBlanks(6);

	int len;
	const char *grantor = sqlda->getText(5, len);
	const char *grantee = sqlda->getText(6, len);
	if(!strcmp(grantor,grantee))
		sqlda->updateText( 5, "_SYSTEM" );

	const char *privilege = sqlda->getText(7, len);

	switch ( *privilege )
	{
		case 'S':
			sqlda->updateText( 7, "SELECT" );
			break;

		case 'I':
			sqlda->updateText( 7, "INSERT" );
			break;

		case 'U':
			sqlda->updateText( 7, "UPDATE" );
			break;

		case 'D':
			sqlda->updateText( 7, "DELETE" );
			break;

		case 'R':
			sqlda->updateText( 7, "REFERENCES" );
			break;
	}

	char * nullable = sqlda->getText(8,len);
	if ( *nullable == '1' )
		sqlda->updateText( 8, "YES" );
	else
		sqlda->updateText( 8, "NO" );

	return true;
}
