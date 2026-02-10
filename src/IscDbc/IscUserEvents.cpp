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
// Phase 9.4: Migrated from ISC isc_que_events to OO API IAttachment::queEvents.
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

using namespace Firebird;

namespace IscDbcLibrary {

// ============================================================
// FbEventCallback — OO API bridge (Phase 9.4)
// ============================================================

void FbEventCallback::eventCallbackFunction(unsigned length, const unsigned char* events)
{
	if (!owner_)
		return;

	// Bridge to legacy callback signature: void(void*, short, char*)
	// The legacy callback receives the IscUserEvents pointer (or an
	// alternate interface) and the raw event buffer for processing.
	owner_->callbackAstRoutine(
		owner_,
		static_cast<short>(length),
		const_cast<char*>(reinterpret_cast<const char*>(events)));
}

// ============================================================
// IscUserEvents
// ============================================================

IscUserEvents::IscUserEvents( IscConnection *connect, PropertiesEvents *context, callbackEvent astRoutine, void *userAppData )
{
	useCount = 1;
	eventBuffer = NULL;
	eventsHandle = nullptr;
	lengthEventBlock = 0;

	connection = connect;
	events = (ParametersEvents*)context;
	events->addRef();
	callbackAstRoutine = astRoutine;
	userData = userAppData;

	callback_.setOwner(this);

	initEventBlock();
}

IscUserEvents::~IscUserEvents()
{
	releaseEventBlock();
}

void IscUserEvents::releaseEventBlock()
{
	// Cancel any pending event subscription
	if (eventsHandle)
	{
		try
		{
			ThrowStatusWrapper status(connection->GDS->_status);
			eventsHandle->cancel(&status);
		}
		catch (...) {}
		eventsHandle = nullptr;
	}

	delete[] eventBuffer;
	eventBuffer = NULL;

	lengthEventBlock = 0;

	if ( events && !events->release() )
		events = NULL;
}

void IscUserEvents::initEventBlock()
{
	unsigned char	*p;
	const char		*q;
	int length = 1;

	ParameterEvent *param = events->getHeadPosition();
	while ( param )
	{
		length += param->lengthNameEvent + sizeof( long ) + sizeof( char );
		param = events->getNext();
	}

	p = eventBuffer = new unsigned char[length];

	if ( !p )
	{
		// throw
		return;
	}

	// initialize the block with event names and counts
	*p++ = 1;

	param = events->getHeadPosition();
	while ( param )
	{
		*p++ = static_cast<unsigned char>(param->lengthNameEvent);
		q = param->nameEvent;

		while ( (*p++ = static_cast<unsigned char>(*q++)) );

		*p++ = 0;
		*p++ = 0;
		*p++ = 0;

		param = events->getNext();
	}

	lengthEventBlock = (short)(p - eventBuffer);
}

void IscUserEvents::queEvents( void * interfase )
{
	// Phase 9.4: Use OO API IAttachment::queEvents instead of ISC isc_que_events.
	// This eliminates the need for _get_database_handle bridge and isc_que_events pointer.
	ThrowStatusWrapper status(connection->GDS->_status);
	try
	{
		// Cancel previous subscription if any
		if (eventsHandle)
		{
			eventsHandle->cancel(&status);
			eventsHandle = nullptr;
		}

		eventsHandle = connection->databaseHandle->queEvents(
			&status,
			&callback_,
			static_cast<unsigned>(lengthEventBlock),
			eventBuffer);
	}
	catch (const FbException& error)
	{
		THROW_ISC_EXCEPTION(connection, error.getStatus());
	}
}

inline
unsigned long IscUserEvents::vaxInteger( const unsigned char * val )
{
	return (unsigned long)val[0] + ((unsigned long)val[1]<<8) + ((unsigned long)val[2]<<16) + ((unsigned long)val[3]<<24);
}

void IscUserEvents::eventCounts( const unsigned char *result )
{
	unsigned char *p = eventBuffer + 1;
	const unsigned char *q = result + 1;

	ParameterEvent *param = events->getHeadPosition();
	while ( param )
	{
		// skip over the event name
		p += *p + 1;
		q += *q + 1;

		// get the change in count
		unsigned long count = vaxInteger( q ) - vaxInteger( p ); 
		if ( count )
		{
			param->countEvents += count;
			if ( param->countEvents )
				param->changed = true;
		}
		else
			param->changed = false;

		int n = sizeof ( unsigned long );
		do 
			*p++ = *q++; 
		while ( --n );

		param = events->getNext();
	}
}

bool IscUserEvents::isChanged( int numEvent )
{
	return events->isChanged( numEvent );
}

unsigned long IscUserEvents::getCountEvents( int numEvent )
{
	return events->getCountExecutedEvents( numEvent );
}

int IscUserEvents::getCountRegisteredNameEvents()
{
	return events->getCount();
}

void IscUserEvents::updateResultEvents( char * result )
{
	eventCounts( reinterpret_cast<const unsigned char*>(result) );
}

void* IscUserEvents::getUserData()
{
	return userData;
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
