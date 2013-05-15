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
 *  The Original Code was created by Ann W. Harrison for IBPhoenix.
 *
 *  Copyright (c) Ann W. Harrison
 *  All Rights Reserved.
 */
 
// SqlTime.h: interface for the SqlTime class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_TIME_H_INCLUDED_)
#define _TIME_H_INCLUDED_

namespace IscDbcLibrary {

class SqlTime
{
public:
	static SqlTime convert (const char *string, int length);
	int getString (int length, char *buffer);
	int getString (const char * format, int length, char *buffer);

	unsigned int timeValue;

};

}; // end namespace IscDbcLibrary

#endif // !defined(_TIME_H_INCLUDED_)
