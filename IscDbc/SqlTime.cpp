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
 *  Copyright (c) Ann W. Harrison
 *  All Rights Reserved.
 */
// SqlTime.cpp: implementation of the SqlTime class.
//
//////////////////////////////////////////////////////////////////////

#include "IscDbc.h"
#include "SqlTime.h"
#include <time.h>
#include <string.h>

namespace IscDbcLibrary {

SqlTime SqlTime::convert(const char * string, int length)
{
	SqlTime time;
	time.timeValue = 0;
	return time;
}

int SqlTime::getString (int length, char * buffer)
{
	return getString ("%H:%M:%S", length, buffer);
}

int SqlTime::getString(const char * format, int length, char * buffer)
{
	struct tm tmTemp;
	struct tm *time = &tmTemp;
	memset (time, 0, sizeof (tmTemp));
		
	int minutes;
	
	minutes = timeValue / (ISC_TIME_SECONDS_PRECISION * 60);
	time->tm_hour = minutes / 60;
	time->tm_min = minutes % 60;
	time->tm_sec = (timeValue / ISC_TIME_SECONDS_PRECISION) % 60;

	return (int)strftime (buffer, length, format, time);
}

}; // end namespace IscDbcLibrary
