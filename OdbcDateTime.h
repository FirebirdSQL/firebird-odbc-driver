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
 *	See OdbcDateTime.cpp for notes re changes
 *
 */

/*
 *	PROGRAM:		OdbcJdbc
 *	MODULE:			OdbcDateTime.cpp
 *	DESCRIPTION:	High level date conversion routines
 *
 * copyright (c) 2000 by Ann W. Harrison
 */
// OdbcDateTime.h: interface for the OdbcDateTime class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ODBCDATETIME_H__9DD752F8_BEC1_41C9_B27B_1AA040E944E7__INCLUDED_)
#define AFX_ODBCDATETIME_H__9DD752F8_BEC1_41C9_B27B_1AA040E944E7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <time.h>
#include "IscDbc/Connection.h"
#include "IscDbc/TimeStamp.h"
#include "IscDbc/SqlTime.h"
#include "IscDbc/DateTime.h"

struct tagTIMESTAMP_STRUCT;
struct tagDATE_STRUCT;

class OdbcDateTime  
{
public:
	OdbcDateTime();
	~OdbcDateTime();
	int	convert (tagDATE_STRUCT * tagDateIn, DateTime * dateTimeOut);
	int	convert (tagTIMESTAMP_STRUCT * tagTimeStampIn, DateTime * dateTimeOut);
	int convert (tagTIMESTAMP_STRUCT * tagTimeStampIn, TimeStamp * timeStampOut);
	int convert (DateTime * dateTimeIn, tagDATE_STRUCT * tagDateOut);
	int convert (TimeStamp *timeStampIn, tagTIMESTAMP_STRUCT * tagTimeStampOut);

private:
//Orig.
//	static signed long OdbcDateTime::ndate (signed long nday, tm *times);
//From B. Schulte
	signed long ndate (signed long nday,signed long nsec, tm *times);
	signed long nday (tm *times);
	signed long yday (tm *times);

};

#endif // !defined(AFX_ODBCDATETIME_H__9DD752F8_BEC1_41C9_B27B_1AA040E944E7__INCLUDED_)
