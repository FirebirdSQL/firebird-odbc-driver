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

// Parameter.cpp: implementation of the Parameter class.
//
//////////////////////////////////////////////////////////////////////

// copyright (c) 1999 - 2000 by James A. Starkey for IBPhoenix.


//#include "stdafx.h"
#include <memory.h>
#include "Parameter.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Parameter::Parameter(Parameter *nxt, const char *nam, int namLen, const char *val, int valLen)
{
	next = nxt;
	nameLength = namLen;
	name = new char [nameLength + 1];
	memcpy (name, nam, nameLength);
	name [nameLength] = 0;
	valueLength = valLen;
	value = new char [valueLength + 1];
	memcpy (value, val, valueLength);
	value [valueLength] = 0;
}

Parameter::~Parameter()
{
	delete [] name;
	delete [] value;
}
