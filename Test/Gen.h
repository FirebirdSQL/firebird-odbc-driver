// Gen.h: interface for the Gen class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GEN_H__CBD41C49_EAEA_11D3_98D6_0000C01D2301__INCLUDED_)
#define AFX_GEN_H__CBD41C49_EAEA_11D3_98D6_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "RString.h"

class Syntax;
class Hash;
class LinkedList;
class Table;

typedef enum {
	GenGeneric,
    GenOracle7,
	GenOracle8,
	GenSqlServer
	} GenType;

struct GenTarget {
    char	*name;
	GenType	type;
	};


extern GenType	genType;


class Gen  
{
public:
	virtual int hash (int tableSize);
	static int hash (const char *string, int tableSize);
	CString getIdentifier();
	CString genFieldList (LinkedList *linkedList);
	virtual CString getName();
	CString name;
	CString comment;
	Gen();
	virtual ~Gen();

};

#endif // !defined(AFX_GEN_H__CBD41C49_EAEA_11D3_98D6_0000C01D2301__INCLUDED_)
