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
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 *
 *	Changes
 *	
 *	2002-05-20	TimeStamp.cpp
 *				Contributed by Bernhard Schulte
 *				o Bring operator() up-to-date with other timestamp changes.
 *				o ditto decodeTime().
 *
 *
 */

// Timestamp.cpp: implementation of the Timestamp class.
//
//////////////////////////////////////////////////////////////////////

#ifdef __BORLANDC__
#include <iostream.h>
#else
#include <stdio.h>
#endif
#include <time.h>
#include <string.h>
#include "IscDbc.h"
#include "TimeStamp.h"

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TimeStamp TimeStamp::convert(const char * string, int length)
{
	TimeStamp timestamp;
	timestamp.date = 0;
	timestamp.nanos = 0;
	return timestamp;
}

int TimeStamp::getTimeString(int length, char * buffer)
{
	struct tm tmTemp;
	struct tm *time = &tmTemp;
	memset (time, 0, sizeof (tmTemp));

	DateTime::decodeDate (date, time);
	decodeTime (nanos, time);

	int len = (int)strftime (buffer, length, "%Y-%m-%d %H:%M:%S", time);
	int nano = (nanos % ISC_TIME_SECONDS_PRECISION) * STD_TIME_SECONDS_PRECISION;
	if( nano )
		len+=sprintf(buffer+len,".%lu",nano);

	return len;
}

int TimeStamp::decodeTime (int nanos, tm * times)
{
	int minutes = nanos / (ISC_TIME_SECONDS_PRECISION * 60);

	times->tm_hour = minutes / 60;
	times->tm_min = minutes % 60;
	times->tm_sec = (nanos / ISC_TIME_SECONDS_PRECISION) % 60;

	return true;
}

}; // end namespace IscDbcLibrary
