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

// IscColumnPrivilegesResultSet.h: interface for the IscColumnPrivilegesResultSet class.
//
/////////////////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_IscColumnPrivilegesResultSet_H__6C3E2AB7_229F_11D4_98DF_0000C01D2301__INCLUDED_)
#define AFX_IscColumnPrivilegesResultSet_H__6C3E2AB7_229F_11D4_98DF_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "IscMetaDataResultSet.h"

namespace IscDbcLibrary {

class IscColumnPrivilegesResultSet : public IscMetaDataResultSet  
{
public:
	virtual bool next();
	void getColumnPrivileges(const char * catalog, const char * schemaPattern, const char * tableNamePattern, const char * columnNamePattern);
	IscColumnPrivilegesResultSet(IscDatabaseMetaData *metaData);

	typedef IscMetaDataResultSet Parent;
	virtual int getColumnDisplaySize(int index);
	virtual int getColumnType(int index, int &realSqlType);
	virtual int getColumnPrecision(int index);

};

}; // end namespace IscDbcLibrary

#endif // !defined(AFX_ISCCOLUMNPRIVILEGESRESULTSET_H__6C3E2AB7_229F_11D4_98DF_0000C01D2301__INCLUDED_)
