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

// Values.cpp: implementation of the Values class.
//
//////////////////////////////////////////////////////////////////////

// copyright (c) 1999 - 2000 by James A. Starkey for IBPhoenix.


#include "IscDbc.h"
#include "Values.h"
#include "Value.h"


#ifdef _DEBUG
static char THIS_FILE[]=__FILE__;
#endif

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Values::Values()
{
	count = 0;
	values = NULL;
}

Values::~Values()
{
	if (values)
		delete [] values;
}

void Values::alloc(int number)
{
	if (number == count)
		{
		for (int n = 0; n < count; ++n)
			values [n].clear();
		return;
		}

	if (values)
		delete [] values;

	count = number;
	values = new Value [count];
}

void Values::clear()
{
	for (int n = 0; n < count; ++n)
		values [n].clear();
}

}; // end namespace IscDbcLibrary
