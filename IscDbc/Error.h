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

// Error.h: interface for the Error class.
//
//////////////////////////////////////////////////////////////////////

// copyright (c) 1999 - 2000 by James A. Starkey for IBPhoenix.


#if !defined(_ERROR_H_)
#define _ERROR_H_

#undef ERROR
#undef ASSERT
#define ERROR	Error::error
#define ASSERT(f)	while (!(f)) Error::assertionFailed (__FILE__, __LINE__)
#define NOT_YET_IMPLEMENTED	ASSERT (false)


class Error  
{
public:
	static void assertionFailed (char *fileName, int line);
	static void error (char *text, ...);
	Error();
	~Error();

};

#endif // !defined(_ERROR_H_)
