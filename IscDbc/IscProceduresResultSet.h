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

// IscProceduresResultSet.h: interface for the IscProceduresResultSet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISCPROCEDURESRESULTSET_H__32C6E497_2C14_11D4_98E0_0000C01D2301__INCLUDED_)
#define AFX_ISCPROCEDURESRESULTSET_H__32C6E497_2C14_11D4_98E0_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "IscMetaDataResultSet.h"

namespace IscDbcLibrary {

class IscProceduresResultSet : public IscMetaDataResultSet  
{
public:
	virtual bool next();
	void getProcedures(const char * catalog, const char * schemaPattern, const char * procedureNamePattern);
	IscProceduresResultSet(IscDatabaseMetaData *metaData);

};

}; // end namespace IscDbcLibrary

#endif // !defined(AFX_ISCPROCEDURESRESULTSET_H__32C6E497_2C14_11D4_98E0_0000C01D2301__INCLUDED_)
