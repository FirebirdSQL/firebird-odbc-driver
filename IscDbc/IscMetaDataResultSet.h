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

// IscMetaDataResultSet.h: interface for the IscMetaDataResultSet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ISCMETADATARESULTSET_H_)
#define _ISCMETADATARESULTSET_H_

#include "IscResultSet.h"
#include "JString.h"	// Added by ClassView

namespace IscDbcLibrary {

class IscDatabaseMetaData;

class IscMetaDataResultSet : public IscResultSet  
{
public:
	void expandPattern(char *& stringOut, const char *prefix, const char * string, const char * pattern);
	void addString(char *& stringOut, const char * string, int length = 0);
	void convertBlobToString( int indSrc, int indDst );
	bool isWildcarded (const char *pattern);
	virtual void prepareStatement (const char *sql);
	virtual bool next();

	IscMetaDataResultSet(IscDatabaseMetaData *meta);

	IscDatabaseMetaData		*metaData;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_ISCMETADATARESULTSET_H_)
