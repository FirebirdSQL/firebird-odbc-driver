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

// IscStatementMetaData.h: interface for the IscStatementMetaData class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ISCSTATEMENTMETADATA_H_)
#define _ISCSTATEMENTMETADATA_H_

#include "Connection.h"

class Sqlda;

class IscStatementMetaData : public StatementMetaData  
{
public:
	virtual int objectVersion();
	virtual bool isNullable (int index);
	virtual int getScale (int index);
	virtual int getPrecision (int index);
	virtual int getColumnType (int index, int &realSqlType);
	virtual int getColumnCount();
	virtual int getColumnDisplaySize(int index);
	virtual const char* getColumnLabel(int index);
	virtual const char* getSqlTypeName(int index);
	virtual const char* getColumnName(int index);
	virtual const char* getTableName(int index);
	virtual const char* getColumnTypeName(int index);
	virtual bool isSigned (int index);
	virtual bool isReadOnly (int index);
	virtual bool isWritable (int index);
	virtual bool isDefinitelyWritable (int index);
	virtual bool isCurrency (int index);
	virtual bool isCaseSensitive (int index);
	virtual bool isAutoIncrement (int index);
	virtual bool isSearchable (int index);
	virtual int	 isBlobOrArray(int index);
	virtual const char*	getSchemaName (int index);
	virtual const char*	getCatalogName (int index);
	virtual void getSqlData(int index, char *& ptData, short *& ptIndData, Blob *& ptDataBlob);
	virtual void setSqlData(int index, long ptData, long ptIndData);
	virtual void saveSqlData(int index);
	virtual void restoreSqlData(int index);

	IscStatementMetaData(Sqlda	* ptSqlda);
	virtual ~IscStatementMetaData();

	Sqlda					*sqlda;
};

#endif // !defined(_ISCSTATEMENTMETADATA_H_)
