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

// Timestamp.cpp: implementation of the Timestamp class.
//
//////////////////////////////////////////////////////////////////////


#include "Engine.h"
#include "TimeStamp.h"
#include <time.h>
#include <string.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


TimeStamp& TimeStamp::operator =(DateTime value)
{
	date = value.date;
	nanos = 0;

	return *this;
}

TimeStamp& TimeStamp::operator =(long value)
{
	date = value;
	nanos = 0;

	return *this;
}

int TimeStamp::getTimeString(int length, char * buffer)
{
	struct tm tmTemp;
	struct tm *time = &tmTemp;
	memset (time, 0, sizeof (tmTemp));

	DateTime::decodeDate (date, time);
	decodeTime (nanos, time);
	
	return strftime (buffer, length, "%Y-%m-%d %H:%M:%S", time);
}

int TimeStamp::decodeTime (long nanos, tm * times)
{
	long seconds = nanos / 100;
	times->tm_hour = seconds / (60 * 60);
	seconds -= times->tm_hour * 60 * 60;
	times->tm_min = seconds / 60;
	times->tm_sec = seconds - times->tm_min * 60;
	return true;
}