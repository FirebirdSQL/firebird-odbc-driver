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

// ParameterEvent.cpp: implementation of the ParameterEvent class.
//
//////////////////////////////////////////////////////////////////////

#include <memory.h>
#include "ParameterEvent.h"

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ParameterEvent::ParameterEvent( ParameterEvent *next, const char *name, int length )
{
	nextParameter = next;
	lengthNameEvent = length;
	countEvents = ~0lu;
	changed = false;

	nameEvent = new char[lengthNameEvent + 1];
	memcpy( nameEvent, name, lengthNameEvent );
	nameEvent[lengthNameEvent] = 0;
}

ParameterEvent::~ParameterEvent()
{
	delete [] nameEvent;
}

}; // end namespace IscDbcLibrary
