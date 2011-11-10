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
#define REMARKS		5

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscTablesResultSet::IscTablesResultSet(IscDatabaseMetaData *metaData)
		: IscMetaDataResultSet(metaData)
{
	sqlAllParam = 0;
	curentRowAllParam = 0;
}

void IscTablesResultSet::getTables(const char * catalog, const char * schemaPattern, const char * tableNamePattern, int typeCount, const char * * types)
{
	const char *sqlAll =  "%";
	char sql[2048] = "";
	char * ptSql = sql;
	char * pt = ptSql;

	addString(pt, "select cast( '");
	if (catalog && *catalog)
		addString(pt, catalog);
	addString(pt, "' as varchar(255)) as table_cat,\n"									// 1
					  "cast (tbl.rdb$owner_name as varchar(31)) as table_schem,\n"		// 2
					  "cast (tbl.rdb$relation_name as varchar(31)) as table_name,\n"	// 3
					  "cast ('TABLE' as varchar(13)) as table_type,\n"					// 4
					  "cast (NULL as varchar(255)) as remarks,\n"						// 5
					  "tbl.rdb$system_flag,\n"											// 6
					  "tbl.rdb$view_blr as view_blr,\n"									// 7
					  "tbl.rdb$description as remarks_blob\n"							// 8
					  "from rdb$relations tbl\n");

	char * ptFirst = sql + strlen(sql);
	const char *sep = " where (";
	bool firstWhere = true;

	do
	{
		if ( !(catalog && *catalog) )
			++sqlAllParam;
		else if ( *(short*)catalog == *(short*)sqlAll // SQL_ALL_CATALOGS
			&& !(schemaPattern && *schemaPattern)
			&& !(tableNamePattern && *tableNamePattern) )
		{
			*ptSql = '\0';
			pt = ptSql;
			addString(pt, "select cast( '");
			addString(pt, metaData->getDSN());
			addString(pt, "' as varchar(31)) as table_cat,\n"	        // 1
				    "cast (NULL as varchar(31)) as table_schem,\n"		// 2
					"cast (NULL as varchar(31)) as table_name,\n"		// 3
					"cast (NULL as varchar(13)) as table_type,\n"		// 4
					"cast (NULL as varchar(255)) as remarks\n"			// 5
					"from rdb$database tbl\n");
			*pt = '\0';
			sqlAllParam = 2; 
			break;
		}

		if ( !(schemaPattern && *schemaPattern) )
			++sqlAllParam;
		else if ( *(short*)schemaPattern == *(short*)sqlAll // SQL_ALL_SCHEMAS
			&& sqlAllParam
			&& !(tableNamePattern && *tableNamePattern) )
		{
			ptSql = "select distinct cast (NULL as varchar(7)) as table_cat,\n"	 // 1
			        "cast (tbl.rdb$owner_name as varchar(31)) as table_schem,\n" // 2
					"cast (NULL as varchar(31)) as table_name,\n"				 // 3
					"cast (NULL as varchar(13)) as table_type,\n"				 // 4
					"cast (NULL as varchar(255)) as remarks\n"					 // 5
				    "from rdb$relations tbl\n";
			sqlAllParam = 2; // unique owners
			break;
		}

		if ( typeCount == 1
			&& *(short*)types[0] == *(short*)sqlAll // SQL_ALL_TABLE_TYPES
			&& sqlAllParam == 2
			&& !(tableNamePattern && *tableNamePattern) )
		{
			ptSql = "select cast (NULL as varchar(7)) as table_cat,\n"		// 1
				    "cast (NULL as varchar(31)) as table_schem,\n"			// 2
					"cast (NULL as varchar(31)) as table_name,\n"			// 3
					"cast ('SYSTEM TABLE' as varchar(13)) as table_type,\n"	// 4
					"cast (NULL as varchar(255)) as remarks\n"				// 5
					"from rdb$database tbl\n";
			sqlAllParam = 3; // unique table types
			break;
		}

		sqlAllParam = 0;
		if (schemaPattern && *schemaPattern)
		{
			expandPattern (ptFirst, " where ","tbl.rdb$owner_name", schemaPattern);
			sep = " and (";
			firstWhere = false;
		}

		if (tableNamePattern && *tableNamePattern)
		{
			expandPattern (ptFirst, firstWhere ? " where " : " and ", "tbl.rdb$relation_name", tableNamePattern);
			sep = " and (";
		}

		pt = ptFirst;
			
		for (int n = 0; n < typeCount; ++n)
			if (!strcmp (types [n], "TABLE"))
				{
				addString(pt, sep);
				addString(pt, "(tbl.rdb$view_blr is null and tbl.rdb$system_flag = 0)");
				sep = " or ";
				}
			else if (!strcmp (types [n], "VIEW"))
				{
				addString(pt, sep);
				addString(pt, "tbl.rdb$view_blr is not null");
				sep = " or ";
				}
			else if (!strcmp (types [n], "SYSTEM TABLE"))
				{
				addString(pt, sep);
				addString(pt, "(tbl.rdb$view_blr is null and tbl.rdb$system_flag = 1)");
				sep = " or ";
				}

		if ( pt > ptFirst )
			{
			ptFirst = pt;
			addString(ptFirst, ")\n");
			}

		addString(ptFirst, " order by tbl.rdb$system_flag desc, tbl.rdb$owner_name, tbl.rdb$relation_name");

	} while ( false );

	prepareStatement (ptSql);
	numberColumns = 5;
}

bool IscTablesResultSet::nextFetch()
{
	if ( sqlAllParam )
	{
		if ( sqlAllParam == 1 )
			return false;
		else if ( sqlAllParam == 3 && curentRowAllParam )
		{
			if ( curentRowAllParam == 1 )
			{
				sqlda->restoreBufferToCurrentSqlda();
				sqlda->updateVarying (4, "TABLE");
			}
			else if ( curentRowAllParam == 2 )
			{
				sqlda->restoreBufferToCurrentSqlda();
				sqlda->updateVarying (4, "VIEW");
			}
			else
				return false;

			++curentRowAllParam;
			return true;
		}
 			
		if (!IscResultSet::nextFetch())
			return false;

		if ( sqlAllParam == 3 )
			sqlda->saveCurrentSqldaToBuffer();

		++curentRowAllParam;
		return true;
	}

	if (!IscResultSet::nextFetch())
		return false;

	if ( !metaData->getUseSchemaIdentifier() )
		sqlda->setNull(2);

	if ( sqlda->getShort (6) )
		sqlda->updateVarying (4, "SYSTEM TABLE");
	else if ( !sqlda->isNull(7) )
		sqlda->updateVarying (4, "VIEW");

	if ( !sqlda->isNull(8) )
		convertBlobToString( 5, 8 );

	return true;
}

}; // end namespace IscDbcLibrary
