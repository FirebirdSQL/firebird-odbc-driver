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

// IscUserEvents.h interface for the user events class.
// Phase 9.4: Migrated from ISC isc_que_events to OO API IAttachment::queEvents.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_UserEvents_H_)
#define _UserEvents_H_

#include <firebird/IdlFbInterfaces.h>

namespace IscDbcLibrary {

class ParametersEvents;

/// OO API event callback bridge (Phase 9.4).
/// Implements IEventCallback to bridge OO API event notifications
/// to the legacy callbackEvent function pointer.
/// Ref-counted lifetime is managed by the owning IscUserEvents.
class FbEventCallback final : public Firebird::IEventCallbackImpl<FbEventCallback, Firebird::ThrowStatusWrapper>
{
public:
	FbEventCallback() : owner_(nullptr) {}
	void setOwner(class IscUserEvents* owner) { owner_ = owner; }

	/// Called by Firebird when events fire.
	void eventCallbackFunction(unsigned length, const unsigned char* events);

	/// IReferenceCounted — prevent premature disposal (owned by IscUserEvents).
	void addRef() {}
	int release() { return 1; }

private:
	class IscUserEvents* owner_;
};

class IscUserEvents : public UserEvents
{
public:

	IscUserEvents( IscConnection *connect, PropertiesEvents *context, callbackEvent astRoutine, void *userAppData = NULL );
	~IscUserEvents();

	void				releaseEventBlock();
	void				initEventBlock();
	inline unsigned long vaxInteger( const unsigned char * val );
	void				eventCounts( const unsigned char *result );

	virtual void		queEvents( void * interfase = NULL );
	virtual bool		isChanged( int numEvent = 0 );
	virtual unsigned long getCountEvents( int numEvent = 0 );
	virtual int			getCountRegisteredNameEvents();
	virtual void		updateResultEvents( char * result );
	virtual void		*getUserData();
	virtual void		addRef();
	virtual int			release();
	virtual int			objectVersion();

private:
	
	int					useCount;
	IscConnection		*connection;
	unsigned char		*eventBuffer;
	short				lengthEventBlock;
	Firebird::IEvents	*eventsHandle;	///< OO API events handle (Phase 9.4)
	FbEventCallback		callback_;		///< OO API callback bridge (Phase 9.4)

public:
	
	ParametersEvents	*events;
	callbackEvent		callbackAstRoutine;
	void				*userData;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_UserEvents_H_)
