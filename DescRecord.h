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


// DescRecord.h: interface for the DescRecord class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_DESCRECORD_H_)
#define _DESCRECORD_H_

#include "OdbcConvert.h"

namespace OdbcJdbcLibrary {

using namespace IscDbcLibrary;

class DescRecord
{
public:
	DescRecord();
	~DescRecord();
	void setDefault(DescRecord *recTo);
	bool operator =(DescRecord *rec);
	void initZeroColumn();

public:
	bool			isDefined;
	bool			isPrepared;
	SQLSMALLINT		callType; // use sqlGetData

	int				isBlobOrArray;
	bool			data_at_exec;
	bool			startedTransfer;
	int				sizeColumnExtendedFetch;
	SQLINTEGER		dataOffset;
	long			currentFetched;
	Blob			*dataBlobPtr; // for blob or array 

	SQLSMALLINT		type;
	SQLSMALLINT		datetimeIntervalCode;
	SQLSMALLINT		conciseType;
	SQLINTEGER		autoUniqueValue;
	JString			baseColumnName;
	JString			baseTableName;
	SQLINTEGER		caseSensitive;
	JString			catalogName;
	SQLINTEGER		datetimeIntervalPrecision;
	SQLINTEGER		displaySize;
	SQLSMALLINT		fixedPrecScale;
	JString			label;
	SQLUINTEGER		length;
	JString			literalPrefix;
	JString			literalSuffix;
	JString			localTypeName;
	JString			name;
	SQLSMALLINT		nullable;
	SQLINTEGER		octetLength;
	SQLINTEGER		*octetLengthPtr;
	SQLSMALLINT		parameterType;
	SQLSMALLINT		precision;
	SQLSMALLINT		scale;
	JString			schemaName;
	SQLSMALLINT		searchable;
	JString			tableName;
	JString			typeName;
	SQLSMALLINT		unSigned;
	SQLSMALLINT		updaTable;
	SQLINTEGER		*indicatorPtr;
	SQLSMALLINT		unNamed;
	SQLPOINTER		dataPtr;

public:

	ADRESS_FUNCTION fnConv;
};

}; // end namespace OdbcJdbcLibrary

#endif // !defined(_DESCRECORD_H_)
