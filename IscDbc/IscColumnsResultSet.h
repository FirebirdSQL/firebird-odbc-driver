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

// IscColumnsMetaData.h: interface for the IscColumnsMetaData class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISCCOLUMNSMETADATA_H__6C3E2ABA_229F_11D4_98DF_0000C01D2301__INCLUDED_)
#define AFX_ISCCOLUMNSMETADATA_H__6C3E2ABA_229F_11D4_98DF_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "IscMetaDataResultSet.h"
#include "IscSqlType.h"

class IscColumnsResultSet : public IscMetaDataResultSet  
{
public:
	virtual int getPrecision (int index);
	virtual int getColumnDisplaySize(int index);
	typedef IscMetaDataResultSet Parent;

	virtual int getColumnType (int index, int &realSqlType);
	virtual bool next();
	void getColumns(const char * catalog, const char * schemaPattern, const char * tableNamePattern, const char * fieldNamePattern);
	IscColumnsResultSet(IscDatabaseMetaData *metaData);
private:
	virtual bool getBLRLiteral (int indexIn, int indexTarget, IscSqlType sqlType);
	virtual void setCharLen (int charLenInd, int fldLenInd, IscSqlType sqlType);
	virtual void checkQuotes (IscSqlType sqlType, JString stringVal);
	virtual void adjustResults (IscSqlType sqlType);	
};

#endif // !defined(AFX_ISCCOLUMNSMETADATA_H__6C3E2ABA_229F_11D4_98DF_0000C01D2301__INCLUDED_)
