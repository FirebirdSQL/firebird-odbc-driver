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

// ParametersEvents.cpp: implementation of the ParametersEvents class.
//
//////////////////////////////////////////////////////////////////////

#include <string.h>
#include "IscDbc.h"
#include "ParametersEvents.h"

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ParametersEvents::ParametersEvents()
{
	useCount = 1;
	parameters = NULL;
	parameterPositions = NULL;
	count = 0;
}

ParametersEvents::~ParametersEvents()
{
	clear();
}

void ParametersEvents::putNameEvent( const char * name )
{
	++count;

	ParameterEvent **parameter = &parameters;

	while ( *parameter )
		parameter = &(*parameter)->nextParameter;

	*parameter = new ParameterEvent( *parameter, name, (int)strlen( name ) );
}

int ParametersEvents::findIndex( const char * name )
{
	int index = 0;
	for ( ParameterEvent *parameter = parameters; parameter; parameter = parameter->nextParameter, index++ )
		if ( !strcasecmp( name, parameter->nameEvent ) )
			return index;

	return -1;
}

const char* ParametersEvents::getNameEvent( int index )
{
	if ( index < 0 || index >= count )
		return NULL;

	ParameterEvent *parameter = parameters;

	while ( index-- )
		parameter = parameter->nextParameter;

	return parameter->nameEvent;
}

unsigned long ParametersEvents::getCountExecutedEvents( int index )
{
	if ( index < 0 || index >= count )
		return 0;

	ParameterEvent *parameter = parameters;

	while ( index-- )
		parameter = parameter->nextParameter;

	return parameter->countEvents;
}

bool ParametersEvents::isChanged( int index )
{
	if ( index < 0 || index >= count )
		return false;

	ParameterEvent *parameter = parameters;

	while ( index-- )
		parameter = parameter->nextParameter;

	return parameter->changed;
}

void ParametersEvents::clear()
{
	for ( ParameterEvent *parameter; parameter = parameters; )
	{
		parameters = parameter->nextParameter;
		delete parameter;
	}
}

void ParametersEvents::addRef()
{
	++useCount;
}

int ParametersEvents::release()
{
	if (--useCount == 0)
	{
		delete this;
		return 0;
	}

	return useCount;
}

}; // end namespace IscDbcLibrary
