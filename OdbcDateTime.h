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

#if !defined(_ODBCDATETIME_H_)
#define _ODBCDATETIME_H_

#include <time.h>
#include "IscDbc/Connection.h"
#include "IscDbc/TimeStamp.h"
#include "IscDbc/SqlTime.h"
#include "IscDbc/DateTime.h"

struct tagTIMESTAMP_STRUCT;
struct tagDATE_STRUCT;

namespace OdbcJdbcLibrary {

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
	signed int ndate (signed int nday,signed int nsec, tm *times);
	signed int nday (tm *times);
	signed int yday (tm *times);

};

}; // end namespace OdbcJdbcLibrary

#endif // !defined(_ODBCDATETIME_H_)
