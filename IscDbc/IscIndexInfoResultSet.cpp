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
#ifdef _WIN32
#include <windows.h>
#endif
#endif

#include <stdio.h>
#include "IscDbc.h"
#include "IscIndexInfoResultSet.h"
#include "IscDatabaseMetaData.h"

#define ASC_DSC	10

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscIndexInfoResultSet::IscIndexInfoResultSet(IscDatabaseMetaData *metaData)
		: IscMetaDataResultSet(metaData)
{

}

void IscIndexInfoResultSet::getIndexInfo(const char * catalog, 
										 const char * schemaPattern, 
										 const char * tableNamePattern, 
										 bool unique, bool approximate)
{
	JString tableStat = 
		"select cast(NULL as char(31)) as table_cat,\n"							// 1
				"\tcast(NULL as char(31)) as table_schem,\n"					// 2
				"\tcast(rl.rdb$relation_name as char(31)) as table_name,\n"		// 3
				"\tcast(0 as smallint) as non_unique,\n"						// 4
				"\tcast(NULL as char(31)) as index_qualifier,\n"				// 5
				"\tcast(NULL as char(31)) index_name,\n"						// 6
				"\tcast(0 as smallint) as index_type,\n"						// 7  SQL_TABLE_STAT
				"\tcast(NULL as smallint) as ordinal_position,\n"				// 8
				"\tcast(NULL as char(31)) as column_name,\n"					// 9
				"\tcast(NULL as char) as asc_or_desc,\n"						// 10
				"\tcast(NULL as integer) as cardinality,\n"						// 11
				"\tcast(NULL as integer) as index_pages,\n"						// 12
				"\tcast(NULL as varchar(31)) as filter_condition,\n"			// 13
				"\tcast(NULL as smallint) as index_type\n"						// 14
		"from rdb$relations rl\n";

	JString sql = 
		"select cast(NULL as char(31)) as table_cat,\n"							// 1
				"\tcast(NULL as char(31)) as table_schem,\n"					// 2
				"\tcast(idx.rdb$relation_name as char(31)) as table_name,\n"	// 3
				"\tcast((1-idx.rdb$unique_flag) as smallint) as non_unique,\n"	// 4
				"\tcast(idx.rdb$index_name as char(31)) as index_qualifier,\n"	// 5
				"\tcast(idx.rdb$index_name as char(31)) as index_name,\n"		// 6
				"\tcast(3 as smallint) as index_type,\n"						// 7 (SQL_INDEX_OTHER)
				"\tcast(seg.rdb$field_position as smallint) as ordinal_position,\n"	// 8
				"\tcast(seg.rdb$field_name as char(31)) as column_name,\n"		// 9
				"\tcast(NULL as char) as asc_or_desc,\n"						// 10
				"\tcast(NULL as integer) as cardinality,\n"						// 11
				"\tcast(NULL as integer) as index_pages,\n"						// 12
				"\tcast(NULL as varchar(31)) as filter_condition,\n"			// 13
				"\tcast(idx.rdb$index_type as smallint) as index_type\n"		// 14
		"from rdb$indices idx, rdb$index_segments seg\n"
		"where idx.rdb$index_name = seg.rdb$index_name\n";

	if (tableNamePattern && *tableNamePattern)
	{
		tableStat += expandPattern (" where ","rl.rdb$relation_name", tableNamePattern);
		sql += expandPattern (" and ","idx.rdb$relation_name", tableNamePattern);

		if ( !metaData->allTablesAreSelectable() )
		{
			tableStat += metaData->existsAccess(" and ", "rl", 0, "\n");
			sql += metaData->existsAccess(" and ", "idx", 0, "\n");
		}
	}

	if (unique)
		sql += " and idx.rdb$unique_flag = 1\n";

	sql += " order by 4, 7, 5, 6, 8\n";
	sql = tableStat + "\tunion\n" + sql;

	prepareStatement (sql);

// SELECT returns 14 columns,
// But all interests only 13 (SQL 92,99)
// This line is forbidden for modifying!!!
	numberColumns = 13; 
}

bool IscIndexInfoResultSet::next()
{
	if (!resultSet->next())
		return false;

	trimBlanks (3);				// table name
	trimBlanks (5);				// qualifier name
	trimBlanks (6);				// index name
	trimBlanks (9);				// field name

	int type = resultSet->getInt(7);

	if( type == 0 ) // #define SQL_TABLE_STAT 0
		resultSet->setNull(4);
	else 
	{
#pragma FB_COMPILER_MESSAGE("RDB$UNIQUE_FLAG Whether there be NULL?; FIXME!")
		if ( resultSet->isNull(4) )
			resultSet->setValue(4, (short)1);

		int position = resultSet->getInt(8);
		resultSet->setValue(8,position+1);

		int type = resultSet->getInt (14);
		resultSet->setValue (10, (type) ? "D" : "A");	
	}

	return true;
}

int IscIndexInfoResultSet::getColumnDisplaySize(int index)
{
	switch (index)
		{
		case 1:
			return 31;

		case ASC_DSC:					//	currently 10
			return 1;
		}

	return Parent::getColumnDisplaySize (index);
}

int IscIndexInfoResultSet::getColumnType(int index, int &realSqlType)
{
	switch (index)
		{
		case ASC_DSC:					//	currently 10
			return JDBC_CHAR;
		}

	return Parent::getColumnType (index, realSqlType);
}

int IscIndexInfoResultSet::getPrecision(int index)
{
	switch (index)
		{
		case ASC_DSC:					//	currently 10
			return 10;
		default:
			return 31;
		}

//	return Parent::getPrecision (index);
}
