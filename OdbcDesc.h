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

#if !defined(AFX_ODBCDESC_H__73DA784A_3271_11D4_98E1_0000C01D2301__INCLUDED_)
#define AFX_ODBCDESC_H__73DA784A_3271_11D4_98E1_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "OdbcObject.h"

enum OdbcDescType {
	odtApplication,
	odtApplicationParameter,
	odtImplementationParameter,
	odtApplicationRow,
	odtImplementationRow
	};

class OdbcConnection;
class StatementMetaData;
class DescRecord;
class OdbcConvert;

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
	void setBindOffsetPtr(SQLINTEGER	**ptBindOffsetPtr);

	DescRecord* getDescRecord (int number);
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
	~OdbcDesc();

	void removeRecords();
	void setDefaultImplDesc (StatementMetaData * ptMetaData);
	void allocBookmarkField();
	RETCODE operator =(OdbcDesc &sour);
	int setConvFn(int recNumber, DescRecord * recordTo);
	int getConciseType(int type);
	int getConciseSize(int type, int length);
	int getDefaultFromSQLToConciseType(int sqlType);
	void addBindColumn(int recNumber, DescRecord * recordApp);
	void delBindColumn(int recNumber);
	void delAllBindColumn();
	void returnData();

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
	StatementMetaData	*metaData;
	OdbcDescType		headType;
	int					recordSlots;
	DescRecord			**records;

	bool				bDefined;
	OdbcConvert			*convert;
	ListBindColumn		*listBind;
};

#endif // !defined(AFX_ODBCDESC_H__73DA784A_3271_11D4_98E1_0000C01D2301__INCLUDED_)
