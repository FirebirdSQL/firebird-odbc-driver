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
 *  The Original Code was created by James A. Starkey
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 */

// Timestamp.h: interface for the Timestamp class.
//
//////////////////////////////////////////////////////////////////////


#if !defined(_TIMESTAMP_H_INCLUDED_)
#define _TIMESTAMP_H_INCLUDED_

#include "SqlTime.h"
#include "DateTime.h"

namespace IscDbcLibrary {

class TimeStamp : public DateTime
{
public:
	static TimeStamp convert (const char *string, int length);
	int getTimeString(int length, char * buffer);
	int decodeTime (int nanos, struct tm * times);

	int	nanos;					// nano seconds
};

}; // end namespace IscDbcLibrary

#endif // !defined(_TIMESTAMP_H_INCLUDED_)
