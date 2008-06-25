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

// Mutex.cpp: implementation of the Mutex class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _WINDOWS
#include <windows.h>
#endif

#include "Mutex.h"

namespace classMutex {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Mutex::Mutex()
{
#ifdef _WINDOWS
	InitializeCriticalSection (&mutex);
#endif

#ifdef _PTHREADS
	int ret = pthread_mutex_init (&mutex, NULL);
#endif

}

Mutex::~Mutex()
{
#ifdef _WINDOWS
	DeleteCriticalSection( &mutex );
#endif

#ifdef _PTHREADS
	int ret = pthread_mutex_destroy (&mutex);
#endif
}

void Mutex::lock()
{
#ifdef _WINDOWS
	EnterCriticalSection (&mutex);
#endif

#ifdef _PTHREADS
	int ret = pthread_mutex_lock (&mutex);
#endif
}

void Mutex::release()
{
#ifdef _WINDOWS
	LeaveCriticalSection (&mutex);
#endif

#ifdef _PTHREADS
	int ret = pthread_mutex_unlock (&mutex);
#endif
}

}; // end namespace classMutex
