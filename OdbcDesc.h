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

// OdbcDesc.h: interface for the OdbcDesc class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ODBCDESC_H_)
#define _ODBCDESC_H_

#include "OdbcObject.h"

enum OdbcDescType {
	odtApplication,
	odtApplicationParameter,
	odtImplementationParameter,
	odtApplicationRow,
	odtImplementationRow,
	odtImplementationGetData
	};

class OdbcConnection;
class StatementMetaData;
class DescRecord;
class OdbcConvert;
class OdbcStatement;

#include "Mlist.h"

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
	// Use to odtImplementationParameter and odtImplementationRow
	void setBindOffsetPtrTo(SQLINTEGER *bindOffsetPtr);
	void setBindOffsetPtrFrom(SQLINTEGER *bindOffsetPtr);

	DescRecord*	getDescRecord(int number, bool bCashe = true);
	RETCODE sqlGetDescField(int recNumber, int fieldId, SQLPOINTER value, int length, SQLINTEGER *lengthPtr);
	RETCODE sqlSetDescField (int recNumber, int fieldId, SQLPOINTER value, int length);
	RETCODE sqlGetDescRec(SQLSMALLINT recNumber, SQLCHAR *Name, SQLSMALLINT BufferLength, SQLSMALLINT *StringLengthPtr, 
							SQLSMALLINT *TypePtr, SQLSMALLINT *SubTypePtr, SQLINTEGER *LengthPtr, SQLSMALLINT *PrecisionPtr, 
							SQLSMALLINT *ScalePtr, SQLSMALLINT *NullablePtr);
	RETCODE sqlSetDescRec( SQLSMALLINT recNumber, SQLSMALLINT type, SQLSMALLINT subType, SQLINTEGER length,
							SQLSMALLINT	precision, SQLSMALLINT scale, SQLPOINTER dataPtr, 
							SQLINTEGER *stringLengthPtr, SQLINTEGER *indicatorPtr);

	virtual OdbcObjectType getType();
	OdbcDesc(OdbcDescType type, OdbcConnection *connect);
	virtual ~OdbcDesc();

	void updateDefined();
	void clearDefined();
	void removeRecords();
	void setDefaultImplDesc (StatementMetaData * ptMetaData);
	void allocBookmarkField();
	RETCODE operator =(OdbcDesc &sour);
	void defFromMetaData(int recNumber, DescRecord * record);
	int setConvFn(int recNumber, DescRecord * recordTo);
	int setConvFnForGetData(int recNumber, DescRecord * recordTo, DescRecord *& recordIRD);
	int getConciseType(int type);
	int getDefaultFromSQLToConciseType(int sqlType);
	void addBindColumn(int recNumber, DescRecord * recordApp);
	void delBindColumn(int recNumber);
	void addGetDataColumn(int recNumber, DescRecord * recordImp);
	void delAllBindColumn();
	void setParent(OdbcStatement *parent);

	RETCODE returnData();
	RETCODE returnGetData(int recNumber);

//Head
	SQLSMALLINT			headAllocType;
	SQLUINTEGER			headArraySize;
	SQLUSMALLINT		*headArrayStatusPtr;	
	SQLINTEGER			*headBindOffsetPtr;
	SQLINTEGER			headBindType; 
	SQLSMALLINT			headCount;
	SQLUINTEGER			*headRowsProcessedPtr;
//

	OdbcConnection		*connection;
	OdbcStatement		*parentStmt;
	StatementMetaData	*metaData;
	OdbcDescType		headType;
	int					recordSlots;
	DescRecord			**records;

	bool				bDefined;
	OdbcConvert			*convert;
	ListBindColumn		*listBind;
};

#endif // !defined(_ODBCDESC_H_)
