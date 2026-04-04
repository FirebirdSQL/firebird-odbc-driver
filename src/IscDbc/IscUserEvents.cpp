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
// Phase 14.6: Migrated from manual OO API event handling to fbcpp::EventListener.
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
#include "IscConnection.h"
#include "SQLError.h"
#include <fb-cpp/EventListener.h>

namespace IscDbcLibrary {

// ============================================================
// IscUserEvents — Phase 14.6: fbcpp::EventListener bridge
// ============================================================

IscUserEvents::IscUserEvents( IscConnection *connect, PropertiesEvents *context, callbackEvent astRoutine, void *userAppData )
{
	useCount = 1;
	connection = connect;
	events = (ParametersEvents*)context;
	events->addRef();
	callbackAstRoutine = astRoutine;
	userData = userAppData;

	// Collect event names from ParametersEvents linked list
	ParameterEvent *param = events->getHeadPosition();
	while ( param )
	{
		eventNames_.emplace_back(param->nameEvent, param->lengthNameEvent);
		param = events->getNext();
	}
}

IscUserEvents::~IscUserEvents()
{
	// Stop the event listener (RAII cleanup)
	eventListener_.reset();

	if ( events && !events->release() )
		events = NULL;
}

void IscUserEvents::onEventFired(const std::vector<fbcpp::EventCount>& counts)
{
	// Bridge fbcpp::EventListener callback to legacy ODBC event interface.
	// Update ParameterEvent counts and changed flags, then invoke the AST callback.
	{
		std::lock_guard<std::mutex> lock(mutex_);

		ParameterEvent *param = events->getHeadPosition();
		for (const auto& ec : counts)
		{
			if (!param) break;

			if (ec.count > 0)
			{
				param->countEvents += ec.count;
				param->changed = true;
			}
			else
			{
				param->changed = false;
			}

			param = events->getNext();
		}
	}

	// Invoke the legacy AST callback (application-defined handler).
	// The legacy signature expects (void* userEvents, short length, char* buffer).
	// With fbcpp::EventListener, raw buffers are not exposed; pass nullptr/0.
	// The app should call SQL_FB_UPDATECOUNT_EVENTS to retrieve processed counts.
	if (callbackAstRoutine)
	{
		callbackAstRoutine(this, 0, nullptr);
	}
}

void IscUserEvents::queEvents( void * interfase )
{
	if (started_ && eventListener_ && eventListener_->isListening())
		return; // Already listening — fbcpp auto-re-queues

	try
	{
		// Stop any existing listener
		eventListener_.reset();

		// Create fbcpp::EventListener with RAII lifecycle
		eventListener_ = std::make_unique<fbcpp::EventListener>(
			*connection->attachment_,
			eventNames_,
			[this](const std::vector<fbcpp::EventCount>& counts) {
				onEventFired(counts);
			});

		started_ = true;
	}
	catch (const fbcpp::DatabaseException& error)
	{
		throw SQLEXCEPTION(RUNTIME_ERROR, error.what());
	}
}

bool IscUserEvents::isChanged( int numEvent )
{
	std::lock_guard<std::mutex> lock(mutex_);
	return events->isChanged( numEvent );
}

unsigned long IscUserEvents::getCountEvents( int numEvent )
{
	std::lock_guard<std::mutex> lock(mutex_);
	return events->getCountExecutedEvents( numEvent );
}

int IscUserEvents::getCountRegisteredNameEvents()
{
	return events->getCount();
}

void IscUserEvents::updateResultEvents( char * result )
{
	// Phase 14.6: With fbcpp::EventListener, event counts are already
	// processed in onEventFired(). The 'result' parameter (raw event buffer)
	// is no longer needed — counts are stored directly in ParameterEvent objects.
	// This method is now a no-op for backward compatibility.
	// Applications should read counts via getCountEvents()/isChanged().
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
