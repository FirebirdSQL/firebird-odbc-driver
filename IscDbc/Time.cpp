// Time.cpp: implementation of the Time class.
//
//////////////////////////////////////////////////////////////////////

#include "Time.h"
#include <time.h>
#include <string.h>

Time& Time::operator =(long value)
{
	timeValue = value;

	return *this;
}



Time Time::convert(const char * string, int length)
{
	Time time;
	time = 0;
	return time;
}

int Time::getString (int length, char * buffer)
{
	return getString ("%H:%M:%S", length, buffer);
}

int Time::getString(const char * format, int length, char * buffer)
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



