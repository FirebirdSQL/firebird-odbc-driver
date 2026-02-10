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

// IscResultSetMetaData.h: interface for the IscResultSetMetaData class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ISCRESULTSETMETADATA_H_)
#define _ISCRESULTSETMETADATA_H_

#include "Connection.h"

namespace IscDbcLibrary {

class IscResultSet;
class Value;

class IscResultSetMetaData : public ResultSetMetaData  
{
public:
	virtual bool isCaseSensitive (int index);
	virtual const char* getCatalogName (int index);
	virtual const char* getSchemaName (int index);
	virtual bool isSearchable(int index);
	virtual bool isAutoIncrement  (int index);
	virtual bool isCurrency (int index);
	virtual bool isDefinitelyWritable (int index);
	virtual bool isReadOnly (int index);
	virtual bool isWritable (int index);
	virtual bool isSigned (int index);
	virtual const char* getColumnLabel (int index);
	virtual int objectVersion();
	virtual bool isNullable (int index);
	virtual int getScale (int index);
	virtual int getPrecision (int index);
	virtual int getNumPrecRadix (int index);
	IscResultSetMetaData(IscResultSet *results, int numberColumns);
	virtual ~IscResultSetMetaData();
	virtual const char* getTableName (int index);
	virtual const char* getColumnName (int index);
	virtual int getColumnDisplaySize (int index);
	virtual int getColumnType (int index);
	virtual const char* getColumnTypeName (int index); 
	virtual int getColumnCount();

	IscResultSet	*resultSet;
	int				numberColumns;
	char			*query;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_ISCRESULTSETMETADATA_H_)
