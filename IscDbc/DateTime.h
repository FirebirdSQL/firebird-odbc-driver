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

// DateTime.h: interface for the DateTime class.
//
//////////////////////////////////////////////////////////////////////


#if !defined(AFX_DATETIME_H__84FD1970_A97F_11D2_AB5C_0000C01D2301__INCLUDED_)
#define AFX_DATETIME_H__84FD1970_A97F_11D2_AB5C_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

struct tm;

class DateTime  
{
public:
	DateTime& operator = (QUAD value);
	DateTime& operator = (long value);
	int getString (int length, char *buffer);
	int getString (const char * format, int length, char * buffer);
	double getDouble();
	static long getNow();
	static long getToday();
	static DateTime conversionError();
	static bool match (const char *str1, const char *str2);
	static int lookup (const char *string, const char **table);
	static DateTime convert (const char *string, int length);
	static signed long decodeDate (signed long nday, tm *times);
	static signed long encodeDate (tm *times);
	static signed long yday (struct tm	*times);

	signed long date;
};


#endif // !defined(AFX_DATETIME_H__84FD1970_A97F_11D2_AB5C_0000C01D2301__INCLUDED_)
