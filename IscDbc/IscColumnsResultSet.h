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

// IscColumnsMetaData.h: interface for the IscColumnsMetaData class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ISCCOLUMNSMETADATA_H_)
#define _ISCCOLUMNSMETADATA_H_

#include "IscMetaDataResultSet.h"
#include "IscBlob.h"
#include "IscArray.h"
#include "IscSqlType.h"

namespace IscDbcLibrary {

class IscColumnsResultSet : public IscMetaDataResultSet  
{
public:
	virtual bool nextFetch();
	void getColumns(const char * catalog, const char * schemaPattern, const char * tableNamePattern, const char * fieldNamePattern);
	IscColumnsResultSet(IscDatabaseMetaData *metaData);
	void initResultSet(IscStatement *stmt);
private:
	virtual bool getDefSource (int indexIn, int indexTarget);
	virtual void setCharLen (int charLenInd, int fldLenInd, IscSqlType &sqlType);
	virtual void checkQuotes (IscSqlType &sqlType, JString stringVal);
	virtual void adjustResults (IscSqlType &sqlType);	

	IscBlob blob;
	CAttrArray arrAttr;
	IscSqlType sqlType;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_ISCCOLUMNSMETADATA_H_)
