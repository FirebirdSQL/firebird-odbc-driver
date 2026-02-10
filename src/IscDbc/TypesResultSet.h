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
 *
 */

// TypesResultSet.h: interface for the TypesResultSet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_TYPESRESULTSET_H_)
#define _TYPESRESULTSET_H_

#include "IscResultSet.h"
#include "Sqlda.h"
#include <vector>

namespace IscDbcLibrary {

struct Types;  // forward declaration

class TypesResultSet : public IscResultSet
{
public: // StatementMetaData
	virtual int objectVersion(){ return STATEMENTMETADATA_VERSION; }

public:
	TypesResultSet( int dataType, int appOdbcVersion, int bytesPerCharacter, IscConnection* conn );
	~TypesResultSet();

	virtual bool nextFetch();
	virtual bool next();

	int			recordNumber;
	int			dataTypes;
	int			serverMajorVersion;
	Sqlda		outputSqlda;
	std::vector<SQLLEN> indicators;
	std::vector<Types> typesInstance;  // per-instance copy of the type info (thread-safe)
};

}; // end namespace IscDbcLibrary

#endif // !defined(_TYPESRESULTSET_H_)
