// Gen.cpp: implementation of the Gen class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
//#include "MCET.h"
#include "Gen.h"
#include "LinkedList.h"
#include "Field.h"

#define ISLOWER(c)			(c >= 'a' && c <= 'z')

GenType	genType = GenGeneric;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Gen::Gen()
{

}

Gen::~Gen()
{

}

CString Gen::getName()
{
	return name;
}

CString Gen::genFieldList(LinkedList *list)
{
	CString string;
	char *sep = " (";

	FOR_OBJECTS (Field*, field, list);
		string += sep;
		string += field->name;
		sep = ", ";
	END_FOR;

	string += ")";

	return string;
}

CString Gen::getIdentifier()
{
	CString string;
	char c;

	for (const char *p = name; c = *p++;)
		if ((c >= 'a' && c <= 'z') ||
		    (c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9'))
			string += c;
		else
			string += '_';

	return string;
}

int Gen::hash(const char * string, int tableSize)
{
	int	value = 0, c;

	while (c = (unsigned) *string++)
		{
		if (ISLOWER (c))
			c -= 'a' - 'A';
		value = value * 11 + c;
		}

	if (value < 0)
		value = -value;

	return value % tableSize;
}

int Gen::hash(int tableSize)
{
	return hash (name, tableSize);
}
