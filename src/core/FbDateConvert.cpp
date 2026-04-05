// FbDateConvert.cpp — DateTime::convert() implementation.
// Phase 14.7.2: Moved from DateTime.cpp.

#include "IscDbc.h"
#include "FbDateConvert.h"
#include "SQLError.h"
#include <ctime>
#include <cstdlib>
#include <cstring>

namespace IscDbcLibrary {

static const char* months[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December", nullptr};

static const char* weekDays[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", nullptr};

static const char* timezones[] = {
    "est", "edt", "cst", "cdt", "mst", "mdt", "pst", "pdt",
    "gmt", "cet", "cest", "met", "bst", nullptr};

static const int tzMinutes[] = {
    -5 * 60, -4 * 60, -6 * 60, -5 * 60, -7 * 60, -6 * 60, -8 * 60, -7 * 60,
    0, 1 * 60, 2 * 60, 1 * 60, 1 * 60};

static bool matchStr(const char* str1, const char* str2)
{
	for (; *str1 && *str2; ++str1, ++str2)
		if (UPPER(*str1) != UPPER(*str2))
			return false;
	return *str1 == 0;
}

static int lookupStr(const char* string, const char** table)
{
	for (const char** tbl = table; *tbl; ++tbl)
		if (matchStr(string, *tbl))
			return static_cast<int>(tbl - table);
	return -1;
}

DateTime DateTime::convert(const char* dateString, int length)
{
	DateTime date;
	const char* end = dateString + length;
	char string[100], *q = string;
	bool numeric = true;
	int year = -1, month = 0, day = 0, hour = 0, second = 0, minute = 0;
	int n, state = 0;
	const char* p = dateString;

	if (matchStr("today", dateString))
	{
		date.date = static_cast<ISC_DATE>(time(nullptr));
		return date;
	}

	for (char c = 1; c;)
	{
		if (p < end) c = *p++;
		else c = 0;

		switch (c)
		{
		case '-':
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
					n = atoi(string);
					if (month == 0) month = n;
					else if (day == 0) day = n;
					else if (year < 0) year = n;
					else switch (state++)
					{
					case 0: hour = n; break;
					case 1: minute = n; break;
					case 2: second = n; break;
					default:
						date.date = 1;
						return date;
					}
				}
				else if ((n = lookupStr(string, months)) >= 0)
				{
					if (month && !day) day = month;
					month = n + 1;
				}
				else if ((n = lookupStr(string, weekDays)) >= 0) {}
				else { lookupStr(string, timezones); }
			}
			q = string;
			numeric = true;
			break;

		case '(':
			break;

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			*q++ = c;
			break;

		case '+':
			break;

		default:
			*q++ = c;
			numeric = false;
		}
	}

	if (year < 0)
	{
		time_t t = time(nullptr);
		struct tm* tm = localtime(&t);
		year = tm->tm_year + 1900;
	}
	else if (year < 100)
	{
		if (year > 70) year += 1900;
		else year += 2000;
	}

	struct tm time;
	memset(&time, 0, sizeof(time));
	time.tm_sec = second;
	time.tm_min = minute;
	time.tm_hour = hour;
	time.tm_mday = day;
	time.tm_mon = month - 1;
	time.tm_year = year - 1900;
	time.tm_isdst = -1;
	date.date = encodeDate(&time);

	if ((!date.date) || (date.date == -1) ||
	    (time.tm_mon != month - 1) || (time.tm_mday != day))
		throw SQLEXCEPTION(CONVERSION_ERROR, "error converting to date from %s", dateString);

	return date;
}

} // namespace IscDbcLibrary
