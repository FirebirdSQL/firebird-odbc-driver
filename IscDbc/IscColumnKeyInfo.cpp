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
 *  The Original Code was created by Vladimir Tsvigun for IBPhoenix.
 *
 *  Copyright (c) 2006 Vladimir Tsvigun
 *  All Rights Reserved.
 *
 */

// IscColumnKeyInfo.cpp: implementation of the IscColumnKeyInfo class.
//
//////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#ifdef _WINDOWS
#include <windows.h>
#endif
#endif

#include <stdio.h>
#include "IscDbc.h"
#include "IscColumnKeyInfo.h"
#include "IscDatabaseMetaData.h"

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscColumnKeyInfo::IscColumnKeyInfo( IscDatabaseMetaData *metaData )
		: IscMetaDataResultSet( metaData )
{

}

bool IscColumnKeyInfo::getColumnKeyInfo( const char *tableName, const char *columnName )
{
	char sqlQuery[1024];

	sprintf( sqlQuery, "select cast(idx.rdb$unique_flag as smallint) as unique_column\n"
			           "  from rdb$indices idx\n"
			           "  join rdb$index_segments seg on idx.rdb$index_name = seg.rdb$index_name\n"
					   "                and seg.rdb$field_name = '%s'\n"
					   "  join rdb$relation_constraints relc on ( relc.rdb$index_name = idx.rdb$index_name\n"
					   "                and relc.rdb$relation_name = idx.rdb$relation_name\n"
					   "                and relc.rdb$constraint_type = 'PRIMARY KEY'\n"
			           "                and idx.rdb$unique_flag = 1 and idx.rdb$relation_name = '%s' )\n",
					columnName, tableName );

	prepareStatement( sqlQuery );

	if ( !sqlda->getCountRowsStaticCursor() )
		return false;

	return !!sqlda->getShort( 1 );
}

}; // end namespace IscDbcLibrary
