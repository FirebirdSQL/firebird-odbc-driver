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

// IscPrimaryKeysResultSet.cpp: implementation of the IscPrimaryKeysResultSet class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "IscDbc.h"
#include "IscPrimaryKeysResultSet.h"
#include "IscDatabaseMetaData.h"

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscPrimaryKeysResultSet::IscPrimaryKeysResultSet(IscDatabaseMetaData *metaData)
		: IscMetaDataResultSet(metaData)
{

}

void IscPrimaryKeysResultSet::getPrimaryKeys(const char * catalog, const char * schemaPattern, const char * tableNamePattern)
{
	char sql[2048] =
		"select cast ('' as varchar(7)) as table_cat,\n"							// 1
				"\tcast (tbl.rdb$owner_name as varchar(31)) as table_schem,\n"		// 2
				"\tcast (rel.rdb$relation_name as varchar(31)) as table_name,\n"	// 3
				"\tcast (seg.rdb$field_name as varchar(31)) as column_name,\n"		// 4
				"\tcast (seg.rdb$field_position+1 as smallint) as key_seq,\n"		// 5
				"\tcast (rel.rdb$constraint_name as varchar(31)) as pk_name\n"		// 6
		"from rdb$relation_constraints rel, rdb$indices idx, rdb$index_segments seg,\n"
		"     rdb$relations tbl\n"
		" where rel.rdb$constraint_type = 'PRIMARY KEY'\n"
			" and rel.rdb$relation_name = tbl.rdb$relation_name\n"
			" and rel.rdb$index_name = idx.rdb$index_name\n"
			" and idx.rdb$index_name = seg.rdb$index_name\n";

	char * ptFirst = sql + strlen(sql);

	if (schemaPattern && *schemaPattern)
		expandPattern (ptFirst, " and ","tbl.rdb$owner_name", schemaPattern);

	if (tableNamePattern && *tableNamePattern)
		expandPattern (ptFirst, " and ","rel.rdb$relation_name", tableNamePattern);

	addString(ptFirst, " order by rel.rdb$relation_name, idx.rdb$index_name, seg.rdb$field_position");
	prepareStatement (sql);
	numberColumns = 6;
}

bool IscPrimaryKeysResultSet::nextFetch()
{
	if (!IscResultSet::nextFetch())
		return false;

	if ( !metaData->getUseSchemaIdentifier() )
		sqlda->setNull(2);

	return true;
}

}; // end namespace IscDbcLibrary
