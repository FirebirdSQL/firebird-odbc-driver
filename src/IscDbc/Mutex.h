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

// Mutex.h: interface for the Mutex class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_MUTEX_H_INCLUDED_)
#define _MUTEX_H_INCLUDED_

#ifdef _PTHREADS
#include <pthread.h>
#endif

#ifdef _WINDOWS
#include <windows.h>
#endif

namespace classMutex {

class Mutex  
{
public:
	void release();
	void lock();
	Mutex();
	~Mutex();

#ifdef _WINDOWS
	CRITICAL_SECTION mutex;
#endif

#ifdef _PTHREADS
	pthread_mutex_t	mutex;
#endif

};

}; // end namespace classMutex

#endif // !defined(_MUTEX_H_INCLUDED_)
