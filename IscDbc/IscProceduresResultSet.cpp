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

// IscProceduresResultSet.cpp: implementation of the IscProceduresResultSet class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "IscDbc.h"
#include "IscProceduresResultSet.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscProceduresResultSet::IscProceduresResultSet(IscDatabaseMetaData *metaData)
		: IscMetaDataResultSet(metaData)
{

}

IscProceduresResultSet::~IscProceduresResultSet()
{

}

void IscProceduresResultSet::getProcedures(const char * catalog, const char * schemaPattern, const char * procedureNamePattern)
{
	JString sql = 
		"select NULL as procedure_cat,\n"							// 1
				"\tNULL as procedure_schem,\n"						// 2
				"\trdb$procedure_name as procedure_name,\n"			// 3
				"\trdb$procedure_inputs as num_input_params,\n"		// 4
				"\trdb$procedure_outputs as num_output_params,\n"	// 5
				"\t0 as num_result_sets,\n"							// 6
				"\trdb$description as remarks,\n"					// 7
				"\t0 as procedure_type\n"							// 8 SQL_PT_UNKNOWN
		"from rdb$procedures\n";

	if (procedureNamePattern && *procedureNamePattern)
		sql += expandPattern (" where ","rdb$procedure_name", procedureNamePattern);

	sql += " order by rdb$procedure_name";
	prepareStatement (sql);
	numberColumns = 8;
}

bool IscProceduresResultSet::next()
{
	if (!resultSet->next())
		return false;

	trimBlanks (3);							// table name

	return true;
}
