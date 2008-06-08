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

// Parameter.h: interface for the Parameter class.
//
//////////////////////////////////////////////////////////////////////


#if !defined(_PARAMETER_H_)
#define _PARAMETER_H_

namespace IscDbcLibrary {

class Parameter
{
public:
	Parameter (Parameter *nxt, const char *nam, int namLen, const char *val, int valLen);
	~Parameter();

	int			nameLength;
	char		*name;
	int			valueLength;
	char		*value;
	Parameter	*next;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_PARAMETER_H_)
