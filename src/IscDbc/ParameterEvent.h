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
 *  The Original Code was created by Vladimir Tsvigun for IBPhoenix.
 *
 *  Copyright (c) 2005 Vladimir Tsvigun
 *  All Rights Reserved.
 */

// ParameterEvent.h: interface for the ParameterEvent class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ParameterEvent_H_)
#define _ParameterEvent_H_

namespace IscDbcLibrary {

class ParameterEvent
{
public:
	ParameterEvent( ParameterEvent *next, const char *name, int length );
	~ParameterEvent();

	char			*nameEvent;
	int				lengthNameEvent;
	unsigned int	countEvents;
	bool			changed;

	ParameterEvent	*nextParameter;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_ParameterEvent_H_)
