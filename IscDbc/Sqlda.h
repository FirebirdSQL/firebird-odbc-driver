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

// Sqlda.h: interface for the Sqlda class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SQLDA_H__6C3E2AB9_229F_11D4_98DF_0000C01D2301__INCLUDED_)
#define AFX_SQLDA_H__6C3E2AB9_229F_11D4_98DF_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define DEFAULT_SQLDA_COUNT		20

struct TempVector {
    char	*temp;
	int		length;
	};

class Value;
class IscConnection;
class CDataStaticCursor;

class Sqlda  
{
public:
	void deleteTemps();
	void* allocTemp (int index, int length);
	const char* getOwnerName (int index);
	int findColumn (const char *columnName);
	void setBlob (XSQLVAR *var, Value *value, IscConnection *connection);
	void setArray (XSQLVAR *var, Value *value, IscConnection *connection);
	void setValue (int slot, Value *value, IscConnection *connection);
	const char* getTableName (int index);
//	static const char* getSqlTypeName (int iscType, int subType);
//	static int getSqlType (int iscType, int subType);
	static int getSqlType (int iscType, int subType, int sqlScale, int &realSqlType);
	static const char* getSqlTypeName (int iscType, int subType, int sqlScale);
	bool isNullable (int index);
	int getScale (int index);
	int getPrecision (int index);
	const char* getColumnName (int index);
	int getColumnDisplaySize (int index);
	int getSubType(int index);
	int getColumnType (int index, int &realSqlType);
	const char * getColumnTypeName (int index);
	void getSqlData(int index, char *& ptData, short *& ptIndData);
	void setSqlData(int index, long ptData, long ptIndData);
	void saveSqlData(int index, long ptData, long ptIndData);
	void restoreSqlData(int index);
	void print();
	void initStaticCursor(IscConnection *connect);
	bool setCurrentRowInBufferStaticCursor(int nRow);
	void getAdressFieldFromCurrentRowInBufferStaticCursor(int column, char *& sqldata, short *& sqlind);
	void copyNextSqldaInBufferStaticCursor();
	void copyNextSqldaFromBufferStaticCursor();
	int getCountRowsStaticCursor();
	int getColumnCount();
	void allocBuffer();
	bool checkOverflow();
	void deleteSqlda();
	operator XSQLDA*();
	Sqlda();
	virtual ~Sqlda();

	CDataStaticCursor * dataStaticCursor;
	int			lengthBufferRows;
	int			*offsetSqldata;
	int			indicatorsOffset;
	long		*saveOrgAdressSqlData;
	long		*saveOrgAdressSqlInd;

	XSQLDA		*sqlda;
	char		tempSqlda [XSQLDA_LENGTH (DEFAULT_SQLDA_COUNT)];
	char		*buffer;
	TempVector	*temps;
	bool		needsbuffer;
};

#endif // !defined(AFX_SQLDA_H__6C3E2AB9_229F_11D4_98DF_0000C01D2301__INCLUDED_)
