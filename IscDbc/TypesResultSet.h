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

#include <vector>
#include "IscResultSet.h"
#include "Sqlda.h"

namespace IscDbcLibrary {

class TypesResultSet : public IscResultSet
{
public: // StatementMetaData
	virtual int objectVersion(){ return STATEMENTMETADATA_VERSION; }

public:
	TypesResultSet( int dataType, int appOdbcVersion, int bytesPerCharacter, IscConnection* conn );
	~TypesResultSet();

	virtual bool nextFetch();
	virtual bool next();
	int findType();

	int			recordNumber;
	int			dataTypes;
	Sqlda		outputSqlda;
	//SQLLEN		*indicators;

	Firebird::ThrowStatusWrapper status;

	FB_MESSAGE( TypesOutputHelper, Firebird::ThrowStatusWrapper,
		( FB_VARCHAR(50), TYPE_NAME )
		( FB_SMALLINT,    DATA_TYPE )
		( FB_INTEGER,     COLUMN_SIZE )
		( FB_VARCHAR(6),  LITERAL_PREFIX )
		( FB_VARCHAR(6),  LITERAL_SUFFIX )
		( FB_VARCHAR(20), CREATE_PARAMS )
		( FB_SMALLINT,    NULLABLE )
		( FB_SMALLINT,    CASE_SENSITIVE )
		( FB_SMALLINT,    SEARCHABLE )
		( FB_SMALLINT,    UNSIGNED_ATTRIBUTE )
		( FB_SMALLINT,    FIXED_PREC_SCALE )
		( FB_SMALLINT,    AUTO_UNIQUE_VALUE )
		( FB_VARCHAR(50), LOCAL_TYPE_NAME )
		( FB_SMALLINT,    MINIMUM_SCALE )
		( FB_SMALLINT,    MAXIMUM_SCALE )
		( FB_SMALLINT,    SQL_DATA_TYPE )
		( FB_SMALLINT,    SQL_DATETIME_SUB )
		( FB_INTEGER,     NUM_PREC_RADIX )
		( FB_SMALLINT,    INTERVAL_PRECISION )
	) typesOutputHelper;

	std::vector< std::vector< char > > localBuffer;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_TYPESRESULTSET_H_)
