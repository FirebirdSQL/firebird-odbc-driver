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
 *  The Original Code was created by Ann W. Harrison for IBPhoenix.
 *
 *  Copyright (c) 2001 Ann W. Harrison
 *  All Rights Reserved.
 */

// IscSpecialColumnsResultSet.h: interface for the IscSpecialColumnsResultSet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ISCSPECIALCOLUMNSRESULTSET_H_)
#define _ISCSPECIALCOLUMNSRESULTSET_H_

#include "IscMetaDataResultSet.h"
#include "IscSqlType.h"

namespace IscDbcLibrary {

class IscSpecialColumnsResultSet : public IscMetaDataResultSet  
{
public:
	IscSpecialColumnsResultSet(IscDatabaseMetaData *metaData);
	virtual void specialColumns (const char * catalog, const char * schema, const char * table, int scope, int nullable);
	virtual bool nextFetch();

private:
	virtual void setCharLen (int charLenInd, int fldLenInd, IscSqlType &sqlType);
	virtual void adjustResults (IscSqlType &sqlType);
	int	index_id;	
	IscSqlType sqlType;
};

}; // end namespace IscDbcLibrary

#endif
