// TypesResultSet.h: interface for the TypesResultSet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TYPESRESULTSET_H__F0866333_9646_11D4_98F5_0000C01D2301__INCLUDED_)
#define AFX_TYPESRESULTSET_H__F0866333_9646_11D4_98F5_0000C01D2301__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IscResultSet.h"

class TypesResultSet : public IscResultSet  
{
public:
	virtual bool isNullable(int index);
	virtual int getPrecision(int index);
	virtual int getScale(int index);
	int getColumnDisplaySize(int index);
	virtual int getColumnType(int index);
	virtual const char* getColumnName(int index);
	virtual bool next();
	TypesResultSet();
	virtual ~TypesResultSet();

	int			recordNumber;
};

#endif // !defined(AFX_TYPESRESULTSET_H__F0866333_9646_11D4_98F5_0000C01D2301__INCLUDED_)
