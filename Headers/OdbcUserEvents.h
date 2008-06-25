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

// OdbcUserEvents.h - This is the include for applications using
//                    the Firebird ODBC Extensions
//
//////////////////////////////////////////////////////////////////////

#if !defined(_OdbcUserEvents_h_)
#define _OdbcUserEvents_h_

typedef struct tagODBC_EVENT_INFO
{
	char			*nameEvent;
	unsigned long	countEvents;
	bool			changed;

} ODBC_EVENT_INFO, *PODBC_EVENT_INFO;

#define INIT_ODBC_EVENT(name) name, 0lu, false
#define COUNT_ODBC_EVENT(name) sizeof(name)/sizeof(*name)

typedef void (*callbackAstProc)( void *userEventsInterfase, short length, char *updated );

typedef struct tagODBC_EVENTS_BLOCK_INFO
{
	PODBC_EVENT_INFO	events;
	int					count;
	void				*userData;
	void				*hdbc;
	callbackAstProc		lpAstProc;

} ODBC_EVENTS_BLOCK_INFO, *PODBC_EVENTS_BLOCK_INFO;

#define INIT_EVENTS_BLOCK_INFO(hdbc,name,astProc,userData) \
				{ (PODBC_EVENT_INFO)name, COUNT_ODBC_EVENT(name), userData, hdbc, astProc }

#define SQL_FB_INIT_EVENTS				201
#define SQL_FB_UPDATECOUNT_EVENTS		202
#define SQL_FB_REQUEUE_EVENTS			203

typedef struct tagODBC_USER_EVENTS_INTERFASE
{
	void				*userData;
	void				*hdbc;
	PODBC_EVENT_INFO	events;
	int					count;

} ODBC_USER_EVENTS_INTERFASE, *PODBC_USER_EVENTS_INTERFASE;

#define SIZE_OF_EVENT_BLOCK_INFO sizeof( EVENT_BLOCK_INFO )

#endif // !defined(_OdbcUserEvents_h_)
