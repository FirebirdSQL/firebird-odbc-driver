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
}

void IscColumnPrivilegesResultSet::getColumnPrivileges(const char * catalog, const char * schemaPattern, const char * tableNamePattern, const char * columnNamePattern)
{
	char sql[4096] = "select cast ('' as varchar(7)) as table_cat,"
				          "cast (tbl.rdb$owner_name as varchar(31)) as table_schem,"
						  "cast (rfr.rdb$relation_name as varchar(31)) as table_name,"
						  "cast (rfr.rdb$field_name as varchar(31)) as column_name,"
						  "cast (priv.rdb$grantor as varchar(31)) as grantor,"
						  "cast (priv.rdb$user as varchar(31)) as grantee,"
						  "cast( priv.rdb$privilege as varchar(11) ) as privilege,"
						  "cast ( priv.rdb$grant_option as varchar(3) ) as is_grantable "
						  "from rdb$relation_fields rfr, rdb$user_privileges priv, rdb$relations tbl\n"
						  " where rfr.rdb$relation_name = priv.rdb$relation_name\n"
						  "		and rfr.rdb$relation_name = tbl.rdb$relation_name\n";
	
	char * ptFirst = sql + strlen(sql);

	if ( !metaData->allTablesAreSelectable() )
	{
		char buf[256];
		int len = sprintf (buf, "and priv.rdb$object_type = 0\n"
					  "and ( (priv.rdb$user = '%s' and priv.rdb$user_type = %d)\n"
					  "\tor (priv.rdb$user = 'PUBLIC' and priv.rdb$user_type = 8) )\n",
						metaData->getUserAccess(),metaData->getUserType());
		addString(ptFirst, buf, len);
	}

	if (schemaPattern && *schemaPattern)
		expandPattern (ptFirst, " and ","tbl.rdb$owner_name", schemaPattern);

	if (tableNamePattern && *tableNamePattern)
		expandPattern (ptFirst, " and ","rfr.rdb$relation_name", tableNamePattern);

	if (columnNamePattern && *columnNamePattern)
		expandPattern (ptFirst, " and ","rfr.rdb$field_name", columnNamePattern);

	addString(ptFirst, " order by rfr.rdb$relation_name, rfr.rdb$field_name, priv.rdb$privilege");

	prepareStatement (sql);
	numberColumns = 8;
}

bool IscColumnPrivilegesResultSet::nextFetch()
{
	if (!IscResultSet::nextFetch())
		return false;

	if ( !metaData->getUseSchemaIdentifier() )
		sqlda->setNull(2);

	int len1, len2;
	const char *grantor = sqlda->getVarying(5, len1);
	const char *grantee = sqlda->getVarying(6, len2);

	if( len1 == len2 && !strncmp(grantor,grantee,len1) )
		sqlda->updateVarying (5, "_SYSTEM");

	const char *privilege = sqlda->getVarying (7, len1);

	switch ( *privilege )
	{
		case 'S':
			sqlda->updateVarying ( 7, "SELECT" );
			break;

		case 'I':
			sqlda->updateVarying ( 7, "INSERT" );
			break;

		case 'U':
			sqlda->updateVarying ( 7, "UPDATE" );
			break;

		case 'D':
			sqlda->updateVarying ( 7, "DELETE" );
			break;

		case 'R':
			sqlda->updateVarying ( 7, "REFERENCES" );
			break;
	}

	char * nullable = sqlda->getVarying (8, len1);
	if ( *nullable == '1' )
		sqlda->updateVarying ( 8, "YES" );
	else
		sqlda->updateVarying ( 8, "NO" );

	return true;
}

}; // end namespace IscDbcLibrary
