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
 *  The Original Code was created by Jame6s A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 */

// DateTime.h: interface for the DateTime class.
//
//////////////////////////////////////////////////////////////////////


#if !defined(_DATETIME_H_)
#define _DATETIME_H_

namespace IscDbcLibrary {

class DateTime  
{
public:
	int getString (int length, char *buffer);
	int getString (const char * format, int length, char * buffer);
	double getDouble();
	static int getNow();
	static int getToday();
	static DateTime conversionError();
	static bool match (const char *str1, const char *str2);
	static int lookup (const char *string, const char **table);
	static DateTime convert (const char *string, int length);
	static signed int decodeDate (signed int nday, struct tm *times);
	static signed int encodeDate (struct tm *times);
	static signed int yday (struct tm	*times);

	signed int date;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_DATETIME_H_)
