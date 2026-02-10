#pragma once

/// @file FbDateConvert.h
/// @brief Inline Firebird date/time encode/decode routines (Phase 9.6).
///
/// This header consolidates the Julian-day arithmetic that was previously
/// copy-pasted in 3 locations (OdbcDateTime.cpp, OdbcConvert.cpp, DateTime.cpp)
/// into a single canonical implementation.  All functions are inline, do no
/// heap allocation, and match the Firebird internal ISC_DATE / ISC_TIME format.
///
/// ISC_DATE = Julian day number (signed 32-bit)
/// ISC_TIME = fractions of a second since midnight in units of 1/10000 sec
/// ISC_TIME_SECONDS_PRECISION = 10000

#include <cstdint>

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

/// Encode a calendar date to ISC_DATE (Julian day number).
/// @param day    Day of month (1-31)
/// @param month  Month (1-12)
/// @param year   Full year (e.g. 2026)
/// @return ISC_DATE value
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
/// @param nday   ISC_DATE value
/// @param[out] day    Day of month (1-31)
/// @param[out] month  Month (1-12)
/// @param[out] year   Full year
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
/// @param hour    Hours (0-23)
/// @param minute  Minutes (0-59)
/// @param second  Seconds (0-59)
/// @return ISC_TIME value (fractional seconds = 0)
static inline ISC_TIME fb_encode_time(int hour, int minute, int second) noexcept
{
	return static_cast<ISC_TIME>(
		((hour * 60 + minute) * 60 + second) * ISC_TIME_SECONDS_PRECISION);
}

/// Decode ISC_TIME to time components (ignoring sub-second fractions).
/// @param ntime        ISC_TIME value
/// @param[out] hour    Hours (0-23)
/// @param[out] minute  Minutes (0-59)
/// @param[out] second  Seconds (0-59)
static inline void fb_decode_time(ISC_TIME ntime, int& hour, int& minute, int& second) noexcept
{
	int minutes = ntime / (ISC_TIME_SECONDS_PRECISION * 60);
	hour = minutes / 60;
	minute = minutes % 60;
	second = (ntime / ISC_TIME_SECONDS_PRECISION) % 60;
}

} // namespace IscDbcLibrary
