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

// IscSqlType.h: interface for the IscSqlType class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISCSQLTYPE_H__32C6E499_2C14_11D4_98E0_0000C01D2301__INCLUDED_)
#define AFX_ISCSQLTYPE_H__32C6E499_2C14_11D4_98E0_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

namespace IscDbcLibrary {

class IscSqlType  
{
public:
	void getType (int blrType, int subType, int length, int bufferLen, int dialect, int precision, int scale);
	IscSqlType(int blrType, int subType, int length, int bufferLen, int dialect, int precision, int scale);
	~IscSqlType();

	int		type;
	char	*typeName;
	int		length;
	int		bufferLength;
	int		precision;
};

}; // end namespace IscDbcLibrary

#endif // !defined(AFX_ISCSQLTYPE_H__32C6E499_2C14_11D4_98E0_0000C01D2301__INCLUDED_)
