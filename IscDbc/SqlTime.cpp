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
 *  Copyright (c) Ann W. Harrison
 *  All Rights Reserved.
 */
// SqlTime.cpp: implementation of the SqlTime class.
//
//////////////////////////////////////////////////////////////////////

#include "SqlTime.h"
#include <time.h>
#include <string.h>

SqlTime& SqlTime::operator =(long value)
{
	timeValue = value;

	return *this;
}



SqlTime SqlTime::convert(const char * string, int length)
{
	SqlTime time;
	time = 0;
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
		
	long seconds = timeValue;
	time->tm_hour = timeValue / (60 * 60);
	seconds -= time->tm_hour * 60 * 60;
	time->tm_min = seconds / 60;
	time->tm_sec = seconds - time->tm_min * 60;
	
	return strftime (buffer, length, format, time);
}



