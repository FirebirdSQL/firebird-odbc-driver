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

// Error.cpp: implementation of the Error class.
//
//////////////////////////////////////////////////////////////////////

// copyright (c) 1999 - 2000 by James A. Starkey for IBPhoenix.

//#include "Engine.h"
#include "Error.h"
#include <stdarg.h>
#include <stdio.h>
#include "SQLError.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Error::Error()
{

}

Error::~Error()
{

}

void Error::error(char * string, ...)
{
	char buffer [256];

	va_list	args;
	va_start (args, string);
	vsprintf (buffer, string, args);
	//printf ("%s\n", buffer);
	throw SQLEXCEPTION (BUG_CHECK, buffer);
}

void Error::assertionFailed(char * fileName, int line)
{
	error ("assertion failed at line %d in file %s", line, fileName);
}
