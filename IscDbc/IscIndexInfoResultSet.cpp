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

IscIndexInfoResultSet::~IscIndexInfoResultSet()
{

}

void IscIndexInfoResultSet::getIndexInfo(const char * catalog, 
										 const char * schemaPattern, 
										 const char * tableNamePattern, 
										 bool unique, bool approximate)
{
	const char *v6 = 
		"select NULL as table_cat,\n"								// 1
				"\tNULL as table_schem,\n"							// 2
				"\tidx.rdb$relation_name as table_name,\n"			// 3
				"\tidx.rdb$unique_flag as non_unique,\n"			// 4
				"\tidx.rdb$index_name as index_qualifier,\n"		// 5
				"\tidx.rdb$index_name as index_name,\n"				// 6
				"\tseg.rdb$field_position as \"TYPE\",\n"			// 7 (SQL_INDEX_OTHER)
				"\tseg.rdb$field_position as ordinal_position,\n"	// 8
				"\tseg.rdb$field_name as column_name,\n"			// 9
				"\tNULL as asc_or_desc,\n"							// 10
				"\tNULL as cardinality,\n"							// 11
				"\tNULL as \"PAGES\",\n"							// 12
				"\tNULL as filter_condition,\n"						// 13
				"\tidx.rdb$index_type as index_type\n"				// 14
		"from rdb$indices idx, rdb$index_segments seg\n"
		" where idx.rdb$index_name = seg.rdb$index_name\n";

	const char *preV6 = 
		"select NULL as table_cat,\n"								// 1
				"\tNULL as table_schem,\n"							// 2
				"\tidx.rdb$relation_name as table_name,\n"			// 3
				"\tidx.rdb$unique_flag as non_unique,\n"			// 4
				"\tidx.rdb$index_name as index_qualifier,\n"		// 5
				"\tidx.rdb$index_name as index_name,\n"				// 6
				"\tseg.rdb$field_position as odbc_type,\n"			// 7 (SQL_INDEX_OTHER)
				"\tseg.rdb$field_position as ordinal_position,\n"	// 8
				"\tseg.rdb$field_name as column_name,\n"			// 9				
				"\tNULL as asc_or_desc,\n"							// 10
				"\t0 as cardinality,\n"								// 11
				"\t0 as odbc_pages,\n"								// 12
				"\tNULL as filter_condition,\n"						// 13
				"\tidx.rdb$index_type as index_type\n"				// 14
		"from rdb$indices idx, rdb$index_segments seg\n"
		" where idx.rdb$index_name = seg.rdb$index_name\n";

	JString sql = (metaData->storesMixedCaseQuotedIdentifiers()) ? v6 : preV6;

	if (tableNamePattern)
		sql += expandPattern (" and idx.rdb$relation_name %s '%s'\n", tableNamePattern);

	if (unique)
		sql += " and idx.rdb$unique_flag = 1\n";

	if (isWildcarded(tableNamePattern)) // all tables
		sql += " order by idx.rdb$relation_name, idx.rdb$unique_flag desc, idx.rdb$index_name, seg.rdb$field_position\n";
	else
		sql += " order by idx.rdb$unique_flag desc, idx.rdb$index_name, seg.rdb$field_position\n";

	prepareStatement (sql);
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
	trimBlanks (10);			// asc or desc

	int uniqueFlag = resultSet->getInt (4);
	resultSet->setValue (4, (uniqueFlag) ? 0 : 1);

	resultSet->setValue( 7, (short)3 );

	int position = resultSet->getInt(8);
	resultSet->setValue(8,position+1);

	int type = resultSet->getInt (14);
	resultSet->setValue (10, (type) ? "D" : "A");	

	return true;
}

int IscIndexInfoResultSet::getColumnDisplaySize(int index)
{
	switch (index)
		{
		case ASC_DSC:					//	currently 10
			return 1;
		}

	return Parent::getColumnDisplaySize (index);
}

int IscIndexInfoResultSet::getColumnType(int index)
{
	switch (index)
		{
		case ASC_DSC:					//	currently 10
			return JDBC_CHAR;
		}

	return Parent::getColumnType (index);
}

int IscIndexInfoResultSet::getColumnPrecision(int index)
{
	switch (index)
		{
		case ASC_DSC:					//	currently 10
			return 1;
		}

	return Parent::getPrecision (index);
}
