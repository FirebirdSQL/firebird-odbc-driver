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
	odtApplicationParameter,
	odtImplementationParameter,
	odtApplicationRow,
	odtImplementationRow
	};

class OdbcConnection;
class DescRecord;

class OdbcDesc : public OdbcObject  
{
public:
	DescRecord* getDescRecord (int number);
	RETCODE sqlSetDescField (int recNumber, int fieldId, SQLPOINTER value, int length);
	virtual OdbcObjectType getType();
	OdbcDesc(OdbcDescType type, OdbcConnection *connect);
	virtual ~OdbcDesc();

	OdbcConnection	*connection;
	OdbcDescType	descType;
	int				recordSlots;
	DescRecord		**records;
	int				descArraySize;
	SQLUINTEGER		*rowsProcessedPtr;	
};

#endif // !defined(AFX_ODBCDESC_H__73DA784A_3271_11D4_98E1_0000C01D2301__INCLUDED_)
