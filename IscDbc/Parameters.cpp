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

// Parameters.cpp: implementation of the Parameters class.
//
//////////////////////////////////////////////////////////////////////

// copyright (c) 1999 - 2000 by James A. Starkey for IBPhoenix.


#include <string.h>
#include "IscDbc.h"
#include "Parameters.h"
#include "Parameter.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Parameters::Parameters()
{
	parameters = NULL;
	count = 0;
}

Parameters::~Parameters()
{
	clear();
}

void Parameters::putValue(const char * name, const char * value)
{
	putValue (name, (int)strlen (name), value, (int)strlen (value));
}

void Parameters::putValue(const char * name, int nameLength, const char * value, int valueLength)
{
	++count;
	parameters = new Parameter (parameters, name, nameLength, value, valueLength);
}

const char* Parameters::findValue(const char * name, const char *defaultValue)
{
	for (Parameter *parameter = parameters; parameter; parameter = parameter->next)
		if (!strcasecmp (name, parameter->name))
			return parameter->value;

	return defaultValue;
}

int Parameters::getCount()
{
	return count;
}

const char* Parameters::getName(int index)
{
	Parameter *parameter = parameters;

	for (int n = 0; n < count; ++n, parameter = parameter->next)
		if (n == index)
			return parameter->name;

	return NULL;
}


const char* Parameters::getValue(int index)
{
	Parameter *parameter = parameters;

	for (int n = 0; n < count; ++n, parameter = parameter->next)
		if (n == index)
			return parameter->value;

	return NULL;
}

void Parameters::copy(Properties * properties)
{
	int count = properties->getCount();

	for (int n = 0; n < count; ++n)
		putValue (properties->getName (n), properties->getValue (n));
}

void Parameters::clear()
{
	for (Parameter *parameter; parameter = parameters;)
	{
		parameters = parameter->next;
		delete parameter;
	}
}

void Parameters::release()
{
	delete this;
}

}; // end namespace IscDbcLibrary
