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
 *  The Original Code was created by Ann W. Harrison for IBPhoenix.
 *
 *  Copyright (c) 2001 Ann W. Harrison
 *  All Rights Reserved.
 */

// IscSpecialColumnsResultSet.h: interface for the IscSpecialColumnsResultSet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISCSPECIALCOLUMNSRESULTSET_H__87400B75_EA1F_46D6_BBED_83CE5B36BCE3__INCLUDED_)
#define AFX_ISCSPECIALCOLUMNSRESULTSET_H__87400B75_EA1F_46D6_BBED_83CE5B36BCE3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IscMetaDataResultSet.h"
#include "IscSqlType.h"

namespace IscDbcLibrary {

class IscSpecialColumnsResultSet : public IscMetaDataResultSet  
{
public:
	IscSpecialColumnsResultSet(IscDatabaseMetaData *metaData);
	virtual void specialColumns (const char * catalog, const char * schema, const char * table, int scope, int nullable);
	bool next();
	typedef IscMetaDataResultSet Parent;
	virtual int getColumnType (int index, int &realSqlType);
	virtual int IscSpecialColumnsResultSet::getColumnDisplaySize(int index);
	virtual int IscSpecialColumnsResultSet::getPrecision(int index);

private:
	virtual void setCharLen (int charLenInd, int fldLenInd, IscSqlType sqlType);
	virtual void adjustResults (IscSqlType sqlType);
	int	index_id;	

};

}; // end namespace IscDbcLibrary

#endif
