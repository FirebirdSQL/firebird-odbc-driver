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
 *
 */

// TypesResultSet.h: interface for the TypesResultSet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TYPESRESULTSET_H__F0866333_9646_11D4_98F5_0000C01D2301__INCLUDED_)
#define AFX_TYPESRESULTSET_H__F0866333_9646_11D4_98F5_0000C01D2301__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IscResultSet.h"

namespace IscDbcLibrary {

class TypesResultSet : public IscResultSet  
{
public:
	virtual bool isNullable(int index);
	virtual int getPrecision(int index);
	virtual int getScale(int index);
	int getColumnDisplaySize(int index);
	virtual int getColumnType(int index, int &realSqlType);
	virtual const char* getSqlTypeName(int index);
	virtual const char* getColumnTypeName(int index);
	virtual const char* getColumnName(int index);
	virtual const char* getColumnLabel(int index);
	virtual const char* getTableName(int index);
	virtual bool next();
	TypesResultSet(int dataType);
	~TypesResultSet();
	int findType();

	int			recordNumber;
	int			dataTypes;
};

}; // end namespace IscDbcLibrary

#endif // !defined(AFX_TYPESRESULTSET_H__F0866333_9646_11D4_98F5_0000C01D2301__INCLUDED_)
