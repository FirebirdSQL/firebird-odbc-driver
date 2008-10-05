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
//
//////////////////////////////////////////////////////////////////////

#if !defined(_UserEvents_H_)
#define _UserEvents_H_

namespace IscDbcLibrary {

class ParametersEvents;

class IscUserEvents : public UserEvents
{
public:

	IscUserEvents( IscConnection *connect, PropertiesEvents *context, callbackEvent astRoutine, void *userAppData = NULL );
	~IscUserEvents();

	void				releaseEventBlock();
	void				initEventBlock();
	inline unsigned long vaxInteger( char * val );
	void				eventCounts( char *result );

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
	char				*eventBuffer;
	ISC_LONG			eventId;
	short				lengthEventBlock;

public:
	
	ParametersEvents	*events;
	callbackEvent		callbackAstRoutine;
	void				*userData;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_UserEvents_H_)
