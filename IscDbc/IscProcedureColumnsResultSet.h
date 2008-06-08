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

// IscProcedureColumnsResultSet.h: interface for the IscProcedureColumnsResultSet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ISCPROCEDURECOLUMNSRESULTSET_H_)
#define _ISCPROCEDURECOLUMNSRESULTSET_H_

#include "IscMetaDataResultSet.h"
#include "IscSqlType.h"

namespace IscDbcLibrary {

class IscProcedureColumnsResultSet : public IscMetaDataResultSet  
{
public:
	virtual bool nextFetch();
	void getProcedureColumns (const char *catalog, 
							  const char *schemaPattern, 
							  const char *procedureNamePattern, 
							  const char *columnNamePattern);
	IscProcedureColumnsResultSet(IscDatabaseMetaData *metaData);

	void adjustResults (IscSqlType &sqlType);

private:
	IscSqlType sqlType;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_ISCPROCEDURECOLUMNSRESULTSET_H_)
