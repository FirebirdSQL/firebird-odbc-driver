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
 */

// DateTime.cpp: implementation of the DateTime class.
//
//////////////////////////////////////////////////////////////////////

#include <time.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include "IscDbc.h"
#include "DateTime.h"
#include "SQLError.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

#define TODAY	"today"

namespace IscDbcLibrary {

const char *months [] = {
    "January",
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September",
	"October",
	"November",
	"December",
	0
	};

const char *weekDays [] = {
    "Sunday",
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday",
	0
	};

static const char *timezones [] = {
	"est",
	"edt",
	"cst",
	"cdt",
	"mst",
	"mdt",
	"pst",
	"pdt",
	"gmt",
	"cet",
	"cest",
	"met",
	"bst",
	0
	};

static const int tzMinutes [] = {
	-5*60,	//	"est",
	-4*60,	//	"edt",
	-6*60,	//	"cst"
	-5*60,	//	"cdt",
	-7*60,	//	"mst",
	-6*60,	//	"mdt",
	-8*60,	//	"pst",
	-7*60,	//	"pdt",
	-0*60,	//	 gmt
	 1*60,	//	cet (central european time)
	 2*60,	//  cest
	 1*60,	//	met
	 1*60,	//	bst
	};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


DateTime DateTime::convert(const char *dateString, int length)
{
	DateTime date;
	const char *end = dateString + length;
	char string [100], *q = string;
	bool numeric = true;
//	bool sign = false;
	int		year = -1;
	int		month = 0;
	int		day = 0;
	int		hour = 0;
	int		second = 0;
	int		minute = 0;
	int		timezone = 0;
	int		n;
	int		state = 0;			// 1 = hour, 2 = minute, 3 = second
	const char *p = dateString;

	if (match (TODAY, dateString))
		{
		date.date = (int) time (NULL);
		return date;
		}

	for (char c = 1; c;)
		{
		if (p < end)
			c = *p++;
		else
			c = 0;
		switch (c)
			{
			case '-':
//				sign = true;
			case ' ':
			case ',':
			case '/':
			case ':':
			case ')':
			case 0:
				if (q > string)
					{
					*q = 0;
					if (numeric)
						{
						n = atoi (string);
						if (month == 0)
							month = n;
						else if (day == 0)
							day = n;
						else if (year < 0)
							year = n;
						else
							switch (state++)
								{
								case 0:
									hour = n;
									break;

								case 1:
									minute = n;
									break;

								case 2:
									second = n;
									break;

								case 3:
									timezone = n / 100 * 60 + n % 100;
									break;

								default:
									return conversionError();
								}
						}
					else if ((n = lookup (string, months)) >= 0)
						{
						if (month && !day)
							day = month;
						month = n + 1;
						}
					else if ((n = lookup (string, weekDays)) >= 0)
						{
						}
					else if ((n = lookup (string, timezones)) >= 0)
						timezone = tzMinutes [n];
					else
						{
						n = lookup (string, timezones);
						//return conversionError();
						}
					}
				q = string;
				numeric = true;
//				sign = false;
				break;

			case '(':
				break;

			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				*q++ = c;
				break;

			case '+':
//				sign = true;
				break;

			default:
				*q++ = c;
				numeric = false;			
			}
		}

	if (year < 0)
		{
		time_t t = time (NULL);
		struct tm *time = localtime (&t);
		year = time->tm_year + 1900;
		}
	else if (year < 100)
		if (year > 70)
			year += 1900;
		else
			year += 2000;

	struct tm	time;
	memset (&time, 0, sizeof (time));
	time.tm_sec = second;
	time.tm_min = minute;
	time.tm_hour = hour;
	time.tm_mday = day;
	time.tm_mon = month - 1;
	time.tm_year = year - 1900;
	time.tm_isdst = -1;
	date.date = encodeDate (&time);

	if ((!date.date) ||
		(date.date == -1) ||
	    (time.tm_mon != month - 1) || 
		(time.tm_mday != day))
		throw SQLEXCEPTION (CONVERSION_ERROR, 
				"error converting to date from %s", dateString);
	return date;
}

int DateTime::lookup(const char * string, const char * * table)
{
	for (const char **tbl = table; *tbl; ++tbl)
		if (match (string, *tbl))
			return tbl - table;

	return -1;
}

bool DateTime::match(const char * str1, const char * str2)
{
	for (; *str1 && *str2; ++str1, ++str2)
		if (UPPER (*str1) != UPPER (*str2))
			return false;

	return *str1 == 0;
}

DateTime DateTime::conversionError()
{
	DateTime date;
	date.date = 01;

	return date;
}

int DateTime::getString(int length, char * buffer)
{
	return getString ("%Y-%m-%d", length, buffer);
}

int DateTime::getString (const char * format, int length, char *buffer)
{
	struct tm tmTemp;
	struct tm *time = &tmTemp;
	memset (time, 0, sizeof (tmTemp));
	
	decodeDate (date, time);
	return (int)strftime (buffer, length, format, time);
	
}

int DateTime::getToday()
{
	time_t	t;
	time (&t);
	struct tm *time = localtime (&t);
	time->tm_hour = 0;
	time->tm_min = 0;
	time->tm_sec = 0;

	return (int) mktime (time);
}

int DateTime::getNow()
{
	time_t t;
	time (&t);

	return (int) t;
}

double DateTime::getDouble()
{
	return (double) date;
}	



signed int DateTime::decodeDate (signed int nday, tm	*times)
{
/**************************************
 *
 *	d e c o d e D a t e
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
	signed int	year, month, day;
	signed int	century;

//	nday -= 1721119 - 2400001;
	nday += 678882;

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
	return true;
}


signed int DateTime::encodeDate (struct tm	*times)
{
/**************************************
 *
 *	e n c o d e D a t e
 *
 **************************************
 *
 * Functional description
 *	Convert a calendar date to a numeric day
 *	(the number of days since the base date).
 *
 **************************************/
	signed short	day, month, year;
	signed int	c, ya;

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

	return (unsigned int) (((QUAD) 146097 * c) / 4 + 
		(1461 * ya) / 4 + 
		(153 * month + 2) / 5 + 
		day + 1721119 - 2400001);
}



signed int DateTime::yday (struct tm	*times)
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

	month = times->tm_mon;
	year = times->tm_year + 1900;

	day = (214 * month + 3) / 7 + times->tm_mday - 1;

	if (month < 2)
		return day;

	if ( year%4 == 0 && year%100 != 0 || year%400 == 0 )
		return day - 1;

	return day - 2;
}

}; // end namespace IscDbcLibrary
