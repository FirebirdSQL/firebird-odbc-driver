// RString.cpp: implementation of the CRString class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "stdafx.h"
//#include "Map.h"
#include "RString.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRString::CRString()
{
	repeat = NULL;
}

CRString::~CRString()
{
	CRepeat *next;

	while (next = repeat)
		{
		repeat = next->next;
		delete next;
		}
}

CRString::CRString(const char *string) : CString (string)
{
	repeat = NULL;
}

void CRString::replace(const char * pattern, const char * text)
{
	CRString string;
	const char *t, *q, *start = *this, *startOfLine = start;

	for (; *start;)
		{
		// Look for a pattern string or end of source string
		for (const char *p = start; *p; ++p)
			{
			if (*p == '\n')
				startOfLine = p + 1;
			if (*p == pattern [0])
				{
				for (q = p + 1, t = pattern + 1; *t && *t == *q; ++t, ++q)
					;
				if (!*t)
					break;
				}
			}
		// Append everything up to pattern or end to output string
		string.ConcatInPlace (p - start, start);
		if (!*p)
			break;
		// We've got a substitution -- prepare indentation string, if necessary
		CString indent;// (startOfLine, p - startOfLine);
		for (t = startOfLine; t < p; ++t)
			indent += (*t == '\t') ? *t : ' ';
		start = q;
		for (t = text; *t;)
			{
			for (q = t; *q && *q != '\n' && *q != '\r'; ++q)
				;
			if (*q == '\n')
				{
				++q;
				string.ConcatInPlace (q - t, t);
				string += indent;
				}
			else if (*q == '\r')
				{
				++q;
				string.ConcatInPlace (q - t - 1, t);
				string += '\n';
				//string += indent;
				}
			else
				string.ConcatInPlace (q - t, t);
			t = q;
			}
		}

	//*this = (const char*) string;
	CRepeat *rpt = repeat;
	*this = (CString) string;
	repeat = rpt;
}

CRString::CRString(CString string) : CString (string)
{
	repeat = NULL;
}

void CRString::replace(const char * pattern, int value)
{
	char buffer [16];
	sprintf (buffer, "%d", value);
	replace (pattern, buffer);
}

CString CRString::getCRLFstring()
{
	CRString string;
	const char *start = *this;

	for (; *start;)
		{
		for (const char *p = start; *p && *p != '\n'; ++p)
			;
		// Append everything up to pattern or end to output string
		string.ConcatInPlace (p - start, start);
		if (!*p)
			break;
		string += "\r\n";
		start = p + 1;
		}

	return string;
}

CString CRString::getCRstring()
{
	CRString string;
	const char *start = *this;

	for (; *start;)
		{
		for (const char *p = start; *p && !(p [0] == '\r' && p [1] == '\n'); ++p)
			;
		// Append everything up to pattern or end to output string
		string.ConcatInPlace (p - start, start);
		if (!*p)
			break;
		string += '\n';
		start = p + 2;
		}

	return string;
}

const char* CRString::apply()
{
	CRepeat *next;

	while (next = repeat)
		{
		repeat = next->next;
		replace (next->pattern, next->string);
		delete next;
		}

	return *this;
}

CRString::CRepeat::CRepeat(CRepeat *prior, const char *pat, const char *what)
{
	next = prior;
	pattern = pat;
	string = what;
}

void CRString::CRepeat::append(const char *what, const char *separator)
{
	if (string != "")
		string += separator;

	string += what;
}

void CRString::append(const char * pattern, const char * string, const char * separator)
{
	for (CRepeat *next = repeat; next; next = next->next)
		if (next->pattern == pattern)
			{
			next->append (string, separator);
			return;
			}

	repeat = new CRepeat (repeat, pattern, string);
}
