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
 *
 *
 *	Change Log
 *
 * 2002-05-20	Updated OdbcDateTime.cpp
 *
 *				Contributed by Bernhard Schulte 
 *				-	improvements to conversion routines
 *				o	TimeStamp Struct to Timestamp 
 *				o	DateTime to Date Struct 
 *				o	TimeStamp to Timestamp Struct
 *				o	ndate takes a new parameter - seconds, 
 *					because days won't fit in otherwise.
 *
 */

/*
 *	PROGRAM:		OdbcJdbc
 *	MODULE:			OdbcDateTime.cpp
 *	DESCRIPTION:	High level date conversion routines
 *
 * copyright (c) 2000 by Ann W. Harrison
 */
// OdbcDateTime.cpp: implementation of the OdbcDateTime class.
//
//////////////////////////////////////////////////////////////////////

#include "OdbcDateTime.h"
#include "Connection.h"
#include <memory.h>
#include "OdbcStatement.h"
#include <time.h>
#include "DateTime.h"
#include "IscDbc/SqlTime.h"
#include "TimeStamp.h"


#define SECONDS_PRECISION          10000L
#define SECONDS_PRECISION_SCALE    -4
typedef signed long		INTERNAL_DATE;
typedef __int64			SINT64;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

OdbcDateTime::OdbcDateTime()
{

}

OdbcDateTime::~OdbcDateTime()
{

}

int	OdbcDateTime::convert (tagDATE_STRUCT * tagDateIn, DateTime * dateTimeOut)
{
	struct tm timeBuffer;
	struct tm* times = &timeBuffer;
	memset (times, 0, sizeof (*times));
	times->tm_year = tagDateIn->year - 1900;
	times->tm_mon = tagDateIn->month - 1;
	times->tm_mday = tagDateIn->day;
	times->tm_isdst = -1;
	dateTimeOut->date = nday ((struct tm*) times);
	return true;
}

int	OdbcDateTime::convert (tagTIMESTAMP_STRUCT * tagTimeStampIn, DateTime * dateTimeOut)
{
	struct tm timeBuffer;
	struct tm* times = &timeBuffer;
	memset (times, 0, sizeof (*times));

	times->tm_year = tagTimeStampIn->year - 1900;
	times->tm_mon = tagTimeStampIn->month - 1;
	times->tm_mday = tagTimeStampIn->day;
	times->tm_hour = tagTimeStampIn->hour;
	times->tm_min = tagTimeStampIn->minute;
	times->tm_sec = tagTimeStampIn->second;
	times->tm_isdst = -1;

	dateTimeOut->date = nday ((struct tm*) times);
	return true;
}



int OdbcDateTime::convert (tagTIMESTAMP_STRUCT * tagTimeStampIn, TimeStamp * timeStampOut)
{
	struct tm timeBuffer;
	struct tm* times = &timeBuffer;

/*	Original conversion
	timeStampOut->nanos = ((times->tm_hour * 60 + times->tm_min) * 60 + 
		times->tm_sec) * SECONDS_PRECISION;
*/
//New conversion supplied by Bernard Schulte
	times->tm_hour = tagTimeStampIn->hour;
	times->tm_min  = tagTimeStampIn->minute;
	times->tm_sec  = tagTimeStampIn->second;
	times->tm_mday = tagTimeStampIn->day;
	times->tm_mon  = tagTimeStampIn->month-1;
	times->tm_year = tagTimeStampIn->year-1900;

	timeStampOut->nanos = ((tagTimeStampIn->hour * 60 + tagTimeStampIn->minute) * 60 + 
		tagTimeStampIn->second) ; 

	timeStampOut->date = nday(times);

	return true;
}	



int OdbcDateTime::convert (DateTime * dateTimeIn, tagDATE_STRUCT * tagDateOut)
{
	struct tm timeBuffer;
	struct tm* times = &timeBuffer;
	memset (times, 0, sizeof (*times));

//	ndate (dateTimeIn->date, times);
//	From B. Schulte
	ndate ((dateTimeIn->date / 24 /60/60),0, times); 

	times->tm_yday = yday (times);
	if ((times->tm_wday = ((dateTimeIn->date) + 3) % 7) < 0)
    times->tm_wday += 7;

	tagDateOut->year = times->tm_year + 1900;
	tagDateOut->month = times->tm_mon + 1;
	tagDateOut->day = times->tm_mday;	
	return true;
	
}


int OdbcDateTime::convert (TimeStamp *timeStampIn, tagTIMESTAMP_STRUCT * tagTimeStampOut)
{
	struct tm timeBuffer;
	struct tm* times = &timeBuffer;
	memset (times, 0, sizeof (*times));
//Orig.
//	ndate (timeStampIn->date, times);
//From B. Schulte
    ndate (timeStampIn->date, timeStampIn->nanos, times);

	tagTimeStampOut->year = times->tm_year + 1900;
	tagTimeStampOut->month = times->tm_mon + 1;
	tagTimeStampOut->day = times->tm_mday;
	tagTimeStampOut->hour = times->tm_hour;
	tagTimeStampOut->minute = times->tm_min;
	tagTimeStampOut->second = times->tm_sec;
	tagTimeStampOut->fraction = timeStampIn->nanos * 10;
	return true;
}

//Orig
//signed long OdbcDateTime::ndate (signed long nday, tm	*times)
//From B. Schulte
// this function got a new parameter (nsec) .. because the days won't fit otherweise
signed long OdbcDateTime::ndate (signed long nday, signed long nsec, tm        *times)

{
/**************************************
 *
 *	n d a t e
 *
 **************************************
 *
 * Functional description
 *	Convert a numeric day to [day, month, year].
 *
 * Calenders are divided into 4 year cycles.
 * 3 Non-Leap years, and 1 leap year.
 * Each cycle takes 365*4 + 1 == 1461 days.
 * There is a further cycle of 100 4 year cycles.
 * Every 100 years, the normally expected leap year
 * is not present.  Every 400 years it is.
 * This cycle takes 100 * 1461 - 3 == 146097 days
 *
 * The origin of the constant 2400001 is unknown.
 * The origin of the constant 1721119 is unknown.
 *
 * The difference between 2400001 and 1721119 is the
 * number of days From 0/0/0000 to our base date of
 * 11/xx/1858. (678882)
 * The origin of the constant 153 is unknown.
 *
 * This whole routine has problems with ndates
 * less than -678882 (Approx 2/1/0000).
 *
 **************************************/
	SLONG	year, month, day;
	SLONG	century;
	SLONG	seconds;

/*  Orig.
	seconds = nday % (60 * 60 * 24);
	nday = nday / (60 * 60 * 24);
*/
// From B. Schulte:
	seconds = nsec;
	

/*	adjust first from the IB base date to the SQL base date*/
	nday += 40587; 

	nday -= 1721119 - 2400001;

	century = (4 * nday - 1) / 146097;
	nday = 4 * nday - 1 - 146097 * century;
	day = nday / 4;

	nday = (4 * day + 3) / 1461;
	day = 4 * day + 3 - 1461 * nday;
	day = (day + 4) / 4;

	month = (5 * day - 3) / 153;
	day = 5 * day - 3 - 153 * month;
	day = (day + 5) / 5;

	year = 100 * century + nday;

	if (month < 10)
		month += 3;
	else
		{
		month -= 9;
		year += 1;
		}

	times->tm_mday = (int) day;
	times->tm_mon = (int) month - 1;
	times->tm_year = (int) year - 1900;
	times->tm_hour = (int) seconds / (60 * 60);
	times->tm_min = (int) (seconds / 60) - ((times->tm_hour) * 60);
	times->tm_sec = (int) (seconds - ((times->tm_hour*60)+times->tm_min)*60);
	return true;
}


INTERNAL_DATE OdbcDateTime::nday (struct tm	*times)
{
/**************************************
 *
 *	n d a y
 *
 **************************************
 *
 * Functional description
 *	Convert a calendar date to a numeric day
 *	(the number of days since the base date).
 *
 **************************************/
	signed short	day, month, year;
	signed long	c, ya;

	day = times->tm_mday;
	month = times->tm_mon + 1;
	year = times->tm_year + 1900;

	if (month > 2)
		month -= 3;
	else
		{
		month += 9;
		year -= 1;
		}

	c = year / 100;
	ya = year - 100 * c;

	return (INTERNAL_DATE) (((SINT64) 146097 * c) / 4 + 
		(1461 * ya) / 4 + 
		(153 * month + 2) / 5 + 
		day + 1721119 - 2400001);
}



signed long OdbcDateTime::yday (struct tm	*times)

{
/**************************************
 *
 *	y d a y
 *
 **************************************
 *
 * Functional description
 *	Convert a calendar date to the day-of-year.
 *
 *	The unix time structure considers
 *	january 1 to be Year day 0, although it
 *	is day 1 of the month.   (Note that QLI,
 *	when printing Year days takes the other
 *	view.)   
 *
 **************************************/
	signed short	day, month, year;
	const unsigned char	*days;
	static const unsigned char month_days [] = {31,28,31,30,31,30,31,31,30,31,30,31};

	day = times->tm_mday;
	month = times->tm_mon;
	year = times->tm_year + 1900;

	--day;

	for (days = month_days; days < month_days + month; days++)
		day += *days;

	if (month < 2)
		return day;

	/* Add a day as we're past the leap-day */
	if (! (year % 4))
		day++;

	/* Ooops - year's divisible by 100 aren't leap years */
	if (! (year % 100))
		day--;

	/* Unless they are also divisible by 400! */
	if (! (year % 400))
		day++;

	return day;
}

