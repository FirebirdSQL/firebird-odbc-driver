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


// DescRecord.h: interface for the DescRecord class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DESCRECORD_H__F3F1D3A4_4083_11D4_98E8_0000C01D2301__INCLUDED_)
#define AFX_DESCRECORD_H__F3F1D3A4_4083_11D4_98E8_0000C01D2301__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class DescRecord  
{
public:
	DescRecord();
	virtual ~DescRecord();

	JString		name;
	int			type;
	int			subType;
	int			length;
	int			precision;
	int			scale;
	bool		nullable;
	int			bufferLength;
	void		*dataPtr;
	SQLINTEGER	*octetLengthPtr;
	SQLINTEGER	*indicatorPtr;
};

#endif // !defined(AFX_DESCRECORD_H__F3F1D3A4_4083_11D4_98E8_0000C01D2301__INCLUDED_)
