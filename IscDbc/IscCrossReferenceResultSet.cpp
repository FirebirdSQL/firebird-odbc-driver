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

// IscCrossReferenceResultSet.cpp: implementation of the IscCrossReferenceResultSet class.
//
//////////////////////////////////////////////////////////////////////

#include <string.h>
#include "IscDbc.h"
#include "IscCrossReferenceResultSet.h"
#include "IscDatabaseMetaData.h"

#define SQL_CASCADE				0
#define SQL_RESTRICT			1
#define SQL_SET_NULL			2
#define SQL_NO_ACTION			3
#define SQL_SET_DEFAULT			4

#define UPD_RULE		10
#define DEL_RULE		11

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscCrossReferenceResultSet::IscCrossReferenceResultSet(IscDatabaseMetaData *metaData)
		: IscMetaDataResultSet(metaData)
{

}

void IscCrossReferenceResultSet::getCrossReference (const char * primaryCatalog, 
													const char * primarySchema, 
													const char * primaryTable, 
													const char * foreignCatalog, 
													const char * foreignSchema, 
													const char * foreignTable)
{
	char sql[4096] =
		"select cast ('' as varchar(7)) as pktable_cat,\n"	// 1
				" cast (ptbl.rdb$owner_name as varchar(31)) as pktable_schem,\n"	// 2
				" cast (pidx.rdb$relation_name as varchar(31)) as pktable_name,\n"	// 3
				" cast (pseg.rdb$field_name as varchar(31)) as pkcolumn_name,\n"	// 4
				" cast ('' as varchar(7)) as fktable_cat,\n"	// 5
				" cast (ftbl.rdb$owner_name as varchar(31)) as fktable_schem,\n"// 6
				" cast (fidx.rdb$relation_name as varchar(31)) as fktable_name,\n"	// 7
				" cast (fseg.rdb$field_name as varchar(31)) as fkcolumn_name,\n"	// 8
				" cast (pseg.rdb$field_position+1 as smallint) as key_seq,\n"		// 9
				" cast (0 as smallint) as update_rule,\n"		// 10
				" cast (0 as smallint) as delete_rule,\n"		// 11
				" cast (fkey.rdb$constraint_name as varchar(31)) as fk_name,\n"		// 12
				" cast (refc.rdb$const_name_uq as varchar(31)) as pk_name,\n"		// 13
				" 7 as deferrability,\n"						// 14	SQL_NOT_DEFERRABLE
				" refc.rdb$update_rule,\n"						// 15
				" refc.rdb$delete_rule\n"						// 16

		"from rdb$relation_constraints fkey,\n"
		"     rdb$relations ftbl,\n"
		"     rdb$relations ptbl,\n"
		"     rdb$indices fidx,\n"
		"     rdb$indices pidx,\n"
		"     rdb$index_segments fseg,\n"
		"     rdb$index_segments pseg,\n"
		"     rdb$ref_constraints refc\n"
		"where fkey.rdb$constraint_type = 'FOREIGN KEY'\n"
		"     and fidx.rdb$relation_name = ftbl.rdb$relation_name\n"
		"     and pidx.rdb$relation_name = ptbl.rdb$relation_name\n";

	char * ptFirst = sql + strlen(sql);

	addString(ptFirst, "  and fkey.rdb$index_name = fidx.rdb$index_name\n"
		"  and fidx.rdb$foreign_key = pidx.rdb$index_name\n"
		"  and fidx.rdb$index_name = fseg.rdb$index_name\n"
		"  and pidx.rdb$index_name = pseg.rdb$index_name\n"
		"  and pseg.rdb$field_position = fseg.rdb$field_position"
		"  and refc.rdb$constraint_name = fkey.rdb$constraint_name" );

	if (primarySchema && *primarySchema)
		expandPattern (ptFirst, " and ","ptbl.rdb$owner_name", primarySchema);

	if (primaryTable && *primaryTable)
		expandPattern (ptFirst, " and ","pidx.rdb$relation_name", primaryTable);

	if (foreignSchema && *foreignSchema)
		expandPattern (ptFirst, " and ","ftbl.rdb$owner_name", foreignSchema);

	if (foreignTable && *foreignTable)
		expandPattern (ptFirst, " and ","fkey.rdb$relation_name", foreignTable);

	addString(ptFirst, " order by pidx.rdb$relation_name, pseg.rdb$field_position\n");
	prepareStatement (sql);
	numberColumns = 14;
}

bool IscCrossReferenceResultSet::nextFetch()
{
	if (!IscResultSet::nextFetch())
		return false;

	if ( !metaData->getUseSchemaIdentifier() )
		sqlda->setNull(2);

	if ( !metaData->getUseSchemaIdentifier() )
		sqlda->setNull(6);

	int len;
	sqlda->updateShort ( 10, getRule ( sqlda->getText(15, len)) );
	sqlda->updateShort ( 11, getRule ( sqlda->getText(16, len)) );

	return true;
}

int IscCrossReferenceResultSet::getRule(const char * rule)
{
	if (stringEqual (rule, "CASCADE"))
		return SQL_CASCADE;

	if (stringEqual (rule, "RESTRICT"))
		return SQL_RESTRICT;
	
	if (stringEqual (rule, "SET NULL"))
		return SQL_SET_NULL;
	
	if (stringEqual (rule, "SET DEFAULT"))
		return SQL_SET_DEFAULT;

	return SQL_NO_ACTION;
}

bool IscCrossReferenceResultSet::stringEqual(const char * p1, const char * p2)
{
	while (*p1 && *p2)
		if (*p1++ != *p2++)
			return false;

	if (*p1 && *p1++ != ' ')
		return false;

	if (*p2 && *p2++ != ' ')
		return false;

	return true;
}

}; // end namespace IscDbcLibrary
