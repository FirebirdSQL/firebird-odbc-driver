/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
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

#ifdef _WIN32
#include <windows.h>
#endif

#include "Mutex.h"

namespace classMutex {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Mutex::Mutex()
{
#ifdef _WIN32
	mutex = CreateMutex (NULL, false, NULL);
#endif

#ifdef _PTHREADS
	int ret = pthread_mutex_init (&mutex, NULL);
#endif

}

Mutex::~Mutex()
{
#ifdef _WIN32
	CloseHandle (mutex);
#endif

#ifdef _PTHREADS
	int ret = pthread_mutex_destroy (&mutex);
#endif
}

void Mutex::lock()
{
#ifdef _WIN32
	int result = WaitForSingleObject (mutex, INFINITE);
#endif

#ifdef _PTHREADS
	int ret = pthread_mutex_lock (&mutex);
#endif
}

void Mutex::release()
{
#ifdef _WIN32
	ReleaseMutex (mutex);
#endif

#ifdef _PTHREADS
	int ret = pthread_mutex_unlock (&mutex);
#endif
}

}; // end namespace classMutex
