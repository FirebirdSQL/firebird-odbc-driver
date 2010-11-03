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

// OdbcDesc.h: interface for the OdbcDesc class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ODBCDESC_H_)
#define _ODBCDESC_H_

#include <string.h>
#include "IscDbc/Connection.h"
#include "OdbcObject.h"
#include "IscDbc/Mlist.h"
#include "DescRecord.h"

namespace OdbcJdbcLibrary {

enum OdbcDescType {
	odtApplication,
	odtApplicationParameter,
	odtImplementationParameter,
	odtApplicationRow,
	odtImplementationRow,
	odtImplementationGetData
	};

class OdbcConnection;
class DescRecord;
class OdbcStatement;

class CBindColumn
{
public:
	int column;
	DescRecord * impRecord;
	DescRecord * appRecord;
	CBindColumn(int col,DescRecord * imp,DescRecord * app)
	{
		column = col;
		impRecord = imp;
		appRecord = app;
	}
	void remove()
	{ 
		column = 0; 
		impRecord = NULL;
		appRecord = NULL;
	}
	CBindColumn & operator =(const CBindColumn & src)
	{ 
		column = src.column;
		impRecord = src.impRecord;
		appRecord = src.appRecord;
		return  *this;
	}
};

class CBindColumnComparator
{
public:
	static int compare(const CBindColumn *a, const CBindColumn *b) 
	{
	    return a->column - b->column;
	}
};
typedef MList<CBindColumn,CBindColumnComparator> ListBindColumn;

class OdbcDesc : public OdbcObject  
{
public:
	inline DescRecord*	getDescRecord(int number, bool bCashe = true);
	SQLRETURN sqlGetDescField(int recNumber, int fieldId, SQLPOINTER value, int length, SQLINTEGER *lengthPtr);
	SQLRETURN sqlSetDescField (int recNumber, int fieldId, SQLPOINTER value, int length);
	SQLRETURN sqlGetDescRec(SQLSMALLINT recNumber, SQLCHAR *Name, SQLSMALLINT BufferLength, SQLSMALLINT *StringLengthPtr, 
							SQLSMALLINT *TypePtr, SQLSMALLINT *SubTypePtr, SQLLEN *LengthPtr, SQLSMALLINT *PrecisionPtr, 
							SQLSMALLINT *ScalePtr, SQLSMALLINT *NullablePtr);
	SQLRETURN sqlSetDescRec( SQLSMALLINT recNumber, SQLSMALLINT type, SQLSMALLINT subType, SQLINTEGER length,
							SQLSMALLINT	precision, SQLSMALLINT scale, SQLPOINTER dataPtr, 
							SQLLEN *stringLengthPtr, SQLLEN *indicatorPtr);

	virtual OdbcConnection* getConnection();
	virtual OdbcObjectType getType();
	OdbcDesc(OdbcDescType type, OdbcConnection *connect);
	~OdbcDesc();

	bool isDefined() { return bDefined; }
	void setDefined( bool def ){ bDefined = def; }
	void updateDefinedIn();
	void updateDefinedOut();
	void clearDefined();
	void releasePrepared();
	void clearPrepared();
	void removeRecords();
	void setDefaultImplDesc (StatementMetaData * ptMetaDataOut, StatementMetaData * ptMetaDataIn = NULL);
	void allocBookmarkField();
	SQLRETURN operator =(OdbcDesc &sour);
	void defFromMetaDataIn(int recNumber, DescRecord * record);
	void defFromMetaDataOut(int recNumber, DescRecord * record);
	int getConciseType(int type);
	int getConciseSize(int type, int length);
	int getDefaultFromSQLToConciseType(int sqlType, int bufferLength = 0);

//Head
	SQLSMALLINT			headAllocType;
	SQLUINTEGER			headArraySize;
	SQLUSMALLINT		*headArrayStatusPtr;	
	SQLLEN				*headBindOffsetPtr;
	SQLINTEGER			headBindType; 
	SQLSMALLINT			headCount;
	SQLULEN				*headRowsProcessedPtr;
//

	OdbcConnection		*connection;
	StatementMetaData	*metaDataIn;
	StatementMetaData	*metaDataOut;
	OdbcDescType		headType;
	int					recordSlots;
	DescRecord			**records;

	bool				bDefined;
};

inline
DescRecord* OdbcDesc::getDescRecord(int number, bool bCashe)
{
	if (number >= recordSlots)
	{
		int oldSlots = recordSlots;
		DescRecord **oldRecords = records;
		recordSlots = number + (bCashe ? 20 : 1);
		records = new DescRecord* [recordSlots];
		memset (records, 0, sizeof (DescRecord*) * recordSlots);
		if (oldSlots)
		{
			memcpy (records, oldRecords, sizeof (DescRecord*) * oldSlots);
			delete [] oldRecords;
		}
	}

	if (number > headCount)
		headCount = number;

	DescRecord * &record = records[number];

	if (record == NULL)
	{
		record = new DescRecord;
		switch(headType)
		{
		case odtImplementationRow:
		case odtImplementationParameter:
			record->isIndicatorSqlDa = true;
		}
	}

	return record;		
}

}; // end namespace OdbcJdbcLibrary

#endif // !defined(_ODBCDESC_H_)
