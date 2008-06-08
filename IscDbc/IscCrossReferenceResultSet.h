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

// IscCrossReferenceResultSet.h: interface for the IscCrossReferenceResultSet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ISCCROSSREFERENCERESULTSET_H_)
#define _ISCCROSSREFERENCERESULTSET_H_

#include "IscMetaDataResultSet.h"

namespace IscDbcLibrary {

class IscCrossReferenceResultSet : public IscMetaDataResultSet  
{
public:
	bool stringEqual (const char *p1, const char *p2);
	int getRule (const char *rule);
	virtual bool nextFetch();
	void getCrossReference(const char* primaryCatalog, const char* primarySchema, const char* primaryTable,const char* foreignCatalog, const char* foreignSchema, const char* foreignTable);
	IscCrossReferenceResultSet(IscDatabaseMetaData *metaData);
};

}; // end namespace IscDbcLibrary

#endif // !defined(_ISCCROSSREFERENCERESULTSET_H_)
