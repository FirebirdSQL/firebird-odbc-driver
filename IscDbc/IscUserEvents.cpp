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

// IscUserEvents.cpp user events class.
//
//////////////////////////////////////////////////////////////////////

#include "stdio.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "IscDbc.h"
#include "ParameterEvent.h"
#include "ParametersEvents.h"
#include "IscUserEvents.h"
#include "SQLError.h"

namespace IscDbcLibrary {

IscUserEvents::IscUserEvents( IscConnection *connect, PropertiesEvents *context, callbackEvent astRoutine )
{
	useCount = 1;
	eventBuffer = NULL;
	eventId = 0lu;
	lengthEventBlock = 0;

	connection = connect;
	events = (ParametersEvents*)context;
	events->addRef();
	callbackAstRoutine = astRoutine;

	initEventBlock();
}

IscUserEvents::~IscUserEvents()
{
	releaseEventBlock();
}

void IscUserEvents::releaseEventBlock()
{
	delete[] eventBuffer;
	eventBuffer = NULL;

	eventId = 0lu;
	lengthEventBlock = 0;

	if ( events && !events->release() )
		events = NULL;
}

void IscUserEvents::initEventBlock()
{
	char		*p;
	const char	*q;
	int			length;
	int			i;

	length = 1;

	i = events->getCount();
	while ( i-- )
		length += events->lengthNameEvent(i) + sizeof( long ) + sizeof( char );

	p = eventBuffer = new char[length];

	if ( !p )
	{
		// throw
		return;
	}

	// initialize the block with event names and counts
	*p++ = 1;

	i = events->getCount();
	while ( i-- )
	{
		*p++ = events->lengthNameEvent(i);
		q = events->getNameEvent(i);

		while ( (*p++ = *q++) );

		*p++ = 0;
		*p++ = 0;
		*p++ = 0;
	}

	lengthEventBlock = (short)(p - eventBuffer);
}

void IscUserEvents::queEvents()
{
	ISC_STATUS statusVector[20];
	connection->GDS->_que_events( statusVector, &connection->databaseHandle,
								   &eventId, lengthEventBlock, eventBuffer,
								   (isc_callback)callbackAstRoutine, (UserEvents*)this );

	if ( statusVector [1] )
		THROW_ISC_EXCEPTION( connection, statusVector );
}

inline
unsigned long IscUserEvents::vaxInteger( char * val )
{
	return (unsigned long)val[0] + ((unsigned long)val[1]<<8) + ((unsigned long)val[2]<<16) + ((unsigned long)val[3]<<24);
}

void IscUserEvents::eventCounts( char *result )
{
	char *p = eventBuffer + 1;
	char *q = result + 1;
	int	i = events->getCount();

	// analyze the event blocks, getting the delta for each event
	while ( i-- )
	{
		// skip over the event name
		p += *p + 1;
		q += *q + 1;

		// get the change in count
		events->updateCountExecutedEvents( i, vaxInteger( q ) - vaxInteger( p ) );

		int n = sizeof ( unsigned long );
		do 
			*p++ = *q++; 
		while ( --n );
	}
}

bool IscUserEvents::isChanged( int numEvent )
{
	return false;
}

int IscUserEvents::getCountUserEvents()
{
	return events->getCount();
}

void IscUserEvents::updateResultEvents( char * result )
{
	eventCounts( result );
}

void IscUserEvents::addRef()
{
	++useCount;
}

int IscUserEvents::release()
{
	if (--useCount == 0)
	{
		delete this;
		return 0;
	}

	return useCount;
}

int IscUserEvents::objectVersion()
{
	return USEREVENTS_VERSION;
}

}; // end namespace IscDbcLibrary
