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

// IscPrimaryKeysResultSet.h: interface for the IscPrimaryKeysResultSet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ISCPRIMARYKEYSRESULTSET_H_)
#define _ISCPRIMARYKEYSRESULTSET_H_

#include "IscMetaDataResultSet.h"

namespace IscDbcLibrary {

class IscPrimaryKeysResultSet : public IscMetaDataResultSet  
{
public:
	virtual bool nextFetch();
	void getPrimaryKeys (const char * catalog, const char * schemaPattern, const char * tableNamePattern);
	IscPrimaryKeysResultSet(IscDatabaseMetaData *metaData);
};

}; // end namespace IscDbcLibrary

#endif // !defined(_ISCPRIMARYKEYSRESULTSET_H_)
