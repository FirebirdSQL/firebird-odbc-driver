#pragma once

/// @file FbDateConvert.h
/// @brief Inline Firebird date/time types and encode/decode routines.
///
/// Phase 14.7.2: Consolidates DateTime/SqlTime/TimeStamp struct definitions
/// and the Julian-day arithmetic (previously in DateTime.cpp, SqlTime.cpp,
/// TimeStamp.cpp) into a single header.
///
/// ISC_DATE = Julian day number (signed 32-bit)
/// ISC_TIME = fractions of a second since midnight in units of 1/10000 sec
/// ISC_TIME_SECONDS_PRECISION = 10000

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cstdlib>

// Use Firebird types if ibase.h is included, otherwise define minimal stubs.
#ifndef FIREBIRD_IBASE_H
using ISC_DATE = int32_t;
using ISC_TIME = uint32_t;
using ISC_INT64 = int64_t;
#ifndef ISC_TIME_SECONDS_PRECISION
#define ISC_TIME_SECONDS_PRECISION 10000
#endif
#endif

namespace IscDbcLibrary {

// -----------------------------------------------------------------------
//  Inline arithmetic helpers
// -----------------------------------------------------------------------

/// Encode a calendar date to ISC_DATE (Julian day number).
static inline ISC_DATE fb_encode_date(int day, int month, int year) noexcept
{
	if (month > 2)
		month -= 3;
	else
	{
		month += 9;
		year -= 1;
	}

	int c = year / 100;
	int ya = year - 100 * c;

	return static_cast<ISC_DATE>(
		(static_cast<ISC_INT64>(146097) * c) / 4 +
		(1461 * ya) / 4 +
		(153 * month + 2) / 5 +
		day + 1721119 - 2400001);
}

/// Decode an ISC_DATE (Julian day number) to calendar date.
static inline void fb_decode_date(ISC_DATE nday, int& day, int& month, int& year) noexcept
{
	nday += 678882;

	int century = (4 * nday - 1) / 146097;
	nday = 4 * nday - 1 - 146097 * century;
	int d = nday / 4;

	nday = (4 * d + 3) / 1461;
	d = 4 * d + 3 - 1461 * nday;
	d = (d + 4) / 4;

	month = (5 * d - 3) / 153;
	d = 5 * d - 3 - 153 * month;
	day = (d + 5) / 5;

	year = 100 * century + nday;

	if (month < 10)
		month += 3;
	else
	{
		month -= 9;
		year += 1;
	}
}

/// Encode time components to ISC_TIME.
static inline ISC_TIME fb_encode_time(int hour, int minute, int second) noexcept
{
	return static_cast<ISC_TIME>(
		((hour * 60 + minute) * 60 + second) * ISC_TIME_SECONDS_PRECISION);
}

/// Decode ISC_TIME to time components (ignoring sub-second fractions).
static inline void fb_decode_time(ISC_TIME ntime, int& hour, int& minute, int& second) noexcept
{
	int minutes = ntime / (ISC_TIME_SECONDS_PRECISION * 60);
	hour = minutes / 60;
	minute = minutes % 60;
	second = (ntime / ISC_TIME_SECONDS_PRECISION) % 60;
}

/// Decode ISC_DATE to struct tm.
static inline void fb_decode_date_to_tm(ISC_DATE nday, struct tm* times) noexcept
{
	int day, month, year;
	fb_decode_date(nday, day, month, year);
	times->tm_mday = day;
	times->tm_mon = month - 1;
	times->tm_year = year - 1900;
}

/// Encode struct tm to ISC_DATE.
static inline ISC_DATE fb_encode_date_from_tm(const struct tm* times) noexcept
{
	return fb_encode_date(times->tm_mday, times->tm_mon + 1, times->tm_year + 1900);
}

/// Decode ISC_TIME to struct tm time fields.
static inline void fb_decode_time_to_tm(ISC_TIME ntime, struct tm* times) noexcept
{
	int h, m, s;
	fb_decode_time(ntime, h, m, s);
	times->tm_hour = h;
	times->tm_min = m;
	times->tm_sec = s;
}

// -----------------------------------------------------------------------
//  Date/time POD types (replacing DateTime.h, SqlTime.h, TimeStamp.h)
// -----------------------------------------------------------------------

/// SQL DATE — wraps ISC_DATE (Julian day number).
struct DateTime
{
	ISC_DATE date;

	/// Format as string (default "%Y-%m-%d").
	int getString(int length, char* buffer) const
	{
		return getString("%Y-%m-%d", length, buffer);
	}

	/// Format as string with custom strftime format.
	int getString(const char* format, int length, char* buffer) const
	{
		struct tm tmTemp;
		std::memset(&tmTemp, 0, sizeof(tmTemp));
		fb_decode_date_to_tm(date, &tmTemp);
		return static_cast<int>(strftime(buffer, length, format, &tmTemp));
	}

	/// Convert as double (the raw ISC_DATE value).
	double getDouble() const { return static_cast<double>(date); }

	/// Parse a date string. Supports ISO format and loose formats.
	static DateTime convert(const char* string, int length);

	/// Decode ISC_DATE to struct tm (legacy API).
	static ISC_DATE decodeDate(ISC_DATE nday, struct tm* times)
	{
		fb_decode_date_to_tm(nday, times);
		return true;
	}

	/// Encode struct tm to ISC_DATE (legacy API).
	static ISC_DATE encodeDate(struct tm* times)
	{
		return fb_encode_date_from_tm(times);
	}
};

/// SQL TIME — wraps ISC_TIME (1/10000-second units since midnight).
struct SqlTime
{
	ISC_TIME timeValue;

	/// Format as string (default "%H:%M:%S").
	int getString(int length, char* buffer) const
	{
		return getString("%H:%M:%S", length, buffer);
	}

	/// Format as string with custom strftime format.
	int getString(const char* format, int length, char* buffer) const
	{
		struct tm tmTemp;
		std::memset(&tmTemp, 0, sizeof(tmTemp));
		fb_decode_time_to_tm(timeValue, &tmTemp);
		return static_cast<int>(strftime(buffer, length, format, &tmTemp));
	}

	/// Parse a time string (stub — returns zero).
	static SqlTime convert(const char* /*string*/, int /*length*/)
	{
		SqlTime t;
		t.timeValue = 0;
		return t;
	}
};

/// SQL TIMESTAMP — combines ISC_DATE + ISC_TIME.
struct TimeStamp : DateTime
{
	ISC_TIME nanos; ///< ISC_TIME sub-second component

	/// Format as "YYYY-MM-DD HH:MM:SS[.ffff]".
	int getTimeString(int length, char* buffer) const
	{
		struct tm tmTemp;
		std::memset(&tmTemp, 0, sizeof(tmTemp));
		fb_decode_date_to_tm(date, &tmTemp);
		fb_decode_time_to_tm(nanos, &tmTemp);

		int len = static_cast<int>(strftime(buffer, length, "%Y-%m-%d %H:%M:%S", &tmTemp));
		// Append sub-second fractions if nonzero.
		int frac = nanos % ISC_TIME_SECONDS_PRECISION;
		if (frac)
			len += std::sprintf(buffer + len, ".%04d", frac);
		return len;
	}

	/// Decode ISC_TIME to struct tm time fields (legacy API).
	static int decodeTime(ISC_TIME nanos, struct tm* times)
	{
		fb_decode_time_to_tm(nanos, times);
		return true;
	}

	/// Parse a timestamp string (stub — returns zero).
	static TimeStamp convert(const char* /*string*/, int /*length*/)
	{
		TimeStamp ts;
		ts.date = 0;
		ts.nanos = 0;
		return ts;
	}
};

} // namespace IscDbcLibrary
