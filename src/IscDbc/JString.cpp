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

/*
 *	PROGRAM:		Virtual Data Manager
 *	MODULE:			JString.cpp
 *	DESCRIPTION:	Transportable flexible string
 *
 * copyright (c) 1997 - 2000 by James A. Starkey for IBPhoenix.
 */

#include <memory.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "JString.h"
//#include "WString.h"


#define ISLOWER(c)			(c >= 'a' && c <= 'z')
#define UPPER(c)			((ISLOWER (c)) ? c - 'a' + 'A' : c)

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

namespace classJString {


JString::JString ()
{
/**************************************
 *
 *		J S t r i n g
 *
 **************************************
 *
 * Functional description
 *		Initialize string object.
 *
 **************************************/

string = NULL;
}

JString::JString (const char *stuff)
{
/**************************************
 *
 *		J S t r i n g
 *
 **************************************
 *
 * Functional description
 *		Initialize string object.
 *
 **************************************/

string = NULL;
setString (stuff);
}

JString::JString (const JString& source)
{
/**************************************
 *
 *		J S t r i n g
 *
 **************************************
 *
 * Functional description
 *		Copy constructor.
 *
 **************************************/

if ((string = source.string))
	++(string [-1]);
}

JString::~JString ()
{
/**************************************
 *
 *		~ J S t r i n g
 *
 **************************************
 *
 * Functional description
 *		Initialize string object.
 *
 **************************************/

release();
}

void JString::append (const char* stuff)
{
/**************************************
 *
 *		a p p e n d
 *
 **************************************
 *
 * Functional description
 *		Append string.
 *
 **************************************/

if (!string)
	{
	setString (stuff);
	return;
	}

int l1 = (int)strlen (string);
int	l2 = (int)strlen (stuff);
char *temp = new char [l1 + l2 + 2];
*temp++ = 1;

memcpy (temp, string, l1);
memcpy (temp + l1, stuff, l2);
temp [l1 + l2] = 0;
release();
string = temp;
}

void JString::setString (const char* stuff)
{
/**************************************
 *
 *		s e t S t r i n g
 *
 **************************************
 *
 * Functional description
 *		Append string.
 *
 **************************************/

//release();

if (stuff)
	setString (stuff, (int)strlen (stuff));
else
	release();
}

void JString::Format (const char* stuff, ...)
{
/**************************************
 *
 *		f o r m a t
 *
 **************************************
 *
 * Functional description
 *		Append string.
 *
 **************************************/
va_list	args;
va_start (args, stuff);
char	temp [1024];

vsprintf (temp, stuff, args);
setString (temp);
}

const char* JString::getString()
{
/**************************************
 *
 *		g e t S t r i n g
 *
 **************************************
 *
 * Functional description
 *		Append string.
 *
 **************************************/

return (string) ? string : "";
}

JString::operator const char* ()
{
/**************************************
 *
 *		o p e r a t o r   c h a r *
 *
 **************************************
 *
 * Functional description
 *		Return string as string.
 *
 **************************************/

return (string) ? string : "";
}

JString& JString::operator = (const char *stuff)
{
/**************************************
 *
 *		o p e r a t o r   c h a r =
 *
 **************************************
 *
 * Functional description
 *		Return string as string.
 *
 **************************************/

setString (stuff);

return *this;
}

JString& JString::operator = (const JString& source)
{
/**************************************
 *
 *		o p e r a t o r   c h a r =
 *
 **************************************
 *
 * Functional description
 *		Return string as string.
 *
 **************************************/

//assign (source.string);
release();

if ((string = source.string))
    ++(string [-1]);

return *this;
}

JString& JString::operator+= (const char *stuff)
{
/**************************************
 *
 *		o p e r a t o r   c h a r + =
 *
 **************************************
 *
 * Functional description
 *		Return string as string.
 *
 **************************************/

append (stuff);

return *this;
}

JString& JString::operator+= (const JString& string)
{
/**************************************
 *
 *		o p e r a t o r   c h a r + =
 *
 **************************************
 *
 * Functional description
 *		Return string as string.
 *
 **************************************/

append (string.string);

return *this;
}

JString operator + (const JString& string1, const char *string2)
{
/**************************************
 *
 *		o p e r a t o r   c h a r +
 *
 **************************************
 *
 * Functional description
 *		Return string as string.
 *
 **************************************/
JString	s = string1;

s.append (string2);

return s;
}

void JString::release ()
{
/**************************************
 *
 *		r e s e t
 *
 **************************************
 *
 * Functional description
 *		Clean out string.
 *
 **************************************/

if (!string)
	return;

--string;

if (--string [0] == 0)
	delete [] string;

string = NULL;
}

bool JString::operator ==(const char * stuff)
{
	if (string)
		return strcmp (string, stuff) == 0;

	return strcmp ("", stuff) == 0;
}

bool JString::operator !=(const char * stuff)
{
	if (string)
		return strcmp (string, stuff) != 0;

	return strcmp ("", stuff) != 0;
}

/***
bool JString::operator !=(const char * string)
{
	return strcmp (start, string) != 0;
}
***/

/***
void JString::set(int length, const char * stuff)
{
	release();
	string = new char [length + 2];
	*string++ = 1;
	strncpy (string, stuff, length);
	string [length] = 0;
}
***/

JString JString::before(char c)
{
	const char *p;

	for (p = string; *p && *p != c;)
		++p;

	if (!*p)
		return *this;

	JString stuff;
	stuff.setString (string, (int)(p - string));

	return stuff;
}

const char* JString::after(char c)
{
	const char *p;

	for (p = string; *p && *p++ != c;)
		;

	return p;
}

bool JString::IsEmpty()
{
return !string || !string [0];
}

int JString::hash(const char * string, int tableSize)
{
	int	value = 0, c;

	while ((c = (unsigned) *string++))
		{
		if (ISLOWER (c))
			c -= 'a' - 'A';
		value = value * 11 + c;
		}

	if (value < 0)
		value = -value;

	return value % tableSize;
}

int JString::hash(int tableSize)
{
	if (!string)
		return 0;

	return hash (string, tableSize);
}

/***
JString::JString(const WCHAR * wString, int len)
{
	string = NULL;
	setString (wString, len);
}

void JString::setString(const WCHAR * wString, int len)
{
	release();
	string = new char [len + 2];
	*string++ = 1;

	for (int n = 0; n < len; ++n)
		string [n] = (char) wString [n];

	string [len] = 0;
}

JString::JString(const WCHAR * wString)
{
	string = NULL;
	setString (wString, WString::length (wString));
}

JString& JString::operator =(const WCHAR * wString)
{
	setString (wString, WString::length (wString));

	return *this;
}
***/

void JString::setString(const char * source, int length)
{
	release();
	string = new char [length + 2];
	*string++ = 1;
	memcpy (string, source, length);
	string [length] = 0;
}

int JString::findSubstring(const char * string, const char * sub)
{
    for (const char *p = string; *p; ++p)
		{
		const char *s, *q;
		for (q = p, s = sub; *s && *q == *s; ++s, ++q)
			;
		if (!*s)
			return (int)(p - string);
		}

	return -1;
}

JString JString::upcase(const char * source)
{
	JString string;
	int len = (int)strlen (source);
	string.alloc (len);
	
	for (int n = 0; n < len; ++n)
		{
		char c = source [n];
		string.string [n] = UPPER (c);
		}

	return string;
}

void JString::alloc(int length)
{
	release();
	string = new char [length + 2];
	*string++ = 1;
	string [length] = 0;
}

bool JString::equalsNoCase(const char * string2)
{
	if (!string)
		return string2 [0] == 0;

	const char *p;

	for (p = string; *p && *string2; ++p, ++string2)
		if (UPPER (*p) != UPPER (*string2))
			return false;

	return *p == *string2;
}

int JString::length()
{
	if (!string)
		return 0;

	const char *p;

	for (p = string; *p; ++p)
		;

	return (int)(p - string);
}

JString::JString(const char * source, int length)
{
	string = NULL;
	setString (source, length);
}

char* JString::getBuffer(int length)
{
	alloc (length);

	return string;
}

void JString::releaseBuffer()
{

}

}; // end namespace classJString
