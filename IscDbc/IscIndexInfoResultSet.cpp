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
 *
 *	Changes
 *
 *	2002-05-20	IscIndexInfoResultSet.cpp
 *
 *				Contributed by ? 
 *				o qualify the column names in getIndexInfo()
 *				
 *
 */

// IscIndexInfoResultSet.cpp: implementation of the IscIndexInfoResultSet class.
//
//////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#ifdef _WINDOWS
#include <windows.h>
#endif
#endif

#include <stdio.h>
#include "IscDbc.h"
#include "IscIndexInfoResultSet.h"
#include "IscDatabaseMetaData.h"

#define ASC_DSC	10

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscIndexInfoResultSet::IscIndexInfoResultSet(IscDatabaseMetaData *metaData)
		: IscMetaDataResultSet(metaData)
{

}

#define SetWhereOrAnd(stat) \
	(stat ? (stat = false,szWhere) : szAnd)

void IscIndexInfoResultSet::getIndexInfo(const char * catalog, 
										 const char * schemaPattern, 
										 const char * tableNamePattern, 
										 bool unique, bool approximate)
{
	const char *szWhere = " where ";
	const char *szAnd = " and ";
	char sqlQuery[4096] =
		"select cast('' as varchar(7)) as table_cat,\n"						// 1
				"\tcast(rl.rdb$owner_name as varchar(31)) as table_schem,\n"	// 2
				"\tcast(rl.rdb$relation_name as varchar(31)) as table_name,\n"	// 3
				"\tcast(0 as smallint) as non_unique,\n"						// 4
				"\tcast(NULL as varchar(31)) as index_qualifier,\n"				// 5
				"\tcast(NULL as varchar(31)) index_name,\n"						// 6
				"\tcast(0 as smallint) as index_type,\n"						// 7  SQL_TABLE_STAT
				"\tcast(NULL as smallint) as ordinal_position,\n"				// 8
				"\tcast(NULL as varchar(31)) as column_name,\n"					// 9
				"\tcast(NULL as char CHARACTER SET NONE) as asc_or_desc,\n"						// 10
				"\tcast(NULL as integer) as cardinality,\n"						// 11
				"\tcast(NULL as integer) as index_pages,\n"						// 12
				"\tcast(NULL as varchar(31)) as filter_condition,\n"			// 13
				"\tcast(NULL as smallint) as index_type,\n"						// 14
				"\tcast(NULL as varchar(31)) as constraint_type\n"				// 15
		"from rdb$relations rl\n";

	char sql[2048] =
		"\tunion\n" 
		"select cast('' as varchar(7)) as table_cat,\n"						// 1
				"\tcast(tbl.rdb$owner_name as varchar(31)) as table_schem,\n"	// 2
				"\tcast(idx.rdb$relation_name as varchar(31)) as table_name,\n"	// 3
				"\tcast((1-idx.rdb$unique_flag) as smallint) as non_unique,\n"	// 4
				"\tcast(idx.rdb$index_name as varchar(31)) as index_qualifier,\n"	// 5
				"\tcast(idx.rdb$index_name as varchar(31)) as index_name,\n"	// 6
				"\tcast(3 as smallint) as index_type,\n"						// 7 (SQL_INDEX_OTHER)
				"\tcast(seg.rdb$field_position as smallint) as ordinal_position,\n"	// 8
				"\tcast(coalesce(seg.rdb$field_name,\n" 
				"substring(idx.rdb$expression_source from 1 for 31)) as varchar(31)) as column_name,\n"	// 9
				"\tcast(NULL as char CHARACTER SET NONE) as asc_or_desc,\n"						// 10
				"\tcast((case when idx.rdb$statistics = 0 then 0 else\n" 
				"1/idx.rdb$statistics end) as integer) as cardinality,\n"		// 11
				"\tcast(NULL as integer) as index_pages,\n"						// 12
				"\tcast(NULL as varchar(31)) as filter_condition,\n"			// 13
				"\tcast(idx.rdb$index_type as smallint) as index_type,\n"		// 14
				"\tcast(relc.rdb$constraint_type as varchar(31)) as constraint_type\n"	// 15
		"from rdb$indices idx\n"
			"\tleft join rdb$relations tbl on tbl.rdb$relation_name = idx.rdb$relation_name\n"
			"\tleft join rdb$index_segments seg on idx.rdb$index_name = seg.rdb$index_name\n"
			"\tleft join rdb$relation_constraints relc on ( relc.rdb$index_name = idx.rdb$index_name\n";

	char * ptFirst = sqlQuery + strlen(sqlQuery);
	char * ptSecond = sql + strlen(sql);
	bool firstWhere = true;
	bool secondWhere = true;

	if (unique)
	{
		addString(ptSecond, "\t\t\tand relc.rdb$relation_name = idx.rdb$relation_name\n"
							"\t\t\tand ( relc.rdb$constraint_type = 'PRIMARY KEY' or relc.rdb$constraint_type = 'UNIQUE' ) )\n"
							"where idx.rdb$unique_flag = 1\n");
		secondWhere = false;
	}
	else
		addString(ptSecond, "\t\t\tand relc.rdb$relation_name = idx.rdb$relation_name )\n");

	if (schemaPattern && *schemaPattern)
	{
		expandPattern (ptFirst, SetWhereOrAnd( firstWhere ),"rl.rdb$owner_name", schemaPattern);
		expandPattern (ptSecond, SetWhereOrAnd( secondWhere ),"tbl.rdb$owner_name", schemaPattern);
	}

	if (tableNamePattern && *tableNamePattern)
	{
		expandPattern (ptFirst, SetWhereOrAnd( firstWhere ),"rl.rdb$relation_name", tableNamePattern);
		expandPattern (ptSecond, SetWhereOrAnd( secondWhere ),"idx.rdb$relation_name", tableNamePattern);
	}

	addString(ptSecond, " order by 4, 7, 15, 5, 6, 8\n");
	addString(ptFirst, sql);

	prepareStatement (sqlQuery);

// SELECT returns 14 columns,
// But all interests only 13 (SQL 92,99)
// This line is forbidden for modifying!!!
	numberColumns = 13; 
}

bool IscIndexInfoResultSet::nextFetch()
{
	if (!IscResultSet::nextFetch())
		return false;

	if ( !metaData->getUseSchemaIdentifier() )
		sqlda->setNull(2);

	short type = sqlda->getShort(7);

	if( type == 0 ) // #define SQL_TABLE_STAT 0
		sqlda->setNull(4);
	else 
	{
		if ( sqlda->isNull(4) )
			sqlda->updateShort(4,1);

		short position = sqlda->getShort(8);
		sqlda->updateShort(8,position+1);

		short type = sqlda->getShort (14);
		sqlda->updateText (10, (type) ? "D" : "A");	
	}

	return true;
}

}; // end namespace IscDbcLibrary
