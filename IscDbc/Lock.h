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

// Lock.h: interface for the Lock class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_LOCK_H_INCLUDED_)
#define _LOCK_H_INCLUDED_

class Mutex;

class Lock  
{
public:
	void release();
	void lock();
	Lock(Mutex *mutex);
	virtual ~Lock();

	Mutex	*mutex;
	bool	locked;
};

#endif // !defined(_LOCK_H_INCLUDED_)
