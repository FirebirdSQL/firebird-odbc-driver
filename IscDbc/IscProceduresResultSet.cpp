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
#include <string.h>
#include <stdlib.h>
#include "IscDbc.h"
#include "IscBlob.h"
#include "IscStatement.h"
#include "IscProceduresResultSet.h"
#include "IscDatabaseMetaData.h"

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscProceduresResultSet::IscProceduresResultSet(IscDatabaseMetaData *metaData)
		: IscMetaDataResultSet(metaData)
{
	addBlr = false;
}

void IscProceduresResultSet::getProcedures(const char * catalog, const char * schemaPattern, const char * procedureNamePattern)
{
	char sql[2048] =
		"select cast (NULL as varchar(7)) as procedure_cat,\n"				// 1
				"\tcast (NULL as varchar(7)) as procedure_schem,\n"			// 2
				"\tcast (proc.rdb$procedure_name as varchar(31)) as procedure_name,\n"	// 3
				"\tproc.rdb$procedure_inputs as num_input_params,\n"		// 4
				"\tproc.rdb$procedure_outputs as num_output_params,\n"		// 5
				"\t1 as num_result_sets,\n"									// 6
				"\tproc.rdb$description as remarks,\n"						// 7
				"\t1 as procedure_type\n";									// 8 SQL_PT_PROCEDURE

	char * ptFirst = sql + strlen(sql);
	const char *sep = " where ";

	if ( addBlr )
		addString(ptFirst, ", proc.rdb$procedure_blr\n"); // 9 BLR_PROCEDURE

	addString(ptFirst, "from rdb$procedures proc\n");

	if (procedureNamePattern && *procedureNamePattern)
	{
		expandPattern (ptFirst, sep,"proc.rdb$procedure_name", procedureNamePattern);
		sep = " and ";
	}

	if ( !metaData->allTablesAreSelectable() )
		metaData->existsAccess(ptFirst, sep, "proc", 5, "");

	addString(ptFirst, " order by proc.rdb$procedure_name");
	prepareStatement (sql);
	numberColumns = 8;
}

bool IscProceduresResultSet::next()
{
	if (!IscResultSet::next())
		return false;

	if ( sqlda->isNull(4) )
		sqlda->updateShort(4, 0);
	if ( sqlda->isNull(5) )
		sqlda->updateShort(5, 0);

	return true;
}

static int gen_blr(int *user_arg, int /*offset*/, char * string)
{
	if ( strstr(string,"blr_send") )
		(*user_arg)++;
	return 1;
}

bool IscProceduresResultSet::canSelectFromProcedure()
{
	int countSUSPEND = 0;
	XSQLVAR *var = sqlda->Var(9);
	IscBlob * blob = (IscBlob *)*(long*)var->sqldata;
	int length = blob->length();
	
	char * buffer = (char*)malloc (length);

	blob->getBytes (0, length, buffer);
	metaData->connection->GDS->_print_blr((char*)buffer,(void (*)())gen_blr, &countSUSPEND,0);

	free(buffer);

	return countSUSPEND > 1;
}

}; // end namespace IscDbcLibrary
