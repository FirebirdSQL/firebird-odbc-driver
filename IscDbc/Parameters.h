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

// Parameters.h: interface for the Parameters class.
//
//////////////////////////////////////////////////////////////////////

// copyright (c) 1999 - 2000 by James A. Starkey for IBPhoenix.


#if !defined(AFX_PARAMETERS_H__13461881_1D25_11D4_98DF_0000C01D2301__INCLUDED_)
#define AFX_PARAMETERS_H__13461881_1D25_11D4_98DF_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Properties.h"

namespace IscDbcLibrary {

class Parameter;

class Parameters : public Properties
{
public:
	void clear();
	void copy (Properties *properties);
	virtual const char* getValue (int index);
	virtual const char* getName (int index);
	virtual int getCount();
	virtual const char* findValue (const char *name, const char *defaultValue);
	virtual void putValue(const char * name, int nameLength, const char * value, int valueLength);
	virtual void putValue(const char * name, const char * value);
	Parameters();
	~Parameters();

	Parameter	*parameters;
	int			count;
};

}; // end namespace IscDbcLibrary

#endif // !defined(AFX_PARAMETERS_H__13461881_1D25_11D4_98DF_0000C01D2301__INCLUDED_)
